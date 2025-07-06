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
}
