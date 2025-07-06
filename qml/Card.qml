import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher
import QtQuick.Controls.impl

Rectangle {
    id: card

    property string title: ""
    property string description: ""
    property Component additionalControl
    property string iconSource: ""
    property int iconWidth: 24
    property int iconHeight: 24
    property color iconColor
    property bool imageMode: false

    // Use a custom property instead of visible
    property bool show: true

    implicitHeight: 70
    color: Constants.cardColor
    radius: 5

    // Animate opacity and transform
    opacity: show ? 1 : 0

    transform: Translate {
        id: translateTransform
        y: show ? 0 : -20

        Behavior on y {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutQuad
            }
        }
    }

    // Smooth animations
    Behavior on opacity {
        NumberAnimation {
            duration: 100
            easing.type: Easing.OutQuad
        }
    }

    // Timer for hiding after animation
    Timer {
        id: hideTimer
        interval: 100 // Match animation duration
        onTriggered: card.visible = false
    }

    onShowChanged: {
        if (show) {
            // Show immediately
            visible = true
        } else {
            // Start hide animation, then hide after delay
            hideTimer.start()
        }
    }

    // Stop timer if show becomes true while hiding
    onVisibleChanged: {
        if (visible && !show) {
            hideTimer.stop()
        }
    }

    Rectangle {
        anchors.fill: parent
        border.width: 1
        color: Constants.cardColor
        border.color: Constants.cardBorderColor
        opacity: 0.5
        radius: 5
    }

    RowLayout {
        id: rowLayout
        spacing: 15
        anchors.fill: parent
        anchors.margins: 15

        IconImage {
            source: card.iconSource
            visible: card.iconSource && card.iconSource !== ""
            sourceSize.width: card.iconWidth
            sourceSize.height: card.iconHeight
            color: card.imageMode ? "#00000000" : (card.iconColor ? card.iconColor : palette.text)
            Layout.preferredHeight: card.iconHeight
            Layout.preferredWidth: card.iconWidth
        }

        ColumnLayout {
            spacing: 0
            Layout.minimumWidth: 100
            Layout.preferredWidth: Layout.maximumWidth
            Layout.maximumWidth: card.width - (card.iconSource ? card.iconWidth + 15 : 0) - (additionalControlLoader.item ? additionalControlLoader.item.width : 0) - 45

            Label {
                text: card.title
                font.pixelSize: 14
                visible: text
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: card.description
                opacity: 0.6
                visible: text
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                maximumLineCount: 2
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.minimumWidth: 10
        }

        Loader {
            id: additionalControlLoader
            sourceComponent: card.additionalControl
            visible: card.additionalControl !== null
        }
    }
}
