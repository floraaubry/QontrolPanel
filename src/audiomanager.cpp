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
#include <QMutex>
#include <QMutexLocker>
#include <QMetaObject>
#include <winver.h>
#pragma comment(lib, "version.lib")

static QThread* g_workerThread = nullptr;
static AudioWorker* g_worker = nullptr;
static QMutex g_initMutex;

static int g_cachedPlaybackVolume = 0;
static int g_cachedRecordingVolume = 0;
static bool g_cachedPlaybackMute = false;
static bool g_cachedRecordingMute = false;

// Helper functions (keep these as they are fast)
QString extractShortName(const QString& fullName) {
    int firstOpenParenIndex = fullName.indexOf('(');
    int lastCloseParenIndex = fullName.lastIndexOf(')');

    if (firstOpenParenIndex != -1 && lastCloseParenIndex != -1 && firstOpenParenIndex < lastCloseParenIndex) {
        return fullName.mid(firstOpenParenIndex + 1, lastCloseParenIndex - firstOpenParenIndex - 1).trimmed();
    }

    return fullName;
}

QIcon getAppIcon(const QString &executablePath) {
    SHFILEINFO shFileInfo;
    if (SHGetFileInfo(executablePath.toStdWString().c_str(),
                      0, &shFileInfo, sizeof(shFileInfo),
                      SHGFI_ICON | SHGFI_LARGEICON)) {
        HICON hIcon = shFileInfo.hIcon;

        ICONINFO iconInfo;
        if (GetIconInfo(hIcon, &iconInfo)) {
            int width = GetSystemMetrics(SM_CXICON);
            int height = GetSystemMetrics(SM_CYICON);

            HDC hdc = GetDC(NULL);
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmColor = iconInfo.hbmColor;

            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmColor);

            BITMAPINFOHEADER biHeader = {0};
            biHeader.biSize = sizeof(BITMAPINFOHEADER);
            biHeader.biWidth = width;
            biHeader.biHeight = -height;
            biHeader.biPlanes = 1;
            biHeader.biBitCount = 32;
            biHeader.biCompression = BI_RGB;

            int imageSize = width * height * 4;
            unsigned char *pixels = new unsigned char[imageSize];

            GetDIBits(hdcMem, hbmColor, 0, height, pixels, (BITMAPINFO *)&biHeader, DIB_RGB_COLORS);

            QImage image(pixels, width, height, QImage::Format_ARGB32);

            SelectObject(hdcMem, hbmOld);
            DeleteDC(hdcMem);
            ReleaseDC(NULL, hdc);

            QPixmap pixmap = QPixmap::fromImage(image);
            delete[] pixels;

            return QIcon(pixmap);
        }
    }
    return QIcon();
}

int getApplicationAudioLevelImpl(const QString& appId) {
    DWORD processId = appId.toULong();

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        return 0;
    }

    CComPtr<IMMDevice> pDevice;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        return 0;
    }

    CComPtr<IAudioSessionManager2> pSessionManager;
    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&pSessionManager);
    if (FAILED(hr)) {
        return 0;
    }

    CComPtr<IAudioSessionEnumerator> pSessionEnumerator;
    hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
    if (FAILED(hr)) {
        return 0;
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

        DWORD pid;
        hr = pSessionControl2->GetProcessId(&pid);
        if (FAILED(hr)) continue;

        if (pid == processId) {
            CComPtr<IAudioMeterInformation> pMeterInfo;
            hr = pSessionControl->QueryInterface(__uuidof(IAudioMeterInformation), (void**)&pMeterInfo);
            if (SUCCEEDED(hr)) {
                float peakLevel = 0.0f;
                hr = pMeterInfo->GetPeakValue(&peakLevel);
                if (SUCCEEDED(hr)) {
                    return static_cast<int>(peakLevel * 100);
                }
            }
        }
    }

    return 0;
}

// Implementation functions (these run on worker thread)
void enumerateDevicesImpl(EDataFlow dataFlow, QList<AudioDevice>& devices) {
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
        device.shortName = extractShortName(device.name);
        device.type = (dataFlow == eRender) ? "Playback" : "Recording";
        device.isDefault = (wcscmp(pwszID, pwszDefaultID) == 0);

        devices.append(device);

        CoTaskMemFree(pwszID);
        PropVariantClear(&varName);
    }

    CoTaskMemFree(pwszDefaultID);
}

void setVolumeImpl(EDataFlow dataFlow, int volume) {
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

int getVolumeImpl(EDataFlow dataFlow) {
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

void setMuteImpl(EDataFlow dataFlow, bool mute) {
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

bool getMuteImpl(EDataFlow dataFlow) {
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

int getAudioLevelPercentageImpl(EDataFlow dataFlow) {
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

bool setDefaultEndpointImpl(const QString &deviceId) {
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

QString getApplicationDisplayName(const QString& executablePath) {
    std::wstring wPath = executablePath.toStdWString();

    // Get version info size
    DWORD dwSize = GetFileVersionInfoSize(wPath.c_str(), NULL);
    if (dwSize == 0) {
        qDebug() << "No version info available for:" << executablePath;
        return QString();
    }

    // Allocate buffer and get version info
    std::vector<BYTE> buffer(dwSize);
    if (!GetFileVersionInfo(wPath.c_str(), 0, dwSize, buffer.data())) {
        qDebug() << "Failed to get version info for:" << executablePath;
        return QString();
    }


    // Get available translations
    LPVOID lpBuffer = NULL;
    UINT uLen = 0;

    // First, try to get the translation table
    if (VerQueryValue(buffer.data(), L"\\VarFileInfo\\Translation", &lpBuffer, &uLen)) {
        DWORD* pTranslations = static_cast<DWORD*>(lpBuffer);
        UINT numTranslations = uLen / sizeof(DWORD);


        // Try each available translation
        for (UINT i = 0; i < numTranslations; i++) {
            DWORD translation = pTranslations[i];
            WORD language = LOWORD(translation);
            WORD codepage = HIWORD(translation);

            QString langCode = QString("%1%2")
                                   .arg(language, 4, 16, QChar('0'))
                                   .arg(codepage, 4, 16, QChar('0'));


            // Try ProductName first
            QString queryPath = QString("\\StringFileInfo\\%1\\ProductName").arg(langCode);
            if (VerQueryValue(buffer.data(), queryPath.toStdWString().c_str(), &lpBuffer, &uLen) && uLen > 0) {
                QString productName = QString::fromWCharArray(static_cast<LPCWSTR>(lpBuffer));
                if (!productName.isEmpty()) {
                    return productName;
                }
            }

            // Try FileDescription
            queryPath = QString("\\StringFileInfo\\%1\\FileDescription").arg(langCode);
            if (VerQueryValue(buffer.data(), queryPath.toStdWString().c_str(), &lpBuffer, &uLen) && uLen > 0) {
                QString fileDescription = QString::fromWCharArray(static_cast<LPCWSTR>(lpBuffer));
                if (!fileDescription.isEmpty()) {
                    return fileDescription;
                }
            }

            // Try CompanyName + executable name as last resort
            queryPath = QString("\\StringFileInfo\\%1\\CompanyName").arg(langCode);
            if (VerQueryValue(buffer.data(), queryPath.toStdWString().c_str(), &lpBuffer, &uLen) && uLen > 0) {
                QString companyName = QString::fromWCharArray(static_cast<LPCWSTR>(lpBuffer));
                if (!companyName.isEmpty()) {
                    QFileInfo fileInfo(executablePath);
                    QString combined = QString("%1 %2").arg(companyName, fileInfo.baseName());
                    return combined;
                }
            }
        }
    } else {
        // Fallback: try common language/codepage combinations
        QStringList commonCodes = {
            "040904b0",  // English US + Unicode
            "040904e4",  // English US + Windows-1252
            "000004b0",  // Language neutral + Unicode
            "000004e4",  // Language neutral + Windows-1252
        };

        for (const QString& langCode : commonCodes) {
            // Try ProductName
            QString queryPath = QString("\\StringFileInfo\\%1\\ProductName").arg(langCode);
            if (VerQueryValue(buffer.data(), queryPath.toStdWString().c_str(), &lpBuffer, &uLen) && uLen > 0) {
                QString productName = QString::fromWCharArray(static_cast<LPCWSTR>(lpBuffer));
                if (!productName.isEmpty()) {
                    return productName;
                }
            }

            // Try FileDescription
            queryPath = QString("\\StringFileInfo\\%1\\FileDescription").arg(langCode);
            if (VerQueryValue(buffer.data(), queryPath.toStdWString().c_str(), &lpBuffer, &uLen) && uLen > 0) {
                QString fileDescription = QString::fromWCharArray(static_cast<LPCWSTR>(lpBuffer));
                if (!fileDescription.isEmpty()) {
                    return fileDescription;
                }
            }
        }
    }

    return QString();
}

QList<Application> enumerateAudioApplicationsImpl() {
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
        QString sessionDisplayName = SUCCEEDED(hr) ? QString::fromWCharArray(pwszDisplayName) : "Unknown Application";
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

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        QString executablePath;
        QString executableName;
        QString finalAppName = sessionDisplayName; // Default to session display name
        QIcon appIcon;

        if (hProcess) {
            WCHAR exePath[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, NULL, exePath, MAX_PATH)) {
                executablePath = QString::fromWCharArray(exePath);

                QFileInfo fileInfo(executablePath);
                executableName = fileInfo.baseName();

                if (!executableName.isEmpty()) {
                    executableName[0] = executableName[0].toUpper();
                }

                // Try to get better display name from version info
                QString betterDisplayName = getApplicationDisplayName(executablePath);
                if (!betterDisplayName.isEmpty()) {
                    finalAppName = betterDisplayName;
                } else if (!executableName.isEmpty()) {
                    // Fallback to capitalized executable name if no version info
                    finalAppName = executableName;
                }

                appIcon = getAppIcon(executablePath);
            }
            CloseHandle(hProcess);
        }

        // Handle special case for Windows system sounds
        if (sessionDisplayName == "@%SystemRoot%\\System32\\AudioSrv.Dll,-202") {
            finalAppName = "System sounds";
            executableName = "System sounds";
            appIcon = QIcon(":/icons/system.png");
        }

        Application app;
        app.id = appId;
        app.name = finalAppName;
        app.executableName = executableName;
        app.isMuted = isMuted;
        app.volume = static_cast<int>(volumeLevel * 100);
        app.icon = appIcon;
        app.audioLevel = 0;

        applications.append(app);
    }

    return applications;
}

bool setApplicationMuteImpl(const QString& appId, bool mute) {
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

        DWORD pid = 0;
        hr = pSessionControl2->GetProcessId(&pid);
        if (FAILED(hr)) continue;

        if (QString::number(pid) == appId) {
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

bool setApplicationVolumeImpl(const QString& appId, int volume) {
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

        DWORD pid = 0;
        hr = pSessionControl2->GetProcessId(&pid);
        if (FAILED(hr)) continue;

        if (QString::number(pid) == appId) {
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

bool getApplicationMuteImpl(const QString &appId) {
    DWORD processId = appId.toULong();

    CComPtr<IMMDeviceEnumerator> pEnumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to create device enumerator";
        return false;
    }

    CComPtr<IMMDevice> pDevice;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        qDebug() << "Failed to get default audio endpoint";
        return false;
    }

    CComPtr<IAudioSessionManager2> pSessionManager;
    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&pSessionManager);
    if (FAILED(hr)) {
        qDebug() << "Failed to get audio session manager";
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

        DWORD pid;
        hr = pSessionControl2->GetProcessId(&pid);
        if (FAILED(hr)) continue;

        if (pid == processId) {
            CComPtr<ISimpleAudioVolume> pSimpleAudioVolume;
            hr = pSessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleAudioVolume);
            if (FAILED(hr)) continue;

            BOOL isMuted;
            hr = pSimpleAudioVolume->GetMute(&isMuted);
            if (FAILED(hr)) continue;

            return isMuted == TRUE;
        }
    }

    return false;
}

void AudioWorker::initializeTimer() {
    m_audioLevelTimer = new QTimer(this);
    connect(m_audioLevelTimer, &QTimer::timeout, this, [this]() {
        int playbackLevel = getAudioLevelPercentageImpl(eRender);
        int recordingLevel = getAudioLevelPercentageImpl(eCapture);
        emit playbackAudioLevel(playbackLevel);
        emit recordingAudioLevel(recordingLevel);

        // Get application audio levels
        QList<QPair<QString, int>> appLevels;
        for (const Application& app : m_cachedApplications) {
            // Handle grouped applications
            QStringList appIDs = app.id.split(";");
            int maxLevel = 0;
            for (const QString& id : appIDs) {
                int level = getApplicationAudioLevelImpl(id);
                maxLevel = qMax(maxLevel, level);
            }
            appLevels.append({app.id, maxLevel});
        }
        emit applicationAudioLevelsChanged(appLevels);
    });
}

void AudioWorker::enumeratePlaybackDevices() {
    QList<AudioDevice> devices;
    enumerateDevicesImpl(eRender, devices);
    emit playbackDevicesReady(devices);
}

void AudioWorker::enumerateRecordingDevices() {
    QList<AudioDevice> devices;
    enumerateDevicesImpl(eCapture, devices);
    emit recordingDevicesReady(devices);
}

void AudioWorker::enumerateApplications() {
    QList<Application> apps = enumerateAudioApplicationsImpl();

    // Add initial audio levels and cache
    for (Application& app : apps) {
        QStringList appIDs = app.id.split(";");
        int maxLevel = 0;
        for (const QString& id : appIDs) {
            int level = getApplicationAudioLevelImpl(id);
            maxLevel = qMax(maxLevel, level);
        }
        app.audioLevel = maxLevel;
    }

    m_cachedApplications = apps;
    emit applicationsReady(apps);
}

void AudioWorker::setPlaybackVolume(int volume) {
    setVolumeImpl(eRender, volume);
    emit playbackVolumeChanged(volume);
}

void AudioWorker::setRecordingVolume(int volume) {
    setVolumeImpl(eCapture, volume);
    emit recordingVolumeChanged(volume);
}

void AudioWorker::setPlaybackMute(bool mute) {
    setMuteImpl(eRender, mute);
    emit playbackMuteChanged(mute);
}

void AudioWorker::setRecordingMute(bool mute) {
    setMuteImpl(eCapture, mute);
    emit recordingMuteChanged(mute);
}

void AudioWorker::setDefaultEndpoint(const QString &deviceId) {
    bool success = setDefaultEndpointImpl(deviceId);

    if (success) {
        // Queue the property queries for next event loop iteration (non-blocking)
        QMetaObject::invokeMethod(this, "updateDeviceProperties", Qt::QueuedConnection);
    }

    emit defaultEndpointChanged(success);
}

void AudioWorker::queryCurrentProperties() {
    // Query fresh values async
    int playbackVol = getVolumeImpl(eRender);
    int recordingVol = getVolumeImpl(eCapture);
    bool playbackMute = getMuteImpl(eRender);
    bool recordingMute = getMuteImpl(eCapture);

    // Update cache
    emit playbackVolumeChanged(playbackVol);
    emit recordingVolumeChanged(recordingVol);
    emit playbackMuteChanged(playbackMute);
    emit recordingMuteChanged(recordingMute);

    // Signal that fresh properties are ready
    emit currentPropertiesReady(playbackVol, recordingVol, playbackMute, recordingMute);
}

void AudioWorker::setApplicationVolume(const QString& appId, int volume) {
    bool success = setApplicationVolumeImpl(appId, volume);
    emit applicationVolumeChanged(appId, success);
}

void AudioWorker::setApplicationMute(const QString& appId, bool mute) {
    bool success = setApplicationMuteImpl(appId, mute);
    emit applicationMuteChanged(appId, success);
}

void AudioWorker::startAudioLevelMonitoring() {
    if (!m_audioLevelTimer) {
        initializeTimer();
    }
    m_audioLevelTimer->start(100);
}

void AudioWorker::stopAudioLevelMonitoring() {
    if (m_audioLevelTimer) {
        m_audioLevelTimer->stop();
    }
}

void AudioWorker::updateDeviceProperties() {
    // Now these run async on next event loop - no UI blocking
    int newPlaybackVolume = getVolumeImpl(eRender);
    int newRecordingVolume = getVolumeImpl(eCapture);
    bool newPlaybackMute = getMuteImpl(eRender);
    bool newRecordingMute = getMuteImpl(eCapture);

    emit playbackVolumeChanged(newPlaybackVolume);
    emit recordingVolumeChanged(newRecordingVolume);
    emit playbackMuteChanged(newPlaybackMute);
    emit recordingMuteChanged(newRecordingMute);
}

void AudioManager::initialize() {
    QMutexLocker locker(&g_initMutex);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to initialize COM library");
    }

    if (!g_workerThread) {
        g_workerThread = new QThread;
        g_worker = new AudioWorker;
        g_worker->moveToThread(g_workerThread);
        g_workerThread->start();

        // Connect to worker signals to update cache
        QObject::connect(g_worker, &AudioWorker::playbackVolumeChanged,
                         [](int volume) { g_cachedPlaybackVolume = volume; });
        QObject::connect(g_worker, &AudioWorker::recordingVolumeChanged,
                         [](int volume) { g_cachedRecordingVolume = volume; });
        QObject::connect(g_worker, &AudioWorker::playbackMuteChanged,
                         [](bool muted) { g_cachedPlaybackMute = muted; });
        QObject::connect(g_worker, &AudioWorker::recordingMuteChanged,
                         [](bool muted) { g_cachedRecordingMute = muted; });

        // Initialize cache asynchronously on worker thread
        QMetaObject::invokeMethod(g_worker, "initializeCache", Qt::QueuedConnection);
    }
}

void AudioWorker::initializeCache() {
    // Initialize cache with current values (runs on worker thread)
    int playbackVol = getVolumeImpl(eRender);
    int recordingVol = getVolumeImpl(eCapture);
    bool playbackMute = getMuteImpl(eRender);
    bool recordingMute = getMuteImpl(eCapture);

    // Emit signals to update cache
    emit playbackVolumeChanged(playbackVol);
    emit recordingVolumeChanged(recordingVol);
    emit playbackMuteChanged(playbackMute);
    emit recordingMuteChanged(recordingMute);
}

void AudioManager::cleanup() {
    QMutexLocker locker(&g_initMutex);

    if (g_workerThread) {
        g_workerThread->quit();
        g_workerThread->wait();
        delete g_worker;
        delete g_workerThread;
        g_worker = nullptr;
        g_workerThread = nullptr;
    }

    g_cachedPlaybackVolume = 0;
    g_cachedRecordingVolume = 0;
    g_cachedPlaybackMute = false;
    g_cachedRecordingMute = false;

    CoUninitialize();
}

AudioWorker* AudioManager::getWorker() {
    return g_worker;
}

// Async wrapper functions
void AudioManager::enumeratePlaybackDevicesAsync() {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "enumeratePlaybackDevices", Qt::QueuedConnection);
    }
}

void AudioManager::enumerateRecordingDevicesAsync() {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "enumerateRecordingDevices", Qt::QueuedConnection);
    }
}

void AudioManager::enumerateApplicationsAsync() {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "enumerateApplications", Qt::QueuedConnection);
    }
}

void AudioManager::setPlaybackVolumeAsync(int volume) {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "setPlaybackVolume", Qt::QueuedConnection, Q_ARG(int, volume));
    }
}

void AudioManager::setRecordingVolumeAsync(int volume) {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "setRecordingVolume", Qt::QueuedConnection, Q_ARG(int, volume));
    }
}

void AudioManager::setPlaybackMuteAsync(bool mute) {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "setPlaybackMute", Qt::QueuedConnection, Q_ARG(bool, mute));
    }
}

void AudioManager::setRecordingMuteAsync(bool mute) {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "setRecordingMute", Qt::QueuedConnection, Q_ARG(bool, mute));
    }
}

void AudioManager::setDefaultEndpointAsync(const QString &deviceId) {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "setDefaultEndpoint", Qt::QueuedConnection, Q_ARG(QString, deviceId));
    }
}

void AudioManager::setApplicationVolumeAsync(const QString& appId, int volume) {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "setApplicationVolume", Qt::QueuedConnection,
                                  Q_ARG(QString, appId), Q_ARG(int, volume));
    }
}

void AudioManager::setApplicationMuteAsync(const QString& appId, bool mute) {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "setApplicationMute", Qt::QueuedConnection,
                                  Q_ARG(QString, appId), Q_ARG(bool, mute));
    }
}

void AudioManager::startAudioLevelMonitoring() {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "startAudioLevelMonitoring", Qt::QueuedConnection);
    }
}

void AudioManager::stopAudioLevelMonitoring() {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "stopAudioLevelMonitoring", Qt::QueuedConnection);
    }
}

int AudioManager::getPlaybackVolume() {
    return g_cachedPlaybackVolume;
}

int AudioManager::getRecordingVolume() {
    return g_cachedRecordingVolume;
}

bool AudioManager::getPlaybackMute() {
    return g_cachedPlaybackMute;
}

bool AudioManager::getRecordingMute() {
    return g_cachedRecordingMute;
}

void AudioManager::queryCurrentPropertiesAsync() {
    if (g_worker) {
        QMetaObject::invokeMethod(g_worker, "queryCurrentProperties", Qt::QueuedConnection);
    }
}
