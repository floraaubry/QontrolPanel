#include "monitormanagerimpl.h"
#include <algorithm>
#include <iostream>

MonitorManagerImpl::MonitorManagerImpl()
    : messageWindow(nullptr)
    , pWMIService(nullptr)
{
    initializeWMI();
    enumerateMonitors();
    setupChangeDetection();
}

MonitorManagerImpl::~MonitorManagerImpl() {
    cleanup();
    if (pWMIService) {
        pWMIService->Release();
        CoUninitialize();
    }
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

    hres = CoSetProxyBlanket(pWMIService, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                             RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    pLoc->Release();
}

void MonitorManagerImpl::detectLaptopDisplays() {
    // Check if this is a laptop by looking for battery
    SYSTEM_POWER_STATUS powerStatus;
    if (GetSystemPowerStatus(&powerStatus) && powerStatus.BatteryFlag != 128) {
        // Has battery, likely a laptop - add internal display entry
        MonitorInfo laptopInfo = {};
        laptopInfo.isLaptopDisplay = true;
        laptopInfo.ddcciTested = true;
        laptopInfo.ddcciWorking = true;
        laptopInfo.cachedBrightness = getLaptopBrightness();

        monitors.insert(monitors.begin(), laptopInfo); // Add as first monitor
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

        // Call WmiSetBrightness method
        IWbemClassObject* pClass = NULL;
        hres = pWMIService->GetObject(bstr_t("WmiMonitorBrightnessMethods"), 0, NULL, &pClass, NULL);

        if (SUCCEEDED(hres)) {
            IWbemClassObject* pInParamsDefinition = NULL;
            hres = pClass->GetMethod(L"WmiSetBrightness", 0, &pInParamsDefinition, NULL);

            if (SUCCEEDED(hres)) {
                IWbemClassObject* pClassInstance = NULL;
                hres = pInParamsDefinition->SpawnInstance(0, &pClassInstance);

                if (SUCCEEDED(hres)) {
                    VARIANT varTimeout, varBrightness;
                    varTimeout.vt = VT_UI4;
                    varTimeout.uintVal = 0;
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
