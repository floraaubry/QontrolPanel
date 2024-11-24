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
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowDoesNotAcceptFocus);
    this->setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(width());
    borderColor = Utils::getTheme() == "light" ? QColor(255, 255, 255, 32) : QColor(0, 0, 0, 52);

    ui->prev->setIcon(Utils::getButtonsIcon("prev"));
    ui->next->setIcon(Utils::getButtonsIcon("next"));
    ui->pause->setIcon(Utils::getButtonsIcon("pause"));

    connect(ui->next, &QToolButton::clicked, this, &MediaFlyout::onNextClicked);
    connect(ui->prev, &QToolButton::clicked, this, &MediaFlyout::onPrevClicked);
    connect(ui->pause, &QToolButton::clicked, this, &MediaFlyout::onPauseClicked);
}

MediaFlyout::~MediaFlyout()
{
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

void MediaFlyout::updateUi(MediaSession session)
{
    // Enable word wrap for the title QLabel
    ui->title->setWordWrap(true);

    // Limit the title to 2 lines
    QString fullTitle = session.title;
    QFontMetrics metrics(ui->title->font());
    int maxLines = 2;
    int lineHeight = metrics.lineSpacing(); // Height of a single line of text
    int maxHeight = maxLines * lineHeight;  // Maximum height for 2 lines

    // Calculate text that fits within the QLabel's width and max height
    QString truncatedTitle;
    QStringList words = fullTitle.split(' '); // Split title into words for manual wrapping
    QString currentLine;
    int currentHeight = 0;

    for (const QString& word : words) {
        // Test appending the word to the current line
        QString testLine = currentLine.isEmpty() ? word : currentLine + ' ' + word;

        // Check if the current line exceeds the QLabel's width
        if (metrics.horizontalAdvance(testLine) > ui->title->width()) {
            // Line is too wide, add it as a completed line
            if (!truncatedTitle.isEmpty()) {
                truncatedTitle += '\n';
            }
            truncatedTitle += currentLine;
            currentHeight += lineHeight;

            // Start a new line with the current word
            currentLine = word;

            // Stop if we've reached the maximum number of lines
            if (currentHeight >= maxHeight) {
                truncatedTitle.chop(3); // Remove last characters if needed
                truncatedTitle += "..."; // Add ellipsis
                break;
            }
        } else {
            // Line is still within width, keep building it
            currentLine = testLine;
        }
    }

    // Add the last line, if there's space left
    if (currentHeight < maxHeight && !currentLine.isEmpty()) {
        if (!truncatedTitle.isEmpty()) {
            truncatedTitle += '\n';
        }
        truncatedTitle += currentLine;
    }

    // Set the processed title text
    ui->title->setText(truncatedTitle);

    // Update other UI elements
    ui->artist->setText(session.artist);
    ui->prev->setEnabled(session.canGoPrevious);
    ui->next->setEnabled(session.canGoNext);
    ui->pause->setEnabled(true);

    // Update media icon
    QPixmap originalIcon = session.icon.pixmap(64, 64);
    QPixmap roundedIcon = roundPixmap(originalIcon, 8);
    ui->icon->setPixmap(roundedIcon);

    // Set play/pause icon
    QString playPause;
    if (session.playbackState == "Playing") {
        playPause = "pause";
    } else {
        playPause = "play";
    }
    ui->pause->setIcon(Utils::getButtonsIcon(playPause));

    // Adjust size of the widget
    this->adjustSize();
}

QPixmap MediaFlyout::roundPixmap(const QPixmap &src, int radius) {
    if (src.isNull()) {
        return QPixmap();
    }

    QPixmap dest(src.size());
    dest.fill(Qt::transparent);

    QPainter painter(&dest);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath path;
    path.addRoundedRect(QRectF(0, 0, src.width(), src.height()), radius, radius);

    painter.setClipPath(path);
    painter.drawPixmap(0, 0, src);
    painter.end();

    return dest;
}

void MediaFlyout::onPrevClicked()
{
    emit requestPrev();
    ui->next->setEnabled(false);
    ui->prev->setEnabled(false);
    ui->pause->setEnabled(false);
}

void MediaFlyout::onNextClicked()
{
    emit requestNext();
    ui->next->setEnabled(false);
    ui->prev->setEnabled(false);
    ui->pause->setEnabled(false);
}

void MediaFlyout::onPauseClicked()
{
    emit requestPause();
    ui->next->setEnabled(false);
    ui->prev->setEnabled(false);
    ui->pause->setEnabled(false);
}

