#include "quicksoundswitcher.h"
#include "utils.h"
#include "audiomanager.h"
#include "shortcutmanager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <Windows.h>

HHOOK QuickSoundSwitcher::keyboardHook = NULL;
HHOOK QuickSoundSwitcher::mouseHook = NULL;
QuickSoundSwitcher* QuickSoundSwitcher::instance = nullptr;
static bool validMouseDown = false;

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QWidget(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , soundPanel(nullptr)
    , settings("Odizinne", "QuickSoundSwitcher")
{
    AudioManager::initialize();
    instance = this;
    createTrayIcon();
    installGlobalMouseHook();
    installKeyboardHook();

    bool firstRun = settings.value("firstRun", true).toBool();

    if (firstRun) {
        settings.setValue("firstRun", false);
        trayIcon->showMessage(
            "Access sound panel from the system tray",
            "This notification won't show again",
            QSystemTrayIcon::NoIcon
            );
    }
}

QuickSoundSwitcher::~QuickSoundSwitcher()
{
    AudioManager::cleanup();
    uninstallGlobalMouseHook();
    uninstallKeyboardHook();
    delete soundPanel;
    instance = nullptr;
}

void QuickSoundSwitcher::createTrayIcon()
{
    onOutputMuteChanged();

    if (settings.value("mixerOnly").toBool()) {
        QString theme = Utils::getTheme();
        if (theme == "light") {
            trayIcon->setIcon(QIcon(":/icons/system_light.png"));
        } else {
            trayIcon->setIcon(QIcon(":/icons/system_dark.png"));
        }
    }
    QMenu *trayMenu = new QMenu(this);

    QAction *mixerOnly = new QAction(tr("Use volume mixer only"), this);
    mixerOnly->setCheckable(true);
    mixerOnly->setChecked(settings.value("mixerOnly", false).toBool());
    connect(mixerOnly, &QAction::triggered, this, &QuickSoundSwitcher::onMixerOnlyStateChanged);

    QAction *startupAction = new QAction(tr("Run at startup"), this);
    startupAction->setCheckable(true);
    startupAction->setChecked(ShortcutManager::isShortcutPresent("QuickSoundSwitcher.lnk"));
    connect(startupAction, &QAction::triggered, this, &QuickSoundSwitcher::onRunAtStartupStateChanged);

    QAction *exitAction = new QAction(tr("Exit"), this);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);

    trayMenu->addAction(startupAction);
    trayMenu->addSeparator();
    trayMenu->addAction(mixerOnly);
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("Quick Sound Switcher");
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &QuickSoundSwitcher::trayIconActivated);
}

void QuickSoundSwitcher::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        togglePanel();
    }
}

void QuickSoundSwitcher::onOutputMuteChanged()
{
    int volumeIcon;
    if (AudioManager::getPlaybackMute()) {
        volumeIcon = 0;
    } else {
        volumeIcon = AudioManager::getPlaybackVolume();
    }

    trayIcon->setIcon(QIcon(Utils::getIcon(1, volumeIcon, NULL)));
}

void QuickSoundSwitcher::onRunAtStartupStateChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    ShortcutManager::manageShortcut(action->isChecked(), "QuickSoundSwitcher.lnk");
}

void QuickSoundSwitcher::installGlobalMouseHook()
{
    if (mouseHook == NULL) {
        mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
    }
}

void QuickSoundSwitcher::uninstallGlobalMouseHook()
{
    if (mouseHook != NULL) {
        UnhookWindowsHookEx(mouseHook);
        mouseHook = NULL;
    }
}

void QuickSoundSwitcher::installKeyboardHook()
{
    if (keyboardHook == NULL) {
        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    }
}

void QuickSoundSwitcher::uninstallKeyboardHook()
{
    if (keyboardHook != NULL) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = NULL;
    }
}

LRESULT CALLBACK QuickSoundSwitcher::MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) {
            QPoint cursorPos = QCursor::pos();
            QRect trayIconRect = instance->trayIcon->geometry();
            QRect soundPanelRect;

            if (instance->soundPanel && instance->soundPanel->soundPanelWindow) {
                soundPanelRect = instance->soundPanel->soundPanelWindow->geometry();
            } else {
                soundPanelRect = QRect();
            }

            validMouseDown = !soundPanelRect.contains(cursorPos) && !trayIconRect.contains(cursorPos);
        }
        else if ((wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) && validMouseDown) {
            QPoint cursorPos = QCursor::pos();
            QRect trayIconRect = instance->trayIcon->geometry();
            QRect soundPanelRect;

            if (instance->soundPanel && instance->soundPanel->soundPanelWindow) {
                soundPanelRect = instance->soundPanel->soundPanelWindow->geometry();
            } else {
                soundPanelRect = QRect();
            }

            validMouseDown = false;

            if (!soundPanelRect.contains(cursorPos) && !trayIconRect.contains(cursorPos)) {
                if (instance->soundPanel) {
                    instance->soundPanel->animateOut();
                }
            }
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK QuickSoundSwitcher::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT pKeyboard = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            switch (pKeyboard->vkCode) {
            case VK_VOLUME_UP:
                instance->adjustOutputVolume(true);
                break;
            case VK_VOLUME_DOWN:
                instance->adjustOutputVolume(false);
                break;
            case VK_VOLUME_MUTE:
                instance->toggleMuteWithKey();
                break;
            default:
                break;
            }
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void QuickSoundSwitcher::adjustOutputVolume(bool up)
{
    int originalVolume = AudioManager::getPlaybackVolume();
    int newVolume;
    if (up) {
        newVolume = originalVolume + 2;
    } else {
        newVolume = originalVolume - 2;
    }

    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMute(false);
    }

    trayIcon->setIcon(QIcon(Utils::getIcon(1, newVolume, NULL)));
    emit volumeChangedWithTray(newVolume) ;
}

void QuickSoundSwitcher::toggleMuteWithKey()
{
    bool muted = !AudioManager::getPlaybackMute();
    int volume = AudioManager::getPlaybackVolume();

    int volumeIcon;
    if (volume == 0 || muted) {
        volumeIcon = 0;
    } else {
        volumeIcon = volume;
    }

    trayIcon->setIcon(QIcon(Utils::getIcon(1, volumeIcon, NULL)));
    emit outputMuteStateChanged(muted);
}

void QuickSoundSwitcher::togglePanel()
{
    if (!soundPanel) {
        soundPanel = new SoundPanel(this);
        connect(soundPanel, &SoundPanel::shouldUpdateTray, this, &QuickSoundSwitcher::onOutputMuteChanged);
        connect(soundPanel, &QObject::destroyed, this, &QuickSoundSwitcher::onSoundPanelClosed);
    } else {
        soundPanel->animateOut();
    }
}

void QuickSoundSwitcher::onSoundPanelClosed() {
    soundPanel = nullptr;
}

void QuickSoundSwitcher::onMixerOnlyStateChanged() {
    QAction *action = qobject_cast<QAction *>(sender());
    settings.setValue("mixerOnly", action->isChecked());

    if (settings.value("mixerOnly").toBool()) {
        QString theme = Utils::getTheme();
        if (theme == "light") {
            trayIcon->setIcon(QIcon(":/icons/system_light.png"));
        } else {
            trayIcon->setIcon(QIcon(":/icons/system_dark.png"));
        }
    } else {
        onOutputMuteChanged();
    }
}
