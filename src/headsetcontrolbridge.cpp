#include "headsetcontrolbridge.h"
#include "headsetcontrolmonitor.h"
#include "audiomanager.h"
#include <QTimer>

HeadsetControlBridge* HeadsetControlBridge::m_instance = nullptr;

HeadsetControlBridge::HeadsetControlBridge(QObject *parent)
    : QObject(parent)
{
    m_instance = this;

    // Connect to monitor with a slight delay to ensure AudioWorker is initialized
    QTimer::singleShot(100, this, &HeadsetControlBridge::connectToMonitor);
}

HeadsetControlBridge::~HeadsetControlBridge()
{
    if (m_instance == this) {
        m_instance = nullptr;
    }
}

HeadsetControlBridge* HeadsetControlBridge::instance()
{
    if (!m_instance) {
        m_instance = new HeadsetControlBridge();
    }
    return m_instance;
}

HeadsetControlBridge* HeadsetControlBridge::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);

    if (!m_instance) {
        m_instance = new HeadsetControlBridge();
    }
    return m_instance;
}

void HeadsetControlBridge::connectToMonitor()
{
    HeadsetControlMonitor* monitor = findMonitor();
    if (monitor) {
        connect(monitor, &HeadsetControlMonitor::capabilitiesChanged,
                this, &HeadsetControlBridge::onMonitorCapabilitiesChanged);
        connect(monitor, &HeadsetControlMonitor::deviceNameChanged,
                this, &HeadsetControlBridge::onMonitorDeviceNameChanged);
        connect(monitor, &HeadsetControlMonitor::batteryStatusChanged,
                this, &HeadsetControlBridge::onMonitorBatteryStatusChanged);
        connect(monitor, &HeadsetControlMonitor::batteryLevelChanged,
                this, &HeadsetControlBridge::onMonitorBatteryLevelChanged);
        connect(monitor, &HeadsetControlMonitor::anyDeviceFoundChanged,
                this, &HeadsetControlBridge::onMonitorAnyDeviceFoundChanged);

        // Emit signals to notify QML of current monitor values
        emit capabilitiesChanged();
        emit deviceNameChanged();
        emit batteryStatusChanged();
        emit batteryLevelChanged();
        emit anyDeviceFoundChanged();
    } else {
        // Retry connection after a delay if monitor not found
        QTimer::singleShot(200, this, &HeadsetControlBridge::connectToMonitor);
    }
}

HeadsetControlMonitor* HeadsetControlBridge::findMonitor() const
{
    AudioManager* audioManager = AudioManager::instance();
    if (audioManager) {
        AudioWorker* worker = audioManager->getWorker();
        return worker ? worker->getHeadsetControlMonitor() : nullptr;
    }
    return nullptr;
}

void HeadsetControlBridge::setMonitoringEnabled(bool enabled)
{
    HeadsetControlMonitor* monitor = findMonitor();
    if (monitor) {
        if (enabled) {
            QMetaObject::invokeMethod(monitor, "startMonitoring", Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(monitor, "stopMonitoring", Qt::QueuedConnection);
        }
    }
}

void HeadsetControlBridge::setLights(bool enabled)
{
    HeadsetControlMonitor* monitor = findMonitor();
    if (monitor) {
        QMetaObject::invokeMethod(monitor, "setLights", Qt::QueuedConnection,
                                  Q_ARG(bool, enabled));
    }
}

void HeadsetControlBridge::setSidetone(int value)
{
    HeadsetControlMonitor* monitor = findMonitor();
    if (monitor) {
        QMetaObject::invokeMethod(monitor, "setSidetone", Qt::QueuedConnection,
                                  Q_ARG(int, value));
    }
}

bool HeadsetControlBridge::hasSidetoneCapability() const
{
    HeadsetControlMonitor* monitor = findMonitor();
    return monitor ? monitor->hasSidetoneCapability() : false;
}

bool HeadsetControlBridge::hasLightsCapability() const
{
    HeadsetControlMonitor* monitor = findMonitor();
    return monitor ? monitor->hasLightsCapability() : false;
}

QString HeadsetControlBridge::deviceName() const
{
    HeadsetControlMonitor* monitor = findMonitor();
    return monitor ? monitor->deviceName() : QString();
}

QString HeadsetControlBridge::batteryStatus() const
{
    HeadsetControlMonitor* monitor = findMonitor();
    return monitor ? monitor->batteryStatus() : QString("BATTERY_UNAVAILABLE");
}

int HeadsetControlBridge::batteryLevel() const
{
    HeadsetControlMonitor* monitor = findMonitor();
    return monitor ? monitor->batteryLevel() : -1;
}

bool HeadsetControlBridge::anyDeviceFound() const
{
    HeadsetControlMonitor* monitor = findMonitor();
    return monitor ? monitor->anyDeviceFound() : false;
}

void HeadsetControlBridge::onMonitorCapabilitiesChanged()
{
    emit capabilitiesChanged();
}

void HeadsetControlBridge::onMonitorDeviceNameChanged()
{
    emit deviceNameChanged();
}

void HeadsetControlBridge::onMonitorBatteryStatusChanged()
{
    emit batteryStatusChanged();
}

void HeadsetControlBridge::onMonitorBatteryLevelChanged()
{
    emit batteryLevelChanged();
}

void HeadsetControlBridge::onMonitorAnyDeviceFoundChanged()
{
    emit anyDeviceFoundChanged();
}
