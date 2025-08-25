import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import QtMultimedia
import Odizinne.QontrolPanel

ApplicationWindow {
    id: introWindow
    width: mainLyt.implicitWidth + 40
    height: mainLyt.implicitHeight + 40
    visible: false
    flags: Qt.FramelessWindowHint | Qt.ToolTip
    color: "#00000000"
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    transientParent: null

    function showIntro() {
        visible = true
        videoPlayer.play()
    }

    function closeIntro() {
        videoPlayer.stop()
        visible = false
        UserSettings.firstRun = false
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        color: palette.window
        radius: 12

        Rectangle {
            anchors.fill: parent
            color: "#00000000"
            radius: 12
            border.width: 1
            border.color: Constants.separatorColor
            opacity: 0.2
        }

        ColumnLayout {
            id: mainLyt
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20

            Label {
                text: qsTr("Welcome to QontrolPanel!")
                font.pixelSize: 22
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("The tray icon is probably hidden and can be added to the tray area by dragging it as shown in the video below.")
                font.pixelSize: 12
                opacity: 0.8
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: parent.width - 40
            }

            Video {
                id: videoPlayer
                Layout.alignment: Qt.AlignCenter
                source: "qrc:/videos/intro.mp4"
                fillMode: VideoOutput.PreserveAspectFit
                loops: MediaPlayer.Infinite

                Rectangle {
                    anchors.fill: parent
                    color: "black"
                    radius: 8
                    z: -1
                }
            }

            Label {
                text: qsTr("You can then click on the tray icon to reveal the panel.")
                font.pixelSize: 12
                opacity: 0.8
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.maximumWidth: parent.width - 40
            }

            MenuSeparator { Layout.fillWidth: true }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("Automatic application update fetching")
                    Layout.fillWidth: true
                }

                LabeledSwitch {
                    checked: UserSettings.autoFetchForAppUpdates
                    onClicked: UserSettings.autoFetchForAppUpdates = checked
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("Automatic translations update")
                    Layout.fillWidth: true
                }

                LabeledSwitch {
                    checked: UserSettings.autoUpdateTranslations
                    onClicked: UserSettings.autoUpdateTranslations = checked
                }
            }

            MenuSeparator { Layout.fillWidth: true }

            Button {
                text: qsTr("Get Started")
                highlighted: true
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 120

                onClicked: introWindow.closeIntro()
            }
        }
    }
}
