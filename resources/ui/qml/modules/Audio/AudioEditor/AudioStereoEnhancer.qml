import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real stereoWidth: 100
    property real midGain: 0
    property real sideGain: 0
    property real stereoDelay: 0
    property real phaseOffset: 0
    property real balance: 0
    property bool monoMaker: false
    property bool stereoEnhance: true
    property real centerBias: 50

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

                    Text { text: "STEREO FIELD"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: fieldCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                var cx = w / 2, cy = h / 2;
                                var r = Math.min(cx, cy) - 10;

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.arc(cx, cy, r, 0, Math.PI * 2);
                                ctx.stroke();

                                ctx.strokeStyle = "#2a2a2a";
                                ctx.beginPath();
                                ctx.moveTo(cx - r, cy);
                                ctx.lineTo(cx + r, cy);
                                ctx.stroke();
                                ctx.beginPath();
                                ctx.moveTo(cx, cy - r);
                                ctx.lineTo(cx, cy + r);
                                ctx.stroke();

                                var widthNorm = stereoWidth / 100;
                                var spread = r * widthNorm * 0.7;

                                var midLevel = 0.6 + midGain / 24;
                                var sideLevel = 0.4 * widthNorm + sideGain / 24;
                                var leftX = -spread * sideLevel;
                                var rightX = spread * sideLevel;
                                var centerY = -midLevel * 10;

                                ctx.fillStyle = "#00ff8888";
                                ctx.beginPath();
                                ctx.arc(cx + leftX, cy + centerY, 6, 0, Math.PI * 2);
                                ctx.fill();
                                ctx.fillStyle = "#ff444488";
                                ctx.beginPath();
                                ctx.arc(cx + rightX, cy + centerY, 6, 0, Math.PI * 2);
                                ctx.fill();

                                ctx.fillStyle = "#ffff4488";
                                ctx.beginPath();
                                ctx.arc(cx + (leftX + rightX) / 2, cy + centerY, 4, 0, Math.PI * 2);
                                ctx.fill();

                                ctx.strokeStyle = "#ff444444";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(cx + leftX, cy + centerY);
                                ctx.lineTo(cx + rightX, cy + centerY);
                                ctx.stroke();

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "center";
                                ctx.fillText("L", cx - spread * 0.8, cy + r - 4);
                                ctx.fillText("R", cx + spread * 0.8, cy + r - 4);
                                ctx.fillText("M", cx, cy - r + 10);
                            }

                            Timer {
                                interval: 100
                                running: true
                                repeat: true
                                onTriggered: fieldCanvas.requestPaint()
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

                    Text { text: "STEREO ENHANCER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Width"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 200; value: stereoWidth; Layout.fillWidth: true; onMoved: stereoWidth = value }
                        Text { text: stereoWidth.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "MID/SIDE PROCESSING"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Mid Gain"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -12; to: 12; value: midGain; Layout.fillWidth: true; onMoved: midGain = value }
                        Text { text: midGain.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Side Gain"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -12; to: 12; value: sideGain; Layout.fillWidth: true; onMoved: sideGain = value }
                        Text { text: sideGain.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Delay"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 20; value: stereoDelay; Layout.fillWidth: true; onMoved: stereoDelay = value }
                        Text { text: stereoDelay.toFixed(1) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Phase"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -180; to: 180; value: phaseOffset; Layout.fillWidth: true; onMoved: phaseOffset = value }
                        Text { text: phaseOffset.toFixed(0) + "°"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Balance"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -100; to: 100; value: balance; Layout.fillWidth: true; onMoved: balance = value }
                        Text { text: balance.toFixed(0); color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: monoMaker
                        onCheckedChanged: monoMaker = checked
                        Text {
                            text: "Mono Maker"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: stereoEnhance
                        onCheckedChanged: stereoEnhance = checked
                        Text {
                            text: "Stereo Enhance"
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
                                AudioBridge.applyReverb(0.5, 0.5, stereoWidth / 100);
                            }
                        }
                    }
                }
            }
        }
    }
}


