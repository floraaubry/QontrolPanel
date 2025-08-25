#include "bluetoothbatterymonitor.h"
#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSettings>

// Include WinRT headers only in the implementation file, with proper order
#include <windows.h>
#include <setupapi.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>
#include <cfgmgr32.h>

// Include WinRT headers after traditional Windows headers
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Networking.Sockets.h>
#include <winrt/Windows.Devices.Bluetooth.Rfcomm.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

BluetoothBatteryMonitor* BluetoothBatteryMonitor::m_instance = nullptr;

//=============================================================================
// BluetoothBatteryWorker Implementation
//=============================================================================

BluetoothBatteryWorker::BluetoothBatteryWorker()
    : m_isMonitoring(false)
    , m_isInitialized(false)
    , m_deviceWatcher(nullptr)
    , m_deviceAddedToken(nullptr)
    , m_deviceRemovedToken(nullptr)
    , m_deviceUpdatedToken(nullptr)
    , m_settings("Odizinne", "QontrolPanel")
{
    //m_scanTimer = new QTimer(this);
    //m_batteryUpdateTimer = new QTimer(this);
//
    //connect(m_scanTimer, &QTimer::timeout, this, &BluetoothBatteryWorker::onScanTimer);
    //connect(m_batteryUpdateTimer, &QTimer::timeout, this, &BluetoothBatteryWorker::onBatteryUpdateTimer);
//
    //m_scanTimer->setSingleShot(false);
    //m_batteryUpdateTimer->setSingleShot(false);
//
    //// Load cached devices from settings
    //loadCachedDevices();
//
    //// Cache device instance IDs on startup
    //cacheDeviceInstanceIds();
}

BluetoothBatteryWorker::~BluetoothBatteryWorker()
{
    stopMonitoring();
    cleanupBluetoothWatcher();
}

void BluetoothBatteryWorker::loadCachedDevices()
{
    QMutexLocker locker(&m_devicesMutex);

    int size = m_settings.beginReadArray("bluetoothDevices");
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);

        BluetoothDeviceBattery device;
        device.deviceId = m_settings.value("deviceId").toString();
        device.deviceName = m_settings.value("deviceName").toString();
        device.macAddress = m_settings.value("macAddress").toString();
        device.vendorId = m_settings.value("vendorId").toString();
        device.productId = m_settings.value("productId").toString();
        device.isConnected = m_settings.value("isConnected", false).toBool();
        device.hasBatteryInfo = m_settings.value("hasBatteryInfo", false).toBool();
        device.batteryLevel = m_settings.value("batteryLevel", -1).toInt();
        device.batteryStatus = m_settings.value("batteryStatus", "UNAVAILABLE").toString();
        device.deviceType = m_settings.value("deviceType", "Unknown").toString();
        device.lastUpdated = m_settings.value("lastUpdated", 0).toLongLong();

        if (!device.deviceId.isEmpty() && !device.macAddress.isEmpty()) {
            m_devices[device.deviceId] = device;
            m_macToDeviceId[device.macAddress] = device.deviceId;
        }
    }
    m_settings.endArray();
}

void BluetoothBatteryWorker::saveCachedDevices()
{
    QMutexLocker locker(&m_devicesMutex);

    m_settings.beginWriteArray("bluetoothDevices");
    int index = 0;
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        const BluetoothDeviceBattery& device = it.value();

        m_settings.setArrayIndex(index++);
        m_settings.setValue("deviceId", device.deviceId);
        m_settings.setValue("deviceName", device.deviceName);
        m_settings.setValue("macAddress", device.macAddress);
        m_settings.setValue("vendorId", device.vendorId);
        m_settings.setValue("productId", device.productId);
        m_settings.setValue("isConnected", device.isConnected);
        m_settings.setValue("hasBatteryInfo", device.hasBatteryInfo);
        m_settings.setValue("batteryLevel", device.batteryLevel);
        m_settings.setValue("batteryStatus", device.batteryStatus);
        m_settings.setValue("deviceType", device.deviceType);
        m_settings.setValue("lastUpdated", device.lastUpdated);
    }
    m_settings.endArray();
}

void BluetoothBatteryWorker::startMonitoring()
{
    if (m_isMonitoring) {
        return;
    }

    try {
        winrt::init_apartment();

        if (!m_isInitialized) {
            initializeBluetoothWatcher();
            m_isInitialized = true;
        }

        m_isMonitoring = true;

        // Emit cached devices immediately for instant UI population
        {
            QMutexLocker locker(&m_devicesMutex);
            QList<BluetoothDeviceBattery> cachedDevicesList;
            for (const auto& device : m_devices) {
                cachedDevicesList.append(device);
            }
            if (!cachedDevicesList.isEmpty()) {
                emit devicesUpdated(cachedDevicesList);
            }
        }

        // Start timers
        m_scanTimer->start(SCAN_INTERVAL_MS);
        m_batteryUpdateTimer->start(BATTERY_UPDATE_INTERVAL_MS);

        // Initial scan to update current state
        scanForDevices();
    }
    catch (const std::exception& e) {
        qWarning() << "BluetoothBatteryMonitor: Failed to start monitoring:" << e.what();
    }
    catch (...) {
        qWarning() << "BluetoothBatteryMonitor: Failed to start monitoring: Unknown error";
    }
}

void BluetoothBatteryWorker::stopMonitoring()
{
    if (!m_isMonitoring) {
        return;
    }

    m_isMonitoring = false;

    m_scanTimer->stop();
    m_batteryUpdateTimer->stop();

    // Save current device state to cache
    saveCachedDevices();
}

void BluetoothBatteryWorker::initializeBluetoothWatcher()
{
    try {
        // For now, we'll use a simpler approach without device watcher
        // to avoid the complex event handler setup that might cause header conflicts
    }
    catch (const std::exception& e) {
        qWarning() << "BluetoothBatteryMonitor: Failed to initialize:" << e.what();
    }
}

void BluetoothBatteryWorker::cleanupBluetoothWatcher()
{
    // Clean up any WinRT objects if needed
    m_deviceWatcher = nullptr;
    m_deviceAddedToken = nullptr;
    m_deviceRemovedToken = nullptr;
    m_deviceUpdatedToken = nullptr;
}

QString BluetoothBatteryWorker::extractMacAddressFromDeviceId(const QString& deviceId)
{
    // Bluetooth device IDs have the format:
    // "Bluetooth#Bluetooth{host_mac}-{device_mac}"
    // We want the device MAC (after the dash), not the host MAC

    // Look for the pattern after the dash
    QRegularExpression deviceMacRegex(R"(-([0-9A-Fa-f]{2}[:-][0-9A-Fa-f]{2}[:-][0-9A-Fa-f]{2}[:-][0-9A-Fa-f]{2}[:-][0-9A-Fa-f]{2}[:-][0-9A-Fa-f]{2}))");
    QRegularExpressionMatch match = deviceMacRegex.match(deviceId);

    if (match.hasMatch()) {
        QString macStr = match.captured(1); // Get the MAC after the dash
        return normalizeBluetoothAddress(macStr);
    }

    // Fallback: look for any MAC pattern
    QRegularExpression anyMacRegex(R"([0-9A-Fa-f]{2}[:-][0-9A-Fa-f]{2}[:-][0-9A-Fa-f]{2}[:-][0-9A-Fa-f]{2}[:-][0-9A-Fa-f]{2}[:-][0-9A-Fa-f]{2})");
    QRegularExpressionMatchIterator it = anyMacRegex.globalMatch(deviceId);

    // Get the last MAC address found (usually the device MAC)
    QString lastMac;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        lastMac = match.captured(0);
    }

    if (!lastMac.isEmpty()) {
        return normalizeBluetoothAddress(lastMac);
    }

    qWarning() << "BluetoothBatteryMonitor: Could not extract MAC address from device ID:" << deviceId;
    return QString();
}

QString BluetoothBatteryWorker::extractBluetoothVendorId(const QString& deviceId)
{
    Q_UNUSED(deviceId)
    return QString(); // Placeholder for now
}

QString BluetoothBatteryWorker::extractBluetoothProductId(const QString& deviceId)
{
    Q_UNUSED(deviceId)
    return QString(); // Placeholder for now
}

QString BluetoothBatteryWorker::normalizeBluetoothAddress(const QString& address)
{
    // Normalize to uppercase with colons: XX:XX:XX:XX:XX:XX
    QString normalized = address.toUpper();
    normalized.replace("-", ":");
    return normalized;
}

void BluetoothBatteryWorker::scanForDevices()
{
    if (!m_isMonitoring) {
        return;
    }
    try {
        // Check if Bluetooth adapter is available first
        try {
            auto adapterAsync = winrt::Windows::Devices::Bluetooth::BluetoothAdapter::GetDefaultAsync();
            if (!adapterAsync) {
                qWarning() << "BluetoothBatteryMonitor: Cannot get Bluetooth adapter async operation";
                return;
            }

            auto adapter = adapterAsync.get();
            if (!adapter) {
                qWarning() << "BluetoothBatteryMonitor: No Bluetooth adapter found";
                return;
            }
        }
        catch (winrt::hresult_error const& ex) {
            HRESULT hr = ex.code();
            qWarning() << "BluetoothBatteryMonitor: Bluetooth adapter check failed, HRESULT:"
                       << QString::number(hr, 16);
            return;
        }

        // Enumerate paired Bluetooth devices using WinRT
        auto deviceSelector = winrt::Windows::Devices::Bluetooth::BluetoothDevice::GetDeviceSelectorFromPairingState(true);
        auto deviceInfoAsync = winrt::Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(deviceSelector);

        if (!deviceInfoAsync) {
            qWarning() << "BluetoothBatteryMonitor: Failed to create device enumeration async operation";
            return;
        }

        auto deviceInfoCollection = deviceInfoAsync.get();

        QList<BluetoothDeviceBattery> currentDevices;
        bool hasUpdates = false;

        for (auto const& deviceInfo : deviceInfoCollection) {
            try {
                QString deviceId = QString::fromWCharArray(deviceInfo.Id().c_str());
                QString deviceName = QString::fromWCharArray(deviceInfo.Name().c_str());

                if (deviceName.isEmpty()) {
                    continue;
                }

                QString macAddress = extractMacAddressFromDeviceId(deviceId);
                if (macAddress.isEmpty()) {
                    continue;
                }

                BluetoothDeviceBattery batteryInfo;
                // Check if we have cached data for this device
                {
                    QMutexLocker locker(&m_devicesMutex);
                    if (m_devices.contains(deviceId)) {
                        batteryInfo = m_devices[deviceId];
                    }
                }

                // Update basic info from current scan
                batteryInfo.deviceId = deviceId;
                batteryInfo.deviceName = deviceName;
                batteryInfo.macAddress = macAddress;
                batteryInfo.vendorId = extractBluetoothVendorId(deviceId);
                batteryInfo.productId = extractBluetoothProductId(deviceId);
                batteryInfo.isConnected = deviceInfo.IsEnabled();
                batteryInfo.lastUpdated = QDateTime::currentMSecsSinceEpoch();

                // Only try to get detailed device info if device seems enabled
                if (deviceInfo.IsEnabled()) {
                    // Try to get more detailed device info and battery level
                    readDeviceBatteryLevel(deviceId);

                    // Get updated info after readDeviceBatteryLevel
                    {
                        QMutexLocker locker(&m_devicesMutex);
                        if (m_devices.contains(deviceId)) {
                            batteryInfo = m_devices[deviceId];
                        }
                    }
                } else {
                    // Device is disabled, mark appropriately
                    batteryInfo.isConnected = false;
                    batteryInfo.hasBatteryInfo = false;
                    batteryInfo.batteryLevel = -1;
                    batteryInfo.batteryStatus = "UNAVAILABLE";
                }

                bool isNewDevice = false;
                {
                    QMutexLocker locker(&m_devicesMutex);
                    if (!m_devices.contains(deviceId)) {
                        isNewDevice = true;
                        hasUpdates = true;
                    }
                    // Always update the device info
                    m_devices[deviceId] = batteryInfo;
                    m_macToDeviceId[macAddress] = deviceId;
                }

                currentDevices.append(batteryInfo);
            }
            catch (winrt::hresult_error const& ex) {
                HRESULT hr = ex.code();
                if (hr == static_cast<HRESULT>(0x800710DF)) { // ERROR_DEVICE_NOT_READY
                    qDebug() << "BluetoothBatteryMonitor: Device not ready during scan, skipping";
                } else {
                    qWarning() << "BluetoothBatteryMonitor: WinRT error processing device info, HRESULT:"
                               << QString::number(hr, 16);
                }
                // Continue with next device instead of breaking entire scan
                continue;
            }
            catch (const std::exception& e) {
                qWarning() << "BluetoothBatteryMonitor: Error processing device info:" << e.what();
                continue;
            }
            catch (...) {
                qWarning() << "BluetoothBatteryMonitor: Unknown error processing device info";
                continue;
            }
        }

        // Save updated cache to settings
        if (hasUpdates) {
            saveCachedDevices();
        }

        emit devicesUpdated(currentDevices);
    }
    catch (winrt::hresult_error const& ex) {
        HRESULT hr = ex.code();
        if (hr == static_cast<HRESULT>(0x8007001F)) { // ERROR_GEN_FAILURE
            qWarning() << "BluetoothBatteryMonitor: Bluetooth adapter general failure - might be turned off";
        } else if (hr == static_cast<HRESULT>(0x80070422)) { // ERROR_SERVICE_DISABLED
            qWarning() << "BluetoothBatteryMonitor: Bluetooth service is disabled";
        } else {
            qWarning() << "BluetoothBatteryMonitor: WinRT error during device scan, HRESULT:"
                       << QString::number(hr, 16)
                       << "Message:" << QString::fromWCharArray(ex.message().c_str());
        }
    }
    catch (const std::exception& e) {
        qWarning() << "BluetoothBatteryMonitor: Error during device scan:" << e.what();
    }
    catch (...) {
        qWarning() << "BluetoothBatteryMonitor: Unknown error during device scan";
    }
}

void BluetoothBatteryWorker::updateDeviceBatteries()
{
    QList<QString> deviceIds;
    {
        QMutexLocker locker(&m_devicesMutex);
        deviceIds = m_devices.keys();
    }

    for (const QString& deviceId : deviceIds) {
        readDeviceBatteryLevel(deviceId);
    }
}

void BluetoothBatteryWorker::readDeviceBatteryLevel(const QString& deviceId)
{
    if (!m_isMonitoring) {
        return;
    }
    BluetoothDeviceBattery batteryInfo;
    {
        QMutexLocker locker(&m_devicesMutex);
        if (!m_devices.contains(deviceId)) {
            return;
        }
        batteryInfo = m_devices[deviceId];
    }
    try {
        // First check if main device is connected
        std::wstring deviceIdStd = deviceId.toStdWString();
        winrt::hstring deviceIdHstring{ deviceIdStd };

        auto asyncOp = winrt::Windows::Devices::Bluetooth::BluetoothDevice::FromIdAsync(deviceIdHstring);
        if (!asyncOp) {
            qDebug() << "BluetoothBatteryMonitor: Failed to create async operation for device:" << deviceId;
            return;
        }

        auto bluetoothDevice = asyncOp.get();

        if (bluetoothDevice) {
            batteryInfo.isConnected = (bluetoothDevice.ConnectionStatus() ==
                                       winrt::Windows::Devices::Bluetooth::BluetoothConnectionStatus::Connected);
            uint64_t deviceAddress = bluetoothDevice.BluetoothAddress();
            batteryInfo.macAddress = bluetoothAddressToString(deviceAddress);
            if (bluetoothDevice.ClassOfDevice()) {
                uint32_t cod = bluetoothDevice.ClassOfDevice().RawValue();
                batteryInfo.deviceType = getDeviceTypeFromClassOfDevice(cod);
            }
        } else {
            // Device not accessible, mark as disconnected
            batteryInfo.isConnected = false;
            batteryInfo.hasBatteryInfo = false;
            batteryInfo.batteryLevel = -1;
            batteryInfo.batteryStatus = "UNAVAILABLE";
        }

        // Only check battery if device is connected
        if (batteryInfo.isConnected) {
            findBatteryInfoEfficiently(batteryInfo);
        } else {
            batteryInfo.hasBatteryInfo = false;
            batteryInfo.batteryLevel = -1;
            batteryInfo.batteryStatus = "UNAVAILABLE";
        }

        batteryInfo.lastUpdated = QDateTime::currentMSecsSinceEpoch();

        // Update cache and emit signal
        {
            QMutexLocker locker(&m_devicesMutex);
            m_devices[deviceId] = batteryInfo;
            m_macToDeviceId[batteryInfo.macAddress] = deviceId;
        }
        // Save to persistent cache after battery update
        saveCachedDevices();
        emit deviceBatteryChanged(batteryInfo);
    }
    catch (winrt::hresult_error const& ex) {
        HRESULT hr = ex.code();

        // Handle specific error codes gracefully
        if (hr == static_cast<HRESULT>(0x800710DF)) { // ERROR_DEVICE_NOT_READY
            qDebug() << "BluetoothBatteryMonitor: Device not ready:" << deviceId;
            // Mark device as disconnected but don't treat as error
            batteryInfo.isConnected = false;
            batteryInfo.hasBatteryInfo = false;
            batteryInfo.batteryLevel = -1;
            batteryInfo.batteryStatus = "UNAVAILABLE";
            batteryInfo.lastUpdated = QDateTime::currentMSecsSinceEpoch();

            {
                QMutexLocker locker(&m_devicesMutex);
                m_devices[deviceId] = batteryInfo;
                m_macToDeviceId[batteryInfo.macAddress] = deviceId;
            }
            saveCachedDevices();
            emit deviceBatteryChanged(batteryInfo);
            return;
        }
        else if (hr == static_cast<HRESULT>(0x80070490)) { // ERROR_NOT_FOUND
            qDebug() << "BluetoothBatteryMonitor: Device not found:" << deviceId;
            return;
        }
        else if (hr == static_cast<HRESULT>(0x80070005)) { // ERROR_ACCESS_DENIED
            qDebug() << "BluetoothBatteryMonitor: Access denied for device:" << deviceId;
            return;
        }
        else {
            qWarning() << "BluetoothBatteryMonitor: WinRT error for device" << deviceId
                       << "HRESULT:" << QString::number(hr, 16)
                       << "Message:" << QString::fromWCharArray(ex.message().c_str());
        }
    }
    catch (const std::exception& e) {
        qWarning() << "BluetoothBatteryMonitor: Error reading device info for" << deviceId << ":" << e.what();
    }
    catch (...) {
        qWarning() << "BluetoothBatteryMonitor: Unknown error reading device info for" << deviceId;
    }
}

bool BluetoothBatteryWorker::findBatteryInfoEfficiently(BluetoothDeviceBattery& batteryInfo)
{
    try {
        // Extract MAC address from the main device MAC
        QString targetMacFormatted = batteryInfo.macAddress.replace(":", "").toUpper();

        // Search for HSP/HFP Audio Gateway device with the same MAC
        QMutexLocker locker(&m_devicesMutex);
        for (auto it = m_deviceInstanceCache.begin(); it != m_deviceInstanceCache.end(); ++it) {
            const QString& cachedDeviceName = it.key();
            const QString& instanceId = it.value();

            // Look specifically for HFP Audio Gateway devices that contain our MAC
            if (cachedDeviceName.contains("Hands-Free Profile AudioGateway", Qt::CaseInsensitive) &&
                instanceId.contains(targetMacFormatted, Qt::CaseInsensitive)) {

                if (getBatteryFromHfpDevice(instanceId, batteryInfo)) {
                    return true;
                }
            }
        }
        locker.unlock();

        return false;
    }
    catch (const std::exception& e) {
        qWarning() << "BluetoothBatteryMonitor: Exception in findBatteryInfoEfficiently:" << e.what();
        return false;
    }
    catch (...) {
        qWarning() << "BluetoothBatteryMonitor: Unknown exception in findBatteryInfoEfficiently";
        return false;
    }
}

// Get battery info specifically from HFP device using SetupAPI
bool BluetoothBatteryWorker::getBatteryFromHfpDevice(const QString& instanceId, BluetoothDeviceBattery& batteryInfo)
{
    try {
        // Initialize COM
        HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
        bool comInitialized = SUCCEEDED(hr);

        // Convert instanceId to wide string
        std::wstring instanceIdW = instanceId.toStdWString();

        // Open device using SetupAPI
        HDEVINFO deviceInfoSet = SetupDiGetClassDevs(nullptr, nullptr, nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);
        if (deviceInfoSet == INVALID_HANDLE_VALUE) {
            if (comInitialized) CoUninitialize();
            return false;
        }

        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(deviceInfoData);

        // Find our specific device
        for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
            WCHAR currentInstanceId[512] = {0};

            if (SetupDiGetDeviceInstanceId(deviceInfoSet, &deviceInfoData, currentInstanceId,
                                           sizeof(currentInstanceId)/sizeof(WCHAR), nullptr)) {

                if (wcscmp(currentInstanceId, instanceIdW.c_str()) == 0) {
                    // Try to get battery property using CM_Get_DevNode_Property
                    DEVPROPKEY batteryKey;
                    // {104EA319-6EE2-4701-BD47-8DDBF425BBE5} 2
                    batteryKey.fmtid.Data1 = 0x104EA319;
                    batteryKey.fmtid.Data2 = 0x6EE2;
                    batteryKey.fmtid.Data3 = 0x4701;
                    batteryKey.fmtid.Data4[0] = 0xBD;
                    batteryKey.fmtid.Data4[1] = 0x47;
                    batteryKey.fmtid.Data4[2] = 0x8D;
                    batteryKey.fmtid.Data4[3] = 0xDB;
                    batteryKey.fmtid.Data4[4] = 0xF4;
                    batteryKey.fmtid.Data4[5] = 0x25;
                    batteryKey.fmtid.Data4[6] = 0xBB;
                    batteryKey.fmtid.Data4[7] = 0xE5;
                    batteryKey.pid = 2;

                    DEVPROPTYPE propertyType;
                    BYTE buffer[256];
                    ULONG bufferSize = sizeof(buffer);

                    CONFIGRET cr = CM_Get_DevNode_Property(deviceInfoData.DevInst, &batteryKey,
                                                           &propertyType, buffer, &bufferSize, 0);

                    if (cr == CR_SUCCESS && propertyType == DEVPROP_TYPE_BYTE && bufferSize > 0) {
                        int batteryLevel = static_cast<int>(buffer[0]);

                        if (batteryLevel >= 0 && batteryLevel <= 100) {
                            batteryInfo.hasBatteryInfo = true;
                            batteryInfo.batteryLevel = batteryLevel;
                            batteryInfo.batteryStatus = "AVAILABLE";
                            if (batteryLevel <= 15) {
                                batteryInfo.batteryStatus = "CRITICAL";
                            }

                            SetupDiDestroyDeviceInfoList(deviceInfoSet);
                            if (comInitialized) CoUninitialize();
                            return true;
                        }
                    }
                    break;
                }
            }
        }

        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        if (comInitialized) CoUninitialize();

        return false;
    }
    catch (const std::exception& e) {
        qWarning() << "BluetoothBatteryMonitor: Exception in getBatteryFromHfpDevice:" << e.what();
        return false;
    }
    catch (...) {
        qWarning() << "BluetoothBatteryMonitor: Unknown exception in getBatteryFromHfpDevice";
        return false;
    }
}

// Enhanced caching with better device filtering
void BluetoothBatteryWorker::cacheDeviceInstanceIds()
{
    try {
        HDEVINFO deviceInfoSet = SetupDiGetClassDevs(
            nullptr,
            nullptr,  // Don't filter by enumerator - get all devices
            nullptr,
            DIGCF_PRESENT | DIGCF_ALLCLASSES
            );

        if (deviceInfoSet == INVALID_HANDLE_VALUE) {
            qWarning() << "BluetoothBatteryMonitor: Failed to get device info set";
            return;
        }

        SP_DEVINFO_DATA deviceInfoData;
        deviceInfoData.cbSize = sizeof(deviceInfoData);

        QMutexLocker locker(&m_devicesMutex);
        m_deviceInstanceCache.clear();

        for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
            WCHAR deviceDesc[512] = {0};
            WCHAR instanceId[512] = {0};

            if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC,
                                                 nullptr, (PBYTE)deviceDesc, sizeof(deviceDesc), nullptr) &&
                SetupDiGetDeviceInstanceId(deviceInfoSet, &deviceInfoData, instanceId,
                                           sizeof(instanceId)/sizeof(WCHAR), nullptr)) {

                QString deviceName = QString::fromWCharArray(deviceDesc);
                QString instanceIdStr = QString::fromWCharArray(instanceId);

                // Cache ALL devices that might be related to Bluetooth
                if (instanceIdStr.contains("BTHENUM", Qt::CaseInsensitive) ||
                    instanceIdStr.contains("Bluetooth", Qt::CaseInsensitive) ||
                    deviceName.contains("Bluetooth", Qt::CaseInsensitive)) {

                    m_deviceInstanceCache[deviceName] = instanceIdStr;
                }
            }
        }

        SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }
    catch (const std::exception& e) {
        qWarning() << "BluetoothBatteryMonitor: Exception caching device IDs:" << e.what();
    }
    catch (...) {
        qWarning() << "BluetoothBatteryMonitor: Unknown exception caching device IDs";
    }
}

QString BluetoothBatteryWorker::getDeviceTypeFromClassOfDevice(uint32_t classOfDevice)
{
    // Major device class is bits 8-12 (5 bits)
    uint32_t majorClass = (classOfDevice >> 8) & 0x1F;

    // Minor device class is bits 2-7 (6 bits)
    uint32_t minorClass = (classOfDevice >> 2) & 0x3F;

    switch (majorClass) {
    case 0x04: // Audio/Video
        switch (minorClass) {
        case 0x01: // Wearable Headset Device
        case 0x02: // Hands-free Device
        case 0x06: // Headphones
            return "Headphones";
        case 0x14: // Loudspeaker
            return "Speaker";
        case 0x18: // Gaming/VR Headset
            return "Headphones";
        default:
            return "Audio Device";
        }
    case 0x05: // Peripheral (HID)
        switch (minorClass & 0x3C) {
        case 0x10: // Keyboard
            return "Keyboard";
        case 0x20: // Mouse
            return "Mouse";
        case 0x30: // Combo keyboard/mouse
            return "Keyboard";
        default:
            return "Input Device";
        }
    case 0x02: // Phone
        return "Phone";
    default:
        return "Unknown";
    }
}

bool BluetoothBatteryWorker::isAudioDevice(uint32_t classOfDevice)
{
    uint32_t majorClass = (classOfDevice >> 8) & 0x1F;
    return majorClass == 0x04; // Audio/Video
}

bool BluetoothBatteryWorker::isInputDevice(uint32_t classOfDevice)
{
    uint32_t majorClass = (classOfDevice >> 8) & 0x1F;
    return majorClass == 0x05; // Peripheral (HID)
}

QString BluetoothBatteryWorker::bluetoothAddressToString(uint64_t address)
{
    return QString("%1:%2:%3:%4:%5:%6")
    .arg((address >> 40) & 0xFF, 2, 16, QChar('0'))
        .arg((address >> 32) & 0xFF, 2, 16, QChar('0'))
        .arg((address >> 24) & 0xFF, 2, 16, QChar('0'))
        .arg((address >> 16) & 0xFF, 2, 16, QChar('0'))
        .arg((address >> 8) & 0xFF, 2, 16, QChar('0'))
        .arg(address & 0xFF, 2, 16, QChar('0'))
        .toUpper();
}

void BluetoothBatteryWorker::onScanTimer()
{
    scanForDevices();
}

void BluetoothBatteryWorker::onBatteryUpdateTimer()
{
    updateDeviceBatteries();
}

//=============================================================================
// BluetoothBatteryMonitor Implementation
//=============================================================================

BluetoothBatteryMonitor::BluetoothBatteryMonitor(QObject *parent)
    : QObject(parent)
    , m_isMonitoring(false)
{
    // Create worker thread
    m_workerThread = new QThread(this);
    m_worker = new BluetoothBatteryWorker();
    m_worker->moveToThread(m_workerThread);

    // Connect worker signals
    connect(m_worker, &BluetoothBatteryWorker::devicesUpdated,
            this, &BluetoothBatteryMonitor::onWorkerDevicesUpdated);
    connect(m_worker, &BluetoothBatteryWorker::deviceBatteryChanged,
            this, &BluetoothBatteryMonitor::onWorkerDeviceBatteryChanged);

    // Connect thread lifecycle
    connect(m_workerThread, &QThread::started, m_worker, &BluetoothBatteryWorker::startMonitoring);
    connect(this, &BluetoothBatteryMonitor::destroyed, m_worker, &BluetoothBatteryWorker::stopMonitoring);
    connect(this, &BluetoothBatteryMonitor::destroyed, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &BluetoothBatteryWorker::deleteLater);

    // Load cached devices immediately for instant UI population
    loadCachedDevicesFromWorker();

    m_workerThread->start();
}

BluetoothBatteryMonitor::~BluetoothBatteryMonitor()
{
    if (m_workerThread->isRunning()) {
        m_workerThread->quit();
        if (!m_workerThread->wait(5000)) {
            m_workerThread->terminate();
            m_workerThread->wait();
        }
    }

    if (m_instance == this) {
        m_instance = nullptr;
    }
}

void BluetoothBatteryMonitor::loadCachedDevicesFromWorker()
{
    // Load cached devices from settings for instant UI population
    QSettings settings("Odizinne", "QontrolPanel");

    int size = settings.beginReadArray("bluetoothDevices");
    QList<BluetoothDeviceBattery> cachedDevices;

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);

        BluetoothDeviceBattery device;
        device.deviceId = settings.value("deviceId").toString();
        device.deviceName = settings.value("deviceName").toString();
        device.macAddress = settings.value("macAddress").toString();
        device.vendorId = settings.value("vendorId").toString();
        device.productId = settings.value("productId").toString();
        device.isConnected = settings.value("isConnected", false).toBool();
        device.hasBatteryInfo = settings.value("hasBatteryInfo", false).toBool();
        device.batteryLevel = settings.value("batteryLevel", -1).toInt();
        device.batteryStatus = settings.value("batteryStatus", "UNAVAILABLE").toString();
        device.deviceType = settings.value("deviceType", "Unknown").toString();
        device.lastUpdated = settings.value("lastUpdated", 0).toLongLong();

        if (!device.deviceId.isEmpty() && !device.macAddress.isEmpty()) {
            cachedDevices.append(device);
        }
    }
    settings.endArray();

    if (!cachedDevices.isEmpty()) {
        m_devices = cachedDevices;
        emit devicesChanged();
    }
}

BluetoothBatteryMonitor* BluetoothBatteryMonitor::instance()
{
    if (!m_instance) {
        m_instance = new BluetoothBatteryMonitor();
    }
    return m_instance;
}

BluetoothBatteryMonitor* BluetoothBatteryMonitor::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return instance();
}

void BluetoothBatteryMonitor::startMonitoring()
{
    if (m_isMonitoring) {
        return;
    }

    m_isMonitoring = true;
    emit monitoringStateChanged();

    QMetaObject::invokeMethod(m_worker, "startMonitoring", Qt::QueuedConnection);
}

void BluetoothBatteryMonitor::stopMonitoring()
{
    if (!m_isMonitoring) {
        return;
    }

    m_isMonitoring = false;
    emit monitoringStateChanged();

    QMetaObject::invokeMethod(m_worker, "stopMonitoring", Qt::QueuedConnection);
}

void BluetoothBatteryMonitor::refreshDevices()
{
    QMetaObject::invokeMethod(m_worker, "scanForDevices", Qt::QueuedConnection);
}

BluetoothDeviceBattery BluetoothBatteryMonitor::getDeviceBatteryByMac(const QString& macAddress) const
{
    QString normalizedMac = macAddress.toUpper().replace("-", ":");

    for (const auto& device : m_devices) {
        if (device.macAddress.compare(normalizedMac, Qt::CaseInsensitive) == 0) {
            return device;
        }
    }
    return BluetoothDeviceBattery();
}

bool BluetoothBatteryMonitor::hasDeviceBatteryInfo(const QString& macAddress) const
{
    auto device = getDeviceBatteryByMac(macAddress);
    return device.hasBatteryInfo && device.batteryLevel >= 0;
}

void BluetoothBatteryMonitor::onWorkerDevicesUpdated(const QList<BluetoothDeviceBattery>& devices)
{
    m_devices = devices;
    emit devicesChanged();
}

void BluetoothBatteryMonitor::onWorkerDeviceBatteryChanged(const BluetoothDeviceBattery& device)
{
    // Update specific device in list
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i].deviceId == device.deviceId) {
            m_devices[i] = device;
            break;
        }
    }

    emit deviceBatteryUpdated(device);
    emit devicesChanged();
}
