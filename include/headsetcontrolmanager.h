#pragma once
#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSettings>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

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

class HeadsetControlManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool hasSidetoneCapability READ hasSidetoneCapability NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool hasLightsCapability READ hasLightsCapability NOTIFY capabilitiesChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(QString batteryStatus READ batteryStatus NOTIFY batteryStatusChanged)
    Q_PROPERTY(int batteryLevel READ batteryLevel NOTIFY batteryLevelChanged)
    Q_PROPERTY(bool anyDeviceFound READ anyDeviceFound NOTIFY anyDeviceFoundChanged)

public:
    explicit HeadsetControlManager(QObject *parent = nullptr);
    ~HeadsetControlManager();

    static HeadsetControlManager* instance();
    static HeadsetControlManager* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;

    Q_INVOKABLE void setMonitoringEnabled(bool enabled);
    Q_INVOKABLE void setLights(bool enabled);
    Q_INVOKABLE void setSidetone(int value);

    QList<HeadsetControlDevice> getCachedDevices() const { return m_cachedDevices; }

    bool hasSidetoneCapability() const { return m_hasSidetoneCapability; }
    bool hasLightsCapability() const { return m_hasLightsCapability; }
    QString deviceName() const { return m_deviceName; }
    QString batteryStatus() const { return m_batteryStatus; }
    int batteryLevel() const { return m_batteryLevel; }
    bool anyDeviceFound() const { return m_anyDeviceFound; }

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

    static HeadsetControlManager* m_instance;
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
