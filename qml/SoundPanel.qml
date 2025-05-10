import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

ApplicationWindow {
    id: window
    width: 400
    height: mainLayout.implicitHeight
    visible: false
    flags: Qt.FramelessWindowHint
    color: "transparent"
    Material.theme: Material.System

    ListModel {
        id: appModel
    }

    function clearApplicationModel() {
        appModel.clear();
    }

    function addApplication(appID, name, isMuted, volume, icon) {
        appModel.append({
                            appID: appID,
                            name: name,
                            isMuted: isMuted,
                            volume: volume,
                            icon: "data:image/png;base64," + icon
                        });
    }

    function clearPlaybackDevices() {
        outputDeviceComboBox.model.clear();
    }

    function addPlaybackDevice(deviceName) {
        outputDeviceComboBox.model.append({name: deviceName});
    }

    function clearRecordingDevices() {
        inputDeviceComboBox.model.clear();
    }

    function addRecordingDevice(deviceName) {
        inputDeviceComboBox.model.append({name: deviceName});
    }

    function setPlaybackDeviceCurrentIndex(index) {
        outputDeviceComboBox.currentIndex = index;
    }

    function setRecordingDeviceCurrentIndex(index) {
        inputDeviceComboBox.currentIndex = index;
    }

    Rectangle {
        anchors.fill: parent
        color: Material.theme === Material.Dark ? "#1C1C1C" : "#E3E3E3"
        radius: Material.MediumScale
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            radius: Material.MediumScale
            border.width: 1
            border.color: Material.accent
            opacity: 0.5
        }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        Label {
            text: qsTr("Audio devices")
            Layout.bottomMargin: -10
            Layout.leftMargin: 10
            color: Material.accent
            visible: !mixerOnly
        }

        Pane {
            visible: !mixerOnly
            Layout.fillWidth: true
            Material.background: Material.theme === Material.Dark ? "#2B2B2B" : "#FFFFFF"
            Material.elevation: 6
            Material.roundedScale: Material.MediumScale
            ColumnLayout {
                id: deviceLayout
                objectName: "gridLayout"
                anchors.fill: parent
                spacing: 5

                ComboBox {
                    id: outputDeviceComboBox
                    Layout.preferredHeight: 40
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                    flat: true
                    font.pixelSize: 15
                    model: ListModel {}
                    contentItem: Label {
                        text: outputDeviceComboBox.currentText
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        font.pixelSize: 15
                        leftPadding: 10
                        width: parent.width
                    }
                    onActivated: {
                        soundPanel.onPlaybackDeviceChanged(outputDeviceComboBox.currentText);
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    RoundButton {
                        id: outputeMuteRoundButton
                        Layout.preferredHeight: 40
                        Layout.preferredWidth: 40
                        flat: true
                        property string volumeIcon: {
                            if (soundPanel.playbackMuted || soundPanel.playbackVolume === 0) {
                                return "qrc:/icons/panel_volume_0.png"
                            } else if (soundPanel.playbackVolume <= 33) {
                                return "qrc:/icons/panel_volume_33.png"
                            } else if (soundPanel.playbackVolume <= 66) {
                                return "qrc:/icons/panel_volume_66.png"
                            } else {
                                return "qrc:/icons/panel_volume_100.png"
                            }
                        }

                        icon.source: volumeIcon
                        onClicked: {
                            soundPanel.onOutputMuteButtonClicked()
                        }
                    }

                    Slider {
                        id: outputSlider
                        value: soundPanel.playbackVolume
                        from: 0
                        to: 100
                        stepSize: 1
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        onValueChanged: {
                            if (pressed) {
                                soundPanel.onPlaybackVolumeChanged(value)
                                soundPanel.playbackVolume = value
                            }
                        }

                        onPressedChanged: {
                            if (!pressed) {
                                soundPanel.onOutputSliderReleased()
                            }
                        }
                    }
                }

                ComboBox {
                    id: inputDeviceComboBox
                    Layout.topMargin: -2
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    Layout.columnSpan: 3
                    flat: true
                    font.pixelSize: 15
                    model: ListModel {}
                    contentItem: Label {
                        text: inputDeviceComboBox.currentText
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        font.pixelSize: 15
                        width: parent.width
                        leftPadding: 10
                    }
                    onActivated: {
                        soundPanel.onRecordingDeviceChanged(inputDeviceComboBox.currentText);
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    RoundButton {
                        id: inputMuteRoundButton
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        flat: true
                        icon.source: soundPanel.recordingMuted ? "qrc:/icons/mic_muted.png" : "qrc:/icons/mic.png"
                        onClicked: {
                            soundPanel.onInputMuteButtonClicked()
                        }
                    }

                    Slider {
                        id: inputSlider
                        value: soundPanel.recordingVolume
                        from: 0
                        to: 100
                        stepSize: 1
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        onValueChanged: {
                            soundPanel.onRecordingVolumeChanged(value)
                            soundPanel.recordingVolume = value
                        }
                    }
                }
            }
        }

        Label {
            text: qsTr("Applications")
            Layout.bottomMargin: -10
            Layout.leftMargin: 10
            color: Material.accent
        }

        Pane {
            Layout.fillWidth: true
            Material.background: Material.theme === Material.Dark ? "#2B2B2B" : "#FFFFFF"
            Material.elevation: 6
            Material.roundedScale: Material.MediumScale
            ColumnLayout {
                id: appLayout
                objectName: "gridLayout"
                anchors.fill: parent
                spacing: 5

                Repeater {
                    id: appRepeater
                    model: appModel
                    onCountChanged: {
                        window.height = mainLayout.implicitHeight + ((45 * appRepeater.count) - 5) + 30
                    }
                    delegate: RowLayout {
                        id: applicationUnitLayout
                        Layout.preferredHeight: 40
                        Layout.fillWidth: true
                        required property var model

                        RoundButton {
                            id: muteRoundButton
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            flat: !checked
                            checkable: true
                            highlighted: checked
                            checked: applicationUnitLayout.model.isMuted
                            ToolTip.text: applicationUnitLayout.model.name
                            ToolTip.visible: hovered
                            ToolTip.delay: 1000
                            icon.source: applicationUnitLayout.model.name === "Windows system sounds" ? "qrc:/icons/system_light.png" : applicationUnitLayout.model.icon
                            icon.color: applicationUnitLayout.model.name === "Windows system sounds" ? undefined : "transparent"
                            onClicked: {
                                applicationUnitLayout.model.isMuted = !applicationUnitLayout.model.isMuted;
                                soundPanel.onApplicationMuteButtonClicked(applicationUnitLayout.model.appID, applicationUnitLayout.model.isMuted);
                            }
                        }

                        Slider {
                            id: volumeSlider
                            from: 0
                            to: 100
                            stepSize: 1
                            value: applicationUnitLayout.model.volume
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            onValueChanged: {
                                soundPanel.onApplicationVolumeSliderValueChanged(applicationUnitLayout.model.appID, value);
                            }
                        }
                    }
                }
            }
        }
    }
}
