#include "monitormanagerimpl.h"
#include <algorithm>
#include <qdebug.h>
#include <qlogging.h>
#include <vector>

MonitorManagerImpl::MonitorManagerImpl()
    : messageWindow(nullptr)
    , pWMIService(nullptr)
    , m_nightLightRegKey(nullptr)
{
    qDebug() << "MonitorManagerImpl constructor start";

    // Initialize COM for this specific thread with consistent apartment model
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        qDebug() << "COM initialization failed:" << QString::number(hr, 16);
        // Try with multithreaded model as fallback
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            qDebug() << "COM initialization failed completely:" << QString::number(hr, 16);
        }
    }

    // Initialize components
    initNightLightRegistry();
    enumerateMonitors();
    setupChangeDetection();

    qDebug() << "MonitorManagerImpl constructor end";
}

MonitorManagerImpl::~MonitorManagerImpl() {
    cleanup();
    cleanupNightLightRegistry();
    cleanupWMI();
    CoUninitialize();
}

bool MonitorManagerImpl::ensureWMIConnection() {
    std::lock_guard<std::mutex> lock(m_wmiMutex);

    auto now = std::chrono::steady_clock::now();

    // If WMI was recently initialized and passes health check, keep it
    if (pWMIService &&
        m_wmiInitialized.load() &&
        (now - m_lastWMIInit) < WMI_CACHE_DURATION &&
        quickWMIHealthCheck()) {
        return true;
    }

    qDebug() << "WMI connection needs (re)initialization";

    // Clean up existing connection
    if (pWMIService) {
        pWMIService->Release();
        pWMIService = nullptr;
    }

    // Initialize fresh connection
    initializeWMI();
    m_lastWMIInit = now;
    m_wmiInitialized.store(pWMIService != nullptr);

    return pWMIService != nullptr;
}

bool MonitorManagerImpl::quickWMIHealthCheck() {
    if (!pWMIService) return false;

    // Quick health check without debug spam
    IEnumWbemClassObject* pTest = nullptr;
    HRESULT testResult = pWMIService->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_ComputerSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pTest);

    bool healthy = SUCCEEDED(testResult);
    if (pTest) pTest->Release();

    if (!healthy) {
        qDebug() << "WMI health check failed:" << QString::number(testResult, 16);
    }

    return healthy;
}

void MonitorManagerImpl::initializeWMI() {
    qDebug() << "WMI initialization start";

    // Initialize security
    HRESULT hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_NONE,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
        );

    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        qDebug() << "CoInitializeSecurity failed:" << QString::number(hres, 16);
    }

    // Create WMI locator
    IWbemLocator* pLoc = nullptr;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        qDebug() << "Failed to create WMI locator:" << QString::number(hres, 16);
        return;
    }

    // Connect to WMI namespace
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\WMI"), NULL, NULL, 0, NULL, 0, 0, &pWMIService);
    if (FAILED(hres)) {
        qDebug() << "ConnectServer failed:" << QString::number(hres, 16);
        pLoc->Release();
        return;
    }

    // Set proxy blanket
    hres = CoSetProxyBlanket(pWMIService, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                             RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        qDebug() << "CoSetProxyBlanket failed:" << QString::number(hres, 16);
        pWMIService->Release();
        pWMIService = nullptr;
    }

    pLoc->Release();
    qDebug() << "WMI initialization" << (pWMIService ? "successful" : "failed");
}

void MonitorManagerImpl::cleanupWMI() {
    std::lock_guard<std::mutex> lock(m_wmiMutex);
    if (pWMIService) {
        pWMIService->Release();
        pWMIService = nullptr;
    }
    m_wmiInitialized.store(false);
}

void MonitorManagerImpl::enumerateMonitors() {
    cleanup();
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)this);
    detectLaptopDisplays();
}

std::wstring MonitorManagerImpl::getMonitorName(int index) const {
    if (index < 0 || index >= monitors.size()) return L"";

    if (monitors[index].isLaptopDisplay) {
        return L"Laptop Internal Display";
    }
    return std::wstring(monitors[index].physicalMonitor.szPhysicalMonitorDescription);
}

bool MonitorManagerImpl::setBrightnessInternal(int monitorIndex, int brightness) {
    if (monitorIndex < 0 || monitorIndex >= monitors.size()) return false;
    if (brightness < 0 || brightness > 100) return false;

    if (monitors[monitorIndex].isLaptopDisplay) {
        return setLaptopBrightness(brightness);
    } else {
        bool result = SetVCPFeature(monitors[monitorIndex].physicalMonitor.hPhysicalMonitor, 0x10, brightness);
        if (result) {
            monitors[monitorIndex].cachedBrightness = brightness;
        }
        return result;
    }
}

bool MonitorManagerImpl::setBrightnessAll(int brightness) {
    if (brightness < 0 || brightness > 100) return false;

    bool allSuccess = true;
    for (int i = 0; i < getMonitorCount(); i++) {
        if (monitors[i].isLaptopDisplay || testDDCCI(i)) {
            bool success = setBrightnessInternal(i, brightness);
            if (!success) allSuccess = false;
        }
    }
    return allSuccess;
}

int MonitorManagerImpl::getBrightnessInternal(int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= monitors.size()) return -1;

    if (monitors[monitorIndex].isLaptopDisplay) {
        return getLaptopBrightness();
    }

    DWORD current, max;
    if (GetVCPFeatureAndVCPFeatureReply(monitors[monitorIndex].physicalMonitor.hPhysicalMonitor, 0x10, NULL, &current, &max)) {
        monitors[monitorIndex].ddcciWorking = true;
        monitors[monitorIndex].cachedBrightness = (int)current;
        return (int)current;
    }

    monitors[monitorIndex].ddcciWorking = false;
    return -1;
}

bool MonitorManagerImpl::testDDCCI(int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= monitors.size()) return false;

    if (monitors[monitorIndex].isLaptopDisplay) {
        return true;
    }

    if (monitors[monitorIndex].ddcciTested) {
        return monitors[monitorIndex].ddcciWorking;
    }

    monitors[monitorIndex].ddcciTested = true;
    return getBrightnessInternal(monitorIndex) != -1;
}

int MonitorManagerImpl::getCachedBrightness(int monitorIndex) {
    if (monitorIndex < 0 || monitorIndex >= monitors.size()) return -1;
    return monitors[monitorIndex].cachedBrightness;
}

void MonitorManagerImpl::setChangeCallback(std::function<void()> callback) {
    changeCallback = callback;
}

void MonitorManagerImpl::detectLaptopDisplays() {
    qDebug() << "Detecting laptop displays";

    bool wmiHasBrightnessSupport = false;

    if (ensureWMIConnection()) {
        // Check for WMI brightness methods
        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hres = pWMIService->ExecQuery(
            bstr_t("WQL"),
            bstr_t("SELECT * FROM WmiMonitorBrightnessMethods"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL, &pEnumerator);

        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (uReturn > 0) {
                wmiHasBrightnessSupport = true;
                qDebug() << "WMI brightness control available";
                if (pclsObj) pclsObj->Release();
            }
            pEnumerator->Release();
        }
    }

    if (wmiHasBrightnessSupport) {
        qDebug() << "WMI brightness detected - ensuring laptop display exists";

        // Check if we already have a laptop display
        bool hasLaptopDisplay = false;
        for (const auto& monitor : monitors) {
            if (monitor.isLaptopDisplay) {
                hasLaptopDisplay = true;
                break;
            }
        }

        // If no laptop display exists, create one
        if (!hasLaptopDisplay) {
            qDebug() << "Creating virtual laptop display";
            MonitorInfo laptopMonitor = {};
            laptopMonitor.physicalMonitor.hPhysicalMonitor = NULL;
            wcscpy_s(laptopMonitor.physicalMonitor.szPhysicalMonitorDescription,
                     PHYSICAL_MONITOR_DESCRIPTION_SIZE, L"Laptop Internal Display");
            laptopMonitor.deviceName = L"LAPTOP";
            laptopMonitor.ddcciTested = true;
            laptopMonitor.ddcciWorking = false;
            laptopMonitor.isLaptopDisplay = true;
            laptopMonitor.cachedBrightness = getLaptopBrightness();

            monitors.insert(monitors.begin(), laptopMonitor);
            qDebug() << "Added laptop display to monitor list";
        } else {
            // Update existing laptop display brightness
            for (auto& monitor : monitors) {
                if (monitor.isLaptopDisplay) {
                    monitor.cachedBrightness = getLaptopBrightness();
                    break;
                }
            }
        }
    }

    qDebug() << "Total monitors after laptop detection:" << monitors.size();
    for (size_t i = 0; i < monitors.size(); i++) {
        qDebug() << "Monitor" << i << ":"
                 << QString::fromStdWString(monitors[i].physicalMonitor.szPhysicalMonitorDescription)
                 << "Laptop:" << monitors[i].isLaptopDisplay
                 << "DDC/CI:" << monitors[i].ddcciWorking
                 << "Cached brightness:" << monitors[i].cachedBrightness;
    }
}

bool MonitorManagerImpl::setLaptopBrightness(int brightness) {
    qDebug() << "Setting laptop brightness to" << brightness;

    if (!ensureWMIConnection()) {
        qDebug() << "WMI connection unavailable";
        return false;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    HRESULT hres = pWMIService->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM WmiMonitorBrightnessMethods"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator);

    if (FAILED(hres)) {
        qDebug() << "WMI brightness methods query failed:" << QString::number(hres, 16);
        return false;
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    bool success = false;

    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        // Get the method class
        IWbemClassObject* pClass = NULL;
        hres = pWMIService->GetObject(bstr_t(L"WmiMonitorBrightnessMethods"), 0, NULL, &pClass, NULL);
        if (SUCCEEDED(hres)) {
            IWbemClassObject* pInParamsDefinition = NULL;
            hres = pClass->GetMethod(L"WmiSetBrightness", 0, &pInParamsDefinition, NULL);
            if (SUCCEEDED(hres)) {
                IWbemClassObject* pClassInstance = NULL;
                hres = pInParamsDefinition->SpawnInstance(0, &pClassInstance);

                if (SUCCEEDED(hres)) {
                    // Set parameters
                    VARIANT varBrightness, varTimeout;
                    VariantInit(&varBrightness);
                    VariantInit(&varTimeout);

                    varBrightness.vt = VT_UI1;
                    varBrightness.bVal = (BYTE)brightness;

                    varTimeout.vt = VT_I4;
                    varTimeout.lVal = 0;

                    HRESULT putBrightness = pClassInstance->Put(L"Brightness", 0, &varBrightness, 0);
                    HRESULT putTimeout = pClassInstance->Put(L"Timeout", 0, &varTimeout, 0);

                    if (SUCCEEDED(putBrightness)) {
                        // Execute method
                        VARIANT vtProp;
                        VariantInit(&vtProp);
                        HRESULT pathResult = pclsObj->Get(L"__PATH", 0, &vtProp, 0, 0);

                        if (SUCCEEDED(pathResult) && vtProp.vt == VT_BSTR) {
                            IWbemClassObject* pOutParams = NULL;
                            hres = pWMIService->ExecMethod(vtProp.bstrVal, _bstr_t(L"WmiSetBrightness"),
                                                           0, NULL, pClassInstance, &pOutParams, NULL);

                            if (SUCCEEDED(hres)) {
                                success = true;
                                qDebug() << "Successfully set laptop brightness to" << brightness;
                            } else {
                                qDebug() << "ExecMethod failed:" << QString::number(hres, 16);
                            }

                            if (pOutParams) pOutParams->Release();
                        }
                        VariantClear(&vtProp);
                    }

                    VariantClear(&varBrightness);
                    VariantClear(&varTimeout);
                    pClassInstance->Release();
                }
                pInParamsDefinition->Release();
            }
            pClass->Release();
        }

        pclsObj->Release();
        break;
    }

    if (pEnumerator) pEnumerator->Release();
    return success;
}

int MonitorManagerImpl::getLaptopBrightness() {
    if (!ensureWMIConnection()) {
        return -1;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    HRESULT hres = pWMIService->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM WmiMonitorBrightness"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator);

    if (FAILED(hres)) return -1;

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    int brightness = -1;

    if (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn != 0) {
            VARIANT vtProp;
            hr = pclsObj->Get(L"CurrentBrightness", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr)) {
                brightness = vtProp.bVal;
            }
            VariantClear(&vtProp);
            pclsObj->Release();
        }
        pEnumerator->Release();
    }

    return brightness;
}

BOOL CALLBACK MonitorManagerImpl::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MonitorManagerImpl* manager = (MonitorManagerImpl*)dwData;

    MONITORINFOEXW monitorInfo = {};
    monitorInfo.cbSize = sizeof(MONITORINFOEXW);
    GetMonitorInfoW(hMonitor, &monitorInfo);

    DWORD numPhysical;
    if (GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &numPhysical)) {
        PHYSICAL_MONITOR* physMonitors = new PHYSICAL_MONITOR[numPhysical];
        if (GetPhysicalMonitorsFromHMONITOR(hMonitor, numPhysical, physMonitors)) {
            for (DWORD i = 0; i < numPhysical; i++) {
                MonitorInfo info = {};
                info.physicalMonitor = physMonitors[i];
                info.deviceName = monitorInfo.szDevice;
                info.ddcciTested = false;
                info.ddcciWorking = false;
                info.isLaptopDisplay = false;
                info.cachedBrightness = -1;

                manager->monitors.push_back(info);
            }
        }
        delete[] physMonitors;
    }
    return TRUE;
}

void MonitorManagerImpl::setupChangeDetection() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"MonitorChangeDetector";
    RegisterClassW(&wc);

    messageWindow = CreateWindowW(
        L"MonitorChangeDetector", L"", 0, 0, 0, 0, 0,
        HWND_MESSAGE, NULL, GetModuleHandle(NULL), this);
    SetWindowLongPtr(messageWindow, GWLP_USERDATA, (LONG_PTR)this);
}

LRESULT CALLBACK MonitorManagerImpl::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DISPLAYCHANGE) {
        MonitorManagerImpl* manager = (MonitorManagerImpl*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (manager) {
            manager->enumerateMonitors();
            if (manager->changeCallback) {
                manager->changeCallback();
            }
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void MonitorManagerImpl::cleanup() {
    for (auto& monitor : monitors) {
        if (!monitor.isLaptopDisplay) {
            DestroyPhysicalMonitor(monitor.physicalMonitor.hPhysicalMonitor);
        }
    }
    monitors.clear();

    if (messageWindow) {
        DestroyWindow(messageWindow);
        messageWindow = nullptr;
    }
}

// Night Light implementation
void MonitorManagerImpl::initNightLightRegistry() {
    const std::wstring keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Current\\default$windows.data.bluelightreduction.bluelightreductionstate\\windows.data.bluelightreduction.bluelightreductionstate";
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ | KEY_WRITE, &m_nightLightRegKey);
    if (result != ERROR_SUCCESS) {
        m_nightLightRegKey = nullptr;
    }
}

void MonitorManagerImpl::cleanupNightLightRegistry() {
    if (m_nightLightRegKey) {
        RegCloseKey(m_nightLightRegKey);
        m_nightLightRegKey = nullptr;
    }
}

bool MonitorManagerImpl::isNightLightSupported() {
    return m_nightLightRegKey != nullptr;
}

bool MonitorManagerImpl::isNightLightEnabled() {
    if (!isNightLightSupported()) return false;

    BYTE data[1024];
    DWORD dataSize = sizeof(data);

    if (RegQueryValueEx(m_nightLightRegKey, L"Data", nullptr, nullptr, data, &dataSize) != ERROR_SUCCESS) {
        return false;
    }

    std::vector<BYTE> bytes(data, data + dataSize);
    if (bytes.size() < 19) return false;

    return bytes[18] == 0x15;
}

void MonitorManagerImpl::enableNightLight() {
    if (isNightLightSupported() && !isNightLightEnabled()) {
        toggleNightLight();
    }
}

void MonitorManagerImpl::disableNightLight() {
    if (isNightLightSupported() && isNightLightEnabled()) {
        toggleNightLight();
    }
}

void MonitorManagerImpl::toggleNightLight() {
    if (!isNightLightSupported()) return;

    BYTE data[1024];
    DWORD dataSize = sizeof(data);

    if (RegQueryValueEx(m_nightLightRegKey, L"Data", nullptr, nullptr, data, &dataSize) != ERROR_SUCCESS) {
        return;
    }

    std::vector<BYTE> newData;
    bool currentlyEnabled = isNightLightEnabled();

    if (currentlyEnabled) {
        newData.resize(41, 0);
        std::copy(data, data + 22, newData.begin());
        std::copy(data + 25, data + 43, newData.begin() + 23);
        newData[18] = 0x13;
    } else {
        newData.resize(43, 0);
        std::copy(data, data + 22, newData.begin());
        std::copy(data + 23, data + 41, newData.begin() + 25);
        newData[18] = 0x15;
        newData[23] = 0x10;
        newData[24] = 0x00;
    }

    for (int i = 10; i < 15; ++i) {
        if (newData[i] != 0xff) {
            newData[i]++;
            break;
        }
    }

    DWORD newDataSize = static_cast<DWORD>(newData.size());
    RegSetValueEx(m_nightLightRegKey, L"Data", 0, REG_BINARY, newData.data(), newDataSize);
}
