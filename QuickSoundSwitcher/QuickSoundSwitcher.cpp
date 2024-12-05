#include "QuickSoundSwitcher.h"
#include "Utils.h"
#include "AudioManager.h"
#include "ShortcutManager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <Windows.h>

HHOOK QuickSoundSwitcher::keyboardHook = NULL;
HHOOK QuickSoundSwitcher::mouseHook = NULL;
QuickSoundSwitcher* QuickSoundSwitcher::instance = nullptr;

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QMainWindow(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , settings("Odizinne", "QuickSoundSwitcher")
    , worker(new MediaSessionWorker)
    , mediaFlyout(new MediaFlyout(this, worker))
    , panel(new Panel(this, mediaFlyout))
    , soundOverlay(new SoundOverlay(this))
    , settingsPage(new SettingsPage(this))
{
    AudioManager::initialize();
    instance = this;
    updateApplicationColorScheme();
    createTrayIcon();
    loadSettings();
    installGlobalMouseHook();
    installKeyboardHook();

    connect(worker, &MediaSessionWorker::sessionReady, this, &QuickSoundSwitcher::onSessionReady);
    connect(worker, &MediaSessionWorker::sessionError, this, &QuickSoundSwitcher::onSessionError);
    //connect(panel, &Panel::volumeChanged, this, &QuickSoundSwitcher::onVolumeChanged);
    //connect(panel, &Panel::outputMuteChanged, this, &QuickSoundSwitcher::onOutputMuteChanged);

    connect(settingsPage, &SettingsPage::settingsChanged, this, &QuickSoundSwitcher::onSettingsChanged);
}

QuickSoundSwitcher::~QuickSoundSwitcher()
{
    AudioManager::cleanup();
    uninstallGlobalMouseHook();
    uninstallKeyboardHook();
    delete worker;
    delete soundOverlay;
    delete panel;
    delete mediaFlyout;
    delete settingsPage;
    instance = nullptr;
}

void QuickSoundSwitcher::createTrayIcon()
{
    onOutputMuteChanged();

    QMenu *trayMenu = new QMenu(this);

    QAction *startupAction = new QAction(tr("Run at startup"), this);
    startupAction->setCheckable(true);
    startupAction->setChecked(ShortcutManager::isShortcutPresent("QuickSoundSwitcher.lnk"));
    connect(startupAction, &QAction::triggered, this, &QuickSoundSwitcher::onRunAtStartupStateChanged);

    QAction *settingsAction = new QAction(tr("Settings"), this);
    connect(settingsAction, &QAction::triggered, this, &QuickSoundSwitcher::showSettings);

    QAction *exitAction = new QAction(tr("Exit"), this);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);

    trayMenu->addAction(startupAction);
    trayMenu->addAction(settingsAction);
    trayMenu->addSeparator();
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("Quick Sound Switcher");
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &QuickSoundSwitcher::trayIconActivated);
}

bool QuickSoundSwitcher::event(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange) {
        updateApplicationColorScheme();
        onOutputMuteChanged();
        return true;
    }
    return QMainWindow::event(event);
}

void QuickSoundSwitcher::updateApplicationColorScheme()
{
    QPalette palette = QApplication::palette();

    // Set the inactive palette to be the same as the active palette
    palette.setColor(QPalette::Inactive, QPalette::Window, palette.color(QPalette::Active, QPalette::Window));
    palette.setColor(QPalette::Inactive, QPalette::WindowText, palette.color(QPalette::Active, QPalette::WindowText));
    palette.setColor(QPalette::Inactive, QPalette::Button, palette.color(QPalette::Active, QPalette::Button));
    palette.setColor(QPalette::Inactive, QPalette::ButtonText, palette.color(QPalette::Active, QPalette::ButtonText));

    // Apply the updated palette to the entire application
    QApplication::setPalette(palette);
}

void QuickSoundSwitcher::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        if (panel && !panel->isAnimating) {
            showPanel();
        }
    }
}

void QuickSoundSwitcher::hidePanel()
{
    panel->animateOut();
}

void QuickSoundSwitcher::onVolumeChanged()
{
    int volumeIcon;
    if (AudioManager::getPlaybackMute()) {
        volumeIcon = 0;
    } else {
        volumeIcon = AudioManager::getPlaybackVolume();
    }

    trayIcon->setIcon(Utils::getIcon(1, volumeIcon, NULL));
}

void QuickSoundSwitcher::onOutputMuteChanged()
{
    int volumeIcon;
    if (AudioManager::getPlaybackMute()) {
        volumeIcon = 0;
    } else {
        volumeIcon = AudioManager::getPlaybackVolume();
    }

    trayIcon->setIcon(Utils::getIcon(1, volumeIcon, NULL));
}

void QuickSoundSwitcher::onRunAtStartupStateChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    ShortcutManager::manageShortcut(action->isChecked(), "QuickSoundSwitcher.lnk");
}

void QuickSoundSwitcher::showSettings()
{
    settingsPage->showWindow();
}

void QuickSoundSwitcher::loadSettings()
{
    volumeIncrement = settings.value("volumeIncrement", 2).toInt();
    mergeSimilarApps = settings.value("mergeSimilarApps", true).toBool();
}

void QuickSoundSwitcher::onSettingsChanged()
{
    loadSettings();
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
        MSLLHOOKSTRUCT *hookStruct = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);

        if (wParam == WM_MOUSEWHEEL) {
            int zDelta = GET_WHEEL_DELTA_WPARAM(hookStruct->mouseData);
            QPoint cursorPos = QCursor::pos();

            QRect trayIconRect = instance->trayIcon->geometry();
            if (trayIconRect.contains(cursorPos)) {
                if (zDelta > 0) {
                    instance->adjustOutputVolume(true);
                } else {
                    instance->adjustOutputVolume(false);
                }
            }
        }

        if (wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) {
            QPoint cursorPos = QCursor::pos();
            QRect trayIconRect = instance->trayIcon->geometry();
            QRect panelRect = instance->panel ? instance->panel->panelWindow->geometry() : QRect();
            QRect mediaFlyoutRect = instance->mediaFlyout->mediaFlyoutWindow ? instance->mediaFlyout->mediaFlyoutWindow->geometry() : QRect();

            if (!trayIconRect.contains(cursorPos) &&
                !panelRect.contains(cursorPos) &&
                !mediaFlyoutRect.contains(cursorPos)) {
                if (instance->panel && !instance->panel->isAnimating && instance->panel->visible) {
                    instance->showPanel();
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
                return 1;
            case VK_VOLUME_DOWN:
                instance->adjustOutputVolume(false);
                return 1;
            case VK_VOLUME_MUTE:
                instance->toggleMuteWithKey();
                return 1;
            default:
                break;
            }
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void QuickSoundSwitcher::adjustOutputVolume(bool up)
{
    int volume = AudioManager::getPlaybackVolume();
    int newVolume;
    if (up) {
        newVolume = volume + volumeIncrement;
        if (newVolume > 100) {
            newVolume = 100;
        }
    } else {
        newVolume = volume - volumeIncrement;
        if (newVolume < 0) {
            newVolume = 0;
        }
    }

    AudioManager::setPlaybackVolume(newVolume);

    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMute(false);
        emit outputMuteStateChanged(false);
    }
    trayIcon->setIcon(Utils::getIcon(1, newVolume, NULL));
    emit volumeChangedWithTray();

    soundOverlay->animateIn();
    soundOverlay->updateVolumeIcon(Utils::getOverlayIcon(newVolume));
    soundOverlay->updateVolumeLabel(newVolume);
    soundOverlay->updateProgressBar(newVolume);
}

void QuickSoundSwitcher::toggleMuteWithKey()
{
    bool newPlaybackMute = !AudioManager::getPlaybackMute();
    AudioManager::setPlaybackMute(newPlaybackMute);

    int volumeIcon;
    if (AudioManager::getPlaybackMute()) {
        volumeIcon = 0;
    } else {
        volumeIcon = AudioManager::getPlaybackVolume();
    }

    trayIcon->setIcon(Utils::getIcon(1, volumeIcon, NULL));

    emit outputMuteStateChanged(newPlaybackMute);

    int volume = AudioManager::getPlaybackVolume();
    soundOverlay->updateVolumeIcon(Utils::getOverlayIcon(volumeIcon));
    soundOverlay->updateVolumeLabel(volume);
    soundOverlay->updateProgressBar(volume);
    soundOverlay->animateIn();
}

void QuickSoundSwitcher::onSessionReady(const MediaSession& session)
{
    mediaFlyout->updateTitle(session.title);
    mediaFlyout->updateArtist(session.artist);
    mediaFlyout->updateIcon(session.icon);
    mediaFlyout->updatePauseButton(session.playbackState);
    mediaFlyout->updateControls(session.canGoPrevious, session.canGoNext);
    mediaFlyout->updateProgress(session.currentTime, session.duration);
}

void QuickSoundSwitcher::onSessionError()
{
    mediaFlyout->setDefaults();
}

void QuickSoundSwitcher::showPanel()
{
    if (panel->isAnimating) return;

    if (!panel->visible) {
        //panel->mergeApps = mergeSimilarApps;
        //panel->populateComboBoxes();
        //panel->setButtons();
        //panel->setSliders();
        //panel->populateApplications();

        worker->process();
        panel->animateIn();
    } else {
        panel->animateOut();
    }
}
