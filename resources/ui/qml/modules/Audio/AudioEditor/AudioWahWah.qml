import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property string filterType: "Bandpass"
    property real freqMin: 400
    property real freqMax: 4000
    property real resonance: 0.5
    property real sensitivity: 50
    property real attack: 20
    property real release: 50
    property real depth: 80
    property real mix: 70
    property bool autoMode: true
    property bool envelopeFollower: true
    property bool invertSweep: false
    property real lfoRate: 0.5
    property real inputLevel: AudioBridge ? Math.max(AudioBridge.leftPeak, AudioBridge.rightPeak) : 0
    property real currentFreq: freqMin

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

                    Text { text: "FILTER SWEEP"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: wahCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var fMin = freqMin;
                                var fMax = freqMax;
                                var res = resonance;

                                var logMin = Math.log(50);
                                var logMax = Math.log(16000);
                                var logRng = logMax - logMin;
                                function f2x(f) { return (Math.log(f) - logMin) / logRng * (w - 10) + 5; }

                                var xMin = f2x(fMin);
                                var xMax = f2x(fMax);
                                var envNorm = Math.min(1, Math.max(0.05, inputLevel * 3 * (sensitivity / 50)));
                                var sweepPos = invertSweep ? 1 - envNorm : envNorm;
                                var currentX = f2x(fMin + (fMax - fMin) * sweepPos);
                                currentFreq = fMin + (fMax - fMin) * sweepPos;

                                ctx.fillStyle = "#E1060020";
                                ctx.fillRect(xMin, 5, xMax - xMin, h - 10);

                                ctx.strokeStyle = "#555";
                                ctx.lineWidth = 1;
                                for (var f = 100; f <= 16000; f *= 2) {
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

                                for (var x = 0; x <= w; x += 2) {
                                    var freq = Math.exp(logMin + (x / w) * logRng);
                                    var env = 0;
                                    var peakCenter = currentFreq;
                                    var q = 1 + res * 10;

                                    if (filterType === "Bandpass") {
                                        var ratio = freq / peakCenter;
                                        env = 1 / Math.sqrt(1 + Math.pow(q * (ratio - 1/ratio), 2));
                                    } else if (filterType === "Lowpass") {
                                        env = freq <= peakCenter ? 1 : 1 / (1 + Math.pow((freq - peakCenter) / (peakCenter / q), 2));
                                    } else if (filterType === "Highpass") {
                                        env = freq >= peakCenter ? 1 : 1 / (1 + Math.pow((peakCenter - freq) / (peakCenter / q), 2));
                                    } else if (filterType === "Peaking") {
                                        env = 1 + (res * 4 - 1) / (1 + Math.pow((freq - peakCenter) / (peakCenter / (q * 2)), 2));
                                    }

                                    env = Math.max(0, Math.min(1, env));
                                    var y = h - 5 - env * (h - 10);
                                    ctx.fillStyle = freq >= fMin && freq <= fMax ? "#E1060022" : "#00000000";
                                    ctx.lineTo(x, Math.max(5, y));
                                }

                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 2;
                                ctx.beginPath();
                                ctx.moveTo(currentX, 5);
                                ctx.lineTo(currentX, h - 5);
                                ctx.stroke();

                                ctx.fillStyle = "#E10600";
                                ctx.beginPath();
                                ctx.arc(currentX, h / 2, 5, 0, Math.PI * 2);
                                ctx.fill();

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Filter: " + filterType, 4, 12);
                                ctx.fillText("Freq: " + currentFreq.toFixed(0) + "Hz", 4, 24);
                                ctx.fillStyle = "#ff8800";
                                ctx.textAlign = "right";
                                ctx.fillText("Env: " + (envNorm * 100).toFixed(0) + "%", w - 4, 12);
                            }

                            Timer {
                                interval: 50
                                running: true
                                repeat: true
                                onTriggered: wahCanvas.requestPaint()
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

                    Text { text: "AUTO-WAH / FILTER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Type"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Bandpass", "Lowpass", "Highpass", "Peaking"]
                            currentIndex: 0
                            onActivated: filterType = currentText
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Freq Min"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 100; to: 2000; value: freqMin; Layout.fillWidth: true; onMoved: freqMin = value }
                        Text { text: freqMin.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Freq Max"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 500; to: 12000; value: freqMax; Layout.fillWidth: true; onMoved: freqMax = value }
                        Text { text: freqMax.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Resonance"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 1; value: resonance; stepSize: 0.01; Layout.fillWidth: true; onMoved: resonance = value }
                        Text { text: resonance.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Depth"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: depth; Layout.fillWidth: true; onMoved: depth = value }
                        Text { text: depth.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Mix"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: mix; Layout.fillWidth: true; onMoved: mix = value }
                        Text { text: mix.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "ENVELOPE"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Sensitivity"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: sensitivity; Layout.fillWidth: true; onMoved: sensitivity = value }
                        Text { text: sensitivity.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Attack"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 1; to: 100; value: attack; Layout.fillWidth: true; onMoved: attack = value; enabled: envelopeFollower }
                        Text { text: attack.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Release"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 10; to: 500; value: release; Layout.fillWidth: true; onMoved: release = value; enabled: envelopeFollower }
                        Text { text: release.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    RowLayout {
                        visible: !envelopeFollower
                        Text { text: "LFO Rate"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.05; to: 8; value: lfoRate; stepSize: 0.01; Layout.fillWidth: true; onMoved: lfoRate = value }
                        Text { text: lfoRate.toFixed(2) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: autoMode
                        onCheckedChanged: autoMode = checked
                        Text {
                            text: "Auto Mode"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: envelopeFollower
                        onCheckedChanged: envelopeFollower = checked
                        Text {
                            text: "Env Follower"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: invertSweep
                        onCheckedChanged: invertSweep = checked
                        Text {
                            text: "Invert Sweep"
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
                    }
                }
            }
        }
    }
}


