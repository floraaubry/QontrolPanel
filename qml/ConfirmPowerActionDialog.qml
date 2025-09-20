import QtQuick
import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

Dialog {
    id: dialog
    closePolicy: Popup.NoAutoClose
    title: {
        switch (action) {
        case 0:
            return qsTr("Hibernate")
        case 1:
            return qsTr("Restart")
        case 2:
            return qsTr("Shutdown")
        case 3:
            return qsTr("Sign Out")
        default:
            return qsTr("Action")
        }
    }
    width: 300
    signal requestClose()
    property string text: {
        switch (action) {
        case 0:
            return qsTr("System will hibernate in %1 seconds").arg(countdownTimer.remaining)
        case 1:
            return qsTr("System will restart in %1 seconds").arg(countdownTimer.remaining)
        case 2:
            return qsTr("System will shutdown in %1 seconds").arg(countdownTimer.remaining)
        case 3:
            return qsTr("You will be signed out in %1 seconds").arg(countdownTimer.remaining)
        default:
            return ""
        }
    }
    property int action: 0
    property string actionText: {
        switch (action) {
        case 0:
            return qsTr("Hibernate")
        case 1:
            return qsTr("Restart")
        case 2:
            return qsTr("Shutdown")
        case 3:
            return qsTr("Sign Out")
        default:
            return qsTr("OK")
        }
    }
    Timer {
        id: countdownTimer
        property int remaining: UserSettings.confirmationTimeout
        interval: 1000
        repeat: true
        running: dialog.visible
        onTriggered: {
            remaining--
            if (remaining <= 0) {
                stop()
                dialog.close()
                dialog.performAction()
            }
        }
        onRunningChanged: {
            if (running) {
                remaining = UserSettings.confirmationTimeout
            }
        }
    }
    Label {
        text: dialog.text
    }
    function performAction() {
        switch (action) {
        case 0:
            SoundPanelBridge.hibernate()
            break
        case 1:
            SoundPanelBridge.restart()
            break
        case 2:
            SoundPanelBridge.shutdown()
            break
        case 3:
            SoundPanelBridge.signOut()
            break
        default:
            break
        }
    }
    footer: DialogButtonBox {
        spacing: 10
        Button {
            text: qsTr("Cancel")
            onClicked: {
                countdownTimer.stop()
                dialog.requestClose()
            }
        }
        Button {
            text: dialog.actionText
            highlighted: true
            onClicked: {
                countdownTimer.stop()
                dialog.close()
                dialog.requestClose()
                dialog.performAction()
            }
        }
    }
}
