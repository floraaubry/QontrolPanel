#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QString>
#include <QList>
#include <QIcon>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

struct AudioDevice
{
    QString id;
    QString name;
    QString shortName;
    QString type;
    bool isDefault;
};

struct Application {
    QString id;
    QString name;
    QString executableName;
    bool isMuted;
    int volume;
    QIcon icon;
    int audioLevel;
};

class AudioWorker : public QObject
{
    Q_OBJECT

public slots:
    void enumeratePlaybackDevices();
    void enumerateRecordingDevices();
    void enumerateApplications();
    void setPlaybackVolume(int volume);
    void setRecordingVolume(int volume);
    void setPlaybackMute(bool mute);
    void setRecordingMute(bool mute);
    void setDefaultEndpoint(const QString &deviceId);
    void setApplicationVolume(const QString& appId, int volume);
    void setApplicationMute(const QString& appId, bool mute);
    void startAudioLevelMonitoring();
    void stopAudioLevelMonitoring();
    void initializeCache();
    void updateDeviceProperties();
    void queryCurrentProperties();

signals:
    void playbackDevicesReady(const QList<AudioDevice>& devices);
    void recordingDevicesReady(const QList<AudioDevice>& devices);
    void applicationsReady(const QList<Application>& applications);
    void playbackVolumeChanged(int volume);
    void recordingVolumeChanged(int volume);
    void playbackMuteChanged(bool muted);
    void recordingMuteChanged(bool muted);
    void playbackAudioLevel(int level);
    void recordingAudioLevel(int level);
    void defaultEndpointChanged(bool success);
    void applicationVolumeChanged(const QString& appId, bool success);
    void applicationMuteChanged(const QString& appId, bool success);
    void currentPropertiesReady(int playbackVol, int recordingVol, bool playbackMute, bool recordingMute);
    void applicationAudioLevelsChanged(const QList<QPair<QString, int>>& levels);

private:
    QTimer* m_audioLevelTimer = nullptr;
    QList<Application> m_cachedApplications;  // Add this
    void initializeTimer();

};

namespace AudioManager
{
void initialize();
void cleanup();

// Async methods
void enumeratePlaybackDevicesAsync();
void enumerateRecordingDevicesAsync();
void enumerateApplicationsAsync();
void setPlaybackVolumeAsync(int volume);
void setRecordingVolumeAsync(int volume);
void setPlaybackMuteAsync(bool mute);
void setRecordingMuteAsync(bool mute);
void setDefaultEndpointAsync(const QString &deviceId);
void setApplicationVolumeAsync(const QString& appId, int volume);
void setApplicationMuteAsync(const QString& appId, bool mute);
void startAudioLevelMonitoring();
void stopAudioLevelMonitoring();
void queryCurrentPropertiesAsync();

// Cached values (non-blocking, always current)
int getPlaybackVolume();
int getRecordingVolume();
bool getPlaybackMute();
bool getRecordingMute();

AudioWorker* getWorker();
}

#endif // AUDIOMANAGER_H
