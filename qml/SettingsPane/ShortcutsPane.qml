pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

ColumnLayout {
    id: lyt
    spacing: 3

    Label {
        text: qsTr("Global Shortcuts")
        font.pixelSize: 22
        font.bold: true
        Layout.bottomMargin: 15
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ScrollView {
            anchors.fill: parent
            ColumnLayout {
                width: parent.width
                spacing: 3

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Enable global shortcuts")
                    description: qsTr("Allow QontrolPanel to respond to keyboard shortcuts globally")

                    additionalControl: LabeledSwitch {
                        checked: UserSettings.globalShortcutsEnabled
                        onClicked: UserSettings.globalShortcutsEnabled = checked
                    }
                }

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Notification on ChatMix toggle")
                    additionalControl: LabeledSwitch {
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
                            text: Context.toggleShortcut
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
                            text: Context.chatMixShortcut
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
                        text: Context.getShortcutText(shortcutDialog.tempModifiers, shortcutDialog.tempKey)
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
}
