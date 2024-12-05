#ifndef PANEL_H
#define PANEL_H

#include "MediaFlyout.h"
#include <QObject>
#include <QQmlApplicationEngine>
#include <QWindow>

class Panel : public QObject
{
    Q_OBJECT

public:
    explicit Panel(QObject *parent = nullptr, MediaFlyout* mediaFlyout = nullptr);
    ~Panel() override;
    void animateIn();
    void animateOut();
    bool isAnimating;
    bool visible;
    QWindow* panelWindow;

private:
    void setDynamicMask();
    MediaFlyout* m_mediaFlyout;
    QQmlApplicationEngine* engine;

private slots:


signals:

};

#endif // PANEL_H
