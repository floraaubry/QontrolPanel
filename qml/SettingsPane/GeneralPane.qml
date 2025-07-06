import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

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
                title: qsTr("Panel mode")
                description: qsTr("Choose what should be displayed in the panel")

                additionalControl: ComboBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    model: [qsTr("Devices + Mixer"), qsTr("Mixer only"), qsTr("Devices only")]
                    currentIndex: UserSettings.panelMode
                    onActivated: UserSettings.panelMode = currentIndex
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Link same input and output devices")
                description: qsTr("Try to match input / output from the same device")

                additionalControl: Switch {
                    checked: UserSettings.linkIO
                    onClicked: UserSettings.linkIO = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Sound keepalive")
                description: qsTr("Emit an inaudible sound to keep bluetooth devices awake")

                additionalControl: Switch {
                    checked: UserSettings.keepAlive
                    onClicked: UserSettings.keepAlive = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Run at system startup")
                description: qsTr("QSS will boot up when your computer starts")

                additionalControl: Switch {
                    checked: SoundPanelBridge.getShortcutState()
                    onClicked: SoundPanelBridge.setStartupShortcut(checked)
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Close device list automatically")
                description: qsTr("Device list will automatically close after selecting a device")

                additionalControl: Switch {
                    checked: UserSettings.closeDeviceListOnClick
                    onClicked: UserSettings.closeDeviceListOnClick = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Group applications by executable")
                description: qsTr("Control multiple stream comming from a single app with one slider")

                additionalControl: Switch {
                    checked: UserSettings.groupApplications
                    onClicked: UserSettings.groupApplications = checked
                }
            }
        }
    }
}
