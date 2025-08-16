#include "headsetcontrolmanager.h"
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>

HeadsetControlManager* HeadsetControlManager::m_instance = nullptr;

HeadsetControlManager::HeadsetControlManager(QObject *parent)
    : QObject(parent)
    , m_fetchTimer(new QTimer(this))
    , m_process(nullptr)
    , m_settings("Odizinne", "QuickSoundSwitcher")
    , m_isMonitoring(false)
{
    m_instance = this;

    m_fetchTimer->setInterval(FETCH_INTERVAL_MS);
    m_fetchTimer->setSingleShot(false);

    connect(m_fetchTimer, &QTimer::timeout, this, &HeadsetControlManager::fetchHeadsetInfo);

    if (m_settings.value("headsetcontrolMonitoring", false).toBool()) {
        startMonitoring();
    }
}

HeadsetControlManager::~HeadsetControlManager()
{
    stopMonitoring();
    if (m_instance == this) {
        m_instance = nullptr;
    }
}

HeadsetControlManager* HeadsetControlManager::instance()
{
    if (!m_instance) {
        m_instance = new HeadsetControlManager();
    }
    return m_instance;
}

void HeadsetControlManager::startMonitoring()
{
    if (m_isMonitoring) {
        return;
    }

    m_isMonitoring = true;

    fetchHeadsetInfo();
    m_fetchTimer->start();

    emit monitoringStateChanged(true);
}

void HeadsetControlManager::stopMonitoring()
{
    if (!m_isMonitoring) {
        return;
    }

    m_isMonitoring = false;
    m_fetchTimer->stop();

    // Clean up any running process
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }

    // Clear device data
    m_devices.clear();
    emit headsetDataUpdated(m_devices);
    emit monitoringStateChanged(false);
}

bool HeadsetControlManager::isMonitoring() const
{
    return m_isMonitoring;
}

void HeadsetControlManager::setMonitoringEnabled(bool enabled)
{
    if (enabled) {
        startMonitoring();
    } else {
        stopMonitoring();
    }
}

void HeadsetControlManager::fetchHeadsetInfo()
{
    if (!m_isMonitoring) {
        return;
    }

    if (m_process && m_process->state() != QProcess::NotRunning) {
        qWarning() << "HeadsetControl process already running, skipping fetch";
        return;
    }

    if (m_process) {
        m_process->deleteLater();
    }

    m_process = new QProcess(this);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &HeadsetControlManager::onProcessFinished);

    QString executablePath = getExecutablePath();
    QStringList arguments;
    arguments << "-o" << "json";

    m_process->start(executablePath, arguments);

    if (!m_process->waitForStarted(5000)) {
        qWarning() << "Failed to start HeadsetControl process:" << m_process->errorString();
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void HeadsetControlManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_process) {
        return;
    }

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QByteArray output = m_process->readAllStandardOutput();
        parseHeadsetControlOutput(output);
    } else {
        qWarning() << "HeadsetControl process failed. Exit code:" << exitCode
                   << "Status:" << exitStatus;
        QByteArray errorOutput = m_process->readAllStandardError();
        if (!errorOutput.isEmpty()) {
            qWarning() << "Error output:" << errorOutput;
        }

        // Clear devices on error
        m_devices.clear();
        emit headsetDataUpdated(m_devices);
    }

    m_process->deleteLater();
    m_process = nullptr;
}

void HeadsetControlManager::parseHeadsetControlOutput(const QByteArray& output)
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

        // Parse capabilities
        QJsonArray capabilitiesArray = deviceObj["capabilities_str"].toArray();
        for (const QJsonValue& cap : capabilitiesArray) {
            device.capabilities << cap.toString();
        }

        // Parse battery information
        if (deviceObj.contains("battery")) {
            QJsonObject batteryObj = deviceObj["battery"].toObject();
            device.batteryStatus = batteryObj["status"].toString();
            device.batteryLevel = batteryObj["level"].toInt();
        } else {
            device.batteryStatus = "BATTERY_UNAVAILABLE";
            device.batteryLevel = 0;
        }

        newDevices.append(device);
    }

    m_devices = newDevices;
    emit headsetDataUpdated(m_devices);
}

QString HeadsetControlManager::getExecutablePath() const
{
    return QCoreApplication::applicationDirPath() + "/dependencies/headsetcontrol.exe";
}

int HeadsetControlManager::getBatteryLevel(const QString& vendorId, const QString& productId) const
{
    for (const HeadsetControlDevice& device : m_devices) {
        // Remove "0x" prefix and compare case-insensitively
        QString deviceVid = device.vendorId.startsWith("0x") ? device.vendorId.mid(2) : device.vendorId;
        QString devicePid = device.productId.startsWith("0x") ? device.productId.mid(2) : device.productId;
        QString inputVid = vendorId.startsWith("0x") ? vendorId.mid(2) : vendorId;
        QString inputPid = productId.startsWith("0x") ? productId.mid(2) : productId;

        if (deviceVid.compare(inputVid, Qt::CaseInsensitive) == 0 &&
            devicePid.compare(inputPid, Qt::CaseInsensitive) == 0) {
            return device.batteryLevel;
        }
    }
    return -1; // Not found or not available
}

QString HeadsetControlManager::getBatteryStatus(const QString& vendorId, const QString& productId) const
{
    for (const HeadsetControlDevice& device : m_devices) {
        // Remove "0x" prefix and compare case-insensitively
        QString deviceVid = device.vendorId.startsWith("0x") ? device.vendorId.mid(2) : device.vendorId;
        QString devicePid = device.productId.startsWith("0x") ? device.productId.mid(2) : device.productId;
        QString inputVid = vendorId.startsWith("0x") ? vendorId.mid(2) : vendorId;
        QString inputPid = productId.startsWith("0x") ? productId.mid(2) : productId;

        if (deviceVid.compare(inputVid, Qt::CaseInsensitive) == 0 &&
            devicePid.compare(inputPid, Qt::CaseInsensitive) == 0) {
            return device.batteryStatus;
        }
    }
    return "BATTERY_UNAVAILABLE";
}
