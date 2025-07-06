#ifndef MEDIASESSIONMANAGER_H
#define MEDIASESSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QTimer>

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
    void initializeTimer();
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

#endif // MEDIASESSIONMANAGER_H
