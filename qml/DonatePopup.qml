import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import QtQuick
import Odizinne.QuickSoundSwitcher

Popup {
    id: donatePopup
    modal: true
    visible: false
    width: 350
    height: implicitHeight + 30
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 20

        Label {
            text: qsTr("Found this app useful?")
            Layout.fillWidth: true
            font.bold: true
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
            color: Constants.darkMode ? "#FFFFFF" : "#000000"
        }

        Label {
            text: qsTr("This app is made with care by an independent developer and is not financed by ad revenue.\nIf you'd like to support my work, any contribution would be greatly appreciated!")
            Layout.fillWidth: true
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: Constants.darkMode ? "#CCCCCC" : "#333333"
            lineHeight: 1.2
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            property int buttonWidth: Math.max(donateButton.implicitWidth, noThanksButton.implicitWidth)

            Button {
                id: donateButton
                Layout.fillWidth: true
                Layout.preferredWidth: parent.buttonWidth
                text: qsTr("Support")
                icon.source: "qrc:/icons/donate.svg"
                icon.color: "#f05670"
                icon.width: 20
                icon.height: 20
                font.bold: true
                onClicked: {
                    Qt.openUrlExternally("https://ko-fi.com/odizinne")
                    donatePopup.close()
                }
            }

            Button {
                id: noThanksButton
                Layout.fillWidth: true
                Layout.preferredWidth: parent.buttonWidth
                text: qsTr("Maybe later")
                onClicked: {
                    donatePopup.close()
                }
            }
        }
    }
}
