pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

ColumnLayout {
    spacing: 3

    Label {
        text: HeadsetControlBridge.deviceName
        font.pixelSize: 22
        font.bold: true
        visible: HeadsetControlBridge.anyDeviceFound
    }

    ProgressBar {
        Layout.bottomMargin: 15
        Layout.fillWidth: true
        from: 0
        to: 100
        value: HeadsetControlBridge.batteryLevel
        indeterminate: HeadsetControlBridge.batteryStatus === "BATTERY_CHARGING"
        visible: HeadsetControlBridge.batteryStatus !== "BATTERY_UNAVAILABLE" && HeadsetControlBridge.anyDeviceFound
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
                    enabled: HeadsetControlBridge.hasLightsCapability
                    Layout.fillWidth: true
                    title: qsTr("Device battery")
                    visible: HeadsetControlBridge.batteryStatus !== "BATTERY_UNAVAILABLE" && HeadsetControlBridge.anyDeviceFound
                    additionalControl: Label {
                        text: HeadsetControlBridge.batteryStatus === "BATTERY_CHARGING" ? qsTr("Charging") : HeadsetControlBridge.batteryLevel + "%"
                    }
                }

                Card {
                    visible: HeadsetControlBridge.anyDeviceFound
                    enabled: HeadsetControlBridge.hasLightsCapability
                    Layout.fillWidth: true
                    title: qsTr("Headset Lighting")
                    description: qsTr("Toggle RGB lights on your headset")

                    additionalControl: LabeledSwitch {
                        checked: UserSettings.headsetcontrolLights
                        onClicked:{
                            UserSettings.headsetcontrolLights = checked
                            HeadsetControlBridge.setLights(checked)
                        }
                    }
                }

                Card {
                    visible: HeadsetControlBridge.anyDeviceFound
                    enabled: HeadsetControlBridge.hasSidetoneCapability
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
                                HeadsetControlBridge.setSidetone(Math.round(value))
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
            visible: !HeadsetControlBridge.anyDeviceFound || !UserSettings.headsetcontrolMonitoring
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
