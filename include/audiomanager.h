#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QString>
#include <QList>
#include <QIcon>
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
};

namespace AudioManager
{
    void initialize();
    void cleanup();

    bool setDefaultEndpoint(const QString &deviceId);

    void enumeratePlaybackDevices(QList<AudioDevice>& playbackDevices);
    void enumerateRecordingDevices(QList<AudioDevice>& recordingDevices);

    void setPlaybackVolume(int volume);
    int getPlaybackVolume();

    void setRecordingVolume(int volume);
    int getRecordingVolume();

    void setPlaybackMute(bool mute);
    bool getPlaybackMute();

    void setRecordingMute(bool mute);
    bool getRecordingMute();

    int getPlaybackAudioLevel();
    int getRecordingAudioLevel();

    QList<Application> enumerateAudioApplications();
    bool setApplicationVolume(const QString& appName, int volume);
    bool setApplicationMute(const QString& appName, bool mute);
    bool getApplicationMute(const QString &appId);
}

#endif // AUDIOMANAGER_H
