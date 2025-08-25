import QtQuick.Controls.FluentWinUI3
import QtQuick

Row {
    spacing: 6

    property alias checked: controlSwitch.checked
    signal clicked()

    Label {
        text: controlSwitch.checked ? qsTr("On") : qsTr("Off")
    }

    Switch {
        id: controlSwitch
        onClicked: parent.clicked()
    }
}
