#include "updater.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include <QDebug>
#include "version.h"

Updater* Updater::m_instance = nullptr;

Updater* Updater::instance()
{
    if (!m_instance) {
        m_instance = new Updater();
    }
    return m_instance;
}

Updater* Updater::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return instance();
}

Updater::Updater(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_updateAvailable(false)
    , m_isChecking(false)
    , m_isDownloading(false)
    , m_downloadProgress(0)
{
}

void Updater::checkForUpdates()
{
    if (m_isChecking || m_isDownloading) {
        return;
    }

    setChecking(true);

    QString urlString = "https://api.github.com/repos/Odizinne/QuickSoundSwitcher/releases/latest";
    QUrl url{urlString};
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::UserAgentHeader, "QuickSoundSwitcher-Updater");

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &Updater::onVersionCheckFinished);
}

void Updater::onVersionCheckFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();

    setChecking(false);

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Update check failed:" << reply->errorString();
        emit updateFinished(false, "Failed to check for updates: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    m_latestVersion = obj["tag_name"].toString();
    if (m_latestVersion.startsWith("v")) {
        m_latestVersion = m_latestVersion.mid(1); // Remove 'v' prefix
    }

    // Find the .exe asset
    QJsonArray assets = obj["assets"].toArray();
    m_downloadUrl.clear();

    for (const auto& asset : assets) {
        QJsonObject assetObj = asset.toObject();
        QString name = assetObj["name"].toString();
        if (name.endsWith(".exe") && name.contains("QuickSoundSwitcher")) {
            m_downloadUrl = assetObj["browser_download_url"].toString();
            break;
        }
    }

    if (m_downloadUrl.isEmpty()) {
        emit updateFinished(false, "No executable found in latest release");
        return;
    }

    QString currentVersion = getCurrentVersion();
    bool wasUpdateAvailable = m_updateAvailable;
    m_updateAvailable = isNewerVersion(m_latestVersion, currentVersion);

    if (wasUpdateAvailable != m_updateAvailable) {
        emit updateAvailableChanged();
    }
    emit latestVersionChanged();

    if (m_updateAvailable) {
        emit updateFinished(true, "Update available: " + m_latestVersion);
    } else {
        emit updateFinished(true, "You are using the latest version");
    }
}

void Updater::downloadAndInstall()
{
    if (m_downloadUrl.isEmpty() || m_isDownloading || m_isChecking) {
        emit updateFinished(false, "Cannot start download");
        return;
    }

    setDownloading(true);
    setDownloadProgress(0);

    QUrl url{m_downloadUrl};
    QNetworkRequest request{url};
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this, &Updater::onDownloadProgress);
    connect(reply, &QNetworkReply::finished, this, &Updater::onDownloadFinished);
}

void Updater::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
        setDownloadProgress(percentage);
    }
}

void Updater::onDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();

    setDownloading(false);
    setDownloadProgress(0);

    if (reply->error() != QNetworkReply::NoError) {
        emit updateFinished(false, "Download failed: " + reply->errorString());
        return;
    }

    // Save to temp file
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFile = tempDir + "/QuickSoundSwitcher_update.exe";

    QFile file(tempFile);
    if (!file.open(QIODevice::WriteOnly)) {
        emit updateFinished(false, "Failed to save update file");
        return;
    }

    file.write(reply->readAll());
    file.close();

    installExecutable(tempFile);
}

void Updater::installExecutable(const QString& newExePath)
{
    QString currentExeDir = QApplication::applicationDirPath();
    QDir dir(currentExeDir);
    dir.cdUp();
    dir.cdUp();
    QString targetDir = QDir::toNativeSeparators(dir.absolutePath());

    QStringList arguments;
    arguments << "/CLOSEAPPLICATIONS";
    arguments << "/FORCECLOSEAPPLICATIONS";
    arguments << "/RESTARTAPPLICATIONS";
    arguments << QString("/DIR=%1").arg(targetDir);
    arguments << "/SILENT";
    arguments << "/NOCANCEL";

    bool started = QProcess::startDetached(newExePath, arguments);

    if (started) {
        emit updateFinished(true, "Update started. Application will restart.");
        QApplication::quit();
    } else {
        emit updateFinished(false, "Failed to start update executable");
    }
}

QString Updater::getCurrentVersion() const
{
    return QString(APP_VERSION_STRING);
}

bool Updater::isNewerVersion(const QString& latest, const QString& current) const
{
    QStringList latestParts = latest.split('.');
    QStringList currentParts = current.split('.');

    // Pad with zeros if needed
    while (latestParts.size() < 3) latestParts.append("0");
    while (currentParts.size() < 3) currentParts.append("0");

    for (int i = 0; i < 3; ++i) {
        int latestNum = latestParts[i].toInt();
        int currentNum = currentParts[i].toInt();

        if (latestNum > currentNum) return true;
        if (latestNum < currentNum) return false;
    }

    return false; // Versions are equal
}

void Updater::setChecking(bool checking)
{
    if (m_isChecking != checking) {
        m_isChecking = checking;
        emit isCheckingChanged();
    }
}

void Updater::setDownloading(bool downloading)
{
    if (m_isDownloading != downloading) {
        m_isDownloading = downloading;
        emit isDownloadingChanged();
    }
}

void Updater::setDownloadProgress(int progress)
{
    if (m_downloadProgress != progress) {
        m_downloadProgress = progress;
        emit downloadProgressChanged();
    }
}
