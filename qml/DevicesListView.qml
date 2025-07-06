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

    // Signals
    signal deviceClicked(string name, string shortName, int index)

    Layout.fillWidth: true
    Layout.preferredHeight: 0
    Layout.leftMargin: -14
    Layout.rightMargin: -14
    color: Constants.footerColor

    Behavior on Layout.preferredHeight {
        NumberAnimation {
            duration: 150
            easing.type: Easing.OutQuad
        }
    }

    // Top border
    Rectangle {
        visible: root.expanded
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: Constants.footerBorderColor
        height: 1
        opacity: 0.1
    }

    // Bottom border
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
            width: devicesList.width
            height: 40
            required property var model
            required property string name
            required property string shortName
            required property bool isDefault
            required property string id
            required property int index

            highlighted: model.isDefault
            text: UserSettings.deviceShortName ? model.shortName : model.name

            onClicked: {
                // Update the model to mark this device as default
                for (let i = 0; i < devicesList.model.count; i++) {
                    devicesList.model.setProperty(i, "isDefault", i === index)
                }

                // Emit signal to parent
                root.deviceClicked(name, shortName, index)
            }
        }
    }

    Timer {
        id: opacityTimer
        interval: 112
        repeat: false
        onTriggered: devicesList.opacity = 1
    }

    onExpandedChanged: {
        if (expanded) {
            Layout.preferredHeight = devicesList.contentHeight + 20
            opacityTimer.start()
        } else {
            Layout.preferredHeight = 0
            devicesList.opacity = 0
        }
    }
}
