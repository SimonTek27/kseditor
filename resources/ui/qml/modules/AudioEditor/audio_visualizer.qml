import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: audioVisualizer
    width: 400
    height: 200
    color: "#121212"
    border.color: "#333333"
    border.width: 1

    property string mode: "spectrum"
    property var spectrumData: []
    property var waveformData: []

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        color: "transparent"

        Canvas {
            id: visualizerCanvas
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                ctx.fillStyle = "#121212";
                ctx.fillRect(0, 0, width, height);

                if (mode === "spectrum") {
                    drawSpectrum(ctx);
                } else if (mode === "waveform") {
                    drawWaveform(ctx);
                }
            }
        }
    }

    RowLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 5

        Button {
            text: "Spectrum"
            width: 70
            height: 24
            onClicked: mode = "spectrum"
        }

        Button {
            text: "Waveform"
            width: 70
            height: 24
            onClicked: mode = "waveform"
        }
    }

    function drawSpectrum(ctx) {
        var barCount = spectrumData.length || 64;
        var barWidth = ctx.canvas.width / barCount;
        var gradient = ctx.createLinearGradient(0, ctx.canvas.height, 0, 0);
        gradient.addColorStop(0, "#00ff00");
        gradient.addColorStop(0.5, "#ffff00");
        gradient.addColorStop(1, "#ff0000");
        ctx.fillStyle = gradient;

        for (var i = 0; i < barCount; i++) {
            var value = spectrumData[i] || Math.random() * 0.8;
            var barHeight = value * ctx.canvas.height;
            ctx.fillRect(i * barWidth, ctx.canvas.height - barHeight, barWidth - 1, barHeight);
        }
    }

    function drawWaveform(ctx) {
        ctx.strokeStyle = "#00aa00";
        ctx.lineWidth = 1;
        ctx.beginPath();

        var data = waveformData.length ? waveformData : generateDummyWaveform();
        var step = ctx.canvas.width / data.length;

        for (var i = 0; i < data.length; i++) {
            var y = (1 - data[i]) * ctx.canvas.height / 2 + ctx.canvas.height / 2;
            if (i === 0) ctx.moveTo(0, y);
            else ctx.lineTo(i * step, y);
        }
        ctx.stroke();
    }

    function generateDummyWaveform() {
        var arr = [];
        for (var i = 0; i < 200; i++) {
            arr.push(0.5 + Math.sin(i * 0.1) * 0.3 + Math.random() * 0.1);
        }
        return arr;
    }

    Timer {
        interval: 50
        running: true
        onTriggered: visualizerCanvas.requestPaint()
    }
}