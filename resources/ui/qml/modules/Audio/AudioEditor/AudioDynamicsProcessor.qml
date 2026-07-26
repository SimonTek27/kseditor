import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real threshold: -20
    property real ratio: 4
    property real attack: 10
    property real release: 100
    property real knee: 6
    property real makeupGain: 3
    property real inputLevel: AudioBridge ? Math.max(AudioBridge.leftPeak, AudioBridge.rightPeak) : 0
    property bool sidechainEnabled: false
    property string sidechainSource: "Internal"

    function dbToLinear(db) { return Math.pow(10, db / 20); }
    function linearToDb(lin) { return lin > 0 ? 20 * Math.log(lin) / Math.LN10 : -120; }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0e0e0e"
                radius: 4
                clip: true

                Canvas {
                    id: graphCanvas
                    anchors.fill: parent
                    property real inputDb: -60
                    property real outputDb: -60
                    property real gainReduction: 0

                    onPaint: {
                        var ctx = getContext("2d");
                        var w = ctx.canvas.width, h = ctx.canvas.height;
                        var m = 20;
                        var gw = w - 2*m, gh = h - 2*m;

                        ctx.fillStyle = "#0e0e0e";
                        ctx.fillRect(0, 0, w, h);

                        ctx.translate(m, m);

                        ctx.strokeStyle = "#333";
                        ctx.lineWidth = 1;
                        ctx.strokeRect(0, 0, gw, gh);

                        for (var i = 0; i <= 6; i++) {
                            var x = (gw / 6) * i;
                            ctx.strokeStyle = "#222";
                            ctx.beginPath();
                            ctx.moveTo(x, 0);
                            ctx.lineTo(x, gh);
                            ctx.stroke();

                            var y = (gh / 6) * i;
                            ctx.beginPath();
                            ctx.moveTo(0, y);
                            ctx.lineTo(gw, y);
                            ctx.stroke();
                        }

                        ctx.fillStyle = "#555";
                        ctx.font = "8px monospace";
                        for (i = 0; i <= 6; i++) {
                            ctx.fillText((-60 + i*10) + "dB", -2, gh - (gh/6)*i + 3);
                            ctx.textAlign = "right";
                        }
                        ctx.textAlign = "center";
                        for (i = 0; i <= 6; i++) {
                            ctx.fillText((-60 + i*10) + "dB", (gw/6)*i, gh + 12);
                        }

                        ctx.strokeStyle = "#555";
                        ctx.lineWidth = 1;
                        ctx.beginPath();
                        ctx.moveTo(0, 0);
                        ctx.lineTo(gw, gh);
                        ctx.stroke();

                        var thrX = (threshold + 60) / 60 * gw;
                        var thrY = gh - (threshold + 60) / 60 * gh;
                        var ratioSlope = 1 / ratio;
                        var kneeStart = threshold - knee/2;
                        var kneeEnd = threshold + knee/2;

                        ctx.strokeStyle = "#00ff88";
                        ctx.lineWidth = 2;
                        ctx.beginPath();
                        var prevY = gh;
                        for (var sx = 0; sx <= gw; sx += 2) {
                            var inDb = (sx / gw) * 60 - 60;
                            var outDb;
                            if (inDb < kneeStart) {
                                outDb = inDb;
                            } else if (inDb < kneeEnd) {
                                var kPos = (inDb - kneeStart) / knee;
                                var linearIn = dbToLinear(inDb);
                                var linearThr = dbToLinear(threshold);
                                var linearKneeStart = dbToLinear(kneeStart);
                                var attenuation = (1 - Math.cos(Math.PI * kPos)) / 2;
                                var compressedDb = threshold + (inDb - threshold) * ratioSlope;
                                outDb = inDb * (1 - attenuation) + compressedDb * attenuation;
                            } else {
                                outDb = threshold + (inDb - threshold) * ratioSlope;
                            }
                            if (outDb > 0) outDb = 0;
                            var sy = gh - (outDb + 60) / 60 * gh;
                            if (sx === 0) ctx.moveTo(sx, sy);
                            else ctx.lineTo(sx, sy);
                        }
                        ctx.stroke();

                        ctx.setLineDash([3, 3]);
                        ctx.strokeStyle = "#ffff0044";
                        ctx.lineWidth = 1;
                        ctx.beginPath();
                        ctx.moveTo(thrX, 0);
                        ctx.lineTo(thrX, gh);
                        ctx.stroke();
                        ctx.setLineDash([]);

                        ctx.fillStyle = "#ff444488";
                        ctx.font = "9px sans-serif";
                        ctx.fillText("Threshold: " + threshold.toFixed(1) + "dB", thrX + 4, 14);
                        ctx.fillStyle = "#00ff8888";
                        ctx.fillText("Ratio: " + ratio.toFixed(1) + ":1", 4, 14);

                        var peakDb = linearToDb(inputLevel);
                        var inX = (peakDb + 60) / 60 * gw;
                        var compressedDb = threshold + Math.max(0, peakDb - threshold) * ratioSlope;
                        var outY = gh - (compressedDb + 60) / 60 * gh;

                        ctx.strokeStyle = "#E10600";
                        ctx.lineWidth = 2;
                        ctx.beginPath();
                        ctx.moveTo(inX, 0);
                        ctx.lineTo(inX, gh);
                        ctx.stroke();

                        ctx.fillStyle = "#E10600";
                        ctx.beginPath();
                        ctx.arc(inX, outY, 5, 0, Math.PI * 2);
                        ctx.fill();

                        gainReduction = Math.max(0, peakDb - compressedDb);

                        ctx.fillStyle = "#ff8800";
                        ctx.font = "bold 10px monospace";
                        ctx.fillText("GR: " + gainReduction.toFixed(1) + "dB", 4, gh - 6);
                    }

                    Timer {
                        interval: 60
                        running: true
                        repeat: true
                        onTriggered: graphCanvas.requestPaint()
                    }
                }
            }

            Rectangle {
                width: 200
                Layout.fillHeight: true
                color: "#1e1e1e"
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    Text { text: "COMPRESSOR"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Threshold"; color: "#aaa"; font.pixelSize: 9 }
                        Slider {
                            from: -60; to: 0; value: threshold
                            Layout.fillWidth: true
                            onMoved: threshold = value
                        }
                        Text { text: threshold.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Ratio"; color: "#aaa"; font.pixelSize: 9 }
                        Slider {
                            from: 1; to: 20; value: ratio; stepSize: 0.5
                            Layout.fillWidth: true
                            onMoved: ratio = value
                        }
                        Text { text: ratio.toFixed(1) + ":1"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Attack"; color: "#aaa"; font.pixelSize: 9 }
                        Slider {
                            from: 0.1; to: 100; value: attack
                            Layout.fillWidth: true
                            onMoved: attack = value
                        }
                        Text { text: attack.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Release"; color: "#aaa"; font.pixelSize: 9 }
                        Slider {
                            from: 10; to: 1000; value: release
                            Layout.fillWidth: true
                            onMoved: release = value
                        }
                        Text { text: release.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Knee"; color: "#aaa"; font.pixelSize: 9 }
                        Slider {
                            from: 0; to: 12; value: knee
                            Layout.fillWidth: true
                            onMoved: knee = value
                        }
                        Text { text: knee.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Makeup"; color: "#aaa"; font.pixelSize: 9 }
                        Slider {
                            from: 0; to: 24; value: makeupGain
                            Layout.fillWidth: true
                            onMoved: makeupGain = value
                        }
                        Text { text: "+" + makeupGain.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        CheckBox {
                            checked: sidechainEnabled
                            onCheckedChanged: sidechainEnabled = checked
                        }
                        Text { text: "Sidechain"; color: "#aaa"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        visible: sidechainEnabled
                        Text { text: "Source"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Internal", "External 1", "External 2", "Sidechain Bus"]
                            currentIndex: 0
                        }
                    }

                    Item { Layout.fillHeight: true }

                    Rectangle {
                        height: 40
                        color: "#0e0e0e"
                        Layout.fillWidth: true
                        radius: 4

                        ColumnLayout {
                            anchors.centerIn: parent
                            Text {
                                text: "Gain Reduction"
                                color: "#888"; font.pixelSize: 8
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Text {
                                text: graphCanvas.gainReduction.toFixed(1) + " dB"
                                color: graphCanvas.gainReduction > 6 ? "#ff4444" : "#ffaa00"
                                font.pixelSize: 18; font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }

                    AppButton {
                        height: 32
                        text: "Apply to Audio"
                        bgcolor: "#E10600"
                        color: "#121212"
                        Layout.fillWidth: true
                        onClicked: {
                            if (AudioBridge) {
                                AudioBridge.applyCompressor(threshold, ratio, attack, release, dbToLinear(makeupGain));
                            }
                        }
                    }
                }
            }
        }
    }
}


