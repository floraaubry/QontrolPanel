#include "mediasessionmanager.h"
#include "logmanager.h"
#include <QMetaObject>
#include <QMutexLocker>
#include <QBuffer>
#include <QByteArray>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;

static QThread* g_mediaWorkerThread = nullptr;
static MediaWorker* g_mediaWorker = nullptr;
static QMutex g_mediaInitMutex;

QPixmap createRoundedPixmap(const QPixmap& source, int targetSize, int radius) {
    // Scale the source to target size while maintaining aspect ratio
    QPixmap scaled = source.scaled(targetSize, targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    // Create a new pixmap with transparent background
    QPixmap rounded(targetSize, targetSize);
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Create a rounded rectangle path
    QPainterPath path;
    path.addRoundedRect(0, 0, targetSize, targetSize, radius, radius);

    // Set the clipping path
    painter.setClipPath(path);

    // Calculate position to center the scaled image
    int x = (targetSize - scaled.width()) / 2;
    int y = (targetSize - scaled.height()) / 2;

    // Draw the scaled image
    painter.drawPixmap(x, y, scaled);

    return rounded;
}

MediaInfo queryMediaInfoImpl(MediaWorker* worker) {
    MediaInfo info;

    try {
        init_apartment();
        auto sessionManager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        auto currentSession = sessionManager.GetCurrentSession();

        if (currentSession) {
            auto properties = currentSession.TryGetMediaPropertiesAsync().get();
            if (properties) {
                info.title = QString::fromWCharArray(properties.Title().c_str());
                info.artist = QString::fromWCharArray(properties.Artist().c_str());
                info.album = QString::fromWCharArray(properties.AlbumTitle().c_str());

                LogManager::instance()->sendLog(LogManager::MediaSessionManager,
                                                QString("Retrieved media info: %1 - %2").arg(info.artist, info.title));

                // Fetch album art
                try {
                    auto thumbnailRef = properties.Thumbnail();
                    if (thumbnailRef) {
                        auto thumbnailStream = thumbnailRef.OpenReadAsync().get();
                        if (thumbnailStream) {
                            auto size = thumbnailStream.Size();
                            if (size > 0) {
                                DataReader reader(thumbnailStream);
                                auto bytesLoaded = reader.LoadAsync(static_cast<uint32_t>(size)).get();

                                if (bytesLoaded > 0) {
                                    std::vector<uint8_t> buffer(bytesLoaded);
                                    reader.ReadBytes(buffer);

                                    QByteArray originalImageData(reinterpret_cast<const char*>(buffer.data()), bytesLoaded);

                                    // Check if album art has changed using cache
                                    if (worker && originalImageData != worker->m_cachedRawAlbumArt) {
                                        LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Album art changed, processing new image");

                                        // Load and process the image only if it changed
                                        QPixmap originalPixmap;
                                        if (originalPixmap.loadFromData(originalImageData)) {
                                            int targetSize = 64;
                                            QPixmap roundedPixmap = createRoundedPixmap(originalPixmap, targetSize, 8);

                                            QByteArray processedImageData;
                                            QBuffer buffer(&processedImageData);
                                            buffer.open(QIODevice::WriteOnly);
                                            roundedPixmap.save(&buffer, "PNG");

                                            QString base64Image = processedImageData.toBase64();
                                            QString dataUri = QString("data:image/png;base64,%1").arg(base64Image);

                                            // Update cache
                                            worker->m_cachedRawAlbumArt = originalImageData;
                                            worker->m_cachedProcessedAlbumArt = dataUri;

                                            info.albumArt = dataUri;

                                            LogManager::instance()->sendLog(LogManager::MediaSessionManager,
                                                                            QString("Album art processed successfully (%1 bytes)").arg(processedImageData.size()));
                                        }
                                    } else if (worker) {
                                        // Use cached processed album art
                                        info.albumArt = worker->m_cachedProcessedAlbumArt;
                                        LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Using cached album art");
                                    }
                                }
                            }
                        }
                    }
                } catch (...) {
                    LogManager::instance()->sendWarn(LogManager::MediaSessionManager, "Failed to fetch album art");
                }
            }

            auto playbackInfo = currentSession.GetPlaybackInfo();
            if (playbackInfo) {
                info.isPlaying = (playbackInfo.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing);
                LogManager::instance()->sendLog(LogManager::MediaSessionManager,
                                                QString("Playback status: %1").arg(info.isPlaying ? "Playing" : "Paused/Stopped"));
            }
        } else {
            LogManager::instance()->sendLog(LogManager::MediaSessionManager, "No active media session found");
        }
    } catch (...) {
        LogManager::instance()->sendCritical(LogManager::MediaSessionManager, "Failed to query media session info");
    }

    return info;
}

void MediaWorker::setupSessionManagerNotifications() {
    try {
        init_apartment();
        m_sessionManager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();

        // Listen for when sessions are added/removed
        m_sessionsChangedToken = m_sessionManager.SessionsChanged(
            [this](GlobalSystemMediaTransportControlsSessionManager const& sender, SessionsChangedEventArgs const& args) {
                Q_UNUSED(sender)
                Q_UNUSED(args)
                QMetaObject::invokeMethod(this, [this]() {
                    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Sessions changed, checking for new active session");
                    ensureCurrentSession();
                    queryMediaInfo();
                }, Qt::QueuedConnection);
            });

        LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Session manager notifications registered");
    } catch (...) {
        LogManager::instance()->sendCritical(LogManager::MediaSessionManager, "Failed to setup session manager notifications");
    }
}

void MediaWorker::cleanupSessionManagerNotifications() {
    if (m_sessionManager && m_sessionsChangedToken.value != 0) {
        try {
            m_sessionManager.SessionsChanged(m_sessionsChangedToken);
            m_sessionsChangedToken = {};
            LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Session manager notifications cleaned up");
        } catch (...) {
            LogManager::instance()->sendWarn(LogManager::MediaSessionManager, "Error cleaning up session manager notifications");
        }
    }
}

bool MediaWorker::ensureCurrentSession() {
    try {
        if (!m_sessionManager) {
            init_apartment();
            m_sessionManager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        }

        auto currentSession = m_sessionManager.GetCurrentSession();

        // If session changed, update notifications
        if (m_currentSession != currentSession) {
            if (m_currentSession) {
                LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Media session changed, updating notifications");
            } else {
                LogManager::instance()->sendLog(LogManager::MediaSessionManager, "New media session detected");
            }

            // Clear cache when session changes
            m_cachedRawAlbumArt.clear();
            m_cachedProcessedAlbumArt.clear();
            LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Album art cache cleared due to session change");

            cleanupSessionNotifications();
            m_currentSession = currentSession;
            if (m_currentSession) {
                setupSessionNotifications();
            }
        }

        return m_currentSession != nullptr;
    } catch (...) {
        LogManager::instance()->sendCritical(LogManager::MediaSessionManager, "Failed to get current session");
        return false;
    }
}

void MediaWorker::setupSessionNotifications() {
    if (!m_currentSession) {
        return;
    }

    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Setting up session event notifications");

    try {
        // Register for media properties changes (title, artist, album art)
        m_propertiesChangedToken = m_currentSession.MediaPropertiesChanged(
            [this](GlobalSystemMediaTransportControlsSession const& session,
                   MediaPropertiesChangedEventArgs const& args) {
                Q_UNUSED(session)
                Q_UNUSED(args)
                QMetaObject::invokeMethod(this, "queryMediaInfo", Qt::QueuedConnection);
            });

        // Register for playback info changes (play/pause state)
        m_playbackInfoChangedToken = m_currentSession.PlaybackInfoChanged(
            [this](GlobalSystemMediaTransportControlsSession const& session,
                   PlaybackInfoChangedEventArgs const& args) {
                Q_UNUSED(session)
                Q_UNUSED(args)
                QMetaObject::invokeMethod(this, "queryMediaInfo", Qt::QueuedConnection);
            });

        LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Session event notifications registered successfully");
    } catch (...) {
        LogManager::instance()->sendCritical(LogManager::MediaSessionManager, "Failed to register session notifications");
    }
}

void MediaWorker::cleanupSessionNotifications() {
    if (m_currentSession) {
        LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Cleaning up session notifications");
        try {
            if (m_propertiesChangedToken.value != 0) {
                m_currentSession.MediaPropertiesChanged(m_propertiesChangedToken);
                m_propertiesChangedToken = {};
            }
            if (m_playbackInfoChangedToken.value != 0) {
                m_currentSession.PlaybackInfoChanged(m_playbackInfoChangedToken);
                m_playbackInfoChangedToken = {};
            }
            LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Session notifications cleaned up successfully");
        } catch (...) {
            LogManager::instance()->sendWarn(LogManager::MediaSessionManager, "Error cleaning up session notifications");
        }
    }
}

void MediaWorker::queryMediaInfo() {
    MediaInfo info = queryMediaInfoImpl(this);
    emit mediaInfoChanged(info);
}

void MediaWorker::startMonitoring() {
    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Starting media session monitoring");

    // Clear cache on start
    m_cachedRawAlbumArt.clear();
    m_cachedProcessedAlbumArt.clear();

    setupSessionManagerNotifications();
    ensureCurrentSession();
    queryMediaInfo();

    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Media monitoring started (fully event-driven)");
}

void MediaWorker::stopMonitoring() {
    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Stopping media session monitoring");

    cleanupSessionNotifications();
    cleanupSessionManagerNotifications();
    m_currentSession = nullptr;
    m_sessionManager = nullptr;

    // Clear cache on stop
    m_cachedRawAlbumArt.clear();
    m_cachedProcessedAlbumArt.clear();

    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Media monitoring stopped");
}

void MediaWorker::playPause() {
    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Toggling play/pause");

    try {
        if (ensureCurrentSession() && m_currentSession) {
            m_currentSession.TryTogglePlayPauseAsync().get();
            LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Play/pause toggled successfully");
            // Event will trigger automatically via PlaybackInfoChanged
        } else {
            LogManager::instance()->sendWarn(LogManager::MediaSessionManager, "No active session for play/pause toggle");
        }
    } catch (...) {
        LogManager::instance()->sendCritical(LogManager::MediaSessionManager, "Failed to toggle play/pause");
    }
}

void MediaWorker::nextTrack() {
    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Skipping to next track");

    try {
        if (ensureCurrentSession() && m_currentSession) {
            m_currentSession.TrySkipNextAsync().get();
            LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Successfully skipped to next track");
            // Event will trigger automatically via MediaPropertiesChanged
        } else {
            LogManager::instance()->sendWarn(LogManager::MediaSessionManager, "No active session for next track");
        }
    } catch (...) {
        LogManager::instance()->sendCritical(LogManager::MediaSessionManager, "Failed to skip to next track");
    }
}

void MediaWorker::previousTrack() {
    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Skipping to previous track");

    try {
        if (ensureCurrentSession() && m_currentSession) {
            m_currentSession.TrySkipPreviousAsync().get();
            LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Successfully skipped to previous track");
            // Event will trigger automatically via MediaPropertiesChanged
        } else {
            LogManager::instance()->sendWarn(LogManager::MediaSessionManager, "No active session for previous track");
        }
    } catch (...) {
        LogManager::instance()->sendCritical(LogManager::MediaSessionManager, "Failed to skip to previous track");
    }
}

void MediaSessionManager::initialize() {
    QMutexLocker locker(&g_mediaInitMutex);

    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Initializing MediaSessionManager");

    if (!g_mediaWorkerThread) {
        g_mediaWorkerThread = new QThread;
        g_mediaWorker = new MediaWorker;
        g_mediaWorker->moveToThread(g_mediaWorkerThread);
        g_mediaWorkerThread->start();

        LogManager::instance()->sendLog(LogManager::MediaSessionManager, "MediaSessionManager worker thread started");
    } else {
        LogManager::instance()->sendLog(LogManager::MediaSessionManager, "MediaSessionManager already initialized");
    }
}

void MediaSessionManager::cleanup() {
    QMutexLocker locker(&g_mediaInitMutex);

    LogManager::instance()->sendLog(LogManager::MediaSessionManager, "Cleaning up MediaSessionManager");

    if (g_mediaWorkerThread) {
        // Stop monitoring and cleanup notifications before quitting thread
        if (g_mediaWorker) {
            QMetaObject::invokeMethod(g_mediaWorker, "stopMonitoring", Qt::BlockingQueuedConnection);
        }

        g_mediaWorkerThread->quit();
        if (!g_mediaWorkerThread->wait(3000)) {
            LogManager::instance()->sendWarn(LogManager::MediaSessionManager, "Worker thread did not quit gracefully, terminating");
            g_mediaWorkerThread->terminate();
            g_mediaWorkerThread->wait(1000);
        }

        delete g_mediaWorker;
        delete g_mediaWorkerThread;
        g_mediaWorker = nullptr;
        g_mediaWorkerThread = nullptr;

        LogManager::instance()->sendLog(LogManager::MediaSessionManager, "MediaSessionManager cleanup complete");
    }
}

void MediaSessionManager::queryMediaInfoAsync() {
    if (g_mediaWorker) {
        QMetaObject::invokeMethod(g_mediaWorker, "queryMediaInfo", Qt::QueuedConnection);
    }
}

void MediaSessionManager::startMonitoringAsync() {
    if (g_mediaWorker) {
        QMetaObject::invokeMethod(g_mediaWorker, "startMonitoring", Qt::QueuedConnection);
    }
}

void MediaSessionManager::stopMonitoringAsync() {
    if (g_mediaWorker) {
        QMetaObject::invokeMethod(g_mediaWorker, "stopMonitoring", Qt::QueuedConnection);
    }
}

void MediaSessionManager::playPauseAsync() {
    if (g_mediaWorker) {
        QMetaObject::invokeMethod(g_mediaWorker, "playPause", Qt::QueuedConnection);
    }
}

void MediaSessionManager::nextTrackAsync() {
    if (g_mediaWorker) {
        QMetaObject::invokeMethod(g_mediaWorker, "nextTrack", Qt::QueuedConnection);
    }
}

void MediaSessionManager::previousTrackAsync() {
    if (g_mediaWorker) {
        QMetaObject::invokeMethod(g_mediaWorker, "previousTrack", Qt::QueuedConnection);
    }
}

MediaWorker* MediaSessionManager::getWorker() {
    return g_mediaWorker;
}
