#include "soundpanel.h"
#include "utils.h"
#include "quicksoundswitcher.h"
#include <QQmlContext>
#include <QQuickItem>
#include <QIcon>
#include <QPixmap>
#include <QBuffer>
#include <QApplication>
#include <QTimer>
#include <QPropertyAnimation>

SoundPanel::SoundPanel(QObject* parent)
    : QObject(parent)
    , soundPanelWindow(nullptr)
    , engine(new QQmlApplicationEngine(this))
    , isWindows10(Utils::isWindows10())
    , isAnimating(false)
    , hWnd(nullptr)
    , settings("Odizinne", "QuickSoundSwitcher")
    , mixerOnly(settings.value("mixerOnly").toBool())
    , systemSoundsMuted(false)
{
    configureQML();
    populateApplicationModel();

    soundPanelWindow = qobject_cast<QWindow*>(engine->rootObjects().value(0));
    hWnd = reinterpret_cast<HWND>(soundPanelWindow->winId());

    setupUI();
    animateIn();

    connect(static_cast<QuickSoundSwitcher*>(parent), &QuickSoundSwitcher::outputMuteStateChanged,
            this, &SoundPanel::onOutputMuteStateChanged);
    connect(static_cast<QuickSoundSwitcher*>(parent), &QuickSoundSwitcher::volumeChangedWithTray,
            this, &SoundPanel::onVolumeChangedWithTray);
}

SoundPanel::~SoundPanel()
{
    delete soundPanelWindow;
    delete engine;
}

void SoundPanel::configureQML() {
    engine->rootContext()->setContextProperty("soundPanel", this);
    engine->rootContext()->setContextProperty("mixerOnly", mixerOnly);
    engine->rootContext()->setContextProperty("componentsOpacity", 0);

    engine->loadFromModule("Odizinne.QuickSoundSwitcher", "SoundPanel");
}

void SoundPanel::setupUI() {
    if (mixerOnly) return;

    populateComboBoxes();
    setPlaybackVolume(AudioManager::getPlaybackVolume());
    setRecordingVolume(AudioManager::getRecordingVolume());
    setPlaybackMuted(AudioManager::getPlaybackMute());
    setRecordingMuted(AudioManager::getRecordingMute());
}

void SoundPanel::onVolumeChangedWithTray(int volume) {
    setPlaybackVolume(volume);
}

void SoundPanel::onOutputMuteStateChanged(bool mutedState) {
    setPlaybackMuted(mutedState);
}

void SoundPanel::animateIn()
{
    if (isAnimating) return;
    isAnimating = true;

    QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();

    QList<QObject*> rootObjects = engine->rootObjects();
    int engineHeight = 0;
    if (!rootObjects.isEmpty()) {
        engineHeight = rootObjects[0]->property("height").toInt();
    }

    soundPanelWindow->resize(330, engineHeight);

    int margin = isWindows10 ? -1 : 12;
    int panelX = availableGeometry.right() - soundPanelWindow->width() + 1 - margin;
    int scalingFactor = isWindows10 ? 70 : 0;
    int startY = availableGeometry.bottom() - (soundPanelWindow->height() * scalingFactor / 100);
    int targetY = availableGeometry.bottom() - soundPanelWindow->height() - margin;

    soundPanelWindow->setPosition(panelX, startY);

    QPropertyAnimation *animation = new QPropertyAnimation(soundPanelWindow, "y", this);
    animation->setDuration(isWindows10 ? 300 : 200);
    animation->setStartValue(startY);
    animation->setEndValue(targetY);
    animation->setEasingCurve(QEasingCurve::OutQuad);

    connect(animation, &QPropertyAnimation::finished, this, [this, animation]() {
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        isAnimating = false;
        animation->deleteLater();
    });

    soundPanelWindow->show();
    animation->start();
}

void SoundPanel::animateOut()
{
    if (isAnimating) return;

    isAnimating = true;

    QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();

    int scalingFactor = isWindows10 ? 70 : 0;
    int startY = soundPanelWindow->y();
    int targetY = availableGeometry.bottom() - (soundPanelWindow->height() * scalingFactor / 100);

    QPropertyAnimation *animation = new QPropertyAnimation(soundPanelWindow, "y", this);
    animation->setDuration(isWindows10 ? 300 : 200);
    animation->setStartValue(startY);
    animation->setEndValue(targetY);
    animation->setEasingCurve(QEasingCurve::InQuad);

    connect(animation, &QPropertyAnimation::finished, this, [this, animation]() {
        soundPanelWindow->hide();
        animation->deleteLater();
        this->deleteLater();
    });

    SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    animation->start();
}

int SoundPanel::playbackVolume() const {
    return m_playbackVolume;
}

void SoundPanel::setPlaybackVolume(int volume) {
    if (m_playbackVolume != volume) {
        m_playbackVolume = volume;
        emit playbackVolumeChanged();
    }
}

int SoundPanel::recordingVolume() const {
    return m_recordingVolume;
}

void SoundPanel::setRecordingVolume(int volume) {
    if (m_recordingVolume != volume) {
        m_recordingVolume = volume;
        emit recordingVolumeChanged();
    }
}

void SoundPanel::onPlaybackVolumeChanged(int volume) {
    AudioManager::setPlaybackVolume(volume);
    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMute(false);
    }
    emit shouldUpdateTray();
}

void SoundPanel::onRecordingVolumeChanged(int volume) {
    AudioManager::setRecordingVolume(volume);
}

void SoundPanel::populateComboBoxes() {
    playbackDevices.clear();
    recordingDevices.clear();

    AudioManager::enumeratePlaybackDevices(playbackDevices);
    AudioManager::enumerateRecordingDevices(recordingDevices);

    QList<QObject*> rootObjects = engine->rootObjects();
    if (rootObjects.isEmpty()) {
        return;
    }

    QObject* rootObject = rootObjects[0];

    QMetaObject::invokeMethod(rootObject, "clearPlaybackDevices");

    int defaultPlaybackIndex = -1;
    for (int i = 0; i < playbackDevices.size(); ++i) {
        const AudioDevice& device = playbackDevices[i];
        if (device.shortName == "") {
            QMetaObject::invokeMethod(rootObject, "addPlaybackDevice",
                                      Q_ARG(QVariant, QVariant::fromValue(device.name)));
        } else {
            QMetaObject::invokeMethod(rootObject, "addPlaybackDevice",
                                      Q_ARG(QVariant, QVariant::fromValue(device.shortName)));
        }

        if (device.isDefault) {
            defaultPlaybackIndex = i;
        }
    }

    if (defaultPlaybackIndex != -1) {
        QMetaObject::invokeMethod(rootObject, "setPlaybackDeviceCurrentIndex",
                                  Q_ARG(QVariant, defaultPlaybackIndex));
    }

    QMetaObject::invokeMethod(rootObject, "clearRecordingDevices");

    int defaultRecordingIndex = -1;
    for (int i = 0; i < recordingDevices.size(); ++i) {
        const AudioDevice& device = recordingDevices[i];
        QMetaObject::invokeMethod(rootObject, "addRecordingDevice",
                                  Q_ARG(QVariant, QVariant::fromValue(device.shortName)));

        if (device.isDefault) {
            defaultRecordingIndex = i;
        }
    }

    if (defaultRecordingIndex != -1) {
        QMetaObject::invokeMethod(rootObject, "setRecordingDeviceCurrentIndex",
                                  Q_ARG(QVariant, defaultRecordingIndex));
    }
}

void SoundPanel::onPlaybackDeviceChanged(const QString &deviceName) {
    const auto& devices = playbackDevices;
    for (const AudioDevice& device : devices) {
        if (device.shortName == deviceName) {
            AudioManager::setDefaultEndpoint(device.id);
            setPlaybackVolume(AudioManager::getPlaybackVolume());
            break;
        }
    }
}

void SoundPanel::onRecordingDeviceChanged(const QString &deviceName) {
    const auto& devices = recordingDevices;
    for (const AudioDevice& device : devices) {
        if (device.shortName == deviceName) {
            AudioManager::setDefaultEndpoint(device.id);
            setRecordingVolume(AudioManager::getRecordingVolume());
            break;
        }
    }
}

void SoundPanel::onOutputMuteButtonClicked() {
    bool isMuted = AudioManager::getPlaybackMute();
    AudioManager::setPlaybackMute(!isMuted);
    setPlaybackMuted(!isMuted);

    emit shouldUpdateTray();
}

void SoundPanel::onInputMuteButtonClicked() {
    bool isMuted = AudioManager::getRecordingMute();
    AudioManager::setRecordingMute(!isMuted);
    setRecordingMuted(!isMuted);
}

void SoundPanel::onOutputSliderReleased() {
    if (!systemSoundsMuted) Utils::playSoundNotification();
}

void SoundPanel::populateApplicationModel() {
    applications = AudioManager::enumerateAudioApplications();

    QMap<QString, QVector<Application>> groupedApps;
    QVector<Application> systemSoundApps;

    const auto& localApplications = applications;
    for (const Application &app : localApplications) {
        if (app.executableName.isEmpty() ||
            app.executableName.compare("QuickSoundSwitcher", Qt::CaseInsensitive) == 0) {
            continue;
        }

        if (app.name == "Windows system sounds") {
            systemSoundApps.append(app);
            continue;
        }
        groupedApps[app.executableName].append(app);
    }

    QList<QObject*> rootObjects = engine->rootObjects();
    if (rootObjects.isEmpty()) {
        return;
    }
    QObject* rootObject = rootObjects[0];

    QMetaObject::invokeMethod(rootObject, "clearApplicationModel");

    for (auto it = groupedApps.begin(); it != groupedApps.end(); ++it) {
        QString executableName = it.key();
        QVector<Application> appGroup = it.value();

        if (appGroup.isEmpty()) {
            continue;
        }

        QString displayName = appGroup[0].name.isEmpty() ? executableName : appGroup[0].name;

        bool isMuted = std::any_of(appGroup.begin(), appGroup.end(), [](const Application &app) {
            return app.isMuted;
        });
        int volume = 0;
        if (!appGroup.isEmpty()) {
            volume = std::max_element(appGroup.begin(), appGroup.end(), [](const Application &a, const Application &b) {
                         return a.volume < b.volume;
                     })->volume;
        }

        QPixmap iconPixmap = appGroup[0].icon.pixmap(16, 16);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        iconPixmap.toImage().save(&buffer, "PNG");
        QString base64Icon = QString::fromUtf8(byteArray.toBase64());

        QStringList appIDs;
        const auto& localAppGroup = appGroup;
        for (const Application &app : localAppGroup) {
            appIDs.append(app.id);
        }

        QMetaObject::invokeMethod(rootObject, "addApplication",
                                  Q_ARG(QVariant, QVariant::fromValue(appIDs.join(";"))),
                                  Q_ARG(QVariant, QVariant::fromValue(displayName)),
                                  Q_ARG(QVariant, QVariant::fromValue(isMuted)),
                                  Q_ARG(QVariant, QVariant::fromValue(volume)),
                                  Q_ARG(QVariant, QVariant::fromValue(base64Icon)));
    }

    if (!systemSoundApps.isEmpty()) {
        QString displayName = systemSoundApps[0].name.isEmpty() ? "Windows system sounds" : systemSoundApps[0].name;

        bool isMuted = std::any_of(systemSoundApps.begin(), systemSoundApps.end(), [](const Application &app) {
            return app.isMuted;
        });
        int volume = 0;
        if (!systemSoundApps.isEmpty()) {
            volume = std::max_element(systemSoundApps.begin(), systemSoundApps.end(), [](const Application &a, const Application &b) {
                         return a.volume < b.volume;
                     })->volume;
        }

        QPixmap iconPixmap = systemSoundApps[0].icon.pixmap(16, 16);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        iconPixmap.toImage().save(&buffer, "PNG");
        QString base64Icon = QString::fromUtf8(byteArray.toBase64());

        QStringList appIDs;
        const auto& localSystemSoundApps = systemSoundApps;
        for (const Application &app : localSystemSoundApps) {
            appIDs.append(app.id);
        }

        QMetaObject::invokeMethod(rootObject, "addApplication",
                                  Q_ARG(QVariant, QVariant::fromValue(appIDs.join(";"))),
                                  Q_ARG(QVariant, QVariant::fromValue(displayName)),
                                  Q_ARG(QVariant, QVariant::fromValue(isMuted)),
                                  Q_ARG(QVariant, QVariant::fromValue(volume)),
                                  Q_ARG(QVariant, QVariant::fromValue(base64Icon)));

        systemSoundsMuted = isMuted;
    }
}

void SoundPanel::onApplicationVolumeSliderValueChanged(QString appID, int volume) {
    AudioManager::setApplicationVolume(appID, volume);
}

void SoundPanel::onApplicationMuteButtonClicked(QString appID, bool state) {
    if (appID == "0") systemSoundsMuted = state;

    AudioManager::setApplicationMute(appID, state);
}

bool SoundPanel::playbackMuted() const {
    return m_playbackMuted;
}

void SoundPanel::setPlaybackMuted(bool muted) {
    if (m_playbackMuted != muted) {
        m_playbackMuted = muted;
        emit playbackMutedChanged();
    }
}

bool SoundPanel::recordingMuted() const {
    return m_recordingMuted;
}

void SoundPanel::setRecordingMuted(bool muted) {
    if (m_recordingMuted != muted) {
        m_recordingMuted = muted;
        emit recordingMutedChanged();
    }
}
