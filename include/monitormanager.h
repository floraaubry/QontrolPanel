#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

// Forward declare the implementation class to avoid Windows headers in Qt code
class MonitorManagerImpl;

class MonitorManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool monitorDetected READ monitorDetected NOTIFY monitorDetectedChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)

public:
    explicit MonitorManager(QObject *parent = nullptr);
    ~MonitorManager();

    static MonitorManager* instance();
    static MonitorManager* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    // QML Properties
    bool monitorDetected() const;
    int brightness() const;

    // QML Methods
    Q_INVOKABLE void setBrightness(int value);
    Q_INVOKABLE void refreshMonitors();

signals:
    void monitorDetectedChanged();
    void brightnessChanged();

private:
    static MonitorManager* m_instance;
    MonitorManagerImpl* m_impl;  // Pointer to implementation
    bool m_monitorDetected;
    int m_currentBrightness;

    void updateMonitorDetection();
    void retrieveCurrentBrightness();
};
