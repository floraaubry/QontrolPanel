#include "monitormanager.h"
#include "monitormanagerimpl.h"
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>
#include <QMutexLocker>
#include <QDebug>

MonitorManager* MonitorManager::m_instance = nullptr;

MonitorManager::MonitorManager(QObject *parent)
    : QObject(parent)
    , m_impl(new MonitorManagerImpl())
    , m_monitorDetected(false)
    , m_currentBrightness(50)
    , m_isUpdating(0)
{
    m_instance = this;

    // Set up callback from impl to Qt - make it async too
    m_impl->setChangeCallback([this]() {
        // Don't call updateMonitorDetection directly - schedule async operation
        QTimer::singleShot(0, this, &MonitorManager::performMonitorDetectionAsync);
    });

    // Initial setup - run asynchronously
    performMonitorDetectionAsync();
}

MonitorManager::~MonitorManager() {
    // Wait for any pending operations to complete
    if (m_currentOperation.isRunning()) {
        m_currentOperation.waitForFinished();
    }

    // Clear the callback to prevent any new operations from starting
    if (m_impl) {
        m_impl->setChangeCallback(nullptr);
    }

    delete m_impl;
    m_impl = nullptr;  // Explicitly set to nullptr
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

bool MonitorManager::isUpdating() const
{
    return m_isUpdating.loadAcquire() > 0;
}

void MonitorManager::setBrightness(int value)
{
    if (value < 0 || value > 100) {
        return;
    }

    // Update the UI immediately for responsiveness
    m_currentBrightness = value;
    emit brightnessChanged();

    // Perform the actual brightness change asynchronously
    performBrightnessUpdateAsync(value);
}

void MonitorManager::refreshMonitors()
{
    performMonitorDetectionAsync();
}

void MonitorManager::performMonitorDetectionAsync()
{
    // Cancel any pending operation
    if (m_currentOperation.isRunning()) {
        m_currentOperation.cancel();
    }

    setIsUpdating(true);

    m_currentOperation = QtConcurrent::run([this]() {
        QMutexLocker locker(&m_mutex);

        // Check if we're still valid and m_impl exists
        if (!m_impl) {
            QMetaObject::invokeMethod(this, "onMonitorDetectionComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(int, -1));
            return;
        }

        try {
            // Re-enumerate monitors
            m_impl->enumerateMonitors();

            // Check for working monitors
            bool hasWorkingMonitor = false;
            int lowestBrightness = 100;
            bool foundBrightness = false;

            int monitorCount = m_impl->getMonitorCount();
            for (int i = 0; i < monitorCount; i++) {
                // Double-check m_impl is still valid before each operation
                if (!m_impl) break;

                // Test DDC/CI capability - this might block but it's now in background
                if (m_impl->testDDCCI(i)) {
                    hasWorkingMonitor = true;

                    // Check again before getting brightness
                    if (!m_impl) break;

                    // Get current brightness - this might also block
                    int brightness = m_impl->getBrightnessInternal(i);
                    if (brightness != -1) {
                        lowestBrightness = std::min(lowestBrightness, brightness);
                        foundBrightness = true;
                    }
                }
            }

            int currentBrightness = foundBrightness ? lowestBrightness : -1;

            // Schedule UI update on main thread
            QMetaObject::invokeMethod(this, "onMonitorDetectionComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, hasWorkingMonitor),
                                      Q_ARG(int, currentBrightness));
        } catch (...) {
            // Handle any exceptions and update UI
            QMetaObject::invokeMethod(this, "onMonitorDetectionComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(int, -1));
        }
    });
}

void MonitorManager::performBrightnessUpdateAsync(int value)
{
    // Cancel any pending operation
    if (m_currentOperation.isRunning()) {
        m_currentOperation.cancel();
    }

    setIsUpdating(true);

    m_currentOperation = QtConcurrent::run([this, value]() {
        QMutexLocker locker(&m_mutex);

        // Check if we're still valid and m_impl exists
        if (!m_impl) {
            QMetaObject::invokeMethod(this, "onBrightnessUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(int, value));
            return;
        }

        try {
            // First, re-enumerate monitors to ensure all are present
            m_impl->enumerateMonitors();

            // Check if m_impl is still valid after enumeration
            if (!m_impl) {
                QMetaObject::invokeMethod(this, "onBrightnessUpdateComplete",
                                          Qt::QueuedConnection,
                                          Q_ARG(bool, false),
                                          Q_ARG(int, value));
                return;
            }

            // Then set brightness on all monitors
            m_impl->setBrightnessAll(value);

            // Check for working monitors and get actual brightness
            bool hasWorkingMonitor = false;
            int lowestBrightness = 100;
            bool foundBrightness = false;

            int monitorCount = m_impl->getMonitorCount();
            for (int i = 0; i < monitorCount; i++) {
                // Double-check m_impl is still valid before each operation
                if (!m_impl) break;

                if (m_impl->testDDCCI(i)) {
                    hasWorkingMonitor = true;

                    // Check again before getting brightness
                    if (!m_impl) break;

                    // Get actual brightness after setting
                    int brightness = m_impl->getBrightnessInternal(i);
                    if (brightness != -1) {
                        lowestBrightness = std::min(lowestBrightness, brightness);
                        foundBrightness = true;
                    }
                }
            }

            int actualBrightness = foundBrightness ? lowestBrightness : value;

            // Schedule UI update on main thread
            QMetaObject::invokeMethod(this, "onBrightnessUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, hasWorkingMonitor),
                                      Q_ARG(int, actualBrightness));
        } catch (...) {
            // Handle any exceptions and update UI
            QMetaObject::invokeMethod(this, "onBrightnessUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(int, value));
        }
    });
}

void MonitorManager::onMonitorDetectionComplete(bool hasMonitors, int currentBrightness)
{
    // Update monitor detection state
    if (m_monitorDetected != hasMonitors) {
        m_monitorDetected = hasMonitors;
        emit monitorDetectedChanged();
    }

    // Update brightness if we got a valid reading
    if (currentBrightness != -1 && m_currentBrightness != currentBrightness) {
        m_currentBrightness = currentBrightness;
        emit brightnessChanged();
    }

    setIsUpdating(false);
}

void MonitorManager::onBrightnessUpdateComplete(bool hasMonitors, int actualBrightness)
{
    // Update monitor detection state in case monitors changed
    if (m_monitorDetected != hasMonitors) {
        m_monitorDetected = hasMonitors;
        emit monitorDetectedChanged();
    }

    // Update brightness with the actual value from hardware
    if (m_currentBrightness != actualBrightness) {
        m_currentBrightness = actualBrightness;
        emit brightnessChanged();
    }

    setIsUpdating(false);
}

void MonitorManager::setIsUpdating(bool updating)
{
    bool wasUpdating = m_isUpdating.loadAcquire() > 0;

    if (updating) {
        m_isUpdating.fetchAndAddAcquire(1);
    } else {
        m_isUpdating.fetchAndSubAcquire(1);
    }

    bool isUpdatingNow = m_isUpdating.loadAcquire() > 0;

    if (wasUpdating != isUpdatingNow) {
        emit isUpdatingChanged();
    }
}
