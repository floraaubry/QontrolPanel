#ifndef UTILS_H
#define UTILS_H

#include <QIcon>
#include <QFrame>

namespace Utils {
    QIcon getIcon(int type, int volume, bool muted);
    void setFrameColorBasedOnWindow(QWidget *window, QFrame *frame);
    QColor adjustColor(const QColor &color, double factor);
    bool isDarkMode(const QColor &color);
    QIcon generateMutedIcon(QPixmap originalPixmap);
    QString getAccentColor(const QString &accentKey);
    QString getTheme();
    void playSoundNotification(bool enabled);
    QPixmap createIconWithAccentBackground();
    QIcon getPlaceholderIcon();
    QIcon getButtonsIcon(QString button);
    void sendPlayPauseKey();
    void sendNextKey();
    void sendPrevKey();
}

#endif // UTILS_H
