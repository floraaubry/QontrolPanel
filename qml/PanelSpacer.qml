import QtQuick

Item {
    signal clicked()

    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}
