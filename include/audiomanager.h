#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QIcon>
#include <QMap>
#include <QAbstractListModel>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include "policyconfig.h"

// Forward declarations
class AudioWorker;

struct AudioApplication {
    QString id;              // Process ID as string
    QString name;            // Display name
    QString executableName;  // Executable name
    QString iconPath;        // Base64 encoded icon
    int volume;              // 0-100
    bool isMuted;            // Mute state
    int streamIndex;         // Index within the same executable (0, 1, 2, ...)

    AudioApplication() : volume(0), isMuted(false), streamIndex(0) {}

    bool operator==(const AudioApplication& other) const {
        return id == other.id;
    }
};

struct AudioDevice {
    QString id;                  // Device ID
    QString name;                // Friendly name
    QString description;         // Device description
    bool isDefault;              // Is default device
    bool isDefaultCommunication; // Is default communication device
    bool isInput;                // true for input, false for output
    QString state;               // Active, Disabled, etc.

    AudioDevice() : isDefault(false), isDefaultCommunication(false), isInput(false) {}

    bool operator==(const AudioDevice& other) const {
        return id == other.id;
    }
};

Q_DECLARE_METATYPE(AudioApplication)
Q_DECLARE_METATYPE(AudioDevice)

class AudioDeviceModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum DeviceRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        IsDefaultRole,
        IsDefaultCommunicationRole,
        IsInputRole,
        StateRole
    };
    Q_ENUM(DeviceRoles)

    explicit AudioDeviceModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Model management
    void setDevices(const QList<AudioDevice>& devices);
    void updateDevice(const AudioDevice& device);
    void removeDevice(const QString& deviceId);

private:
    QList<AudioDevice> m_devices;
    int findDeviceIndex(const QString& deviceId) const;
};

class AudioWorker : public QObject
{
    Q_OBJECT

public:
    enum DataFlow {
        Output = 0,  // eRender
        Input = 1    // eCapture
    };
    Q_ENUM(DataFlow)

    AudioWorker();
    ~AudioWorker();

    // Helper to convert Windows enum to our enum
    static DataFlow fromWindowsDataFlow(EDataFlow flow) {
        return (flow == eRender) ? Output : Input;
    }

    static EDataFlow toWindowsDataFlow(DataFlow flow) {
        return (flow == Output) ? eRender : eCapture;
    }

    bool hasProcessId(DWORD processId);

public slots:
    void initialize();
    void cleanup();
    void setOutputVolume(int volume);
    void setInputVolume(int volume);
    void setOutputMute(bool mute);
    void setInputMute(bool mute);
    void setApplicationVolume(const QString& appId, int volume);
    void setApplicationMute(const QString& appId, bool mute);
    void setDefaultDevice(const QString& deviceId, bool isInput, bool forCommunications = false);
    void onVolumeChanged(AudioWorker::DataFlow dataFlow, float volume, bool muted);
    void onSessionCreated();
    void onSessionDisconnected();
    void onApplicationSessionVolumeChanged(const QString& appId, float volume, bool muted);
    void onDeviceAdded(const QString& deviceId);
    void onDeviceRemoved(const QString& deviceId);
    void onDefaultDeviceChanged(DataFlow dataFlow, const QString& deviceId);

    void enumerateApplications();
    void enumerateDevices();

    void startAudioLevelMonitoring();
    void stopAudioLevelMonitoring();

signals:
    void outputVolumeChanged(int volume);
    void inputVolumeChanged(int volume);
    void outputMuteChanged(bool muted);
    void inputMuteChanged(bool muted);
    void applicationsChanged(const QList<AudioApplication>& applications);
    void applicationVolumeChanged(const QString& appId, int volume);
    void applicationMuteChanged(const QString& appId, bool muted);
    void devicesChanged(const QList<AudioDevice>& devices);
    void deviceAdded(const AudioDevice& device);
    void deviceRemoved(const QString& deviceId);
    void defaultDeviceChanged(const QString& deviceId, bool isInput);
    void initializationComplete();

    void outputAudioLevelChanged(int level);
    void inputAudioLevelChanged(int level);

    void applicationAudioLevelChanged(const QString& appId, int level);

private slots:
    void updateAudioLevels();
    void initializeAudioLevelTimer();
    void startApplicationAudioLevelMonitoring();
    void stopApplicationAudioLevelMonitoring();

private:
    // COM objects
    IMMDeviceEnumerator* m_deviceEnumerator;
    class VolumeNotificationClient* m_outputVolumeClient;
    class VolumeNotificationClient* m_inputVolumeClient;
    class SessionNotificationClient* m_sessionNotificationClient;
    class DeviceNotificationClient* m_deviceNotificationClient;
    IAudioSessionManager2* m_sessionManager;
    IPolicyConfig* m_policyConfig;

    // Volume controls for monitoring
    IAudioEndpointVolume* m_outputVolumeControl;
    IAudioEndpointVolume* m_inputVolumeControl;

    // Session event clients for tracking individual app volume changes
    QMap<QString, class SessionEventsClient*> m_sessionEventClients;

    // Cache
    int m_outputVolume;
    int m_inputVolume;
    bool m_outputMuted;
    bool m_inputMuted;
    QList<AudioApplication> m_applications;
    QList<AudioDevice> m_devices;

    HRESULT initializeCOM();
    void setupVolumeNotifications();
    void setupSessionNotifications();
    void setupDeviceNotifications();
    void updateCurrentVolumes();
    void setVolumeForDevice(EDataFlow dataFlow, int volume);
    void setMuteForDevice(EDataFlow dataFlow, bool mute);

    AudioDevice createAudioDeviceFromInterface(IMMDevice* device, EDataFlow dataFlow);
    QString getDeviceProperty(IMMDevice* device, const PROPERTYKEY& key);

    QIcon getApplicationIcon(const QString& executablePath);
    QString getApplicationDisplayName(const QString& executablePath);
    QString getExecutablePath(DWORD processId);

    struct SessionInfo {
        IAudioSessionControl* sessionControl;
        SessionEventsClient* eventsClient;
        QString appId;

        SessionInfo() : sessionControl(nullptr), eventsClient(nullptr) {}
    };

    QList<SessionInfo> m_activeSessions;
    QMap<QString, ISimpleAudioVolume*> m_sessionVolumeControls;

    QTimer* m_audioLevelTimer;
    QList<AudioApplication> m_cachedApplications;

    int getDeviceAudioLevel(EDataFlow dataFlow);

    QMap<QString, IAudioMeterInformation*> m_sessionMeterControls;
    QMap<QString, int> m_applicationAudioLevels; // Cache levels

    int getApplicationAudioLevel(const QString& appId);
    void updateApplicationAudioLevels();

    bool m_sessionManagerInvalid;
    bool ensureValidSessionManager();
};

// Device change notification callback
class DeviceNotificationClient : public IMMNotificationClient
{
private:
    LONG m_cRef;
    AudioWorker* m_worker;

public:
    DeviceNotificationClient(AudioWorker* worker);
    ~DeviceNotificationClient();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IMMNotificationClient methods
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState);
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId);
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key);
};

// Volume change notification callback
class VolumeNotificationClient : public IAudioEndpointVolumeCallback
{
private:
    LONG m_cRef;
    AudioWorker* m_worker;
    EDataFlow m_dataFlow;

public:
    VolumeNotificationClient(AudioWorker* worker, EDataFlow dataFlow);
    ~VolumeNotificationClient();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IAudioEndpointVolumeCallback methods
    HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify);
};

// Session notification for application changes
class SessionNotificationClient : public IAudioSessionNotification
{
private:
    LONG m_cRef;
    AudioWorker* m_worker;

public:
    SessionNotificationClient(AudioWorker* worker);
    ~SessionNotificationClient();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IAudioSessionNotification methods
    HRESULT STDMETHODCALLTYPE OnSessionCreated(IAudioSessionControl *NewSession);
};

// Session events for individual application volume/mute changes
class SessionEventsClient : public IAudioSessionEvents
{
private:
    LONG m_cRef;
    AudioWorker* m_worker;
    QString m_appId;

public:
    SessionEventsClient(AudioWorker* worker, const QString& appId);
    ~SessionEventsClient();

    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IAudioSessionEvents methods
    HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(LPCWSTR NewDisplayName, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnIconPathChanged(LPCWSTR NewIconPath, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolumeArray[], DWORD ChangedChannel, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(LPCGUID NewGroupingParam, LPCGUID EventContext);
    HRESULT STDMETHODCALLTYPE OnStateChanged(AudioSessionState NewState);
    HRESULT STDMETHODCALLTYPE OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason);
};

class AudioManager : public QObject
{
    Q_OBJECT

public:
    static AudioManager* instance();

    void initialize();
    void cleanup();

    // Async methods
    void setOutputVolumeAsync(int volume);
    void setInputVolumeAsync(int volume);
    void setOutputMuteAsync(bool mute);
    void setInputMuteAsync(bool mute);
    void setApplicationVolumeAsync(const QString& appId, int volume);
    void setApplicationMuteAsync(const QString& appId, bool mute);
    void setDefaultDeviceAsync(const QString& deviceId, bool isInput, bool forCommunications = false);

    // Cached values (thread-safe)
    int getOutputVolume() const;
    int getInputVolume() const;
    bool getOutputMute() const;
    bool getInputMute() const;
    QList<AudioApplication> getApplications() const;
    QList<AudioDevice> getDevices() const;

    AudioWorker* getWorker();

    void startAudioLevelMonitoring();
    void stopAudioLevelMonitoring();

    void startApplicationAudioLevelMonitoring();
    void stopApplicationAudioLevelMonitoring();

signals:
    void outputVolumeChanged(int volume);
    void inputVolumeChanged(int volume);
    void outputMuteChanged(bool muted);
    void inputMuteChanged(bool muted);
    void applicationsChanged(const QList<AudioApplication>& applications);
    void applicationVolumeChanged(const QString& appId, int volume);
    void applicationMuteChanged(const QString& appId, bool muted);
    void devicesChanged(const QList<AudioDevice>& devices);
    void deviceAdded(const AudioDevice& device);
    void deviceRemoved(const QString& deviceId);
    void defaultDeviceChanged(const QString& deviceId, bool isInput);
    void initializationComplete();

    void outputAudioLevelChanged(int level);
    void inputAudioLevelChanged(int level);
    void applicationAudioLevelChanged(const QString& appId, int level);

private slots:
    void onWorkerOutputVolumeChanged(int volume);
    void onWorkerInputVolumeChanged(int volume);
    void onWorkerOutputMuteChanged(bool muted);
    void onWorkerInputMuteChanged(bool muted);
    void onWorkerApplicationsChanged(const QList<AudioApplication>& apps);
    void onWorkerDevicesChanged(const QList<AudioDevice>& devices);

private:
    AudioManager(QObject* parent = nullptr);
    ~AudioManager();

    static AudioManager* m_instance;
    static QMutex m_mutex;

    QThread* m_workerThread;
    AudioWorker* m_worker;

    // Thread-safe cache
    mutable QMutex m_cacheMutex;
    int m_cachedOutputVolume;
    int m_cachedInputVolume;
    bool m_cachedOutputMute;
    bool m_cachedInputMute;
    QList<AudioApplication> m_cachedApplications;
    QList<AudioDevice> m_cachedDevices;
};
