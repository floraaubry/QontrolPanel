#include "windowfocusmanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFileInfo>
#include <QMetaObject>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

WindowFocusManager* WindowFocusManager::s_instance = nullptr;

WindowFocusManager::WindowFocusManager(QObject *parent)
    : QObject(parent)
    , m_winEventHook(nullptr)
    , m_isMonitoring(false)
{
    s_instance = this;
    loadSettings();

    connect(this, &WindowFocusManager::applicationFocusChanged,
            this, &WindowFocusManager::onApplicationFocusChanged);
}

WindowFocusManager::~WindowFocusManager()
{
    stopMonitoring();
    saveSettings();
    s_instance = nullptr;
}

void WindowFocusManager::startMonitoring()
{
    if (m_isMonitoring) {
        return;
    }

    m_winEventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        NULL, WinEventProc, 0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
        );

    if (m_winEventHook) {
        m_isMonitoring = true;
        qDebug() << "Window focus monitoring started";
    } else {
        qDebug() << "Failed to start window focus monitoring";
    }
}

void WindowFocusManager::stopMonitoring()
{
    if (m_winEventHook) {
        UnhookWinEvent(m_winEventHook);
        m_winEventHook = nullptr;
    }
    m_isMonitoring = false;
}

void CALLBACK WindowFocusManager::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    Q_UNUSED(hWinEventHook)
    Q_UNUSED(idObject)
    Q_UNUSED(idChild)
    Q_UNUSED(dwEventThread)
    Q_UNUSED(dwmsEventTime)

    if (!s_instance || event != EVENT_SYSTEM_FOREGROUND || !hwnd) {
        return;
    }

    QString executableName = s_instance->getExecutableNameFromHwnd(hwnd);
    if (executableName.isEmpty()) {
        return;
    }

    // Emit signal asynchronously to main thread
    QMetaObject::invokeMethod(s_instance, "applicationFocusChanged",
                              Qt::QueuedConnection,
                              Q_ARG(QString, executableName),
                              Q_ARG(bool, true));
}

QString WindowFocusManager::getExecutableNameFromHwnd(HWND hwnd)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    return getExecutableNameFromPid(pid);
}

QString WindowFocusManager::getExecutableNameFromPid(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        return QString();
    }

    WCHAR exePath[MAX_PATH];
    if (GetModuleFileNameEx(hProcess, NULL, exePath, MAX_PATH)) {
        QString path = QString::fromWCharArray(exePath);
        QFileInfo fileInfo(path);
        CloseHandle(hProcess);
        return fileInfo.baseName();
    }

    CloseHandle(hProcess);
    return QString();
}

void WindowFocusManager::onApplicationFocusChanged(const QString& executableName, bool hasFocus)
{
    // If the same app is gaining focus, no need to process
    if (hasFocus && m_currentFocusedApp == executableName) {
        return;
    }

    QString previousFocusedApp = m_currentFocusedApp;

    if (hasFocus) {
        m_currentFocusedApp = executableName;

        // Previous app lost focus
        if (!previousFocusedApp.isEmpty() && previousFocusedApp != executableName) {
            m_currentFocusState[previousFocusedApp] = false;
            if (m_backgroundMutedApps.contains(previousFocusedApp)) {
                emit applicationFocusChanged(previousFocusedApp, false);
            }
        }

        // Current app gained focus
        m_currentFocusState[executableName] = true;
        if (m_backgroundMutedApps.contains(executableName)) {
            emit applicationFocusChanged(executableName, true);
        }
    }
}

bool WindowFocusManager::isApplicationMutedInBackground(const QString& executableName) const
{
    return m_backgroundMutedApps.contains(executableName);
}

void WindowFocusManager::setApplicationMutedInBackground(const QString& executableName, bool muted)
{
    if (muted) {
        m_backgroundMutedApps.insert(executableName);
    } else {
        m_backgroundMutedApps.remove(executableName);
    }
    saveSettings();
}

QStringList WindowFocusManager::getBackgroundMutedApplications() const
{
    return m_backgroundMutedApps.values();
}

void WindowFocusManager::loadSettings()
{
    QString filePath = getSettingsFilePath();
    QFile file(filePath);

    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray mutedAppsArray = root["backgroundMutedApps"].toArray();

    m_backgroundMutedApps.clear();
    for (const QJsonValue& value : mutedAppsArray) {
        m_backgroundMutedApps.insert(value.toString());
    }
}

void WindowFocusManager::saveSettings()
{
    QString filePath = getSettingsFilePath();
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QJsonArray mutedAppsArray;
    for (const QString& app : m_backgroundMutedApps) {
        mutedAppsArray.append(app);
    }

    QJsonObject root;
    root["backgroundMutedApps"] = mutedAppsArray;

    QJsonDocument doc(root);
    file.write(doc.toJson());
}

QString WindowFocusManager::getSettingsFilePath() const
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    return appDataPath + "/backgroundmute.json";
}
