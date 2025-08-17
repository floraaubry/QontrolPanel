#pragma once
#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>

class HeadsetControlMonitor;

class HeadsetControlBridge : public QObject
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
    explicit HeadsetControlBridge(QObject *parent = nullptr);
    ~HeadsetControlBridge();

    static HeadsetControlBridge* instance();
    static HeadsetControlBridge* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    Q_INVOKABLE void setMonitoringEnabled(bool enabled);
    Q_INVOKABLE void setLights(bool enabled);
    Q_INVOKABLE void setSidetone(int value);

    bool hasSidetoneCapability() const;
    bool hasLightsCapability() const;
    QString deviceName() const;
    QString batteryStatus() const;
    int batteryLevel() const;
    bool anyDeviceFound() const;

signals:
    void capabilitiesChanged();
    void deviceNameChanged();
    void batteryStatusChanged();
    void batteryLevelChanged();
    void anyDeviceFoundChanged();

private slots:
    void onMonitorCapabilitiesChanged();
    void onMonitorDeviceNameChanged();
    void onMonitorBatteryStatusChanged();
    void onMonitorBatteryLevelChanged();
    void onMonitorAnyDeviceFoundChanged();

private:
    static HeadsetControlBridge* m_instance;
    HeadsetControlMonitor* findMonitor() const;
    void connectToMonitor();
};
