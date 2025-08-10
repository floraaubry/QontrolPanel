pragma Singleton
import QtQuick
import Odizinne.QuickSoundSwitcher

QtObject {
    property bool darkMode: SoundPanelBridge.getDarkMode()
    property color footerColor: darkMode ? "#1c1c1c" : "#eeeeee"
    property color footerBorderColor: darkMode ? "#0F0F0F" : "#A0A0A0"
    property color panelColor: darkMode ? "#242424" : "#f2f2f2"
    property color cardColor: darkMode ? "#2b2b2b" : "#fbfbfb"
    property color cardBorderColor: darkMode ? "#0F0F0F" : "#D0D0D0"
    property color separatorColor: darkMode ? "#E3E3E3" : "#A0A0A0"
    property string systemIcon: darkMode ? "qrc:/icons/system_light.png" : "qrc:/icons/system_dark.png"

    function getTrayIcon(volume, muted) {
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
}
