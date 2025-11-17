#include "headsetcontrolmonitor.h"
#include "logmanager.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>

HeadsetControlMonitor::HeadsetControlMonitor(QObject *parent)
    : QObject(parent)
    , m_fetchTimer(new QTimer(this))
    , m_process(nullptr)
    , m_settings("Odizinne", "QontrolPanel")
    , m_isMonitoring(false)
    , m_hasSidetoneCapability(false)
    , m_hasLightsCapability(false)
    , m_deviceName("")
    , m_batteryStatus("BATTERY_UNAVAILABLE")
    , m_batteryLevel(-1)
    , m_anyDeviceFound(false)
{
    LogManager::instance()->sendLog(LogManager::HeadsetControlManager, "HeadsetControlMonitor initialized");

    m_fetchTimer->setInterval(FETCH_INTERVAL_MS);
    m_fetchTimer->setSingleShot(false);

    connect(m_fetchTimer, &QTimer::timeout, this, &HeadsetControlMonitor::fetchHeadsetInfo);

    if (m_settings.value("headsetcontrolMonitoring", false).toBool()) {
        LogManager::instance()->sendLog(LogManager::HeadsetControlManager, "Auto-starting headset monitoring from saved settings");
        startMonitoring();
    }
}

HeadsetControlMonitor::~HeadsetControlMonitor()
{
    LogManager::instance()->sendLog(LogManager::HeadsetControlManager, "HeadsetControlMonitor destructor called");
    stopMonitoring();
}

void HeadsetControlMonitor::startMonitoring()
{
    if (m_isMonitoring) {
        LogManager::instance()->sendLog(LogManager::HeadsetControlManager, "Headset monitoring already running, ignoring start request");
        return;
    }

    LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                    QString("Starting headset monitoring (fetch interval: %1ms)").arg(FETCH_INTERVAL_MS));

    m_isMonitoring = true;

    m_fetchTimer->start();
    fetchHeadsetInfo();

    emit monitoringStateChanged(true);
}

void HeadsetControlMonitor::stopMonitoring()
{
    if (!m_isMonitoring) {
        return;
    }

    LogManager::instance()->sendLog(LogManager::HeadsetControlManager, "Stopping headset monitoring");

    m_isMonitoring = false;
    m_fetchTimer->stop();

    if (m_process && m_process->state() != QProcess::NotRunning) {
        LogManager::instance()->sendLog(LogManager::HeadsetControlManager, "Terminating running HeadsetControl process");
        m_process->kill();
        m_process->waitForFinished(3000);
    }

    m_cachedDevices.clear();
    m_hasSidetoneCapability = false;
    m_hasLightsCapability = false;
    m_deviceName = "";
    m_batteryStatus = "";
    m_batteryLevel = 0;
    bool wasDeviceFound = m_anyDeviceFound;
    m_anyDeviceFound = false;

    emit capabilitiesChanged();
    emit deviceNameChanged();
    emit batteryStatusChanged();
    emit batteryLevelChanged();
    if (wasDeviceFound) {
        emit anyDeviceFoundChanged();
    }

    emit headsetDataUpdated(m_cachedDevices);
    emit monitoringStateChanged(false);

    LogManager::instance()->sendLog(LogManager::HeadsetControlManager, "Headset monitoring stopped and data cleared");
}

bool HeadsetControlMonitor::isMonitoring() const
{
    return m_isMonitoring;
}

void HeadsetControlMonitor::setLights(bool enabled)
{
    if (!m_hasLightsCapability) {
        LogManager::instance()->sendWarn(LogManager::HeadsetControlManager, "Cannot set lights - device does not support lights capability");
        return;
    }

    LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                    QString("Setting headset lights: %1").arg(enabled ? "ON" : "OFF"));

    QStringList arguments;
    arguments << "-l" << (enabled ? "1" : "0");
    executeHeadsetControlCommand(arguments);
}

void HeadsetControlMonitor::setSidetone(int value)
{
    if (!m_hasSidetoneCapability) {
        LogManager::instance()->sendWarn(LogManager::HeadsetControlManager, "Cannot set sidetone - device does not support sidetone capability");
        return;
    }

    value = qBound(0, value, 128);

    LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                    QString("Setting headset sidetone to %1").arg(value));

    QStringList arguments;
    arguments << "-s" << QString::number(value);
    executeHeadsetControlCommand(arguments);
}

void HeadsetControlMonitor::executeHeadsetControlCommand(const QStringList& arguments)
{
    QString executablePath = getExecutablePath();
    if (!QFile::exists(executablePath)) {
        LogManager::instance()->sendCritical(LogManager::HeadsetControlManager,
                                             QString("HeadsetControl executable not found at: %1").arg(executablePath));
        return;
    }

    LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                    QString("Executing HeadsetControl command with arguments: %1").arg(arguments.join(" ")));

    QProcess* commandProcess = new QProcess(this);

    connect(commandProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [commandProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
                    LogManager::instance()->sendCritical(LogManager::HeadsetControlManager,
                                                         QString("HeadsetControl command failed with exit code: %1").arg(exitCode));
                    QByteArray errorOutput = commandProcess->readAllStandardError();
                    if (!errorOutput.isEmpty()) {
                        LogManager::instance()->sendCritical(LogManager::HeadsetControlManager,
                                                             QString("HeadsetControl error output: %1").arg(QString::fromUtf8(errorOutput)));
                    }
                } else {
                    LogManager::instance()->sendLog(LogManager::HeadsetControlManager, "HeadsetControl command executed successfully");
                }
                commandProcess->deleteLater();
            });

    // Add error handling for command startup failures
    connect(commandProcess, &QProcess::errorOccurred, this,
            [commandProcess](QProcess::ProcessError error) {
                LogManager::instance()->sendCritical(LogManager::HeadsetControlManager,
                                                     QString("HeadsetControl command process error: %1").arg(static_cast<int>(error)));
                commandProcess->deleteLater();
            });

    commandProcess->start(executablePath, arguments);
}

void HeadsetControlMonitor::fetchHeadsetInfo()
{
    // Early return if monitoring is stopped
    if (!m_isMonitoring) {
        return;
    }

    // Don't start new process if one is already running
    if (m_process && m_process->state() != QProcess::NotRunning) {
        return;
    }

    QString executablePath = getExecutablePath();
    if (!QFile::exists(executablePath)) {
        LogManager::instance()->sendCritical(LogManager::HeadsetControlManager,
                                             QString("HeadsetControl executable not found at: %1").arg(executablePath));
        return;
    }

    m_process = new QProcess(this);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &HeadsetControlMonitor::onProcessFinished);

    // Add error handling for startup failures
    connect(m_process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
                LogManager::instance()->sendCritical(LogManager::HeadsetControlManager,
                                                     QString("HeadsetControl process error: %1").arg(static_cast<int>(error)));
                if (m_process) {
                    m_process->deleteLater();
                    m_process = nullptr;
                }
            });

    QStringList arguments;
    arguments << "-o" << "JSON";

    m_process->start(executablePath, arguments);
}

void HeadsetControlMonitor::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_process) {
        return;
    }

    // Ignore results if monitoring was stopped while process was running
    if (!m_isMonitoring) {
        m_process->deleteLater();
        m_process = nullptr;
        return;
    }

    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        QByteArray output = m_process->readAllStandardOutput();
        LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                        QString("HeadsetControl process completed successfully (%1 bytes output)").arg(output.size()));
        parseHeadsetControlOutput(output);
    } else if (exitCode == 1) {
        // No device found - clear cached data
        LogManager::instance()->sendLog(LogManager::HeadsetControlManager, "No headset devices found");

        m_cachedDevices.clear();
        m_hasSidetoneCapability = false;
        m_hasLightsCapability = false;
        m_deviceName = "";
        m_batteryStatus = "";
        m_batteryLevel = 0;
        bool wasDeviceFound = m_anyDeviceFound;
        m_anyDeviceFound = false;

        emit capabilitiesChanged();
        emit deviceNameChanged();
        emit batteryStatusChanged();
        emit batteryLevelChanged();
        if (wasDeviceFound) {
            emit anyDeviceFoundChanged();
        }
        emit headsetDataUpdated(m_cachedDevices);
    } else {
        LogManager::instance()->sendCritical(LogManager::HeadsetControlManager,
                                             QString("HeadsetControl process failed with exit code: %1, status: %2").arg(exitCode).arg(static_cast<int>(exitStatus)));
        QByteArray errorOutput = m_process->readAllStandardError();
        if (!errorOutput.isEmpty()) {
            LogManager::instance()->sendCritical(LogManager::HeadsetControlManager,
                                                 QString("HeadsetControl error output: %1").arg(QString::fromUtf8(errorOutput)));
        }

        // Still emit current cached devices on error
        emit headsetDataUpdated(m_cachedDevices);
    }

    m_process->deleteLater();
    m_process = nullptr;
}

void HeadsetControlMonitor::parseHeadsetControlOutput(const QByteArray& output)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(output, &error);

    if (error.error != QJsonParseError::NoError) {
        LogManager::instance()->sendCritical(LogManager::HeadsetControlManager,
                                             QString("Failed to parse HeadsetControl JSON output: %1").arg(error.errorString()));
        return;
    }

    QJsonObject root = doc.object();
    if (!root.contains("devices")) {
        LogManager::instance()->sendCritical(LogManager::HeadsetControlManager,
                                             "HeadsetControl output missing 'devices' field");
        return;
    }

    QJsonArray devicesArray = root["devices"].toArray();
    QList<HeadsetControlDevice> newDevices;

    if (devicesArray.isEmpty()) {
        LogManager::instance()->sendLog(LogManager::HeadsetControlManager, "No devices in HeadsetControl output");
        m_cachedDevices = newDevices;
        updateCapabilities();
        emit headsetDataUpdated(m_cachedDevices);
        return;
    }

    LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                    QString("Processing %1 headset devices from output").arg(devicesArray.size()));

    for (const QJsonValue& deviceValue : devicesArray) {
        QJsonObject deviceObj = deviceValue.toObject();

        if (deviceObj["status"].toString() != "success") {
            LogManager::instance()->sendWarn(LogManager::HeadsetControlManager,
                                             QString("Skipping device with non-success status: %1").arg(deviceObj["status"].toString()));
            continue;
        }

        HeadsetControlDevice device;
        device.deviceName = deviceObj["device"].toString();
        device.vendor = deviceObj["vendor"].toString();
        device.product = deviceObj["product"].toString();
        device.vendorId = deviceObj["id_vendor"].toString();
        device.productId = deviceObj["id_product"].toString();

        QJsonArray capabilitiesArray = deviceObj["capabilities"].toArray();
        for (const QJsonValue& cap : capabilitiesArray) {
            device.capabilities << cap.toString();
        }

        if (deviceObj.contains("battery")) {
            QJsonObject batteryObj = deviceObj["battery"].toObject();
            device.batteryStatus = batteryObj["status"].toString();
            device.batteryLevel = batteryObj["level"].toInt();

            LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                            QString("Device %1: Battery %2 at %3%").arg(device.deviceName, device.batteryStatus).arg(device.batteryLevel));
        } else {
            device.batteryStatus = "BATTERY_UNAVAILABLE";
            device.batteryLevel = 0;
        }

        LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                        QString("Found headset device: %1 (%2 %3) with %4 capabilities")
                                            .arg(device.deviceName, device.vendor, device.product).arg(device.capabilities.size()));

        newDevices.append(device);
    }

    m_cachedDevices = newDevices;
    updateCapabilities();

    // Update bridge battery info from the first device with valid battery info
    if (!m_cachedDevices.isEmpty()) {
        const HeadsetControlDevice& primaryDevice = m_cachedDevices.first();
        if (primaryDevice.batteryStatus != m_batteryStatus) {
            m_batteryStatus = primaryDevice.batteryStatus;
            emit batteryStatusChanged();
        }
        if (primaryDevice.batteryLevel != m_batteryLevel) {
            m_batteryLevel = primaryDevice.batteryLevel;
            emit batteryLevelChanged();
        }
    }

    emit headsetDataUpdated(m_cachedDevices);
}

void HeadsetControlMonitor::updateCapabilities()
{
    bool newSidetoneCapability = false;
    bool newLightsCapability = false;
    QString newDeviceName = "";
    bool newAnyDeviceFound = !m_cachedDevices.isEmpty();
    bool wasDeviceFound = m_anyDeviceFound;

    if (!m_cachedDevices.isEmpty()) {
        const HeadsetControlDevice& device = m_cachedDevices.first();
        newDeviceName = device.deviceName;

        newSidetoneCapability = device.capabilities.contains("CAP_SIDETONE");
        newLightsCapability = device.capabilities.contains("CAP_LIGHTS");

        LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                        QString("Device capabilities - Sidetone: %1, Lights: %2")
                                            .arg(newSidetoneCapability ? "YES" : "NO", newLightsCapability ? "YES" : "NO"));
    }

    if (newSidetoneCapability != m_hasSidetoneCapability ||
        newLightsCapability != m_hasLightsCapability) {
        m_hasSidetoneCapability = newSidetoneCapability;
        m_hasLightsCapability = newLightsCapability;
        emit capabilitiesChanged();
    }

    if (newDeviceName != m_deviceName) {
        m_deviceName = newDeviceName;
        emit deviceNameChanged();
    }

    if (newAnyDeviceFound != m_anyDeviceFound) {
        m_anyDeviceFound = newAnyDeviceFound;
        emit anyDeviceFoundChanged();

        if (!wasDeviceFound && newAnyDeviceFound) {
            LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                            "New headset device detected, applying saved settings");

            if (newLightsCapability) {
                bool lightsEnabled = m_settings.value("headsetcontrolLights", false).toBool();
                LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                                QString("Applying saved lights setting: %1").arg(lightsEnabled ? "ON" : "OFF"));
                setLights(lightsEnabled);
            }
            if (newSidetoneCapability) {
                int sidetoneValue = m_settings.value("headsetcontrolSidetone", 0).toInt();
                LogManager::instance()->sendLog(LogManager::HeadsetControlManager,
                                                QString("Applying saved sidetone setting: %1").arg(sidetoneValue));
                setSidetone(sidetoneValue);
            }
        }
    }
}

QString HeadsetControlMonitor::getExecutablePath() const
{
    return QCoreApplication::applicationDirPath() + "/dependencies/headsetcontrol.exe";
}
