pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import Odizinne.QontrolPanel

ColumnLayout {
    id: logViewer
    spacing: 3

    property string selectedSender: "All"
    property int maxLogEntries: 1000
    property bool autoScroll: true

    Component.onCompleted: {
        LogManager.setQmlReady()
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
            model: ListModel {
                id: senderOptions
                ListElement { text: "All"; value: "All" }
                ListElement { text: "AudioManager"; value: "AudioManager" }
                ListElement { text: "MediaSessionManager"; value: "MediaSessionManager" }
                ListElement { text: "MonitorManager"; value: "MonitorManager" }
                ListElement { text: "SoundPanelBridge"; value: "SoundPanelBridge" }
                ListElement { text: "Updater"; value: "Updater" }
                ListElement { text: "ShortcutManager"; value: "ShortcutManager" }
                ListElement { text: "FileSystem"; value: "FileSystem" }
                ListElement { text: "Core"; value: "Core" }
                ListElement { text: "LocalServer"; value: "LocalServer" }
                ListElement { text: "UI"; value: "UI" }
                ListElement { text: "HeadsetControlManager"; value: "HeadsetControlManager" }
            }
            textRole: "text"
            valueRole: "value"
            onCurrentValueChanged: {
                logViewer.selectedSender = currentValue
                filteredModel.applyFilter()
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
            onClicked: logViewer.clearLogs()
        }
    }

    ScrollView {
        id: scrollView
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.topMargin: 5
        clip: true
        background: Rectangle {
            color: "#1e1e1e"
            border.color: "#3e3e3e"
            border.width: 1
            radius: 4
        }

        ListView {
            id: logListView
            model: filteredModel
            clip: true

            property real savedContentY: 0

            onCountChanged: {
                if (logViewer.autoScroll) {
                    Qt.callLater(logViewer.scrollToBottom)
                } else {
                    // Restore previous scroll position
                    Qt.callLater(function() {
                        contentY = savedContentY
                    })
                }
            }

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
                        typeColor = "#00dd00"
                        break
                        case LogManager.Warning:
                        typeColor = "#ffaa00"
                        break
                        case LogManager.Critical:
                        typeColor = "#ff4444"
                        break
                        default:
                        typeColor = "#ffffff"
                    }

                    let senderColor = logViewer.getSenderColor(model.sender)
                    let message = model.message
                    let regex = /^(\[[^\]]+\]\s+)(\w+)(\s+)\[(\w+)\](\s+-.*)$/
                    let match = message.match(regex)

                    if (match) {
                        let timestamp = match[1]
                        let sender = match[2]
                        let spacer1 = match[3]
                        let type = match[4]
                        let content = match[5]
                        return `<span style="color: #888888;">${timestamp}</span><span style="color: ${senderColor};">${sender}</span><span style="color: #888888;">${spacer1}</span><span style="color: ${typeColor};">[${type}]</span><span style="color: #ffffff;">${content}</span>`
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

    ListModel {
        id: logModel
    }

    ListModel {
        id: filteredModel

        function applyFilter() {
            // Save current scroll position if not auto-scrolling
            if (!logViewer.autoScroll) {
                logListView.savedContentY = logListView.contentY
            }

            clear()
            for (let i = 0; i < logModel.count; i++) {
                let item = logModel.get(i)
                if (logViewer.selectedSender === "All" || item.sender === logViewer.selectedSender) {
                    append(item)
                }
            }
        }
    }

    Connections {
        target: LogManager

        function onLogReceived(message, type) {
            logViewer.addLogEntry(message, type)
        }

        function onBufferedLogsReady(logs) {
            for (let i = 0; i < logs.length; i++) {
                let log = logs[i]
                logViewer.addLogEntry(log.message, log.type)
            }
        }
    }

    function getSenderColor(sender) {
        switch(sender) {
        case "AudioManager":
            return "#ff6b9d"      // Pink
        case "MediaSessionManager":
            return "#4fc3f7"      // Light Blue
        case "MonitorManager":
            return "#81c784"      // Light Green
        case "SoundPanelBridge":
            return "#ffb74d"      // Orange
        case "Updater":
            return "#ba68c8"      // Purple
        case "ShortcutManager":
            return "#64b5f6"      // Blue
        case "FileSystem":
            return "#4db6ac"      // Teal
        case "Core":
            return "#f06292"      // Pink-Red
        case "LocalServer":
            return "#9575cd"      // Light Purple
        case "UI":
            return "#ffd54f"      // Yellow
        case "HeadsetControlManager":
            return "#a1c181"      // Sage Green
        default:
            return "#cccccc"      // Light Gray
        }
    }

    function copyAllLogs() {
        // Get version info from SoundPanelBridge
        let version = SoundPanelBridge.getAppVersion()
        let commitHash = SoundPanelBridge.getCommitHash()
        let currentDate = new Date().toISOString().split('T')[0] // YYYY-MM-DD format

        // Create header
        let header = "QontrolPanel Log Export\n"
        header += "========================\n"
        header += "Version: " + version + "\n"
        header += "Commit: " + commitHash + "\n"
        header += "Export Date: " + currentDate + "\n"
        header += "Filter: " + selectedSender + "\n"
        header += "Total Entries: " + filteredModel.count + "\n"
        header += "========================\n\n"

        // Collect log entries
        let allLogsText = header
        for (let i = 0; i < filteredModel.count; i++) {
            let item = filteredModel.get(i)
            allLogsText += item.message + "\n"
        }

        if (filteredModel.count > 0) {
            // Remove the last newline
            allLogsText = allLogsText.slice(0, -1)
        }

        // Copy to clipboard
        hiddenTextEdit.text = allLogsText
        hiddenTextEdit.selectAll()
        hiddenTextEdit.copy()
    }

    function addLogEntry(message, type) {
        let sender = extractSenderFromMessage(message)

        if (logModel.count >= maxLogEntries) {
            logModel.remove(0, logModel.count - maxLogEntries + 1)
        }

        logModel.append({
                            "message": message,
                            "type": type,
                            "sender": sender
                        })

        filteredModel.applyFilter()
    }

    function extractSenderFromMessage(message) {
        let regex = /^\[[^\]]+\]\s+(\w+)\s+\[\w+\]/
        let match = message.match(regex)
        return match ? match[1] : "Unknown"
    }

    function scrollToBottom() {
        if (filteredModel.count > 0) {
            logListView.positionViewAtEnd()
        }
    }

    function clearLogs() {
        logModel.clear()
        filteredModel.clear()
    }

    // Hidden TextEdit for clipboard operations
    TextEdit {
        id: hiddenTextEdit
        visible: false
        width: 0
        height: 0
    }
}
