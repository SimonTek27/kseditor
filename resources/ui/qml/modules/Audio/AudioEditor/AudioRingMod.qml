import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real carrierFreq: 440
    property real mix: 60
    property real depth: 80
    property real feedback: 20
    property real damp: 0.3
    property real phase: 0
    property string carrierWave: "Sine"
    property bool hfDamping: true
    property bool dcBlock: true
    property bool carrierOnly: false
    property bool amMode: false
    property real inputLevel: AudioBridge ? Math.max(AudioBridge.leftPeak, AudioBridge.rightPeak) : 0

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

                    Text { text: "CARRIER WAVEFORM"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: ringCanvas
                            anchors.fill: parent
                            property real phaseOff: 0

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                ctx.strokeStyle = "#444";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, h / 2);
                                ctx.lineTo(w, h / 2);
                                ctx.stroke();

                                var fc = carrierFreq;
                                var amp = 1.0;
                                var envNorm = Math.min(1, inputLevel * 3);

                                ctx.strokeStyle = "#ffffff15";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                for (var c = 0; c <= w; c += 3) {
                                    var clean = Math.sin((c / w) * Math.PI * 6) * 0.6;
                                    var cy = h / 2 - clean * (h / 2 - 10);
                                    if (c === 0) ctx.moveTo(c, cy);
                                    else ctx.lineTo(c, cy);
                                }
                                ctx.stroke();

                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 1.5;
                                ctx.beginPath();
                                for (var x = 0; x <= w; x += 2) {
                                    var t = (x / w) * 4 * Math.PI;
                                    var carrier = 0;
                                    var carrierPhase = t + phaseOff + phase * Math.PI / 180;

                                    if (carrierWave === "Sine") {
                                        carrier = Math.sin(carrierPhase * fc / 200);
                                    } else if (carrierWave === "Square") {
                                        carrier = Math.sin(carrierPhase * fc / 200) > 0 ? 1 : -1;
                                    } else if (carrierWave === "Saw") {
                                        carrier = 2 * ((carrierPhase * fc / 200 / (2 * Math.PI)) % 1) - 1;
                                    } else if (carrierWave === "Triangle") {
                                        var tri = 2 * Math.abs(2 * ((carrierPhase * fc / 200 / (2 * Math.PI)) % 1) - 1) - 1;
                                        carrier = tri;
                                    } else if (carrierWave === "Noise") {
                                        carrier = Math.sin(x * 13.7 + x * 7.3) * Math.cos(x * 5.1);
                                    }

                                    var modulated = amMode
                                        ? (1 + carrier * depth / 100) * 0.5
                                        : carrier * (depth / 100);

                                    var output = modulated * envNorm;
                                    output = Math.max(-1, Math.min(1, output));

                                    var y = h / 2 - output * (h / 2 - 10);
                                    if (x === 0) ctx.moveTo(x, Math.max(4, Math.min(h - 4, y)));
                                    else ctx.lineTo(x, Math.max(4, Math.min(h - 4, y)));
                                }
                                ctx.stroke();

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Carrier: " + carrierWave + " @ " + fc.toFixed(0) + "Hz", 4, 12);
                                ctx.fillText("Mode: " + (amMode ? "AM" : "Ring Mod"), 4, 24);
                                ctx.fillStyle = "#00ff8844";
                                ctx.textAlign = "right";
                                ctx.fillText("Input: " + (envNorm * 100).toFixed(0) + "%", w - 4, 12);
                            }

                            Timer {
                                interval: 60
                                running: true
                                repeat: true
                                onTriggered: { ringCanvas.phaseOff += 0.05; ringCanvas.requestPaint() }
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

                    Text { text: "RING MODULATOR"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Carrier"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Sine", "Square", "Saw", "Triangle", "Noise"]
                            currentIndex: 0
                            onActivated: carrierWave = currentText
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Frequency"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 20; to: 4000; value: carrierFreq; Layout.fillWidth: true; onMoved: carrierFreq = value }
                        Text { text: carrierFreq.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
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
                    RowLayout {
                        Text { text: "Feedback"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: feedback; Layout.fillWidth: true; onMoved: feedback = value }
                        Text { text: feedback.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Phase"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 360; value: phase; Layout.fillWidth: true; onMoved: phase = value }
                        Text { text: phase.toFixed(0) + "°"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Damp"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 1; value: damp; stepSize: 0.01; Layout.fillWidth: true; onMoved: damp = value }
                        Text { text: damp.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }

                    CheckBox {
                        checked: hfDamping
                        onCheckedChanged: hfDamping = checked
                        Text {
                            text: "HF Damping"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: dcBlock
                        onCheckedChanged: dcBlock = checked
                        Text {
                            text: "DC Block"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: amMode
                        onCheckedChanged: amMode = checked
                        Text {
                            text: "AM Mode"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: carrierOnly
                        onCheckedChanged: carrierOnly = checked
                        Text {
                            text: "Carrier Only"
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


