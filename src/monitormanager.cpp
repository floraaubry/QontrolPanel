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
    , m_nightLightEnabled(false)
    , m_nightLightSupported(false)
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

    // Check Night Light support and status
    checkNightLightAsync();
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
    m_impl = nullptr;
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

bool MonitorManager::nightLightEnabled() const
{
    return m_nightLightEnabled;
}

bool MonitorManager::nightLightSupported() const
{
    return m_nightLightSupported;
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

void MonitorManager::setNightLightEnabled(bool enabled)
{
    if (!m_nightLightSupported) {
        qDebug() << "Night Light not supported on this system";
        return;
    }

    // Update the UI immediately for responsiveness
    m_nightLightEnabled = enabled;
    emit nightLightEnabledChanged();

    // Perform the actual Night Light change asynchronously
    performNightLightUpdateAsync(enabled);
}

void MonitorManager::toggleNightLight()
{
    setNightLightEnabled(!m_nightLightEnabled);
}

void MonitorManager::refreshMonitors()
{
    performMonitorDetectionAsync();
    checkNightLightAsync();
}

void MonitorManager::checkNightLightAsync()
{
    setIsUpdating(true);

    QtConcurrent::run([this]() {
        QMutexLocker locker(&m_mutex);

        if (!m_impl) {
            QMetaObject::invokeMethod(this, "onNightLightUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(bool, false));
            return;
        }

        try {
            bool supported = m_impl->isNightLightSupported();
            bool enabled = supported ? m_impl->isNightLightEnabled() : false;

            QMetaObject::invokeMethod(this, "onNightLightUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, supported),
                                      Q_ARG(bool, enabled));
        } catch (...) {
            QMetaObject::invokeMethod(this, "onNightLightUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(bool, false));
        }
    });
}

void MonitorManager::performNightLightUpdateAsync(bool enabled)
{
    if (!m_nightLightSupported) {
        return;
    }

    setIsUpdating(true);

    QtConcurrent::run([this, enabled]() {
        QMutexLocker locker(&m_mutex);

        if (!m_impl) {
            QMetaObject::invokeMethod(this, "onNightLightUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(bool, false));
            return;
        }

        try {
            if (enabled) {
                m_impl->enableNightLight();
            } else {
                m_impl->disableNightLight();
            }

            // Get the actual state after the change
            bool actualEnabled = m_impl->isNightLightEnabled();

            QMetaObject::invokeMethod(this, "onNightLightUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, true),
                                      Q_ARG(bool, actualEnabled));
        } catch (...) {
            QMetaObject::invokeMethod(this, "onNightLightUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, m_nightLightSupported),
                                      Q_ARG(bool, m_nightLightEnabled));
        }
    });
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

        if (!m_impl) {
            QMetaObject::invokeMethod(this, "onMonitorDetectionComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(int, -1));
            return;
        }

        try {
            m_impl->enumerateMonitors();

            bool hasWorkingMonitor = false;
            int lowestBrightness = 100;
            bool foundBrightness = false;

            int monitorCount = m_impl->getMonitorCount();
            for (int i = 0; i < monitorCount; i++) {
                if (!m_impl) break;

                if (m_impl->testDDCCI(i)) {
                    hasWorkingMonitor = true;

                    if (!m_impl) break;

                    int brightness = m_impl->getBrightnessInternal(i);
                    if (brightness != -1) {
                        lowestBrightness = std::min(lowestBrightness, brightness);
                        foundBrightness = true;
                    }
                }
            }

            int currentBrightness = foundBrightness ? lowestBrightness : -1;

            QMetaObject::invokeMethod(this, "onMonitorDetectionComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, hasWorkingMonitor),
                                      Q_ARG(int, currentBrightness));
        } catch (...) {
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

        if (!m_impl) {
            QMetaObject::invokeMethod(this, "onBrightnessUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(int, value));
            return;
        }

        try {
            bool hasWorkingMonitor = false;
            int lowestBrightness = value;

            int monitorCount = m_impl->getMonitorCount();
            for (int i = 0; i < monitorCount; i++) {
                if (!m_impl) break;

                if (m_impl->setBrightnessInternal(i, value)) {
                    hasWorkingMonitor = true;
                    int actualBrightness = m_impl->getBrightnessInternal(i);
                    if (actualBrightness != -1) {
                        lowestBrightness = std::min(lowestBrightness, actualBrightness);
                    }
                }
            }

            QMetaObject::invokeMethod(this, "onBrightnessUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, hasWorkingMonitor),
                                      Q_ARG(int, lowestBrightness));
        } catch (...) {
            QMetaObject::invokeMethod(this, "onBrightnessUpdateComplete",
                                      Qt::QueuedConnection,
                                      Q_ARG(bool, false),
                                      Q_ARG(int, value));
        }
    });
}

void MonitorManager::onMonitorDetectionComplete(bool hasMonitors, int currentBrightness)
{
    if (m_monitorDetected != hasMonitors) {
        m_monitorDetected = hasMonitors;
        emit monitorDetectedChanged();
    }

    if (currentBrightness != -1 && m_currentBrightness != currentBrightness) {
        m_currentBrightness = currentBrightness;
        emit brightnessChanged();
    }

    setIsUpdating(false);
}

void MonitorManager::onBrightnessUpdateComplete(bool hasMonitors, int actualBrightness)
{
    if (m_monitorDetected != hasMonitors) {
        m_monitorDetected = hasMonitors;
        emit monitorDetectedChanged();
    }

    if (m_currentBrightness != actualBrightness) {
        m_currentBrightness = actualBrightness;
        emit brightnessChanged();
    }

    setIsUpdating(false);
}

void MonitorManager::onNightLightUpdateComplete(bool supported, bool enabled)
{
    if (m_nightLightSupported != supported) {
        m_nightLightSupported = supported;
        emit nightLightSupportedChanged();
    }

    if (m_nightLightEnabled != enabled) {
        m_nightLightEnabled = enabled;
        emit nightLightEnabledChanged();
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
