#include "audiomanager.h"
#include "policyconfig.h"
#include <Audiopolicy.h>
#include <atlbase.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <QDebug>
#include <psapi.h>
#include <Shlobj.h>
#include <QIcon>
#include <QPixmap>
#include <QFileInfo>

void AudioManager::initialize() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to initialize COM library");
    }
}

void AudioManager::cleanup() {
    CoUninitialize();
}

QString extractShortName(const QString& fullName) {
    int firstOpenParenIndex = fullName.indexOf('(');
    int lastCloseParenIndex = fullName.lastIndexOf(')');

    if (firstOpenParenIndex != -1 && lastCloseParenIndex != -1 && firstOpenParenIndex < lastCloseParenIndex) {
        return fullName.mid(firstOpenParenIndex + 1, lastCloseParenIndex - firstOpenParenIndex - 1).trimmed();
    }

    return fullName;
}

void enumerateDevices(EDataFlow dataFlow, QList<AudioDevice>& devices) {

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

    if (FAILED(hr)) {
        qDebug() << "Failed to create device enumerator";

        return;
    }

    CComPtr<IMMDeviceCollection> pCollection;
    hr = pEnumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr)) {
        qDebug() << "Failed to enumerate devices";

        return;
    }

    // Get the default audio endpoint for this data flow and role
    CComPtr<IMMDevice> pDefaultDevice;
    hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDefaultDevice);
    if (FAILED(hr)) {
        qDebug() << "Failed to get default audio endpoint";

        return;
    }

    LPWSTR pwszDefaultID = nullptr;
    hr = pDefaultDevice->GetId(&pwszDefaultID);
    if (FAILED(hr)) {
        qDebug() << "Failed to get default device ID";

        return;
    }

    UINT count;
    pCollection->GetCount(&count);

    for (UINT i = 0; i < count; ++i) {
        CComPtr<IMMDevice> pDevice;
        pCollection->Item(i, &pDevice);

        LPWSTR pwszID = nullptr;
        pDevice->GetId(&pwszID);

        CComPtr<IPropertyStore> pProps;
        pDevice->OpenPropertyStore(STGM_READ, &pProps);

        PROPVARIANT varName;
        PropVariantInit(&varName);
        pProps->GetValue(PKEY_Device_FriendlyName, &varName);

        AudioDevice device;
        device.id = QString::fromWCharArray(pwszID);
        device.name = QString::fromWCharArray(varName.pwszVal);
        device.shortName = extractShortName(device.name); // Customize if needed
        device.type = (dataFlow == eRender) ? "Playback" : "Recording";
        device.isDefault = (wcscmp(pwszID, pwszDefaultID) == 0); // Check if this device is the default one

        devices.append(device);

        CoTaskMemFree(pwszID);
        PropVariantClear(&varName);
    }

    CoTaskMemFree(pwszDefaultID);

}

void AudioManager::enumeratePlaybackDevices(QList<AudioDevice>& playbackDevices) {
    enumerateDevices(eRender, playbackDevices);
}

void AudioManager::enumerateRecordingDevices(QList<AudioDevice>& recordingDevices) {
    enumerateDevices(eCapture, recordingDevices);
}

void setVolume(EDataFlow dataFlow, int volume) {

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioEndpointVolume> pVolumeControl;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVolumeControl);
    }
    if (SUCCEEDED(hr)) {
        pVolumeControl->SetMasterVolumeLevelScalar(static_cast<float>(volume) / 100.0f, nullptr);
    }

}

int getVolume(EDataFlow dataFlow) {

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioEndpointVolume> pVolumeControl;
    float volume = 0.0f;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVolumeControl);
    }
    if (SUCCEEDED(hr)) {
        pVolumeControl->GetMasterVolumeLevelScalar(&volume);
    }


    return static_cast<int>(std::round(volume * 100.0f));
}

void setMute(EDataFlow dataFlow, bool mute) {

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioEndpointVolume> pVolumeControl;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVolumeControl);
    }
    if (SUCCEEDED(hr)) {
        pVolumeControl->SetMute(mute, nullptr);
    }
}

bool getMute(EDataFlow dataFlow) {

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioEndpointVolume> pVolumeControl;
    BOOL mute = FALSE;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVolumeControl);
    }
    if (SUCCEEDED(hr)) {
        pVolumeControl->GetMute(&mute);
    }


    return mute;
}

void AudioManager::setPlaybackVolume(int volume) {
    setVolume(eRender, volume);
}

int AudioManager::getPlaybackVolume() {
    return getVolume(eRender);
}

void AudioManager::setRecordingVolume(int volume) {
    setVolume(eCapture, volume);
}

int AudioManager::getRecordingVolume() {
    return getVolume(eCapture);
}

void AudioManager::setPlaybackMute(bool mute) {
    setMute(eRender, mute);
}

bool AudioManager::getPlaybackMute() {
    return getMute(eRender);
}

void AudioManager::setRecordingMute(bool mute) {
    setMute(eCapture, mute);
}

bool AudioManager::getRecordingMute() {
    return getMute(eCapture);
}

int getAudioLevelPercentage(EDataFlow dataFlow) {

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioMeterInformation> pMeterInfo;
    float peakLevel = 0.0f;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, nullptr, (void**)&pMeterInfo);
    }
    if (SUCCEEDED(hr)) {
        hr = pMeterInfo->GetPeakValue(&peakLevel);
        if (FAILED(hr)) {
            qDebug() << "Failed to get audio peak level";


            return 0;
        }
    } else {
        qDebug() << "Failed to activate IAudioMeterInformation";


        return 0;
    }


    return static_cast<int>(peakLevel * 100);
}

int AudioManager::getPlaybackAudioLevel() {
    return getAudioLevelPercentage(eRender);
}

int AudioManager::getRecordingAudioLevel() {
    return getAudioLevelPercentage(eCapture);
}

bool AudioManager::setDefaultEndpoint(const QString &deviceId)
{
    HRESULT hr;
    IMMDeviceEnumerator *deviceEnumerator = nullptr;
    IMMDevice *defaultDevice = nullptr;
    IPolicyConfig *policyConfig = nullptr;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&deviceEnumerator));
    if (FAILED(hr)) {
        qDebug() << "Failed to create device enumerator";

        return false;
    }

    hr = deviceEnumerator->GetDevice(reinterpret_cast<LPCWSTR>(deviceId.utf16()), &defaultDevice);
    if (FAILED(hr)) {
        qDebug() << "Failed to get device by ID";
        deviceEnumerator->Release();

        return false;
    }

    hr = CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_ALL,
                          IID_PPV_ARGS(&policyConfig));
    if (FAILED(hr)) {
        qDebug() << "Failed to create PolicyConfig interface";
        defaultDevice->Release();
        deviceEnumerator->Release();

        return false;
    }

    // Set as default device for all audio roles (Console, Multimedia, Communications)
    hr = policyConfig->SetDefaultEndpoint(reinterpret_cast<LPCWSTR>(deviceId.utf16()), eConsole);
    policyConfig->SetDefaultEndpoint(reinterpret_cast<LPCWSTR>(deviceId.utf16()), eMultimedia);
    policyConfig->SetDefaultEndpoint(reinterpret_cast<LPCWSTR>(deviceId.utf16()), eCommunications);

    policyConfig->Release();
    defaultDevice->Release();
    deviceEnumerator->Release();


    if (FAILED(hr)) {
        qDebug() << "Failed to set default audio output device";

        return false;
    }


    return true;
}

QIcon getAppIcon(const QString &executablePath) {
    SHFILEINFO shFileInfo;
    if (SHGetFileInfo(executablePath.toStdWString().c_str(),
                      0, &shFileInfo, sizeof(shFileInfo),
                      SHGFI_ICON | SHGFI_LARGEICON)) {
        HICON hIcon = shFileInfo.hIcon;

        // Retrieve the icon's bitmap information
        ICONINFO iconInfo;
        if (GetIconInfo(hIcon, &iconInfo)) {
            // Get the size of the icon
            int width = GetSystemMetrics(SM_CXICON);
            int height = GetSystemMetrics(SM_CYICON);

            // Create a compatible DC to extract the pixels
            HDC hdc = GetDC(NULL);
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmColor = iconInfo.hbmColor;  // Icon bitmap

            // Select the icon's bitmap into the memory DC
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmColor);

            // Create a BITMAPINFO structure to retrieve pixel data
            BITMAPINFOHEADER biHeader = {0};
            biHeader.biSize = sizeof(BITMAPINFOHEADER);
            biHeader.biWidth = width;
            biHeader.biHeight = -height;  // Negative height for top-down DIB
            biHeader.biPlanes = 1;
            biHeader.biBitCount = 32;  // 32-bit color (ARGB)
            biHeader.biCompression = BI_RGB;

            int imageSize = width * height * 4;  // 4 bytes per pixel (ARGB)
            unsigned char *pixels = new unsigned char[imageSize];

            // Get the pixel data from the bitmap into the pixel buffer
            GetDIBits(hdcMem, hbmColor, 0, height, pixels, (BITMAPINFO *)&biHeader, DIB_RGB_COLORS);

            // Create a QImage from the pixel buffer
            QImage image(pixels, width, height, QImage::Format_ARGB32);

            // Clean up
            SelectObject(hdcMem, hbmOld);
            DeleteDC(hdcMem);
            ReleaseDC(NULL, hdc);

            // Create a QPixmap from the QImage and return it
            QPixmap pixmap = QPixmap::fromImage(image);

            // Free the pixel buffer
            delete[] pixels;

            // Return QIcon from QPixmap
            return QIcon(pixmap);
        }
    }
    return QIcon();  // Return an empty icon if extraction failed
}

QList<Application> AudioManager::enumerateAudioApplications() {

    QList<Application> applications;

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to create device enumerator";

        return applications;
    }

    CComPtr<IMMDevice> pDevice;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        qDebug() << "Failed to get default audio endpoint";

        return applications;
    }

    CComPtr<IAudioSessionManager2> pSessionManager;
    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&pSessionManager);
    if (FAILED(hr)) {
        qDebug() << "Failed to get audio session manager";

        return applications;
    }

    CComPtr<IAudioSessionEnumerator> pSessionEnumerator;
    hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to get session enumerator";

        return applications;
    }

    int sessionCount;
    pSessionEnumerator->GetCount(&sessionCount);

    for (int i = 0; i < sessionCount; ++i) {
        CComPtr<IAudioSessionControl> pSessionControl;
        hr = pSessionEnumerator->GetSession(i, &pSessionControl);
        if (FAILED(hr)) {
            qDebug() << "Failed to get session control";
            continue;
        }

        CComPtr<IAudioSessionControl2> pSessionControl2;
        hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
        if (FAILED(hr)) {
            qDebug() << "Failed to query IAudioSessionControl2";
            continue;
        }

        DWORD processId;
        pSessionControl2->GetProcessId(&processId);
        QString appId = QString::number(processId);

        LPWSTR pwszDisplayName = nullptr;
        hr = pSessionControl->GetDisplayName(&pwszDisplayName);
        QString appName = SUCCEEDED(hr) ? QString::fromWCharArray(pwszDisplayName) : "Unknown Application";
        CoTaskMemFree(pwszDisplayName);

        CComPtr<ISimpleAudioVolume> pSimpleAudioVolume;
        hr = pSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleAudioVolume);
        if (FAILED(hr)) {
            qDebug() << "Failed to get ISimpleAudioVolume";
            continue;
        }

        BOOL isMuted;
        pSimpleAudioVolume->GetMute(&isMuted);

        float volumeLevel;
        pSimpleAudioVolume->GetMasterVolume(&volumeLevel);

        // Fetch executable path and icon
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        QString executablePath;
        QString executableName;
        QIcon appIcon;
        if (hProcess) {
            WCHAR exePath[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, NULL, exePath, MAX_PATH)) {
                executablePath = QString::fromWCharArray(exePath);

                // Extract executable name from path
                QFileInfo fileInfo(executablePath);
                executableName = fileInfo.baseName(); // Extract name without extension

                // Capitalize the first character if possible
                if (!executableName.isEmpty()) {
                    executableName[0] = executableName[0].toUpper();
                }

                // Use the helper function to extract the icon
                appIcon = getAppIcon(executablePath);  // Extract icon using SHGetFileInfo or ExtractIcon
            }
            CloseHandle(hProcess);
        }

        // Override executable name and icon if the app is the audio service
        if (appName == "@%SystemRoot%\\System32\\AudioSrv.Dll,-202") {
            appName = "Windows system sounds";
            executableName = "Windows system sounds";
            appIcon = QIcon(":/icons/system.png");
        }

        // Create an Application struct and populate fields
        Application app;
        app.id = appId;
        app.name = appName;
        app.executableName = executableName;
        app.isMuted = isMuted;
        app.volume = static_cast<int>(volumeLevel * 100);
        app.icon = appIcon; // Assign the extracted (or overridden) icon

        applications.append(app);
    }


    return applications;
}

bool AudioManager::setApplicationMute(const QString& appId, bool mute) {

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioSessionManager2> pSessionManager;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&pSessionManager);
    }
    if (FAILED(hr)) {
        qDebug() << "Failed to initialize audio session manager";

        return false;
    }

    CComPtr<IAudioSessionEnumerator> pSessionEnumerator;
    hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to get session enumerator";

        return false;
    }

    int sessionCount;
    pSessionEnumerator->GetCount(&sessionCount);

    for (int i = 0; i < sessionCount; ++i) {
        CComPtr<IAudioSessionControl> pSessionControl;
        hr = pSessionEnumerator->GetSession(i, &pSessionControl);
        if (FAILED(hr)) continue;

        CComPtr<IAudioSessionControl2> pSessionControl2;
        hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
        if (FAILED(hr)) continue;

        // Get the process ID of the application
        DWORD pid = 0;
        hr = pSessionControl2->GetProcessId(&pid);
        if (FAILED(hr)) continue;

        // Compare the process ID to the appId (PID)
        if (QString::number(pid) == appId) {
            // Set mute status
            CComPtr<ISimpleAudioVolume> pAudioVolume;
            hr = pSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
            if (SUCCEEDED(hr)) {
                pAudioVolume->SetMute(mute, nullptr);
                return true;
            }
        }
    }


    return false;
}


bool AudioManager::setApplicationVolume(const QString& appId, int volume) {

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    CComPtr<IMMDevice> pDevice;
    CComPtr<IAudioSessionManager2> pSessionManager;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    }
    if (SUCCEEDED(hr)) {
        hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&pSessionManager);
    }
    if (FAILED(hr)) {
        qDebug() << "Failed to initialize audio session manager";

        return false;
    }

    CComPtr<IAudioSessionEnumerator> pSessionEnumerator;
    hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to get session enumerator";

        return false;
    }

    int sessionCount;
    pSessionEnumerator->GetCount(&sessionCount);

    for (int i = 0; i < sessionCount; ++i) {
        CComPtr<IAudioSessionControl> pSessionControl;
        hr = pSessionEnumerator->GetSession(i, &pSessionControl);
        if (FAILED(hr)) continue;

        CComPtr<IAudioSessionControl2> pSessionControl2;
        hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
        if (FAILED(hr)) continue;

        // Get the process ID of the application
        DWORD pid = 0;
        hr = pSessionControl2->GetProcessId(&pid);
        if (FAILED(hr)) continue;

        // Compare the process ID to the appId (PID)
        if (QString::number(pid) == appId) {
            // Set volume level
            CComPtr<ISimpleAudioVolume> pAudioVolume;
            hr = pSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
            if (SUCCEEDED(hr)) {
                float volumeScalar = static_cast<float>(volume) / 100.0f;
                pAudioVolume->SetMasterVolume(volumeScalar, nullptr);

                return true;
            }
        }
    }


    return false;
}


bool AudioManager::getApplicationMute(const QString &appId) {

    DWORD processId = appId.toULong();

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to create device enumerator";

        return false;
    }

    // Get the default audio endpoint for rendering
    CComPtr<IMMDevice> pDevice;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        qDebug() << "Failed to get default audio endpoint";

        return false;
    }

    // Get the audio session manager
    CComPtr<IAudioSessionManager2> pSessionManager;
    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&pSessionManager);
    if (FAILED(hr)) {
        qDebug() << "Failed to get audio session manager";

        return false;
    }

    // Enumerate audio sessions to find the specific one
    CComPtr<IAudioSessionEnumerator> pSessionEnumerator;
    hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to get session enumerator";

        return false;
    }

    int sessionCount;
    pSessionEnumerator->GetCount(&sessionCount);

    for (int i = 0; i < sessionCount; ++i) {
        CComPtr<IAudioSessionControl> pSessionControl;
        hr = pSessionEnumerator->GetSession(i, &pSessionControl);
        if (FAILED(hr)) continue;

        CComPtr<IAudioSessionControl2> pSessionControl2;
        hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
        if (FAILED(hr)) continue;

        // Retrieve the process ID of the session
        DWORD pid;
        hr = pSessionControl2->GetProcessId(&pid);
        if (FAILED(hr)) continue;

        // If the process ID matches, get the mute status
        if (pid == processId) {
            CComPtr<ISimpleAudioVolume> pSimpleAudioVolume;
            hr = pSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleAudioVolume);
            if (FAILED(hr)) continue;

            BOOL isMuted;
            hr = pSimpleAudioVolume->GetMute(&isMuted);
            if (FAILED(hr)) continue;

            return isMuted == TRUE;  // Return true if muted, false otherwise
        }
    }

    return false;  // Default return value if not found
}
