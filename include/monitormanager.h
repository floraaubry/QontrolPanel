#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMetaObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

// Forward declare the implementation class to avoid Windows headers in Qt code
class MonitorManagerImpl;

struct Monitor {
    QString id;
    QString name;
    QString friendlyName;
    int brightness;
    int minBrightness;
    int maxBrightness;
    bool isSupported;
    bool isLaptopDisplay;

    Monitor() : brightness(50), minBrightness(0), maxBrightness(100),
        isSupported(false), isLaptopDisplay(false) {}
};

class MonitorWorker : public QObject
{
    Q_OBJECT

public:
    explicit MonitorWorker(QObject *parent = nullptr);
    ~MonitorWorker();

public slots:
    void enumerateMonitors();
    void setBrightness(const QString& monitorId, int brightness);
    void setBrightnessAll(int brightness);
    void refreshBrightnessLevels();
    void checkNightLightStatus();
    void setNightLight(bool enabled);
    void toggleNightLight();
    void setDDCCIBrightness(int brightness);
    void setWMIBrightness(int brightness);

signals:
    void monitorsReady(const QList<Monitor>& monitors);
    void brightnessChanged(const QString& monitorId, int brightness);
    void monitorBrightnessLevelsReady(const QList<QPair<QString, int>>& levels);
    void nightLightStatusReady(bool supported, bool enabled);
    void nightLightChanged(bool enabled);
    void ddcciBrightnessChanged(int brightness);
    void wmiBrightnessChanged(int brightness);

private:
    MonitorManagerImpl* m_impl;
    QList<Monitor> m_monitors;
    QMutex m_monitorsMutex;

    void updateMonitorFromImpl();
};

class MonitorManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool monitorDetected READ monitorDetected NOTIFY monitorDetectedChanged)
    Q_PROPERTY(int brightness READ brightness NOTIFY brightnessChanged)
    Q_PROPERTY(bool nightLightEnabled READ nightLightEnabled NOTIFY nightLightEnabledChanged)
    Q_PROPERTY(bool nightLightSupported READ nightLightSupported NOTIFY nightLightSupportedChanged)

public:
    static void initialize();
    static void cleanup();
    static MonitorWorker* getWorker();
    static MonitorManager* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
    static MonitorManager* instance();

    // Async methods (thread-safe)
    static void enumerateMonitorsAsync();
    static void setBrightnessAsync(const QString& monitorId, int brightness);
    static void setBrightnessAllAsync(int brightness);
    static void refreshBrightnessLevelsAsync();
    static void checkNightLightAsync();
    static void setNightLightAsync(bool enabled);
    static void toggleNightLightAsync();

    static void setDDCCIBrightnessAsync(int brightness);
    static void setWMIBrightnessAsync(int brightness);

    // Cached getters (thread-safe, non-blocking)
    static QList<Monitor> getMonitors();
    static int getMonitorBrightness(const QString& monitorId);
    static bool getNightLightEnabled();
    static bool getNightLightSupported();

    // QML Properties
    bool monitorDetected() const;
    int brightness() const;
    bool nightLightEnabled() const;
    bool nightLightSupported() const;

    // QML Methods
    Q_INVOKABLE void setBrightness(int value);
    Q_INVOKABLE void refreshMonitors();
    Q_INVOKABLE void setNightLightEnabled(bool enabled);
    Q_INVOKABLE void toggleNightLight();

    Q_INVOKABLE void setDDCCIBrightness(int brightness);
    Q_INVOKABLE void setWMIBrightness(int brightness);

signals:
    void monitorDetectedChanged();
    void brightnessChanged();
    void nightLightEnabledChanged();
    void nightLightSupportedChanged();
    void monitorsChanged(const QList<Monitor>& monitors);

private:
    explicit MonitorManager(QObject *parent = nullptr);
    ~MonitorManager();

    static MonitorWorker* s_worker;
    static QThread* s_workerThread;
    static MonitorManager* s_instance;

    // Cached data
    static QList<Monitor> s_cachedMonitors;
    static bool s_nightLightEnabled;
    static bool s_nightLightSupported;
    static QMutex s_cacheMutex;

    // Instance properties
    bool m_monitorDetected;
    int m_currentBrightness;
    bool m_nightLightEnabled;
    bool m_nightLightSupported;

    static void updateCache(const QList<Monitor>& monitors);
    static void updateNightLightCache(bool supported, bool enabled);
};
