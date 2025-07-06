import QtQuick
import QtMultimedia
import Odizinne.QuickSoundSwitcher

Item {
    id: root

    Connections {
        target: UserSettings
        function onKeepAliveChanged() {
            if (UserSettings.keepAlive) {
                root.startKeepAlive()
            } else {
                root.stopKeepAlive()
            }
        }
    }

    Component.onCompleted: {
        updateKeepAliveAudioDevice()
        if (UserSettings.keepAlive) {
            startKeepAlive()
        }
    }

    function updateKeepAliveAudioDevice() {
        const device = mediaDevices.defaultAudioOutput
        keepAliveAudioOutput.device = device
        if (keepAlivePlayer.playbackState === MediaPlayer.PlayingState) {
            keepAlivePlayer.stop()
            keepAlivePlayer.play()
        }
    }

    function startKeepAlive() {
        if (keepAlivePlayer.playbackState !== MediaPlayer.PlayingState) {
            updateKeepAliveAudioDevice()
            keepAlivePlayer.play()
        }
    }

    function stopKeepAlive() {
        if (keepAlivePlayer.playbackState === MediaPlayer.PlayingState) {
            keepAlivePlayer.stop()
        }
    }

    MediaDevices {
        id: mediaDevices
        onAudioOutputsChanged: {
            root.updateKeepAliveAudioDevice()
        }
    }

    MediaPlayer {
        id: keepAlivePlayer
        source: "qrc:/sounds/empty.wav"
        audioOutput: AudioOutput {
            id: keepAliveAudioOutput
            volume: 0.1
            device: mediaDevices.defaultAudioOutput
        }
        loops: MediaPlayer.Infinite
    }
}
