#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

class LogManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum Sender {
        AudioManager,
        MediaSessionManager,
        MonitorManager,
        SoundPanelBridge,
        Updater,
        ShortcutManager,
        Core,
        LocalServer,
        Ui,
        PowerManager,
        HeadsetControlManager,
        WindowFocusManager
    };
    Q_ENUM(Sender)

    enum LogType {
        Info,
        Warning,
        Critical
    };
    Q_ENUM(LogType)

    static LogManager* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static LogManager* instance();

    Q_INVOKABLE void sendLog(Sender sender, const QString &content);
    Q_INVOKABLE void sendWarn(Sender sender, const QString &content);
    Q_INVOKABLE void sendCritical(Sender sender, const QString &content);
    Q_INVOKABLE void setQmlReady();

    Q_INVOKABLE void setAudioManagerLogging(bool enabled);
    Q_INVOKABLE void setMediaSessionManagerLogging(bool enabled);
    Q_INVOKABLE void setMonitorManagerLogging(bool enabled);
    Q_INVOKABLE void setSoundPanelBridgeLogging(bool enabled);
    Q_INVOKABLE void setUpdaterLogging(bool enabled);
    Q_INVOKABLE void setShortcutManagerLogging(bool enabled);
    Q_INVOKABLE void setCoreLogging(bool enabled);
    Q_INVOKABLE void setLocalServerLogging(bool enabled);
    Q_INVOKABLE void setUiLogging(bool enabled);
    Q_INVOKABLE void setPowerManagerLogging(bool enabled);
    Q_INVOKABLE void setHeadsetControlManagerLogging(bool enabled);
    Q_INVOKABLE void setWindowFocusManagerLogging(bool enabled);

signals:
    void logReceived(const QString &formattedMessage, LogType type);
    void bufferedLogsReady(const QVariantList &logs);

private:
    explicit LogManager(QObject *parent = nullptr);
    static LogManager* m_instance;

    QString senderToString(Sender sender) const;
    QString formatMessage(Sender sender, LogType type, const QString &content) const;
    void emitLog(Sender sender, LogType type, const QString &content);
    bool isLoggingEnabled(Sender sender) const;

    QList<QPair<QString, LogType>> m_bufferedLogs;
    bool m_qmlReady = false;

    bool m_audioManagerLogging;
    bool m_mediaSessionManagerLogging;
    bool m_monitorManagerLogging;
    bool m_soundPanelBridgeLogging;
    bool m_updaterLogging;
    bool m_shortcutManagerLogging;
    bool m_coreLogging;
    bool m_localServerLogging;
    bool m_uiLogging;
    bool m_powerManagerLogging;
    bool m_headsetControlManagerLogging;
    bool m_windowFocusManagerLogging;
};

#endif // LOGMANAGER_H
