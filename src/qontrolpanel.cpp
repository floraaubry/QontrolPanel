#include "qontrolpanel.h"
#include "soundpanelbridge.h"
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

HHOOK QontrolPanel::keyboardHook = NULL;
HHOOK QontrolPanel::mouseHook = NULL;
QontrolPanel* QontrolPanel::instance = nullptr;
static bool validMouseDown = false;

QontrolPanel::QontrolPanel(QWidget *parent)
    : QWidget(parent)
    , engine(nullptr)
    , panelWindow(nullptr)
    , settings("Odizinne", "QontrolPanel")
    , localServer(nullptr)
{
    MediaSessionManager::initialize();
    instance = this;
    initializeQMLEngine();
    setupLocalServer();

    if (settings.value("globalShortcutsEnabled").toBool()) {
        installKeyboardHook();
    }

    if (SoundPanelBridge::instance()) {
        connect(SoundPanelBridge::instance(), &SoundPanelBridge::languageChanged,
                this, &QontrolPanel::onLanguageChanged);
    }
}

QontrolPanel::~QontrolPanel()
{
    MediaSessionManager::cleanup();
    uninstallGlobalMouseHook();
    uninstallKeyboardHook();
    destroyQMLEngine();
    cleanupLocalServer();
    instance = nullptr;
}

void QontrolPanel::initializeQMLEngine()
{
    if (engine) {
        return;
    }

    engine = new QQmlApplicationEngine(this);
    engine->loadFromModule("Odizinne.QontrolPanel", "Main");

    if (!engine->rootObjects().isEmpty()) {
        panelWindow = qobject_cast<QWindow*>(engine->rootObjects().first());
        if (panelWindow) {
            panelWindow->setProperty("visible", false);

            connect(panelWindow, &QWindow::visibleChanged,
                    this, &QontrolPanel::onPanelVisibilityChanged);

            connect(panelWindow, SIGNAL(globalShortcutsToggled(bool)),
                    this, SLOT(onGlobalShortcutsToggled(bool)));
        }
    }
}

void QontrolPanel::onGlobalShortcutsToggled(bool enabled)
{
    if (enabled) {
        installKeyboardHook();
    } else {
        uninstallKeyboardHook();
    }
}

void QontrolPanel::onPanelVisibilityChanged(bool visible)
{
    isPanelVisible = visible;

    if (visible) {
        installGlobalMouseHook();
    } else {
        uninstallGlobalMouseHook();
    }
}

void QontrolPanel::destroyQMLEngine()
{
    if (engine) {
        engine->deleteLater();
        engine = nullptr;
    }
    panelWindow = nullptr;
}

QString QontrolPanel::elideDeviceText(const QString& deviceName, int volume, bool muted)
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

void QontrolPanel::installGlobalMouseHook()
{
    if (mouseHook == NULL) {
        mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
    }
}

void QontrolPanel::uninstallGlobalMouseHook()
{
    if (mouseHook != NULL) {
        UnhookWindowsHookEx(mouseHook);
        mouseHook = NULL;
    }
}

void QontrolPanel::installKeyboardHook()
{
    if (keyboardHook == NULL) {
        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    }
}

void QontrolPanel::uninstallKeyboardHook()
{
    if (keyboardHook != NULL) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = NULL;
    }
}

LRESULT CALLBACK QontrolPanel::MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) {
            QPoint cursorPos = QCursor::pos();
            QRect soundPanelRect;

            if (instance->panelWindow && instance->isPanelVisible) {
                soundPanelRect = instance->panelWindow->geometry();
            } else {
                soundPanelRect = QRect();
            }

            validMouseDown = !soundPanelRect.contains(cursorPos);
        }
        else if ((wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) && validMouseDown) {
            QPoint cursorPos = QCursor::pos();
            QRect soundPanelRect;

            if (instance->panelWindow && instance->isPanelVisible) {
                soundPanelRect = instance->panelWindow->geometry();
            } else {
                soundPanelRect = QRect();
            }

            validMouseDown = false;

            if (!soundPanelRect.contains(cursorPos)) {
                QMetaObject::invokeMethod(instance->panelWindow, "hidePanel");
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

bool QontrolPanel::isModifierPressed(int qtModifier)
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

int QontrolPanel::qtKeyToVirtualKey(int qtKey)
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

LRESULT CALLBACK QontrolPanel::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT pKeyboard = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (instance->settings.value("globalShortcutsEnabled", true).toBool()) {
                if (instance->handleCustomShortcut(pKeyboard->vkCode)) {
                    return 1; // Block the key event
                }
            }
        }
    }

    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

bool QontrolPanel::handleCustomShortcut(DWORD vkCode)
{
    if (!settings.value("globalShortcutsEnabled", true).toBool()) {
        return false;
    }

    if (SoundPanelBridge::instance() && SoundPanelBridge::instance()->areGlobalShortcutsSuspended()) {
        return false;
    }

    int panelKey = qtKeyToVirtualKey(settings.value("panelShortcutKey", static_cast<int>(Qt::Key_S)).toInt());
    int panelModifiers = settings.value("panelShortcutModifiers", static_cast<int>(Qt::ControlModifier | Qt::ShiftModifier)).toInt();

    if (vkCode == panelKey && isModifierPressed(panelModifiers)) {
        togglePanel();
        return true;
    }

    int chatMixKey = qtKeyToVirtualKey(settings.value("chatMixShortcutKey", static_cast<int>(Qt::Key_M)).toInt());
    int chatMixModifiers = settings.value("chatMixShortcutModifiers", static_cast<int>(Qt::ControlModifier | Qt::ShiftModifier)).toInt();

    if (vkCode == chatMixKey && isModifierPressed(chatMixModifiers)) {
        toggleChatMix();
        return true;
    }

    return false;
}

void QontrolPanel::toggleChatMix()
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
            SoundPanelBridge::instance()->requestChatMixNotification(message);
        }
    }
}

void QontrolPanel::togglePanel()
{
    if (!panelWindow) return;

    if (!isPanelVisible) {
        QMetaObject::invokeMethod(panelWindow, "showPanel");
    } else {
        QMetaObject::invokeMethod(panelWindow, "hidePanel");
    }
}

void QontrolPanel::onLanguageChanged()
{
    if (engine) {
        engine->retranslate();
    }
}

void QontrolPanel::setupLocalServer()
{
    localServer = new QLocalServer(this);

    // Remove any existing server (in case of unclean shutdown)
    QLocalServer::removeServer("QontrolPanel");

    if (!localServer->listen("QontrolPanel")) {
        qWarning() << "Failed to create local server:" << localServer->errorString();
        return;
    }

    connect(localServer, &QLocalServer::newConnection,
            this, &QontrolPanel::onNewConnection);
}

void QontrolPanel::cleanupLocalServer()
{
    if (localServer) {
        localServer->close();
        QLocalServer::removeServer("QontrolPanel");
        delete localServer;
        localServer = nullptr;
    }
}

void QontrolPanel::onNewConnection()
{
    QLocalSocket* clientSocket = localServer->nextPendingConnection();
    if (!clientSocket) {
        return;
    }

    connect(clientSocket, &QLocalSocket::readyRead, this, [this, clientSocket]() {
        QByteArray data = clientSocket->readAll();
        QString message = QString::fromUtf8(data);

        if (message == "show_panel") {
            if (panelWindow) {
                if (SoundPanelBridge::instance()) {
                    QMetaObject::invokeMethod(panelWindow, "showPanel");
                }
            }
        }

        clientSocket->disconnectFromServer();
    });

    connect(clientSocket, &QLocalSocket::disconnected,
            this, &QontrolPanel::onClientDisconnected);
}

void QontrolPanel::onClientDisconnected()
{
    QLocalSocket* clientSocket = qobject_cast<QLocalSocket*>(sender());
    if (clientSocket) {
        clientSocket->deleteLater();
    }
}
