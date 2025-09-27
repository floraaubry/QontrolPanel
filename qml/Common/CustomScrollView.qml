import QtQuick
import QtQuick.Controls
import Odizinne.QontrolPanel

ScrollView {
    id: control

    ScrollBar.vertical.policy: control.contentHeight > control.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
    ScrollBar.horizontal.policy: control.contentWidth > control.width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

    Component.onCompleted: {
        if (ScrollBar.vertical.contentItem) {
            ScrollBar.vertical.contentItem.color = Qt.binding(function() {
                return Constants.darkMode ? "#9f9f9f" : "#666666"
            })
        }
        if (ScrollBar.horizontal.contentItem) {
            ScrollBar.horizontal.contentItem.color = Qt.binding(function() {
                return Constants.darkMode ? "#9f9f9f" : "#666666"
            })
        }
    }
}
