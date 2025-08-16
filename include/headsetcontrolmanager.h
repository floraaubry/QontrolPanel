#pragma once
#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSettings>

struct HeadsetControlDevice {
    QString deviceName;
    QString vendor;
    QString product;
    QString vendorId;
    QString productId;
    QString batteryStatus;  // "BATTERY_AVAILABLE", "BATTERY_CHARGING", "BATTERY_UNAVAILABLE"
    int batteryLevel;       // 0-100
    QStringList capabilities;

    HeadsetControlDevice() : batteryLevel(0) {}
};

Q_DECLARE_METATYPE(HeadsetControlDevice)

class HeadsetControlManager : public QObject
{
    Q_OBJECT

public:
    explicit HeadsetControlManager(QObject *parent = nullptr);
    ~HeadsetControlManager();

    static HeadsetControlManager* instance();

    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;

    Q_INVOKABLE void setMonitoringEnabled(bool enabled);

    // Get battery info for a specific device by VID/PID
    Q_INVOKABLE int getBatteryLevel(const QString& vendorId, const QString& productId) const;
    Q_INVOKABLE QString getBatteryStatus(const QString& vendorId, const QString& productId) const;

signals:
    void headsetDataUpdated(const QList<HeadsetControlDevice>& devices);
    void monitoringStateChanged(bool enabled);

private slots:
    void fetchHeadsetInfo();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void parseHeadsetControlOutput(const QByteArray& output);
    QString getExecutablePath() const;

    static HeadsetControlManager* m_instance;
    QTimer* m_fetchTimer;
    QProcess* m_process;
    QSettings m_settings;
    QList<HeadsetControlDevice> m_devices;
    bool m_isMonitoring;

    static const int FETCH_INTERVAL_MS = 5000; // 5 seconds
};
