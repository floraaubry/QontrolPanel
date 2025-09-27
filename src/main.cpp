#include "qontrolpanel.h"
#include "logmanager.h"
#include <QApplication>
#include <QProcess>
#include <QLocalSocket>
#include <QLocalServer>
#include <QLoggingCategory>

bool tryConnectToExistingInstance()
{
    QLocalSocket socket;
    socket.connectToServer("QontrolPanel");

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
    QLoggingCategory::setFilterRules(
        "qt.multimedia.*=false\n"
        "qt.qpa.mime*=false"
        );

    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    a.setOrganizationName("Odizinne");
    a.setApplicationName("QontrolPanel");
    qRegisterMetaType<LogManager::LogType>("LogType");
    LogManager::instance()->sendLog(LogManager::Core, "Starting application");

    if (tryConnectToExistingInstance()) {
        qDebug() << "Sent show panel request to existing instance";
        return 0;
    }

    QontrolPanel w;

    return a.exec();
}
