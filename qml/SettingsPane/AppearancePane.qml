import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import QtQuick
import Odizinne.QontrolPanel

ColumnLayout {
    spacing: 3

    Label {
        text: qsTr("Appearance & Position")
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
                title: qsTr("Panel position")
                description: ""

                additionalControl: CustomComboBox {
                    Layout.preferredHeight: 35
                    model: [qsTr("Top"), qsTr("Bottom"), qsTr("Left"), qsTr("Right")]
                    currentIndex: UserSettings.panelPosition
                    onActivated: UserSettings.panelPosition = currentIndex
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Taskbar offset")
                description: qsTr("Windows taskbar size (Only set if using W11 and taskbar is not at screen bottom)")

                additionalControl: SpinBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    from: 0
                    to: 200
                    editable: true
                    value: UserSettings.taskbarOffset
                    onValueModified: UserSettings.taskbarOffset = value
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Panel X margin")
                description: qsTr("Control panel X axis margin")

                additionalControl: SpinBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    from: 0
                    to: 200
                    editable: true
                    value: UserSettings.xAxisMargin
                    onValueModified: UserSettings.xAxisMargin = value
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Panel Y margin")
                description: qsTr("Control panel Y axis margin")

                additionalControl: SpinBox {
                    Layout.preferredHeight: 35
                    Layout.preferredWidth: 160
                    from: 0
                    to: 200
                    editable: true
                    value: UserSettings.yAxisMargin
                    onValueModified: UserSettings.yAxisMargin = value
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Media info display")
                description: qsTr("Display currently playing media from Windows known sources")

                additionalControl: CustomComboBox {
                    Layout.preferredHeight: 35
                    model: [qsTr("Flyout (interactive)"), qsTr("Panel (informative)"), qsTr("Hidden")]
                    currentIndex: UserSettings.mediaMode
                    onActivated: UserSettings.mediaMode = currentIndex
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Applications and devices label")
                description: ""

                additionalControl: LabeledSwitch {
                    checked: UserSettings.displayDevAppLabel
                    onClicked: UserSettings.displayDevAppLabel = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Show audio level")
                description: qsTr("Display audio level value in slider")

                additionalControl: LabeledSwitch {
                    checked: UserSettings.showAudioLevel
                    onClicked: UserSettings.showAudioLevel = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Opacity animations")

                additionalControl: LabeledSwitch {
                    checked: UserSettings.opacityAnimations
                    onClicked: UserSettings.opacityAnimations = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Display device icon")

                additionalControl: LabeledSwitch {
                    checked: UserSettings.deviceIcon
                    onClicked: UserSettings.deviceIcon = checked
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Tray icon theme")
                description: qsTr("Choose the color of the system tray icon")
                additionalControl: CustomComboBox {
                    Layout.preferredHeight: 35
                    model: [qsTr("Auto"), qsTr("Dark"), qsTr("Light")]
                    currentIndex: UserSettings.trayIconTheme
                    onActivated: UserSettings.trayIconTheme = currentIndex
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Tray icon style")
                description: qsTr("Choose the appearance of the system tray icon")
                additionalControl: CustomComboBox {
                    Layout.preferredHeight: 35
                    model: [qsTr("Normal"), qsTr("Filled"), qsTr("Battery")]
                    currentIndex: UserSettings.iconStyle
                    onActivated: UserSettings.iconStyle = currentIndex
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Avoid applications overflow")
                description: qsTr("Wrap applications in scrollview when more than 4 applications are displayed")

                additionalControl: LabeledSwitch {
                    checked: UserSettings.avoidApplicationsOverflow
                    onClicked: UserSettings.avoidApplicationsOverflow = checked
                }
            }
        }
    }
}
