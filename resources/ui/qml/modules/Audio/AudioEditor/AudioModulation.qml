import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property string effectMode: "Chorus"
    property real rate: 0.5
    property real depth: 50
    property real feedback: 30
    property real mix: 50
    property real delay: 8
    property real phase: 90
    property real stereoWidth: 70
    property real resonance: 0.3
    property real spread: 0.5
    property int voices: 3
    property bool syncMode: false
    property bool invertSide: false

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

                    Text { text: "MODULATION LFO"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: modCanvas
                            anchors.fill: parent
                            property real phase_offset: 0

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var depthNorm = depth / 100;
                                var rateHz = rate;

                                ctx.strokeStyle = "#444";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, h / 2);
                                ctx.lineTo(w, h / 2);
                                ctx.stroke();

                                var modeIdx = ["Chorus","Flanger","Phaser","Tremolo","Vibrato"].indexOf(effectMode);
                                if (modeIdx < 0) modeIdx = 0;

                                for (var ch = 0; ch < voices; ch++) {
                                    var chPhase = (ch / voices) * Math.PI * 2 * spread;
                                    if (invertSide && ch === 1) chPhase += Math.PI;
                                    var colors = ["#E10600", "#00ff88", "#4488ff", "#ff8800", "#ff44ff"];
                                    ctx.strokeStyle = colors[ch % colors.length];
                                    ctx.lineWidth = ch === 0 ? 2 : 1;
                                    ctx.globalAlpha = ch === 0 ? 1.0 : 0.5;

                                    ctx.beginPath();
                                    for (var x = 0; x <= w; x += 2) {
                                        var t = (x / w) * (modeIdx === 1 ? 6 : 4) * Math.PI;
                                        var lfo = 0;
                                        if (modeIdx === 0) {
                                            lfo = Math.sin(t + chPhase) * depthNorm * (h / 3) + 2;
                                        } else if (modeIdx === 1) {
                                            var tri = 2 * Math.abs(2 * ((t + chPhase) / (2 * Math.PI) % 1) - 1) - 1;
                                            lfo = tri * depthNorm * (h / 3);
                                        } else if (modeIdx === 2) {
                                            lfo = Math.sin(t + chPhase) * depthNorm * (h / 3);
                                            var env = Math.sin(t * 0.3 + chPhase) * 0.3 + 0.7;
                                            lfo *= env;
                                        } else if (modeIdx === 3) {
                                            lfo = Math.abs(Math.sin(t + chPhase)) * depthNorm * (h / 3) - depthNorm * (h / 6);
                                        } else if (modeIdx === 4) {
                                            lfo = Math.sin(t * 3 + chPhase) * depthNorm * (h / 4);
                                        }
                                        var y = h / 2 + lfo;
                                        if (x === 0) ctx.moveTo(x, Math.max(4, Math.min(h - 4, y)));
                                        else ctx.lineTo(x, Math.max(4, Math.min(h - 4, y)));
                                    }
                                    ctx.stroke();
                                }
                                ctx.globalAlpha = 1.0;

                                var phaseDeg = phase;
                                var phOff = (phaseDeg / 360) * w;
                                ctx.strokeStyle = "#ffff0044";
                                ctx.lineWidth = 1;
                                ctx.setLineDash([2, 4]);
                                ctx.beginPath();
                                ctx.moveTo(phOff, 5);
                                ctx.lineTo(phOff, h - 5);
                                ctx.stroke();
                                ctx.setLineDash([]);

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Mode: " + effectMode + " | Rate: " + rateHz.toFixed(2) + "Hz", 4, 12);
                                ctx.textAlign = "right";
                                ctx.fillText("Voices: " + voices, w - 4, 12);
                            }

                            Timer {
                                interval: 50
                                running: true
                                repeat: true
                                onTriggered: { modCanvas.phase_offset += 0.05; modCanvas.requestPaint() }
                            }
                        }
                    }

                    RowLayout {
                        Text { text: "Mode"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Chorus", "Flanger", "Phaser", "Tremolo", "Vibrato"]
                            currentIndex: 0
                            onActivated: effectMode = currentText
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

                    Text { text: "MODULATION"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Rate"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.05; to: 8; value: rate; stepSize: 0.01; Layout.fillWidth: true; onMoved: rate = value }
                        Text { text: rate.toFixed(2) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Depth"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: depth; Layout.fillWidth: true; onMoved: depth = value }
                        Text { text: depth.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Feedback"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: feedback; Layout.fillWidth: true; onMoved: feedback = value }
                        Text { text: feedback.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Mix"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: mix; Layout.fillWidth: true; onMoved: mix = value }
                        Text { text: mix.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "PARAMETERS"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Delay"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.5; to: 20; value: delay; Layout.fillWidth: true; onMoved: delay = value }
                        Text { text: delay.toFixed(1) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Phase"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 360; value: phase; Layout.fillWidth: true; onMoved: phase = value }
                        Text { text: phase.toFixed(0) + "°"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Spread"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 1; value: spread; stepSize: 0.01; Layout.fillWidth: true; onMoved: spread = value }
                        Text { text: spread.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Width"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: stereoWidth; Layout.fillWidth: true; onMoved: stereoWidth = value }
                        Text { text: stereoWidth.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Resonance"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 1; value: resonance; stepSize: 0.01; Layout.fillWidth: true; onMoved: resonance = value }
                        Text { text: resonance.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }

                    RowLayout {
                        Text { text: "Voices"; color: "#aaa"; font.pixelSize: 9 }
                        SpinBox {
                            from: 1; to: 8; value: voices
                            Layout.fillWidth: true
                            onValueModified: voices = value
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: syncMode
                        onCheckedChanged: syncMode = checked
                        Text {
                            text: "Tempo Sync"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: invertSide
                        onCheckedChanged: invertSide = checked
                        Text {
                            text: "Invert Side"
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


