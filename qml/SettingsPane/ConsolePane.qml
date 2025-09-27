pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

ColumnLayout {
    id: logViewer
    spacing: 3

    property string selectedSender: "All"
    property bool autoScroll: true

    Component.onCompleted: {
        LogBridge.applyFilter(selectedSender)
    }

    Connections {
        target: LogBridge

        function onLogEntryAdded(message, type, sender) {
            LogBridge.applyFilter(logViewer.selectedSender)

            if (logViewer.autoScroll) {
                Qt.callLater(logViewer.scrollToBottom)
            }
        }
    }

    Label {
        text: qsTr("Console output")
        font.pixelSize: 22
        font.bold: true
        Layout.bottomMargin: 15
    }

    RowLayout {
        spacing: 10

        Label {
            text: "Filter by:"
        }

        CustomComboBox {
            id: senderFilter
            Layout.preferredWidth: 200
            model: ListModel {
                id: senderOptions
                ListElement { text: "All"; value: "All" }
                ListElement { text: "AudioManager"; value: "AudioManager" }
                ListElement { text: "MediaSessionManager"; value: "MediaSessionManager" }
                ListElement { text: "MonitorManager"; value: "MonitorManager" }
                ListElement { text: "SoundPanelBridge"; value: "SoundPanelBridge" }
                ListElement { text: "Updater"; value: "Updater" }
                ListElement { text: "ShortcutManager"; value: "ShortcutManager" }
                ListElement { text: "Core"; value: "Core" }
                ListElement { text: "LocalServer"; value: "LocalServer" }
                ListElement { text: "UI"; value: "UI" }
                ListElement { text: "PowerManager"; value: "PowerManager" }
                ListElement { text: "HeadsetControlManager"; value: "HeadsetControlManager" }
                ListElement { text: "WindowFocusManager"; value: "WindowFocusManager" }
            }
            textRole: "text"
            valueRole: "value"
            onCurrentValueChanged: {
                logViewer.selectedSender = currentValue
                LogBridge.applyFilter(logViewer.selectedSender)
            }
        }

        Item {
            Layout.fillWidth: true
        }

        CheckBox {
            text: "Auto-scroll"
            checked: logViewer.autoScroll
            onCheckedChanged: logViewer.autoScroll = checked
        }

        Button {
            text: "Copy All"
            onClicked: logViewer.copyAllLogs()
        }

        Button {
            text: "Clear"
            onClicked: LogBridge.clearLogs()
        }
    }

    CustomScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.topMargin: 5
        clip: true
        background: Rectangle {
            color: palette.window
            border.color: Constants.darkMode ? "#3e3e3e" : palette.mid
            border.width: 1
            radius: 4
        }

        ListView {
            id: logListView
            model: LogBridge.filteredModel
            clip: true
            delegate: Label {
                required property var model
                width: logListView.width
                wrapMode: Text.Wrap
                font.family: "Consolas, Monaco, Courier New, monospace"
                font.pixelSize: 12
                textFormat: Text.RichText
                text: {
                    let typeColor = ""
                    switch(model.type) {
                        case LogManager.Info:
                        typeColor = "#00ff00"
                        break
                        case LogManager.Warning:
                        typeColor = "#ff8800"
                        break
                        case LogManager.Critical:
                        typeColor = "#ff2222"
                        break
                        default:
                        typeColor = palette.text
                    }
                    let senderColor = logViewer.getSenderColor(model.sender)
                    let message = model.message || ""  // Provide fallback for undefined
                    let regex = /^(\[[^\]]+\]\s+)(\w+)(\s+)\[(\w+)\](\s+-.*)$/
                    let match = message.match(regex)
                    if (match) {
                        let timestamp = match[1]
                        let sender = match[2]
                        let spacer1 = match[3]
                        let type = match[4]
                        let content = match[5]
                        return `<span style="color: #888888;">${timestamp}</span><span style="color: ${senderColor};">${sender}</span><span style="color: #888888;">${spacer1}</span><span style="color: ${typeColor};">[${type}]</span><span style="color: ${palette.text};">${content}</span>`
                    } else {
                        return `<span style="color: ${typeColor};">${message}</span>`
                    }
                }
                topPadding: 1
                bottomPadding: 1
                leftPadding: 5
                rightPadding: 5
            }
        }
    }

    function getSenderColor(sender) {
        switch(sender) {
        case "AudioManager":
            return "#ff1493"      // Deep Pink
        case "MediaSessionManager":
            return "#00bfff"      // Deep Sky Blue
        case "MonitorManager":
            return "#32cd32"      // Lime Green
        case "SoundPanelBridge":
            return "#ff6600"      // Vibrant Orange
        case "Updater":
            return "#9932cc"      // Dark Orchid
        case "ShortcutManager":
            return "#1e90ff"      // Dodger Blue
        case "Core":
            return "#dc143c"      // Crimson
        case "LocalServer":
            return "#8a2be2"      // Blue Violet
        case "UI":
            return "#ffd700"      // Gold
        case "PowerManager":
            return "#ff69b4"      // Hot Pink
        case "HeadsetControlManager":
            return "#7fff00"      // Chartreuse
        case "WindowFocusManager":
            return "#00ff7f"      // Spring Green
        default:
            return "#ffffff"      // White
        }
    }

    function copyAllLogs() {
        let version = SoundPanelBridge.getAppVersion()
        let commitHash = SoundPanelBridge.getCommitHash()
        let currentDate = new Date().toISOString().split('T')[0]
        let header = "QontrolPanel Log Export\n"
        header += "========================\n"
        header += "Version: " + version + "\n"
        header += "Commit: " + commitHash + "\n"
        header += "Export Date: " + currentDate + "\n"
        header += "Filter: " + selectedSender + "\n"
        header += "Total Entries: " + LogBridge.filteredModel.count + "\n"
        header += "========================\n\n"

        let allLogsText = header
        for (let i = 0; i < LogBridge.filteredModel.count; i++) {
            let item = LogBridge.filteredModel.get(i)
            allLogsText += item.message + "\n"
        }

        if (LogBridge.filteredModel.count > 0) {
            allLogsText = allLogsText.slice(0, -1)
        }

        hiddenTextEdit.text = allLogsText
        hiddenTextEdit.selectAll()
        hiddenTextEdit.copy()
    }

    function scrollToBottom() {
        if (LogBridge.filteredModel.count > 0) {
            logListView.positionViewAtEnd()
        }
    }

    TextEdit {
        id: hiddenTextEdit
        visible: false
        Layout.preferredWidth: 0
        Layout.preferredHeight: 0
    }
}
