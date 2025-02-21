import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Universal 2.15

ApplicationWindow {
    id: window
    width: 360
    height: gridLayout.implicitHeight
    visible: false
    flags: Qt.FramelessWindowHint
    color: "transparent"
    Universal.theme: Universal.System
    Universal.accent: accentColor

    property bool blockOutputSignal: false
    property bool blockInputSignal: false

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

    function setOutputImageSource(source) {
        outputImage.source = source;
    }

    function setInputImageSource(source) {
        inputImage.source = source;
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
        blockOutputSignal = true;
        outputDeviceComboBox.currentIndex = index;
        blockOutputSignal = false;
    }

    function setRecordingDeviceCurrentIndex(index) {
        blockInputSignal = true;
        inputDeviceComboBox.currentIndex = index;
        blockInputSignal = false;
    }

    Rectangle {
        anchors.fill: parent
        color: nativeWindowColor

        Rectangle {
            height: 1
            width: parent.width
            color: Qt.rgba(255, 255, 255, 0.15)
            anchors.top: parent.top
        }

        // Left border
        Rectangle {
            width: 1
            height: parent.height
            color: Qt.rgba(255, 255, 255, 0.15)
            anchors.left: parent.left
            anchors.top: parent.top
        }
    }

    GridLayout {
        id: gridLayout
        objectName: "gridLayout"
        anchors.fill: parent
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        anchors.topMargin: 0
        anchors.bottomMargin: 0
        columns: 3
        rowSpacing: 0

        ComboBox {
            id: outputDeviceComboBox
            visible: !mixerOnly
            Layout.preferredHeight: 40
            Layout.columnSpan: 3
            Layout.fillWidth: true
            flat: true
            font.pixelSize: 15
            model: ListModel {}
            contentItem: Text {
                text: outputDeviceComboBox.currentText
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                font.pixelSize: 15
                leftPadding: 10
                width: parent.width
                color: textColor
            }
            onCurrentTextChanged: {
                if (!window.blockOutputSignal) {
                    soundPanel.onPlaybackDeviceChanged(outputDeviceComboBox.currentText);
                }
            }
        }

        Button {
            id: outputeMuteButton
            visible: !mixerOnly
            Layout.leftMargin: 10
            Layout.preferredHeight: 40
            Layout.preferredWidth: 40
            flat: true
            icon.source: outputIcon
            Layout.bottomMargin: 10
            onClicked: {
                soundPanel.onOutputMuteButtonClicked()
            }
        }

        Slider {
            id: outputSlider
            visible: !mixerOnly
            value: soundPanel.playbackVolume
            from: 0
            to: 100
            stepSize: 1
            Layout.fillWidth: true
            Layout.preferredHeight: -1
            Layout.bottomMargin: 10
            onValueChanged: {
                if (pressed) {
                    soundPanel.onPlaybackVolumeChanged(value)
                }
            }

            onPressedChanged: {
                if (!pressed) {
                    soundPanel.onOutputSliderReleased()
                }
            }
        }

        Label {
            id: outputVolume
            visible: !mixerOnly
            text: String(outputSlider.value)
            Layout.rightMargin: 25
            Layout.leftMargin: 10
            Layout.preferredHeight: -1
            Layout.preferredWidth: 20
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 20
            Layout.bottomMargin: 10
        }

        Rectangle {
            id: inputSeparator
            visible: !mixerOnly
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: borderColor
            Layout.columnSpan: 3
        }

        ComboBox {
            id: inputDeviceComboBox
            visible: !mixerOnly
            Layout.topMargin: -2
            Layout.preferredHeight: 40
            Layout.fillWidth: true
            Layout.columnSpan: 3
            flat: true
            font.pixelSize: 15
            model: ListModel {}
            contentItem: Text {
                text: inputDeviceComboBox.currentText
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                font.pixelSize: 15
                width: parent.width
                color: textColor
                leftPadding: 10
            }
            onCurrentTextChanged: {
                if (!window.blockInputSignal) {
                    soundPanel.onRecordingDeviceChanged(inputDeviceComboBox.currentText);
                }
            }
        }

        Button {
            id: inputMuteButton
            visible: !mixerOnly
            Layout.leftMargin: 10
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            flat: true
            icon.source: inputIcon
            Layout.bottomMargin: 10
            onClicked: {
                soundPanel.onInputMuteButtonClicked()
            }
        }

        Slider {
            id: inputSlider
            visible: !mixerOnly
            value: soundPanel.recordingVolume
            from: 0
            to: 100
            stepSize: 1
            Layout.fillWidth: true
            Layout.preferredHeight: -1
            Layout.bottomMargin: 10
            onValueChanged: {
                soundPanel.onRecordingVolumeChanged(value)
            }
        }

        Label {
            id: inputVolume
            visible: !mixerOnly
            text: String(inputSlider.value)
            Layout.rightMargin: 25
            Layout.leftMargin: 10
            Layout.preferredHeight: -1
            Layout.preferredWidth: 20
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 20
            Layout.bottomMargin: 10
        }

        Rectangle {
            id: appSeparator
            visible: !mixerOnly
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: borderColor
            Layout.columnSpan: 3
            Layout.bottomMargin: 10
        }

        Item {
            visible: mixerOnly
            Layout.columnSpan: 3
            Layout.preferredHeight: 10
        }

        Repeater {
            id: appRepeater
            model: appModel
            onCountChanged: {
                window.height = gridLayout.implicitHeight + (50 * appRepeater.count)
            }
            delegate: RowLayout {
                Layout.bottomMargin: 10
                Layout.leftMargin: 10
                Layout.fillWidth: true
                Layout.columnSpan: 3

                Button {
                    id: muteButton
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    flat: true
                    checkable: true
                    checked: model.isMuted
                    ToolTip.text: model.name
                    ToolTip.visible: hovered
                    ToolTip.delay: 1000
                    icon.source: model.name === "Windows system sounds" ? systemSoundsIcon : model.icon
                    icon.color: model.name === "Windows system sounds" ? undefined : "transparent"
                    onClicked: {
                        model.isMuted = !model.isMuted;
                        soundPanel.onApplicationMuteButtonClicked(model.appID, model.isMuted);
                    }
                }

                Slider {
                    id: volumeSlider
                    from: 0
                    to: 100
                    stepSize: 1
                    value: model.volume
                    Layout.fillWidth: true
                    onValueChanged: {
                        soundPanel.onApplicationVolumeSliderValueChanged(model.appID, value);
                    }
                }

                Label {
                    text: volumeSlider.value
                    Layout.rightMargin: 25
                    Layout.leftMargin: 10
                    Layout.preferredHeight: -1
                    Layout.preferredWidth: 20
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 20
                }
            }
        }
    }
}
