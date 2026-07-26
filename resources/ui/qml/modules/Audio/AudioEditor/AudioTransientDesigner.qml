import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real attackGain: 3
    property real sustainGain: 0
    property real attackTime: 5
    property real releaseTime: 50
    property real sensitivity: 50
    property real lookahead: 2
    property bool splitMode: false
    property bool soloTransient: false
    property bool soloSustain: false
    property real ratio: 1.0

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

                    Text { text: "TRANSIENT ANALYSIS"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: transCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var attackBoost = attackGain / 12;

                                ctx.strokeStyle = "#555";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                var prevX = 0, prevY = h;
                                for (var x = 0; x <= w; x += 3) {
                                    var t = x / w;
                                    var env = Math.exp(-t * 5);
                                    var trans = env * (1 + attackBoost * 4);
                                    if (x < w * 0.02) trans += 0.5 * attackBoost;
                                    var noise = (Math.random() - 0.5) * 0.05;
                                    var y = h - (trans + noise) * (h - 4);
                                    y = Math.max(4, Math.min(h - 4, y));
                                    if (x === 0) ctx.moveTo(x, y);
                                    else ctx.lineTo(x, y);
                                }
                                ctx.stroke();

                                var sustainLevel = 0.2 + sustainGain / 12;
                                ctx.strokeStyle = "#00ff8888";
                                ctx.lineWidth = 1.5;
                                ctx.beginPath();
                                ctx.moveTo(0, h - h * sustainLevel);
                                ctx.lineTo(w, h - h * sustainLevel);
                                ctx.stroke();

                                var sensNorm = sensitivity / 100;
                                var thresholdY = h - sensNorm * (h - 10);
                                ctx.strokeStyle = "#ffff0044";
                                ctx.lineWidth = 1;
                                ctx.setLineDash([3, 3]);
                                ctx.beginPath();
                                ctx.moveTo(0, thresholdY);
                                ctx.lineTo(w, thresholdY);
                                ctx.stroke();
                                ctx.setLineDash([]);

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.fillText("Attack: +" + attackGain.toFixed(0) + "dB", 4, 12);
                                ctx.fillText("Sustain: " + sustainGain.toFixed(0) + "dB", 4, h - 4);
                                ctx.fillStyle = "#ffff0044";
                                ctx.fillText("Sensitivity", w - 60, thresholdY - 4);
                            }

                            Timer {
                                interval: 100
                                running: true
                                repeat: true
                                onTriggered: transCanvas.requestPaint()
                            }
                        }
                    }

                    RowLayout {
                        Text { text: "Detection"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Broadband", "Low", "Mid", "High"]
                            currentIndex: 0
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

                    Text { text: "TRANSIENT DESIGNER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Attack"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -12; to: 12; value: attackGain; Layout.fillWidth: true; onMoved: attackGain = value }
                        Text { text: (attackGain >= 0 ? "+" : "") + attackGain.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Sustain"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -12; to: 12; value: sustainGain; Layout.fillWidth: true; onMoved: sustainGain = value }
                        Text { text: (sustainGain >= 0 ? "+" : "") + sustainGain.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Attack Time"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.1; to: 50; value: attackTime; Layout.fillWidth: true; onMoved: attackTime = value }
                        Text { text: attackTime.toFixed(1) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Release"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 5; to: 500; value: releaseTime; Layout.fillWidth: true; onMoved: releaseTime = value }
                        Text { text: releaseTime.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Sensitivity"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: sensitivity; Layout.fillWidth: true; onMoved: sensitivity = value }
                        Text { text: sensitivity.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Lookahead"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 10; value: lookahead; stepSize: 0.1; Layout.fillWidth: true; onMoved: lookahead = value }
                        Text { text: lookahead.toFixed(1) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: splitMode
                        onCheckedChanged: splitMode = checked
                        Text {
                            text: "Split Mode"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: soloTransient
                        onCheckedChanged: soloTransient = checked
                        Text {
                            text: "Solo Transient"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: soloSustain
                        onCheckedChanged: soloSustain = checked
                        Text {
                            text: "Solo Sustain"
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


