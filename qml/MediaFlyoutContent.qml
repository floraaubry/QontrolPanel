import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

ColumnLayout {
    id: mediaLayout
    opacity: 0
    spacing: 10

    Behavior on opacity {
        NumberAnimation {
            duration: 300
            easing.type: Easing.OutQuad
        }
    }

    ColumnLayout {
        RowLayout {
            id: infosLyt
            ColumnLayout {
                Label {
                    text: SoundPanelBridge.mediaTitle || ""
                    font.pixelSize: 14
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Label {
                    text: SoundPanelBridge.mediaArtist || ""
                    font.pixelSize: 12
                    opacity: 0.7
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
            Image {
                Layout.preferredWidth: 64
                Layout.preferredHeight: 64
                Layout.alignment: Qt.AlignVCenter
                source: SoundPanelBridge.mediaArt || ""
                fillMode: Image.PreserveAspectCrop
                visible: SoundPanelBridge.mediaArt !== ""
            }
        }

        RowLayout {
            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                icon.source: "qrc:/icons/prev.png"
                onClicked: SoundPanelBridge.previousTrack()
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
            }

            ToolButton {
                icon.source: SoundPanelBridge.isMediaPlaying ? "qrc:/icons/pause.png" : "qrc:/icons/play.png"
                onClicked: SoundPanelBridge.playPause()
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
            }

            ToolButton {
                icon.source: "qrc:/icons/next.png"
                onClicked: SoundPanelBridge.nextTrack()
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }
}
