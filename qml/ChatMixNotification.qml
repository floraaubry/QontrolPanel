import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import QtQuick.Controls.impl
import QtQuick.Window
import Odizinne.QuickSoundSwitcher
import QtMultimedia

ApplicationWindow {
    id: notificationWindow
    width: contentRow.implicitWidth + 30
    height: 50
    visible: false
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "#00000000"

    property string message: ""
    property bool isAnimatingIn: false
    property bool isAnimatingOut: false

    transientParent: null

    onWidthChanged: {
        if (visible) {
            positionWindow()
        }
    }

    Component.onCompleted: {
        positionWindow()
    }

    SoundEffect {
        id: chatMixOffEffect
        source: "qrc:/sounds/outcome-failure.wav"
    }

    SoundEffect {
        id: chatMixOnEffect
        source: "qrc:/sounds/outcome-success.wav"
    }

    Connections {
        target: SoundPanelBridge
        function onChatMixNotificationRequested(message) {
            notificationWindow.showNotification(message)

            if (!UserSettings.chatMixShortcutNotification) return
            if (UserSettings.chatMixEnabled) {
                chatMixOnEffect.play()
            } else {
                chatMixOffEffect.play()
            }
        }
    }

    Timer {
        id: autoHideTimer
        interval: 3000
        repeat: false
        onTriggered: notificationWindow.hideNotification()
    }

    PropertyAnimation {
        id: showAnimation
        target: contentTransform
        property: "y"
        duration: 300
        easing.type: Easing.OutQuad
        onStarted: {
            notificationWindow.isAnimatingIn = true
            contentOpacityAnimation.start()
        }
        onFinished: {
            notificationWindow.isAnimatingIn = false
            autoHideTimer.start()
        }
    }

    PropertyAnimation {
        id: hideAnimation
        target: contentTransform
        property: "y"
        duration: 250
        easing.type: Easing.OutQuad
        from: 0
        to: -notificationWindow.height
        onStarted: {
            notificationWindow.isAnimatingOut = true
            hideOpacityAnimation.start()
        }
        onFinished: {
            notificationWindow.visible = false
            notificationWindow.isAnimatingOut = false
            contentTransform.y = -notificationWindow.height
        }
    }

    PropertyAnimation {
        id: contentOpacityAnimation
        target: notificationRect
        property: "opacity"
        duration: 200
        easing.type: Easing.OutQuad
        from: 0
        to: 1
    }

    PropertyAnimation {
        id: hideOpacityAnimation
        target: notificationRect
        property: "opacity"
        duration: 150
        easing.type: Easing.InQuad
        from: 1
        to: 0
    }

    Translate {
        id: contentTransform
        x: 0
        y: -notificationWindow.height
    }

    Text {
        id: enabledMeasure
        text: qsTr("ChatMix Enabled")
        font.pixelSize: 14
        visible: false
    }

    Text {
        id: disabledMeasure
        text: qsTr("ChatMix Disabled")
        font.pixelSize: 14
        visible: false
    }

    function showNotification(text) {
        if (visible || isAnimatingIn) {
            message = text
            autoHideTimer.restart()
            return
        }

        message = text
        positionWindow()

        if (isAnimatingOut) {
            hideAnimation.stop()
            hideOpacityAnimation.stop()
            isAnimatingOut = false

            showAnimation.from = contentTransform.y
            showAnimation.to = 0

            let progress = Math.abs(contentTransform.y) / notificationWindow.height
            showAnimation.duration = Math.max(150, 300 * progress)
            showAnimation.start()
            return
        }

        contentTransform.y = -notificationWindow.height
        notificationRect.opacity = 0

        showAnimation.from = -notificationWindow.height
        showAnimation.to = 0
        showAnimation.duration = 300

        visible = true
        showAnimation.start()
    }

    function hideNotification() {
        if (isAnimatingOut) {
            return
        }

        if (isAnimatingIn) {
            showAnimation.stop()
            contentOpacityAnimation.stop()
            isAnimatingIn = false
        }

        autoHideTimer.stop()
        hideAnimation.start()
    }

    function positionWindow() {
        const screen = Qt.application.screens[0]
        x = (screen.width - width) / 2
        y = SoundPanelBridge.taskbarPosition === "top" ? 60 : 12
    }

    Rectangle {
        id: notificationRect
        anchors.fill: parent
        color: Constants.panelColor
        radius: 5
        opacity: 0

        transform: Translate {
            x: contentTransform.x
            y: contentTransform.y
        }

        Rectangle {
            anchors.fill: parent
            color: "#00000000"
            radius: 5
            border.width: 1
            border.color: Constants.separatorColor
            opacity: 0.15
        }

        RowLayout {
            id: contentRow
            anchors.fill: parent
            anchors.margins: 15
            spacing: 12

            IconImage {
                source: "qrc:/icons/headset.svg"
                sourceSize.width: 20
                sourceSize.height: 20
                color: enabled ? palette.accent : palette.text
                opacity: enabled ? 1.0 : 0.5
                Layout.alignment: Qt.AlignVCenter
                enabled: UserSettings.chatMixEnabled

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                        easing.type: Easing.OutQuad
                    }
                }

                Behavior on opacity {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.OutQuad
                    }
                }
            }

            Label {
                text: notificationWindow.message
                font.pixelSize: 14
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: Math.max(enabledMeasure.implicitWidth, disabledMeasure.implicitWidth)
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
