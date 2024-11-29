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

QString Utils::truncateTitle(QString title, QFontMetrics metrics, int labelWidth)
{
    int maxLines = 2;
    int lineHeight = metrics.lineSpacing();
    int maxHeight = maxLines * lineHeight;

    QString truncatedTitle;
    QStringList words = title.split(' ');
    QString currentLine;
    int currentHeight = 0;

    for (const QString& word : words) {
        QString testLine = currentLine.isEmpty() ? word : currentLine + ' ' + word;

        if (metrics.horizontalAdvance(testLine) > labelWidth) {
            if (!truncatedTitle.isEmpty()) {
                truncatedTitle += '\n';
            }
            truncatedTitle += currentLine;
            currentHeight += lineHeight;
            currentLine = word;

            if (currentHeight >= maxHeight) {
                truncatedTitle.chop(3);
                truncatedTitle += "...";
                break;
            }
        } else {
            currentLine = testLine;
        }
    }

    if (currentHeight < maxHeight && !currentLine.isEmpty()) {
        if (!truncatedTitle.isEmpty()) {
            truncatedTitle += '\n';
        }
        truncatedTitle += currentLine;
    }
    return truncatedTitle;
}
