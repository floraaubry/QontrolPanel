pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

Rectangle {
    id: root

    property alias model: applicationsList.model
    property bool expanded: false
    property real contentOpacity: 0
    property string executableName: ""

    // Export the height needed when fully expanded
    property real expandedNeededHeight: {
        if (applicationsList.count < 2 || !model) {
            return 0
        }
        return applicationsList.contentHeight + 20
    }

    // Signals
    signal applicationVolumeChanged(string appId, int volume)
    signal applicationMuteChanged(string appId, bool muted)

    Layout.fillWidth: true
    Layout.preferredHeight: expanded ? expandedNeededHeight : 0
    Layout.leftMargin: -14
    Layout.rightMargin: -14
    color: Constants.footerColor

    function closeContextMenus() {
        if (renameContextMenu.visible) {
            renameContextMenu.close()
        }
    }

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
        id: applicationsList
        anchors.fill: parent
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        clip: true
        interactive: false
        opacity: root.contentOpacity

        Behavior on opacity {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutQuad
            }
        }

        delegate: RowLayout {
            id: individualAppLayout
            width: applicationsList.width
            height: 40
            spacing: 0
            required property var model
            required property int index

            NFToolButton {
                id: muteButton
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                flat: !checked
                checkable: true
                highlighted: checked
                checked: individualAppLayout.model.isMuted
                ToolTip.text: individualAppLayout.model.name
                ToolTip.visible: hovered
                ToolTip.delay: 1000
                opacity: highlighted ? 0.3 : (enabled ? 1 : 0.5)
                icon.color: "transparent"
                icon.source: individualAppLayout.model.name == qsTr("System sounds") ? Constants.systemIcon : individualAppLayout.model.iconPath

                onClicked: {
                    let newMutedState = !individualAppLayout.model.isMuted
                    root.applicationMuteChanged(individualAppLayout.model.appId, newMutedState)
                }

                Component.onCompleted: {
                    palette.accent = palette.button
                }
            }

            ColumnLayout {
                spacing: -4

                Label {
                    visible: UserSettings.displayDevAppLabel
                    opacity: UserSettings.chatMixEnabled ? 0.3 : 0.5
                    elide: Text.ElideRight
                    Layout.preferredWidth: 200
                    Layout.leftMargin: 18
                    Layout.rightMargin: 25
                    text: {
                        let originalName = individualAppLayout.model.name
                        let streamIndex = individualAppLayout.model.streamIndex

                        if (originalName === "System sounds") {
                            return qsTr("System sounds")
                        }

                        // Get custom display name from AudioBridge
                        let displayName = AudioBridge.getDisplayNameForApplication(originalName, streamIndex)

                        // Add lock indicator
                        if (AudioBridge.isApplicationLocked(originalName, streamIndex)) {
                            displayName += " 🔒"
                        }

                        return displayName
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton && individualAppLayout.model.name !== "System sounds") {
                                renameContextMenu.originalName = individualAppLayout.model.name
                                renameContextMenu.streamIndex = individualAppLayout.model.streamIndex
                                renameContextMenu.currentCustomName = AudioBridge.getCustomApplicationName(individualAppLayout.model.name, individualAppLayout.model.streamIndex)
                                renameContextMenu.popup()
                                console.log(individualAppLayout.model.streamIndex)
                            }
                        }
                    }
                }

                ProgressSlider {
                    id: volumeSlider
                    from: 0
                    to: 100
                    enabled: !UserSettings.chatMixEnabled && !muteButton.highlighted
                    //opacity: enabled ? 1 : 0.5
                    audioLevel: {
                        AudioBridge.applicationAudioLevels
                        return AudioBridge.getApplicationAudioLevel(individualAppLayout.model.appId)
                    }
                    opacity: AudioBridge.isApplicationLocked(individualAppLayout.model.name, individualAppLayout.model.streamIndex) ? 0.5 : (enabled ? 1 : 0.5)

                    onActiveFocusChanged: {
                        focus = false
                    }

                    Layout.fillWidth: true

                    value: {
                        if (pressed) {
                            return value
                        }

                        if (!UserSettings.chatMixEnabled) {
                            return individualAppLayout.model.volume
                        }

                        let appName = individualAppLayout.model.name
                        let isCommApp = AudioBridge.isCommApp(appName)

                        return isCommApp ? 100 : UserSettings.chatMixValue
                    }

                    Behavior on opacity {
                        NumberAnimation {
                            duration: 300
                            easing.type: Easing.Linear
                        }
                    }

                    ToolTip {
                        parent: volumeSlider.handle
                        visible: volumeSlider.pressed
                        text: Math.round(volumeSlider.value).toString()
                    }

                    onValueChanged: {
                        if (!UserSettings.chatMixEnabled && pressed) {
                            root.applicationVolumeChanged(individualAppLayout.model.appId, value)
                        }
                    }

                    onPressedChanged: {
                        if (!pressed && !UserSettings.chatMixEnabled) {
                            root.applicationVolumeChanged(individualAppLayout.model.appId, value)
                        }

                        if (!UserSettings.showAudioLevel) return

                        if (pressed) {
                            AudioBridge.stopApplicationAudioLevelMonitoring()
                        } else {
                            AudioBridge.startApplicationAudioLevelMonitoring()
                        }
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

    // Context menu for renaming applications
    Menu {
        id: renameContextMenu

        property string originalName: ""
        property int streamIndex: 0
        property string currentCustomName: ""

        Connections {
            target: AudioBridge
            function onApplicationLockChanged(originalName, streamIndex, isLocked) {
                // Check if this signal is for our menu item
                if (originalName === renameContextMenu.originalName &&
                    streamIndex === renameContextMenu.streamIndex) {
                    // Force the menu item to update by toggling a property
                    lockMenuItem.text = isLocked ? qsTr("Unlock") : qsTr("Lock")
                }
            }
        }

        MenuItem {
            text: qsTr("Rename Application")
            onTriggered: renameDialog.open()
        }

        MenuItem {
            text: qsTr("Reset to Original Name")
            enabled: renameContextMenu.currentCustomName !== renameContextMenu.originalName
            onTriggered: {
                AudioBridge.setCustomApplicationName(renameContextMenu.originalName, renameContextMenu.streamIndex, "")
            }
        }

        MenuItem {
            id: lockMenuItem
            text: AudioBridge.isApplicationLocked(renameContextMenu.originalName, renameContextMenu.streamIndex)
                  ? qsTr("Unlock") : qsTr("Lock")
            onTriggered: {
                let isCurrentlyLocked = AudioBridge.isApplicationLocked(renameContextMenu.originalName, renameContextMenu.streamIndex)
                AudioBridge.setApplicationLocked(renameContextMenu.originalName, renameContextMenu.streamIndex, !isCurrentlyLocked)
            }
        }
    }

    // Rename dialog
    Dialog {
        id: renameDialog
        title: qsTr("Rename Application")
        modal: true
        width: 300
        dim: false
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            Label {
                text: qsTr("Original name: ") + renameContextMenu.originalName
                font.bold: true
            }

            Label {
                text: qsTr("Stream index: ") + renameContextMenu.streamIndex
                opacity: 0.7
            }

            Label {
                text: qsTr("Custom name:")
            }

            TextField {
                id: customNameField
                Layout.fillWidth: true
                placeholderText: renameContextMenu.originalName
                text: renameContextMenu.currentCustomName

                Keys.onReturnPressed: {
                    AudioBridge.setCustomApplicationName(
                        renameContextMenu.originalName,
                        renameContextMenu.streamIndex,
                        customNameField.text.trim()
                    )
                    renameDialog.close()
                }
            }

            RowLayout {
                spacing: 15
                Layout.topMargin: 10

                Button {
                    text: qsTr("Cancel")
                    onClicked: renameDialog.close()
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Save")
                    highlighted: true
                    Layout.fillWidth: true
                    onClicked: {
                        AudioBridge.setCustomApplicationName(
                            renameContextMenu.originalName,
                            renameContextMenu.streamIndex,
                            customNameField.text.trim()
                        )
                        renameDialog.close()
                    }
                }
            }
        }

        onOpened: {
            customNameField.text = renameContextMenu.currentCustomName
            customNameField.forceActiveFocus()
            customNameField.selectAll()
        }
    }
}
