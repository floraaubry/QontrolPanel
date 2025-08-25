#pragma once

#include <QMainWindow>
#include <QSettings>
#include <QQmlApplicationEngine>
#include <QWindow>
#include <Windows.h>
#include <QLocalServer>
#include <QLocalSocket>

class QontrolPanel : public QWidget
{
    Q_OBJECT

public:
    QontrolPanel(QWidget *parent = nullptr);
    ~QontrolPanel();
    static QontrolPanel* instance;

private slots:
    void onLanguageChanged();
    void onNewConnection();
    void onClientDisconnected();
    void onPanelVisibilityChanged(bool visible);
    void onGlobalShortcutsToggled(bool enabled);

private:
    QQmlApplicationEngine* engine;
    QWindow* panelWindow;
    QSettings settings;
    QAction *outputDeviceAction;
    QAction *inputDeviceAction;
    QString currentOutputDevice;
    QString currentInputDevice;

    void initializeQMLEngine();
    void destroyQMLEngine();
    void togglePanel();
    void showPanel();
    void hidePanel();
    QString elideDeviceText(const QString& deviceName, int volume, bool muted);

    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK mouseHook;
    static HHOOK keyboardHook;

    void installGlobalMouseHook();
    void uninstallGlobalMouseHook();
    void installKeyboardHook();
    void uninstallKeyboardHook();

    bool handleCustomShortcut(DWORD vkCode);
    void toggleChatMix();
    bool isModifierPressed(int qtModifier);
    int qtKeyToVirtualKey(int qtKey);

    QLocalServer* localServer;
    void setupLocalServer();
    void cleanupLocalServer();
    bool isPanelVisible = false;
};
