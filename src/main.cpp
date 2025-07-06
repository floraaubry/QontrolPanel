#include "quicksoundswitcher.h"
#include <QApplication>
#include <QProcess>
#include <QLocalSocket>
#include <QLocalServer>

bool tryConnectToExistingInstance()
{
    QLocalSocket socket;
    socket.connectToServer("QuickSoundSwitcher");

    if (socket.waitForConnected(1000)) {
        socket.write("show_panel");
        socket.waitForBytesWritten(1000);
        socket.disconnectFromServer();
        return true;
    }

    return false;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    a.setOrganizationName("Odizinne");
    a.setApplicationName("QuickSoundSwitcher");

    if (tryConnectToExistingInstance()) {
        qDebug() << "Sent show panel request to existing instance";
        return 0;
    }

    QuickSoundSwitcher w;

    return a.exec();
}
