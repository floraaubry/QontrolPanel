#ifndef UTILS_H
#define UTILS_H

#include <QIcon>
#include <QFrame>

namespace Utils {
    QString getIcon(int type, int volume, bool muted);
    QString getAccentColor(const QString &accentKey);
    QString getTheme();
    void playSoundNotification();
    bool isWindows10();
}

#endif // UTILS_H
