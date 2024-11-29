#ifndef QUICKSOUNDSWITCHER_H
#define QUICKSOUNDSWITCHER_H

#include "Panel.h"
#include "SettingsPage.h"
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QSettings>

class QuickSoundSwitcher : public QMainWindow
{
    Q_OBJECT

public:
    QuickSoundSwitcher(QWidget *parent = nullptr);
    ~QuickSoundSwitcher();
    static QuickSoundSwitcher* instance;
    void adjustOutputVolume(bool up);
    void toggleMuteWithKey();

protected:
    bool event(QEvent *event) override;

private slots:
    void onPanelClosed();
    void onRunAtStartupStateChanged();

private:
    QSystemTrayIcon *trayIcon;
    QSettings settings;
    Panel* panel;
    void createTrayIcon();
    void showPanel();
    void showMediaFlyout();
    void hidePanel();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK mouseHook;
    static HHOOK keyboardHook;
    void installGlobalMouseHook();
    void uninstallGlobalMouseHook();

    static const int HOTKEY_ID = 1;
    void loadSettings();
    void onSettingsChanged();
    void onSettingsClosed();
    void showSettings();
    void updateApplicationColorScheme();
    bool mergeSimilarApps;
    int volumeIncrement;
    SettingsPage *settingsPage;

signals:
    void outputMuteStateChanged(bool state);
    void volumeChangedWithTray(int newVolume);
};

#endif // QUICKSOUNDSWITCHER_H
