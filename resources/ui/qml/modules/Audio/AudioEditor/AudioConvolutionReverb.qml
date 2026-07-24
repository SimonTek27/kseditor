import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real wetMix: 30
    property real roomSize: 0.6
    property real damping: 0.5
    property real decay: 2.0
    property real preDelay: 20
    property real earlyReflections: 0.3
    property string irSource: "Small Room"
    property bool irLoaded: false

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
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Text { text: "IMPULSE RESPONSE"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: irCanvas
                            anchors.fill: parent
                            property real decayRate: decay

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#555";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var cx = w / 2;
                                for (var i = 0; i < 100; i++) {
                                    var t = i / 100;
                                    var amp = Math.exp(-t * decayRate * 3) * (1 - Math.random() * 0.3);
                                    var y = h/2 - amp * (h/2 - 4) * (t < 0.01 ? 1 : 1);
                                    var x = t * w;
                                    if (i === 0) ctx.moveTo(x, y);
                                    else ctx.lineTo(x, y);
                                }
                                ctx.strokeStyle = irLoaded ? "#00ff88" : "#444";
                                ctx.lineWidth = 1.5;
                                ctx.stroke();

                                for (var j = 0; j < 100; j++) {
                                    var t2 = 0.1 + j / 100 * 0.9;
                                    var amp2 = Math.exp(-t2 * decayRate * 3) * (Math.random() * 0.4 - 0.2);
                                    var y2 = h/2 - amp2 * (h/2 - 4);
                                    var x2 = t2 * w;
                                    if (j === 0) ctx.moveTo(x2, y2);
                                    else ctx.lineTo(x2, y2);
                                }
                                ctx.strokeStyle = irLoaded ? "#88ffaa" : "#333";
                                ctx.lineWidth = 0.8;
                                ctx.stroke();

                                if (!irLoaded) {
                                    ctx.fillStyle = "#666";
                                    ctx.font = "11px sans-serif";
                                    ctx.textAlign = "center";
                                    ctx.fillText("No IR Loaded", w/2, h/2 + 4);
                                }
                            }
                        }
                    }

                    RowLayout {
                        AppButton {
                            height: 28
                            text: "Load IR..."
                            bgcolor: "#3e3e42"
                            color: "#ffffff"
                            onClicked: irLoaded = true
                        }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Small Room", "Medium Hall", "Large Hall", "Plate", "Spring", "Cathedral", "Ambient"]
                            currentIndex: 0
                            onActivated: irSource = currentText
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

                    Text { text: "CONVOLUTION REVERB"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Mix"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: wetMix; Layout.fillWidth: true; onMoved: wetMix = value }
                        Text { text: wetMix.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Room"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.1; to: 1.0; value: roomSize; stepSize: 0.05; Layout.fillWidth: true; onMoved: roomSize = value }
                        Text { text: roomSize.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Damping"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 1; value: damping; stepSize: 0.01; Layout.fillWidth: true; onMoved: damping = value }
                        Text { text: damping.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Decay"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.1; to: 10; value: decay; Layout.fillWidth: true; onMoved: { decay = value; irCanvas.requestPaint(); } }
                        Text { text: decay.toFixed(1) + "s"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Pre-Delay"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 200; value: preDelay; Layout.fillWidth: true; onMoved: preDelay = value }
                        Text { text: preDelay.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Early Ref."; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 1; value: earlyReflections; stepSize: 0.01; Layout.fillWidth: true; onMoved: earlyReflections = value }
                        Text { text: earlyReflections.toFixed(2); color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "IR Info"; color: "#666"; font.pixelSize: 9 }
                    RowLayout {
                        Text { text: "Source:"; color: "#888"; font.pixelSize: 9 }
                        Text { text: irSource; color: "#fff"; font.pixelSize: 9 }
                    }
                    RowLayout {
                        Text { text: "Loaded:"; color: "#888"; font.pixelSize: 9 }
                        Text { text: irLoaded ? "Yes" : "No"; color: irLoaded ? "#00ff88" : "#ff4444"; font.pixelSize: 9 }
                    }
                    RowLayout {
                        Text { text: "Length:"; color: "#888"; font.pixelSize: 9 }
                        Text { text: irLoaded ? "2.4s" : "N/A"; color: "#fff"; font.pixelSize: 9 }
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


