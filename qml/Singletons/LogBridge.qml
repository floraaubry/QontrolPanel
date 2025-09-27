pragma Singleton
import QtQuick
import Odizinne.QontrolPanel

Item {
    id: logBridge

    property int maxLogEntries: 1000

    property ListModel logModel: ListModel {}
    property ListModel filteredModel: ListModel {}

    signal logEntryAdded(string message, int type, string sender)
    signal logsCleared()
    signal filterRequested(string selectedSender)

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

        if (logModel.count >= maxLogEntries) {
            logModel.remove(0, logModel.count - maxLogEntries + 1)
        }

        logModel.append({
            "message": message,
            "type": type,
            "sender": sender
        })

        logEntryAdded(message, type, sender)
    }

    function extractSenderFromMessage(message) {
        let regex = /^\[[^\]]+\]\s+(\w+)\s+\[\w+\]/
        let match = message.match(regex)
        return match ? match[1] : "Unknown"
    }

    function clearLogs() {
        logModel.clear()
        filteredModel.clear()
        logsCleared()
    }

    function applyFilter(selectedSender) {
        filteredModel.clear()
        for (let i = 0; i < logModel.count; i++) {
            let item = logModel.get(i)
            if (selectedSender === "All" || item.sender === selectedSender) {
                filteredModel.append(item)
            }
        }
    }
}
