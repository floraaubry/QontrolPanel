// qml/SettingsPane/ShortcutsPane.qml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

ColumnLayout {
    id: lyt
    spacing: 3

    Label {
        text: qsTr("Global Shortcuts")
        font.pixelSize: 22
        font.bold: true
        Layout.bottomMargin: 15
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        ColumnLayout {
            width: parent.width
            spacing: 3

            Card {
                Layout.fillWidth: true
                title: qsTr("Enable global shortcuts")
                description: qsTr("Allow QuickSoundSwitcher to respond to keyboard shortcuts globally")

                additionalControl: Switch {
                    checked: UserSettings.globalShortcutsEnabled
                    onClicked: UserSettings.globalShortcutsEnabled = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Notification on ChatMix toggle")
                additionalControl: Switch {
                    checked: UserSettings.chatMixShortcutNotification
                    onClicked: UserSettings.chatMixShortcutNotification = checked
                }
            }

            Card {
                enabled: UserSettings.globalShortcutsEnabled
                Layout.fillWidth: true
                title: qsTr("Show/Hide Panel")
                description: qsTr("Shortcut to toggle the main panel visibility")

                additionalControl: RowLayout {
                    spacing: 15

                    Label {
                        text: lyt.getShortcutText(UserSettings.panelShortcutModifiers, UserSettings.panelShortcutKey)
                        opacity: 0.7
                    }

                    Button {
                        text: qsTr("Change")
                        onClicked: {
                            shortcutDialog.openForPanel()
                        }
                    }
                }
            }

            Card {
                enabled: UserSettings.globalShortcutsEnabled
                Layout.fillWidth: true
                title: qsTr("Toggle ChatMix")
                description: qsTr("Shortcut to enable/disable ChatMix feature")

                additionalControl: RowLayout {
                    spacing: 15
                    Label {
                        text: lyt.getShortcutText(UserSettings.chatMixShortcutModifiers, UserSettings.chatMixShortcutKey)
                        opacity: 0.7
                    }

                    Button {
                        text: qsTr("Change")
                        onClicked: {
                            shortcutDialog.openForChatMix()
                        }
                    }
                }
            }
        }
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

    // Single reusable shortcut dialog
    Dialog {
        id: shortcutDialog
        modal: true
        width: 400
        anchors.centerIn: parent

        property string dialogTitle: ""
        property string shortcutType: "" // "panel" or "chatmix"
        property int tempModifiers: Qt.ControlModifier | Qt.ShiftModifier
        property int tempKey: Qt.Key_S

        title: dialogTitle

        onOpened: {
            SoundPanelBridge.suspendGlobalShortcuts()
            // Focus the input rectangle when dialog opens
            Qt.callLater(function() {
                inputRect.forceActiveFocus()
            })
        }

        onClosed: {
            SoundPanelBridge.resumeGlobalShortcuts()
        }

        function openForPanel() {
            dialogTitle = qsTr("Set Panel Shortcut")
            shortcutType = "panel"
            tempModifiers = UserSettings.panelShortcutModifiers
            tempKey = UserSettings.panelShortcutKey
            open()
        }

        function openForChatMix() {
            dialogTitle = qsTr("Set ChatMix Shortcut")
            shortcutType = "chatmix"
            tempModifiers = UserSettings.chatMixShortcutModifiers
            tempKey = UserSettings.chatMixShortcutKey
            open()
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            Label {
                text: qsTr("Press the desired key combination")
                Layout.fillWidth: true
            }

            Rectangle {
                id: inputRect
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                color: Constants.footerColor
                radius: 5
                focus: true

                Rectangle {
                    opacity: parent.focus ? 1 : 0.15
                    border.width: 1
                    border.color: parent.focus? palette.accent : Constants.footerBorderColor
                    color: Constants.footerColor
                    radius: 5
                    anchors.fill: parent

                    Behavior on opacity {
                        NumberAnimation {
                            duration: 200
                            easing.type: Easing.OutQuad
                        }
                    }

                    Behavior on border.color {
                        ColorAnimation {
                            duration: 200
                            easing.type: Easing.OutQuad
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    text: lyt.getShortcutText(shortcutDialog.tempModifiers, shortcutDialog.tempKey)
                    font.family: "Consolas, monospace"
                    font.pixelSize: 16
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: parent.forceActiveFocus()
                }

                Keys.onPressed: function(event) {
                    let modifiers = 0
                    if (event.modifiers & Qt.ControlModifier) modifiers |= Qt.ControlModifier
                    if (event.modifiers & Qt.ShiftModifier) modifiers |= Qt.ShiftModifier
                    if (event.modifiers & Qt.AltModifier) modifiers |= Qt.AltModifier

                    shortcutDialog.tempModifiers = modifiers
                    shortcutDialog.tempKey = event.key
                    event.accepted = true
                }
            }

            RowLayout {
                spacing: 15
                Button {
                    text: qsTr("Cancel")
                    onClicked: shortcutDialog.close()
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Apply")
                    highlighted: true
                    Layout.fillWidth: true
                    onClicked: {
                        if (shortcutDialog.shortcutType === "panel") {
                            UserSettings.panelShortcutModifiers = shortcutDialog.tempModifiers
                            UserSettings.panelShortcutKey = shortcutDialog.tempKey
                        } else if (shortcutDialog.shortcutType === "chatmix") {
                            UserSettings.chatMixShortcutModifiers = shortcutDialog.tempModifiers
                            UserSettings.chatMixShortcutKey = shortcutDialog.tempKey
                        }
                        shortcutDialog.close()
                    }
                }
            }
        }
    }
}
