#include "headsetcontrolmonitor.h"
#include <QDebug>
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
    , m_batteryStatus("")
    , m_batteryLevel(0)
    , m_anyDeviceFound(false)
{
    m_fetchTimer->setInterval(FETCH_INTERVAL_MS);
    m_fetchTimer->setSingleShot(false);

    connect(m_fetchTimer, &QTimer::timeout, this, &HeadsetControlMonitor::fetchHeadsetInfo);

    if (m_settings.value("headsetcontrolMonitoring", false).toBool()) {
        startMonitoring();
    }
}

HeadsetControlMonitor::~HeadsetControlMonitor()
{
    stopMonitoring();
}

void HeadsetControlMonitor::startMonitoring()
{
    if (m_isMonitoring) {
        return;
    }

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

    m_isMonitoring = false;
    m_fetchTimer->stop();

    if (m_process && m_process->state() != QProcess::NotRunning) {
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
}

bool HeadsetControlMonitor::isMonitoring() const
{
    return m_isMonitoring;
}

void HeadsetControlMonitor::setLights(bool enabled)
{
    if (!m_hasLightsCapability) {
        qWarning() << "Device does not support lights capability";
        return;
    }

    QStringList arguments;
    arguments << "-l" << (enabled ? "1" : "0");
    executeHeadsetControlCommand(arguments);
}

void HeadsetControlMonitor::setSidetone(int value)
{
    if (!m_hasSidetoneCapability) {
        qWarning() << "Device does not support sidetone capability";
        return;
    }

    value = qBound(0, value, 128);

    QStringList arguments;
    arguments << "-s" << QString::number(value);
    executeHeadsetControlCommand(arguments);
}

void HeadsetControlMonitor::executeHeadsetControlCommand(const QStringList& arguments)
{
    QString executablePath = getExecutablePath();
    if (!QFile::exists(executablePath)) {
        qWarning() << "HeadsetControl executable not found at:" << executablePath;
        return;
    }

    QProcess* commandProcess = new QProcess(this);

    connect(commandProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [commandProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
                    qWarning() << "HeadsetControl command failed. Exit code:" << exitCode;
                    QByteArray errorOutput = commandProcess->readAllStandardError();
                    if (!errorOutput.isEmpty()) {
                        qWarning() << "Error output:" << errorOutput;
                    }
                }
                commandProcess->deleteLater();
            });

    // Add error handling for command startup failures
    connect(commandProcess, &QProcess::errorOccurred, this,
            [commandProcess](QProcess::ProcessError error) {
                qWarning() << "HeadsetControl command process error:" << error;
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
        qWarning() << "HeadsetControl executable not found at:" << executablePath;
        return;
    }

    m_process = new QProcess(this);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &HeadsetControlMonitor::onProcessFinished);

    // Add error handling for startup failures
    connect(m_process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
                qWarning() << "HeadsetControl process error:" << error;
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
        parseHeadsetControlOutput(output);
    } else if (exitCode == 1) {
        // No device found - clear cached data
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
        qWarning() << "HeadsetControl process failed. Exit code:" << exitCode
                   << "Status:" << exitStatus;
        QByteArray errorOutput = m_process->readAllStandardError();
        if (!errorOutput.isEmpty()) {
            qWarning() << "Error output:" << errorOutput;
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
        qWarning() << "Failed to parse HeadsetControl JSON output:" << error.errorString();
        return;
    }

    QJsonObject root = doc.object();
    if (!root.contains("devices")) {
        qWarning() << "HeadsetControl output missing 'devices' field";
        return;
    }

    QJsonArray devicesArray = root["devices"].toArray();
    QList<HeadsetControlDevice> newDevices;

    if (devicesArray.isEmpty()) {
        m_cachedDevices = newDevices;
        updateCapabilities();
        emit headsetDataUpdated(m_cachedDevices);
        return;
    }

    for (const QJsonValue& deviceValue : devicesArray) {
        QJsonObject deviceObj = deviceValue.toObject();

        if (deviceObj["status"].toString() != "success") {
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
        } else {
            device.batteryStatus = "BATTERY_UNAVAILABLE";
            device.batteryLevel = 0;
        }

        if (device.batteryStatus != m_batteryStatus) {
            m_batteryStatus = device.batteryStatus;
            emit batteryStatusChanged();
        }

        if (device.batteryLevel != m_batteryLevel) {
            m_batteryLevel = device.batteryLevel;
            emit batteryLevelChanged();
        }

        newDevices.append(device);
    }

    m_cachedDevices = newDevices;
    updateCapabilities();
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
            if (newLightsCapability) {
                setLights(m_settings.value("headsetcontrolLights", false).toBool());
            }
            if (newSidetoneCapability) {
                setSidetone(m_settings.value("headsetcontrolSidetone", 0).toInt());
            }
        }
    }
}

QString HeadsetControlMonitor::getExecutablePath() const
{
    return QCoreApplication::applicationDirPath() + "/dependencies/headsetcontrol.exe";
}
