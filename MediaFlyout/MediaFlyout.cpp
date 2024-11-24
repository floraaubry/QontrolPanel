#include "MediaFlyout.h"
#include "ui_MediaFlyout.h"
#include "Utils.h"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>

MediaFlyout::MediaFlyout(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::MediaFlyout)
    , workerThread(nullptr)
    , worker(nullptr)
    , mediaSessionTimer(nullptr)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus);
    this->setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(width());
    borderColor = Utils::getTheme() == "light" ? QColor(255, 255, 255, 32) : QColor(0, 0, 0, 52);
}

MediaFlyout::~MediaFlyout()
{
    stopMonitoringMediaSession();
    delete ui;
}

void MediaFlyout::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.setBrush(this->palette().color(QPalette::Window));
    painter.setPen(Qt::NoPen);

    QPainterPath path;
    path.addRoundedRect(this->rect().adjusted(1, 1, -1, -1), 8, 8);
    painter.drawPath(path);

    QPen borderPen(borderColor);
    borderPen.setWidth(1);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);

    QPainterPath borderPath;
    borderPath.addRoundedRect(this->rect().adjusted(0, 0, -1, -1), 8, 8);
    painter.drawPath(borderPath);
}

void MediaFlyout::animateIn(QRect trayIconGeometry, int panelHeight)
{
    QPoint trayIconPos = trayIconGeometry.topLeft();
    int trayIconCenterX = trayIconPos.x() + trayIconGeometry.width() / 2;

    int panelX = trayIconCenterX - this->width() / 2;
    QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
    int startY = screenGeometry.bottom();
    int targetY = trayIconGeometry.top() - this->height() - 12 - 12 - panelHeight;

    this->move(panelX, startY);
    this->show();

    const int durationMs = 300;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);

    connect(animationTimer, &QTimer::timeout, this, [=, this]() mutable {
        if (currentStep >= totalSteps) {
            animationTimer->stop();
            animationTimer->deleteLater();
            this->move(panelX, targetY); // Ensure final position is set
            return;
        }

        double t = static_cast<double>(currentStep) / totalSteps; // Normalized time (0 to 1)
        // Easing function: Smooth deceleration
        double easedT = 1 - pow(1 - t, 3);

        // Interpolated Y position
        int currentY = startY + easedT * (targetY - startY);
        this->move(panelX, currentY);

        ++currentStep;
    });
}

void MediaFlyout::animateOut(QRect trayIconGeometry)
{
    QPoint trayIconPos = trayIconGeometry.topLeft();
    int trayIconCenterX = trayIconPos.x() + trayIconGeometry.width() / 2;

    int panelX = trayIconCenterX - this->width() / 2; // Center horizontally
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int startY = this->y();
    int targetY = screenGeometry.bottom(); // Move to the bottom of the screen

    const int durationMs = 300;
    const int refreshRate = 1;
    const double totalSteps = durationMs / refreshRate;

    int currentStep = 0;
    QTimer *animationTimer = new QTimer(this);

    animationTimer->start(refreshRate);

    connect(animationTimer, &QTimer::timeout, this, [=, this]() mutable {
        if (currentStep >= totalSteps) {
            animationTimer->stop();
            animationTimer->deleteLater();

            return;
        }

        double t = static_cast<double>(currentStep) / totalSteps; // Normalized time (0 to 1)
        // Easing function: Smooth deceleration
        double easedT = 1 - pow(1 - t, 3);

        // Interpolated Y position
        int currentY = startY + easedT * (targetY - startY);
        this->move(panelX, currentY);

        ++currentStep;
    });
}

void MediaFlyout::startMonitoringMediaSession()
{
    if (!mediaSessionTimer) {
        mediaSessionTimer = new QTimer(this);
        connect(mediaSessionTimer, &QTimer::timeout, this, &MediaFlyout::getMediaSession);
        mediaSessionTimer->start(1000);  // 1000 milliseconds = 1 second
        qDebug() << "Started media session monitoring.";
    }
}

void MediaFlyout::stopMonitoringMediaSession()
{
    // Stop the timer and delete it if it's not null
    if (mediaSessionTimer) {
        mediaSessionTimer->stop();
        delete mediaSessionTimer;  // Timer will be deleted
        mediaSessionTimer = nullptr;  // Set to null to avoid dangling pointer
        qDebug() << "Stopped media session monitoring.";
    }
}

void MediaFlyout::getMediaSession()
{
    // Create a new worker thread and worker object every second
    workerThread = new QThread(this);
    worker = new MediaSessionWorker();

    // Move worker to a separate thread
    worker->moveToThread(workerThread);

    // Connect signals and slots
    connect(workerThread, &QThread::started, worker, &MediaSessionWorker::process);
    connect(worker, &MediaSessionWorker::sessionReady, this, &MediaFlyout::onSessionReady);
    connect(worker, &MediaSessionWorker::sessionError, this, &MediaFlyout::onSessionError);
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);

    // Start the worker thread
    workerThread->start();
}

void MediaFlyout::onSessionReady(const MediaSession& session)
{
    workerThread->quit();
    workerThread->wait();

    if (!this->isVisible()) {
        emit sessionActive();
    }

    updateUi(session);
}

void MediaFlyout::onSessionError(const QString& error)
{
    workerThread->quit();
    workerThread->wait();

    emit sessionInactive();
}

void MediaFlyout::updateUi(MediaSession session)
{
    ui->title->setText(session.title);
    ui->artist->setText(session.artist);
    ui->prev->setEnabled(session.canGoPrevious);
    ui->next->setEnabled(session.canGoNext);
}
