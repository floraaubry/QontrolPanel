#ifndef SOUNDPANELBRIDGE_H
#define SOUNDPANELBRIDGE_H

#include "audiomanager.h"
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include <QSettings>
#include <QScreen>
#include <QGuiApplication>
#include <QTranslator>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QVariant>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

class SoundPanelBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int playbackVolume READ playbackVolume WRITE setPlaybackVolume NOTIFY playbackVolumeChanged)
    Q_PROPERTY(int recordingVolume READ recordingVolume WRITE setRecordingVolume NOTIFY recordingVolumeChanged)
    Q_PROPERTY(bool playbackMuted READ playbackMuted WRITE setPlaybackMuted NOTIFY playbackMutedChanged)
    Q_PROPERTY(bool recordingMuted READ recordingMuted WRITE setRecordingMuted NOTIFY recordingMutedChanged)
    Q_PROPERTY(int panelMode READ panelMode NOTIFY panelModeChanged)
    Q_PROPERTY(bool deviceChangeInProgress READ deviceChangeInProgress NOTIFY deviceChangeInProgressChanged)
    Q_PROPERTY(QString taskbarPosition READ taskbarPosition NOTIFY taskbarPositionChanged)

    Q_PROPERTY(QString mediaTitle READ mediaTitle NOTIFY mediaInfoChanged)
    Q_PROPERTY(QString mediaArtist READ mediaArtist NOTIFY mediaInfoChanged)
    Q_PROPERTY(bool isMediaPlaying READ isMediaPlaying NOTIFY mediaInfoChanged)
    Q_PROPERTY(QString mediaArt READ mediaArt NOTIFY mediaInfoChanged)

    // ChatMix properties
    Q_PROPERTY(int chatMixValue READ chatMixValue WRITE setChatMixValue NOTIFY chatMixValueChanged)
    Q_PROPERTY(QVariantList commAppsList READ commAppsList NOTIFY commAppsListChanged)

    Q_PROPERTY(QVariantList applicationAudioLevels READ applicationAudioLevels NOTIFY applicationAudioLevelsChanged)
    Q_PROPERTY(int playbackAudioLevel READ playbackAudioLevel NOTIFY playbackAudioLevelChanged)
    Q_PROPERTY(int recordingAudioLevel READ recordingAudioLevel NOTIFY recordingAudioLevelChanged)


public:
    explicit SoundPanelBridge(QObject* parent = nullptr);
    ~SoundPanelBridge() override;

    static SoundPanelBridge* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
    static SoundPanelBridge* instance();

    int playbackVolume() const;
    void setPlaybackVolume(int volume);

    int recordingVolume() const;
    void setRecordingVolume(int volume);

    bool playbackMuted() const;
    void setPlaybackMuted(bool muted);

    bool recordingMuted() const;
    void setRecordingMuted(bool muted);

    int panelMode() const;
    QString taskbarPosition() const;

    Q_INVOKABLE void initializeData();
    Q_INVOKABLE void refreshData();
    Q_INVOKABLE bool getShortcutState();
    Q_INVOKABLE void setStartupShortcut(bool enabled);
    Q_INVOKABLE bool getDarkMode();
    Q_INVOKABLE QString getAppVersion() const;
    Q_INVOKABLE QString getQtVersion() const;
    Q_INVOKABLE QString getCommitHash() const;
    Q_INVOKABLE QString getBuildTimestamp() const;

    QString mediaTitle() const;
    QString mediaArtist() const;
    bool isMediaPlaying() const;
    QString mediaArt() const;

    bool deviceChangeInProgress() const;

    Q_INVOKABLE void playPause();
    Q_INVOKABLE void nextTrack();
    Q_INVOKABLE void previousTrack();
    Q_INVOKABLE void startMediaMonitoring();
    Q_INVOKABLE void stopMediaMonitoring();

    Q_INVOKABLE QString getCurrentLanguageCode() const;
    Q_INVOKABLE void changeApplicationLanguage(int languageIndex);
    Q_INVOKABLE QString getLanguageCodeFromIndex(int index) const;

    int chatMixValue() const;
    void setChatMixValue(int value);
    QVariantList commAppsList() const;
    Q_INVOKABLE void applyChatMixToApplications();
    Q_INVOKABLE bool isCommApp(const QString& name) const;
    Q_INVOKABLE void addCommApp(const QString& name);
    Q_INVOKABLE void removeCommApp(const QString& name);
    Q_INVOKABLE void restoreOriginalVolumes();
    Q_INVOKABLE void updateMissingCommAppIcons();

    QVariantList applicationAudioLevels() const;
    int playbackAudioLevel() const;
    int recordingAudioLevel() const;

    Q_INVOKABLE void startAudioLevelMonitoring();
    Q_INVOKABLE void stopAudioLevelMonitoring();

    Q_INVOKABLE void toggleChatMixFromShortcut(bool enabled);
    Q_INVOKABLE void suspendGlobalShortcuts();
    Q_INVOKABLE void resumeGlobalShortcuts();
    bool areGlobalShortcutsSuspended() const;
    void requestChatMixNotification(QString message);

    Q_INVOKABLE void downloadLatestTranslations();
    Q_INVOKABLE void cancelTranslationDownload();
    Q_INVOKABLE QStringList getLanguageNativeNames() const;
    Q_INVOKABLE QStringList getLanguageCodes() const;

public slots:
    void onPlaybackVolumeChanged(int volume);
    void onRecordingVolumeChanged(int volume);
    void onPlaybackDeviceChanged(const QString &deviceName);
    void onRecordingDeviceChanged(const QString &deviceName);
    void onOutputMuteButtonClicked();
    void onInputMuteButtonClicked();
    void onOutputSliderReleased();
    void onApplicationVolumeSliderValueChanged(QString appID, int volume);
    void onApplicationMuteButtonClicked(QString appID, bool state);

    void updateVolumeFromTray(int volume);
    void updateMuteStateFromTray(bool muted);
    void refreshPanelModeState();

private slots:
    void checkForTranslationUpdates();

signals:
    void playbackVolumeChanged();
    void recordingVolumeChanged();
    void playbackMutedChanged();
    void recordingMutedChanged();
    void panelModeChanged();
    void shouldUpdateTray();
    void playbackDevicesChanged(const QVariantList& devices);
    void recordingDevicesChanged(const QVariantList& devices);
    void applicationsChanged(const QVariantList& applications);
    void dataInitializationComplete();
    void deviceChangeInProgressChanged();
    void taskbarPositionChanged();
    void mediaInfoChanged();
    void languageChanged();
    void chatMixValueChanged();
    void commAppsListChanged();
    void applicationAudioLevelsChanged();
    void playbackAudioLevelChanged();
    void recordingAudioLevelChanged();
    void chatMixEnabledChanged(bool enabled);
    void chatMixNotificationRequested(QString mesasge);
    void translationDownloadStarted();
    void translationDownloadProgress(const QString& language, int bytesReceived, int bytesTotal);
    void translationDownloadFinished(bool success, const QString& message);
    void translationDownloadError(const QString& errorMessage);
    void translationFileCompleted(const QString& language, int completed, int total);

private:
    static SoundPanelBridge* m_instance;
    QList<AudioDevice> playbackDevices;
    QList<AudioDevice> recordingDevices;
    QList<Application> applications;
    int m_playbackVolume = 0;
    int m_recordingVolume = 0;
    bool m_playbackMuted = false;
    bool m_recordingMuted = false;
    QSettings settings;
    bool systemSoundsMuted;

    void populatePlaybackDevices();
    void populateRecordingDevices();
    void populateApplications();
    QVariantList convertDevicesToVariant(const QList<AudioDevice>& devices);
    QVariantList convertApplicationsToVariant(const QList<Application>& apps);

    bool m_playbackDevicesReady = false;
    bool m_recordingDevicesReady = false;
    bool m_currentPropertiesReady = false;
    bool m_applicationsReady = false;
    bool m_isInitializing = false;
    int m_currentPanelMode = 0;

    void checkDataInitializationComplete();
    bool m_deviceChangeInProgress = false;

    QString detectTaskbarPosition() const;
    QString m_mediaTitle;
    QString m_mediaArtist;
    bool m_isMediaPlaying = false;
    QString m_mediaArt;

    QTranslator *translator;

    struct CommApp {
        QString name;
        QString icon;
    };

    int m_chatMixValue = 50;
    QList<CommApp> m_commApps;
    QTimer* m_chatMixCheckTimer;

    void checkAndApplyChatMixToNewApps();
    QStringList getCommAppsFromSettings() const;
    void startChatMixMonitoring();
    void stopChatMixMonitoring();
    QString getCommAppsFilePath() const;
    void loadCommAppsFromFile();
    void saveCommAppsToFile();

    QVariantMap m_applicationAudioLevels;
    int m_playbackAudioLevel = 0;
    int m_recordingAudioLevel = 0;
    bool m_globalShortcutsSuspended = false;

    QNetworkAccessManager* m_networkManager;
    QList<QNetworkReply*> m_activeDownloads;
    int m_totalDownloads;
    int m_completedDownloads;
    int m_failedDownloads;

    void downloadTranslationFile(const QString& languageCode, const QString& githubUrl);
    void onTranslationFileDownloaded();
    QString getTranslationDownloadPath() const;

    QTimer* m_autoUpdateTimer;
};

#endif // SOUNDPANELBRIDGE_H
