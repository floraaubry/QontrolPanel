import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Controls.impl
import QtQuick.Layouts
import Odizinne.QontrolPanel

Rectangle {
    id: footer
    color: Constants.footerColor
    bottomLeftRadius: 12
    bottomRightRadius: 12

    signal showUpdatePane()
    signal hidePanel()
    signal showSettingsWindow()
    signal showPowerConfirmationWindow(int action)

    function closePowerMenu() {
        powerMenu.close()
    }

    Rectangle {
        height: 1
        color: Constants.footerBorderColor
        opacity: 0.15
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        Image {
            id: compactMedia
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            Layout.alignment: Qt.AlignVCenter
            source: SoundPanelBridge.mediaArt || ""
            fillMode: Image.PreserveAspectCrop
            visible: SoundPanelBridge.mediaArt !== "" && UserSettings.mediaMode === 1
        }

        Item {
            Layout.preferredWidth: updateLyt.implicitWidth
            Layout.preferredHeight: Math.max(updateIcon.height, updateLabel.height)
            Layout.leftMargin: 10
            visible: !compactMedia.visible && Updater.updateAvailable

            RowLayout {
                id: updateLyt
                anchors.fill: parent
                spacing: 10

                IconImage {
                    id: updateIcon
                    source: "qrc:/icons/update.svg"
                    color: "#edb11a"
                    sourceSize.width: 16
                    sourceSize.height: 16
                }
                Label {
                    id: updateLabel
                    text: qsTr("Update available")
                    color: "#edb11a"
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: updateLabel.font.underline = true
                onExited: updateLabel.font.underline = false
                onClicked: footer.showUpdatePane()
            }
        }

        RowLayout {
            id: batteryIndicator
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 10
            spacing: 5
            Component.onCompleted: {
                visible = Qt.binding(function() {
                    return (HeadsetControlBridge.anyDeviceFound &&
                            HeadsetControlBridge.batteryStatus !== "BATTERY_UNAVAILABLE" &&
                            !compactMedia.visible &&
                            !Updater.updateAvailable) ? true : false
                })
            }

            Image {
                id: batteryIcon
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                fillMode: Image.PreserveAspectFit
                Component.onCompleted: {
                    source = Qt.binding(function() {
                        return (HeadsetControlBridge.batteryStatus === "BATTERY_CHARGING")
                        ? Constants.getBatteryChargingIconStatic(HeadsetControlBridge.batteryLevel)
                        : Constants.getBatteryIcon(HeadsetControlBridge.batteryLevel);
                    })
                }
            }

            Label {
                id: batteryLabel
                Layout.topMargin: -2
                font.pixelSize: 12
                opacity: 0.8
                Component.onCompleted: {
                    text = Qt.binding(function() {
                        return HeadsetControlBridge.batteryLevel + "%";
                    })
                }
            }
        }

        ColumnLayout {
            spacing: 2
            Layout.fillWidth: true
            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Label {
                text: SoundPanelBridge.mediaTitle || ""
                font.pixelSize: 14
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
                visible: UserSettings.mediaMode === 1
            }

            Label {
                text: SoundPanelBridge.mediaArtist || ""
                font.pixelSize: 12
                opacity: 0.7
                elide: Text.ElideRight
                Layout.fillWidth: true
                visible: UserSettings.mediaMode === 1
            }

            Item {
                Layout.fillHeight: true
            }
        }

        NFToolButton {
            icon.source: "qrc:/icons/settings.svg"
            icon.width: 16
            icon.height: 16
            Layout.preferredHeight: width
            antialiasing: true
            onClicked: {
                footer.showSettingsWindow()
                footer.hidePanel()
            }
        }

        NFToolButton {
            visible: UserSettings.enablePowerMenu
            icon.source: "qrc:/icons/power.svg"
            icon.width: 16
            icon.height: 16
            Layout.preferredHeight: width
            antialiasing: true
            onClicked: powerMenu.visible ? powerMenu.close() : powerMenu.open()

            PowerMenu {
                id: powerMenu
                y: parent.y - powerMenu.height - 15
                x: parent.x
                onSetPowerAction: function(action) {
                    footer.showPowerConfirmationWindow(action)
                }
            }
        }
    }
}
