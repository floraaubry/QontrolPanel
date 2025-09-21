import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

Menu {
    id: powerMenu

    signal setPowerAction(int action)

    MenuItem {
        enabled: SoundPanelBridge.isSleepSupported()
        icon.source: "qrc:/icons/sleep.svg"
        text: qsTr("Sleep")
        onTriggered: SoundPanelBridge.sleep()
    }

    MenuItem {
        icon.source: "qrc:/icons/hibernate.svg"
        text: qsTr("Hibernate")
        onTriggered: {
            if (UserSettings.showPowerDialogConfirmation) {
                powerMenu.setPowerAction(0)
            } else {
                SoundPanelBridge.hibernate()
            }
        }
        enabled: SoundPanelBridge.isHibernateSupported()
    }

    MenuItem {
        icon.source: "qrc:/icons/restart.svg"
        text: qsTr("Restart")
        onTriggered: {
            if (UserSettings.showPowerDialogConfirmation) {
                powerMenu.setPowerAction(1)
            } else {
                SoundPanelBridge.restart()
            }
        }
    }

    MenuItem {
        icon.source: "qrc:/icons/shutdown.svg"
        text: qsTr("Shutdown")
        onTriggered: {
            if (UserSettings.showPowerDialogConfirmation) {
                powerMenu.setPowerAction(2)
            } else {
                SoundPanelBridge.shutdown()
            }
        }
    }

    MenuSeparator {}

    MenuItem {
        icon.source: "qrc:/icons/lock.svg"
        text: qsTr("Lock")
        onTriggered: SoundPanelBridge.lockAccount()
    }

    MenuItem {
        icon.source: "qrc:/icons/signout.svg"
        text: qsTr("Sign Out")
        onTriggered: {
            if (UserSettings.showPowerDialogConfirmation) {
                powerMenu.setPowerAction(3)
            } else {
                SoundPanelBridge.signOut()
            }
        }
    }

    MenuItem {
        icon.source: "qrc:/icons/switch.svg"
        enabled: SoundPanelBridge.hasMultipleUsers()
        text: qsTr("Switch User")
        onTriggered: SoundPanelBridge.switchAccount()
    }
}
