import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

ColumnLayout {
    spacing: 3

    Label {
        text: qsTr("Communication Apps")
        font.pixelSize: 22
        font.bold: true
        Layout.bottomMargin: 15
    }

    Connections {
        target: SoundPanelBridge
        function onApplicationsChanged() {
            SoundPanelBridge.updateMissingCommAppIcons()
        }
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true

        Column {
            width: parent.width
            spacing: 3

            Card {
                width: parent.width
                title: qsTr("Activate ChatMix")

                additionalControl: Switch {
                    id: activateChatMixSwitch
                    checked: UserSettings.activateChatmix
                    onClicked: {
                        if (checked) {
                            chatMixWarningDialog.open()
                        } else {
                            UserSettings.activateChatmix = checked
                            UserSettings.chatMixEnabled = checked
                            SoundPanelBridge.restoreOriginalVolumes()
                        }
                    }
                }
            }

            Card {
                show: UserSettings.activateChatmix
                width: parent.width
                title: qsTr("Enable ChatMix")
                description: qsTr("Control communication apps separately from other applications")

                additionalControl: Switch {
                    checked: UserSettings.chatMixEnabled
                    onClicked: {
                        if (checked) {
                            UserSettings.chatMixEnabled = checked
                            SoundPanelBridge.applyChatMixToApplications()
                        } else {
                            UserSettings.chatMixEnabled = checked
                            SoundPanelBridge.restoreOriginalVolumes()
                        }
                    }
                }
            }

            Card {
                show: UserSettings.activateChatmix
                width: parent.width
                title: qsTr("ChatMix volume")
                description: qsTr("The volume to set for non communication applications when ChatMix is enabled")
                enabled: UserSettings.chatMixEnabled

                additionalControl: Slider {
                    value: UserSettings.chatMixValue
                    from: 0
                    to: 100
                    Layout.fillWidth: true
                    enabled: UserSettings.chatMixEnabled

                    onValueChanged: {
                        UserSettings.chatMixValue = value
                    }

                    onPressedChanged: {
                        if (pressed) return

                        SoundPanelBridge.chatMixValue = UserSettings.chatMixValue
                        if (UserSettings.chatMixEnabled) {
                            SoundPanelBridge.applyChatMixToApplications()
                        }
                    }
                }
            }

            Card {
                show: UserSettings.activateChatmix
                width: parent.width
                title: qsTr("Restored volume")
                description: qsTr("The volume to set for applications when ChatMix is disabled")
                enabled: UserSettings.chatMixEnabled

                additionalControl: Slider {
                    id: chatMixSlider
                    value: UserSettings.chatmixRestoreVolume
                    from: 0
                    to: 100
                    Layout.fillWidth: true

                    onValueChanged: {
                        UserSettings.chatmixRestoreVolume = value
                    }
                }
            }

            Card {
                show: UserSettings.activateChatmix
                width: parent.width
                title: qsTr("Communication Applications")
                description: qsTr("Add application names that should be treated as communication apps")

                additionalControl: Button {
                    text: qsTr("Add App")
                    onClicked: addAppDialog.open()
                }
            }

            Repeater {
                model: SoundPanelBridge.commAppsList
                Card {
                    show: UserSettings.activateChatmix
                    id: appCard
                    required property var model
                    width: parent.width
                    title: model.name
                    iconSource: model.icon
                    imageMode: true
                    iconWidth: 20
                    iconHeight: 20
                    additionalControl: Button {
                        text: qsTr("Remove")
                        onClicked: {
                            SoundPanelBridge.removeCommApp(appCard.model.name)
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: chatMixWarningDialog
        title: qsTr("Enable ChatMix Warning")
        modal: true
        width: 400
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 20

            Label {
                text: qsTr("Activating ChatMix will initially set all non communication application volumes to 50%. This might cause loud audio output.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("It is recommended to lower your master volume before proceeding to avoid sudden loud sounds.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                font.bold: true
                color: "orange"
            }

            RowLayout {
                spacing: 15
                Layout.topMargin: 10

                Button {
                    text: qsTr("Cancel")
                    onClicked: {
                        activateChatMixSwitch.checked = false
                        chatMixWarningDialog.close()
                    }
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Activate")
                    highlighted: true
                    Layout.fillWidth: true
                    onClicked: {
                        UserSettings.activateChatmix = true
                        UserSettings.chatMixEnabled = true
                        SoundPanelBridge.applyChatMixToApplications()
                        chatMixWarningDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: addAppDialog
        title: qsTr("Add Communication App")
        modal: true
        width: 350
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            Label {
                text: qsTr("Enter application name (e.g., Discord)")
            }

            TextField {
                id: executableField
                Layout.fillWidth: true
                placeholderText: qsTr("Discord")
            }

            RowLayout {
                spacing: 15
                Button {
                    text: qsTr("Cancel")
                    onClicked: addAppDialog.close()
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Add")
                    enabled: executableField.text.length > 0
                    Layout.fillWidth: true
                    highlighted: true
                    onClicked: {
                        SoundPanelBridge.addCommApp(executableField.text)
                        executableField.text = ""
                        addAppDialog.close()
                    }
                }
            }
        }
    }
}
