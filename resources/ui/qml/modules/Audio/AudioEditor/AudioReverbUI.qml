import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property string reverbType: "Hall"
    property real roomSize: 0.6
    property real decay: 2.0
    property real damping: 0.5
    property real diffusion: 0.6
    property real wetMix: 35
    property real preDelay: 10
    property real stereoWidth: 80
    property real hiDamp: 6000
    property real loDamp: 200
    property bool tailOnly: false
    property bool freeze: false
    property real modulation: 0.2

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

                    Text { text: "REVERB DECAY"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: rvbCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var typeIdx = ["Room","Hall","Plate","Spring","Cathedral","Chamber"].indexOf(reverbType);
                                if (typeIdx < 0) typeIdx = 1;
                                var density = 0.2 + typeIdx * 0.12;
                                var dec = decay;
                                var diff = diffusion;

                                ctx.fillStyle = "#00ff8810";
                                ctx.beginPath();
                                ctx.moveTo(0, h - 5);
                                for (var x = 0; x <= w; x += 2) {
                                    var t = (x / w) * 5;
                                    var env = Math.exp(-t / dec) * (1 + Math.sin(t * 10 * diff) * 0.1 * diff);
                                    var irNoise = 0;
                                    for (var n = 0; n < 4; n++) {
                                        irNoise += Math.sin(x * (2 + n * 1.5) + n * 3.7) * (0.05 * density / (1 + n));
                                    }
                                    var y = h - 5 - (env + irNoise) * (h - 10);
                                    if (x === 0) ctx.moveTo(x, Math.max(5, y));
                                    else ctx.lineTo(x, Math.max(5, y));
                                }
                                ctx.strokeStyle = "#00ff8844";
                                ctx.lineWidth = 1.5;
                                ctx.stroke();

                                ctx.fillStyle = "#E1060020";
                                ctx.beginPath();
                                ctx.moveTo(0, h - 5);
                                var earlyRef = 0.3;
                                for (var x2 = 0; x2 <= w * 0.15; x2 += 2) {
                                    var t2 = (x2 / w) * 5;
                                    var erEnv = Math.exp(-t2 * 8) * earlyRef;
                                    var erY = h - 5 - erEnv * (h - 10);
                                    if (x2 === 0) ctx.moveTo(x2, Math.max(5, erY));
                                    else ctx.lineTo(x2, Math.max(5, erY));
                                }
                                ctx.strokeStyle = "#E1060044";
                                ctx.lineWidth = 1.5;
                                ctx.stroke();

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Type: " + reverbType, 4, 12);
                                ctx.fillText("Decay: " + dec.toFixed(1) + "s", 4, 24);
                                ctx.fillText("Diffusion: " + (diff * 100).toFixed(0) + "%", w - 90, 12);

                                ctx.strokeStyle = "#ffffff11";
                                ctx.lineWidth = 1;
                                ctx.setLineDash([2, 4]);
                                ctx.beginPath();
                                ctx.moveTo(w * 0.15, 5);
                                ctx.lineTo(w * 0.15, h - 5);
                                ctx.stroke();
                                ctx.setLineDash([]);
                                ctx.fillStyle = "#666";
                                ctx.font = "7px monospace";
                                ctx.textAlign = "center";
                                ctx.fillText("Early Ref.", w * 0.15, h - 1);
                            }

                            Timer {
                                interval: 100
                                running: true
                                repeat: true
                                onTriggered: rvbCanvas.requestPaint()
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

                    Text { text: "ALGORITHMIC REVERB"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Type"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Room", "Hall", "Plate", "Spring", "Cathedral", "Chamber"]
                            currentIndex: 1
                            onActivated: reverbType = currentText
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Room Size"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.1; to: 1.0; value: roomSize; stepSize: 0.01; Layout.fillWidth: true; onMoved: roomSize = value }
                        Text { text: roomSize.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Decay"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.1; to: 10; value: decay; Layout.fillWidth: true; onMoved: decay = value }
                        Text { text: decay.toFixed(1) + "s"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Pre-Delay"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 200; value: preDelay; Layout.fillWidth: true; onMoved: preDelay = value }
                        Text { text: preDelay.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Mix"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: wetMix; Layout.fillWidth: true; onMoved: wetMix = value }
                        Text { text: wetMix.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "DAMPING"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Damping"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 1; value: damping; stepSize: 0.01; Layout.fillWidth: true; onMoved: damping = value }
                        Text { text: damping.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Hi Damp"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 500; to: 20000; value: hiDamp; Layout.fillWidth: true; onMoved: hiDamp = value }
                        Text { text: hiDamp.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Lo Damp"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 20; to: 2000; value: loDamp; Layout.fillWidth: true; onMoved: loDamp = value }
                        Text { text: loDamp.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Diffusion"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 1; value: diffusion; stepSize: 0.01; Layout.fillWidth: true; onMoved: diffusion = value }
                        Text { text: diffusion.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Modulation"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 1; value: modulation; stepSize: 0.01; Layout.fillWidth: true; onMoved: modulation = value }
                        Text { text: modulation.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Width"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: stereoWidth; Layout.fillWidth: true; onMoved: stereoWidth = value }
                        Text { text: stereoWidth.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: tailOnly
                        onCheckedChanged: tailOnly = checked
                        Text {
                            text: "Tail Only"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: freeze
                        onCheckedChanged: freeze = checked
                        Text {
                            text: "Freeze"
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
                                AudioBridge.applyReverb(roomSize, damping, wetMix / 100);
                            }
                        }
                    }
                }
            }
        }
    }
}


