pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

ColumnLayout {
    spacing: 3

    Label {
        text: qsTr("Renaming")
        font.pixelSize: 22
        font.bold: true
        Layout.bottomMargin: 15
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 10
        Layout.bottomMargin: 15

        Label {
            text: qsTr("Show:")
        }

        Item {
            Layout.fillWidth: true
        }

        CustomComboBox {
            id: viewSelector
            model: [qsTr("Devices"), qsTr("Applications"), qsTr("Streams")]
            currentIndex: 0
        }
    }

    CustomScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ColumnLayout {
            width: parent.width
            spacing: 3

            Repeater {
                model: AudioBridge.outputDevices

                Card {
                    id: deviceCard
                    Layout.fillWidth: true
                    visible: viewSelector.currentIndex === 0
                    required property var model
                    title: model.name || ""

                    additionalControl: TextField {
                        Layout.preferredWidth: 200
                        placeholderText: qsTr("Custom name")
                        text: AudioBridge.getCustomDeviceName(deviceCard.model.name || "")

                        onTextChanged: {
                            if (text !== AudioBridge.getCustomDeviceName(deviceCard.model.name || "")) {
                                AudioBridge.setCustomDeviceName(deviceCard.model.name || "", text)
                            }
                        }
                    }
                }
            }

            Repeater {
                model: AudioBridge.inputDevices

                Card {
                    id: inDeviceCard
                    Layout.fillWidth: true
                    required property var model
                    title: model.name || ""

                    additionalControl: TextField {
                        Layout.preferredWidth: 250
                        placeholderText: qsTr("Custom name")
                        text: AudioBridge.getCustomDeviceName(inDeviceCard.model.name || "")

                        onTextChanged: {
                            if (text !== AudioBridge.getCustomDeviceName(inDeviceCard.model.name || "")) {
                                AudioBridge.setCustomDeviceName(inDeviceCard.model.name || "", text)
                            }
                        }
                    }
                }
            }

            Repeater {
                model: AudioBridge.groupedApplications

                Card {
                    id: groupAppCard
                    visible: viewSelector.currentIndex === 1
                    Layout.fillWidth: true
                    required property var model
                    title: model.displayName || ""

                    additionalControl: TextField {
                        Layout.preferredWidth: 200
                        placeholderText: qsTr("Custom name")
                        text: AudioBridge.getCustomExecutableName(groupAppCard.model.executableName || "")

                        onTextChanged: {
                            if (text !== AudioBridge.getCustomExecutableName(groupAppCard.model.executableName || "")) {
                                AudioBridge.setCustomExecutableName(groupAppCard.model.executableName || "", text)
                            }
                        }
                    }
                }
            }

            Repeater {
                model: AudioBridge.applications

                Card {
                    id: appCard
                    Layout.fillWidth: true
                    required property var model
                    title: model.name || ""
                    visible: viewSelector.currentIndex === 2

                    additionalControl: TextField {
                        Layout.preferredWidth: 200
                        placeholderText: qsTr("Custom name")
                        text: AudioBridge.getCustomApplicationName(appCard.model.name || "", appCard.model.streamIndex || 0)

                        onTextChanged: {
                            if (text !== AudioBridge.getCustomApplicationName(appCard.model.name || "", appCard.model.streamIndex || 0)) {
                                AudioBridge.setCustomApplicationName(appCard.model.name || "", appCard.model.streamIndex || 0, text)
                            }
                        }
                    }
                }
            }
        }
    }
}
