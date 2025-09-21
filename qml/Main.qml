pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import QtQuick.Controls.impl
import QtQuick.Window
import Odizinne.QontrolPanel
import Qt.labs.platform as Platform

ApplicationWindow {
    id: panel
    visible: false
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "#00000000"
    width: {
        let baseWidth = 360
        if (panel.taskbarPos === "left") {
            baseWidth += UserSettings.xAxisMargin
        }
        if (panel.taskbarPos === "top" || panel.taskbarPos === "bottom" || panel.taskbarPos === "right") {
            if (panel.taskbarPos === "right") {
                baseWidth += UserSettings.xAxisMargin
            } else {
                baseWidth += UserSettings.xAxisMargin
            }
        }

        return baseWidth
    }
    height: {
        let baseMargins = 30
        let newHeight = mainLayout.implicitHeight + baseMargins
        if (mediaLayout.visible) {
            newHeight += mediaLayout.implicitHeight
            newHeight += spacer.height
        }
        newHeight += panel.maxDeviceListSpace
        if (panel.taskbarPos === "top") {
            newHeight += UserSettings.yAxisMargin
        }
        if (panel.taskbarPos === "bottom") {
            newHeight += UserSettings.yAxisMargin
        } else if (panel.taskbarPos === "left" || panel.taskbarPos === "right") {
            newHeight += UserSettings.yAxisMargin
        }

        return newHeight
    }
    palette.accent: Constants.accentColor

    property bool isAnimatingIn: false
    property bool isAnimatingOut: false
    property string taskbarPos: SoundPanelBridge.taskbarPosition
    property real listCompensationOffset: maxDeviceListSpace - currentUsedListSpace
    property real maxDeviceListSpace: {
        let outputSpace = outputDevicesRect.expandedNeededHeight || 0
        let inputSpace = inputDevicesRect.expandedNeededHeight || 0

        return outputSpace + inputSpace
    }
    property real currentUsedListSpace: {
        let usedSpace = 0
        if (outputDevicesRect.expanded) {
            usedSpace += outputDevicesRect.expandedNeededHeight || 0
        }
        if (inputDevicesRect.expanded) {
            usedSpace += inputDevicesRect.expandedNeededHeight || 0
        }
        return usedSpace
    }

    signal hideAnimationFinished()
    signal showAnimationFinished()
    signal hideAnimationStarted()
    signal globalShortcutsToggled(bool enabled)

    onVisibleChanged: {
        if (!visible) {
            if (UserSettings.opacityAnimations) {
                mediaLayout.opacity = 0
                mainLayout.opacity = 0
            }

            if (UserSettings.showAudioLevel) {
                AudioBridge.stopAudioLevelMonitoring()
                AudioBridge.stopApplicationAudioLevelMonitoring()
            }
        } else {
            if (UserSettings.showAudioLevel) {
                AudioBridge.startAudioLevelMonitoring()
                AudioBridge.startApplicationAudioLevelMonitoring()
            }
            MonitorManager.refreshMonitors()
            brightnessSlider.value = MonitorManager.brightness
        }
    }

    Component.onCompleted: {
        SoundPanelBridge.startMediaMonitoring()
    }

    PowerConfirmationWindow {
        id: powerConfirmationWindow
    }

    Connections {
        target: UserSettings
        function onOpacityAnimationsChanged() {
            if (panel.visible) return

            if (UserSettings.opacityAnimations) {
                mediaLayout.opacity = 0
                mainLayout.opacity = 0
            } else {
                mediaLayout.opacity = 1
                mainLayout.opacity = 1
            }
        }

        function onGlobalShortcutsEnabledChanged() {
            panel.globalShortcutsToggled(UserSettings.globalShortcutsEnabled)
        }
    }

    Connections {
        target: AudioBridge
        function onOutputDeviceCountChanged() {
            if (AudioBridge.outputDevices.count <= 1) {
                outputDevicesRect.expanded = false
            }
        }
        function onInputDeviceCountChanged() {
            if (AudioBridge.inputDevices.count <= 1) {
                inputDevicesRect.expanded = false
            }
        }
    }

    IntroWindow {
        id: introWindow
    }

    SystemTray {
        id: systemTray
        onTogglePanelRequested: {
            if (panel.visible) {
                panel.hidePanel()
            } else {
                panel.showPanel()
            }
        }

        onShowIntroRequested: {
            introWindow.showIntro()
        }
    }

    Shortcut {
        sequence: StandardKey.Cancel
        onActivated: {
            if (!panel.isAnimatingIn && !panel.isAnimatingOut && panel.visible) {
                panel.hidePanel()
            }
        }
    }

    ChatMixNotification {}

    MouseArea {
        height: panel.maxDeviceListSpace - panel.currentUsedListSpace
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: UserSettings.panelPosition === 0 ? parent.bottom : undefined
        anchors.top: UserSettings.panelPosition === 0 ? undefined : parent.top
        onClicked: panel.hidePanel()
    }

    Timer {
        id: contentOpacityTimer
        interval: 160
        repeat: false
        onTriggered: mainLayout.opacity = 1
    }

    Timer {
        id: flyoutOpacityTimer
        interval: 160
        repeat: false
        onTriggered: mediaLayout.opacity = 1
    }

    onHeightChanged: {
        if (isAnimatingIn && !isAnimatingOut) {
            positionPanelAtTarget()
        }
    }

    PropertyAnimation {
        id: showAnimation
        target: contentTransform
        duration: 250
        easing.type: Easing.OutCubic
        onStarted: {
            contentOpacityTimer.start()
            flyoutOpacityTimer.start()
        }
        onFinished: {
            panel.isAnimatingIn = false
            panel.showAnimationFinished()
        }
    }

    PropertyAnimation {
        id: hideAnimation
        target: contentTransform
        duration: 250
        easing.type: Easing.InCubic
        onFinished: {
            panel.visible = false
            panel.isAnimatingOut = false
            panel.hideAnimationFinished()
        }
    }

    Translate {
        id: contentTransform
        property real x: 0
        property real y: 0
    }

    Translate {
        id: listCompensationTransform
        x: 0
        y: (panel.isAnimatingIn || panel.isAnimatingOut) ? 0 : -panel.listCompensationOffset

        Behavior on y {
            enabled: !panel.isAnimatingIn && !panel.isAnimatingOut
            NumberAnimation {
                duration: 150
                easing.type: Easing.OutQuad
            }
        }
    }

    function showPanel() {
        if (isAnimatingIn || isAnimatingOut) {
            return
        }

        MonitorManager.refreshMonitors()

        isAnimatingIn = true
        panel.taskbarPos = SoundPanelBridge.taskbarPosition
        panel.visible = true

        positionPanelAtTarget()
        setInitialTransform()

        Qt.callLater(function() {
            Qt.callLater(function() {
                let newHeight = mainLayout.implicitHeight + 30
                if (mediaLayout.visible) {
                    newHeight += mediaLayout.implicitHeight
                }
                if (spacer.visible) {
                    newHeight += spacer.height
                }
                newHeight += panel.maxDeviceListSpace
                let appListView = 0
                for (let i = 0; i < appRepeater.count; ++i) {
                    let item = appRepeater.itemAt(i)
                    if (item && item.hasOwnProperty('applicationListHeight')) {
                        appListView += item.applicationListHeight || 0
                    }
                }
                newHeight += appListView
                newHeight += UserSettings.yAxisMargin

                panel.height = newHeight

                Qt.callLater(panel.startAnimation)
            })
        })
    }

    function positionPanelAtTarget() {
        const screenWidth = SoundPanelBridge.getAvailableDesktopWidth()
        const screenHeight = SoundPanelBridge.getAvailableDesktopHeight()

        switch (panel.taskbarPos) {
        case "top":
            panel.x = screenWidth - width
            panel.y = UserSettings.taskbarOffset
            break
        case "bottom":
            panel.x = screenWidth - width
            panel.y = screenHeight - height - UserSettings.taskbarOffset
            break
        case "left":
            panel.x = UserSettings.taskbarOffset
            panel.y = screenHeight - height
            break
        case "right":
            panel.x = screenWidth - width - UserSettings.taskbarOffset
            panel.y = screenHeight - height
            break
        default:
            panel.x = screenWidth - width
            panel.y = screenHeight - height - UserSettings.taskbarOffset
            break
        }
    }

    function setInitialTransform() {
        switch (panel.taskbarPos) {
        case "top":
            contentTransform.y = -cont.height
            contentTransform.x = 0
            break
        case "bottom":
            contentTransform.y = cont.height
            contentTransform.x = 0
            break
        case "left":
            contentTransform.x = -cont.width
            contentTransform.y = 0
            break
        case "right":
            contentTransform.x = cont.width
            contentTransform.y = 0
            break
        default:
            contentTransform.y = cont.height
            contentTransform.x = 0
            break
        }
    }

    function startAnimation() {
        if (!isAnimatingIn) return

        showAnimation.properties = panel.taskbarPos === "left" || panel.taskbarPos === "right" ? "x" : "y"
        showAnimation.from = panel.taskbarPos === "left" || panel.taskbarPos === "right" ? contentTransform.x : contentTransform.y
        showAnimation.to = 0
        showAnimation.start()
    }

    function hidePanel() {
        if (isAnimatingIn || isAnimatingOut) {
            return
        }

        if (executableRenameContextMenu.visible) {
            executableRenameContextMenu.close()
        }

        outputDevicesRect.closeContextMenus()
        inputDevicesRect.closeContextMenus()

        for (let i = 0; i < appRepeater.count; ++i) {
            let item = appRepeater.itemAt(i)
            if (item && item.children) {
                for (let j = 0; j < item.children.length; ++j) {
                    let child = item.children[j]
                    if (child && child.hasOwnProperty('closeContextMenus')) {
                        child.closeContextMenus()
                    }
                }
            }
        }

        outputDevicesRect.expanded = false
        inputDevicesRect.expanded = false

        for (let i = 0; i < appRepeater.count; ++i) {
            let item = appRepeater.itemAt(i)
            if (item && item.children) {
                for (let j = 0; j < item.children.length; ++j) {
                    let child = item.children[j]
                    if (child && child.hasOwnProperty('expanded')) {
                        child.expanded = false
                    }
                }
            }
        }

        powerMenu.close()

        isAnimatingOut = true
        panel.hideAnimationStarted()

        switch (panel.taskbarPos) {
        case "top":
            hideAnimation.properties = "y"
            hideAnimation.from = contentTransform.y
            hideAnimation.to = -height
            break
        case "bottom":
            hideAnimation.properties = "y"
            hideAnimation.from = contentTransform.y
            hideAnimation.to = height
            break
        case "left":
            hideAnimation.properties = "x"
            hideAnimation.from = contentTransform.x
            hideAnimation.to = -width
            break
        case "right":
            hideAnimation.properties = "x"
            hideAnimation.from = contentTransform.x
            hideAnimation.to = width
            break
        default:
            hideAnimation.properties = "y"
            hideAnimation.from = contentTransform.y
            hideAnimation.to = height
            break
        }

        hideAnimation.start()
    }

    function shouldShowSeparator(currentLayoutIndex) {
        const visibilities = [
            UserSettings.enableDeviceManager,
            UserSettings.enableApplicationMixer && AudioBridge.isReady && AudioBridge.applications.rowCount() > 0,
            UserSettings.activateChatmix,
            MonitorManager.monitorDetected && UserSettings.allowBrightnessControl
        ]

        // Current layout must be visible
        if (!visibilities[currentLayoutIndex]) return false

        // Check if ANY previous layout is visible
        for (let i = 0; i < currentLayoutIndex; i++) {
            if (visibilities[i]) return true
        }

        // No previous layout is visible, so don't show separator
        return false
    }

    Connections {
        target: SoundPanelBridge
        function onChatMixEnabledChanged(enabled) {
            UserSettings.chatMixEnabled = enabled

            if (enabled) {
                AudioBridge.applyChatMixToApplications(UserSettings.chatMixValue)
            } else {
                AudioBridge.restoreOriginalVolumes()
            }
        }

        function onUpdateAvailableNotification(version) {
            systemTray.showMessage(
                        qsTr("Update Available"),
                        qsTr("Version %1 is available for download").arg(version),
                        Platform.SystemTrayIcon.Information,
                        3000
                        )
        }
    }

    KeepAlive {}

    SettingsWindow {
        id: settingsWindow
        Connections {
            target: systemTray
            function onSettingsWindowRequested() {
                settingsWindow.show()
            }
        }
    }

    Item {
        id: cont
        anchors.bottom: UserSettings.panelPosition === 0 ? undefined : parent.bottom
        anchors.top: UserSettings.panelPosition === 0 ? parent.top : undefined
        anchors.right: parent.right
        anchors.left: parent.left
        transform: Translate {
            x: contentTransform.x
            y: contentTransform.y
        }

        width: {
            let baseWidth = 360 + 30
            if (panel.taskbarPos === "left") {
                baseWidth += UserSettings.xAxisMargin + UserSettings.taskbarOffset
            }
            if (panel.taskbarPos === "top" || panel.taskbarPos === "bottom" || panel.taskbarPos === "right") {
                if (panel.taskbarPos === "right") {
                    baseWidth += UserSettings.xAxisMargin + UserSettings.taskbarOffset
                } else {
                    baseWidth += UserSettings.xAxisMargin
                }
            }
            return baseWidth
        }

        height: {
            let baseMargins = 30
            let newHeight = mainLayout.implicitHeight + baseMargins
            if (mediaLayout.visible) {
                newHeight += mediaLayout.implicitHeight
                newHeight += spacer.height
            }
            if (panel.taskbarPos === "top") {
                newHeight += UserSettings.yAxisMargin
            }
            if (panel.taskbarPos === "bottom") {
                newHeight += UserSettings.yAxisMargin
            } else if (panel.taskbarPos === "left" || panel.taskbarPos === "right") {
                newHeight += UserSettings.yAxisMargin
            }
            return newHeight
        }

        GridLayout {
            id: mainGrid
            anchors.fill: parent
            columns: 3
            rows: 3
            columnSpacing: 0
            rowSpacing: 0

            Item {
                id: topSpacer
                Layout.row: 0
                Layout.column: 0
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.preferredHeight: (panel.taskbarPos === "top") ? UserSettings.yAxisMargin : 0
                visible: panel.taskbarPos === "top"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        panel.hidePanel()
                    }
                }
            }

            Item {
                id: leftSpacer
                Layout.row: 1
                Layout.column: 0
                Layout.fillHeight: true
                Layout.preferredWidth: (panel.taskbarPos === "left") ? UserSettings.xAxisMargin : 0
                visible: panel.taskbarPos === "left"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        panel.hidePanel()
                    }
                }
            }

            Item {
                id: contentContainer
                Layout.row: 1
                Layout.column: 1
                Layout.fillHeight: true
                Layout.preferredWidth: 360

                Rectangle {
                    anchors.fill: mainLayout
                    anchors.margins: -15
                    color: Constants.panelColor
                    radius: 12
                    Rectangle {
                        anchors.fill: parent
                        color: "#00000000"
                        radius: 12
                        border.width: 1
                        border.color: "#E3E3E3"
                        opacity: 0.15
                    }
                }

                Rectangle {
                    id: mediaLayoutBackground
                    anchors.fill: mediaLayout
                    anchors.margins: -15
                    color: Constants.panelColor
                    visible: mediaLayout.visible
                    radius: 12
                    opacity: 0

                    onVisibleChanged: {
                        if (visible) {
                            fadeInAnimation.start()
                        } else {
                            fadeOutAnimation.start()
                        }
                    }

                    PropertyAnimation {
                        id: fadeInAnimation
                        target: mediaLayoutBackground
                        property: "opacity"
                        from: 0
                        to: 1
                        duration: 300
                        easing.type: Easing.OutQuad
                    }

                    PropertyAnimation {
                        id: fadeOutAnimation
                        target: mediaLayoutBackground
                        property: "opacity"
                        from: 1
                        to: 0
                        duration: 300
                        easing.type: Easing.OutQuad
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: "#00000000"
                        radius: 12
                        border.width: 1
                        border.color: "#E3E3E3"
                        opacity: 0.2
                    }
                }

                MediaFlyoutContent {
                    id: mediaLayout
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.topMargin: 15
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    anchors.bottomMargin: 0
                    opacity: 0
                    visible: UserSettings.mediaMode === 0 && (SoundPanelBridge.mediaTitle !== "")
                    onVisibleChanged: {
                        if (panel.visible) {
                            opacity = 1
                        }
                    }

                    Behavior on opacity {
                        NumberAnimation {
                            duration: 300
                            easing.type: Easing.OutQuad
                        }
                    }
                }

                Item {
                    id: spacer
                    anchors.top: mediaLayout.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 42
                    visible: mediaLayout.visible
                }

                ColumnLayout {
                    id: mainLayout
                    anchors.top: mediaLayout.visible ? spacer.bottom : parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    anchors.bottomMargin: 15
                    anchors.topMargin: mediaLayout.visible ? 0 : 15
                    spacing: 10
                    opacity: 0

                    Behavior on opacity {
                        NumberAnimation {
                            duration: 300
                            easing.type: Easing.OutQuad
                        }
                    }

                    ColumnLayout {
                        id: deviceLayout
                        spacing: 5
                        visible: UserSettings.enableDeviceManager

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            spacing: 0
                            NFToolButton {
                                Layout.preferredHeight: 40
                                Layout.preferredWidth: 40
                                flat: true
                                property string volumeIcon: {
                                    if (AudioBridge.outputMuted || AudioBridge.outputVolume === 0) {
                                        return "qrc:/icons/panel_volume_0.svg"
                                    } else if (AudioBridge.outputVolume <= 33) {
                                        return "qrc:/icons/panel_volume_33.svg"
                                    } else if (AudioBridge.outputVolume <= 66) {
                                        return "qrc:/icons/panel_volume_66.svg"
                                    } else {
                                        return "qrc:/icons/panel_volume_100.svg"
                                    }
                                }

                                icon.source: volumeIcon
                                onClicked: {
                                    AudioBridge.setOutputMute(!AudioBridge.outputMuted)
                                }
                            }

                            ColumnLayout {
                                spacing: -4
                                Label {
                                    visible: UserSettings.displayDevAppLabel
                                    opacity: 0.5
                                    elide: Text.ElideRight
                                    Layout.preferredWidth: outputSlider.implicitWidth - 30
                                    Layout.leftMargin: 18
                                    Layout.rightMargin: 25
                                    text: AudioBridge.outputDeviceDisplayName
                                }

                                ProgressSlider {
                                    id: outputSlider
                                    value: pressed ? value : AudioBridge.outputVolume
                                    from: 0
                                    to: 100
                                    Layout.fillWidth: true
                                    audioLevel: AudioBridge.outputAudioLevel

                                    onValueChanged: {
                                        if (pressed) {
                                            AudioBridge.setOutputVolume(value)
                                        }
                                    }

                                    onPressedChanged: {
                                        if (!pressed) {
                                            AudioBridge.setOutputVolume(value)
                                            SoundPanelBridge.playFeedbackSound()
                                        }
                                    }
                                }
                            }

                            NFToolButton {
                                icon.source: "qrc:/icons/arrow.svg"
                                rotation: outputDevicesRect.expanded ? 90 : 0
                                visible: AudioBridge.isReady && AudioBridge.outputDevices.count > 1
                                Layout.preferredHeight: 35
                                Layout.preferredWidth: 35
                                Behavior on rotation {
                                    NumberAnimation {
                                        duration: 150
                                        easing.type: Easing.Linear
                                    }
                                }

                                onClicked: {
                                    outputDevicesRect.expanded = !outputDevicesRect.expanded
                                }
                            }
                        }

                        DevicesListView {
                            id: outputDevicesRect
                            model: AudioBridge.outputDevices
                            onDeviceClicked: function(name, index) {
                                AudioBridge.setOutputDevice(index)
                                if (UserSettings.closeDeviceListOnClick) {
                                    expanded = false
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            spacing: 0
                            NFToolButton {
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                flat: true
                                icon.source: AudioBridge.inputMuted ? "qrc:/icons/mic_muted.svg" : "qrc:/icons/mic.svg"
                                icon.height: 16
                                icon.width: 16
                                onClicked: {
                                    AudioBridge.setInputMute(!AudioBridge.inputMuted)
                                }
                            }

                            ColumnLayout {
                                spacing: -4
                                Label {
                                    visible: UserSettings.displayDevAppLabel
                                    opacity: 0.5
                                    elide: Text.ElideRight
                                    Layout.preferredWidth: inputSlider.implicitWidth - 30
                                    Layout.leftMargin: 18
                                    Layout.rightMargin: 25
                                    text: AudioBridge.inputDeviceDisplayName
                                }

                                ProgressSlider {
                                    id: inputSlider
                                    value: pressed ? value : AudioBridge.inputVolume
                                    from: 0
                                    to: 100
                                    audioLevel: AudioBridge.inputAudioLevel
                                    Layout.fillWidth: true

                                    onValueChanged: {
                                        if (pressed) {
                                            AudioBridge.setInputVolume(value)
                                        }
                                    }

                                    onPressedChanged: {
                                        if (!pressed) {
                                            AudioBridge.setInputVolume(value)
                                        }
                                    }
                                }
                            }

                            NFToolButton {
                                icon.source: "qrc:/icons/arrow.svg"
                                rotation: inputDevicesRect.expanded ? 90 : 0
                                Layout.preferredHeight: 35
                                Layout.preferredWidth: 35
                                visible: AudioBridge.isReady && AudioBridge.inputDevices.count > 1
                                Behavior on rotation {
                                    NumberAnimation {
                                        duration: 150
                                        easing.type: Easing.Linear
                                    }
                                }

                                onClicked: {
                                    inputDevicesRect.expanded = !inputDevicesRect.expanded
                                }
                            }
                        }

                        DevicesListView {
                            id: inputDevicesRect
                            model: AudioBridge.inputDevices
                            onDeviceClicked: function(name, index) {
                                AudioBridge.setInputDevice(index)
                                if (UserSettings.closeDeviceListOnClick) {
                                    expanded = false
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: deviceLytSeparator
                        Layout.preferredHeight: 1
                        Layout.fillWidth: true
                        color: Constants.separatorColor
                        opacity: 0.15
                        visible: panel.shouldShowSeparator(1)
                        Layout.rightMargin: -14
                        Layout.leftMargin: -14
                    }

                    ColumnLayout {
                        id: appLayout
                        spacing: 5
                        visible: UserSettings.enableApplicationMixer && AudioBridge.isReady && AudioBridge.applications.rowCount() > 0
                        Layout.fillWidth: true

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.preferredHeight: {
                                if (UserSettings.avoidApplicationsOverflow && appRepeater.count > 4) {
                                    return 4 * 45 + 3 * 5  // 4 apps * 45px height + 3 spacings * 5px
                                } else {
                                    return appRepeater.count * 45 + Math.max(0, (appRepeater.count - 1) * 5)
                                }
                            }
                            clip: false
                            ScrollBar.vertical: ScrollBar {
                                id: verticalScrollBar
                                parent: parent
                                anchors.top: parent.top
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.rightMargin: -8
                                width: 8

                                policy: (UserSettings.avoidApplicationsOverflow && appRepeater.count > 4) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

                                contentItem: Rectangle {
                                    implicitWidth: 6
                                    implicitHeight: 6
                                    radius: width / 2
                                    color: Constants.darkMode ? "#9f9f9f" : "#8a8a8a"
                                    opacity: 0.0

                                    states: State {
                                        name: "active"
                                        when: verticalScrollBar.policy === ScrollBar.AlwaysOn || (verticalScrollBar.active && verticalScrollBar.size < 1.0)
                                        PropertyChanges {
                                            target: verticalScrollBar.contentItem
                                            opacity: 0.75
                                        }
                                    }

                                    transitions: Transition {
                                        from: "active"
                                        SequentialAnimation {
                                            PauseAnimation { duration: 450 }
                                            NumberAnimation {
                                                target: verticalScrollBar.contentItem
                                                duration: 200
                                                property: "opacity"
                                                to: 0.0
                                            }
                                        }
                                    }
                                }
                            }

                            ColumnLayout {
                                width: appLayout.width
                                spacing: 5

                                Repeater {
                                    id: appRepeater
                                    model: AudioBridge.groupedApplications
                                    delegate: ColumnLayout {
                                        id: appDelegateRoot
                                        spacing: 5
                                        Layout.fillWidth: true
                                        required property var model
                                        required property int index

                                        readonly property real applicationListHeight: individualAppsRect.expandedNeededHeight

                                        RowLayout {
                                            Layout.preferredHeight: 40
                                            Layout.fillWidth: true
                                            spacing: 0

                                            NFToolButton {
                                                id: executableMuteButton
                                                Layout.preferredWidth: 40
                                                Layout.preferredHeight: 40
                                                flat: !checked
                                                checkable: true
                                                highlighted: checked
                                                checked: appDelegateRoot.model.allMuted
                                                ToolTip.text: appDelegateRoot.model.displayName
                                                ToolTip.visible: hovered
                                                ToolTip.delay: 1000
                                                opacity: highlighted ? 0.3 : (enabled ? 1 : 0.5)
                                                icon.color: "transparent"
                                                icon.source: appDelegateRoot.model.displayName === qsTr("System sounds") ? Constants.systemIcon : appDelegateRoot.model.iconPath

                                                onClicked: {
                                                    AudioBridge.setExecutableMute(appDelegateRoot.model.executableName, checked)
                                                }

                                                Component.onCompleted: {
                                                    palette.accent = palette.button
                                                }
                                            }

                                            ColumnLayout {
                                                spacing: -4

                                                Label {
                                                    visible: UserSettings.displayDevAppLabel
                                                    opacity: UserSettings.chatMixEnabled ? 0.3 : 0.5
                                                    elide: Text.ElideRight
                                                    Layout.preferredWidth: 200
                                                    Layout.leftMargin: 18
                                                    Layout.rightMargin: 25
                                                    text: {
                                                        let name = appDelegateRoot.model.displayName
                                                        if (UserSettings.chatMixEnabled && AudioBridge.isCommApp(name)) {
                                                            name += " (Comm)"
                                                        }
                                                        return name
                                                    }

                                                    MouseArea {
                                                        anchors.fill: parent
                                                        acceptedButtons: Qt.RightButton
                                                        onClicked: function(mouse) {
                                                            if (mouse.button === Qt.RightButton && appDelegateRoot.model.executableName !== "System sounds") {
                                                                executableRenameContextMenu.originalName = appDelegateRoot.model.executableName
                                                                executableRenameContextMenu.currentCustomName = AudioBridge.getCustomExecutableName(appDelegateRoot.model.executableName)
                                                                executableRenameContextMenu.popup()
                                                            }
                                                        }
                                                    }
                                                }

                                                ProgressSlider {
                                                    onActiveFocusChanged: {
                                                        focus = false
                                                    }
                                                    id: executableVolumeSlider
                                                    from: 0
                                                    to: 100
                                                    enabled: !UserSettings.chatMixEnabled && !executableMuteButton.highlighted
                                                    opacity: enabled ? 1 : 0.5
                                                    Layout.fillWidth: true
                                                    displayProgress: appDelegateRoot.model.displayName !== qsTr("System sounds")
                                                    audioLevel: appDelegateRoot.model.displayName !== qsTr("System sounds")
                                                                ? (appDelegateRoot.model.averageAudioLevel || 0)
                                                                : 0

                                                    value: pressed ? value : appDelegateRoot.model.averageVolume

                                                    onValueChanged: {
                                                        if (!UserSettings.chatMixEnabled && pressed) {
                                                            AudioBridge.setExecutableVolume(appDelegateRoot.model.executableName, value)
                                                        }
                                                    }

                                                    onPressedChanged: {
                                                        if (!pressed && !UserSettings.chatMixEnabled) {
                                                            AudioBridge.setExecutableVolume(appDelegateRoot.model.executableName, value)
                                                        }
                                                    }
                                                }
                                            }

                                            NFToolButton {
                                                onActiveFocusChanged: {
                                                    focus = false
                                                }

                                                icon.source: "qrc:/icons/arrow.svg"
                                                rotation: individualAppsRect.expanded ? 90 : 0
                                                visible: appDelegateRoot.model.sessionCount > 1
                                                Layout.preferredHeight: 35
                                                Layout.preferredWidth: 35

                                                Behavior on rotation {
                                                    NumberAnimation {
                                                        duration: 150
                                                        easing.type: Easing.Linear
                                                    }
                                                }

                                                onClicked: {
                                                    individualAppsRect.expanded = !individualAppsRect.expanded
                                                }
                                            }
                                        }

                                        ApplicationsListView {
                                            id: individualAppsRect
                                            model: AudioBridge.getSessionsForExecutable(appDelegateRoot.model.executableName)
                                            executableName: appDelegateRoot.model.executableName

                                            onApplicationVolumeChanged: function(appId, volume) {
                                                AudioBridge.setApplicationVolume(appId, volume)
                                            }

                                            onApplicationMuteChanged: function(appId, muted) {
                                                AudioBridge.setApplicationMute(appId, muted)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: appsLytSeparator
                        visible: panel.shouldShowSeparator(2)
                        Layout.preferredHeight: 1
                        Layout.fillWidth: true
                        color: Constants.separatorColor
                        opacity: 0.15
                        Layout.rightMargin: -14
                        Layout.leftMargin: -14
                    }

                    ColumnLayout {
                        visible: UserSettings.activateChatmix
                        id: chatMixLayout
                        spacing: 5

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            spacing: 0

                            NFToolButton {
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                icon.width: 15
                                icon.height: 15
                                icon.source: "qrc:/icons/headset.svg"
                                icon.color: palette.text
                                checkable: true
                                checked: !UserSettings.chatMixEnabled
                                opacity: checked ? 0.3 : 1

                                Component.onCompleted: {
                                    palette.accent = palette.button
                                }

                                onClicked: {
                                    UserSettings.chatMixEnabled = !checked

                                    if (!checked) {
                                        AudioBridge.applyChatMixToApplications(UserSettings.chatMixValue)
                                    } else {
                                        AudioBridge.restoreOriginalVolumes()
                                    }
                                }
                            }

                            ColumnLayout {
                                spacing: -4

                                Label {
                                    visible: UserSettings.displayDevAppLabel
                                    opacity: 0.5
                                    text: qsTr("ChatMix")
                                    Layout.leftMargin: 18
                                    Layout.rightMargin: 25
                                }

                                NFSlider {
                                    id: chatMixSlider
                                    value: UserSettings.chatMixValue
                                    from: 0
                                    to: 100
                                    Layout.fillWidth: true
                                    enabled: UserSettings.chatMixEnabled

                                    ToolTip {
                                        parent: chatMixSlider.handle
                                        visible: chatMixSlider.pressed || chatMixSlider.hovered
                                        delay: chatMixSlider.pressed ? 0 : 1000
                                        text: Math.round(chatMixSlider.value).toString()
                                    }

                                    onValueChanged: {
                                        UserSettings.chatMixValue = value
                                        if (UserSettings.chatMixEnabled) {
                                            AudioBridge.applyChatMixToApplications(Math.round(value))
                                        }
                                    }
                                }
                            }

                            IconImage {
                                Layout.preferredWidth: 35
                                Layout.preferredHeight: 35
                                sourceSize.width: 15
                                sourceSize.height: 15
                                color: palette.text
                                source: "qrc:/icons/music.svg"
                                enabled: UserSettings.chatMixEnabled
                            }
                        }
                    }

                    Rectangle {
                        id: chatMixLytSeparator
                        visible: panel.shouldShowSeparator(3)
                        Layout.preferredHeight: 1
                        Layout.fillWidth: true
                        color: Constants.separatorColor
                        opacity: 0.15
                        Layout.rightMargin: -14
                        Layout.leftMargin: -14
                    }

                    ColumnLayout {
                        visible: MonitorManager.monitorDetected && UserSettings.allowBrightnessControl
                        id: brightnessLayout
                        spacing: 5

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            spacing: 0

                            NFToolButton {
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                enabled: MonitorManager.nightLightSupported
                                onClicked: MonitorManager.toggleNightLight()
                                IconImage {
                                    anchors.centerIn: parent
                                    sourceSize.width: 22
                                    sourceSize.height: 22
                                    color: palette.text
                                    source: MonitorManager.nightLightEnabled ? "qrc:/icons/nightlight.svg" : "qrc:/icons/brightness.svg"
                                }
                            }

                            ColumnLayout {
                                spacing: -4

                                Label {
                                    visible: UserSettings.displayDevAppLabel
                                    opacity: 0.5
                                    text: qsTr("Brightness")
                                    Layout.leftMargin: 18
                                    Layout.rightMargin: 25
                                }

                                Slider {
                                    id: brightnessSlider
                                    from: 0
                                    to: 100
                                    Layout.fillWidth: true
                                    onValueChanged: {
                                        if (pressed) {
                                            MonitorManager.setWMIBrightness(Math.round(value))
                                            MonitorManager.setDDCCIBrightness(Math.round(value), UserSettings.ddcciQueueDelay)
                                        }
                                    }

                                    ToolTip {
                                        parent: brightnessSlider.handle
                                        visible: brightnessSlider.pressed || brightnessSlider.hovered
                                        delay: brightnessSlider.pressed ? 0 : 1000
                                        text: Math.round(brightnessSlider.value).toString()
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        color: Constants.footerColor
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        bottomLeftRadius: 12
                        bottomRightRadius: 12
                        Layout.preferredHeight: 50
                        Layout.leftMargin: -14
                        Layout.rightMargin: -14
                        Layout.bottomMargin: -14

                        Rectangle {
                            height: 1
                            color: Constants.footerBorderColor
                            opacity: 0.15
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.left: parent.left
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 10

                            Image {
                                id: compactMedia
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                Layout.alignment: Qt.AlignVCenter
                                source: SoundPanelBridge.mediaArt || ""
                                fillMode: Image.PreserveAspectCrop
                                visible: SoundPanelBridge.mediaArt !== "" && UserSettings.mediaMode === 1
                            }

                            Item {
                                Layout.preferredWidth: updateLyt.implicitWidth
                                Layout.preferredHeight: Math.max(updateIcon.height, updateLabel.height)
                                Layout.leftMargin: 10
                                visible: !compactMedia.visible && Updater.updateAvailable

                                RowLayout {
                                    id: updateLyt
                                    anchors.fill: parent
                                    spacing: 10

                                    IconImage {
                                        id: updateIcon
                                        source: "qrc:/icons/update.svg"
                                        color: "#edb11a"
                                        sourceSize.width: 16
                                        sourceSize.height: 16
                                    }
                                    Label {
                                        id: updateLabel
                                        text: qsTr("Update available")
                                        color: "#edb11a"
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onEntered: {
                                        updateLabel.font.underline = true
                                    }
                                    onExited: {
                                        updateLabel.font.underline = false
                                    }
                                    onClicked: {
                                        settingsWindow.showUpdatePane()
                                    }
                                }
                            }

                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Item {
                                    Layout.fillHeight: true
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: SoundPanelBridge.mediaTitle || ""
                                    font.pixelSize: 14
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                    visible: UserSettings.mediaMode === 1
                                }

                                Label {
                                    text: SoundPanelBridge.mediaArtist || ""
                                    font.pixelSize: 12
                                    opacity: 0.7
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                    visible: UserSettings.mediaMode === 1
                                }

                                Item {
                                    Layout.fillHeight: true
                                }
                            }

                            NFToolButton {
                                icon.source: "qrc:/icons/settings.svg"
                                icon.width: 14
                                icon.height: 14
                                antialiasing: true
                                onClicked: {
                                    settingsWindow.show()
                                    panel.hidePanel()
                                }
                            }

                            NFToolButton {
                                visible: UserSettings.enablePowerMenu
                                icon.source: "qrc:/icons/shutdown.svg"
                                icon.width: 16
                                icon.height: 16
                                antialiasing: true
                                onClicked: {
                                    powerMenu.visible ? powerMenu.close() : powerMenu.open()
                                }

                                PowerMenu {
                                    id: powerMenu
                                    y: parent.y - powerMenu.height
                                    x: parent.x
                                    onSetPowerAction: function(action) {
                                        powerConfirmationWindow.setAction(action)
                                        powerConfirmationWindow.show()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item {
                id: rightSpacer
                Layout.row: 1
                Layout.column: 2
                Layout.fillHeight: true
                Layout.preferredWidth: {
                    if (panel.taskbarPos === "top" || panel.taskbarPos === "bottom" || panel.taskbarPos === "right") {
                        return UserSettings.xAxisMargin
                    } else {
                        return 0
                    }
                }
                visible: panel.taskbarPos === "top" || panel.taskbarPos === "bottom" || panel.taskbarPos === "right"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        panel.hidePanel()
                    }
                }
            }

            Item {
                id: bottomSpacer
                Layout.row: 2
                Layout.column: 0
                Layout.columnSpan: 3
                Layout.fillWidth: true
                Layout.preferredHeight: {
                    if (panel.taskbarPos === "bottom" || panel.taskbarPos === "left" || panel.taskbarPos === "right") {
                        return UserSettings.yAxisMargin
                    } else {
                        return 0
                    }
                }
                visible: panel.taskbarPos === "bottom" || panel.taskbarPos === "left" || panel.taskbarPos === "right"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        panel.hidePanel()
                    }
                }
            }
        }
    }

    Menu {
        id: executableRenameContextMenu

        property string originalName: ""
        property string currentCustomName: ""

        MenuItem {
            text: qsTr("Rename Application")
            onTriggered: executableRenameDialog.open()
        }

        MenuItem {
            text: qsTr("Reset to Original Name")
            enabled: executableRenameContextMenu.currentCustomName !== executableRenameContextMenu.originalName
            onTriggered: {
                AudioBridge.setCustomExecutableName(executableRenameContextMenu.originalName, "")
            }
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("Mute in Background")
            checkable: true
            checked: AudioBridge.isApplicationMutedInBackground(executableRenameContextMenu.originalName)
            onTriggered: {
                AudioBridge.setApplicationMutedInBackground(executableRenameContextMenu.originalName, checked)
            }
        }
    }

    Dialog {
        id: executableRenameDialog
        title: qsTr("Rename Application")
        modal: true
        width: 300
        dim: false
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            Label {
                text: qsTr("Original name: ") + executableRenameContextMenu.originalName
                font.bold: true
            }

            Label {
                text: qsTr("Custom name:")
            }

            TextField {
                id: executableCustomNameField
                Layout.fillWidth: true
                placeholderText: executableRenameContextMenu.originalName

                Keys.onReturnPressed: {
                    AudioBridge.setCustomExecutableName(
                        executableRenameContextMenu.originalName,
                        executableCustomNameField.text.trim()
                        )
                    executableRenameDialog.close()
                }
            }

            RowLayout {
                spacing: 15
                Layout.topMargin: 10

                Button {
                    text: qsTr("Cancel")
                    onClicked: executableRenameDialog.close()
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Save")
                    highlighted: true
                    Layout.fillWidth: true
                    onClicked: {
                        AudioBridge.setCustomExecutableName(
                            executableRenameContextMenu.originalName,
                            executableCustomNameField.text.trim()
                            )
                        executableRenameDialog.close()
                    }
                }
            }
        }

        onOpened: {
            executableCustomNameField.text = executableRenameContextMenu.currentCustomName
            executableCustomNameField.forceActiveFocus()
            executableCustomNameField.selectAll()
        }
    }
}
