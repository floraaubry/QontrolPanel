#include "SoundOverlay.h"
#include "ui_SoundOverlay.h"
#include <QTimer>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QPropertyAnimation>

SoundOverlay::SoundOverlay(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SoundOverlay)
    , shown(false)
    , animationTimerOut(nullptr)
    , isAnimatingOut(false)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    expireTimer = new QTimer(this);
    connect(expireTimer, &QTimer::timeout, this, &SoundOverlay::animateOut);
}

SoundOverlay::~SoundOverlay()
{
    delete ui;
}

void SoundOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.setBrush(this->palette().color(QPalette::Window));
    painter.setPen(Qt::NoPen);

    QPainterPath path;
    path.addRoundedRect(this->rect().adjusted(1, 1, -1, -1), 8, 8);
    painter.drawPath(path);

    QPen borderPen(QColor(0, 0, 0, 37));
    borderPen.setWidth(1);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);

    QPainterPath borderPath;
    borderPath.addRoundedRect(this->rect().adjusted(0, 0, -1, -1), 8, 8);
    painter.drawPath(borderPath);
}

void SoundOverlay::animateIn()
{
    if (isAnimatingOut) return;

    if (shown) {
        expireTimer->start(3000);
        return;
    }

    shown = true;

    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    QRect availableScreenGeometry = QApplication::primaryScreen()->availableGeometry();
    int taskbarHeight = screenGeometry.height() - availableScreenGeometry.height();
    int screenCenterX = screenGeometry.center().x();
    int margin = 12;
    int soundOverlayX = screenCenterX - this->width() / 2;
    int targetY = screenGeometry.bottom() - this->height() - taskbarHeight - margin;

    this->move(soundOverlayX, targetY);

    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(300);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::Linear);

    this->setWindowOpacity(0.0);
    this->show();

    QObject::connect(animation, &QPropertyAnimation::finished, this, [=, this]() {
        animation->deleteLater();
        expireTimer->start(3000);
    });

    animation->start();
}

void SoundOverlay::animateOut()
{
    shown = false;
    isAnimatingOut = true;

    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(300);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->setEasingCurve(QEasingCurve::Linear);

    QObject::connect(animation, &QPropertyAnimation::finished, this, [=, this]() {
        animation->deleteLater();
        isAnimatingOut = false;
        emit overlayClosed();
    });

    animation->start();
}

void SoundOverlay::updateVolumeIconAndLabel(QIcon icon, int volume)
{
    ui->iconLabel->setPixmap(icon.pixmap(16, 16));
    ui->valueLabel->setText(QString::number(volume));
    ui->progressBar->setValue(volume);
}

void SoundOverlay::updateMuteIcon(QIcon icon)
{
    ui->iconLabel->setPixmap(icon.pixmap(16, 16));
}

