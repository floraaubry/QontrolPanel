pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

ColumnLayout {
    spacing: 3

    Label {
        text: HeadsetControlManager.deviceName
        font.pixelSize: 22
        font.bold: true
        visible: HeadsetControlManager.anyDeviceFound
    }

    ProgressBar {
        Layout.bottomMargin: 15
        Layout.fillWidth: true
        from: 0
        to: 100
        value: HeadsetControlManager.batteryLevel
        indeterminate: HeadsetControlManager.batteryStatus === "BATTERY_CHARGING"
        visible: HeadsetControlManager.batteryStatus !== "BATTERY_UNAVAILABLE" && HeadsetControlManager.anyDeviceFound
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
                    visible: HeadsetControlManager.anyDeviceFound
                    enabled: HeadsetControlManager.hasLightsCapability
                    Layout.fillWidth: true
                    title: qsTr("Headset Lighting")
                    description: qsTr("Toggle RGB lights on your headset")

                    additionalControl: Switch {
                        checked: UserSettings.headsetcontrolLights
                        onClicked:{
                            UserSettings.headsetcontrolLights = checked
                            HeadsetControlManager.setLights(checked)
                        }
                    }
                }

                Card {
                    visible: HeadsetControlManager.anyDeviceFound
                    enabled: HeadsetControlManager.hasSidetoneCapability
                    Layout.fillWidth: true
                    title: qsTr("Microphone Sidetone")
                    description: qsTr("Adjust your voice feedback level")
                    additionalControl: Slider {
                        from: 0
                        to: 128
                        value: UserSettings.headsetcontrolSidetone
                        onPressedChanged: {
                            if (!pressed) {
                                UserSettings.headsetcontrolSidetone = Math.round(value)
                                HeadsetControlManager.setSidetone(Math.round(value))
                            }
                        }
                    }
                }
            }
        }

        Label {
            anchors.centerIn: parent
            opacity: 0.5
            text: UserSettings.headsetcontrolMonitoring ? qsTr("No compatible device found.") : qsTr("HeadsetControl monitoring is disabled\nYou can enable it in the General tab.")
            visible: !HeadsetControlManager.anyDeviceFound || !UserSettings.headsetcontrolMonitoring
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
