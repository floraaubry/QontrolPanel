#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include <QFuture>
#include <QMutex>
#include <QAtomicInt>

// Forward declare the implementation class to avoid Windows headers in Qt code
class MonitorManagerImpl;

class MonitorManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool monitorDetected READ monitorDetected NOTIFY monitorDetectedChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(bool isUpdating READ isUpdating NOTIFY isUpdatingChanged)
    Q_PROPERTY(bool nightLightEnabled READ nightLightEnabled WRITE setNightLightEnabled NOTIFY nightLightEnabledChanged)
    Q_PROPERTY(bool nightLightSupported READ nightLightSupported NOTIFY nightLightSupportedChanged)

public:
    explicit MonitorManager(QObject *parent = nullptr);
    ~MonitorManager();

    static MonitorManager* instance();
    static MonitorManager* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    // QML Properties
    bool monitorDetected() const;
    int brightness() const;
    bool isUpdating() const;
    bool nightLightEnabled() const;
    bool nightLightSupported() const;

    // QML Methods
    Q_INVOKABLE void setBrightness(int value);
    Q_INVOKABLE void refreshMonitors();
    Q_INVOKABLE void setNightLightEnabled(bool enabled);
    Q_INVOKABLE void toggleNightLight();

signals:
    void monitorDetectedChanged();
    void brightnessChanged();
    void isUpdatingChanged();
    void nightLightEnabledChanged();
    void nightLightSupportedChanged();

private slots:
    void onMonitorDetectionComplete(bool hasMonitors, int currentBrightness);
    void onBrightnessUpdateComplete(bool hasMonitors, int actualBrightness);
    void onNightLightUpdateComplete(bool supported, bool enabled);

private:
    static MonitorManager* m_instance;
    MonitorManagerImpl* m_impl;  // Pointer to implementation
    bool m_monitorDetected;
    int m_currentBrightness;
    bool m_nightLightEnabled;
    bool m_nightLightSupported;

    QAtomicInt m_isUpdating;
    QMutex m_mutex;
    QFuture<void> m_currentOperation;

    void setIsUpdating(bool updating);
    void performMonitorDetectionAsync();
    void performBrightnessUpdateAsync(int value);
    void performNightLightUpdateAsync(bool enabled);
    void checkNightLightAsync();
};
