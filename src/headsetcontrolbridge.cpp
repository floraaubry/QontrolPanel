#include "headsetcontrolbridge.h"
#include <QDebug>

HeadsetControlBridge* HeadsetControlBridge::m_instance = nullptr;

HeadsetControlBridge::HeadsetControlBridge(QObject *parent)
    : QObject(parent)
    , m_manager(HeadsetControlManager::instance())
{
    m_instance = this;

    connect(m_manager, &HeadsetControlManager::monitoringStateChanged,
            this, &HeadsetControlBridge::onMonitoringStateChanged);
}

HeadsetControlBridge::~HeadsetControlBridge()
{
    if (m_instance == this) {
        m_instance = nullptr;
    }
}

HeadsetControlBridge* HeadsetControlBridge::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    if (!m_instance) {
        m_instance = new HeadsetControlBridge();
    }
    return m_instance;
}

HeadsetControlBridge* HeadsetControlBridge::instance()
{
    return m_instance;
}

bool HeadsetControlBridge::isMonitoring() const
{
    return m_manager ? m_manager->isMonitoring() : false;
}

void HeadsetControlBridge::setMonitoringEnabled(bool enabled)
{
    if (m_manager) {
        m_manager->setMonitoringEnabled(enabled);
    }
}

void HeadsetControlBridge::onMonitoringStateChanged(bool enabled)
{
    Q_UNUSED(enabled)
    emit monitoringStateChanged();
}
