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

QIcon Utils::getIcon(int type, int volume, bool muted)
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
        return QIcon(":/icons/tray_" + theme + "_" + volumeSymbol + ".png");
    } else if (type == 2) {
        if (muted) {
            return QIcon(":/icons/headset_" + theme + "_muted.png");
        } else {
            return QIcon(":/icons/headset_" + theme + ".png");
        }
    } else {
        if (muted) {
            return QIcon(":/icons/mic_" + theme + "_muted.png");
        } else {
            return QIcon(":/icons/mic_" + theme + ".png");
        }
    }
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

QColor Utils::adjustColor(const QColor &color, double factor)
{
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    int a = color.alpha();

    r = std::min(std::max(static_cast<int>(r * factor), 0), 255);
    g = std::min(std::max(static_cast<int>(g * factor), 0), 255);
    b = std::min(std::max(static_cast<int>(b * factor), 0), 255);

    return QColor(r, g, b, a);
}

bool Utils::isDarkMode(const QColor &color)
{
    int r = color.red();
    int g = color.green();
    int b = color.blue();
    double brightness = (r + g + b) / 3.0;
    return brightness < 127;
}

void Utils::setFrameColorBasedOnWindow(QWidget *window, QFrame *frame)
{
    QColor main_bg_color = window->palette().color(QPalette::Window);
    QColor frame_bg_color;

    if (isDarkMode(main_bg_color)) {
        frame_bg_color = adjustColor(main_bg_color, 1.75);  // Brighten color
    } else {
        frame_bg_color = adjustColor(main_bg_color, 0.95);  // Darken color
    }

    QPalette palette = frame->palette();
    palette.setBrush(QPalette::Window, QBrush(frame_bg_color));
    frame->setAutoFillBackground(true);
    frame->setPalette(palette);
}

QIcon Utils::generateMutedIcon(QPixmap originalPixmap)
{
    QPixmap mutedLayer(":/icons/muted_layer.png");
    mutedLayer = mutedLayer.scaled(originalPixmap.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPainter painter(&originalPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawPixmap(0, 0, mutedLayer);
    painter.end();

    return QIcon(originalPixmap);
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

void Utils::playSoundNotification(bool enabled)
{
    const wchar_t* soundFile;

    if (enabled) {
        soundFile = L"C:\\Windows\\Media\\Speech On.wav";
    } else {
        soundFile = L"C:\\Windows\\Media\\Speech Off.wav";
    }

    PlaySound(soundFile, NULL, SND_FILENAME | SND_ASYNC);
}

QPixmap Utils::createIconWithAccentBackground()
{
    QString theme = Utils::getTheme();
    QString accentColorStr;
    if (theme == "light") {
        accentColorStr = Utils::getAccentColor("light2");
    } else {
        accentColorStr = Utils::getAccentColor("dark1");
    }

    QColor accentColor(accentColorStr);
    QPixmap background(80, 80);
    background.fill(accentColor);
    QPainter painter(&background);
    int brightness = (accentColor.red() * 0.299) + (accentColor.green() * 0.587) + (accentColor.blue() * 0.114);
    QString iconTheme = (brightness > 127) ? ":/icons/overlay_dark.png" : ":/icons/overlay_light.png";  // Bright background -> dark icon, dark background -> light icon

    QPixmap icon(iconTheme);

    int x = (background.width() - icon.width()) / 2;
    int y = (background.height() - icon.height()) / 2;

    painter.drawPixmap(x, y, icon);
    painter.end();

    return background;
}

QIcon Utils::getPlaceholderIcon()
{
    QString theme = Utils::getTheme();
    return QIcon(QString(":/icons/placeholder_%1.png").arg(theme));
}

QIcon Utils::getButtonsIcon(QString button)
{
    QString theme = Utils::getTheme();
    return QIcon(QString(":/icons/%1_%2.png").arg(button, theme));
}

void Utils::sendPlayPauseKey()
{
    // Simulate the play/pause key press using the SendInput function
    INPUT input[1] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_MEDIA_PLAY_PAUSE;  // Key for Media Play/Pause

    // Send key press
    SendInput(1, input, sizeof(INPUT));

    // Sleep to simulate key release
    ZeroMemory(input, sizeof(INPUT));
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_MEDIA_PLAY_PAUSE;

    // Send key release
    input[0].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, input, sizeof(INPUT));

    qDebug() << "Play/Pause key sent!";
}

void Utils::sendNextKey()
{
    // Simulate the next track key press using the SendInput function
    INPUT input[1] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_MEDIA_NEXT_TRACK;  // Key for Next Track

    // Send key press
    SendInput(1, input, sizeof(INPUT));

    // Sleep to simulate key release
    ZeroMemory(input, sizeof(INPUT));
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_MEDIA_NEXT_TRACK;

    // Send key release
    input[0].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, input, sizeof(INPUT));

    qDebug() << "Next Track key sent!";
}

void Utils::sendPrevKey()
{
    // Simulate the previous track key press using the SendInput function
    INPUT input[1] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_MEDIA_PREV_TRACK;  // Key for Previous Track

    // Send key press
    SendInput(1, input, sizeof(INPUT));

    // Sleep to simulate key release
    ZeroMemory(input, sizeof(INPUT));
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_MEDIA_PREV_TRACK;

    // Send key release
    input[0].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, input, sizeof(INPUT));

    qDebug() << "Previous Track key sent!";
}
