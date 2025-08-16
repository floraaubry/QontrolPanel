import QtQuick.Controls.FluentWinUI3
import QtQuick
import Odizinne.QuickSoundSwitcher
Dialog {
    id: donatePopup
    modal: true
    visible: false
    width: 380
    focus: true
    title: qsTr("Found this app useful?")
    Label {
        anchors.fill: parent
        text: qsTr("This app is made with care by an independent developer and is not financed by ad revenue.\nIf you'd like to support my work, any contribution would be greatly appreciated!")
        wrapMode: Text.WordWrap
        lineHeight: 1.2
        opacity: 0.7
    }
    footer: DialogButtonBox {
        Button {
            text: qsTr("Support")
            icon.source: "qrc:/icons/donate.svg"
            icon.width: 16
            icon.height: 16
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                Qt.openUrlExternally("https://ko-fi.com/odizinne")
                donatePopup.close()
            }
        }
        Button {
            text: qsTr("Maybe later")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: {
                donatePopup.close()
            }
        }
    }
}
