#include "monitormanager.h"
#include "monitormanagerimpl.h"
#include <QApplication>
#include <QDebug>
#include <QTimer>
#include "logmanager.h"
// Static member definitions
MonitorWorker* MonitorManager::s_worker = nullptr;
QThread* MonitorManager::s_workerThread = nullptr;
MonitorManager* MonitorManager::s_instance = nullptr;
QList<Monitor> MonitorManager::s_cachedMonitors;
bool MonitorManager::s_nightLightEnabled = false;
bool MonitorManager::s_nightLightSupported = false;
QMutex MonitorManager::s_cacheMutex;

// MonitorWorker implementation
MonitorWorker::MonitorWorker(QObject *parent)
    : QObject(parent)
    , m_impl(new MonitorManagerImpl())
    , m_ddcciBrightnessTimer(new QTimer(this))
    , m_pendingDDCCIBrightness(0)
    , m_hasPendingDDCCIBrightness(false)
{
    // Set up callback from impl to worker thread
    m_impl->setChangeCallback([this]() {
        // This callback runs in worker thread, so we can call enumerateMonitors directly
        enumerateMonitors();
    });

    // Initial enumeration
    enumerateMonitors();
    checkNightLightStatus();

    m_ddcciBrightnessTimer->setSingleShot(true);
    connect(m_ddcciBrightnessTimer, &QTimer::timeout, this, [this]() {
        // If we have a pending brightness change, execute it
        if (m_hasPendingDDCCIBrightness) {
            int brightness = m_pendingDDCCIBrightness;
            m_hasPendingDDCCIBrightness = false;

            // Execute the pending brightness change with same delay
            setDDCCIBrightness(brightness, 16);
        }
    });
}

MonitorWorker::~MonitorWorker()
{
    if (m_ddcciBrightnessTimer && m_ddcciBrightnessTimer->isActive()) {
        m_ddcciBrightnessTimer->stop();
    }

    delete m_impl;
}

void MonitorWorker::cleanup()
{
    // Stop and cleanup the timer from the correct thread
    if (m_ddcciBrightnessTimer) {
        m_ddcciBrightnessTimer->stop();
        m_ddcciBrightnessTimer->deleteLater();
        m_ddcciBrightnessTimer = nullptr;
    }

    m_hasPendingDDCCIBrightness = false;
}

void MonitorWorker::enumerateMonitors()
{
    QMutexLocker locker(&m_monitorsMutex);

    if (!m_impl) {
        return;
    }

    // Update the implementation's monitor list
    m_impl->enumerateMonitors();

    // Convert to Qt types
    updateMonitorFromImpl();

    emit monitorsReady(m_monitors);
}

void MonitorWorker::updateMonitorFromImpl()
{
    m_monitors.clear();

    if (!m_impl) {
        return;
    }

    int count = m_impl->getMonitorCount();
    for (int i = 0; i < count; i++) {
        Monitor monitor;
        monitor.id = QString::number(i);
        monitor.name = QString::fromStdWString(m_impl->getMonitorName(i));
        monitor.friendlyName = monitor.name;
        monitor.brightness = m_impl->getCachedBrightness(i);
        monitor.isSupported = m_impl->testDDCCI(i);
        monitor.isLaptopDisplay = (monitor.name.contains("Laptop", Qt::CaseInsensitive) ||
                                   monitor.name.contains("Internal", Qt::CaseInsensitive));

        int currentBrightness = m_impl->getBrightnessInternal(i);
        if (currentBrightness != -1) {
            monitor.brightness = currentBrightness;
        } else {
            monitor.brightness = 50; // Default only if we can't read it
            monitor.isSupported = false;
        }

        monitor.minBrightness = 0;
        monitor.maxBrightness = 100;

        m_monitors.append(monitor);
    }
}

void MonitorWorker::setBrightness(const QString& monitorId, int brightness)
{
    QMutexLocker locker(&m_monitorsMutex);

    if (!m_impl) {
        return;
    }

    bool ok;
    int index = monitorId.toInt(&ok);
    if (!ok || index < 0) {
        return;
    }

    if (m_impl->setBrightnessInternal(index, brightness)) {
        // Update cached value
        for (Monitor& monitor : m_monitors) {
            if (monitor.id == monitorId) {
                monitor.brightness = brightness;
                break;
            }
        }
        emit brightnessChanged(monitorId, brightness);
    }
}

void MonitorWorker::setBrightnessAll(int brightness)
{
    QMutexLocker locker(&m_monitorsMutex);

    if (!m_impl) {
        return;
    }

    bool success = m_impl->setBrightnessAll(brightness);
    if (success) {
        // Update all cached values
        for (Monitor& monitor : m_monitors) {
            if (monitor.isSupported) {
                monitor.brightness = brightness;
                emit brightnessChanged(monitor.id, brightness);
            }
        }
    }
}

void MonitorWorker::refreshBrightnessLevels()
{
    QMutexLocker locker(&m_monitorsMutex);

    if (!m_impl) {
        return;
    }

    QList<QPair<QString, int>> levels;

    for (Monitor& monitor : m_monitors) {
        if (monitor.isSupported) {
            bool ok;
            int index = monitor.id.toInt(&ok);
            if (ok && index >= 0) {
                int brightness = m_impl->getBrightnessInternal(index);
                if (brightness != -1) {
                    monitor.brightness = brightness;
                    levels.append(qMakePair(monitor.id, brightness));
                }
            }
        }
    }

    emit monitorBrightnessLevelsReady(levels);
}

void MonitorWorker::checkNightLightStatus()
{
    if (!m_impl) {
        emit nightLightStatusReady(false, false);
        return;
    }

    bool supported = m_impl->isNightLightSupported();
    bool enabled = supported ? m_impl->isNightLightEnabled() : false;

    emit nightLightStatusReady(supported, enabled);
}

void MonitorWorker::setNightLight(bool enabled)
{
    if (!m_impl) {
        return;
    }

    if (!m_impl->isNightLightSupported()) {
        return;
    }

    if (enabled) {
        m_impl->enableNightLight();
    } else {
        m_impl->disableNightLight();
    }

    // Check actual state after change
    bool actualEnabled = m_impl->isNightLightEnabled();
    emit nightLightChanged(actualEnabled);
}

void MonitorWorker::toggleNightLight()
{
    if (!m_impl) {
        return;
    }

    if (!m_impl->isNightLightSupported()) {
        return;
    }

    m_impl->toggleNightLight();

    // Check actual state after toggle
    bool enabled = m_impl->isNightLightEnabled();
    emit nightLightChanged(enabled);
}

void MonitorWorker::setDDCCIBrightness(int brightness, int delayMs)
{
    // If timer is already running, buffer this request
    if (m_ddcciBrightnessTimer->isActive()) {
        // If we already have a pending request, just update it (don't queue multiple)
        if (m_hasPendingDDCCIBrightness) {
            m_pendingDDCCIBrightness = brightness;
            return; // Just update the pending value and return
        }

        // Buffer this request
        m_pendingDDCCIBrightness = brightness;
        m_hasPendingDDCCIBrightness = true;
        return;
    }

    // Execute the brightness change immediately
    QMutexLocker locker(&m_monitorsMutex);

    if (!m_impl) {
        return;
    }

    LogManager::instance()->sendLog(LogManager::MonitorManager,
                                    QString("Setting DDC/CI brightness to %1% for all external monitors with delay %2ms").arg(brightness).arg(delayMs));

    bool anySuccess = false;

    // Set brightness for all DDC/CI-supported monitors (non-laptop displays)
    for (int i = 0; i < m_impl->getMonitorCount(); i++) {
        if (!m_monitors[i].isLaptopDisplay && m_impl->testDDCCI(i)) {
            if (m_impl->setBrightnessInternal(i, brightness)) {
                // Update cached value in Qt monitor list
                if (i < m_monitors.size()) {
                    m_monitors[i].brightness = brightness;
                }
                anySuccess = true;
                LogManager::instance()->sendLog(LogManager::MonitorManager,
                                                QString("Set DDC/CI brightness for monitor %1").arg(i));
            } else {
                LogManager::instance()->sendWarn(LogManager::MonitorManager,
                                                 QString("Failed to set DDC/CI brightness for monitor %1").arg(i));
            }
        }
    }

    if (anySuccess) {
        emit ddcciBrightnessChanged(brightness);
        LogManager::instance()->sendLog(LogManager::MonitorManager,
                                        "DDC/CI brightness change completed successfully");
    } else {
        LogManager::instance()->sendWarn(LogManager::MonitorManager,
                                         "No DDC/CI monitors responded to brightness change");
    }

    // Start the timer to block subsequent calls
    m_ddcciBrightnessTimer->start(delayMs);
}

void MonitorWorker::setWMIBrightness(int brightness)
{
    QMutexLocker locker(&m_monitorsMutex);

    if (!m_impl) {
        return;
    }

    LogManager::instance()->sendLog(LogManager::MonitorManager,
                                    QString("Setting WMI brightness to %1% for all laptop displays").arg(brightness));

    bool anySuccess = false;

    // Set brightness for all WMI-controlled monitors (laptop displays)
    for (int i = 0; i < m_impl->getMonitorCount(); i++) {
        if (m_monitors[i].isLaptopDisplay) {
            if (m_impl->setBrightnessInternal(i, brightness)) {
                // Update cached value in Qt monitor list
                if (i < m_monitors.size()) {
                    m_monitors[i].brightness = brightness;
                }
                anySuccess = true;
                LogManager::instance()->sendLog(LogManager::MonitorManager,
                                                QString("Set WMI brightness for monitor %1").arg(i));
            } else {
                LogManager::instance()->sendWarn(LogManager::MonitorManager,
                                                 QString("Failed to set WMI brightness for monitor %1").arg(i));
            }
        }
    }

    if (anySuccess) {
        emit wmiBrightnessChanged(brightness);
        LogManager::instance()->sendLog(LogManager::MonitorManager,
                                        "WMI brightness change completed successfully");
    } else {
        LogManager::instance()->sendWarn(LogManager::MonitorManager,
                                         "No WMI monitors responded to brightness change");
    }
}

// MonitorManager implementation
MonitorManager::MonitorManager(QObject *parent)
    : QObject(parent)
    , m_monitorDetected(false)
    , m_currentBrightness(50)
    , m_nightLightEnabled(false)
    , m_nightLightSupported(false)
{
    s_instance = this;
    initialize();
}

MonitorManager::~MonitorManager()
{
    LogManager::instance()->sendLog(LogManager::MonitorManager, "MonitorManager destructor called");
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void MonitorManager::initialize()
{
    LogManager::instance()->sendLog(LogManager::MonitorManager, "MonitorManager initialization started");

    if (s_worker) {
        LogManager::instance()->sendLog(LogManager::MonitorManager, "MonitorManager already initialized, skipping");
        return; // Already initialized
    }

    LogManager::instance()->sendLog(LogManager::MonitorManager, "Creating worker thread");
    s_workerThread = new QThread();
    s_worker = new MonitorWorker();
    s_worker->moveToThread(s_workerThread);

    // Connect worker signals to cache updates
    QObject::connect(s_worker, &MonitorWorker::monitorsReady,
                     [](const QList<Monitor>& monitors) {
                         updateCache(monitors);
                     });

    QObject::connect(s_worker, &MonitorWorker::brightnessChanged,
                     [](const QString& monitorId, int brightness) {
                         QMutexLocker locker(&s_cacheMutex);
                         for (Monitor& monitor : s_cachedMonitors) {
                             if (monitor.id == monitorId) {
                                 monitor.brightness = brightness;
                                 break;
                             }
                         }
                     });

    QObject::connect(s_worker, &MonitorWorker::nightLightStatusReady,
                     [](bool supported, bool enabled) {
                         updateNightLightCache(supported, enabled);
                     });

    QObject::connect(s_worker, &MonitorWorker::nightLightChanged,
                     [](bool enabled) {
                         updateNightLightCache(s_nightLightSupported, enabled);
                     });

    // New DDC/CI and WMI brightness signals
    QObject::connect(s_worker, &MonitorWorker::ddcciBrightnessChanged,
                     [](int brightness) {
                         QMutexLocker locker(&s_cacheMutex);
                         // Update all DDC/CI monitors in cache
                         for (Monitor& monitor : s_cachedMonitors) {
                             if (!monitor.isLaptopDisplay && monitor.isSupported) {
                                 monitor.brightness = brightness;
                             }
                         }
                     });

    QObject::connect(s_worker, &MonitorWorker::wmiBrightnessChanged,
                     [](int brightness) {
                         QMutexLocker locker(&s_cacheMutex);
                         // Update all WMI monitors in cache
                         for (Monitor& monitor : s_cachedMonitors) {
                             if (monitor.isLaptopDisplay) {
                                 monitor.brightness = brightness;
                             }
                         }
                     });

    // Connect to instance if it exists
    if (s_instance) {
        QObject::connect(s_worker, &MonitorWorker::monitorsReady,
                         s_instance, [](const QList<Monitor>& monitors) {
                             if (s_instance) {
                                 bool hasMonitors = !monitors.isEmpty() &&
                                                    std::any_of(monitors.begin(), monitors.end(),
                                                                [](const Monitor& m) { return m.isSupported; });
                                 if (s_instance->m_monitorDetected != hasMonitors) {
                                     s_instance->m_monitorDetected = hasMonitors;
                                     emit s_instance->monitorDetectedChanged();
                                 }

                                 // Update brightness from first supported monitor
                                 int newBrightness = 50;
                                 for (const Monitor& monitor : monitors) {
                                     if (monitor.isSupported) {
                                         newBrightness = monitor.brightness;
                                         break;
                                     }
                                 }

                                 if (s_instance->m_currentBrightness != newBrightness) {
                                     s_instance->m_currentBrightness = newBrightness;
                                     emit s_instance->brightnessChanged();
                                 }

                                 emit s_instance->monitorsChanged(monitors);
                             }
                         });

        QObject::connect(s_worker, &MonitorWorker::nightLightStatusReady,
                         s_instance, [](bool supported, bool enabled) {
                             if (s_instance) {
                                 if (s_instance->m_nightLightSupported != supported) {
                                     s_instance->m_nightLightSupported = supported;
                                     emit s_instance->nightLightSupportedChanged();
                                 }
                                 if (s_instance->m_nightLightEnabled != enabled) {
                                     s_instance->m_nightLightEnabled = enabled;
                                     emit s_instance->nightLightEnabledChanged();
                                 }
                             }
                         });

        QObject::connect(s_worker, &MonitorWorker::nightLightChanged,
                         s_instance, [](bool enabled) {
                             if (s_instance && s_instance->m_nightLightEnabled != enabled) {
                                 s_instance->m_nightLightEnabled = enabled;
                                 emit s_instance->nightLightEnabledChanged();
                             }
                         });

        // Connect new DDC/CI and WMI brightness signals to instance
        QObject::connect(s_worker, &MonitorWorker::ddcciBrightnessChanged,
                         s_instance, [](int brightness) {
                             if (s_instance && s_instance->m_currentBrightness != brightness) {
                                 s_instance->m_currentBrightness = brightness;
                                 emit s_instance->brightnessChanged();
                             }
                         });

        QObject::connect(s_worker, &MonitorWorker::wmiBrightnessChanged,
                         s_instance, [](int brightness) {
                             if (s_instance && s_instance->m_currentBrightness != brightness) {
                                 s_instance->m_currentBrightness = brightness;
                                 emit s_instance->brightnessChanged();
                             }
                         });
    }

    LogManager::instance()->sendLog(LogManager::MonitorManager, "Starting worker thread");
    s_workerThread->start();

    QMetaObject::invokeMethod(s_worker, "enumerateMonitors", Qt::QueuedConnection);
    QMetaObject::invokeMethod(s_worker, "refreshBrightnessLevels", Qt::QueuedConnection);

    LogManager::instance()->sendLog(LogManager::MonitorManager, "MonitorManager initialization complete");
}

void MonitorManager::cleanup()
{
    if (!s_workerThread) return;

    // Step 1: Tell worker to cleanup from its own thread
    if (s_worker) {
        QMetaObject::invokeMethod(s_worker, "cleanup", Qt::QueuedConnection);

        // Step 2: Tell worker to delete itself from its own thread
        QMetaObject::invokeMethod(s_worker, "deleteLater", Qt::QueuedConnection);
        s_worker = nullptr;
    }

    // Step 3: Quit thread and wait
    s_workerThread->quit();
    if (!s_workerThread->wait(5000)) {
        qWarning() << "MonitorWorker thread did not finish gracefully, terminating...";
        s_workerThread->terminate();
        s_workerThread->wait(1000);
    }

    // Step 4: Delete thread
    delete s_workerThread;
    s_workerThread = nullptr;
}

MonitorWorker* MonitorManager::getWorker()
{
    return s_worker;
}

MonitorManager* MonitorManager::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    if (!s_instance) {
        s_instance = new MonitorManager();
    }
    return s_instance;
}

MonitorManager* MonitorManager::instance()
{
    return s_instance;
}

// Async methods
void MonitorManager::enumerateMonitorsAsync()
{
    if (s_worker) {
        QMetaObject::invokeMethod(s_worker, "enumerateMonitors", Qt::QueuedConnection);
    }
}

void MonitorManager::setBrightnessAsync(const QString& monitorId, int brightness)
{
    if (s_worker) {
        QMetaObject::invokeMethod(s_worker, "setBrightness", Qt::QueuedConnection,
                                  Q_ARG(QString, monitorId), Q_ARG(int, brightness));
    }
}

void MonitorManager::setBrightnessAllAsync(int brightness)
{
    if (s_worker) {
        QMetaObject::invokeMethod(s_worker, "setBrightnessAll", Qt::QueuedConnection,
                                  Q_ARG(int, brightness));
    }
}

void MonitorManager::refreshBrightnessLevelsAsync()
{
    if (s_worker) {
        QMetaObject::invokeMethod(s_worker, "refreshBrightnessLevels", Qt::QueuedConnection);
    }
}

void MonitorManager::checkNightLightAsync()
{
    if (s_worker) {
        QMetaObject::invokeMethod(s_worker, "checkNightLightStatus", Qt::QueuedConnection);
    }
}

void MonitorManager::setNightLightAsync(bool enabled)
{
    if (s_worker) {
        QMetaObject::invokeMethod(s_worker, "setNightLight", Qt::QueuedConnection,
                                  Q_ARG(bool, enabled));
    }
}

void MonitorManager::toggleNightLightAsync()
{
    if (s_worker) {
        QMetaObject::invokeMethod(s_worker, "toggleNightLight", Qt::QueuedConnection);
    }
}

// Cached getters
QList<Monitor> MonitorManager::getMonitors()
{
    QMutexLocker locker(&s_cacheMutex);
    return s_cachedMonitors;
}

int MonitorManager::getMonitorBrightness(const QString& monitorId)
{
    QMutexLocker locker(&s_cacheMutex);

    for (const Monitor& monitor : std::as_const(s_cachedMonitors)) {
        if (monitor.id == monitorId) {
            return monitor.brightness;
        }
    }
    return 50;
}

bool MonitorManager::getNightLightEnabled()
{
    QMutexLocker locker(&s_cacheMutex);
    return s_nightLightEnabled;
}

bool MonitorManager::getNightLightSupported()
{
    QMutexLocker locker(&s_cacheMutex);
    return s_nightLightSupported;
}

// QML Properties
bool MonitorManager::monitorDetected() const
{
    return m_monitorDetected;
}

int MonitorManager::brightness() const
{
    return m_currentBrightness;
}

bool MonitorManager::nightLightEnabled() const
{
    return m_nightLightEnabled;
}

bool MonitorManager::nightLightSupported() const
{
    return m_nightLightSupported;
}

// QML Methods
void MonitorManager::setBrightness(int value)
{
    setBrightnessAllAsync(value);
}

void MonitorManager::refreshMonitors()
{
    enumerateMonitorsAsync();
    checkNightLightAsync();
}

void MonitorManager::setNightLightEnabled(bool enabled)
{
    setNightLightAsync(enabled);
}

void MonitorManager::toggleNightLight()
{
    toggleNightLightAsync();
}

// Private methods
void MonitorManager::updateCache(const QList<Monitor>& monitors)
{
    QMutexLocker locker(&s_cacheMutex);
    s_cachedMonitors = monitors;
}

void MonitorManager::updateNightLightCache(bool supported, bool enabled)
{
    QMutexLocker locker(&s_cacheMutex);
    s_nightLightSupported = supported;
    s_nightLightEnabled = enabled;
}

void MonitorManager::setDDCCIBrightnessAsync(int brightness, int delayMs)
{
    if (s_worker) {
        QMetaObject::invokeMethod(s_worker, "setDDCCIBrightness", Qt::QueuedConnection,
                                  Q_ARG(int, brightness), Q_ARG(int, delayMs));
    }
}

void MonitorManager::setWMIBrightnessAsync(int brightness)
{
    if (s_worker) {
        QMetaObject::invokeMethod(s_worker, "setWMIBrightness", Qt::QueuedConnection,
                                  Q_ARG(int, brightness));
    }
}

void MonitorManager::setDDCCIBrightness(int brightness, int delayMs)
{
    setDDCCIBrightnessAsync(brightness, delayMs);
}

void MonitorManager::setWMIBrightness(int brightness)
{
    setWMIBrightnessAsync(brightness);
}
