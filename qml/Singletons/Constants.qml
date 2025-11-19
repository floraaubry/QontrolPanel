pragma Singleton
import QtQuick
import Odizinne.QontrolPanel

Item {
    id: constants
    property bool darkMode: Qt.application.styleHints.colorScheme === Qt.ColorScheme.Dark
    property color accentColor: darkMode ? palette.accent : palette.highlight
    property color footerColor: darkMode ? "#1c1c1c" : "#eeeeee"
    property color footerBorderColor: darkMode ? "#0F0F0F" : "#A0A0A0"
    property color panelColor: darkMode ? "#242424" : "#f2f2f2"
    property color cardColor: darkMode ? "#2b2b2b" : "#fbfbfb"
    property color cardBorderColor: darkMode ? "#1d1d1d" : "#e5e5e5"
    property color separatorColor: darkMode ? "#E3E3E3" : "#A0A0A0"
    property string systemIcon: darkMode ? "qrc:/icons/system_light.png" : "qrc:/icons/system_dark.png"

    // Battery charging animation timer
    property int chargingAnimationFrame: 20

    Timer {
        id: chargingTimer
        interval: 1000
        repeat: true
        running: true

        onTriggered: {
            switch (constants.chargingAnimationFrame) {
                case 20:
                parent.chargingAnimationFrame = 40
                break
                case 40:
                parent.chargingAnimationFrame = 60
                break
                case 60:
                parent.chargingAnimationFrame = 80
                break
                case 80:
                parent.chargingAnimationFrame = 100
                break
                case 100:
                parent.chargingAnimationFrame = 20
                break
                default:
                parent.chargingAnimationFrame = 20
                break
            }
        }
    }

    function getTrayIcon(volume, muted) {
        // Check for battery icon style first
        if (UserSettings.iconStyle === 2 && HeadsetControlManager.anyDeviceFound) {
            if (HeadsetControlManager.batteryStatus === "BATTERY_CHARGING") {
                return getBatteryChargingIcon()
            } else {
                return getBatteryIcon(HeadsetControlManager.batteryLevel)
            }
        }

        let theme
        if (UserSettings.trayIconTheme === 1) {
            // Dark
            theme = "dark"
        } else if (UserSettings.trayIconTheme === 2) {
            // Light
            theme = "light"
        } else {
            // Auto
            theme = darkMode ? "light" : "dark"
        }

        let volumeLevel
        if (muted || volume === 0) {
            volumeLevel = "0"
        } else if (volume > 66) {
            volumeLevel = "100"
        } else if (volume > 33) {
            volumeLevel = "66"
        } else {
            volumeLevel = "33"
        }

        let filledSuffix = UserSettings.iconStyle === 1 ? "_filled" : ""
        return `qrc:/icons/tray_${theme}_${volumeLevel}${filledSuffix}.png`
    }

    function getBatteryIcon(batteryLevel) {
        let theme = darkMode ? "light" : "dark"

        if (batteryLevel === -1) return ""

        // Map battery level to icon ranges
        let iconLevel
        if (batteryLevel >= 80) {
            iconLevel = "100"
        } else if (batteryLevel >= 60) {
            iconLevel = "080"
        } else if (batteryLevel >= 40) {
            iconLevel = "060"
        } else if (batteryLevel >= 20) {
            iconLevel = "040"
        } else {
            iconLevel = "020"
        }

        return `qrc:/icons/headsetcontrol/battery-${iconLevel}-${theme}.png`
    }

    function getBatteryChargingIconStatic(batteryLevel) {
        let theme = darkMode ? "light" : "dark"

        if (batteryLevel === -1) return ""

        // Map battery level to icon ranges
        let iconLevel
        if (batteryLevel >= 80) {
            iconLevel = "100"
        } else if (batteryLevel >= 60) {
            iconLevel = "80"
        } else if (batteryLevel >= 40) {
            iconLevel = "60"
        } else if (batteryLevel >= 20) {
            iconLevel = "40"
        } else {
            iconLevel = "20"
        }

        return `qrc:/icons/headsetcontrol/battery-${iconLevel}-charging-${theme}.png`
    }

    function getBatteryChargingIcon() {
        let theme = darkMode ? "light" : "dark"

        if (!chargingTimer.running) {
            chargingTimer.start()
        }

        return `qrc:/icons/headsetcontrol/battery-${chargingAnimationFrame}-charging-${theme}.png`
    }
}
