#include "quicksoundswitcher.h"
#include "soundpanelbridge.h"
#include "utils.h"
#include "audiomanager.h"
#include "mediasessionmanager.h"
#include <QMenu>
#include <QApplication>
#include <QScreen>
#include <QRect>
#include <QWindow>
#include <QQmlContext>
#include <QTimer>
#include <QFontMetrics>
#include <Windows.h>
#include <QProcess>

HHOOK QuickSoundSwitcher::keyboardHook = NULL;
HHOOK QuickSoundSwitcher::mouseHook = NULL;
QuickSoundSwitcher* QuickSoundSwitcher::instance = nullptr;
static bool validMouseDown = false;

QuickSoundSwitcher::QuickSoundSwitcher(QWidget *parent)
    : QWidget(parent)
    , trayIcon(new QSystemTrayIcon(this))
    , engine(nullptr)
    , panelWindow(nullptr)
    , isPanelVisible(false)
    , settings("Odizinne", "QuickSoundSwitcher")
    , outputDeviceAction(nullptr)
    , inputDeviceAction(nullptr)
    , currentOutputDevice("")
    , currentInputDevice("")
    , localServer(nullptr)
{
    AudioManager::initialize();
    MediaSessionManager::initialize();
    instance = this;

    setupLocalServer();
    // Initialize QML engine once
    initializeQMLEngine();

    // Connect to audio worker for tray icon updates
    if (AudioManager::getWorker()) {
        connect(AudioManager::getWorker(), &AudioWorker::playbackVolumeChanged,
                this, &QuickSoundSwitcher::onOutputMuteChanged);
        connect(AudioManager::getWorker(), &AudioWorker::playbackMuteChanged,
                this, &QuickSoundSwitcher::onOutputMuteChanged);

        // Connect for tray menu updates
        connect(AudioManager::getWorker(), &AudioWorker::playbackVolumeChanged,
                this, &QuickSoundSwitcher::updateTrayMenuDeviceInfo);
        connect(AudioManager::getWorker(), &AudioWorker::playbackMuteChanged,
                this, &QuickSoundSwitcher::updateTrayMenuDeviceInfo);
        connect(AudioManager::getWorker(), &AudioWorker::recordingVolumeChanged,
                this, &QuickSoundSwitcher::updateTrayMenuDeviceInfo);
        connect(AudioManager::getWorker(), &AudioWorker::recordingMuteChanged,
                this, &QuickSoundSwitcher::updateTrayMenuDeviceInfo);
        connect(AudioManager::getWorker(), &AudioWorker::defaultEndpointChanged,
                this, &QuickSoundSwitcher::initializeTrayMenuDeviceInfo);
    }

    if (SoundPanelBridge::instance()) {
        connect(SoundPanelBridge::instance(), &SoundPanelBridge::languageChanged,
                this, &QuickSoundSwitcher::onLanguageChanged);
    }

    createTrayIcon();
    installKeyboardHook();

    // Initialize device info after a short delay to let everything settle
    QTimer::singleShot(1000, this, &QuickSoundSwitcher::initializeTrayMenuDeviceInfo);

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
    if (SoundPanelBridge::instance()) {
        bool chatMixEnabled = QSettings("Odizinne", "QuickSoundSwitcher").value("chatMixEnabled", false).toBool();
        if (chatMixEnabled) {
            SoundPanelBridge::instance()->restoreOriginalVolumes();

            QEventLoop loop;
            QTimer::singleShot(200, &loop, &QEventLoop::quit);
            loop.exec();
        }
    }

    AudioManager::cleanup();
    MediaSessionManager::cleanup();
    uninstallGlobalMouseHook();
    uninstallKeyboardHook();
    destroyQMLEngine();
    cleanupLocalServer();
    instance = nullptr;
}

void QuickSoundSwitcher::initializeQMLEngine()
{
    if (engine) {
        return; // Already initialized
    }

    engine = new QQmlApplicationEngine(this);
    engine->loadFromModule("Odizinne.QuickSoundSwitcher", "SoundPanel");

    if (!engine->rootObjects().isEmpty()) {
        panelWindow = qobject_cast<QWindow*>(engine->rootObjects().first());
        if (panelWindow) {
            panelWindow->setProperty("visible", false);

            connect(panelWindow, SIGNAL(hideAnimationFinished()),
                    this, SLOT(onPanelHideAnimationFinished()));
        }
    }

    if (SoundPanelBridge::instance()) {
        connect(SoundPanelBridge::instance(), &SoundPanelBridge::shouldUpdateTray,
                this, &QuickSoundSwitcher::onOutputMuteChanged);

        // Connect to data initialization complete signal
        connect(SoundPanelBridge::instance(), &SoundPanelBridge::dataInitializationComplete,
                this, &QuickSoundSwitcher::onDataInitializationComplete);
    }
}

void QuickSoundSwitcher::destroyQMLEngine()
{
    if (engine) {
        engine->deleteLater();
        engine = nullptr;
    }
    panelWindow = nullptr;
}

void QuickSoundSwitcher::createTrayIcon()
{
    onOutputMuteChanged();

    if (settings.value("panelMode", 0).toInt() == 1) {
        QString theme = Utils::getTheme();
        if (theme == "light") {
            trayIcon->setIcon(QIcon(":/icons/system_light.png"));
        } else {
            trayIcon->setIcon(QIcon(":/icons/system_dark.png"));
        }
    }

    QMenu *trayMenu = new QMenu(this);

    outputDeviceAction = new QAction(tr("Output: Loading..."), this);
    trayMenu->addAction(outputDeviceAction);

    inputDeviceAction = new QAction(tr("Input: Loading..."), this);
    trayMenu->addAction(inputDeviceAction);

    trayMenu->addSeparator();

    QAction *lSoundSettingsAction = new QAction(tr("Windows sound settings (Legacy)"), this);
    trayMenu->addAction(lSoundSettingsAction);

    QAction *mSoundSettingsAction = new QAction(tr("Windows sound settings (Modern)"), this);
    trayMenu->addAction(mSoundSettingsAction);

    trayMenu->addSeparator();

    QAction *exitAction = new QAction(tr("Exit"), this);
    trayMenu->addAction(exitAction);

    connect(exitAction, &QAction::triggered, this, &QApplication::quit);
    connect(lSoundSettingsAction, &QAction::triggered, this, &QuickSoundSwitcher::openLegacySoundSettings);
    connect(mSoundSettingsAction, &QAction::triggered, this, &QuickSoundSwitcher::openModernSoundSettings);



    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("Quick Sound Switcher");
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &QuickSoundSwitcher::trayIconActivated);
}

void QuickSoundSwitcher::initializeTrayMenuDeviceInfo()
{
    if (!AudioManager::getWorker()) return;

    // Get device names
    connect(AudioManager::getWorker(), &AudioWorker::playbackDevicesReady,
            this, [this](const QList<AudioDevice>& devices) {
                for (const AudioDevice& device : devices) {
                    if (device.isDefault) {
                        currentOutputDevice = device.shortName.isEmpty() ? device.name : device.shortName;
                        updateTrayMenuDeviceInfo();
                        break;
                    }
                }
            }, Qt::SingleShotConnection);

    connect(AudioManager::getWorker(), &AudioWorker::recordingDevicesReady,
            this, [this](const QList<AudioDevice>& devices) {
                for (const AudioDevice& device : devices) {
                    if (device.isDefault) {
                        currentInputDevice = device.shortName.isEmpty() ? device.name : device.shortName;
                        updateTrayMenuDeviceInfo();
                        break;
                    }
                }
            }, Qt::SingleShotConnection);

    AudioManager::enumeratePlaybackDevicesAsync();
    AudioManager::enumerateRecordingDevicesAsync();
}

void QuickSoundSwitcher::updateTrayMenuDeviceInfo()
{
    if (!outputDeviceAction || !inputDeviceAction) return;

    // Get current volumes and mute states (from cache, non-blocking)
    int outputVolume = AudioManager::getPlaybackVolume();
    int inputVolume = AudioManager::getRecordingVolume();
    bool outputMuted = AudioManager::getPlaybackMute();
    bool inputMuted = AudioManager::getRecordingMute();

    // Update output device info
    QString outputText;
    if (!currentOutputDevice.isEmpty()) {
        outputText = QString("Output: %1").arg(elideDeviceText(currentOutputDevice, outputVolume, outputMuted));
    } else {
        outputText = QString("Output: %1%").arg(outputVolume);
    }
    outputDeviceAction->setText(outputText);

    // Update input device info
    QString inputText;
    if (!currentInputDevice.isEmpty()) {
        inputText = QString("Input: %1").arg(elideDeviceText(currentInputDevice, inputVolume, inputMuted));
    } else {
        inputText = QString("Input: %1%").arg(inputVolume);
    }
    inputDeviceAction->setText(inputText);
}

QString QuickSoundSwitcher::elideDeviceText(const QString& deviceName, int volume, bool muted)
{
    QFontMetrics metrics(QApplication::font());

    // Calculate available width (menu width minus some padding)
    int maxWidth = 250;

    QString volumeText = QString(" %1%").arg(volume);
    QString prefix = "Output: "; // Account for the prefix
    int availableWidth = maxWidth - metrics.horizontalAdvance(prefix + volumeText);

    QString elidedName = metrics.elidedText(deviceName, Qt::ElideRight, availableWidth);
    return QString("%1%2").arg(elidedName, volumeText);
}

void QuickSoundSwitcher::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        togglePanel();
    }
}

void QuickSoundSwitcher::onOutputMuteChanged()
{
    // Use the cached getters (now non-blocking)
    int volumeIcon;
    if (AudioManager::getPlaybackMute()) {
        volumeIcon = 0;
    } else {
        volumeIcon = AudioManager::getPlaybackVolume();
    }

    trayIcon->setIcon(QIcon(Utils::getIcon(1, volumeIcon, NULL)));
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

LRESULT CALLBACK QuickSoundSwitcher::MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) {
            QPoint cursorPos = QCursor::pos();
            QRect trayIconRect = instance->trayIcon->geometry();
            QRect soundPanelRect;

            if (instance->panelWindow && instance->isPanelVisible) {
                soundPanelRect = instance->panelWindow->geometry();
            } else {
                soundPanelRect = QRect();
            }

            validMouseDown = !soundPanelRect.contains(cursorPos) && !trayIconRect.contains(cursorPos);
        }
        else if ((wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) && validMouseDown) {
            QPoint cursorPos = QCursor::pos();
            QRect trayIconRect = instance->trayIcon->geometry();
            QRect soundPanelRect;

            if (instance->panelWindow && instance->isPanelVisible) {
                soundPanelRect = instance->panelWindow->geometry();
            } else {
                soundPanelRect = QRect();
            }

            validMouseDown = false;

            if (!soundPanelRect.contains(cursorPos) && !trayIconRect.contains(cursorPos)) {
                if (instance->isPanelVisible) {
                    instance->hidePanel();
                }
            }
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// In quicksoundswitcher.cpp - add these helper functions
bool QuickSoundSwitcher::isModifierPressed(int qtModifier)
{
    bool result = true;
    if (qtModifier & Qt::ControlModifier) {
        result &= (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    }
    if (qtModifier & Qt::ShiftModifier) {
        result &= (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    }
    if (qtModifier & Qt::AltModifier) {
        result &= (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    }
    return result;
}

int QuickSoundSwitcher::qtKeyToVirtualKey(int qtKey)
{
    // Convert common Qt keys to Windows virtual keys
    switch (qtKey) {
    case Qt::Key_A: return 0x41;
    case Qt::Key_B: return 0x42;
    case Qt::Key_C: return 0x43;
    case Qt::Key_D: return 0x44;
    case Qt::Key_E: return 0x45;
    case Qt::Key_F: return 0x46;
    case Qt::Key_G: return 0x47;
    case Qt::Key_H: return 0x48;
    case Qt::Key_I: return 0x49;
    case Qt::Key_J: return 0x4A;
    case Qt::Key_K: return 0x4B;
    case Qt::Key_L: return 0x4C;
    case Qt::Key_M: return 0x4D;
    case Qt::Key_N: return 0x4E;
    case Qt::Key_O: return 0x4F;
    case Qt::Key_P: return 0x50;
    case Qt::Key_Q: return 0x51;
    case Qt::Key_R: return 0x52;
    case Qt::Key_S: return 0x53;
    case Qt::Key_T: return 0x54;
    case Qt::Key_U: return 0x55;
    case Qt::Key_V: return 0x56;
    case Qt::Key_W: return 0x57;
    case Qt::Key_X: return 0x58;
    case Qt::Key_Y: return 0x59;
    case Qt::Key_Z: return 0x5A;
    case Qt::Key_F1: return VK_F1;
    case Qt::Key_F2: return VK_F2;
    case Qt::Key_F3: return VK_F3;
    case Qt::Key_F4: return VK_F4;
    case Qt::Key_F5: return VK_F5;
    case Qt::Key_F6: return VK_F6;
    case Qt::Key_F7: return VK_F7;
    case Qt::Key_F8: return VK_F8;
    case Qt::Key_F9: return VK_F9;
    case Qt::Key_F10: return VK_F10;
    case Qt::Key_F11: return VK_F11;
    case Qt::Key_F12: return VK_F12;
    default: return 0;
    }
}

// Update the KeyboardProc function
LRESULT CALLBACK QuickSoundSwitcher::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT pKeyboard = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // Handle volume keys first
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
                // Check for custom shortcuts
                if (instance->settings.value("globalShortcutsEnabled", true).toBool()) {
                    instance->handleCustomShortcut(pKeyboard->vkCode);
                }
                break;
            }
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// In src/quicksoundswitcher.cpp - update handleCustomShortcut:
void QuickSoundSwitcher::handleCustomShortcut(DWORD vkCode)
{
    // Check if global shortcuts are enabled in settings
    if (!settings.value("globalShortcutsEnabled", true).toBool()) {
        return;
    }

    // Check if shortcuts are temporarily suspended (e.g., dialog open)
    if (SoundPanelBridge::instance() && SoundPanelBridge::instance()->areGlobalShortcutsSuspended()) {
        return;
    }

    // Panel shortcut
    int panelKey = qtKeyToVirtualKey(settings.value("panelShortcutKey", static_cast<int>(Qt::Key_S)).toInt());
    int panelModifiers = settings.value("panelShortcutModifiers", static_cast<int>(Qt::ControlModifier | Qt::ShiftModifier)).toInt();

    if (vkCode == panelKey && isModifierPressed(panelModifiers)) {
        togglePanel();
        return;
    }

    // ChatMix shortcut
    int chatMixKey = qtKeyToVirtualKey(settings.value("chatMixShortcutKey", static_cast<int>(Qt::Key_M)).toInt());
    int chatMixModifiers = settings.value("chatMixShortcutModifiers", static_cast<int>(Qt::ControlModifier | Qt::ShiftModifier)).toInt();

    if (vkCode == chatMixKey && isModifierPressed(chatMixModifiers)) {
        toggleChatMix();
        return;
    }
}

void QuickSoundSwitcher::toggleChatMix()
{
    if (!settings.value("activateChatmix", false).toBool()) {
        return;
    }

    if (SoundPanelBridge::instance()) {
        settings.sync();
        bool currentState = settings.value("chatMixEnabled", false).toBool();
        QString statusText = currentState ? tr("Disabled") : tr("Enabled");
        QString message = tr("ChatMix %1").arg(statusText);

        SoundPanelBridge::instance()->toggleChatMixFromShortcut(!currentState);

        if (settings.value("chatMixShortcutNotification", true).toBool()) {
            //trayIcon->showMessage(
            //    message,
            //    QString(),
            //    QSystemTrayIcon::Information,
            //    1000
            //    );

            SoundPanelBridge::instance()->requestChatMixNotification(message);
        }
    }
}

void QuickSoundSwitcher::adjustOutputVolume(bool up)
{
    // Use cached getter (non-blocking)
    int originalVolume = AudioManager::getPlaybackVolume();
    int newVolume;
    if (up) {
        newVolume = originalVolume + 2;
    } else {
        newVolume = originalVolume - 2;
    }

    newVolume = qMax(0, qMin(100, newVolume));

    // Use async for setting volume
    AudioManager::setPlaybackVolumeAsync(newVolume);

    // Use cached getter (non-blocking)
    if (AudioManager::getPlaybackMute()) {
        AudioManager::setPlaybackMuteAsync(false);
    }

    // Update tray icon immediately with new volume for responsiveness
    trayIcon->setIcon(QIcon(Utils::getIcon(1, newVolume, NULL)));

    if (SoundPanelBridge::instance()) {
        SoundPanelBridge::instance()->updateVolumeFromTray(newVolume);
    }
}

void QuickSoundSwitcher::toggleMuteWithKey()
{
    // Use cached getters (non-blocking)
    bool currentMuted = AudioManager::getPlaybackMute();
    bool newMuted = !currentMuted;
    int volume = AudioManager::getPlaybackVolume();

    // Use async for setting mute
    AudioManager::setPlaybackMuteAsync(newMuted);

    int volumeIcon;
    if (volume == 0 || newMuted) {
        volumeIcon = 0;
    } else {
        volumeIcon = volume;
    }

    // Update tray icon immediately for responsiveness
    trayIcon->setIcon(QIcon(Utils::getIcon(1, volumeIcon, NULL)));

    if (SoundPanelBridge::instance()) {
        SoundPanelBridge::instance()->updateMuteStateFromTray(newMuted);
    }
}

void QuickSoundSwitcher::togglePanel()
{
    if (!isPanelVisible) {
        showPanel();
    } else {
        hidePanel();
    }
}

void QuickSoundSwitcher::showPanel()
{
    if (!isPanelVisible && panelWindow) {
        if (SoundPanelBridge::instance()) {
            // Start loading data but don't show panel yet
            SoundPanelBridge::instance()->initializeData();

            isPanelVisible = true;
            installGlobalMouseHook();

            // Panel will animate in when data is ready (handled by onDataInitializationComplete)
        }
    }
}

void QuickSoundSwitcher::onDataInitializationComplete()
{
    if (panelWindow && isPanelVisible) {
        QMetaObject::invokeMethod(panelWindow, "showPanel");
    }
}

void QuickSoundSwitcher::hidePanel()
{
    if (panelWindow && isPanelVisible) {
        QMetaObject::invokeMethod(panelWindow, "hidePanel");
    }
}

void QuickSoundSwitcher::onPanelHideAnimationFinished()
{
    isPanelVisible = false;
    uninstallGlobalMouseHook();
    // No need to destroy engine anymore - just keep it hidden
}

void QuickSoundSwitcher::onLanguageChanged()
{
    if (engine) {
        engine->retranslate();
    }

    updateTrayMenu();
}

void QuickSoundSwitcher::updateTrayMenu()
{
    if (!trayIcon || !trayIcon->contextMenu()) {
        return;
    }

    // Get the current menu
    QMenu *trayMenu = trayIcon->contextMenu();

    // Update the Exit action text
    QList<QAction*> actions = trayMenu->actions();
    for (QAction* action : actions) {
        if (action->text().contains("Exit") || action->text().contains("Quitter") ||
            action->text().contains("Esci") || action->text().contains("Beenden")) {
            action->setText(tr("Exit"));
            break;
        }
    }

    // Update device info actions text
    if (outputDeviceAction) {
        QString currentText = outputDeviceAction->text();
        // Extract the device info part and rebuild with translated prefix
        int colonIndex = currentText.indexOf(':');
        if (colonIndex != -1) {
            QString deviceInfo = currentText.mid(colonIndex);
            outputDeviceAction->setText(tr("Output") + deviceInfo);
        }
    }

    if (inputDeviceAction) {
        QString currentText = inputDeviceAction->text();
        int colonIndex = currentText.indexOf(':');
        if (colonIndex != -1) {
            QString deviceInfo = currentText.mid(colonIndex);
            inputDeviceAction->setText(tr("Input") + deviceInfo);
        }
    }
}

void QuickSoundSwitcher::openLegacySoundSettings() {
    QProcess::startDetached("control", QStringList() << "mmsys.cpl");
}

void QuickSoundSwitcher::openModernSoundSettings() {
    QProcess::startDetached("explorer", QStringList() << "ms-settings:sound");
}

void QuickSoundSwitcher::setupLocalServer()
{
    localServer = new QLocalServer(this);

    // Remove any existing server (in case of unclean shutdown)
    QLocalServer::removeServer("QuickSoundSwitcher");

    if (!localServer->listen("QuickSoundSwitcher")) {
        qWarning() << "Failed to create local server:" << localServer->errorString();
        return;
    }

    connect(localServer, &QLocalServer::newConnection,
            this, &QuickSoundSwitcher::onNewConnection);

    qDebug() << "Local server started successfully";
}

void QuickSoundSwitcher::cleanupLocalServer()
{
    if (localServer) {
        localServer->close();
        QLocalServer::removeServer("QuickSoundSwitcher");
        delete localServer;
        localServer = nullptr;
    }
}

void QuickSoundSwitcher::onNewConnection()
{
    QLocalSocket* clientSocket = localServer->nextPendingConnection();
    if (!clientSocket) {
        return;
    }

    connect(clientSocket, &QLocalSocket::readyRead, this, [this, clientSocket]() {
        QByteArray data = clientSocket->readAll();
        QString message = QString::fromUtf8(data);

        qDebug() << "Received message from new instance:" << message;

        if (message == "show_panel") {
            // Show the panel
            showPanel();
        }

        clientSocket->disconnectFromServer();
    });

    connect(clientSocket, &QLocalSocket::disconnected,
            this, &QuickSoundSwitcher::onClientDisconnected);
}

void QuickSoundSwitcher::onClientDisconnected()
{
    QLocalSocket* clientSocket = qobject_cast<QLocalSocket*>(sender());
    if (clientSocket) {
        clientSocket->deleteLater();
    }
}
