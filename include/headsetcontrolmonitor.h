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
    int batteryLevel;       // -1 - 100
    QStringList capabilities;
    HeadsetControlDevice() : batteryLevel(0) {}
};
Q_DECLARE_METATYPE(HeadsetControlDevice)

class HeadsetControlMonitor : public QObject
{
    Q_OBJECT

public:
    explicit HeadsetControlMonitor(QObject *parent = nullptr);
    ~HeadsetControlMonitor();

    bool isMonitoring() const;
    QList<HeadsetControlDevice> getCachedDevices() const { return m_cachedDevices; }

    bool hasSidetoneCapability() const { return m_hasSidetoneCapability; }
    bool hasLightsCapability() const { return m_hasLightsCapability; }
    QString deviceName() const { return m_deviceName; }
    QString batteryStatus() const { return m_batteryStatus; }
    int batteryLevel() const { return m_batteryLevel; }
    bool anyDeviceFound() const { return m_anyDeviceFound; }

public slots:
    void startMonitoring();
    void stopMonitoring();
    void setLights(bool enabled);
    void setSidetone(int value);

signals:
    void headsetDataUpdated(const QList<HeadsetControlDevice>& devices);
    void monitoringStateChanged(bool enabled);
    void capabilitiesChanged();
    void deviceNameChanged();
    void batteryStatusChanged();
    void batteryLevelChanged();
    void anyDeviceFoundChanged();

private slots:
    void fetchHeadsetInfo();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void parseHeadsetControlOutput(const QByteArray& output);
    QString getExecutablePath() const;
    void updateCapabilities();
    void executeHeadsetControlCommand(const QStringList& arguments);

    QTimer* m_fetchTimer;
    QProcess* m_process;
    QSettings m_settings;
    QList<HeadsetControlDevice> m_cachedDevices;  // Cached last successful result
    bool m_isMonitoring;

    bool m_hasSidetoneCapability;
    bool m_hasLightsCapability;
    QString m_deviceName;
    QString m_batteryStatus;
    int m_batteryLevel;
    bool m_anyDeviceFound;

    static const int FETCH_INTERVAL_MS = 5000;
};
