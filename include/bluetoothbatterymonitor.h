#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QList>
#include <QString>
#include <QHash>
#include <QSettings>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

// Include Windows headers first, before WinRT
#include <windows.h>

// Use forward declarations instead of including WinRT headers in the header file
namespace winrt {
namespace Windows::Foundation {
template<typename T> struct IAsyncOperation;
struct IAsyncAction;
}
namespace Windows::Devices::Enumeration {
struct DeviceWatcher;
struct DeviceInformation;
struct DeviceInformationUpdate;
}
namespace Windows::Devices::Bluetooth {
struct BluetoothDevice;
struct BluetoothLEDevice;
}
namespace Windows::Devices::Bluetooth::GenericAttributeProfile {
struct GattDeviceService;
struct GattCharacteristic;
}
}

struct BluetoothDeviceBattery {
    QString deviceId;           // Windows device ID
    QString deviceName;
    QString macAddress;         // Primary identifier (XX:XX:XX:XX:XX:XX)
    QString vendorId;           // Bluetooth vendor ID (if available)
    QString productId;          // Bluetooth product ID (if available)
    bool isConnected;
    bool hasBatteryInfo;
    int batteryLevel;           // 0-100, -1 if unavailable
    QString batteryStatus;      // "AVAILABLE", "CHARGING", "UNAVAILABLE", "CRITICAL"
    QString deviceType;         // "Headphones", "Speaker", "Mouse", "Keyboard", "Unknown"
    qint64 lastUpdated;         // Timestamp

    BluetoothDeviceBattery() : isConnected(false), hasBatteryInfo(false),
        batteryLevel(-1), batteryStatus("UNAVAILABLE"), deviceType("Unknown"), lastUpdated(0) {}
};

Q_DECLARE_METATYPE(BluetoothDeviceBattery)

class BluetoothBatteryWorker : public QObject
{
    Q_OBJECT

public:
    BluetoothBatteryWorker();
    ~BluetoothBatteryWorker();

public slots:
    void startMonitoring();
    void stopMonitoring();
    void scanForDevices();
    void updateDeviceBatteries();

signals:
    void devicesUpdated(const QList<BluetoothDeviceBattery>& devices);
    void deviceBatteryChanged(const BluetoothDeviceBattery& device);

private slots:
    void onScanTimer();
    void onBatteryUpdateTimer();

private:
    // Device discovery and management
    void initializeBluetoothWatcher();
    void cleanupBluetoothWatcher();

    // Caching methods
    void loadCachedDevices();
    void saveCachedDevices();

    // Device identification
    QString extractMacAddressFromDeviceId(const QString& deviceId);
    QString extractBluetoothVendorId(const QString& deviceId);
    QString extractBluetoothProductId(const QString& deviceId);
    QString bluetoothAddressToString(uint64_t address);
    QString normalizeBluetoothAddress(const QString& address);

    // Battery level reading
    void readDeviceBatteryLevel(const QString& deviceId);
    bool findBatteryInfoEfficiently(BluetoothDeviceBattery& batteryInfo);
    bool findBatteryInPnpDevices(BluetoothDeviceBattery& batteryInfo);
    bool getBatteryFromHfpDevice(const QString& instanceId, BluetoothDeviceBattery& batteryInfo);
    bool checkBatteryProperty(const QString& instanceId, BluetoothDeviceBattery& batteryInfo);
    void cacheDeviceInstanceIds();

    QHash<QString, QString> m_deviceInstanceCache;

    // Utility functions
    QString getDeviceTypeFromClassOfDevice(uint32_t classOfDevice);
    bool isAudioDevice(uint32_t classOfDevice);
    bool isInputDevice(uint32_t classOfDevice);

    QTimer* m_scanTimer;
    QTimer* m_batteryUpdateTimer;
    QHash<QString, BluetoothDeviceBattery> m_devices;     // Keyed by deviceId
    QHash<QString, QString> m_macToDeviceId;              // MAC address to device ID mapping
    QMutex m_devicesMutex;
    QSettings m_settings;                                 // For persistent caching

    // WinRT objects - using void* to avoid header conflicts
    void* m_deviceWatcher;
    void* m_deviceAddedToken;
    void* m_deviceRemovedToken;
    void* m_deviceUpdatedToken;

    bool m_isMonitoring;
    bool m_isInitialized;

    static const int SCAN_INTERVAL_MS = 10000;           // 10 seconds
    static const int BATTERY_UPDATE_INTERVAL_MS = 30000; // 30 seconds
};

class BluetoothBatteryMonitor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isMonitoring READ isMonitoring NOTIFY monitoringStateChanged)
    Q_PROPERTY(QList<BluetoothDeviceBattery> devices READ devices NOTIFY devicesChanged)

public:
    explicit BluetoothBatteryMonitor(QObject *parent = nullptr);
    ~BluetoothBatteryMonitor();

    static BluetoothBatteryMonitor* instance();
    static BluetoothBatteryMonitor* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    Q_INVOKABLE void startMonitoring();
    Q_INVOKABLE void stopMonitoring();
    Q_INVOKABLE void refreshDevices();

    bool isMonitoring() const { return m_isMonitoring; }
    QList<BluetoothDeviceBattery> devices() const { return m_devices; }

    // Get battery info for specific device by MAC address
    Q_INVOKABLE BluetoothDeviceBattery getDeviceBatteryByMac(const QString& macAddress) const;

    // Check if device has battery info available by MAC address
    Q_INVOKABLE bool hasDeviceBatteryInfo(const QString& macAddress) const;

    // Get cached devices for audio manager integration
    QList<BluetoothDeviceBattery> getCachedDevices() const { return m_devices; }

signals:
    void devicesChanged();
    void monitoringStateChanged();
    void deviceBatteryUpdated(const BluetoothDeviceBattery& device);

private slots:
    void onWorkerDevicesUpdated(const QList<BluetoothDeviceBattery>& devices);
    void onWorkerDeviceBatteryChanged(const BluetoothDeviceBattery& device);

private:
    void loadCachedDevicesFromWorker();

    static BluetoothBatteryMonitor* m_instance;

    QThread* m_workerThread;
    BluetoothBatteryWorker* m_worker;

    QList<BluetoothDeviceBattery> m_devices;
    bool m_isMonitoring;
};
