#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QString>

namespace ShortcutManager {

    bool isShortcutPresent(QString shortcutName);
    void manageShortcut(bool state, QString shortcutName);
}

#endif // SHORTCUTMANAGER_H
