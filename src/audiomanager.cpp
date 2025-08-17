#include "audiomanager.h"
#include <QDebug>
#include <QMutexLocker>
#include <QMetaObject>
#include <QBuffer>
#include <QPixmap>
#include <QFileInfo>
#include <QPainter>
#include <QTimer>
#include <QRegularExpression>
#include <atlbase.h>
#include <psapi.h>
#include <Shlobj.h>
#include <winver.h>
#include <Functiondiscoverykeys_devpkey.h>
#pragma comment(lib, "version.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")

SIZE_T getProcessMemoryUsage(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) {
        return 0;
    }

    PROCESS_MEMORY_COUNTERS_EX memCounters;
    SIZE_T memoryUsage = 0;

    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&memCounters, sizeof(memCounters))) {
        memoryUsage = memCounters.WorkingSetSize; // Current physical memory usage
    }

    CloseHandle(hProcess);
    return memoryUsage;
}

AudioManager* AudioManager::m_instance = nullptr;
QMutex AudioManager::m_mutex;

// AudioDeviceModel implementation
AudioDeviceModel::AudioDeviceModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AudioDeviceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_devices.count();
}

QVariant AudioDeviceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_devices.count())
        return QVariant();
    const AudioDevice& device = m_devices.at(index.row());
    switch (role) {
    case IdRole:
        return device.id;
    case NameRole:
        return device.name;
    case DescriptionRole:
        return device.description;
    case IsDefaultRole:
        return device.isDefault;
    case IsDefaultCommunicationRole:
        return device.isDefaultCommunication;
    case IsInputRole:
        return device.isInput;
    case StateRole:
        return device.state;
    case VendorIdRole:
        return device.vendorId;
    case ProductIdRole:
        return device.productId;
    case BatteryPercentageRole:
        return device.batteryPercentage;
    case BatteryStatusRole:
        return device.batteryStatus;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> AudioDeviceModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "deviceId";
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[IsDefaultRole] = "isDefault";
    roles[IsDefaultCommunicationRole] = "isDefaultCommunication";
    roles[IsInputRole] = "isInput";
    roles[StateRole] = "state";
    roles[VendorIdRole] = "vendorId";
    roles[ProductIdRole] = "productId";
    roles[BatteryPercentageRole] = "batteryPercentage";
    roles[BatteryStatusRole] = "batteryStatus";
    return roles;
}

void AudioDeviceModel::setDevices(const QList<AudioDevice>& devices)
{
    beginResetModel();
    m_devices = devices;
    endResetModel();
}

void AudioDeviceModel::updateDevice(const AudioDevice& device)
{
    int index = findDeviceIndex(device.id);
    if (index >= 0) {
        m_devices[index] = device;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex);
    }
}

void AudioDeviceModel::removeDevice(const QString& deviceId)
{
    int index = findDeviceIndex(deviceId);
    if (index >= 0) {
        beginRemoveRows(QModelIndex(), index, index);
        m_devices.removeAt(index);
        endRemoveRows();
    }
}

int AudioDeviceModel::findDeviceIndex(const QString& deviceId) const
{
    for (int i = 0; i < m_devices.count(); ++i) {
        if (m_devices[i].id == deviceId) {
            return i;
        }
    }
    return -1;
}

// DeviceNotificationClient implementation
DeviceNotificationClient::DeviceNotificationClient(AudioWorker* worker)
    : m_cRef(1), m_worker(worker)
{
}

DeviceNotificationClient::~DeviceNotificationClient()
{
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::QueryInterface(REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid) {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    } else if (__uuidof(IMMNotificationClient) == riid) {
        AddRef();
        *ppvInterface = (IMMNotificationClient*)this;
    } else {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

ULONG STDMETHODCALLTYPE DeviceNotificationClient::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE DeviceNotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef) {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
    //QString deviceId = QString::fromWCharArray(pwstrDeviceId);

    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "enumerateDevices", Qt::QueuedConnection);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
    QString deviceId = QString::fromWCharArray(pwstrDeviceId);

    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "onDeviceAdded", Qt::QueuedConnection,
                                  Q_ARG(QString, deviceId));
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
    QString deviceId = QString::fromWCharArray(pwstrDeviceId);

    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "onDeviceRemoved", Qt::QueuedConnection,
                                  Q_ARG(QString, deviceId));
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
{
    QString deviceId = pwstrDefaultDeviceId ? QString::fromWCharArray(pwstrDefaultDeviceId) : QString();
    AudioWorker::DataFlow dataFlow = AudioWorker::fromWindowsDataFlow(flow);

    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "onDefaultDeviceChanged", Qt::QueuedConnection,
                                  Q_ARG(AudioWorker::DataFlow, dataFlow),
                                  Q_ARG(QString, deviceId));
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DeviceNotificationClient::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "enumerateDevices", Qt::QueuedConnection);
    }
    return S_OK;
}

// VolumeNotificationClient implementation
VolumeNotificationClient::VolumeNotificationClient(AudioWorker* worker, EDataFlow dataFlow)
    : m_cRef(1), m_worker(worker), m_dataFlow(dataFlow)
{
}

VolumeNotificationClient::~VolumeNotificationClient()
{
}

HRESULT STDMETHODCALLTYPE VolumeNotificationClient::QueryInterface(REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid) {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    } else if (__uuidof(IAudioEndpointVolumeCallback) == riid) {
        AddRef();
        *ppvInterface = (IAudioEndpointVolumeCallback*)this;
    } else {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

ULONG STDMETHODCALLTYPE VolumeNotificationClient::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE VolumeNotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef) {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE VolumeNotificationClient::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify)
{
    if (m_worker && pNotify) {
        float volume = pNotify->fMasterVolume;
        bool muted = pNotify->bMuted;

        AudioWorker::DataFlow dataFlow = AudioWorker::fromWindowsDataFlow(m_dataFlow);

        QMetaObject::invokeMethod(m_worker, "onVolumeChanged", Qt::QueuedConnection,
                                  Q_ARG(AudioWorker::DataFlow, dataFlow),
                                  Q_ARG(float, volume),
                                  Q_ARG(bool, muted));
    }
    return S_OK;
}

// SessionNotificationClient implementation
SessionNotificationClient::SessionNotificationClient(AudioWorker* worker)
    : m_cRef(1), m_worker(worker)
{
}

SessionNotificationClient::~SessionNotificationClient()
{
}

HRESULT STDMETHODCALLTYPE SessionNotificationClient::QueryInterface(REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid) {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    } else if (__uuidof(IAudioSessionNotification) == riid) {
        AddRef();
        *ppvInterface = (IAudioSessionNotification*)this;
    } else {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

ULONG STDMETHODCALLTYPE SessionNotificationClient::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE SessionNotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef) {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE SessionNotificationClient::OnSessionCreated(IAudioSessionControl *NewSession)
{
    if (!NewSession || !m_worker) {
        return S_OK;
    }

    // Get the process ID of the new session
    CComPtr<IAudioSessionControl2> sessionControl2;
    HRESULT hr = NewSession->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);
    if (SUCCEEDED(hr)) {
        DWORD processId = 0;
        hr = sessionControl2->GetProcessId(&processId);

        if (SUCCEEDED(hr)) {
            // Only skip our own process (audio feedback)
            if (processId == GetCurrentProcessId()) {
                return S_OK;
            }
        }
    }

    // Always enumerate when a new session is created
    // This ensures we don't miss new streams from existing processes
    QMetaObject::invokeMethod(m_worker, "enumerateApplications", Qt::QueuedConnection);

    return S_OK;
}

// SessionEventsClient implementation
SessionEventsClient::SessionEventsClient(AudioWorker* worker, const QString& appId)
    : m_cRef(1), m_worker(worker), m_appId(appId)
{
}

SessionEventsClient::~SessionEventsClient()
{
}

HRESULT STDMETHODCALLTYPE SessionEventsClient::QueryInterface(REFIID riid, VOID **ppvInterface)
{
    if (IID_IUnknown == riid) {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    } else if (__uuidof(IAudioSessionEvents) == riid) {
        AddRef();
        *ppvInterface = (IAudioSessionEvents*)this;
    } else {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

ULONG STDMETHODCALLTYPE SessionEventsClient::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE SessionEventsClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&m_cRef);
    if (0 == ulRef) {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE SessionEventsClient::OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "onApplicationSessionVolumeChanged", Qt::QueuedConnection,
                                                 Q_ARG(QString, m_appId),
                                                 Q_ARG(float, NewVolume),
                                                 Q_ARG(bool, NewMute));
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SessionEventsClient::OnStateChanged(AudioSessionState NewState)
{

    if (NewState == AudioSessionStateExpired) {
        if (m_worker) {
            QMetaObject::invokeMethod(m_worker, "onSessionDisconnected", Qt::QueuedConnection);
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SessionEventsClient::OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "onSessionDisconnected", Qt::QueuedConnection);
    }
    return S_OK;
}

// Stub implementations for other required methods
HRESULT STDMETHODCALLTYPE SessionEventsClient::OnDisplayNameChanged(LPCWSTR NewDisplayName, LPCGUID EventContext) { return S_OK; }
HRESULT STDMETHODCALLTYPE SessionEventsClient::OnIconPathChanged(LPCWSTR NewIconPath, LPCGUID EventContext) { return S_OK; }
HRESULT STDMETHODCALLTYPE SessionEventsClient::OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolumeArray[], DWORD ChangedChannel, LPCGUID EventContext) { return S_OK; }
HRESULT STDMETHODCALLTYPE SessionEventsClient::OnGroupingParamChanged(LPCGUID NewGroupingParam, LPCGUID EventContext) { return S_OK; }

// AudioWorker implementation
AudioWorker::AudioWorker()
    : m_deviceEnumerator(nullptr)
    , m_outputVolumeClient(nullptr)
    , m_inputVolumeClient(nullptr)
    , m_sessionNotificationClient(nullptr)
    , m_deviceNotificationClient(nullptr)
    , m_sessionManager(nullptr)
    , m_policyConfig(nullptr)
    , m_outputVolumeControl(nullptr)
    , m_inputVolumeControl(nullptr)
    , m_outputVolume(0)
    , m_inputVolume(0)
    , m_outputMuted(false)
    , m_inputMuted(false)
    , m_audioLevelTimer(nullptr)
    , m_sessionManagerInvalid(false)
    , m_bluetoothBatteryMonitor(nullptr)
    , m_headsetControlMonitor(nullptr)
{
    qRegisterMetaType<AudioApplication>("AudioApplication");
    qRegisterMetaType<QList<AudioApplication>>("QList<AudioApplication>");
    qRegisterMetaType<AudioDevice>("AudioDevice");
    qRegisterMetaType<QList<AudioDevice>>("QList<AudioDevice>");

    m_bluetoothBatteryMonitor = BluetoothBatteryMonitor::instance();
    connect(m_bluetoothBatteryMonitor, &BluetoothBatteryMonitor::deviceBatteryUpdated,
            this, &AudioWorker::onBluetoothDeviceBatteryUpdated);

    m_headsetControlMonitor = new HeadsetControlMonitor(this);
    connect(m_headsetControlMonitor, &HeadsetControlMonitor::headsetDataUpdated,
            this, &AudioWorker::onHeadsetDataUpdated);

    m_bluetoothBatteryMonitor->startMonitoring();
}

AudioWorker::~AudioWorker()
{
    cleanup();
}

void AudioWorker::initialize()
{

    HRESULT hr = initializeCOM();
    if (FAILED(hr)) {
        qDebug() << "Failed to initialize COM, HRESULT:" << QString::number(hr, 16);
        return;
    }

    // Create device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&m_deviceEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to create device enumerator, HRESULT:" << QString::number(hr, 16);
        return;
    }

    // Create policy config for setting default devices
    hr = CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_ALL,
                          __uuidof(IPolicyConfig), (void**)&m_policyConfig);
    if (FAILED(hr)) {
        qDebug() << "Failed to create policy config, trying Vista version. HRESULT:" << QString::number(hr, 16);
        hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), nullptr, CLSCTX_ALL,
                              __uuidof(IPolicyConfigVista), (void**)&m_policyConfig);
        if (FAILED(hr)) {
            qDebug() << "Failed to create Vista policy config, HRESULT:" << QString::number(hr, 16);
            m_policyConfig = nullptr;
        }
    }

    // Set up notifications
    setupVolumeNotifications();
    setupSessionNotifications();
    setupDeviceNotifications();

    // Get initial state
    updateCurrentVolumes();
    enumerateDevices();
    enumerateApplications();

    emit initializationComplete();
}

void AudioWorker::cleanup()
{
    if (m_bluetoothBatteryMonitor) {
        m_bluetoothBatteryMonitor->stopMonitoring();
    }

    if (m_audioLevelTimer) {
        m_audioLevelTimer->stop();
        delete m_audioLevelTimer;
        m_audioLevelTimer = nullptr;
    }

    for (auto it = m_sessionMeterControls.begin(); it != m_sessionMeterControls.end(); ++it) {
        if (it.value()) {
            it.value()->Release();
        }
    }
    m_sessionMeterControls.clear();
    m_applicationAudioLevels.clear();

    // Clean up volume controls cache
    for (auto it = m_sessionVolumeControls.begin(); it != m_sessionVolumeControls.end(); ++it) {
        if (it.value()) {
            it.value()->Release();
        }
    }
    m_sessionVolumeControls.clear();

    // Clean up session notifications SECOND
    if (m_sessionManager && m_sessionNotificationClient) {
        m_sessionManager->UnregisterSessionNotification(m_sessionNotificationClient);
        m_sessionNotificationClient->Release();
        m_sessionNotificationClient = nullptr;
        m_sessionManager->Release();
        m_sessionManager = nullptr;
    }

    // Clean up device notifications
    if (m_deviceEnumerator && m_deviceNotificationClient) {
        m_deviceEnumerator->UnregisterEndpointNotificationCallback(m_deviceNotificationClient);
    }
    if (m_deviceNotificationClient) {
        m_deviceNotificationClient->Release();
        m_deviceNotificationClient = nullptr;
    }

    // Clean up active sessions
    for (auto& sessionInfo : m_activeSessions) {
        if (sessionInfo.sessionControl && sessionInfo.eventsClient) {
            sessionInfo.sessionControl->UnregisterAudioSessionNotification(sessionInfo.eventsClient);
        }
        if (sessionInfo.eventsClient) {
            sessionInfo.eventsClient->Release();
        }
        if (sessionInfo.sessionControl) {
            sessionInfo.sessionControl->Release();
        }
    }
    m_activeSessions.clear();

    // Clean up session event clients (legacy)
    for (auto it = m_sessionEventClients.begin(); it != m_sessionEventClients.end(); ++it) {
        it.value()->Release();
    }
    m_sessionEventClients.clear();

    // Cleanup volume notifications
    if (m_outputVolumeControl && m_outputVolumeClient) {
        m_outputVolumeControl->UnregisterControlChangeNotify(m_outputVolumeClient);
        m_outputVolumeControl->Release();
        m_outputVolumeControl = nullptr;
    }

    if (m_inputVolumeControl && m_inputVolumeClient) {
        m_inputVolumeControl->UnregisterControlChangeNotify(m_inputVolumeClient);
        m_inputVolumeControl->Release();
        m_inputVolumeControl = nullptr;
    }

    if (m_outputVolumeClient) {
        m_outputVolumeClient->Release();
        m_outputVolumeClient = nullptr;
    }

    if (m_inputVolumeClient) {
        m_inputVolumeClient->Release();
        m_inputVolumeClient = nullptr;
    }

    if (m_policyConfig) {
        m_policyConfig->Release();
        m_policyConfig = nullptr;
    }

    if (m_deviceEnumerator) {
        m_deviceEnumerator->Release();
        m_deviceEnumerator = nullptr;
    }

    CoUninitialize();
}

void AudioWorker::onBluetoothDeviceBatteryUpdated(const BluetoothDeviceBattery& bluetoothDevice)
{
    updateDevicesWithBluetoothBattery(bluetoothDevice);
    emit devicesChanged(m_devices);
}

void AudioWorker::updateDevicesWithBluetoothBattery(const BluetoothDeviceBattery& bluetoothDevice)
{
    for (AudioDevice& audioDevice : m_devices) {
        if (!audioDevice.macAddress.isEmpty() && !bluetoothDevice.macAddress.isEmpty()) {
            if (normalizeBluetoothAddress(audioDevice.macAddress) ==
                normalizeBluetoothAddress(bluetoothDevice.macAddress)) {
                updateAudioDeviceBatteryFromBluetooth(audioDevice, bluetoothDevice);
                break;
            }
        }
    }
}

void AudioWorker::updateAudioDeviceBatteryFromBluetooth(AudioDevice& audioDevice, const BluetoothDeviceBattery& bluetoothDevice)
{
    if (bluetoothDevice.hasBatteryInfo && bluetoothDevice.batteryLevel >= 0) {
        audioDevice.batteryPercentage = bluetoothDevice.batteryLevel;

        if (bluetoothDevice.batteryStatus == "AVAILABLE") {
            audioDevice.batteryStatus = "BATTERY_AVAILABLE";
        } else if (bluetoothDevice.batteryStatus == "CHARGING") {
            audioDevice.batteryStatus = "BATTERY_CHARGING";
        } else if (bluetoothDevice.batteryStatus == "CRITICAL") {
            audioDevice.batteryStatus = "BATTERY_AVAILABLE";
        } else {
            audioDevice.batteryStatus = "BATTERY_UNAVAILABLE";
        }

        if (!bluetoothDevice.macAddress.isEmpty()) {
            audioDevice.macAddress = bluetoothDevice.macAddress;
        }
        if (!bluetoothDevice.vendorId.isEmpty()) {
            audioDevice.vendorId = bluetoothDevice.vendorId;
        }
        if (!bluetoothDevice.productId.isEmpty()) {
            audioDevice.productId = bluetoothDevice.productId;
        }
    } else if (!bluetoothDevice.isConnected) {
        audioDevice.batteryPercentage = -1;
        audioDevice.batteryStatus = "BATTERY_UNAVAILABLE";
    }
}

QString AudioWorker::normalizeBluetoothAddress(const QString& address)
{
    // Normalize to uppercase with colons: XX:XX:XX:XX:XX:XX
    QString normalized = address.toUpper();
    normalized.replace("-", ":");
    return normalized;
}

void AudioWorker::updateAllBluetoothDeviceBatteries()
{
    if (!m_bluetoothBatteryMonitor) {
        return;
    }

    QList<BluetoothDeviceBattery> bluetoothDevices = m_bluetoothBatteryMonitor->getCachedDevices();
    for (const BluetoothDeviceBattery& bluetoothDevice : bluetoothDevices) {
        updateDevicesWithBluetoothBattery(bluetoothDevice);
    }
}

void AudioWorker::onHeadsetDataUpdated(const QList<HeadsetControlDevice>& headsetDevices)
{
    updateDevicesBatteryInfo(headsetDevices);
    emit devicesChanged(m_devices);
}

void AudioWorker::updateDevicesBatteryInfo(const QList<HeadsetControlDevice>& headsetDevices)
{

    for (AudioDevice& audioDevice : m_devices) {
        bool foundMatch = false;
        bool hadHeadsetBattery = (audioDevice.batteryPercentage != -1 && audioDevice.macAddress.isEmpty());

        // Find matching headset by VID/PID (only if we have devices to check)
        for (const HeadsetControlDevice& headsetDevice : headsetDevices) {
            QString deviceVid = headsetDevice.vendorId.startsWith("0x") ? headsetDevice.vendorId.mid(2) : headsetDevice.vendorId;
            QString devicePid = headsetDevice.productId.startsWith("0x") ? headsetDevice.productId.mid(2) : headsetDevice.productId;
            QString audioVid = audioDevice.vendorId.startsWith("0x") ? audioDevice.vendorId.mid(2) : audioDevice.vendorId;
            QString audioPid = audioDevice.productId.startsWith("0x") ? audioDevice.productId.mid(2) : audioDevice.productId;

            if (deviceVid.compare(audioVid, Qt::CaseInsensitive) == 0 &&
                devicePid.compare(audioPid, Qt::CaseInsensitive) == 0) {
                audioDevice.batteryPercentage = headsetDevice.batteryLevel;
                audioDevice.batteryStatus = headsetDevice.batteryStatus;
                foundMatch = true;
                break;
            }
        }

        // Reset battery info for non-Bluetooth devices that had HeadsetControl battery but no longer match
        if (hadHeadsetBattery && !foundMatch) {
            audioDevice.batteryPercentage = -1;
            audioDevice.batteryStatus = "BATTERY_UNAVAILABLE";
        }
    }

    // Update with current Bluetooth devices (this should preserve Bluetooth battery info)
    updateAllBluetoothDeviceBatteries();
}

bool AudioWorker::hasProcessId(DWORD processId)
{
    for (const AudioApplication& app : m_applications) {
        // Extract process ID from app.id (format: "processId_sessionPtr")
        QString pidStr = app.id.section('_', 0, 0);
        if (pidStr.toUInt() == processId) {
            return true;
        }
    }
    return false;
}

void AudioWorker::initializeAudioLevelTimer()
{
    if (!m_audioLevelTimer) {
        m_audioLevelTimer = new QTimer(this);
        connect(m_audioLevelTimer, &QTimer::timeout, this, &AudioWorker::updateAudioLevels);
    }
}

void AudioWorker::startAudioLevelMonitoring()
{
    if (!m_audioLevelTimer) {
        initializeAudioLevelTimer();
    }
    m_audioLevelTimer->start(100); // Update every 100ms
}

void AudioWorker::stopAudioLevelMonitoring()
{
    if (m_audioLevelTimer) {
        m_audioLevelTimer->stop();
    }
}

void AudioWorker::updateAudioLevels()
{
    // Get device audio levels
    int outputLevel = getDeviceAudioLevel(eRender);
    int inputLevel = getDeviceAudioLevel(eCapture);

    emit outputAudioLevelChanged(outputLevel);
    emit inputAudioLevelChanged(inputLevel);

    // Add application levels
    updateApplicationAudioLevels();
}

// Add these helper methods to AudioWorker
int AudioWorker::getDeviceAudioLevel(EDataFlow dataFlow)
{
    if (!m_deviceEnumerator) return 0;

    CComPtr<IMMDevice> device;
    HRESULT hr = m_deviceEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &device);
    if (FAILED(hr)) return 0;

    CComPtr<IAudioMeterInformation> meterInfo;
    hr = device->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, nullptr, (void**)&meterInfo);
    if (FAILED(hr)) return 0;

    float peakLevel = 0.0f;
    hr = meterInfo->GetPeakValue(&peakLevel);
    if (FAILED(hr)) return 0;

    return static_cast<int>(peakLevel * 100);
}

HRESULT AudioWorker::initializeCOM()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (hr == RPC_E_CHANGED_MODE) {
        CoUninitialize();
        hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    }
    return hr;
}

void AudioWorker::setupVolumeNotifications()
{
    // Clean up existing volume notifications first
    if (m_outputVolumeControl && m_outputVolumeClient) {
        m_outputVolumeControl->UnregisterControlChangeNotify(m_outputVolumeClient);
        m_outputVolumeControl->Release();
        m_outputVolumeControl = nullptr;
        m_outputVolumeClient->Release();
        m_outputVolumeClient = nullptr;
    }

    if (m_inputVolumeControl && m_inputVolumeClient) {
        m_inputVolumeControl->UnregisterControlChangeNotify(m_inputVolumeClient);
        m_inputVolumeControl->Release();
        m_inputVolumeControl = nullptr;
        m_inputVolumeClient->Release();
        m_inputVolumeClient = nullptr;
    }

    if (!m_deviceEnumerator) return;

    // Setup output volume notifications
    CComPtr<IMMDevice> outputDevice;
    HRESULT hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &outputDevice);
    if (SUCCEEDED(hr)) {
        hr = outputDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                                    nullptr, (void**)&m_outputVolumeControl);
        if (SUCCEEDED(hr)) {
            m_outputVolumeClient = new VolumeNotificationClient(this, eRender);
            m_outputVolumeControl->RegisterControlChangeNotify(m_outputVolumeClient);
        }
    }

    // Setup input volume notifications
    CComPtr<IMMDevice> inputDevice;
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &inputDevice);
    if (SUCCEEDED(hr)) {
        hr = inputDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                                   nullptr, (void**)&m_inputVolumeControl);
        if (SUCCEEDED(hr)) {
            m_inputVolumeClient = new VolumeNotificationClient(this, eCapture);
            m_inputVolumeControl->RegisterControlChangeNotify(m_inputVolumeClient);
        }
    }

    // Update current volumes for the new devices
    updateCurrentVolumes();
}

void AudioWorker::setupSessionNotifications()
{
    // Clean up existing session notifications first
    if (m_sessionManager && m_sessionNotificationClient) {
        m_sessionManager->UnregisterSessionNotification(m_sessionNotificationClient);
        m_sessionNotificationClient->Release();
        m_sessionNotificationClient = nullptr;
        m_sessionManager->Release();
        m_sessionManager = nullptr;
    }

    if (!m_deviceEnumerator) {
        qDebug() << "ERROR: No device enumerator available";
        return;
    }

    CComPtr<IMMDevice> device;
    HRESULT hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) {
        qDebug() << "ERROR: Failed to get default audio endpoint, HRESULT:" << QString::number(hr, 16);
        return;
    }

    hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL,
                          nullptr, (void**)&m_sessionManager);
    if (FAILED(hr)) {
        qDebug() << "ERROR: Failed to activate session manager, HRESULT:" << QString::number(hr, 16);
        return;
    }

    m_sessionNotificationClient = new SessionNotificationClient(this);

    hr = m_sessionManager->RegisterSessionNotification(m_sessionNotificationClient);
    if (SUCCEEDED(hr)) {
        m_sessionManagerInvalid = false; // Clear the invalid flag on successful setup
    } else {
        qDebug() << "ERROR: Failed to register session notifications, HRESULT:" << QString::number(hr, 16);
        m_sessionNotificationClient->Release();
        m_sessionNotificationClient = nullptr;
    }
}

int AudioWorker::getApplicationAudioLevel(const QString& appId)
{
    if (appId == "system_sounds") {
        return 0;
    }

    auto it = m_sessionMeterControls.find(appId);
    if (it == m_sessionMeterControls.end()) {
        return 0;
    }

    float peakLevel = 0.0f;
    HRESULT hr = it.value()->GetPeakValue(&peakLevel);
    if (FAILED(hr)) {
        return 0;
    }

    return static_cast<int>(peakLevel * 100);
}

void AudioWorker::updateApplicationAudioLevels()
{
    for (auto it = m_sessionMeterControls.begin(); it != m_sessionMeterControls.end(); ++it) {
        int newLevel = getApplicationAudioLevel(it.key());
        int cachedLevel = m_applicationAudioLevels.value(it.key(), -1);

        if (newLevel != cachedLevel) {
            m_applicationAudioLevels[it.key()] = newLevel;
            emit applicationAudioLevelChanged(it.key(), newLevel);
        }
    }
}

void AudioWorker::startApplicationAudioLevelMonitoring()
{
    if (!m_audioLevelTimer) {
        initializeAudioLevelTimer();
    }
    m_audioLevelTimer->start(100);
}

void AudioWorker::stopApplicationAudioLevelMonitoring()
{
    if (m_audioLevelTimer) {
        m_audioLevelTimer->stop();
    }
}

void AudioWorker::setupDeviceNotifications()
{
    if (!m_deviceEnumerator) return;

    m_deviceNotificationClient = new DeviceNotificationClient(this);
    HRESULT hr = m_deviceEnumerator->RegisterEndpointNotificationCallback(m_deviceNotificationClient);
    if (SUCCEEDED(hr)) {
    } else {
        qDebug() << "Failed to register device notifications, HRESULT:" << QString::number(hr, 16);
        m_deviceNotificationClient->Release();
        m_deviceNotificationClient = nullptr;
    }
}

void AudioWorker::updateCurrentVolumes()
{
    // Get current output volume and mute state
    if (m_outputVolumeControl) {
        float volume = 0.0f;
        BOOL muted = FALSE;

        if (SUCCEEDED(m_outputVolumeControl->GetMasterVolumeLevelScalar(&volume))) {
            m_outputVolume = static_cast<int>(std::round(volume * 100.0f));
            emit outputVolumeChanged(m_outputVolume);
        }

        if (SUCCEEDED(m_outputVolumeControl->GetMute(&muted))) {
            m_outputMuted = muted;
            emit outputMuteChanged(m_outputMuted);
        }
    }

    // Get current input volume and mute state
    if (m_inputVolumeControl) {
        float volume = 0.0f;
        BOOL muted = FALSE;

        if (SUCCEEDED(m_inputVolumeControl->GetMasterVolumeLevelScalar(&volume))) {
            m_inputVolume = static_cast<int>(std::round(volume * 100.0f));
            emit inputVolumeChanged(m_inputVolume);
        }

        if (SUCCEEDED(m_inputVolumeControl->GetMute(&muted))) {
            m_inputMuted = muted;
            emit inputMuteChanged(m_inputMuted);
        }
    }
}

void AudioWorker::updateDevicesBatteryInfoFromCache()
{
    if (!m_bluetoothBatteryMonitor) {
        return;
    }

    QList<BluetoothDeviceBattery> cachedBluetoothDevices = m_bluetoothBatteryMonitor->getCachedDevices();

    for (const BluetoothDeviceBattery& bluetoothDevice : cachedBluetoothDevices) {
        updateDevicesWithBluetoothBattery(bluetoothDevice);
    }
}

void AudioWorker::enumerateDevices()
{
    if (!m_deviceEnumerator) {
        qDebug() << "No device enumerator available";
        return;
    }

    QList<AudioDevice> newDevices;

    // Enumerate both input and output devices
    for (int dataFlowIndex = 0; dataFlowIndex < 2; ++dataFlowIndex) {
        EDataFlow dataFlow = (dataFlowIndex == 0) ? eRender : eCapture;
        QString flowName = (dataFlow == eRender) ? "Output" : "Input";

        CComPtr<IMMDeviceCollection> deviceCollection;
        HRESULT hr = m_deviceEnumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &deviceCollection);
        if (FAILED(hr)) {
            qDebug() << "Failed to enumerate" << flowName << "devices, HRESULT:" << QString::number(hr, 16);
            continue;
        }

        UINT deviceCount = 0;
        deviceCollection->GetCount(&deviceCount);

        CComPtr<IMMDevice> defaultDevice;
        CComPtr<IMMDevice> defaultCommDevice;
        m_deviceEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &defaultDevice);
        m_deviceEnumerator->GetDefaultAudioEndpoint(dataFlow, eCommunications, &defaultCommDevice);

        LPWSTR defaultDeviceId = nullptr;
        LPWSTR defaultCommDeviceId = nullptr;
        if (defaultDevice) {
            defaultDevice->GetId(&defaultDeviceId);
        }
        if (defaultCommDevice) {
            defaultCommDevice->GetId(&defaultCommDeviceId);
        }

        for (UINT i = 0; i < deviceCount; ++i) {
            CComPtr<IMMDevice> device;
            hr = deviceCollection->Item(i, &device);
            if (FAILED(hr)) continue;

            AudioDevice audioDevice = createAudioDeviceFromInterface(device, dataFlow);
            if (!audioDevice.id.isEmpty()) {
                if (defaultDeviceId && audioDevice.id == QString::fromWCharArray(defaultDeviceId)) {
                    audioDevice.isDefault = true;
                }
                if (defaultCommDeviceId && audioDevice.id == QString::fromWCharArray(defaultCommDeviceId)) {
                    audioDevice.isDefaultCommunication = true;
                }

                newDevices.append(audioDevice);
            }
        }

        if (defaultDeviceId) {
            CoTaskMemFree(defaultDeviceId);
        }
        if (defaultCommDeviceId) {
            CoTaskMemFree(defaultCommDeviceId);
        }
    }

    m_devices = newDevices;

    updateDevicesBatteryInfoFromCache();

    if (m_headsetControlMonitor && m_headsetControlMonitor->isMonitoring()) {
        QList<HeadsetControlDevice> cachedDevices = m_headsetControlMonitor->getCachedDevices();
        if (!cachedDevices.isEmpty()) {
            updateDevicesBatteryInfo(cachedDevices);
        }
        // Trigger a fresh fetch
        QMetaObject::invokeMethod(m_headsetControlMonitor,
                                  "fetchHeadsetInfo",
                                  Qt::QueuedConnection);
    } else if (m_headsetControlMonitor) {
        // If monitoring is stopped, clear any headset battery info
        updateDevicesBatteryInfo(QList<HeadsetControlDevice>());
    }

    emit devicesChanged(m_devices);
}

AudioDevice AudioWorker::createAudioDeviceFromInterface(IMMDevice* device, EDataFlow dataFlow)
{
    AudioDevice audioDevice;
    if (!device) return audioDevice;

    // Get device ID
    LPWSTR deviceId = nullptr;
    HRESULT hr = device->GetId(&deviceId);
    if (SUCCEEDED(hr) && deviceId) {
        audioDevice.id = QString::fromWCharArray(deviceId);
        CoTaskMemFree(deviceId);
    }

    // Get device state
    DWORD state = 0;
    hr = device->GetState(&state);
    if (SUCCEEDED(hr)) {
        switch (state) {
        case DEVICE_STATE_ACTIVE:
            audioDevice.state = "Active";
            break;
        case DEVICE_STATE_DISABLED:
            audioDevice.state = "Disabled";
            break;
        case DEVICE_STATE_NOTPRESENT:
            audioDevice.state = "Not Present";
            break;
        case DEVICE_STATE_UNPLUGGED:
            audioDevice.state = "Unplugged";
            break;
        default:
            audioDevice.state = "Unknown";
            break;
        }
    }

    // Get device properties
    audioDevice.name = getDeviceProperty(device, PKEY_Device_FriendlyName);
    audioDevice.description = getDeviceProperty(device, PKEY_Device_DeviceDesc);
    audioDevice.isInput = (dataFlow == eCapture);

    if (audioDevice.name.isEmpty()) {
        audioDevice.name = audioDevice.description;
    }
    if (audioDevice.name.isEmpty()) {
        audioDevice.name = "Unknown Device";
    }

    // Extract USB VID/PID and Bluetooth MAC address information
    CComPtr<IPropertyStore> propertyStore;
    hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
    if (SUCCEEDED(hr)) {
        DWORD propertyCount = 0;
        hr = propertyStore->GetCount(&propertyCount);
        if (SUCCEEDED(hr)) {
            for (DWORD i = 0; i < propertyCount && audioDevice.macAddress.isEmpty(); ++i) {
                PROPERTYKEY propertyKey;
                PROPVARIANT propVariant;
                PropVariantInit(&propVariant);

                hr = propertyStore->GetAt(i, &propertyKey);
                if (SUCCEEDED(hr)) {
                    hr = propertyStore->GetValue(propertyKey, &propVariant);
                    if (SUCCEEDED(hr) && propVariant.vt == VT_LPWSTR) {
                        QString propValue = QString::fromWCharArray(propVariant.pwszVal);

                        // Look for USB VID/PID pattern
                        if (audioDevice.vendorId.isEmpty() && propValue.contains("VID_") && propValue.contains("PID_")) {
                            QRegularExpression regex(R"(VID_([0-9A-F]{4}).*PID_([0-9A-F]{4}))");
                            QRegularExpressionMatch match = regex.match(propValue);
                            if (match.hasMatch()) {
                                audioDevice.vendorId = match.captured(1);
                                audioDevice.productId = match.captured(2);
                            }
                        }

                        // Look for Bluetooth MAC address (BTHENUM device path)
                        if (propValue.contains("BTHENUM", Qt::CaseInsensitive)) {
                            // Primary: Look for device MAC after &0& in BTHENUM path
                            QRegularExpression deviceMacRegex(R"(&0&([0-9A-Fa-f]{12})(_[cC]|\#))");
                            QRegularExpressionMatch match = deviceMacRegex.match(propValue);

                            if (match.hasMatch()) {
                                QString macCandidate = match.captured(1);
                                QString formattedMac = QString("%1:%2:%3:%4:%5:%6")
                                                           .arg(macCandidate.mid(0, 2))
                                                           .arg(macCandidate.mid(2, 2))
                                                           .arg(macCandidate.mid(4, 2))
                                                           .arg(macCandidate.mid(6, 2))
                                                           .arg(macCandidate.mid(8, 2))
                                                           .arg(macCandidate.mid(10, 2));

                                audioDevice.macAddress = normalizeBluetoothAddress(formattedMac);
                            }
                        }
                    }
                }
                PropVariantClear(&propVariant);
            }
        }
    }

    return audioDevice;
}

QString AudioWorker::getDeviceProperty(IMMDevice* device, const PROPERTYKEY& key)
{
    if (!device) return QString();

    CComPtr<IPropertyStore> propertyStore;
    HRESULT hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
    if (FAILED(hr)) return QString();

    PROPVARIANT propVariant;
    PropVariantInit(&propVariant);

    hr = propertyStore->GetValue(key, &propVariant);
    if (SUCCEEDED(hr) && propVariant.vt == VT_LPWSTR) {
        QString result = QString::fromWCharArray(propVariant.pwszVal);
        PropVariantClear(&propVariant);
        return result;
    }

    PropVariantClear(&propVariant);
    return QString();
}

void AudioWorker::enumerateApplications()
{
    if (!ensureValidSessionManager()) {
        qDebug() << "Failed to restore session manager, skipping application enumeration";
        return;
    }

    // Clean up existing volume controls cache
    for (auto it = m_sessionVolumeControls.begin(); it != m_sessionVolumeControls.end(); ++it) {
        if (it.value()) {
            it.value()->Release();
        }
    }
    m_sessionVolumeControls.clear();

    // Clean up existing meter controls
    for (auto it = m_sessionMeterControls.begin(); it != m_sessionMeterControls.end(); ++it) {
        if (it.value()) {
            it.value()->Release();
        }
    }
    m_sessionMeterControls.clear();

    // Clean up existing sessions
    for (auto& sessionInfo : m_activeSessions) {
        if (sessionInfo.sessionControl && sessionInfo.eventsClient) {
            sessionInfo.sessionControl->UnregisterAudioSessionNotification(sessionInfo.eventsClient);
        }
        if (sessionInfo.eventsClient) {
            sessionInfo.eventsClient->Release();
        }
        if (sessionInfo.sessionControl) {
            sessionInfo.sessionControl->Release();
        }
    }
    m_activeSessions.clear();

    QList<AudioApplication> newApplications;
    bool foundSystemSounds = false;

    // Use the existing session manager if available, otherwise create a new one
    IAudioSessionManager2* sessionManager = m_sessionManager;
    CComPtr<IAudioSessionManager2> tempSessionManager;

    if (!sessionManager) {
        // If we don't have the main session manager, create a temporary one
        CComPtr<IMMDeviceEnumerator> pEnumerator;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                      __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (FAILED(hr)) {
            qDebug() << "Failed to create device enumerator";
            return;
        }

        CComPtr<IMMDevice> pDevice;
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        if (FAILED(hr)) {
            qDebug() << "Failed to get default audio endpoint";
            return;
        }

        hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&tempSessionManager);
        if (FAILED(hr)) {
            qDebug() << "Failed to get audio session manager";
            return;
        }
        sessionManager = tempSessionManager;
    }

    CComPtr<IAudioSessionEnumerator> pSessionEnumerator;
    HRESULT hr = sessionManager->GetSessionEnumerator(&pSessionEnumerator);
    if (FAILED(hr)) {
        qDebug() << "Failed to get session enumerator";
        return;
    }

    int sessionCount = 0;
    pSessionEnumerator->GetCount(&sessionCount);

    // First pass: collect all session data
    struct TempSessionData {
        IAudioSessionControl* sessionControl;
        ISimpleAudioVolume* volumeControl;
        IAudioMeterInformation* meterControl;
        QString appId;
        QString executableName;
        QString finalAppName;
        QString iconPath;
        int volume;
        bool isMuted;
        bool isSystemSounds;
        DWORD processId;
        int sessionIndex;
        QString sessionDisplayName;
        uintptr_t sessionControlPtr;
    };

    QList<TempSessionData> tempSessions;

    for (int i = 0; i < sessionCount; ++i) {
        IAudioSessionControl* sessionControl = nullptr;
        hr = pSessionEnumerator->GetSession(i, &sessionControl);
        if (FAILED(hr) || !sessionControl) {
            continue;
        }

        CComPtr<IAudioSessionControl2> sessionControl2;
        hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);
        if (FAILED(hr)) {
            sessionControl->Release();
            continue;
        }

        DWORD processId = 0;
        hr = sessionControl2->GetProcessId(&processId);
        if (FAILED(hr)) {
            sessionControl->Release();
            continue;
        }

        // Get session display name first to check for system sounds
        LPWSTR pwszDisplayName = nullptr;
        hr = sessionControl->GetDisplayName(&pwszDisplayName);
        QString sessionDisplayName = SUCCEEDED(hr) ? QString::fromWCharArray(pwszDisplayName) : "Unknown Application";
        CoTaskMemFree(pwszDisplayName);

        // Check if session is active - but make exception for system sounds
        bool isSystemSounds = (sessionDisplayName == "@%SystemRoot%\\System32\\AudioSrv.Dll,-202" ||
                               processId == 0);

        if (!isSystemSounds && processId == GetCurrentProcessId()) {
            sessionControl->Release();
            continue;
        }

        if (isSystemSounds) {
            foundSystemSounds = true;
        }

        // Get volume info and cache the volume control
        CComPtr<ISimpleAudioVolume> pSimpleAudioVolume;
        hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pSimpleAudioVolume);
        if (FAILED(hr)) {
            sessionControl->Release();
            continue;
        }

        // Get meter info for audio levels - but exclude system sounds
        CComPtr<IAudioMeterInformation> pMeterInfo;
        if (!isSystemSounds) {
            hr = sessionControl->QueryInterface(__uuidof(IAudioMeterInformation), (void**)&pMeterInfo);
            if (FAILED(hr)) {
                qDebug() << "Failed to get meter info for session" << i;
            }
        }

        BOOL isMuted = FALSE;
        pSimpleAudioVolume->GetMute(&isMuted);

        float volumeLevel = 0.0f;
        pSimpleAudioVolume->GetMasterVolume(&volumeLevel);

        // Process application info
        TempSessionData tempData;
        tempData.sessionControl = sessionControl;
        tempData.volumeControl = pSimpleAudioVolume.Detach(); // Detach from CComPtr
        tempData.meterControl = isSystemSounds ? nullptr : pMeterInfo.Detach(); // Only assign meter for non-system sounds
        tempData.volume = static_cast<int>(volumeLevel * 100);
        tempData.isMuted = isMuted;
        tempData.isSystemSounds = isSystemSounds;
        tempData.processId = processId;
        tempData.sessionIndex = i;
        tempData.sessionDisplayName = sessionDisplayName;
        tempData.sessionControlPtr = reinterpret_cast<uintptr_t>(sessionControl);

        QString executablePath;
        QString executableName;
        QString finalAppName = sessionDisplayName;

        // Handle special case for Windows system sounds first
        if (isSystemSounds) {
            finalAppName = tr("System sounds");
            executableName = tr("System sounds");
            tempData.appId = "system_sounds";
            tempData.iconPath = "";
        }
        else {
            // Handle regular applications
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
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
                        finalAppName = executableName;
                    }

                    // Get icon using the improved method
                    QIcon appIcon = getApplicationIcon(executablePath);
                    if (!appIcon.isNull()) {
                        QPixmap pixmap = appIcon.pixmap(32, 32);
                        QByteArray byteArray;
                        QBuffer buffer(&byteArray);
                        buffer.open(QIODevice::WriteOnly);
                        pixmap.save(&buffer, "PNG");
                        tempData.iconPath = "data:image/png;base64," + byteArray.toBase64();
                    }
                }
                CloseHandle(hProcess);
            }

            // Create unique ID using process ID and session control pointer for stability
            tempData.appId = QString::number(processId) + "_" + QString::number(tempData.sessionControlPtr);
        }

        tempData.executableName = executableName;
        tempData.finalAppName = finalAppName;

        if (finalAppName.trimmed().isEmpty()) {
            sessionControl->Release();
            continue;
        }

        tempSessions.append(tempData);
    }

    QMap<QString, QList<TempSessionData*>> executableGroups;

    for (int i = 0; i < tempSessions.count(); ++i) {
        TempSessionData& tempData = tempSessions[i];
        if (!executableGroups.contains(tempData.executableName)) {
            executableGroups[tempData.executableName] = QList<TempSessionData*>();
        }
        executableGroups[tempData.executableName].append(&tempData);
    }

    for (auto& group : executableGroups) {
        std::sort(group.begin(), group.end(),
                  [](const TempSessionData* a, const TempSessionData* b) {
                      // Get memory usage for both processes
                      SIZE_T memoryA = getProcessMemoryUsage(a->processId);
                      SIZE_T memoryB = getProcessMemoryUsage(b->processId);

                      // Sort by memory usage descending (bigger memory = lower index)
                      return memoryA > memoryB;
                  });
    }

    // Third pass: create final applications with proper stream indices
    for (const auto& group : executableGroups) {
        for (int streamIndex = 0; streamIndex < group.count(); ++streamIndex) {
            const TempSessionData* tempData = group[streamIndex];

            AudioApplication app;
            app.id = tempData->appId;
            app.name = tempData->finalAppName;
            app.executableName = tempData->executableName;
            app.iconPath = tempData->iconPath;
            app.volume = tempData->volume;
            app.isMuted = tempData->isMuted;
            app.streamIndex = tempData->isSystemSounds ? 0 : streamIndex;

            // Cache the volume control
            m_sessionVolumeControls[app.id] = tempData->volumeControl;

            // Cache the meter control for audio levels - but only for non-system sounds
            if (tempData->meterControl && !tempData->isSystemSounds) {
                m_sessionMeterControls[app.id] = tempData->meterControl;
            }

            // Register session events for real-time updates
            SessionEventsClient* eventsClient = new SessionEventsClient(this, app.id);
            hr = tempData->sessionControl->RegisterAudioSessionNotification(eventsClient);
            if (SUCCEEDED(hr)) {
                SessionInfo sessionInfo;
                sessionInfo.sessionControl = tempData->sessionControl; // Keep alive
                sessionInfo.eventsClient = eventsClient;
                sessionInfo.appId = app.id;
                m_activeSessions.append(sessionInfo);
            } else {
                eventsClient->Release();
                tempData->sessionControl->Release();
                qDebug() << "✗ Failed to register events for app:" << app.name;
            }

            newApplications.append(app);
        }
    }

    // If we didn't find system sounds in the sessions, create a default entry
    if (!foundSystemSounds) {
        AudioApplication systemApp;
        systemApp.id = "system_sounds";
        systemApp.name = "System sounds";
        systemApp.executableName = "System sounds";
        systemApp.volume = 100; // Default volume
        systemApp.isMuted = false;
        systemApp.iconPath = "";
        systemApp.streamIndex = 0;

        newApplications.append(systemApp);
    }

    m_applications = newApplications;

    emit applicationsChanged(newApplications);
}

QString AudioWorker::getExecutablePath(DWORD processId)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return QString();

    WCHAR path[MAX_PATH];
    DWORD pathSize = MAX_PATH;

    QString result;
    if (QueryFullProcessImageName(hProcess, 0, path, &pathSize)) {
        result = QString::fromWCharArray(path);
    }

    CloseHandle(hProcess);
    return result;
}

QIcon AudioWorker::getApplicationIcon(const QString& executablePath)
{
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

            if (hbmColor) {
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

                // Clean up icon info
                if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
                if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);

                DestroyIcon(hIcon);
                return QIcon(pixmap);
            }

            // Clean up icon info if color bitmap is null
            if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
            if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);

            DeleteDC(hdcMem);
            ReleaseDC(NULL, hdc);
        }
        DestroyIcon(hIcon);
    }
    return QIcon();
}

QString AudioWorker::getApplicationDisplayName(const QString& executablePath)
{
    std::wstring wPath = executablePath.toStdWString();

    DWORD dwSize = GetFileVersionInfoSize(wPath.c_str(), NULL);
    if (dwSize == 0) {
        return QString();
    }

    std::vector<BYTE> buffer(dwSize);
    if (!GetFileVersionInfo(wPath.c_str(), 0, dwSize, buffer.data())) {
        return QString();
    }

    LPVOID lpBuffer = NULL;
    UINT uLen = 0;

    // Try ProductName first
    if (VerQueryValue(buffer.data(), L"\\StringFileInfo\\040904b0\\ProductName", &lpBuffer, &uLen) && uLen > 0) {
        return QString::fromWCharArray(static_cast<LPCWSTR>(lpBuffer));
    }

    // Try FileDescription
    if (VerQueryValue(buffer.data(), L"\\StringFileInfo\\040904b0\\FileDescription", &lpBuffer, &uLen) && uLen > 0) {
        return QString::fromWCharArray(static_cast<LPCWSTR>(lpBuffer));
    }

    return QString();
}

// Event handlers
void AudioWorker::onVolumeChanged(AudioWorker::DataFlow dataFlow, float volume, bool muted)
{
    int volumePercent = static_cast<int>(std::round(volume * 100.0f));

    if (dataFlow == Output) {
        if (m_outputVolume != volumePercent) {
            m_outputVolume = volumePercent;
            emit outputVolumeChanged(volumePercent);
        }
        if (m_outputMuted != muted) {
            m_outputMuted = muted;
            emit outputMuteChanged(muted);
        }
    } else if (dataFlow == Input) {
        if (m_inputVolume != volumePercent) {
            m_inputVolume = volumePercent;
            emit inputVolumeChanged(volumePercent);
        }
        if (m_inputMuted != muted) {
            m_inputMuted = muted;
            emit inputMuteChanged(muted);
        }
    }
}

void AudioWorker::onSessionCreated()
{
    enumerateApplications();
}

void AudioWorker::onSessionDisconnected()
{
    enumerateApplications();
}

void AudioWorker::onDeviceAdded(const QString& deviceId)
{
    enumerateDevices();
}

void AudioWorker::onDeviceRemoved(const QString& deviceId)
{
    enumerateDevices();
}

void AudioWorker::onDefaultDeviceChanged(DataFlow dataFlow, const QString& deviceId)
{
    enumerateDevices();

    // Re-setup volume notifications for the new default devices
    setupVolumeNotifications();

    // Just mark sessions as invalid for output device changes
    if (dataFlow == Output) {
        m_sessionManagerInvalid = true;
    }

    emit defaultDeviceChanged(deviceId, dataFlow == Input);
}

bool AudioWorker::ensureValidSessionManager()
{
    if (!m_sessionManagerInvalid && m_sessionManager) {
        return true; // Already valid
    }

    // Clean up old session manager
    if (m_sessionManager && m_sessionNotificationClient) {
        m_sessionManager->UnregisterSessionNotification(m_sessionNotificationClient);
        m_sessionNotificationClient->Release();
        m_sessionNotificationClient = nullptr;
        m_sessionManager->Release();
        m_sessionManager = nullptr;
    }

    if (!m_deviceEnumerator) {
        return false;
    }

    CComPtr<IMMDevice> device;
    HRESULT hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) {
        return false;
    }

    hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL,
                          nullptr, (void**)&m_sessionManager);
    if (FAILED(hr)) {
        return false;
    }

    m_sessionNotificationClient = new SessionNotificationClient(this);
    hr = m_sessionManager->RegisterSessionNotification(m_sessionNotificationClient);

    if (SUCCEEDED(hr)) {
        m_sessionManagerInvalid = false;
        return true;
    } else {
        m_sessionNotificationClient->Release();
        m_sessionNotificationClient = nullptr;
        return false;
    }
}

void AudioWorker::onApplicationSessionVolumeChanged(const QString& appId, float volume, bool muted)
{
    int volumePercent = static_cast<int>(std::round(volume * 100.0f));
    bool foundApp = false;
    for (int i = 0; i < m_applications.count(); ++i) {
        if (m_applications[i].id == appId) {
            foundApp = true;
            bool volumeChanged = (m_applications[i].volume != volumePercent);
            bool muteChanged = (m_applications[i].isMuted != muted);

            if (volumeChanged || muteChanged) {
                m_applications[i].volume = volumePercent;
                m_applications[i].isMuted = muted;

                if (volumeChanged) {
                    emit applicationVolumeChanged(appId, volumePercent);
                }
                if (muteChanged) {
                    emit applicationMuteChanged(appId, muted);
                }
            }
            break;
        }
    }

    if (!foundApp) {
        QMetaObject::invokeMethod(this, "enumerateApplications", Qt::QueuedConnection);
    }
}

// Control methods
void AudioWorker::setOutputVolume(int volume)
{
    setVolumeForDevice(eRender, volume);
}

void AudioWorker::setInputVolume(int volume)
{
    setVolumeForDevice(eCapture, volume);
}

void AudioWorker::setOutputMute(bool mute)
{
    setMuteForDevice(eRender, mute);
}

void AudioWorker::setInputMute(bool mute)
{
    setMuteForDevice(eCapture, mute);
}

void AudioWorker::setApplicationVolume(const QString& appId, int volume)
{
    auto it = m_sessionVolumeControls.find(appId);
    if (it != m_sessionVolumeControls.end()) {
        float volumeScalar = static_cast<float>(volume) / 100.0f;
        it.value()->SetMasterVolume(volumeScalar, nullptr);
    }
}

void AudioWorker::setApplicationMute(const QString& appId, bool mute)
{
    auto it = m_sessionVolumeControls.find(appId);
    if (it != m_sessionVolumeControls.end()) {
        it.value()->SetMute(mute, nullptr);
    }
}

void AudioWorker::setDefaultDevice(const QString& deviceId, bool isInput, bool forCommunications)
{
    if (!m_policyConfig) {
        qDebug() << "No policy config available, cannot set default device";
        return;
    }

    ERole role = forCommunications ? eCommunications : eConsole;
    std::wstring wDeviceId = deviceId.toStdWString();

    HRESULT hr = m_policyConfig->SetDefaultEndpoint(wDeviceId.c_str(), role);
    if (SUCCEEDED(hr)) {
    } else {
        qDebug() << "Failed to set default device, HRESULT:" << QString::number(hr, 16);
    }
}

void AudioWorker::setVolumeForDevice(EDataFlow dataFlow, int volume)
{
    IAudioEndpointVolume* volumeControl = (dataFlow == eRender) ? m_outputVolumeControl : m_inputVolumeControl;
    if (volumeControl) {
        float volumeScalar = static_cast<float>(volume) / 100.0f;
        volumeControl->SetMasterVolumeLevelScalar(volumeScalar, nullptr);
    }
}

void AudioWorker::setMuteForDevice(EDataFlow dataFlow, bool mute)
{
    IAudioEndpointVolume* volumeControl = (dataFlow == eRender) ? m_outputVolumeControl : m_inputVolumeControl;
    if (volumeControl) {
        volumeControl->SetMute(mute, nullptr);
    }
}

// AudioManager implementation
AudioManager::AudioManager(QObject* parent)
    : QObject(parent)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_cachedOutputVolume(0)
    , m_cachedInputVolume(0)
    , m_cachedOutputMute(false)
    , m_cachedInputMute(false)
{
}

AudioManager::~AudioManager()
{
    cleanup();
}

AudioManager* AudioManager::instance()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance) {
        m_instance = new AudioManager();
    }
    return m_instance;
}

void AudioManager::initialize()
{
    QMutexLocker locker(&m_mutex);

    if (m_workerThread) {
        qDebug() << "AudioManager already initialized";
        return;
    }

    m_workerThread = new QThread(this);
    m_worker = new AudioWorker();

    if (!m_worker) {
        qDebug() << "Failed to create AudioWorker";
        delete m_workerThread;
        m_workerThread = nullptr;
        return;
    }

    connect(m_worker, &AudioWorker::outputVolumeChanged,
            this, &AudioManager::onWorkerOutputVolumeChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::inputVolumeChanged,
            this, &AudioManager::onWorkerInputVolumeChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::outputMuteChanged,
            this, &AudioManager::onWorkerOutputMuteChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::inputMuteChanged,
            this, &AudioManager::onWorkerInputMuteChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::applicationsChanged,
            this, &AudioManager::onWorkerApplicationsChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::devicesChanged,
            this, &AudioManager::onWorkerDevicesChanged, Qt::QueuedConnection);

    connect(m_worker, &AudioWorker::applicationVolumeChanged, this, &AudioManager::applicationVolumeChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::applicationMuteChanged, this, &AudioManager::applicationMuteChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::deviceAdded, this, &AudioManager::deviceAdded, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::deviceRemoved, this, &AudioManager::deviceRemoved, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::defaultDeviceChanged, this, &AudioManager::defaultDeviceChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::initializationComplete, this, &AudioManager::initializationComplete, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::outputAudioLevelChanged, this, &AudioManager::outputAudioLevelChanged, Qt::QueuedConnection);
    connect(m_worker, &AudioWorker::inputAudioLevelChanged, this, &AudioManager::inputAudioLevelChanged, Qt::QueuedConnection);

    connect(m_worker, &AudioWorker::applicationAudioLevelChanged,
            this, &AudioManager::applicationAudioLevelChanged, Qt::QueuedConnection);

    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::started, m_worker, &AudioWorker::initialize);
    m_workerThread->start();
}

void AudioManager::onWorkerOutputVolumeChanged(int volume)
{
    QMutexLocker cacheLock(&m_cacheMutex);
    m_cachedOutputVolume = volume;
    emit outputVolumeChanged(volume);
}

void AudioManager::onWorkerInputVolumeChanged(int volume)
{
    QMutexLocker cacheLock(&m_cacheMutex);
    m_cachedInputVolume = volume;
    emit inputVolumeChanged(volume);
}

void AudioManager::onWorkerOutputMuteChanged(bool muted)
{
    QMutexLocker cacheLock(&m_cacheMutex);
    m_cachedOutputMute = muted;
    emit outputMuteChanged(muted);
}

void AudioManager::onWorkerInputMuteChanged(bool muted)
{
    QMutexLocker cacheLock(&m_cacheMutex);
    m_cachedInputMute = muted;
    emit inputMuteChanged(muted);
}

void AudioManager::onWorkerApplicationsChanged(const QList<AudioApplication>& apps)
{
    QMutexLocker cacheLock(&m_cacheMutex);
    m_cachedApplications = apps;
    emit applicationsChanged(apps);
}

void AudioManager::onWorkerDevicesChanged(const QList<AudioDevice>& devices)
{
    QMutexLocker cacheLock(&m_cacheMutex);
    m_cachedDevices = devices;
    emit devicesChanged(devices);
}

void AudioManager::cleanup()
{
    QMutexLocker locker(&m_mutex);

    if (!m_workerThread) return;

    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "cleanup", Qt::QueuedConnection);
    }

    m_workerThread->quit();
    if (!m_workerThread->wait(5000)) {
        m_workerThread->terminate();
        m_workerThread->wait();
    }

    delete m_worker;
    m_worker = nullptr;

    delete m_workerThread;
    m_workerThread = nullptr;

    QMutexLocker cacheLock(&m_cacheMutex);
    m_cachedOutputVolume = 0;
    m_cachedInputVolume = 0;
    m_cachedOutputMute = false;
    m_cachedInputMute = false;
    m_cachedApplications.clear();
    m_cachedDevices.clear();
}

void AudioManager::startApplicationAudioLevelMonitoring()
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "startApplicationAudioLevelMonitoring", Qt::QueuedConnection);
    }
}

void AudioManager::stopApplicationAudioLevelMonitoring()
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "stopApplicationAudioLevelMonitoring", Qt::QueuedConnection);
    }
}

void AudioManager::startAudioLevelMonitoring()
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "startAudioLevelMonitoring", Qt::QueuedConnection);
    }
}

void AudioManager::stopAudioLevelMonitoring()
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "stopAudioLevelMonitoring", Qt::QueuedConnection);
    }
}

// Async methods
void AudioManager::setOutputVolumeAsync(int volume)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setOutputVolume", Qt::QueuedConnection, Q_ARG(int, volume));
    }
}

void AudioManager::setInputVolumeAsync(int volume)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setInputVolume", Qt::QueuedConnection, Q_ARG(int, volume));
    }
}

void AudioManager::setOutputMuteAsync(bool mute)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setOutputMute", Qt::QueuedConnection, Q_ARG(bool, mute));
    }
}

void AudioManager::setInputMuteAsync(bool mute)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setInputMute", Qt::QueuedConnection, Q_ARG(bool, mute));
    }
}

void AudioManager::setApplicationVolumeAsync(const QString& appId, int volume)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setApplicationVolume", Qt::QueuedConnection,
                                  Q_ARG(QString, appId), Q_ARG(int, volume));
    }
}

void AudioManager::setApplicationMuteAsync(const QString& appId, bool mute)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setApplicationMute", Qt::QueuedConnection,
                                  Q_ARG(QString, appId), Q_ARG(bool, mute));
    }
}

void AudioManager::setDefaultDeviceAsync(const QString& deviceId, bool isInput, bool forCommunications)
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "setDefaultDevice", Qt::QueuedConnection,
                                  Q_ARG(QString, deviceId), Q_ARG(bool, isInput), Q_ARG(bool, forCommunications));
    }
}

// Cached getters
int AudioManager::getOutputVolume() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedOutputVolume;
}

int AudioManager::getInputVolume() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedInputVolume;
}

bool AudioManager::getOutputMute() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedOutputMute;
}

bool AudioManager::getInputMute() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedInputMute;
}

QList<AudioApplication> AudioManager::getApplications() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedApplications;
}

QList<AudioDevice> AudioManager::getDevices() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cachedDevices;
}

AudioWorker* AudioManager::getWorker()
{
    return m_worker;
}
