#include "shortcutmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QShortcut>
#include <QStandardPaths>

void ShortcutManager::manageShortcut(bool state, QString shortcutName)
{
    QString applicationPath = QCoreApplication::applicationFilePath();
    QString startupPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + QDir::separator() + "Startup";
    QString shortcutPath = startupPath + QDir::separator() + shortcutName;

    if (state) {
        QFile::link(applicationPath, shortcutPath);
    } else {
        QFile::remove(shortcutPath);
    }
}

bool ShortcutManager::isShortcutPresent(QString shortcutName)
{
    QString startupPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + QDir::separator() + "Startup";
    QString shortcutPath = startupPath + QDir::separator() + shortcutName;

    return QFile::exists(shortcutPath);
}

