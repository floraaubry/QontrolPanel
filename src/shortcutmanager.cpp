#include "shortcutmanager.h"
#include <QCoreApplication>
#include <QSettings>
#include <QDir>

void ShortcutManager::manageShortcut(bool state, QString shortcutName)
{
    QSettings registry("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);

    if (state) {
        QString applicationPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        registry.setValue(shortcutName, applicationPath);
    } else {
        registry.remove(shortcutName);
    }
}

bool ShortcutManager::isShortcutPresent(QString shortcutName)
{
    QSettings registry("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);

    return registry.contains(shortcutName);
}
