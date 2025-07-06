import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QuickSoundSwitcher

ColumnLayout {
    spacing: 3

    Label {
        text: qsTr("Application language")
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
                id: dlCard
                Layout.fillWidth: true
                title: qsTr("Update Translations")
                description: qsTr("Download the latest translation files from GitHub")

                property bool downloadInProgress: false

                additionalControl: RowLayout {
                    spacing: 10

                    BusyIndicator {
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        visible: dlCard.downloadInProgress
                        running: visible
                    }

                    Button {
                        text: qsTr("Download")
                        enabled: !dlCard.downloadInProgress
                        onClicked: {
                            dlCard.downloadInProgress = true
                            SoundPanelBridge.downloadLatestTranslations()
                        }
                    }
                }

                Connections {
                    target: SoundPanelBridge

                    function onTranslationDownloadFinished(success, message) {
                        dlCard.downloadInProgress = false
                        if (success) {
                            toastNotification.showToast(qsTr("Download completed successfully!"), true)
                            SoundPanelBridge.changeApplicationLanguage(UserSettings.languageIndex)
                        } else {
                            toastNotification.showToast(qsTr("Download failed: %1").arg(message), false)
                        }
                    }

                    function onTranslationDownloadError(errorMessage) {
                        dlCard.downloadInProgress = false
                        toastNotification.showToast(qsTr("Error: %1").arg(errorMessage), false)
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Auto update translations")
                description: qsTr("Fetch for translations update at startup and every 4 hours")

                additionalControl: Switch {
                    checked: UserSettings.autoUpdateTranslations
                    onClicked: UserSettings.autoUpdateTranslations = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Application language")

                additionalControl: ComboBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    model: {
                        let names = [qsTr("System")]
                        names = names.concat(SoundPanelBridge.getLanguageNativeNames())
                        return names
                    }
                    currentIndex: UserSettings.languageIndex
                    onActivated: {
                        UserSettings.languageIndex = currentIndex
                        SoundPanelBridge.changeApplicationLanguage(currentIndex)
                        currentIndex = UserSettings.languageIndex
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Translation author")
                description: ""

                additionalControl: Label {
                    text: qsTr("Unknow author")
                    opacity: 0.5
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Translation last updated")
                description: ""

                additionalControl: Label {
                    text: qsTr("Unknow date")
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

            color: isSuccess ? "#4CAF50" : "#F44336"  // Green for success, red for error

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
                interval: 3000  // Show for 3 seconds
                onTriggered: hideAnimation.start()
            }
        }
    }
}
