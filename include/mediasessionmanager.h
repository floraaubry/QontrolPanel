#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <windows.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Foundation.h>

using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::Foundation;

struct MediaInfo {
    QString title;
    QString artist;
    QString album;
    bool isPlaying = false;
    QString albumArt;
};

class MediaWorker : public QObject
{
    Q_OBJECT

public slots:
    void queryMediaInfo();
    void startMonitoring();
    void stopMonitoring();
    void playPause();
    void nextTrack();
    void previousTrack();

signals:
    void mediaInfoChanged(const MediaInfo& info);

private:
    QTimer* m_updateTimer = nullptr;
    GlobalSystemMediaTransportControlsSession m_currentSession{ nullptr };

    // Event tokens for cleanup
    event_token m_propertiesChangedToken{};
    event_token m_playbackInfoChangedToken{};

    void initializeTimer();
    void setupSessionNotifications();
    void cleanupSessionNotifications();
    bool ensureCurrentSession();
};

namespace MediaSessionManager
{
void initialize();
void cleanup();
void queryMediaInfoAsync();
void startMonitoringAsync();
void stopMonitoringAsync();
void playPauseAsync();
void nextTrackAsync();
void previousTrackAsync();
MediaWorker* getWorker();
}
