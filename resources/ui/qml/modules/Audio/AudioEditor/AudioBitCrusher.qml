import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property int bitDepth: 8
    property real sampleRateRedux: 1
    property real mix: 60
    property real noiseShape: 30
    property real aliasing: 20
    property real foldback: 0
    property real wetGain: 0
    property real dryGain: -6
    property bool dither: true
    property bool noiseShaper: false
    property bool hifiMode: false
    property string algorithm: "Quantize"

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

                    Text { text: "BIT REDUCTION"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: crusherCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var bits = bitDepth;
                                var rateRedux = sampleRateRedux;
                                var quantSteps = Math.max(2, Math.pow(2, bits));
                                var oversample = hifiMode ? 4 : 1;

                                ctx.strokeStyle = "#444";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, h / 2);
                                ctx.lineTo(w, h / 2);
                                ctx.stroke();

                                var samples = Math.max(2, w / rateRedux);

                                ctx.strokeStyle = "#555";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                var prevY = h / 2;
                                var prevX = 0;
                                for (var i = 0; i <= w; i += Math.max(1, Math.floor(rateRedux / oversample))) {
                                    var input = Math.sin((i / w) * Math.PI * 8) * 0.7
                                        + Math.sin((i / w) * Math.PI * 16) * 0.3;
                                    var quantized = Math.round(input * quantSteps) / quantSteps;

                                    if (algorithm === "Foldback" && foldback > 0) {
                                        if (quantized > 1 - foldback / 100) quantized = 2 - quantized;
                                        if (quantized < -1 + foldback / 100) quantized = -2 - quantized;
                                    }

                                    if (algorithm === "Rectify") {
                                        quantized = Math.abs(input);
                                    }

                                    if (noiseShaper && dither) {
                                        var ditherNoise = (Math.random() - 0.5) / quantSteps * noiseShape / 50;
                                        quantized += ditherNoise;
                                    }

                                    quantized = Math.max(-1, Math.min(1, quantized));

                                    var y = h / 2 - quantized * (h / 2 - 8);
                                    if (i === 0) ctx.moveTo(i, Math.max(4, Math.min(h - 4, y)));
                                    else {
                                        ctx.lineTo(i, Math.max(4, Math.min(h - 4, prevY)));
                                        ctx.lineTo(i, Math.max(4, Math.min(h - 4, y)));
                                    }
                                    prevY = y;
                                    prevX = i;
                                }
                                ctx.stroke();

                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                for (var s = 0; s <= w; s += 2) {
                                    var clean = Math.sin((s / w) * Math.PI * 8) * 0.7
                                        + Math.sin((s / w) * Math.PI * 16) * 0.3;
                                    var cy = h / 2 - clean * (h / 2 - 8);
                                    if (s === 0) ctx.moveTo(s, cy);
                                    else ctx.lineTo(s, cy);
                                }
                                ctx.stroke();

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Bit Depth: " + bits + " bit", 4, 12);
                                ctx.textAlign = "right";
                                ctx.fillText("Rate: " + (44100 / rateRedux / 1000).toFixed(1) + "kHz", w - 4, 12);
                            }

                            Timer {
                                interval: 100
                                running: true
                                repeat: true
                                onTriggered: crusherCanvas.requestPaint()
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

                    Text { text: "BIT CRUSHER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Text { text: "RESOLUTION"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Bit Depth"; color: "#aaa"; font.pixelSize: 9 }
                        SpinBox {
                            from: 1; to: 16; value: bitDepth
                            Layout.fillWidth: true
                            onValueModified: bitDepth = value
                        }
                    }
                    RowLayout {
                        Text { text: "Rate Redux"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 1; to: 64; value: sampleRateRedux; stepSize: 1; Layout.fillWidth: true; onMoved: sampleRateRedux = value }
                        Text { text: "1/" + sampleRateRedux.toFixed(0); color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Mix"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: mix; Layout.fillWidth: true; onMoved: mix = value }
                        Text { text: mix.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Dry Gain"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -24; to: 0; value: dryGain; Layout.fillWidth: true; onMoved: dryGain = value }
                        Text { text: dryGain.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Wet Gain"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -24; to: 12; value: wetGain; Layout.fillWidth: true; onMoved: wetGain = value }
                        Text { text: (wetGain >= 0 ? "+" : "") + wetGain.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "ALGORITHM"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Type"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Quantize", "Foldback", "Rectify", "Sample Hold"]
                            currentIndex: 0
                            onActivated: algorithm = currentText
                        }
                    }

                    RowLayout {
                        visible: algorithm === "Foldback"
                        Text { text: "Foldback"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: foldback; Layout.fillWidth: true; onMoved: foldback = value }
                        Text { text: foldback.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "DITHER / NOISE"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Noise Shape"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: noiseShape; Layout.fillWidth: true; onMoved: noiseShape = value }
                        Text { text: noiseShape.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    CheckBox {
                        checked: dither
                        onCheckedChanged: dither = checked
                        Text {
                            text: "Dither"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: noiseShaper
                        onCheckedChanged: noiseShaper = checked
                        Text {
                            text: "Noise Shaper"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: hifiMode
                        onCheckedChanged: hifiMode = checked
                        Text {
                            text: "Hi-Fi Mode"
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


