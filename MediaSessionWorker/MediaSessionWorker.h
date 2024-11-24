#ifndef MEDIASESSIONWORKER_H
#define MEDIASESSIONWORKER_H

#include <QObject>
#include "MediaSession.h"

class MediaSessionWorker : public QObject
{
    Q_OBJECT

public:
    explicit MediaSessionWorker(QObject* parent = nullptr);
    ~MediaSessionWorker();
    void process();

signals:
    void sessionReady(const MediaSession& session);
    void sessionError(const QString& error);
};

#endif // MEDIASESSIONWORKER_H
