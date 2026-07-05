import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"
    property string activeView: "spectrum"
    property var spectrumData: AudioBridge ? AudioBridge.getSpectrumData() : []
    property var melData: AudioBridge ? AudioBridge.getMelSpectrum(64) : []
    property real leftPeak: AudioBridge ? AudioBridge.leftPeak : 0
    property real rightPeak: AudioBridge ? AudioBridge.rightPeak : 0
    property real leftRMS: AudioBridge ? AudioBridge.leftRMS : 0
    property real rightRMS: AudioBridge ? AudioBridge.rightRMS : 0
    property real peakHoldL: 0
    property real peakHoldR: 0

    onLeftPeakChanged: { if (leftPeak > peakHoldL) peakHoldL = leftPeak; }
    onRightPeakChanged: { if (rightPeak > peakHoldR) peakHoldR = rightPeak; }

    Timer {
        interval: 50
        running: true
        repeat: true
        onTriggered: {
            if (AudioBridge) {
                spectrumData = AudioBridge.getSpectrumData();
                melData = AudioBridge.getMelSpectrum(64);
                leftPeak = AudioBridge.leftPeak;
                rightPeak = AudioBridge.rightPeak;
                leftRMS = AudioBridge.leftRMS;
                rightRMS = AudioBridge.rightRMS;
            }
            barCanvas.requestPaint();
            if (activeView === "waterfall") waterfallCanvas.requestPaint();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Rectangle {
            height: 32
            color: "#1e1e1e"
            Layout.fillWidth: true
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4

                KsButton {
                    text: "FFT Spectrum"
                    flat: true
                    height: 26
                    bgcolor: activeView === "spectrum" ? "#E10600" : "#3e3e42"
                    color: activeView === "spectrum" ? "#121212" : "#ffffff"
                    onClicked: activeView = "spectrum"
                }
                KsButton {
                    text: "Spectrogram"
                    flat: true
                    height: 26
                    bgcolor: activeView === "waterfall" ? "#E10600" : "#3e3e42"
                    color: activeView === "waterfall" ? "#121212" : "#ffffff"
                    onClicked: activeView = "waterfall"
                }
                KsButton {
                    text: "Mel Scale"
                    flat: true
                    height: 26
                    bgcolor: activeView === "mel" ? "#E10600" : "#3e3e42"
                    color: activeView === "mel" ? "#121212" : "#ffffff"
                    onClicked: activeView = "mel"
                }
                KsButton {
                    text: "Phase Scope"
                    flat: true
                    height: 26
                    bgcolor: activeView === "phase" ? "#E10600" : "#3e3e42"
                    color: activeView === "phase" ? "#121212" : "#ffffff"
                    onClicked: activeView = "phase"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "Peak Hold"
                    color: "#888"
                    font.pixelSize: 10
                }
                Rectangle {
                    width: 12; height: 12; radius: 6
                    color: "#ff4444"
                }
            }
        }

        Item { Layout.fillWidth: true; Layout.fillHeight: true; clip: true

            Canvas {
                id: barCanvas
                anchors.fill: parent
                visible: activeView === "spectrum" || activeView === "mel"
                onPaint: {
                    var ctx = getContext("2d");
                    var w = ctx.canvas.width, h = ctx.canvas.height;
                    ctx.fillStyle = "#0e0e0e";
                    ctx.fillRect(0, 0, w, h);

                    var data = activeView === "mel" ? melData : spectrumData;
                    if (!data || data.length === 0) {
                        ctx.fillStyle = "#333";
                        ctx.font = "12px sans-serif";
                        ctx.textAlign = "center";
                        ctx.fillText("Load audio to view spectrum", w/2, h/2);
                        return;
                    }

                    var barCount = Math.min(data.length, w / 3);
                    var barWidth = w / barCount;

                    for (var i = 0; i < barCount; i++) {
                        var val = Math.min(Math.max(Number(data[i]) || 0, 0), 1);
                        var barHeight = Math.max(val * h, 1);

                        var r, g, b;
                        if (val < 0.5) {
                            r = 0; g = Math.round(255 * (val / 0.5));
                            b = Math.round(128 * (1 - val / 0.5));
                        } else if (val < 0.75) {
                            r = Math.round(255 * ((val - 0.5) / 0.25));
                            g = 255;
                            b = 0;
                        } else {
                            r = 255;
                            g = Math.round(255 * (1 - (val - 0.75) / 0.25));
                            b = 0;
                        }

                        ctx.fillStyle = Qt.rgba(r/255, g/255, b/255, 1);
                        ctx.fillRect(i * barWidth, h - barHeight, barWidth - 1, barHeight);

                        ctx.fillStyle = Qt.rgba(1, 1, 1, 0.3);
                        ctx.fillRect(i * barWidth, h - barHeight, barWidth - 1, 1);
                    }

                    ctx.fillStyle = Qt.rgba(1, 1, 1, 0.5);
                    ctx.font = "9px monospace";
                    for (i = 0; i < 5; i++) {
                        var y = (h / 5) * i;
                        ctx.fillText((-20 * i) + "dB", 4, y + 10);
                    }

                    var freqLabels = ["31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"];
                    for (i = 0; i < freqLabels.length; i++) {
                        var x = (w / freqLabels.length) * i;
                        ctx.fillText(freqLabels[i], x + 2, h - 2);
                    }
                }
            }

            Canvas {
                id: waterfallCanvas
                anchors.fill: parent
                visible: activeView === "waterfall"
                property var rows: []
                property int maxRows: 128

                function pushRow() {
                    var data = AudioBridge ? AudioBridge.getSpectrumData() : [];
                    rows.unshift(data.map(function(v) { return Number(v) || 0; }));
                    if (rows.length > maxRows) rows.pop();
                    requestPaint();
                }

                onPaint: {
                    var ctx = getContext("2d");
                    var w = ctx.canvas.width, h = ctx.canvas.height;
                    ctx.fillStyle = "#0e0e0e";
                    ctx.fillRect(0, 0, w, h);

                    if (rows.length === 0) {
                        ctx.fillStyle = "#333";
                        ctx.font = "12px sans-serif";
                        ctx.textAlign = "center";
                        ctx.fillText("Collecting spectrogram data...", w/2, h/2);
                        return;
                    }

                    var rowH = h / maxRows;
                    var barWidth = w / (rows[0] ? rows[0].length : 64);

                    for (var r = 0; r < rows.length; r++) {
                        var row = rows[r];
                        if (!row) continue;
                        for (var i = 0; i < row.length; i++) {
                            var val = Math.min(Math.max(row[i], 0), 1);
                            var intensity = Math.round(val * 255);
                            ctx.fillStyle = Qt.rgba(
                                intensity/255,
                                Math.max(0, Math.min(1, (intensity - 128) / 128)),
                                Math.max(0, (128 - intensity) / 128),
                                0.9
                            );
                            ctx.fillRect(i * barWidth, r * rowH, barWidth + 1, rowH + 1);
                        }
                    }
                }

                Timer {
                    interval: 80
                    running: parent.visible
                    repeat: true
                    onTriggered: waterfallCanvas.pushRow()
                }
            }

            Canvas {
                id: phaseCanvas
                anchors.fill: parent
                visible: activeView === "phase"
                property var history: []
                property int maxPoints: 256

                onPaint: {
                    var ctx = getContext("2d");
                    var w = ctx.canvas.width, h = ctx.canvas.height;
                    var cx = w / 2, cy = h / 2, r = Math.min(cx, cy) * 0.85;

                    ctx.fillStyle = "#0e0e0e";
                    ctx.fillRect(0, 0, w, h);

                    ctx.strokeStyle = "#333";
                    ctx.lineWidth = 1;
                    ctx.beginPath();
                    ctx.arc(cx, cy, r, 0, Math.PI * 2);
                    ctx.stroke();
                    ctx.beginPath();
                    ctx.moveTo(cx - r, cy);
                    ctx.lineTo(cx + r, cy);
                    ctx.moveTo(cx, cy - r);
                    ctx.lineTo(cx, cy + r);
                    ctx.stroke();

                    var l = root.leftPeak || 0.01;
                    var rVal = root.rightPeak || 0.01;
                    var x = cx + (l - rVal) / (l + rVal + 0.001) * r * 0.8;
                    var y = cy - Math.min(l, rVal) / Math.max(l, rVal, 0.001) * r * 0.8;

                    history.push({x: x, y: y});
                    if (history.length > maxPoints) history.shift();

                    ctx.strokeStyle = "#00ff88";
                    ctx.lineWidth = 2;
                    ctx.beginPath();
                    for (var i = 0; i < history.length; i++) {
                        if (i === 0) ctx.moveTo(history[i].x, history[i].y);
                        else ctx.lineTo(history[i].x, history[i].y);
                    }
                    ctx.stroke();

                    ctx.fillStyle = "#ff4444";
                    ctx.beginPath();
                    ctx.arc(x, y, 4, 0, Math.PI * 2);
                    ctx.fill();

                    ctx.fillStyle = "#888";
                    ctx.font = "10px sans-serif";
                    ctx.fillText("In Phase", 8, 16);
                    ctx.fillText("Out of Phase", 8, h - 8);
                    ctx.textAlign = "right";
                    ctx.fillText("L", cx - 8, cy - 8);
                    ctx.fillText("R", cx + 8, cy + 14);
                }

                Timer {
                    interval: 40
                    running: parent.visible
                    repeat: true
                    onTriggered: phaseCanvas.requestPaint()
                }
            }
        }

        Rectangle {
            height: 48
            color: "#1e1e1e"
            Layout.fillWidth: true
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 16

                ColumnLayout {
                    Text { text: "L"; color: "#888"; font.pixelSize: 10 }
                    RowLayout {
                        Rectangle {
                            width: 120; height: 14; radius: 2
                            color: "#0e0e0e"; clip: true
                            Rectangle {
                                height: 14
                                color: leftPeak > 0.85 ? "#ff4444" : leftPeak > 0.7 ? "#ffaa00" : "#00cc66"
                                width: Math.min(parent.width * Math.max(leftPeak, leftRMS), parent.width)
                                Behavior on width { SmoothedAnimation { duration: 60 } }
                            }
                            Rectangle {
                                y: 0; width: 2; height: 14; color: "#ffffff"
                                x: Math.min(parent.width * peakHoldL, parent.width - 2)
                                Behavior on x { SmoothedAnimation { duration: 200 } }
                            }
                        }
                        Text { text: Math.round(leftPeak * 100) + "%"; color: "#aaa"; font.pixelSize: 9 }
                    }
                }

                ColumnLayout {
                    Text { text: "R"; color: "#888"; font.pixelSize: 10 }
                    RowLayout {
                        Rectangle {
                            width: 120; height: 14; radius: 2
                            color: "#0e0e0e"; clip: true
                            Rectangle {
                                height: 14
                                color: rightPeak > 0.85 ? "#ff4444" : rightPeak > 0.7 ? "#ffaa00" : "#00cc66"
                                width: Math.min(parent.width * Math.max(rightPeak, rightRMS), parent.width)
                                Behavior on width { SmoothedAnimation { duration: 60 } }
                            }
                            Rectangle {
                                y: 0; width: 2; height: 14; color: "#ffffff"
                                x: Math.min(parent.width * peakHoldR, parent.width - 2)
                                Behavior on x { SmoothedAnimation { duration: 200 } }
                            }
                        }
                        Text { text: Math.round(rightPeak * 100) + "%"; color: "#aaa"; font.pixelSize: 9 }
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: {
                        var p = Math.max(leftPeak, rightPeak);
                        if (p < 0.01) return "-inf dB";
                        return Math.round(20 * Math.log(p) / Math.LN10) + " dB";
                    }
                    color: "#E10600"
                    font.pixelSize: 14
                    font.bold: true
                }

                Text {
                    text: "Peak: " + Math.round(Math.max(peakHoldL, peakHoldR) * 100) + "%"
                    color: "#888"; font.pixelSize: 10
                    MouseArea {
                        anchors.fill: parent
                        onClicked: { peakHoldL = 0; peakHoldR = 0; }
                    }
                }
            }
        }
    }
}


