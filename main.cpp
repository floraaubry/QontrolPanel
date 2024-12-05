#include "QuickSoundSwitcher.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QProcess>

bool isAnotherInstanceRunning(const QString& processName)
{
    QProcess process;
    process.start("tasklist", QStringList() << "/FI" << QString("IMAGENAME eq %1").arg(processName));
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    int count = output.count(processName, Qt::CaseInsensitive);

    return count > 1;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    QPalette palette = QApplication::palette();

    // Set the inactive palette to be the same as the active palette
    palette.setColor(QPalette::Inactive, QPalette::Window, palette.color(QPalette::Active, QPalette::Window));
    palette.setColor(QPalette::Inactive, QPalette::WindowText, palette.color(QPalette::Active, QPalette::WindowText));
    palette.setColor(QPalette::Inactive, QPalette::Button, palette.color(QPalette::Active, QPalette::Button));
    palette.setColor(QPalette::Inactive, QPalette::ButtonText, palette.color(QPalette::Active, QPalette::ButtonText));

    // Apply the updated palette to the entire application
    QApplication::setPalette(palette);

    const QString processName = "QuickSoundSwitcher.exe";
    if (isAnotherInstanceRunning(processName)) {
        qDebug() << "Another instance is already running. Exiting...";
        return 0;
    }

    QLocale locale;
    QString languageCode = locale.name().section('_', 0, 0);
    QTranslator translator;

    if (translator.load(":/translations/QuickSoundSwitcher_" + languageCode + ".qm")) {
        a.installTranslator(&translator);
    }

    QuickSoundSwitcher w;

    return a.exec();
}
