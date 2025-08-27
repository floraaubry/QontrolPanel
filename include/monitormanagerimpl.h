#pragma once
// Windows-only headers, no Qt
#include <windows.h>
#include <physicalmonitorenumerationapi.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>

#pragma comment(lib, "dxva2.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "wbemuuid.lib")

class MonitorManagerImpl
{
private:
    struct MonitorInfo {
        PHYSICAL_MONITOR physicalMonitor;
        std::wstring deviceName;
        bool ddcciTested;
        bool ddcciWorking;
        bool isLaptopDisplay;
        int cachedBrightness;
    };

    std::vector<MonitorInfo> monitors;
    HWND messageWindow;
    std::function<void()> changeCallback;

    // WMI connection management
    IWbemServices* pWMIService;
    mutable std::mutex m_wmiMutex;
    std::atomic<bool> m_wmiInitialized{false};
    std::chrono::steady_clock::time_point m_lastWMIInit;
    static constexpr auto WMI_CACHE_DURATION = std::chrono::minutes(5);

    // Night Light registry management
    HKEY m_nightLightRegKey;
    void initNightLightRegistry();
    void cleanupNightLightRegistry();

public:
    MonitorManagerImpl();
    ~MonitorManagerImpl();

    void enumerateMonitors();
    int getMonitorCount() const { return static_cast<int>(monitors.size()); }
    std::wstring getMonitorName(int index) const;
    bool setBrightnessInternal(int monitorIndex, int brightness);
    bool setBrightnessAll(int brightness);
    int getBrightnessInternal(int monitorIndex);
    bool testDDCCI(int monitorIndex);
    int getCachedBrightness(int monitorIndex);
    void setChangeCallback(std::function<void()> callback);

    // Night Light functionality
    bool isNightLightSupported();
    bool isNightLightEnabled();
    void enableNightLight();
    void disableNightLight();
    void toggleNightLight();

private:
    // WMI connection management
    bool ensureWMIConnection();
    bool quickWMIHealthCheck();
    void initializeWMI();
    void cleanupWMI();

    // Monitor detection and brightness control
    void detectLaptopDisplays();
    bool setLaptopBrightness(int brightness);
    int getLaptopBrightness();

    // Windows API callbacks
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
    void setupChangeDetection();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void cleanup();
};
