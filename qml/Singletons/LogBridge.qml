pragma Singleton
import QtQuick
import Odizinne.QontrolPanel

Item {
    id: logBridge

    property int maxLogEntries: 500
    property ListModel logModel: ListModel {}
    property ListModel filteredModel: ListModel {}
    property string currentFilter: "All"
    property var senderRegex: /^\[[^\]]+\]\s+(\w+)\s+\[\w+\]/

    signal logEntryAdded(string message, int type, string sender)
    signal logsCleared()

    Component.onCompleted: {
        LogManager.setQmlReady()
    }

    Connections {
        target: LogManager

        function onLogReceived(message, type) {
            logBridge.addLogEntry(message, type)
        }

        function onBufferedLogsReady(logs) {
            for (let i = 0; i < logs.length; i++) {
                let log = logs[i]
                logBridge.addLogEntry(log.message, log.type)
            }
        }
    }

    function addLogEntry(message, type) {
        let sender = extractSenderFromMessage(message)

        // Proper cleanup - remove excess entries FIRST
        while (logModel.count >= maxLogEntries) {
            logModel.remove(0)
        }

        let newEntry = {
            "message": message,
            "type": type,
            "sender": sender
        }

        logModel.append(newEntry)

        // Only add to filtered model if it matches current filter
        if (currentFilter === "All" || sender === currentFilter) {
            filteredModel.append(newEntry)
        }

        logEntryAdded(message, type, sender)
    }

    function extractSenderFromMessage(message) {
        let match = message.match(senderRegex)
        return match ? match[1] : "Unknown"
    }

    function clearLogs() {
        logModel.clear()
        filteredModel.clear()
        logsCleared()
    }

    function applyFilter(selectedSender) {
        if (selectedSender === currentFilter) return // No change needed

        currentFilter = selectedSender
        filteredModel.clear()

        if (selectedSender === "All") {
            // Copy all entries
            for (let i = 0; i < logModel.count; i++) {
                filteredModel.append(logModel.get(i))
            }
        } else {
            // Filter by sender
            for (let i = 0; i < logModel.count; i++) {
                let item = logModel.get(i)
                if (item.sender === selectedSender) {
                    filteredModel.append(item)
                }
            }
        }
    }
}
