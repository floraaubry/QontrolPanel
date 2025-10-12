#pragma once
#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <QByteArray>
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
    GlobalSystemMediaTransportControlsSessionManager m_sessionManager{ nullptr };
    GlobalSystemMediaTransportControlsSession m_currentSession{ nullptr };

    // Event tokens for cleanup
    event_token m_sessionsChangedToken{};
    event_token m_propertiesChangedToken{};
    event_token m_playbackInfoChangedToken{};

    // Cache for album art to avoid reprocessing
    QByteArray m_cachedRawAlbumArt;
    QString m_cachedProcessedAlbumArt;

    void setupSessionManagerNotifications();
    void cleanupSessionManagerNotifications();
    void setupSessionNotifications();
    void cleanupSessionNotifications();
    bool ensureCurrentSession();

    friend MediaInfo queryMediaInfoImpl(MediaWorker* worker);
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
