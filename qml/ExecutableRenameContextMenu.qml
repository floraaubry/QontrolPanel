import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

Menu {
    id: executableRenameContextMenu

    property string originalName: ""
    property string currentCustomName: ""

    MenuItem {
        text: qsTr("Rename Application")
        onTriggered: executableRenameDialog.open()
    }

    MenuItem {
        text: qsTr("Reset to Original Name")
        enabled: executableRenameContextMenu.currentCustomName !== executableRenameContextMenu.originalName
        onTriggered: {
            AudioBridge.setCustomExecutableName(executableRenameContextMenu.originalName, "")
        }
    }

    MenuSeparator {}

    MenuItem {
        text: qsTr("Mute in Background")
        checkable: true
        checked: AudioBridge.isApplicationMutedInBackground(executableRenameContextMenu.originalName)
        onTriggered: {
            AudioBridge.setApplicationMutedInBackground(executableRenameContextMenu.originalName, checked)
        }
    }
}
