import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

ColumnLayout {
    spacing: 3

    Label {
        text: qsTr("Debug and information")
        font.pixelSize: 22
        font.bold: true
        Layout.bottomMargin: 15
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        ColumnLayout {
            width: parent.width
            spacing: 3
            Card {
                Layout.fillWidth: true
                title: qsTr("Application version")
                description: ""

                property int clickCount: 0

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        parent.clickCount++
                        if (parent.clickCount >= 5) {
                            //easterEggDialog.open()
                            Context.easterEggRequested()
                            parent.clickCount = 0
                        }
                    }
                }
                additionalControl: Label {
                    text: SoundPanelBridge.getAppVersion()
                    opacity: 0.5
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("QT version")
                description: ""

                additionalControl: Label {
                    text: SoundPanelBridge.getQtVersion()
                    opacity: 0.5
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Commit")
                description: ""

                additionalControl: Label {
                    text: SoundPanelBridge.getCommitHash()
                    opacity: 0.5
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Build date")
                description: ""

                additionalControl: Label {
                    text: SoundPanelBridge.getBuildTimestamp()
                    opacity: 0.5
                }
            }
        }
    }
}
