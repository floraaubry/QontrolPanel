import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel
import QtQuick

ApplicationWindow {
    id: powerConfirmationWindow
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
    width: Screen.width
    height: Screen.height
    color: "transparent"
    visible: false
    opacity: 0

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.7
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 200
            easing.type: Easing.OutQuad
        }
    }

    ConfirmPowerActionDialog {
        id: powerActionDialog
        anchors.centerIn: parent
        onRequestClose: powerConfirmationWindow.hide()
    }

    function setAction(index) {
        powerActionDialog.action = index
    }

    function show() {
        visible = true
        opacity = 1
    }

    function hide() {
        powerActionDialog.close()
        opacity = 0
    }

    onOpacityChanged: {
        if (opacity === 0 && visible) {
            visible = false
        } else if (opacity === 1 && visible) {
            powerActionDialog.open()
        }
    }
}
