#include "soundpanelbridge.h"
#include "shortcutmanager.h"
#include <QBuffer>
#include <QPixmap>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QStyleHints>
#include <Windows.h>
#include <mmsystem.h>
#include "version.h"
#include "mediasessionmanager.h"
#include "languages.h"
#include "updater.h"
#include "logmanager.h"
#include <powrprof.h>
#include <wtsapi32.h>
#include <lm.h>

#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "user32.lib")


SoundPanelBridge* SoundPanelBridge::m_instance = nullptr;

SoundPanelBridge::SoundPanelBridge(QObject* parent)
    : QObject(parent)
    , settings("Odizinne", "QontrolPanel")
    , m_currentPanelMode(0)
    , m_darkMode(QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark)
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

    if (QGuiApplication::styleHints()) {
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                this, &SoundPanelBridge::onColorSchemeChanged);
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
    return ShortcutManager::isShortcutPresent("QontrolPanel.lnk");
}

void SoundPanelBridge::setStartupShortcut(bool enabled)
{
    ShortcutManager::manageShortcut(enabled, "QontrolPanel.lnk");
}

int SoundPanelBridge::panelMode() const
{
    return settings.value("panelMode", 0).toInt();
}

void SoundPanelBridge::refreshPanelModeState()
{
    emit panelModeChanged();
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

    QString translationFile = QString("./i18n/QontrolPanel_%1.qm").arg(languageCode);
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
    QString baseUrl = "https://github.com/Odizinne/QontrolPanel/raw/refs/heads/main/i18n/compiled/QontrolPanel_%1.qm";

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

    QString userAgent = QString("QontrolPanel/%1").arg(APP_VERSION_STRING);
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
        QString fileName = QString("QontrolPanel_%1.qm").arg(languageCode);
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
    LogManager::instance()->sendLog(LogManager::SoundPanelBridge,
                                    QString("Loading translation progress data from: %1").arg(progressFilePath));

    QFile file(progressFilePath);
    if (!file.exists()) {
        LogManager::instance()->sendWarn(LogManager::SoundPanelBridge,
                                         QString("Translation progress file does not exist: %1").arg(progressFilePath));
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        LogManager::instance()->sendCritical(LogManager::SoundPanelBridge,
                                             QString("Failed to open translation progress file: %1").arg(file.errorString()));
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        LogManager::instance()->sendCritical(LogManager::SoundPanelBridge,
                                             "Failed to parse translation progress JSON - invalid format");
        return;
    }

    m_translationProgress = doc.object();
    LogManager::instance()->sendLog(LogManager::SoundPanelBridge,
                                    "Translation progress data loaded successfully");
    emit translationProgressDataLoaded();
}

void SoundPanelBridge::downloadTranslationProgressFile()
{
    QString githubUrl = "https://raw.githubusercontent.com/Odizinne/QontrolPanel/main/i18n/compiled/translation_progress.json";
    LogManager::instance()->sendLog(LogManager::SoundPanelBridge,
                                    QString("Downloading translation progress file from: %1").arg(githubUrl));

    QNetworkRequest request(githubUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "QontrolPanel");
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            LogManager::instance()->sendLog(LogManager::SoundPanelBridge,
                                            QString("Translation progress data downloaded (%1 bytes)").arg(data.size()));

            // Save to file
            QString progressFilePath = getTranslationProgressPath();
            QFile file(progressFilePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
                LogManager::instance()->sendLog(LogManager::SoundPanelBridge,
                                                "Translation progress file saved successfully");
                // Load the data
                loadTranslationProgressData();
            } else {
                LogManager::instance()->sendCritical(LogManager::SoundPanelBridge,
                                                     QString("Failed to save translation progress file: %1").arg(file.errorString()));
            }
        } else {
            LogManager::instance()->sendCritical(LogManager::SoundPanelBridge,
                                                 QString("Failed to download translation progress: %1").arg(reply->errorString()));
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

bool SoundPanelBridge::darkMode() const
{
    return m_darkMode;
}

void SoundPanelBridge::updateDarkModeFromSystem()
{
    bool newDarkMode = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;

    if (m_darkMode != newDarkMode) {
        m_darkMode = newDarkMode;
        emit darkModeChanged();
    }
}

void SoundPanelBridge::onColorSchemeChanged()
{
    updateDarkModeFromSystem();
}

bool SoundPanelBridge::hasMultipleUsers()
{
    LogManager::instance()->sendLog(LogManager::SoundPanelBridge, "Checking for multiple users on system");

    LPUSER_INFO_1 buffer = nullptr;
    DWORD entriesRead = 0;
    DWORD totalEntries = 0;
    DWORD realUserCount = 0;
    NET_API_STATUS status = NetUserEnum(nullptr, 1, FILTER_NORMAL_ACCOUNT,
                                        (LPBYTE*)&buffer, MAX_PREFERRED_LENGTH,
                                        &entriesRead, &totalEntries, nullptr);
    if (status == NERR_Success) {
        LogManager::instance()->sendLog(LogManager::PowerManager,
                                        QString("Found %1 user accounts, analyzing...").arg(entriesRead));

        for (DWORD i = 0; i < entriesRead; i++) {
            // Skip built-in system accounts
            QString username = QString::fromWCharArray(buffer[i].usri1_name);
            // Skip common system accounts
            if (username.compare("Administrator", Qt::CaseInsensitive) != 0 &&
                username.compare("Guest", Qt::CaseInsensitive) != 0 &&
                username.compare("DefaultAccount", Qt::CaseInsensitive) != 0 &&
                username.compare("WDAGUtilityAccount", Qt::CaseInsensitive) != 0 &&
                !username.startsWith("_") &&
                !(buffer[i].usri1_flags & UF_ACCOUNTDISABLE)) {
                realUserCount++;
                LogManager::instance()->sendLog(LogManager::PowerManager,
                                                QString("Real user found: %1").arg(username));
            } else {
                LogManager::instance()->sendLog(LogManager::PowerManager,
                                                QString("Skipping system account: %1").arg(username));
            }
        }
        NetApiBufferFree(buffer);
        bool result = realUserCount > 1;
        LogManager::instance()->sendLog(LogManager::PowerManager,
                                        QString("Real users found: %1, hasMultipleUsers returning: %2")
                                            .arg(realUserCount).arg(result ? "true" : "false"));
        return result;
    }

    LogManager::instance()->sendCritical(LogManager::PowerManager,
                                         QString("Failed to enumerate users, NetUserEnum error: %1").arg(status));
    return false;
}

bool SoundPanelBridge::isHibernateSupported()
{
    SYSTEM_POWER_CAPABILITIES powerCaps;
    ZeroMemory(&powerCaps, sizeof(powerCaps));

    if (GetPwrCapabilities(&powerCaps)) {
        return powerCaps.SystemS4 && powerCaps.HiberFilePresent;
    }

    return false;
}

bool SoundPanelBridge::isSleepSupported()
{
    SYSTEM_POWER_CAPABILITIES powerCaps;
    if (GetPwrCapabilities(&powerCaps)) {
        return powerCaps.SystemS1 || powerCaps.SystemS2 || powerCaps.SystemS3;
    }
    return false;
}

bool SoundPanelBridge::enableShutdownPrivilege()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get a token for this process
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    // Get the LUID for the shutdown privilege
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

    if (GetLastError() != ERROR_SUCCESS) {
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

bool SoundPanelBridge::shutdown()
{
    if (!enableShutdownPrivilege()) {
        return false;
    }

    return ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_APPLICATION);
}

bool SoundPanelBridge::restart()
{
    if (!enableShutdownPrivilege()) {
        return false;
    }

    return ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_APPLICATION);
}

bool SoundPanelBridge::sleep()
{
    return SetSuspendState(FALSE, FALSE, FALSE);
}

bool SoundPanelBridge::hibernate()
{
    return SetSuspendState(TRUE, FALSE, FALSE);
}

bool SoundPanelBridge::lockAccount()
{
    return LockWorkStation();
}

bool SoundPanelBridge::signOut()
{
    return ExitWindowsEx(EWX_LOGOFF, SHTDN_REASON_MAJOR_APPLICATION);
}

bool SoundPanelBridge::switchAccount()
{
    // Lock workstation which allows user switching from lock screen
    return LockWorkStation();
}
