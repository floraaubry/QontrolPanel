#include "monitormanager.h"
#include "monitormanagerimpl.h"
#include <QSettings>
#include <QTimer>

MonitorManager* MonitorManager::m_instance = nullptr;

MonitorManager::MonitorManager(QObject *parent)
    : QObject(parent)
    , m_impl(new MonitorManagerImpl())
    , m_monitorDetected(false)
    , m_currentBrightness(50)
{
    m_instance = this;

    // Set up callback from impl to Qt
    m_impl->setChangeCallback([this]() {
        updateMonitorDetection();
        retrieveCurrentBrightness();
    });

    // Initial setup
    updateMonitorDetection();
    retrieveCurrentBrightness();
}

MonitorManager::~MonitorManager() {
    delete m_impl;
    m_instance = nullptr;
}

MonitorManager* MonitorManager::instance()
{
    return m_instance;
}

MonitorManager* MonitorManager::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    if (!m_instance) {
        m_instance = new MonitorManager();
    }
    return m_instance;
}

bool MonitorManager::monitorDetected() const
{
    return m_monitorDetected;
}

int MonitorManager::brightness() const
{
    return m_currentBrightness;
}

void MonitorManager::setBrightness(int value)
{
    if (value < 0 || value > 100) {
        return;
    }

    if (!m_monitorDetected) {
        return;
    }

    // Update the UI immediately for responsiveness
    m_currentBrightness = value;
    emit brightnessChanged();

    // Perform the actual brightness change asynchronously
    QTimer::singleShot(0, [this, value]() {
        m_impl->setBrightnessAll(value);
    });
}

void MonitorManager::refreshMonitors()
{
    m_impl->enumerateMonitors();
    updateMonitorDetection();
    retrieveCurrentBrightness();
}

void MonitorManager::updateMonitorDetection()
{
    bool previousState = m_monitorDetected;
    bool hasWorkingMonitor = false;

    int monitorCount = m_impl->getMonitorCount();
    for (int i = 0; i < monitorCount; i++) {
        if (m_impl->testDDCCI(i)) {
            hasWorkingMonitor = true;
            break;
        }
    }

    m_monitorDetected = hasWorkingMonitor;

    if (m_monitorDetected != previousState) {
        emit monitorDetectedChanged();
    }
}

void MonitorManager::retrieveCurrentBrightness()
{
    if (!m_monitorDetected) {
        return;
    }

    int lowestBrightness = 100;
    bool foundAny = false;

    int monitorCount = m_impl->getMonitorCount();
    for (int i = 0; i < monitorCount; i++) {
        if (m_impl->testDDCCI(i)) {
            int brightness = m_impl->getBrightnessInternal(i);
            if (brightness != -1) {
                lowestBrightness = std::min(lowestBrightness, brightness);
                foundAny = true;
            }
        }
    }

    if (foundAny && lowestBrightness != m_currentBrightness) {
        m_currentBrightness = lowestBrightness;
        emit brightnessChanged();
    }
}
