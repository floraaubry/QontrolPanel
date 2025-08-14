pragma ComponentBehavior: Bound

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

    Connections {
        target: Updater

        function onUpdateFinished(success, message) {
            toastNotification.showToast(message, success)
        }
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
                id: releaseNotesCard
                Layout.fillWidth: true
                title: qsTr("Release notes")
                description: qsTr("View what's new in version %1").arg(Updater.latestVersion)
                visible: Updater.updateAvailable && Updater.hasReleaseNotes
                additionalControl: Button {
                    text: qsTr("Show")
                    enabled: Updater.hasReleaseNotes
                    onClicked: releaseNotesDialog.open()
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Auto check for app updates")
                description: qsTr("Check for application updates at startup and every 4 hours")

                additionalControl: Switch {
                    checked: UserSettings.autoFetchForAppUpdates
                    onClicked: UserSettings.autoFetchForAppUpdates = checked
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

        Rectangle {
            id: toastNotification

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20

            width: Math.min(toastText.implicitWidth + 40, parent.width - 40)
            height: 50
            radius: 8

            visible: false
            opacity: 0

            property bool isSuccess: true

            color: isSuccess ? "#4CAF50" : "#F44336"

            function showToast(message, success) {
                toastText.text = message
                isSuccess = success
                visible = true
                showAnimation.start()
                hideTimer.start()
            }

            Label {
                id: toastText
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            NumberAnimation {
                id: showAnimation
                target: toastNotification
                property: "opacity"
                from: 0
                to: 1
                duration: 300
                easing.type: Easing.OutQuad
            }

            NumberAnimation {
                id: hideAnimation
                target: toastNotification
                property: "opacity"
                from: 1
                to: 0
                duration: 300
                easing.type: Easing.InQuad
                onFinished: toastNotification.visible = false
            }

            Timer {
                id: hideTimer
                interval: 3000
                onTriggered: hideAnimation.start()
            }
        }

        Dialog {
            id: releaseNotesDialog
            title: qsTr("Version %1").arg(Updater.latestVersion)
            modal: true
            width: 400
            height: 300
            anchors.centerIn: parent
            standardButtons: Dialog.Close

            ScrollView {
                anchors.fill: parent

                Label {
                    text: Updater.releaseNotes || qsTr("No release notes available")
                    width: releaseNotesDialog.width - 60
                    wrapMode: Text.WordWrap
                    textFormat: Text.PlainText
                }
            }
        }
    }
}
