import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.FluentWinUI3 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    visible: false
    width: 330
    height: 245
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    Rectangle {
        anchors.fill: parent
        color: nativeWindowColor
        radius: 8
        border.width: 1
        border.color: Qt.rgba(255, 255, 255, 0.15)

        GridLayout {
            id: outputLabel
            width: parent.width
            columnSpacing: 12

            Button {
                Layout.topMargin: 15

                Layout.leftMargin: 15
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                flat: true
                Image {
                    anchors.margins: 12
                    anchors.fill: parent
                    source: "qrc:/icons/tray_light_100.png"
                    fillMode: Image.PreserveAspectFit
                }
            }
            Item {
                Layout.topMargin: 15
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                Slider {
                    id: slider
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    anchors.right: parent.right // Align the right edge to the parent
                    anchors.leftMargin: -10
                }

                ProgressBar {
                    id: progressBar
                    width: parent.width
                    anchors.leftMargin: 13
                    anchors.rightMargin: 19
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        bottom: parent.bottom
                    }
                    value: 1
                    opacity: 1
                }
            }
        }

        GridLayout {
            id: outputControls
            anchors.top: outputLabel.bottom
            width: parent.width
            columnSpacing: 12
            rowSpacing: 0
            columns: 2

            ComboBox {
                Layout.topMargin: 12
                Layout.leftMargin: 15
                Layout.rightMargin: 15
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                model: ListModel {
                    ListElement { name: "Ce casque est incroyable" }
                    ListElement { name: "Option 2" }
                    ListElement { name: "Option 3" }
                }
            }
        }

        Item {
            height: 1
            id: outputSeparator
            anchors.top: outputControls.bottom
            width: parent.width

            Rectangle {

                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.2)
                anchors.horizontalCenter: parent.horizontalCenter
            }

            anchors.topMargin: 15
        }

        GridLayout {
            id: inputLabel
            anchors.top: outputSeparator.bottom
            width: parent.width
            columnSpacing: 12

            Button {
                Layout.topMargin: 15

                Layout.leftMargin: 15
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                flat: true
                Image {
                    anchors.margins: 12
                    anchors.fill: parent
                    source: "qrc:/icons/mic_light.png"
                    fillMode: Image.PreserveAspectFit
                }
            }
            Item {
                Layout.topMargin: 15
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                Slider {
                    id: inputSlider
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    anchors.right: parent.right // Align the right edge to the parent
                    anchors.leftMargin: -10
                }

                ProgressBar {
                    id: inputProgressBar
                    width: parent.width
                    anchors.leftMargin: 13
                    anchors.rightMargin: 19
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        bottom: parent.bottom
                    }
                    value: 0.5
                    opacity: 1
                }
            }
        }

        GridLayout {
            id: inputControls
            anchors.top: inputLabel.bottom
            width: parent.width
            columnSpacing: 12
            rowSpacing: 0
            columns: 2

            ComboBox {
                Layout.topMargin: 12
                Layout.leftMargin: 15
                Layout.rightMargin: 15
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                model: ListModel {
                    ListElement { name: "Ce micro est moins ouf" }
                    ListElement { name: "Option 2" }
                    ListElement { name: "Option 3" }
                }
            }
        }

        Item {
            height: 1
            id: inputSeparator
            anchors.top: inputControls.bottom
            width: parent.width

            Rectangle {

                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.2)
                anchors.horizontalCenter: parent.horizontalCenter
            }

            anchors.topMargin: 15
        }
    }
}


