import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real tapeSpeed: 7.5
    property real wowFlutter: 0.3
    property real saturation: 40
    property real bias: 50
    property real noiseFloor: 5
    property real crosstalk: 2
    property real hissLevel: 10
    property bool noiseReduction: false
    property bool autoCalibrate: true
    property string tapeType: "Chrome II"
    property real tracking: 0

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

                    Text { text: "TAPE CHARACTERISTICS"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: tapeCanvas
                            anchors.fill: parent
                            property real phase: 0

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var saturationNorm = saturation / 100;
                                var biasNorm = (bias - 50) / 50;

                                ctx.strokeStyle = "#444";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, h - 4);
                                var prevX = 0, prevY = h - 4;
                                for (var x = 0; x <= w; x += 4) {
                                    var input = (x / w) * 2 - 1;
                                    var shaped = input;
                                    shaped += saturationNorm * 0.3 * (input * input * input);
                                    shaped = shaped * (1 + biasNorm * 0.2);
                                    shaped = Math.tanh(shaped * (1 + saturationNorm * 0.5));
                                    var y = h/2 - shaped * (h/2 - 6);
                                    if (x === 0) ctx.moveTo(x, y);
                                    else ctx.lineTo(x, y);
                                }
                                ctx.stroke();

                                ctx.strokeStyle = "#666";
                                ctx.lineWidth = 0.5;
                                ctx.setLineDash([2, 4]);
                                ctx.beginPath();
                                ctx.moveTo(0, h/2);
                                ctx.lineTo(w, h/2);
                                ctx.stroke();
                                ctx.setLineDash([]);

                                var flutterAmp = wowFlutter / 100 * 8;
                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 0.5;
                                ctx.beginPath();
                                for (var fx = 0; fx <= w; fx += 2) {
                                    var fm = Math.sin((fx / w) * Math.PI * 8 + phase) * flutterAmp;
                                    var fy = h/2 + fm + Math.sin((fx / w) * Math.PI * 3 + phase * 1.5) * flutterAmp * 0.5;
                                    if (fx === 0) ctx.moveTo(fx, fy);
                                    else ctx.lineTo(fx, fy);
                                }
                                ctx.stroke();

                                ctx.fillStyle = "#E1060044";
                                ctx.font = "8px monospace";
                                ctx.fillText("Saturation: " + saturation.toFixed(0) + "%", 4, 12);
                                ctx.fillText("Speed: " + tapeSpeed.toFixed(1) + " ips", 4, h - 4);
                            }

                            Timer {
                                interval: 50
                                running: true
                                repeat: true
                                onTriggered: { tapeCanvas.phase += 0.1; tapeCanvas.requestPaint() }
                            }
                        }
                    }

                    RowLayout {
                        Text { text: "Tape:"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Type I (Ferric)", "Chrome II", "Metal IV", "Studio Master"]
                            currentIndex: 1
                            onActivated: tapeType = currentText
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

                    Text { text: "TAPE EMULATOR"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Speed"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 1.875; to: 30; value: tapeSpeed; stepSize: 0.1; Layout.fillWidth: true; onMoved: tapeSpeed = value }
                        Text { text: tapeSpeed.toFixed(1) + "ips"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Saturation"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: saturation; Layout.fillWidth: true; onMoved: saturation = value }
                        Text { text: saturation.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Bias"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: bias; Layout.fillWidth: true; onMoved: bias = value }
                        Text { text: bias.toFixed(0); color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "DEGRADATION"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Wow/Flutter"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 5; value: wowFlutter; stepSize: 0.01; Layout.fillWidth: true; onMoved: wowFlutter = value }
                        Text { text: wowFlutter.toFixed(2) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Noise Floor"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 20; value: noiseFloor; Layout.fillWidth: true; onMoved: noiseFloor = value }
                        Text { text: "-" + (100 - noiseFloor * 5).toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Crosstalk"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 15; value: crosstalk; Layout.fillWidth: true; onMoved: crosstalk = value }
                        Text { text: crosstalk.toFixed(1) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Hiss"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 30; value: hissLevel; Layout.fillWidth: true; onMoved: hissLevel = value }
                        Text { text: hissLevel.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: noiseReduction
                        onCheckedChanged: noiseReduction = checked
                        Text {
                            text: "Dolby NR"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: autoCalibrate
                        onCheckedChanged: autoCalibrate = checked
                        Text {
                            text: "Auto Calibrate"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Item { Layout.fillHeight: true }

                    KsButton {
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


