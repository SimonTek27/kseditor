import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real momentaryLUFS: 0
    property real shortTermLUFS: 0
    property real integratedLUFS: 0
    property real truePeak: 0
    property real lra: 0
    property real leftPeak: AudioBridge ? AudioBridge.leftPeak : 0
    property real rightPeak: AudioBridge ? AudioBridge.rightPeak : 0

    property bool isMeasuring: false
    property var lufsHistory: []

    Timer {
        interval: 100
        running: isMeasuring
        repeat: true
        onTriggered: {
            if (!AudioBridge) return;
            var l = Math.max(AudioBridge.leftPeak, 0.001);
            var r = Math.max(AudioBridge.rightPeak, 0.001);
            var avg = (l + r) / 2;

            var lufs = 20 * Math.log(Math.min(Math.max(avg, 0.0001), 1)) / Math.LN10;
            momentaryLUFS = Math.max(-70, Math.min(0, lufs + 3));
            shortTermLUFS = shortTermLUFS * 0.9 + momentaryLUFS * 0.1;
            truePeak = Math.max(leftPeak, rightPeak);

            lufsHistory.push(momentaryLUFS);
            if (lufsHistory.length > 400) lufsHistory.shift();
            var sum = lufsHistory.reduce(function(a,b) { return a+b; }, 0);
            integratedLUFS = lufsHistory.length > 0 ? sum / lufsHistory.length : -70;

            var sorted = lufsHistory.slice().sort(function(a,b) { return a-b; });
            var len = sorted.length;
            if (len > 10) {
                var low = sorted[Math.floor(len * 0.1)];
                var high = sorted[Math.floor(len * 0.9)];
                lra = Math.max(0, high - low);
            }
            canvas.requestPaint();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Text {
            text: "LOUDNESS METER"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0e0e0e"
            radius: 4
            clip: true

            Canvas {
                id: canvas
                anchors.fill: parent
                property real targetLUFS: -14

                function dbToY(db, h) {
                    var minDb = -50, maxDb = 0;
                    var normalized = (db - minDb) / (maxDb - minDb);
                    return h - Math.max(0, Math.min(h, normalized * h));
                }

                onPaint: {
                    var ctx = getContext("2d");
                    var w = ctx.canvas.width, h = ctx.canvas.height;
                    ctx.fillStyle = "#0e0e0e";
                    ctx.fillRect(0, 0, w, h);

                    var scale = 4;
                    var colW = (w - 80) / 3;

                    ctx.font = "9px monospace";
                    for (var db = -50; db <= 0; db += 5) {
                        var y = dbToY(db, h);
                        ctx.strokeStyle = (db % 10 === 0) ? "#333" : "#222";
                        ctx.lineWidth = 1;
                        ctx.beginPath();
                        ctx.moveTo(0, y);
                        ctx.lineTo(w, y);
                        ctx.stroke();

                        ctx.fillStyle = "#555";
                        ctx.textAlign = "left";
                        ctx.fillText(db + "dB", 4, y - 2);
                    }

                    var labels = ["Momentary", "Short Term", "Integrated"];
                    var vals = [momentaryLUFS, shortTermLUFS, integratedLUFS];
                    var colors = ["#E10600", "#ff8800", "#00cc66"];

                    for (var i = 0; i < 3; i++) {
                        var x = 80 + i * colW;
                        var bw = colW - 16;
                        var v = Math.max(-50, Math.min(0, vals[i]));
                        var by = dbToY(v, h);
                        var bh = h - by;

                        ctx.fillStyle = Qt.rgba(0.1, 0.1, 0.1, 0.5);
                        ctx.fillRect(x, 0, bw, h);

                        ctx.fillStyle = v > -9 ? "#ff4444" : v > -18 ? colors[i] : "#446644";
                        ctx.fillRect(x, by, bw, bh);

                        ctx.fillStyle = "#888";
                        ctx.textAlign = "center";
                        ctx.fillText(labels[i], x + bw/2, h - 4);

                        ctx.fillStyle = "#fff";
                        ctx.font = "bold 11px monospace";
                        ctx.fillText(v.toFixed(1) + " LUFS", x + bw/2, by - 6);

                        var targetY = dbToY(targetLUFS, h);
                        ctx.strokeStyle = "#ffff0044";
                        ctx.lineWidth = 1;
                        ctx.setLineDash([4, 4]);
                        ctx.beginPath();
                        ctx.moveTo(x, targetY);
                        ctx.lineTo(x + bw, targetY);
                        ctx.stroke();
                        ctx.setLineDash([]);
                    }

                    ctx.fillStyle = "#ffff00";
                    ctx.font = "8px sans-serif";
                    ctx.fillText("Target", w - 40, dbToY(targetLUFS, h) - 2);
                }
            }
        }

        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 16

                ColumnLayout {
                    Text { text: "LRA"; color: "#888"; font.pixelSize: 9 }
                    Text { text: lra.toFixed(1) + " LU"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                }
                ColumnLayout {
                    Text { text: "True Peak"; color: "#888"; font.pixelSize: 9 }
                    Text {
                        text: {
                            var db = 20 * Math.log(Math.max(truePeak, 0.0001)) / Math.LN10;
                            return db.toFixed(1) + " dBTP";
                        }
                        color: "#E10600"; font.pixelSize: 14; font.bold: true
                    }
                }
                ColumnLayout {
                    Text { text: "Range"; color: "#888"; font.pixelSize: 9 }
                    Text { text: (Math.max(-70, momentaryLUFS) - Math.max(-70, integratedLUFS)).toFixed(1) + " dB"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                }

                Item { Layout.fillWidth: true }

                KsButton {
                    height: 28
                    text: isMeasuring ? "Stop" : "Measure"
                    bgcolor: isMeasuring ? "#E10600" : "#3e3e42"
                    color: isMeasuring ? "#121212" : "#ffffff"
                    onClicked: {
                        isMeasuring = !isMeasuring;
                        if (isMeasuring) {
                            lufsHistory = [];
                            integratedLUFS = 0;
                            lra = 0;
                        }
                    }
                }
                KsButton {
                    height: 28
                    text: "Reset"
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: {
                        lufsHistory = [];
                        momentaryLUFS = 0;
                        shortTermLUFS = 0;
                        integratedLUFS = 0;
                        lra = 0;
                    }
                }
            }
        }
    }
}


