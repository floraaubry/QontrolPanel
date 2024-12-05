import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.FluentWinUI3 2.15

ApplicationWindow {
    width: 330
    height: 180
    visible: false
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    Rectangle {
        anchors.fill: parent
        color: nativeWindowColor
        radius: 8
        border.width: 1
        border.color: Qt.rgba(255, 255, 255, 0.15)
    }

    GridLayout {
        id: timeline
        y: 12
        height: 24
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        layoutDirection: Qt.LeftToRight
        flow: GridLayout.LeftToRight
        rows: 1
        columns: 3
        columnSpacing: 12

        Label {
            id: currentTimeLabel
            text: qsTr("%1:%2").arg(Math.floor(mediaFlyout.currentTime / 60).toString().padStart(2, '0')).arg((mediaFlyout.currentTime % 60).toString().padStart(2, '0'))
            font.pixelSize: 13
        }

        ProgressBar {
            id: progressBar
            from: 0
            to: mediaFlyout.totalTime
            value: mediaFlyout.currentTime
            Layout.fillWidth: true
        }

        Label {
            id: totalTimeLabel
            text: qsTr("%1:%2").arg(Math.floor(mediaFlyout.totalTime / 60).toString().padStart(2, '0')).arg((mediaFlyout.totalTime % 60).toString().padStart(2, '0'))
            font.pixelSize: 13
        }
    }

    ColumnLayout {
        id: columnLayout
        anchors.top: timeline.bottom
        height: 64
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 15
        anchors.leftMargin: 15
        anchors.rightMargin: 15

        Label {
            id: titleLabel
            Layout.preferredWidth: parent.width - icon.width - 20  // Adjust width to avoid overlap
            text: mediaFlyout.title
            Layout.fillWidth: true
            font.bold: true
            font.pixelSize: 14
        }

        Label {
            id: artistLabel
            text: mediaFlyout.artist
            Layout.fillWidth: true
            font.pixelSize: 13
        }
    }

    GridLayout {
        id: gridLayout1
        anchors.top: timeline.bottom
        width: 64
        height: 64
        anchors.right: parent.right
        anchors.topMargin: 15
        anchors.leftMargin: 15
        anchors.rightMargin: 15

        Image {
            id: icon
            Layout.fillHeight: true
            Layout.fillWidth: true
            source: mediaFlyout.iconSource
            fillMode: Image.PreserveAspectFit
        }
    }

    RowLayout {
        width: parent.width
        spacing: 15
        anchors.top: gridLayout1.bottom
        anchors.topMargin: 15

        Item { Layout.fillWidth: true }

        Button {
            id: prev
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            enabled: mediaFlyout.isPrevEnabled
            flat: true
            Image {
                anchors.margins: 12
                anchors.fill: parent
                source: "qrc:/icons/prev_light.png"
                fillMode: Image.PreserveAspectFit
            }
            onClicked: mediaFlyout.onPrevButtonClicked()
        }

        Button {
            id: pause
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            flat: true
            Image {
                anchors.margins: 12
                anchors.fill: parent
                source: mediaFlyout.isPlaying ? "qrc:/icons/pause_light.png" : "qrc:/icons/play_light.png"
                fillMode: Image.PreserveAspectFit
            }
            onClicked: mediaFlyout.onPauseButtonClicked()
        }

        Button {
            id: next
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            enabled: mediaFlyout.isNextEnabled
            flat: true
            Image {
                anchors.margins: 12
                anchors.fill: parent
                source: "qrc:/icons/next_light.png"
                fillMode: Image.PreserveAspectFit
            }
            onClicked: mediaFlyout.onNextButtonClicked()
        }

        Item { Layout.fillWidth: true }
    }
}
