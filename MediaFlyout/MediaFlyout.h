#ifndef MEDIASFLYOUT_H
#define MEDIASFLYOUT_H

#include <QWidget>
#include <QThread>
#include <QTimer>
#include "MediaSessionWorker.h"
#include "MediaSession.h"

namespace Ui {
class MediaFlyout;
}

class MediaFlyout : public QWidget
{
    Q_OBJECT

public:
    explicit MediaFlyout(QWidget* parent = nullptr);
    ~MediaFlyout() override;
    void startMonitoringMediaSession();
    void stopMonitoringMediaSession();
    void animateIn(QRect trayIconGeometry, int panelHeight);
    void animateOut(QRect trayIconGeometry);

public slots:
    void getMediaSession();
    void onPrevClicked();
    void onNextClicked();
    void onPauseClicked();

private slots:
    void onSessionReady(const MediaSession& session);
    void onSessionError(const QString& error);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::MediaFlyout *ui;
    QThread* workerThread;
    MediaSessionWorker* worker;
    QTimer* mediaSessionTimer;
    QColor borderColor;
    void updateUi(MediaSession session);
    QPixmap roundPixmap(const QPixmap &src, int radius);

signals:
    void sessionActive();
    void sessionInactive();
};

#endif // MEDIASFLYOUT_H
