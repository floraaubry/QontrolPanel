#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include "headsetcontrolmanager.h"

class HeadsetControlBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit HeadsetControlBridge(QObject *parent = nullptr);
    ~HeadsetControlBridge() override;

    static HeadsetControlBridge* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
    static HeadsetControlBridge* instance();

    bool isMonitoring() const;

    Q_INVOKABLE void setMonitoringEnabled(bool enabled);

signals:
    void monitoringStateChanged();

private slots:
    void onMonitoringStateChanged(bool enabled);

private:
    static HeadsetControlBridge* m_instance;
    HeadsetControlManager* m_manager;
};
