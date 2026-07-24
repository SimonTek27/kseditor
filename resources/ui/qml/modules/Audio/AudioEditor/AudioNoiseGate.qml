import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real threshold: -40
    property real floor: -80
    property real attack: 1
    property real hold: 20
    property real release: 100
    property real range: 40
    property real hysteresis: 3
    property bool duckingMode: false
    property bool lookahead: false
    property real inputLevel: AudioBridge ? Math.max(AudioBridge.leftPeak, AudioBridge.rightPeak) : 0
    property bool gateOpen: false

    function dbToLinear(db) { return Math.pow(10, db / 20); }
    function linearToDb(lin) { return lin > 0 ? 20 * Math.log(lin) / Math.LN10 : -100; }

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
                    spacing: 6

                    Text { text: "GATE ENVELOPE"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: gateCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var attackMs = Math.max(0.5, attack);
                                var holdMs = Math.max(1, hold);
                                var releaseMs = Math.max(1, release);
                                var totalMs = attackMs + holdMs + releaseMs;
                                var scale = w / totalMs;

                                ctx.strokeStyle = "#555";
                                ctx.lineWidth = 1;

                                var thresholdY = (1 - (threshold + 100) / 120) * (h - 10) + 5;
                                var floorY = (1 - (floor + 100) / 120) * (h - 10) + 5;

                                ctx.setLineDash([3, 3]);
                                ctx.strokeStyle = "#ffff0044";
                                ctx.beginPath();
                                ctx.moveTo(0, thresholdY);
                                ctx.lineTo(w, thresholdY);
                                ctx.stroke();

                                ctx.strokeStyle = "#ff444444";
                                ctx.beginPath();
                                ctx.moveTo(0, floorY);
                                ctx.lineTo(w, floorY);
                                ctx.stroke();
                                ctx.setLineDash([]);

                                var aEnd = attackMs * scale;
                                var hEnd = aEnd + holdMs * scale;
                                var rEnd = hEnd + releaseMs * scale;

                                var gateVal = gateOpen ? 1 : 0;

                                ctx.fillStyle = "#00ff8822";
                                ctx.strokeStyle = "#00ff88";
                                ctx.lineWidth = 2;
                                ctx.beginPath();
                                ctx.moveTo(0, h - 5);

                                for (var x = 0; x <= w; x += 2) {
                                    var t = x / scale;
                                    var val = 0;
                                    if (x < 0.5) val = gateOpen ? 1 : 0;
                                    else if (x >= 0 && x < aEnd) {
                                        var p = x / aEnd;
                                        val = gateOpen && x < aEnd * (1 - gateVal) ? 0 : gateOpen ? 1 : p;
                                    } else if (x >= aEnd && x < hEnd) {
                                        val = gateOpen ? 1 : 0;
                                    } else if (x >= hEnd && x <= rEnd) {
                                        var q = (x - hEnd) / (rEnd - hEnd);
                                        val = gateOpen ? 1 - q : 0;
                                    } else {
                                        val = 0;
                                    }
                                    var y = h - 5 - val * (h - 10);
                                    ctx.lineTo(x, y);
                                }
                                ctx.stroke();

                                ctx.fillStyle = "#00ff8844";
                                ctx.font = "8px monospace";
                                ctx.fillText("Attack: " + attack.toFixed(1) + "ms", 4, 12);
                                ctx.fillText("Hold: " + hold.toFixed(0) + "ms", 4, 24);
                                ctx.fillText("Release: " + release.toFixed(0) + "ms", w - 80, 12);
                                ctx.fillStyle = "#ffff0044";
                                ctx.fillText("Thresh: " + threshold.toFixed(0) + "dB", w - 80, h - 4);

                                var peakDb = linearToDb(inputLevel);
                                var peakY = (1 - (peakDb + 100) / 120) * (h - 10) + 5;
                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, peakY);
                                ctx.lineTo(w, peakY);
                                ctx.stroke();

                                gateOpen = peakDb > threshold;
                            }

                            Timer {
                                interval: 50
                                running: true
                                repeat: true
                                onTriggered: gateCanvas.requestPaint()
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

                    Text { text: "NOISE GATE"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Threshold"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -80; to: 0; value: threshold; Layout.fillWidth: true; onMoved: threshold = value }
                        Text { text: threshold.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Floor"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -96; to: -20; value: floor; Layout.fillWidth: true; onMoved: floor = value }
                        Text { text: floor.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Range"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 80; value: range; Layout.fillWidth: true; onMoved: range = value }
                        Text { text: range.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Attack"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.1; to: 50; value: attack; Layout.fillWidth: true; onMoved: attack = value }
                        Text { text: attack.toFixed(1) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Hold"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 1; to: 500; value: hold; Layout.fillWidth: true; onMoved: hold = value }
                        Text { text: hold.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Release"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 5; to: 1000; value: release; Layout.fillWidth: true; onMoved: release = value }
                        Text { text: release.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Hysteresis"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 12; value: hysteresis; Layout.fillWidth: true; onMoved: hysteresis = value }
                        Text { text: hysteresis.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: duckingMode
                        onCheckedChanged: duckingMode = checked
                        Text {
                            text: "Ducking Mode"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: lookahead
                        onCheckedChanged: lookahead = checked
                        Text {
                            text: "Lookahead"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    RowLayout {
                        Text { text: "Status:"; color: "#888"; font.pixelSize: 9 }
                        Text {
                            text: gateOpen ? "OPEN" : "CLOSED"
                            color: gateOpen ? "#00ff88" : "#ff4444"
                            font.bold: true; font.pixelSize: 11
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
                                AudioBridge.applyCompressor(threshold, 20, attack, release, 0);
                            }
                        }
                    }
                }
            }
        }
    }
}


