#ifndef MEDIASFLYOUT_H
#define MEDIASFLYOUT_H

#include <QWidget>
#include <QThread>
#include <QTimer>
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
    void animateIn(QRect trayIconGeometry, int panelHeight);
    void animateOut(QRect trayIconGeometry);
    void updateUi(MediaSession session);

public slots:
    void onPrevClicked();
    void onNextClicked();
    void onPauseClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::MediaFlyout *ui;
    QColor borderColor;
    QPixmap roundPixmap(const QPixmap &src, int radius);

signals:
    void requestPrev();
    void requestNext();
    void requestPause();
};

#endif // MEDIASFLYOUT_H
