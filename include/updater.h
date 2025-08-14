#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QQmlEngine>

class Updater : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY latestVersionChanged)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(bool isChecking READ isChecking NOTIFY isCheckingChanged)
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY isDownloadingChanged)
    Q_PROPERTY(int downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)

public:
    static Updater* instance();
    static Updater* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void downloadAndInstall();

    bool updateAvailable() const { return m_updateAvailable; }
    QString latestVersion() const { return m_latestVersion; }
    QString currentVersion() const { return getCurrentVersion(); }
    bool isChecking() const { return m_isChecking; }
    bool isDownloading() const { return m_isDownloading; }
    int downloadProgress() const { return m_downloadProgress; }

signals:
    void updateAvailableChanged();
    void latestVersionChanged();
    void isCheckingChanged();
    void isDownloadingChanged();
    void downloadProgressChanged();
    void updateFinished(bool success, const QString& message);

private slots:
    void onVersionCheckFinished();
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    explicit Updater(QObject *parent = nullptr);
    static Updater* m_instance;

    QNetworkAccessManager* m_networkManager;
    bool m_updateAvailable;
    bool m_isChecking;
    bool m_isDownloading;
    int m_downloadProgress;
    QString m_latestVersion;
    QString m_downloadUrl;

    QString getCurrentVersion() const;
    bool isNewerVersion(const QString& latest, const QString& current) const;
    void installExecutable(const QString& newExePath);
    void setChecking(bool checking);
    void setDownloading(bool downloading);
    void setDownloadProgress(int progress);
};
