import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.FluentWinUI3 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    visible: false
    width: 330
    height: 194
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
            Label {
                padding: 10
                Layout.preferredWidth: 80
                text: "Output"
                font.pixelSize: 16
                font.bold: true
            }

            Item {
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
                    anchors.leftMargin: -15
                }

                ProgressBar {
                    id: progressBar
                    width: parent.width
                    anchors.leftMargin: 6
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

            Button {
                Layout.leftMargin: 11
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                text: "o"
            }



            ComboBox {
                Layout.rightMargin: 11
                Layout.fillWidth: true
                Layout.preferredHeight: 40

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
            Label {
                padding: 10
                Layout.preferredWidth: 80
                text: "Input"
                font.pixelSize: 16
                font.bold: true
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                Slider {
                    id: outputSlider
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    anchors.right: parent.right
                    anchors.leftMargin: -15
                }

                ProgressBar {
                    id: outputProgressBar
                    width: parent.width
                    anchors.leftMargin: 6
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
            id: inputControls
            anchors.top: inputLabel.bottom
            width: parent.width
            columnSpacing: 12
            rowSpacing: 0
            columns: 2

            Button {
                Layout.leftMargin: 11
                Layout.preferredHeight: 40
                Layout.preferredWidth: 40
                text: "i"
            }



            ComboBox {
                Layout.rightMargin: 11
                Layout.fillWidth: true
                Layout.preferredHeight: 40

            }
        }
    }
}


