pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

ColumnLayout {
    spacing: 3
    Label {
        text: qsTr("Components")
        font.pixelSize: 22
        font.bold: true
        Layout.bottomMargin: 15
    }
    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
        CustomScrollView {
            anchors.fill: parent
            ColumnLayout {
                width: parent.width
                spacing: 3

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Enable audio device manager")
                    description: qsTr("Display audio device manager in panel")
                    enabled: UserSettings.enableApplicationMixer || UserSettings.allowBrightnessControl
                    additionalControl: LabeledSwitch {
                        enabled: UserSettings.enableApplicationMixer || UserSettings.allowBrightnessControl
                        checked: UserSettings.enableDeviceManager
                        onClicked: UserSettings.enableDeviceManager = checked
                    }
                }

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Enable application volume mixer")
                    description: qsTr("Display application volume mixer in panel")
                    enabled: UserSettings.enableDeviceManager || UserSettings.allowBrightnessControl
                    additionalControl: LabeledSwitch {
                        enabled: UserSettings.enableDeviceManager || UserSettings.allowBrightnessControl
                        checked: UserSettings.enableApplicationMixer
                        onClicked: UserSettings.enableApplicationMixer = checked
                    }
                }

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Enable brightness control")
                    description: qsTr("Show brightness control in panel")
                    enabled: UserSettings.enableDeviceManager || UserSettings.enableApplicationMixer
                    additionalControl: LabeledSwitch {
                        enabled: UserSettings.enableDeviceManager || UserSettings.enableApplicationMixer
                        checked: UserSettings.allowBrightnessControl
                        onClicked: UserSettings.allowBrightnessControl = checked
                    }
                }

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Enable power menu")
                    description: qsTr("Show power button in the panel footer")
                    additionalControl: LabeledSwitch {
                        checked: UserSettings.enablePowerMenu
                        onClicked: UserSettings.enablePowerMenu = checked
                    }
                }

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Enable HeadsetControl integration")
                    description: qsTr("Monitor battery using HeadsetControl for supported devices")
                    additionalControl: LabeledSwitch {
                        checked: UserSettings.headsetcontrolMonitoring
                        onClicked: UserSettings.headsetcontrolMonitoring = checked
                    }
                }
            }
        }
    }
}
