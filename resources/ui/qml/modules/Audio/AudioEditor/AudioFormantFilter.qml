import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real formantShift: 0
    property real formant1Freq: 800
    property real formant1Gain: 0
    property real formant1BW: 100
    property real formant2Freq: 2200
    property real formant2Gain: 0
    property real formant2BW: 120
    property real formant3Freq: 3500
    property real formant3Gain: 0
    property real formant3BW: 150
    property real formant4Freq: 4500
    property real formant4Gain: 0
    property real formant4BW: 200
    property real formant5Freq: 5500
    property real formant5Gain: 0
    property real formant5BW: 300
    property bool vocalMorph: false
    property string vowelSource: "A (ah)"
    property string vowelTarget: "E (ee)"
    property real morphBlend: 50

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

                    Text { text: "FORMANT RESPONSE"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: formantCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var logMin = Math.log(100);
                                var logMax = Math.log(10000);
                                var logRng = logMax - logMin;
                                function f2x(f) { return (Math.log(f) - logMin) / logRng * (w - 10) + 5; }

                                ctx.strokeStyle = "#2a2a2a";
                                ctx.lineWidth = 1;
                                for (var f = 200; f <= 8000; f *= 2) {
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

                                var formants = [
                                    {f: formant1Freq, g: formant1Gain, bw: formant1BW},
                                    {f: formant2Freq, g: formant2Gain, bw: formant2BW},
                                    {f: formant3Freq, g: formant3Gain, bw: formant3BW},
                                    {f: formant4Freq, g: formant4Gain, bw: formant4BW},
                                    {f: formant5Freq, g: formant5Gain, bw: formant5BW}
                                ];

                                var colors = ["#E10600", "#00ff88", "#4488ff", "#ff8800", "#ff44ff"];

                                ctx.strokeStyle = "#555";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(5, h - 5);

                                for (var x = 5; x <= w - 5; x += 2) {
                                    var freq = Math.exp(logMin + ((x - 5) / (w - 10)) * logRng);
                                    var totalGain = 0;

                                    for (var fi = 0; fi < formants.length; fi++) {
                                        var ff = formants[fi].f;
                                        var fg = formants[fi].g;
                                        var fbw = formants[fi].bw;
                                        if (fbw <= 0) continue;
                                        var q = ff / fbw;
                                        var ratio = freq / ff;
                                        var resonance = fg > 0
                                            ? 1 + fg / 12 * (1 / (1 + Math.pow(q * (ratio - 1/ratio), 2)))
                                            : 1 / (1 + Math.pow(q * (ratio - 1/ratio), 2));
                                        totalGain += fg > 0 ? (resonance - 1) : (resonance - 1);
                                    }
                                    totalGain = 1 + totalGain;

                                    var y = h - 5 - Math.max(0, Math.min(2, totalGain)) / 2 * (h - 10);
                                    ctx.lineTo(x, Math.max(5, y));
                                }
                                ctx.stroke();

                                var fShift = formantShift;
                                for (var fi2 = 0; fi2 < formants.length; fi2++) {
                                    var shiftedF = formants[fi2].f * Math.pow(2, fShift / 12);
                                    var sfx = f2x(shiftedF);
                                    ctx.strokeStyle = colors[fi2];
                                    ctx.lineWidth = 2;
                                    ctx.beginPath();
                                    ctx.moveTo(sfx, 5);
                                    ctx.lineTo(sfx, h - 5);
                                    ctx.stroke();
                                }

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Shift: " + (formantShift >= 0 ? "+" : "") + formantShift.toFixed(1) + " st", 4, 12);
                                ctx.fillText("Vocal Morph: " + (vocalMorph ? "ON" : "OFF"), 4, 24);
                            }

                            Timer {
                                interval: 100
                                running: true
                                repeat: true
                                onTriggered: formantCanvas.requestPaint()
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

                    Text { text: "FORMANT FILTER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Shift"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -12; to: 12; value: formantShift; stepSize: 0.5; Layout.fillWidth: true; onMoved: formantShift = value }
                        Text { text: (formantShift >= 0 ? "+" : "") + formantShift.toFixed(1) + "st"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "FORMANT 1"; color: "#E10600"; font.pixelSize: 9 }
                    RowLayout {
                        Text { text: "Freq"; color: "#aaa"; font.pixelSize: 8; Layout.preferredWidth: 30 }
                        Slider { from: 200; to: 2000; value: formant1Freq; Layout.fillWidth: true; onMoved: formant1Freq = value }
                        Text { text: formant1Freq.toFixed(0) + "Hz"; color: "#888"; font.pixelSize: 8 }
                    }
                    RowLayout {
                        Text { text: "Gain"; color: "#aaa"; font.pixelSize: 8; Layout.preferredWidth: 30 }
                        Slider { from: -12; to: 12; value: formant1Gain; Layout.fillWidth: true; onMoved: formant1Gain = value }
                        Text { text: (formant1Gain >= 0 ? "+" : "") + formant1Gain.toFixed(0) + "dB"; color: "#888"; font.pixelSize: 8 }
                    }
                    RowLayout {
                        Text { text: "BW"; color: "#aaa"; font.pixelSize: 8; Layout.preferredWidth: 30 }
                        Slider { from: 30; to: 500; value: formant1BW; Layout.fillWidth: true; onMoved: formant1BW = value }
                        Text { text: formant1BW.toFixed(0) + "Hz"; color: "#888"; font.pixelSize: 8 }
                    }

                    Text { text: "FORMANT 2"; color: "#00ff88"; font.pixelSize: 9 }
                    RowLayout {
                        Text { text: "Freq"; color: "#aaa"; font.pixelSize: 8; Layout.preferredWidth: 30 }
                        Slider { from: 500; to: 4000; value: formant2Freq; Layout.fillWidth: true; onMoved: formant2Freq = value }
                        Text { text: formant2Freq.toFixed(0) + "Hz"; color: "#888"; font.pixelSize: 8 }
                    }
                    RowLayout {
                        Text { text: "Gain"; color: "#aaa"; font.pixelSize: 8; Layout.preferredWidth: 30 }
                        Slider { from: -12; to: 12; value: formant2Gain; Layout.fillWidth: true; onMoved: formant2Gain = value }
                        Text { text: (formant2Gain >= 0 ? "+" : "") + formant2Gain.toFixed(0) + "dB"; color: "#888"; font.pixelSize: 8 }
                    }
                    RowLayout {
                        Text { text: "BW"; color: "#aaa"; font.pixelSize: 8; Layout.preferredWidth: 30 }
                        Slider { from: 30; to: 500; value: formant2BW; Layout.fillWidth: true; onMoved: formant2BW = value }
                        Text { text: formant2BW.toFixed(0) + "Hz"; color: "#888"; font.pixelSize: 8 }
                    }

                    Text { text: "FORMANT 3"; color: "#4488ff"; font.pixelSize: 9 }
                    RowLayout {
                        Text { text: "Freq"; color: "#aaa"; font.pixelSize: 8; Layout.preferredWidth: 30 }
                        Slider { from: 1000; to: 6000; value: formant3Freq; Layout.fillWidth: true; onMoved: formant3Freq = value }
                        Text { text: formant3Freq.toFixed(0) + "Hz"; color: "#888"; font.pixelSize: 8 }
                    }
                    RowLayout {
                        Text { text: "Gain"; color: "#aaa"; font.pixelSize: 8; Layout.preferredWidth: 30 }
                        Slider { from: -12; to: 12; value: formant3Gain; Layout.fillWidth: true; onMoved: formant3Gain = value }
                        Text { text: (formant3Gain >= 0 ? "+" : "") + formant3Gain.toFixed(0) + "dB"; color: "#888"; font.pixelSize: 8 }
                    }
                    RowLayout {
                        Text { text: "BW"; color: "#aaa"; font.pixelSize: 8; Layout.preferredWidth: 30 }
                        Slider { from: 30; to: 500; value: formant3BW; Layout.fillWidth: true; onMoved: formant3BW = value }
                        Text { text: formant3BW.toFixed(0) + "Hz"; color: "#888"; font.pixelSize: 8 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "VOCAL MORPH"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Source"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["A (ah)", "E (ee)", "I (ih)", "O (oh)", "U (oo)"]
                            currentIndex: 0
                            onActivated: vowelSource = currentText
                        }
                    }
                    RowLayout {
                        Text { text: "Target"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["A (ah)", "E (ee)", "I (ih)", "O (oh)", "U (oo)"]
                            currentIndex: 1
                            onActivated: vowelTarget = currentText
                        }
                    }
                    RowLayout {
                        Text { text: "Blend"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: morphBlend; Layout.fillWidth: true; onMoved: morphBlend = value; enabled: vocalMorph }
                        Text { text: morphBlend.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    CheckBox {
                        checked: vocalMorph
                        onCheckedChanged: vocalMorph = checked
                        Text {
                            text: "Enable Vocal Morph"
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
                                AudioBridge.setPitch(formantShift);
                            }
                        }
                    }
                }
            }
        }
    }
}


