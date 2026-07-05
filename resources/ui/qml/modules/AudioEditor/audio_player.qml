import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: audioPlayer
    height: 48
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property bool playing: false
    property real position: 0
    property real duration: 0
    property real volume: 0.8

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10

        Button {
            id: playBtn
            width: 36
            height: 36
            text: playing ? "⏸" : "▶"
            onClicked: playing = !playing
        }

        Button {
            width: 32
            height: 32
            text: "⏹"
            onClicked: {
                playing = false;
                position = 0;
            }
        }

        Slider {
            id: positionSlider
            Layout.fillWidth: true
            from: 0
            to: duration || 100
            value: position
            onValueChanged: position = value
        }

        Text {
            text: formatTime(position) + " / " + formatTime(duration)
            color: "#888888"
            font.pixelSize: 11
        }

        Slider {
            id: volumeSlider
            width: 80
            from: 0
            to: 1
            value: volume
            onValueChanged: volume = value
        }

        Text {
            text: Math.round(volume * 100) + "%"
            color: "#888888"
            font.pixelSize: 11
            width: 40
        }
    }

    function formatTime(seconds) {
        var mins = Math.floor(seconds / 60);
        var secs = Math.floor(seconds % 60);
        return mins.toString().padStart(2, '0') + ":" + secs.toString().padStart(2, '0');
    }
}