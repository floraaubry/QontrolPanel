pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

Rectangle {
    id: root

    property alias model: devicesList.model
    property bool expanded: false
    property real contentOpacity: 0

    property real expandedNeededHeight: devicesList.contentHeight + 20

    signal deviceClicked(string name, int index)

    Layout.fillWidth: true
    Layout.preferredHeight: expanded ? expandedNeededHeight : 0
    Layout.leftMargin: -14
    Layout.rightMargin: -14
    color: Constants.footerColor

    function closeContextMenus() {
        if (deviceRenameContextMenu.visible) {
            deviceRenameContextMenu.close()
        }
        if (deviceIconDialog.visible) {
            deviceIconDialog.close()
        }
    }

    Connections {
        target: AudioBridge
        function onDeviceRenameUpdated() {
            devicesList.forceLayout()
        }
        function onDeviceIconUpdated() {
            devicesList.forceLayout()
        }
    }

    Behavior on Layout.preferredHeight {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    Rectangle {
        visible: root.expanded
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: Constants.footerBorderColor
        height: 1
        opacity: 0.1
    }

    Rectangle {
        visible: root.expanded
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: Constants.footerBorderColor
        height: 1
        opacity: 0.1
    }

    ListView {
        id: devicesList
        anchors.fill: parent
        anchors.margins: 10
        clip: true
        interactive: false
        opacity: root.contentOpacity

        Behavior on opacity {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutQuad
            }
        }

        delegate: ItemDelegate {
            id: del
            width: devicesList.width
            height: 40
            required property var model
            required property string name
            required property string description
            required property bool isDefault
            required property string deviceId
            required property int index

            highlighted: model.isDefault
            icon.source: UserSettings.deviceIcon ? AudioBridge.getDisplayIconForDevice(model.name || "", model.isInput || false) : ""
            icon.width: 14
            icon.height: 14
            spacing: 6
            text: AudioBridge.getDisplayNameForDevice(model.name || "")
            rightPadding: batteryImage.visible ? (batteryImage.width + 30) : 15

            Image {
                id: batteryImage
                anchors.right: del.right
                anchors.verticalCenter: del.verticalCenter
                anchors.rightMargin: 15

                property int batteryLevel: del.model.batteryPercentage
                property string batteryStatus: del.model.batteryStatus

                visible: UserSettings.headsetcontrolMonitoring && batteryLevel !== -1 && batteryStatus !== "BATTERY_UNAVAILABLE"
                sourceSize.height: 16
                sourceSize.width: 16
                source: {
                    if (batteryStatus === "BATTERY_CHARGING") {
                        return Constants.getBatteryChargingIcon()
                    } else {
                        return Constants.getBatteryIcon(batteryLevel)
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function(mouse) {
                    if (mouse.button === Qt.LeftButton) {
                        root.deviceClicked(del.model.name, del.index)
                    } else if (mouse.button === Qt.RightButton) {
                        deviceRenameContextMenu.originalName = del.model.name || ""
                        deviceRenameContextMenu.currentCustomName = AudioBridge.getCustomDeviceName(del.model.name || "")
                        deviceRenameContextMenu.isInput = del.model.isInput || false
                        deviceRenameContextMenu.popup()
                    }
                }
            }
        }
    }

    Timer {
        id: opacityTimer
        interval: 112
        repeat: false
        onTriggered: root.contentOpacity = 1
    }

    onExpandedChanged: {
        if (expanded) {
            if (UserSettings.opacityAnimations) {
                opacityTimer.start()
            } else {
                root.contentOpacity = 1
            }
        } else {
            if (UserSettings.opacityAnimations) {
                root.contentOpacity = 0
            } else {
                root.contentOpacity = 0
            }
        }
    }

    Menu {
        id: deviceRenameContextMenu

        property string originalName: ""
        property string currentCustomName: ""
        property bool isInput: false

        MenuItem {
            text: qsTr("Rename Device")
            onTriggered: deviceRenameDialog.open()
        }

        MenuItem {
            text: qsTr("Change Icon")
            enabled: UserSettings.deviceIcon
            onTriggered: deviceIconDialog.open()
        }

        MenuItem {
            text: qsTr("Reset to Original Name")
            enabled: deviceRenameContextMenu.currentCustomName !== deviceRenameContextMenu.originalName
            onTriggered: {
                AudioBridge.setCustomDeviceName(deviceRenameContextMenu.originalName, "")
            }
        }
    }

    Dialog {
        id: deviceRenameDialog
        title: qsTr("Rename Device")
        modal: true
        width: 300
        dim: false
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            TextField {
                id: deviceCustomNameField
                Layout.fillWidth: true
                placeholderText: deviceRenameContextMenu.originalName

                Keys.onReturnPressed: {
                    AudioBridge.setCustomDeviceName(
                        deviceRenameContextMenu.originalName,
                        deviceCustomNameField.text.trim()
                        )
                    deviceRenameDialog.close()
                }
            }

            RowLayout {
                spacing: 15
                Layout.topMargin: 10

                Button {
                    text: qsTr("Cancel")
                    onClicked: deviceRenameDialog.close()
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Save")
                    highlighted: true
                    Layout.fillWidth: true
                    onClicked: {
                        AudioBridge.setCustomDeviceName(
                            deviceRenameContextMenu.originalName,
                            deviceCustomNameField.text.trim()
                            )
                        deviceRenameDialog.close()
                    }
                }
            }
        }

        onVisibleChanged: {
            if(!visible) return

            deviceCustomNameField.text = deviceRenameContextMenu.currentCustomName
            deviceCustomNameField.forceActiveFocus()
        }
    }

    Dialog {
        id: deviceIconDialog
        title: qsTr("Change Device Icon")
        modal: true
        width: 300
        dim: false
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            GridLayout {
                columns: 4
                rows: 2
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter

                property var iconNames: ["monitor", "microphone", "headset", "headset-mic", "speaker", "earbuds", "webcam", "gamepad"]

                Repeater {
                    model: parent.iconNames

                    delegate: Button {
                        required property string modelData
                        required property int index

                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40

                        icon.source: "qrc:/icons/devices/" + modelData + ".png"
                        icon.width: 24
                        icon.height: 24

                        onClicked: {
                            AudioBridge.setCustomDeviceIcon(deviceRenameContextMenu.originalName, modelData)
                            deviceIconDialog.close()
                        }

                        ToolTip.text: modelData.charAt(0).toUpperCase() + modelData.slice(1).replace('-', ' ')
                        ToolTip.visible: hovered
                    }
                }
            }

            Button {
                text: qsTr("Cancel")
                onClicked: deviceIconDialog.close()
                Layout.fillWidth: true
            }
        }
    }
}
