import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import ksEditor.Audio 1.0

Rectangle {
    id: audioPlayer
    height: 48
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property bool playing: AudioBridge ? AudioBridge.isPlaying : false
    property real position: AudioBridge ? AudioBridge.getPositionMs() / 1000 : 0
    property real duration: AudioBridge ? AudioBridge.getDurationMs() / 1000 : 0
    property real volume: 0.8

    onPlayingChanged: { if (!playing && AudioBridge) AudioBridge.stop() }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10

        Button {
            id: playBtn
            width: 36; height: 36
            text: playing ? "\u23F8" : "\u25B6"
            onClicked: {
                if (AudioBridge) {
                    if (playing) AudioBridge.pause();
                    else AudioBridge.play();
                }
            }
        }

        Button {
            width: 32; height: 32; text: "\u23F9"
            onClicked: {
                if (AudioBridge) AudioBridge.stop()
            }
        }

        Slider {
            id: positionSlider
            Layout.fillWidth: true
            from: 0; to: duration || 100
            value: position
            onPressedChanged: {
                if (!pressed && AudioBridge)
                    AudioBridge.setPositionMs(value * 1000)
            }
        }

        Text {
            text: formatTime(position) + " / " + formatTime(duration)
            color: "#888888"; font.pixelSize: 11
        }

        Slider {
            id: volumeSlider
            width: 80; from: 0; to: 1; value: volume
            onValueChanged: volume = value
        }

        Text {
            text: Math.round(volume * 100) + "%"
            color: "#888888"; font.pixelSize: 11; width: 40
        }
    }

    function formatTime(seconds) {
        var mins = Math.floor(seconds / 60);
        var secs = Math.floor(seconds % 60);
        return mins.toString().padStart(2, '0') + ":" + secs.toString().padStart(2, '0');
    }
}
