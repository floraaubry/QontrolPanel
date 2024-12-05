#include "Panel.h"
#include <QQmlContext>
#include <QQuickItem>
#include <QList>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QPalette>
#include <QApplication>

Panel::Panel(QObject *parent, MediaFlyout* mediaFlyout)
    : QObject(parent)
    , isAnimating(false)
    , visible(false)
    , m_mediaFlyout(mediaFlyout)
{
    engine = new QQmlApplicationEngine(this);
    engine->rootContext()->setContextProperty("panel", this);

    QColor windowColor = QGuiApplication::palette().color(QPalette::Window);
    // Expose the native window color to QML
    engine->rootContext()->setContextProperty("nativeWindowColor", windowColor);

    engine->load(QUrl(QStringLiteral("qrc:/qml/Panel.qml")));

    panelWindow = qobject_cast<QWindow*>(engine->rootObjects().first());
}

Panel::~Panel()
{
}

void Panel::setDynamicMask()
{
    QRect windowRect = this->panelWindow->geometry();
    QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();

    availableGeometry.adjust(0, 0, -12, 0);
    QRect visiblePart = windowRect.intersected(availableGeometry);

    if (!visiblePart.isEmpty()) {
        QRegion mask(visiblePart.translated(-windowRect.topLeft()));
        this->panelWindow->setMask(mask);
        m_mediaFlyout->mediaFlyoutWindow->setMask(mask);
        this->panelWindow->setVisible(true);
        m_mediaFlyout->mediaFlyoutWindow->setVisible(true);
    } else {
        this->panelWindow->setVisible(false);
        m_mediaFlyout->mediaFlyoutWindow->setVisible(false);
    }
}

void Panel::animateIn()
{
    isAnimating = true;

    QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();
    int margin = 12;
    int startX = availableGeometry.right();
    int targetX = availableGeometry.right() - this->panelWindow->width() - margin;
    int panelY = availableGeometry.bottom() - margin - this->panelWindow->height();
    int flyoutY = availableGeometry.bottom() - margin - this->panelWindow->height() - margin - m_mediaFlyout->mediaFlyoutWindow->height();

    this->panelWindow->setPosition(startX, panelY);
    m_mediaFlyout->mediaFlyoutWindow->setPosition(startX, flyoutY);

    const int durationMs = 60;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;
    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);
    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        double t = static_cast<double>(currentStep) / totalSteps;
        double easedT = 1 - pow(1 - t, 3);
        int currentX = startX + easedT * (targetX - startX);

        if (currentX == targetX) {
            animationTimer->stop();
            animationTimer->deleteLater();
            visible = true;
            isAnimating = false;
            return;
        }
        setDynamicMask();
        this->panelWindow->setPosition(currentX, panelY);
        m_mediaFlyout->mediaFlyoutWindow->setPosition(currentX, flyoutY);
        ++currentStep;
    });
}

void Panel::animateOut()
{
    if (isAnimating || !visible)
        return;

    isAnimating = true;

    QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();
    int startX = this->panelWindow->x();
    int targetX = availableGeometry.right();
    int panelY = this->panelWindow->y();
    int flyoutY = m_mediaFlyout->mediaFlyoutWindow->y();

    const int durationMs = 60;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;
    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);
    connect(animationTimer, &QTimer::timeout, this, [=]() mutable {
        double t = static_cast<double>(currentStep) / totalSteps;
        double easedT = 1 - pow(1 - t, 3);
        int currentX = startX + easedT * (targetX - startX);

        if (currentX == targetX) {
            animationTimer->stop();
            animationTimer->deleteLater();
            this->panelWindow->setVisible(false);
            m_mediaFlyout->mediaFlyoutWindow->setVisible(false);
            visible = false;
            isAnimating = false;
            return;
        }
        setDynamicMask();
        this->panelWindow->setPosition(currentX, panelY);
        m_mediaFlyout->mediaFlyoutWindow->setPosition(currentX, flyoutY);
        ++currentStep;
    });
}

