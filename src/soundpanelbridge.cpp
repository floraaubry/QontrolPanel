#include "soundpanelbridge.h"
#include "shortcutmanager.h"
#include <QBuffer>
#include <QPixmap>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <Windows.h>
#include <mmsystem.h>
#include "version.h"
#include "mediasessionmanager.h"
#include "languages.h"
#include "updater.h"

SoundPanelBridge* SoundPanelBridge::m_instance = nullptr;

SoundPanelBridge::SoundPanelBridge(QObject* parent)
    : QObject(parent)
    , settings("Odizinne", "QuickSoundSwitcher")
    , m_currentPanelMode(0)
    , translator(new QTranslator(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_totalDownloads(0)
    , m_completedDownloads(0)
    , m_failedDownloads(0)
    , m_autoUpdateTimer(new QTimer(this))
    , m_autoUpdateCheckTimer(new QTimer(this))
{
    m_instance = this;
    changeApplicationLanguage(settings.value("languageIndex", 0).toInt());
    loadTranslationProgressData();

    if (MediaSessionManager::getWorker()) {
        connect(MediaSessionManager::getWorker(), &MediaWorker::mediaInfoChanged,
                this, [this](const MediaInfo& info) {
                    m_mediaTitle = info.title;
                    m_mediaArtist = info.artist;
                    m_mediaArt = info.albumArt;
                    m_isMediaPlaying = info.isPlaying;
                    emit mediaInfoChanged();
                });
    }

    m_autoUpdateTimer->setInterval(4 * 60 * 60 * 1000);
    m_autoUpdateTimer->setSingleShot(false);
    connect(m_autoUpdateTimer, &QTimer::timeout, this, &SoundPanelBridge::checkForTranslationUpdates);
    m_autoUpdateTimer->start();

    if (settings.value("autoUpdateTranslations", false).toBool()) {
        QTimer::singleShot(5000, this, &SoundPanelBridge::checkForTranslationUpdates);
    }

    m_autoUpdateCheckTimer->setInterval(4 * 60 * 60 * 1000);
    m_autoUpdateCheckTimer->setSingleShot(false);
    connect(m_autoUpdateCheckTimer, &QTimer::timeout, this, &SoundPanelBridge::checkForAppUpdates);
    m_autoUpdateCheckTimer->start();

    if (settings.value("autoFetchForAppUpdates", false).toBool()) {
        QTimer::singleShot(5000, this, &SoundPanelBridge::checkForAppUpdates);
    }
}

SoundPanelBridge::~SoundPanelBridge()
{
    if (m_instance == this) {
        m_instance = nullptr;
    }
}

SoundPanelBridge* SoundPanelBridge::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    if (!m_instance) {
        m_instance = new SoundPanelBridge();
    }
    return m_instance;
}

SoundPanelBridge* SoundPanelBridge::instance()
{
    return m_instance;
}

void SoundPanelBridge::checkForTranslationUpdates()
{
    if (!settings.value("autoUpdateTranslations", false).toBool()) {
        m_autoUpdateTimer->stop();
        return;
    }

    if (m_activeDownloads.isEmpty()) {
        downloadLatestTranslations();
    }
}

bool SoundPanelBridge::getShortcutState()
{
    return ShortcutManager::isShortcutPresent("QuickSoundSwitcher.lnk");
}

void SoundPanelBridge::setStartupShortcut(bool enabled)
{
    ShortcutManager::manageShortcut(enabled, "QuickSoundSwitcher.lnk");
}

int SoundPanelBridge::panelMode() const
{
    return settings.value("panelMode", 0).toInt();
}

void SoundPanelBridge::refreshPanelModeState()
{
    emit panelModeChanged();
}

bool SoundPanelBridge::getDarkMode() {
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
}

QString SoundPanelBridge::taskbarPosition() const
{
    return detectTaskbarPosition();
}

QString SoundPanelBridge::detectTaskbarPosition() const
{
    switch (settings.value("panelPosition", 1).toInt()) {
    case 0:
        return "top";
    case 1:
        return "bottom";
    case 2:
        return "left";
    case 3:
        return "right";
    default:
        return "bottom";
    }
}

QString SoundPanelBridge::getAppVersion() const
{
    return APP_VERSION_STRING;
}

QString SoundPanelBridge::getQtVersion() const
{
    return QT_VERSION_STRING;
}

QString SoundPanelBridge::mediaTitle() const {
    return m_mediaTitle;
}

QString SoundPanelBridge::mediaArtist() const {
    return m_mediaArtist;
}

bool SoundPanelBridge::isMediaPlaying() const {
    return m_isMediaPlaying;
}

QString SoundPanelBridge::mediaArt() const {
    return m_mediaArt;
}

void SoundPanelBridge::playPause() {
    MediaSessionManager::playPauseAsync();
}

void SoundPanelBridge::nextTrack() {
    MediaSessionManager::nextTrackAsync();
}

void SoundPanelBridge::previousTrack() {
    MediaSessionManager::previousTrackAsync();
}

void SoundPanelBridge::startMediaMonitoring() {
    if (settings.value("displayMediaInfos", true).toBool()) {
        MediaSessionManager::startMonitoringAsync();
    }
}

void SoundPanelBridge::stopMediaMonitoring() {
    MediaSessionManager::stopMonitoringAsync();
}

QString SoundPanelBridge::getCurrentLanguageCode() const {
    QLocale locale;
    QString languageCode = locale.name().section('_', 0, 0);

    if (languageCode == "zh") {
        QString fullLocale = locale.name();
        if (fullLocale.startsWith("zh_CN")) {
            return "zh_CN";
        }
        return "zh_CN";
    }

    return languageCode;
}

void SoundPanelBridge::changeApplicationLanguage(int languageIndex)
{
    qApp->removeTranslator(translator);
    delete translator;
    translator = new QTranslator(this);

    QString languageCode;
    if (languageIndex == 0) {
        languageCode = getCurrentLanguageCode();
    } else {
        languageCode = getLanguageCodeFromIndex(languageIndex);
    }

    QString translationFile = QString("./i18n/QuickSoundSwitcher_%1.qm").arg(languageCode);
    if (translator->load(translationFile)) {
        qGuiApp->installTranslator(translator);
    } else {
        qWarning() << "Failed to load translation file:" << translationFile;
    }

    emit languageChanged();
}

QString SoundPanelBridge::getLanguageCodeFromIndex(int index) const
{
    if (index == 0) {
        QLocale systemLocale;
        QString langCode = systemLocale.name().section('_', 0, 0);

        if (langCode == "zh") {
            QString fullLocale = systemLocale.name();
            if (fullLocale.startsWith("zh_CN")) {
                return "zh_CN";
            }
            return "zh_CN";
        }

        return langCode;
    }

    auto languages = getSupportedLanguages();
    if (index > 0 && index <= languages.size()) {
        return languages[index - 1].code;
    }

    return "en";
}

QString SoundPanelBridge::getCommitHash() const
{
    return QString(GIT_COMMIT_HASH);
}

QString SoundPanelBridge::getBuildTimestamp() const
{
    return QString(BUILD_TIMESTAMP);
}

void SoundPanelBridge::toggleChatMixFromShortcut(bool enabled)
{
    emit chatMixEnabledChanged(enabled);
}

void SoundPanelBridge::suspendGlobalShortcuts()
{
    m_globalShortcutsSuspended = true;
}

void SoundPanelBridge::resumeGlobalShortcuts()
{
    m_globalShortcutsSuspended = false;
}

bool SoundPanelBridge::areGlobalShortcutsSuspended() const
{
    return m_globalShortcutsSuspended;
}

void SoundPanelBridge::requestChatMixNotification(QString message) {
    emit chatMixNotificationRequested(message);
}

void SoundPanelBridge::downloadLatestTranslations()
{
    cancelTranslationDownload();

    m_totalDownloads = 0;
    m_completedDownloads = 0;
    m_failedDownloads = 0;

    QStringList languageCodes = getLanguageCodes();
    QString baseUrl = "https://github.com/Odizinne/QuickSoundSwitcher/raw/refs/heads/main/i18n/compiled/QuickSoundSwitcher_%1.qm";

    m_totalDownloads = languageCodes.size();
    emit translationDownloadStarted();

    for (const QString& langCode : languageCodes) {
        QString url = baseUrl.arg(langCode);
        downloadTranslationFile(langCode, url);
    }

    downloadTranslationProgressFile();

    if (m_totalDownloads == 0) {
        emit translationDownloadFinished(false, tr("No translation files to download"));
    }
}

void SoundPanelBridge::cancelTranslationDownload()
{
    for (QNetworkReply* reply : m_activeDownloads) {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
    }
    m_activeDownloads.clear();
}

void SoundPanelBridge::downloadTranslationFile(const QString& languageCode, const QString& githubUrl)
{
    QUrl url(githubUrl);
    QNetworkRequest request;
    request.setUrl(url);

    QString userAgent = QString("QuickSoundSwitcher/%1").arg(APP_VERSION_STRING);
    request.setRawHeader("User-Agent", userAgent.toUtf8());
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Connection", "keep-alive");
    request.setTransferTimeout(30000);

    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("languageCode", languageCode);
    m_activeDownloads.append(reply);

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, languageCode](qint64 bytesReceived, qint64 bytesTotal) {
                emit translationDownloadProgress(languageCode,
                                                 static_cast<int>(bytesReceived),
                                                 static_cast<int>(bytesTotal));
            });

    connect(reply, &QNetworkReply::finished, this, &SoundPanelBridge::onTranslationFileDownloaded);

    connect(reply, &QNetworkReply::errorOccurred, this,
            [this, languageCode](QNetworkReply::NetworkError error) {
                qWarning() << "Network error for" << languageCode << ":" << error;
            });
}

void SoundPanelBridge::onTranslationFileDownloaded()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QString languageCode = reply->property("languageCode").toString();
    m_activeDownloads.removeAll(reply);

    if (reply->error() == QNetworkReply::NoError) {
        QString downloadPath = getTranslationDownloadPath();
        QString fileName = QString("QuickSoundSwitcher_%1.qm").arg(languageCode);
        QString filePath = downloadPath + "/" + fileName;

        QDir().mkpath(downloadPath);

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            QByteArray data = reply->readAll();
            if (!data.isEmpty()) {
                file.write(data);
                file.close();
            } else {
                qWarning() << "Downloaded empty file for:" << languageCode;
                m_failedDownloads++;
            }
        } else {
            qWarning() << "Failed to save translation file:" << filePath;
            m_failedDownloads++;
        }
    } else {
        qWarning() << "Failed to download translation for" << languageCode
                   << "Error:" << reply->error()
                   << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                   << "Message:" << reply->errorString();
        m_failedDownloads++;
    }

    m_completedDownloads++;
    emit translationFileCompleted(languageCode, m_completedDownloads, m_totalDownloads);

    if (m_completedDownloads >= m_totalDownloads) {
        bool success = (m_failedDownloads == 0);
        QString message;

        if (success) {
            message = tr("All translations downloaded successfully");
            changeApplicationLanguage(settings.value("languageIndex", 0).toInt());
        } else {
            message = tr("Downloaded %1 of %2 translation files")
            .arg(m_totalDownloads - m_failedDownloads)
                .arg(m_totalDownloads);
        }

        emit translationDownloadFinished(success, message);

        if (success) {
            int currentLangIndex = settings.value("languageIndex", 0).toInt();
            changeApplicationLanguage(currentLangIndex);
        }
    }

    reply->deleteLater();
}

QString SoundPanelBridge::getTranslationDownloadPath() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    return appDir + "/i18n";
}

QStringList SoundPanelBridge::getLanguageNativeNames() const
{
    auto names = ::getLanguageNativeNames();
    return names;
}

QStringList SoundPanelBridge::getLanguageCodes() const
{
    return ::getLanguageCodes();
}

void SoundPanelBridge::openLegacySoundSettings() {
    QProcess::startDetached("control", QStringList() << "mmsys.cpl");
}

void SoundPanelBridge::openModernSoundSettings() {
    QProcess::startDetached("explorer", QStringList() << "ms-settings:sound");
}

int SoundPanelBridge::getAvailableDesktopWidth() const
{
    if (QGuiApplication::primaryScreen()) {
        return QGuiApplication::primaryScreen()->availableGeometry().width();
    }
    return 1920;
}

int SoundPanelBridge::getAvailableDesktopHeight() const
{
    if (QGuiApplication::primaryScreen()) {
        return QGuiApplication::primaryScreen()->availableGeometry().height();
    }
    return 1080;
}

void SoundPanelBridge::playFeedbackSound()
{
    PlaySound(TEXT("SystemDefault"), NULL, SND_ALIAS | SND_ASYNC);
}

QString SoundPanelBridge::getTranslationProgressPath() const
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    return appDataPath + "/translation_progress.json";
}

void SoundPanelBridge::loadTranslationProgressData()
{
    QString progressFilePath = getTranslationProgressPath();
    QFile file(progressFilePath);

    if (!file.exists()) {
        qDebug() << "Translation progress file does not exist:" << progressFilePath;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open translation progress file:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull()) {
        qDebug() << "Failed to parse translation progress JSON";
        return;
    }

    m_translationProgress = doc.object();
    emit translationProgressDataLoaded();

    qDebug() << "Loaded translation progress data for" << m_translationProgress.size() << "languages";
}

void SoundPanelBridge::downloadTranslationProgressFile()
{
    QString githubUrl = "https://raw.githubusercontent.com/Odizinne/QuickSoundSwitcher/main/i18n/compiled/translation_progress.json";

    QNetworkRequest request(githubUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "QuickSoundSwitcher");

    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();

            // Save to file
            QString progressFilePath = getTranslationProgressPath();
            QFile file(progressFilePath);

            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
                qDebug() << "Translation progress data downloaded successfully";

                // Load the data
                loadTranslationProgressData();
            } else {
                qDebug() << "Failed to save translation progress file:" << file.errorString();
            }
        } else {
            qDebug() << "Failed to download translation progress:" << reply->errorString();
        }

        reply->deleteLater();
    });
}

int SoundPanelBridge::getTranslationProgress(const QString& languageCode)
{
    if (m_translationProgress.contains(languageCode)) {
        return m_translationProgress[languageCode].toInt();
    }
    return 0; // Return 0% if language not found
}

bool SoundPanelBridge::hasTranslationProgressData()
{
    return !m_translationProgress.isEmpty();
}

void SoundPanelBridge::checkForAppUpdates()
{
    // Only proceed if auto fetch is enabled
    if (!settings.value("autoFetchForAppUpdates", false).toBool()) {
        return;
    }

    // Only check if not already checking/downloading
    if (Updater::instance()->isChecking() || Updater::instance()->isDownloading()) {
        return;
    }

    // Connect to updater signals to handle the result
    connect(Updater::instance(), &Updater::updateFinished, this,
            [this](bool success, const QString& message) {
                // Disconnect to avoid multiple connections
                disconnect(Updater::instance(), &Updater::updateFinished, this, nullptr);

                if (success && Updater::instance()->updateAvailable()) {
                    // Emit signal for QML to show tray notification
                    emit updateAvailableNotification(Updater::instance()->latestVersion());
                }
            }, Qt::SingleShotConnection);

    // Start the update check
    Updater::instance()->checkForUpdates();
}
