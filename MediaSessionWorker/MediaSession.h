#ifndef MEDIASESSION_H
#define MEDIASESSION_H

#include <QString>

struct MediaSession
{
    QString title;
    QString artist;
    QString playbackState;
    bool canGoNext;
    bool canGoPrevious;
    int duration;
    int currentTime;
};

#endif // MEDIASESSION_H
