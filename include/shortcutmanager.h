#pragma once

#include <QString>

namespace ShortcutManager {

    bool isShortcutPresent(QString shortcutName);
    void manageShortcut(bool state, QString shortcutName);
}
