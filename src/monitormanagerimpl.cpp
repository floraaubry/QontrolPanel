#include "monitormanagerimpl.h"
#include <algorithm>
#include <vector>

MonitorManagerImpl::MonitorManagerImpl()
    : messageWindow(nullptr)
    , pWMIService(nullptr)
    , m_nightLightRegKey(nullptr)
{
    initializeWMI();
    initNightLightRegistry();
    enumerateMonitors();
    detectLaptopDisplays();
    setupChangeDetection();
}

MonitorManagerImpl::~MonitorManagerImpl() {
    cleanup();
    cleanupNightLightRegistry();

    if (pWMIService) {
        pWMIService->Release();
        pWMIService = nullptr;
    }

    CoUninitialize();
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
            printf("Setting monitor %d (%s) brightness to %d... ",
                   i,
                   monitors[i].isLaptopDisplay ? "Laptop" : "External",
                   brightness);
            bool success = setBrightnessInternal(i, brightness);
            printf("%s\n", success ? "OK" : "Failed");
            if (!success) allSuccess = false;
        } else {
            printf("Skipping monitor %d (DDC/CI not working)\n", i);
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
        return true; // Laptop displays use WMI, always "working" if WMI is available
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

void MonitorManagerImpl::initializeWMI() {
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return;

    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) return;

    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\WMI"), NULL, NULL, 0, NULL, 0, 0, &pWMIService);
    if (FAILED(hres)) {
        pLoc->Release();
        return;
    }

    hres = CoSetProxyBlanket(pWMIService, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    pLoc->Release();
}

void MonitorManagerImpl::detectLaptopDisplays() {
    // Mark any monitors that might be laptop displays
    for (auto& monitor : monitors) {
        std::wstring name = monitor.physicalMonitor.szPhysicalMonitorDescription;
        // Common laptop display identifiers
        if (name.find(L"Generic PnP Monitor") != std::wstring::npos ||
            name.find(L"Default Monitor") != std::wstring::npos ||
            name.empty()) {
            // Further check if this is actually a laptop display via WMI
            if (pWMIService && getLaptopBrightness() != -1) {
                monitor.isLaptopDisplay = true;
            }
        }
    }
}

bool MonitorManagerImpl::setLaptopBrightness(int brightness) {
    if (!pWMIService) return false;

    IEnumWbemClassObject* pEnumerator = NULL;
    HRESULT hres = pWMIService->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM WmiMonitorBrightnessMethods"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator);

    if (FAILED(hres)) return false;

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        // Get the method
        IWbemClassObject* pClass = NULL;
        hres = pWMIService->GetObject(bstr_t(L"WmiMonitorBrightnessMethods"), 0, NULL, &pClass, NULL);
        if (SUCCEEDED(hres)) {
            IWbemClassObject* pInParamsDefinition = NULL;
            hres = pClass->GetMethod(L"WmiSetBrightness", 0, &pInParamsDefinition, NULL);
            if (SUCCEEDED(hres)) {
                IWbemClassObject* pClassInstance = NULL;
                hres = pInParamsDefinition->SpawnInstance(0, &pClassInstance);
                if (SUCCEEDED(hres)) {
                    VARIANT varTimeout, varBrightness;
                    varTimeout.vt = VT_UI4;
                    varTimeout.ulVal = 0;
                    varBrightness.vt = VT_UI1;
                    varBrightness.bVal = (BYTE)brightness;

                    pClassInstance->Put(L"Timeout", 0, &varTimeout, 0);
                    pClassInstance->Put(L"Brightness", 0, &varBrightness, 0);

                    // Get the instance path
                    VARIANT vtProp;
                    pclsObj->Get(L"__PATH", 0, &vtProp, 0, 0);

                    IWbemClassObject* pOutParams = NULL;
                    _bstr_t methodName(L"WmiSetBrightness");
                    hres = pWMIService->ExecMethod(vtProp.bstrVal, methodName, 0, NULL, pClassInstance, &pOutParams, NULL);
                    VariantClear(&vtProp);
                    if (pOutParams) pOutParams->Release();
                    pClassInstance->Release();
                }
                pInParamsDefinition->Release();
            }
            pClass->Release();
        }

        pclsObj->Release();
        break; // Only set for first instance
    }

    pEnumerator->Release();
    return SUCCEEDED(hres);
}

int MonitorManagerImpl::getLaptopBrightness() {
    if (!pWMIService) return -1;

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
void MonitorManagerImpl::initNightLightRegistry()
{
    const std::wstring keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Current\\default$windows.data.bluelightreduction.bluelightreductionstate\\windows.data.bluelightreduction.bluelightreductionstate";

    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ | KEY_WRITE, &m_nightLightRegKey);
    if (result != ERROR_SUCCESS) {
        m_nightLightRegKey = nullptr;
    }
}

void MonitorManagerImpl::cleanupNightLightRegistry()
{
    if (m_nightLightRegKey) {
        RegCloseKey(m_nightLightRegKey);
        m_nightLightRegKey = nullptr;
    }
}

bool MonitorManagerImpl::isNightLightSupported()
{
    return m_nightLightRegKey != nullptr;
}

bool MonitorManagerImpl::isNightLightEnabled()
{
    if (!isNightLightSupported()) return false;

    BYTE data[1024];
    DWORD dataSize = sizeof(data);

    if (RegQueryValueEx(m_nightLightRegKey, L"Data", nullptr, nullptr, data, &dataSize) != ERROR_SUCCESS) {
        return false;
    }

    std::vector<BYTE> bytes(data, data + dataSize);
    if (bytes.size() < 19) return false;

    return bytes[18] == 0x15; // 21 in decimal = enabled
}

void MonitorManagerImpl::enableNightLight()
{
    if (isNightLightSupported() && !isNightLightEnabled()) {
        toggleNightLight();
    }
}

void MonitorManagerImpl::disableNightLight()
{
    if (isNightLightSupported() && isNightLightEnabled()) {
        toggleNightLight();
    }
}

void MonitorManagerImpl::toggleNightLight()
{
    if (!isNightLightSupported()) return;

    BYTE data[1024];
    DWORD dataSize = sizeof(data);

    if (RegQueryValueEx(m_nightLightRegKey, L"Data", nullptr, nullptr, data, &dataSize) != ERROR_SUCCESS) {
        return;
    }

    std::vector<BYTE> newData;
    bool currentlyEnabled = isNightLightEnabled();

    if (currentlyEnabled) {
        // Disable: Allocate 41 bytes and modify the necessary fields
        newData.resize(41, 0);
        std::copy(data, data + 22, newData.begin());
        std::copy(data + 25, data + 43, newData.begin() + 23);
        newData[18] = 0x13; // Disable
    } else {
        // Enable: Allocate 43 bytes and modify the necessary fields
        newData.resize(43, 0);
        std::copy(data, data + 22, newData.begin());
        std::copy(data + 23, data + 41, newData.begin() + 25);
        newData[18] = 0x15; // Enable
        newData[23] = 0x10;
        newData[24] = 0x00;
    }

    // Increment bytes from index 10 to 14 (version tracking)
    for (int i = 10; i < 15; ++i) {
        if (newData[i] != 0xff) {
            newData[i]++;
            break;
        }
    }

    // Set the modified data back to the registry
    DWORD newDataSize = static_cast<DWORD>(newData.size());
    RegSetValueEx(m_nightLightRegKey, L"Data", 0, REG_BINARY, newData.data(), newDataSize);
}
