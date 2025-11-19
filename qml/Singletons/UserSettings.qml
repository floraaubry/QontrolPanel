pragma Singleton

import QtCore

Settings {
    property bool enableDeviceManager: true
    property bool enableApplicationMixer: true
    property bool keepAlive: false
    property int panelPosition: 1
    property int taskbarOffset: 0
    property int xAxisMargin: 12
    property int yAxisMargin: 12
    property int mediaMode: 0
    property bool displayDevAppLabel: true
    property bool closeDeviceListOnClick: true
    property int languageIndex: 0

    property var commApps: []
    property int chatMixValue: 50
    property bool chatMixEnabled: false
    property bool activateChatmix: false
    property bool showAudioLevel: true
    property int chatmixRestoreVolume: 80

    property bool globalShortcutsEnabled: true
    property int panelShortcutKey: 83        // Qt.Key_S
    property int panelShortcutModifiers: 117440512  // Qt.ControlModifier | Qt.ShiftModifier
    property int chatMixShortcutKey: 77      // Qt.Key_M
    property int chatMixShortcutModifiers: 117440512  // Qt.ControlModifier | Qt.ShiftModifier
    property bool chatMixShortcutNotification: true
    property bool autoUpdateTranslations: false
    property bool opacityAnimations: true
    property bool firstRun: true

    property bool deviceIcon: true
    property int trayIconTheme: 0
    property int iconStyle: 0

    property bool autoFetchForAppUpdates: false
    property bool headsetcontrolMonitoring: true
    property bool headsetcontrolLights: true
    property int headsetcontrolSidetone: 0
    property bool allowBrightnessControl: true
    property bool avoidApplicationsOverflow: false
    property int ddcciQueueDelay: 500

    property bool enablePowerMenu: true
    property bool showPowerDialogConfirmation: true
    property int confirmationTimeout: 60

    property int ddcciBrightness: 100

    property bool audioManagerLogging: true
    property bool mediaSessionManagerLogging: true
    property bool monitorManagerLogging: true
    property bool soundPanelBridgeLogging: true
    property bool updaterLogging: true
    property bool shortcutManagerLogging: true
    property bool coreLogging: true
    property bool localServerLogging: true
    property bool uiLogging: true
    property bool powerManagerLogging: true
    property bool headsetControlManagerLogging: true
    property bool windowFocusManagerLogging: true
    property bool displayBatteryFooter: true
    property int panelStyle: 0
}
