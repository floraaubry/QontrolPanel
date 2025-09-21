import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

ColumnLayout {
    spacing: 3

    Label {
        text: qsTr("General Settings")
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
                title: qsTr("Sound keepalive")
                description: qsTr("Emit an inaudible sound to keep bluetooth devices awake")

                additionalControl: LabeledSwitch {
                    checked: UserSettings.keepAlive
                    onClicked: UserSettings.keepAlive = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Run at system startup")
                description: qsTr("QSS will boot up when your computer starts")

                additionalControl: LabeledSwitch {
                    checked: SoundPanelBridge.getShortcutState()
                    onClicked: SoundPanelBridge.setStartupShortcut(checked)
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Close device list automatically")
                description: qsTr("Device list will automatically close after selecting a device")

                additionalControl: LabeledSwitch {
                    checked: UserSettings.closeDeviceListOnClick
                    onClicked: UserSettings.closeDeviceListOnClick = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Show power action confirmation")
                additionalControl: LabeledSwitch {
                    checked: UserSettings.showPowerDialogConfirmation
                    onClicked: UserSettings.showPowerDialogConfirmation = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Power action confirmation timeout")
                additionalControl: SpinBox {
                    from: 5
                    to: 60
                    editable: true
                    value: UserSettings.confirmationTimeout
                    onValueChanged: UserSettings.confirmationTimeout = value
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("DDC/CI brightness update rate")
                description: qsTr("Controls how frequently brightness commands are sent to external monitors")
                additionalControl: ComboBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    model: [qsTr("Normal"), qsTr("Fast"), qsTr("Faster"), qsTr("Lightspeed")]
                    currentIndex: {
                        switch(UserSettings.ddcciQueueDelay) {
                            case 500: return 0
                            case 250: return 1
                            case 100: return 2
                            case 1: return 3
                            default: return 0
                        }
                    }
                    onActivated: {
                        switch(currentIndex) {
                            case 0: UserSettings.ddcciQueueDelay = 500; break
                            case 1: UserSettings.ddcciQueueDelay = 250; break
                            case 2: UserSettings.ddcciQueueDelay = 100; break
                            case 3: UserSettings.ddcciQueueDelay = 1; break
                        }
                    }
                }
            }
        }
    }
}
