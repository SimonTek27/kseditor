import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real threshold: -40
    property real floor: -80
    property real attack: 5
    property real release: 50
    property real lowFreq: 100
    property real highFreq: 8000
    property real reduction: 20
    property real sensitivity: 50
    property real hysteresis: 3
    property int fftSize: 2048
    property bool spectralMode: true
    property bool noiseFloor: false
    property bool listenNoise: false
    property bool adaptive: true
    property string filterSlope: "Gentle"
    property real inputLevel: AudioBridge ? Math.max(AudioBridge.leftPeak, AudioBridge.rightPeak) : 0

    function dbToLinear(db) { return Math.pow(10, db / 20); }
    function linearToDb(lin) { return lin > 0 ? 20 * Math.log(lin) / Math.LN10 : -100; }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0e0e0e"
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    Text { text: "SPECTRAL GATE"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: specGateCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var logMin = Math.log(20);
                                var logMax = Math.log(20000);
                                var logRng = logMax - logMin;
                                function f2x(f) { return (Math.log(f) - logMin) / logRng * (w - 10) + 5; }

                                var xLow = f2x(lowFreq);
                                var xHigh = f2x(highFreq);

                                ctx.fillStyle = "#E1060010";
                                ctx.fillRect(xLow, 5, xHigh - xLow, h - 10);

                                ctx.strokeStyle = "#2a2a2a";
                                ctx.lineWidth = 1;
                                for (var f = 50; f <= 16000; f *= 2) {
                                    var fx = f2x(f);
                                    ctx.beginPath();
                                    ctx.moveTo(fx, 5);
                                    ctx.lineTo(fx, h - 5);
                                    ctx.stroke();
                                    ctx.fillStyle = "#444";
                                    ctx.font = "7px monospace";
                                    ctx.textAlign = "center";
                                    ctx.fillText((f >= 1000 ? (f/1000).toFixed(0) + "k" : f) + "", fx, h - 2);
                                }

                                var peakDb = linearToDb(inputLevel);
                                var thrDb = threshold;
                                var sensNorm = sensitivity / 100;

                                var slopeNames = ["Gentle", "Medium", "Brickwall"];
                                var slopeIdx = slopeNames.indexOf(filterSlope);
                                if (slopeIdx < 0) slopeIdx = 0;
                                var slopes = [0.5, 2, 8];

                                ctx.strokeStyle = "#555";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                for (var x = 5; x <= w - 5; x += 3) {
                                    var freq = Math.exp(logMin + ((x - 5) / (w - 10)) * logRng);
                                    var gateDb = 0;

                                    if (freq >= lowFreq && freq <= highFreq && spectralMode) {
                                        var aboveThr = peakDb - thrDb;
                                        gateDb = aboveThr > 0
                                            ? 0
                                            : Math.max(-reduction, aboveThr * slopes[slopeIdx]);
                                    }

                                    var gateLinear = dbToLinear(gateDb);
                                    var y = h - 5 - gateLinear * (h - 10);
                                    ctx.lineTo(x, Math.max(5, y));
                                }
                                ctx.stroke();

                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 2;
                                ctx.beginPath();
                                ctx.moveTo(xLow, 5);
                                ctx.lineTo(xLow, h - 5);
                                ctx.stroke();
                                ctx.beginPath();
                                ctx.moveTo(xHigh, 5);
                                ctx.lineTo(xHigh, h - 5);
                                ctx.stroke();

                                ctx.setLineDash([3, 3]);
                                var thrY = (1 - (thrDb + 100) / 120) * (h - 10) + 5;
                                ctx.strokeStyle = "#ffff0044";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(5, thrY);
                                ctx.lineTo(w - 5, thrY);
                                ctx.stroke();
                                ctx.setLineDash([]);

                                for (var bin = 0; bin < 40; bin++) {
                                    var bFreq = 50 + bin * 200;
                                    if (bFreq > 16000) break;
                                    var bx = f2x(bFreq);
                                    var energy = (Math.sin(bin * 0.5 + peakDb * 0.05) * 0.5 + 0.5) * (1 + peakDb / 40);
                                    var noiseFloorVal = noiseFloor ? (Math.sin(bin * 1.3) * 0.15 + 0.2) : 0;
                                    energy = Math.max(noiseFloorVal, energy * 0.3);

                                    var barH = Math.max(2, energy * (h - 20) * 0.6);
                                    var isGated = energy * 60 < -thrDb / 60;
                                    ctx.fillStyle = isGated ? "#E1060044" : "#00ff8844";
                                    ctx.fillRect(bx, h - 6 - barH, 3, barH);
                                }

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Band: " + lowFreq.toFixed(0) + "-" + highFreq.toFixed(0) + "Hz", 4, 12);
                                ctx.fillText("Slope: " + filterSlope, 4, 24);
                                ctx.fillStyle = "#ff8800";
                                ctx.textAlign = "right";
                                ctx.fillText("Reduction: " + reduction.toFixed(0) + "dB", w - 4, 12);
                            }

                            Timer {
                                interval: 80
                                running: true
                                repeat: true
                                onTriggered: specGateCanvas.requestPaint()
                            }
                        }
                    }
                }
            }

            Rectangle {
                width: 220
                Layout.fillHeight: true
                color: "#1e1e1e"
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    Text { text: "SPECTRAL GATE"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Threshold"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -80; to: 0; value: threshold; Layout.fillWidth: true; onMoved: threshold = value }
                        Text { text: threshold.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Floor"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -96; to: -20; value: floor; Layout.fillWidth: true; onMoved: floor = value }
                        Text { text: floor.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Reduction"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 48; value: reduction; Layout.fillWidth: true; onMoved: reduction = value }
                        Text { text: reduction.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Low Freq"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 20; to: 2000; value: lowFreq; Layout.fillWidth: true; onMoved: lowFreq = value }
                        Text { text: lowFreq.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "High Freq"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 500; to: 16000; value: highFreq; Layout.fillWidth: true; onMoved: highFreq = value }
                        Text { text: highFreq.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Attack"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.1; to: 50; value: attack; Layout.fillWidth: true; onMoved: attack = value }
                        Text { text: attack.toFixed(1) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Release"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 5; to: 500; value: release; Layout.fillWidth: true; onMoved: release = value }
                        Text { text: release.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Sensitivity"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: sensitivity; Layout.fillWidth: true; onMoved: sensitivity = value }
                        Text { text: sensitivity.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Hysteresis"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 12; value: hysteresis; Layout.fillWidth: true; onMoved: hysteresis = value }
                        Text { text: hysteresis.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Slope"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Gentle", "Medium", "Brickwall"]
                            currentIndex: 0
                            onActivated: filterSlope = currentText
                        }
                    }

                    CheckBox {
                        checked: spectralMode
                        onCheckedChanged: spectralMode = checked
                        Text {
                            text: "Spectral Mode"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: noiseFloor
                        onCheckedChanged: noiseFloor = checked
                        Text {
                            text: "Noise Floor"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: listenNoise
                        onCheckedChanged: listenNoise = checked
                        Text {
                            text: "Listen Noise"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: adaptive
                        onCheckedChanged: adaptive = checked
                        Text {
                            text: "Adaptive"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Item { Layout.fillHeight: true }

                    AppButton {
                        height: 32
                        text: "Apply to Audio"
                        bgcolor: "#E10600"
                        color: "#121212"
                        Layout.fillWidth: true
                        onClicked: {
                            if (AudioBridge) {
                                AudioBridge.applyCompressor(threshold, 10, attack, release, dbToLinear(-reduction));
                            }
                        }
                    }
                }
            }
        }
    }
}


