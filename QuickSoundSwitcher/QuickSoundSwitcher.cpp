#include "QuickSoundSwitcher.h"
#include "Utils.h"
#include "AudioManager.h"
#include "ShortcutManager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>

HHOOK QuickSoundSwitcher::keyboardHook = NULL;
HHOOK QuickSoundSwitcher::mouseHook = NULL;
QuickSoundSwitcher* QuickSoundSwitcher::instance = nullptr;

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QMainWindow(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , settings("Odizinne", "QuickSoundSwitcher")
    , panel(nullptr)
    , settingsPage(nullptr)
{
    instance = this;
    createTrayIcon();
    updateApplicationColorScheme();
    loadSettings();
    installGlobalMouseHook();
}

QuickSoundSwitcher::~QuickSoundSwitcher()
{
    uninstallGlobalMouseHook();
    delete panel;
    delete settingsPage;
    instance = nullptr;
}

void QuickSoundSwitcher::createTrayIcon()
{
    trayIcon->setIcon(QIcon(":/icons/icon.png"));
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
        return true;
    }
    return QMainWindow::event(event);
}

void QuickSoundSwitcher::updateApplicationColorScheme()
{
    QString mode = Utils::getTheme();
    QString color;
    QPalette palette = QApplication::palette();

    if (mode == "light") {
        color = Utils::getAccentColor("light1");
    } else {
        color = Utils::getAccentColor("dark1");
    }

    QColor highlightColor(color);

    palette.setColor(QPalette::Highlight, highlightColor);
    qApp->setPalette(palette);
}

void QuickSoundSwitcher::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        showPanel();
    }
}

void QuickSoundSwitcher::onPanelClosed()
{
    delete panel;
    panel = nullptr;
}

void QuickSoundSwitcher::onRunAtStartupStateChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    ShortcutManager::manageShortcut(action->isChecked(), "QuickSoundSwitcher.lnk");
}

void QuickSoundSwitcher::showSettings()
{
    if (settingsPage) {
        settingsPage->showNormal();
        settingsPage->raise();
        settingsPage->activateWindow();
        return;
    }

    settingsPage = new SettingsPage;
    settingsPage->setAttribute(Qt::WA_DeleteOnClose);
    connect(settingsPage, &SettingsPage::closed, this, &QuickSoundSwitcher::onSettingsClosed);
    connect(settingsPage, &SettingsPage::settingsChanged, this, &QuickSoundSwitcher::onSettingsChanged);
    settingsPage->show();
}

void QuickSoundSwitcher::loadSettings()
{
    mergeSimilarApps = settings.value("mergeSimilarApps", true).toBool();
}

void QuickSoundSwitcher::onSettingsChanged()
{
    loadSettings();
}

void QuickSoundSwitcher::onSettingsClosed()
{
    disconnect(settingsPage, &SettingsPage::closed, this, &QuickSoundSwitcher::onSettingsClosed);
    disconnect(settingsPage, &SettingsPage::settingsChanged, this, &QuickSoundSwitcher::onSettingsChanged);
    settingsPage = nullptr;
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

LRESULT CALLBACK QuickSoundSwitcher::MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) {
            QPoint cursorPos = QCursor::pos();
            QRect panelRect = instance->panel ? instance->panel->geometry() : QRect();

            if (!panelRect.contains(cursorPos)) {
                if (instance->panel && !instance->panel->isAnimating) {
                    emit instance->panel->lostFocus();
                }
            }
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void QuickSoundSwitcher::showPanel()
{
    if (panel) {
        return;
    }

    panel = new Panel(this);
    panel->mergeApps = mergeSimilarApps;
    panel->populateApplications();
    connect(panel, &Panel::lostFocus, this, &QuickSoundSwitcher::hidePanel);
    connect(panel, &Panel::panelClosed, this, &QuickSoundSwitcher::onPanelClosed);

    panel->animateIn(trayIcon->geometry());
}

void QuickSoundSwitcher::hidePanel()
{
    panel->animateOut();
}


