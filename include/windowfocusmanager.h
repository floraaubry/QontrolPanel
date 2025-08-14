#ifndef WINDOWFOCUSMANAGER_H
#define WINDOWFOCUSMANAGER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QString>
#include <Windows.h>

class WindowFocusManager : public QObject
{
    Q_OBJECT

public:
    explicit WindowFocusManager(QObject *parent = nullptr);
    ~WindowFocusManager();

    void startMonitoring();
    void stopMonitoring();

    bool isApplicationMutedInBackground(const QString& executableName) const;
    void setApplicationMutedInBackground(const QString& executableName, bool muted);

    QStringList getBackgroundMutedApplications() const;

signals:
    void applicationFocusChanged(const QString& executableName, bool hasFocus);

private slots:
    void onApplicationFocusChanged(const QString& executableName, bool hasFocus);

private:
    static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
    static WindowFocusManager* s_instance;

    QString getExecutableNameFromHwnd(HWND hwnd);
    QString getExecutableNameFromPid(DWORD pid);

    void loadSettings();
    void saveSettings();
    QString getSettingsFilePath() const;

    HWINEVENTHOOK m_winEventHook;
    QSet<QString> m_backgroundMutedApps;
    QMap<QString, bool> m_currentFocusState; // executable -> has focus
    QString m_currentFocusedApp;
    bool m_isMonitoring;
};

#endif // WINDOWFOCUSMANAGER_H
