#include "Utils.h"
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

QString toHex(BYTE value)
{
    const char* hexDigits = "0123456789ABCDEF";
    return QString("%1%2")
        .arg(hexDigits[value >> 4])
        .arg(hexDigits[value & 0xF]);
}

QString Utils::getAccentColor(const QString &accentKey)
{
    HKEY hKey;
    BYTE accentPalette[32];  // AccentPalette contains 32 bytes
    DWORD bufferSize = sizeof(accentPalette);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegGetValueW(hKey, NULL, L"AccentPalette", RRF_RT_REG_BINARY, NULL, accentPalette, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);

            int index = 0;
            if (accentKey == "light3") index = 0;
            else if (accentKey == "light2") index = 4;
            else if (accentKey == "light1") index = 8;
            else if (accentKey == "normal") index = 12;
            else if (accentKey == "dark1") index = 16;
            else if (accentKey == "dark2") index = 20;
            else if (accentKey == "dark3") index = 24;
            else {
                qDebug() << "Invalid accentKey provided.";
                return "#FFFFFF";
            }

            QString red = toHex(accentPalette[index]);
            QString green = toHex(accentPalette[index + 1]);
            QString blue = toHex(accentPalette[index + 2]);

            return QString("#%1%2%3").arg(red, green, blue);
        } else {
            qDebug() << "Failed to retrieve AccentPalette from the registry.";
        }

        RegCloseKey(hKey);
    } else {
        qDebug() << "Failed to open registry key.";
    }

    return "#FFFFFF";
}

void Utils::playSoundNotification()
{
    const wchar_t* soundFile = L"C:\\Windows\\Media\\Windows Background.wav";
    PlaySound(soundFile, NULL, SND_FILENAME | SND_ASYNC);
}

int getBuildNumber()
{
    QSettings registry("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", QSettings::NativeFormat);
    QVariant buildVariant = registry.value("CurrentBuild");

    if (!buildVariant.isValid()) {
        buildVariant = registry.value("CurrentBuildNumber");
    }

    if (buildVariant.isValid() && buildVariant.canConvert<QString>()) {
        bool ok;
        int buildNumber = buildVariant.toString().toInt(&ok);
        if (ok) {
            return buildNumber;
        }
    }

    qDebug() << "Failed to retrieve build number from the registry.";
    return -1;
}


bool Utils::isWindows10()
{
    int buildNumber = getBuildNumber();
    return (buildNumber >= 10240 && buildNumber < 22000);
}
