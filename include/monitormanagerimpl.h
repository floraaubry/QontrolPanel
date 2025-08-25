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
    IWbemServices* pWMIService;

public:
    MonitorManagerImpl();
    ~MonitorManagerImpl();

    void enumerateMonitors();
    int getMonitorCount() const { return monitors.size(); }
    std::wstring getMonitorName(int index) const;
    bool setBrightnessInternal(int monitorIndex, int brightness);
    bool setBrightnessAll(int brightness);
    int getBrightnessInternal(int monitorIndex);
    bool testDDCCI(int monitorIndex);
    int getCachedBrightness(int monitorIndex);
    void setChangeCallback(std::function<void()> callback);

private:
    void initializeWMI();
    void detectLaptopDisplays();
    bool setLaptopBrightness(int brightness);
    int getLaptopBrightness();
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
    void setupChangeDetection();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void cleanup();
};
