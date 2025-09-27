pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

Dialog {
    id: loggingDialog
    width: 500
    modal: true
    title: qsTr("Log Categories")
    standardButtons: Dialog.Close
    property bool masterEnabled: {
        return UserSettings.audioManagerLogging ||
               UserSettings.mediaSessionManagerLogging ||
               UserSettings.monitorManagerLogging ||
               UserSettings.soundPanelBridgeLogging ||
               UserSettings.updaterLogging ||
               UserSettings.shortcutManagerLogging ||
               UserSettings.coreLogging ||
               UserSettings.localServerLogging ||
               UserSettings.uiLogging ||
               UserSettings.powerManagerLogging ||
               UserSettings.headsetControlManagerLogging ||
               UserSettings.windowFocusManagerLogging
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        RowLayout {
            Label {
                text: qsTr("Enable All Logging")
            }

            Switch {
                id: masterSwitch
                checked: loggingDialog.masterEnabled
                onToggled: {
                    let enableAll = checked
                    UserSettings.audioManagerLogging = enableAll
                    UserSettings.mediaSessionManagerLogging = enableAll
                    UserSettings.monitorManagerLogging = enableAll
                    UserSettings.soundPanelBridgeLogging = enableAll
                    UserSettings.updaterLogging = enableAll
                    UserSettings.shortcutManagerLogging = enableAll
                    UserSettings.coreLogging = enableAll
                    UserSettings.localServerLogging = enableAll
                    UserSettings.uiLogging = enableAll
                    UserSettings.powerManagerLogging = enableAll
                    UserSettings.headsetControlManagerLogging = enableAll
                    UserSettings.windowFocusManagerLogging = enableAll

                    LogManager.setAudioManagerLogging(enableAll)
                    LogManager.setMediaSessionManagerLogging(enableAll)
                    LogManager.setMonitorManagerLogging(enableAll)
                    LogManager.setSoundPanelBridgeLogging(enableAll)
                    LogManager.setUpdaterLogging(enableAll)
                    LogManager.setShortcutManagerLogging(enableAll)
                    LogManager.setCoreLogging(enableAll)
                    LogManager.setLocalServerLogging(enableAll)
                    LogManager.setUiLogging(enableAll)
                    LogManager.setPowerManagerLogging(enableAll)
                    LogManager.setHeadsetControlManagerLogging(enableAll)
                    LogManager.setWindowFocusManagerLogging(enableAll)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Constants.darkMode ? "#3e3e3e" : palette.mid
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                width: parent.width
                spacing: 8

                RowLayout {
                    spacing: 20

                    ColumnLayout {
                        spacing: 8
                        Layout.fillWidth: true

                        RowLayout {
                            Label {
                                text: "AudioManager"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.audioManagerLogging
                                onToggled: {
                                    UserSettings.audioManagerLogging = checked
                                    LogManager.setAudioManagerLogging(checked)
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "MediaSessionManager"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.mediaSessionManagerLogging
                                onToggled: {
                                    UserSettings.mediaSessionManagerLogging = checked
                                    LogManager.setMediaSessionManagerLogging(checked)
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "MonitorManager"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.monitorManagerLogging
                                onToggled: {
                                    UserSettings.monitorManagerLogging = checked
                                    LogManager.setMonitorManagerLogging(checked)
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "SoundPanelBridge"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.soundPanelBridgeLogging
                                onToggled: {
                                    UserSettings.soundPanelBridgeLogging = checked
                                    LogManager.setSoundPanelBridgeLogging(checked)
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "Updater"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.updaterLogging
                                onToggled: {
                                    UserSettings.updaterLogging = checked
                                    LogManager.setUpdaterLogging(checked)
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "ShortcutManager"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.shortcutManagerLogging
                                onToggled: {
                                    UserSettings.shortcutManagerLogging = checked
                                    LogManager.setShortcutManagerLogging(checked)
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        spacing: 8
                        Layout.fillWidth: true

                        RowLayout {
                            Label {
                                text: "Core"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.coreLogging
                                onToggled: {
                                    UserSettings.coreLogging = checked
                                    LogManager.setCoreLogging(checked)
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "LocalServer"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.localServerLogging
                                onToggled: {
                                    UserSettings.localServerLogging = checked
                                    LogManager.setLocalServerLogging(checked)
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "UI"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.uiLogging
                                onToggled: {
                                    UserSettings.uiLogging = checked
                                    LogManager.setUiLogging(checked)
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "PowerManager"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.powerManagerLogging
                                onToggled: {
                                    UserSettings.powerManagerLogging = checked
                                    LogManager.setPowerManagerLogging(checked)
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "HeadsetControlManager"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.headsetControlManagerLogging
                                onToggled: {
                                    UserSettings.headsetControlManagerLogging = checked
                                    LogManager.setHeadsetControlManagerLogging(checked)
                                }
                            }
                        }

                        RowLayout {
                            Label {
                                text: "WindowFocusManager"
                                Layout.fillWidth: true
                            }
                            Switch {
                                checked: UserSettings.windowFocusManagerLogging
                                onToggled: {
                                    UserSettings.windowFocusManagerLogging = checked
                                    LogManager.setWindowFocusManagerLogging(checked)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
