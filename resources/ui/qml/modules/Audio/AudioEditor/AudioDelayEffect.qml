import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property int numTaps: 2
    property real delayTime: 250
    property real feedback: 30
    property real wetMix: 40
    property real lowCut: 200
    property real highCut: 8000
    property real stereoSpread: 50
    property real pingPong: 0
    property bool tempoSync: false
    property bool reverse: false
    property string noteDivision: "1/4"

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

                    Text { text: "DELAY TAPS"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: delayCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var delayMs = delayTime;
                                var maxTime = Math.max(200, delayMs * numTaps * 1.5);
                                var scale = (w - 20) / maxTime;

                                for (var t = 0; t < 2000; t += 100) {
                                    var tx = 10 + t * scale;
                                    if (tx > w - 10) break;
                                    ctx.strokeStyle = "#2a2a2a";
                                    ctx.lineWidth = 1;
                                    ctx.beginPath();
                                    ctx.moveTo(tx, 10);
                                    ctx.lineTo(tx, h - 10);
                                    ctx.stroke();

                                    ctx.fillStyle = "#444";
                                    ctx.font = "7px monospace";
                                    ctx.textAlign = "center";
                                    ctx.fillText(t + "ms", tx, h - 2);
                                }

                                var feedbackDecay = feedback / 100;
                                var spread = pingPong / 100;

                                for (var i = 0; i < numTaps; i++) {
                                    var tapTime = delayMs * (i + 1);
                                    var tapAmp = Math.pow(1 - feedback / 100, i);
                                    var tx2 = 10 + tapTime * scale;
                                    var panOffset = (i % 2 === 0 ? -1 : 1) * spread * (h / 2 - 20) * 0.4;
                                    var yCenter = h / 2 + panOffset;

                                    if (reverse) {
                                        tapAmp = 1 - i / numTaps;
                                    }

                                    var barH = tapAmp * (h - 30);

                                    var gradient = ctx.createLinearGradient(tx2 - 4, yCenter, tx2 + 4, yCenter);
                                    gradient.addColorStop(0, "#E10600");
                                    gradient.addColorStop(1, "#00ff88");
                                    ctx.fillStyle = gradient;
                                    ctx.globalAlpha = Math.max(0.15, tapAmp);
                                    ctx.fillRect(tx2 - 3, yCenter - barH / 2, 6, barH);

                                    ctx.globalAlpha = Math.max(0.5, tapAmp);
                                    ctx.strokeStyle = "#fff";
                                    ctx.lineWidth = 1;
                                    ctx.beginPath();
                                    ctx.arc(tx2, yCenter, 3, 0, Math.PI * 2);
                                    ctx.stroke();

                                    ctx.fillStyle = "#fff";
                                    ctx.font = "7px monospace";
                                    ctx.textAlign = "center";
                                    ctx.fillText((i + 1) + "", tx2, yCenter + 3);

                                    ctx.globalAlpha = 1.0;

                                    if (i > 0) {
                                        var prevTime = delayMs * (i);
                                        var prevX = 10 + prevTime * scale;
                                        var prevPanOffset = ((i - 1) % 2 === 0 ? -1 : 1) * spread * (h / 2 - 20) * 0.4;
                                        var prevY = h / 2 + prevPanOffset;

                                        ctx.strokeStyle = "#ffffff22";
                                        ctx.lineWidth = 0.5;
                                        ctx.beginPath();
                                        ctx.moveTo(prevX, prevY);
                                        ctx.lineTo(tx2, yCenter);
                                        ctx.stroke();
                                    }
                                }

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Taps: " + numTaps + " | Feedback: " + feedback.toFixed(0) + "%", 4, 14);
                            }

                            Timer {
                                interval: 100
                                running: true
                                repeat: true
                                onTriggered: delayCanvas.requestPaint()
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

                    Text { text: "DELAY"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Time"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 10; to: 1000; value: delayTime; Layout.fillWidth: true; onMoved: delayTime = value; enabled: !tempoSync }
                        Text { text: delayTime.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Feedback"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: feedback; Layout.fillWidth: true; onMoved: feedback = value }
                        Text { text: feedback.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Mix"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: wetMix; Layout.fillWidth: true; onMoved: wetMix = value }
                        Text { text: wetMix.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "FILTERING"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Low Cut"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 20; to: 2000; value: lowCut; Layout.fillWidth: true; onMoved: lowCut = value }
                        Text { text: lowCut.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "High Cut"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 500; to: 20000; value: highCut; Layout.fillWidth: true; onMoved: highCut = value }
                        Text { text: highCut.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "STEREO"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Spread"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: stereoSpread; Layout.fillWidth: true; onMoved: stereoSpread = value }
                        Text { text: stereoSpread.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Ping-Pong"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: pingPong; Layout.fillWidth: true; onMoved: pingPong = value }
                        Text { text: pingPong.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    RowLayout {
                        Text { text: "Taps"; color: "#aaa"; font.pixelSize: 9 }
                        SpinBox {
                            from: 1; to: 8; value: numTaps
                            Layout.fillWidth: true
                            onValueModified: numTaps = value
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: tempoSync
                        onCheckedChanged: tempoSync = checked
                        Text {
                            text: "Tempo Sync"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    RowLayout {
                        visible: tempoSync
                        Text { text: "Division"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["1/16", "1/8", "1/4", "1/2", "1/1", "3/4"]
                            currentIndex: 2
                            onActivated: noteDivision = currentText
                        }
                    }
                    CheckBox {
                        checked: reverse
                        onCheckedChanged: reverse = checked
                        Text {
                            text: "Reverse"
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
                                AudioBridge.applyReverb(delayTime / 1000, feedback / 100, wetMix / 100);
                            }
                        }
                    }
                }
            }
        }
    }
}


