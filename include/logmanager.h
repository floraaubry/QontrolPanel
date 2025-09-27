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
        FileSystem,
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

signals:
    void logReceived(const QString &formattedMessage, LogType type);
    void bufferedLogsReady(const QVariantList &logs);

private:
    explicit LogManager(QObject *parent = nullptr);
    static LogManager* m_instance;

    QString senderToString(Sender sender) const;
    QString formatMessage(Sender sender, LogType type, const QString &content) const;
    void emitLog(Sender sender, LogType type, const QString &content);
    QList<QPair<QString, LogType>> m_bufferedLogs;
    bool m_qmlReady = false;
};

#endif // LOGMANAGER_H
