pragma Singleton

import QtQuick
import Odizinne.QontrolPanel

QtObject {
    signal easterEggRequested()
    property string toggleShortcut: getShortcutText(UserSettings.panelShortcutModifiers, UserSettings.panelShortcutKey)
    property string chatMixShortcut: getShortcutText(UserSettings.chatMixShortcutModifiers, UserSettings.chatMixShortcutKey)

    function getKeyText(key) {
        const keyMap = {
            [Qt.Key_A]: "A", [Qt.Key_B]: "B", [Qt.Key_C]: "C", [Qt.Key_D]: "D",
            [Qt.Key_E]: "E", [Qt.Key_F]: "F", [Qt.Key_G]: "G", [Qt.Key_H]: "H",
            [Qt.Key_I]: "I", [Qt.Key_J]: "J", [Qt.Key_K]: "K", [Qt.Key_L]: "L",
            [Qt.Key_M]: "M", [Qt.Key_N]: "N", [Qt.Key_O]: "O", [Qt.Key_P]: "P",
            [Qt.Key_Q]: "Q", [Qt.Key_R]: "R", [Qt.Key_S]: "S", [Qt.Key_T]: "T",
            [Qt.Key_U]: "U", [Qt.Key_V]: "V", [Qt.Key_W]: "W", [Qt.Key_X]: "X",
            [Qt.Key_Y]: "Y", [Qt.Key_Z]: "Z",
            [Qt.Key_F1]: "F1", [Qt.Key_F2]: "F2", [Qt.Key_F3]: "F3", [Qt.Key_F4]: "F4",
            [Qt.Key_F5]: "F5", [Qt.Key_F6]: "F6", [Qt.Key_F7]: "F7", [Qt.Key_F8]: "F8",
            [Qt.Key_F9]: "F9", [Qt.Key_F10]: "F10", [Qt.Key_F11]: "F11", [Qt.Key_F12]: "F12"
        }
        return keyMap[key] || "Unknown"
    }

    function getShortcutText(modifiers, key) {
        let parts = []

        if (modifiers & Qt.ControlModifier) parts.push("Ctrl")
        if (modifiers & Qt.ShiftModifier) parts.push("Shift")
        if (modifiers & Qt.AltModifier) parts.push("Alt")

        let keyText = getKeyText(key)
        if (keyText) parts.push(keyText)

        return parts.join(" + ")
    }
}
