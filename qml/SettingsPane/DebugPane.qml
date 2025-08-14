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
                title: qsTr("Application Updates")
                description: Updater.updateAvailable ? qsTr("Version %1 is available").arg(Updater.latestVersion) : ""

                additionalControl: Column {
                    spacing: 5

                    Button {
                        id: updateBtn
                        text: {
                            if (Updater.isChecking) return qsTr("Checking...")
                            if (Updater.isDownloading) return qsTr("Downloading...")
                            if (Updater.updateAvailable) return qsTr("Download and Install")
                            return qsTr("Check for Updates")
                        }

                        enabled: !Updater.isChecking && !Updater.isDownloading
                        onClicked: {
                            if (Updater.updateAvailable) {
                                Updater.downloadAndInstall()
                            } else {
                                Updater.checkForUpdates()
                            }
                        }
                    }

                    ProgressBar {
                        width: updateBtn.implicitWidth
                        from: 0
                        to: 100
                        value: Updater.downloadProgress
                        visible: Updater.isDownloading
                    }
                }
            }

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
