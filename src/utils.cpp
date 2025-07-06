#include "utils.h"
#include <QPainter>
#include <windows.h>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QPalette>
#include <QPushButton>
#include <QComboBox>
#include <QPainterPath>

#undef min  // Undefine min macro to avoid conflicts with std::min
#undef max  // Undefine max macro to avoid conflicts with std::max

QString Utils::getTheme()
{
    // Determine the theme based on registry value
    QSettings settings(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    int value = settings.value("AppsUseLightTheme", 1).toInt();

    return (value == 0) ? "light" : "dark";
}

QString Utils::getIcon(int type, int volume, bool muted)
{
    QString theme = getTheme();
    if (type == 1) {
        QString volumeSymbol;
        if (volume > 66) {
            volumeSymbol = "100";
        } else if (volume > 33) {
            volumeSymbol = "66";
        } else if (volume > 0) {
            volumeSymbol = "33";
        } else {
            volumeSymbol = "0";
        }
        return QString(":/icons/tray_" + theme + "_" + volumeSymbol + ".png");
    } else if (type == 2) {
        QString volumeSymbol;
        if (volume > 66) {
            volumeSymbol = "100";
        } else if (volume > 33) {
            volumeSymbol = "66";
        } else if (volume > 0) {
            volumeSymbol = "33";
        } else {
            volumeSymbol = "0";
        }
        return QString(":/icons/panel_volume_" + volumeSymbol + ".png");
    } else {
        if (muted) {
            return QString(":/icons/mic_muted.png");
        } else {
            return QString(":/icons/mic.png");
        }
    }
}

void Utils::playSoundNotification()
{
    const wchar_t* soundFile = L"C:\\Windows\\Media\\Windows Background.wav";
    PlaySound(soundFile, NULL, SND_FILENAME | SND_ASYNC);
}
