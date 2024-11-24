#ifndef MEDIASESSION_H
#define MEDIASESSION_H

#include <QString>
#include <QIcon>

struct MediaSession
{
    QString title;
    QString artist;
    QString playbackState;
    bool canGoNext;
    bool canGoPrevious;
    int duration;
    int currentTime;
    QIcon icon;
};

#endif // MEDIASESSION_H
