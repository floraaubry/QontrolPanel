pragma ComponentBehavior: Bound

import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import QtQuick
import Odizinne.QuickSoundSwitcher

ApplicationWindow {
    id: root
    height: 450
    minimumHeight: 450
    width: 900
    minimumWidth: 900
    visible: false
    transientParent: null
    title: qsTr("QuickSoundSwitcher - Settings")

    property int rowHeight: 35

    Popup {
        anchors.centerIn: parent
        width: mainPopupLyt.implicitWidth + 50
        height: implicitHeight + 20
        id: easterEggDialog
        modal: true

        Connections {
            target: Context
            function onEasterEggRequested() {
                easterEggDialog.open()
            }
        }

        ColumnLayout {
            id: mainPopupLyt
            spacing: 15
            anchors.fill: parent

            Label {
                text: "You just lost the game"
                font.bold: true
                font.pixelSize: 18
                Layout.alignment: Qt.AlignCenter
            }

            Button {
                text: "Too bad"
                onClicked: easterEggDialog.close()
                Layout.alignment: Qt.AlignCenter
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        Item {
            Layout.preferredWidth: 200
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                ListView {
                    id: sidebarList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    interactive: false
                    model: [
                        {
                            text: qsTr("General"),
                            icon: "qrc:/icons/general.svg"
                        },
                        {
                            text: qsTr("Appearance"),
                            icon: "qrc:/icons/wand.svg"
                        },
                        {
                            text: qsTr("ChatMix"),
                            icon: "qrc:/icons/headset.svg"
                        },
                        {
                            text: qsTr("Shortcuts"),
                            icon: "qrc:/icons/keyboard.svg"
                        },
                        {
                            text: qsTr("Language"),
                            icon: "qrc:/icons/language.svg"
                        },
                        {
                            text: qsTr("Debug"),
                            icon: "qrc:/icons/chip.svg"
                        }
                    ]
                    currentIndex: 0

                    Connections {
                        target: UserSettings

                        function onLanguageIndexChanged() {
                            Qt.callLater(function() {
                                sidebarList.currentIndex = 4
                            })
                        }
                    }

                    delegate: ItemDelegate {
                        id: del
                        width: parent.width
                        height: 43
                        spacing: 10
                        required property var modelData
                        required property int index

                        property bool isCurrentItem: ListView.isCurrentItem

                        highlighted: ListView.isCurrentItem
                        icon.source: del.modelData.icon
                        text: del.modelData.text
                        icon.width: 18
                        icon.height: 18

                        onClicked: {
                            if (sidebarList.currentIndex !== index) {
                                sidebarList.currentIndex = index
                                switch(index) {
                                    case 0: stackView.push(generalPaneComponent); break
                                    case 1: stackView.push(appearancePaneComponent); break
                                    case 2: stackView.push(commAppsPaneComponent); break
                                    case 3: stackView.push(shortcutsPaneComponent); break
                                    case 4: stackView.push(languagePaneComponent); break
                                    case 5: stackView.push(debugPaneComponent); break
                                }
                            }
                        }
                    }
                }
            }
        }

        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: generalPaneComponent

            popEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150; easing.type: Easing.InQuint }
                    NumberAnimation { property: "y"; from: (stackView.mirrored ? -0.3 : 0.3) * -stackView.width; to: 0; duration: 300; easing.type: Easing.OutCubic }
                }
            }

            pushEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150; easing.type: Easing.InQuint }
                    NumberAnimation { property: "y"; from: (stackView.mirrored ? -0.3 : 0.3) * stackView.width; to: 0; duration: 300; easing.type: Easing.OutCubic }
                }
            }

            popExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150; easing.type: Easing.OutQuint }
            }

            pushExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150; easing.type: Easing.OutQuint }
            }

            replaceEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150; easing.type: Easing.InQuint }
                    NumberAnimation { property: "y"; from: (stackView.mirrored ? -0.3 : 0.3) * stackView.width; to: 0; duration: 300; easing.type: Easing.OutCubic }
                }
            }

            Component {
                id: generalPaneComponent
                GeneralPane{}
            }

            Component {
                id: languagePaneComponent
                LanguagePane {}
            }

            Component {
                id: commAppsPaneComponent
                CommAppsPane {}
            }

            Component {
                id: shortcutsPaneComponent
                ShortcutsPane {}
            }

            Component {
                id: appearancePaneComponent
                AppearancePane {}
            }

            Component {
                id: debugPaneComponent
                DebugPane {}
            }
        }
    }
}

