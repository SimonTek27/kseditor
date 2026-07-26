import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real threshold: -20
    property real depth: 12
    property real attack: 5
    property real release: 100
    property real hold: 50
    property real ratio: 4
    property real reduction: 0
    property real sidechainLevel: 0
    property string triggerSource: "Internal"
    property bool duckOnPeak: true
    property bool autoRelease: true
    property bool bypass: false
    property real makeupGain: 0

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

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    Text { text: "DUCKING ENVELOPE"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: duckCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                ctx.fillStyle = "#E1060010";
                                ctx.fillRect(0, 0, w, h);

                                var depthNorm = depth / 24;
                                var thresholdDb = threshold;

                                ctx.strokeStyle = "#444";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, h - 5);
                                ctx.lineTo(w, h - 5);
                                ctx.stroke();

                                var attackMs = Math.max(1, attack);
                                var holdMs = Math.max(1, hold);
                                var releaseMs = Math.max(1, release);
                                var totalMs = attackMs + holdMs + releaseMs;
                                var scale = w / totalMs;

                                var aEnd = attackMs * scale;
                                var hEnd = aEnd + holdMs * scale;
                                var duckAmount = depthNorm;

                                ctx.strokeStyle = "#00ff88";
                                ctx.lineWidth = 2;
                                ctx.beginPath();

                                for (var x = 0; x <= w; x += 2) {
                                    var gain = 1.0;
                                    if (x < aEnd) {
                                        var p = (aEnd - x) / aEnd;
                                        gain = 1 - duckAmount * (1 - p);
                                    } else if (x < hEnd) {
                                        gain = 1 - duckAmount;
                                    } else if (x <= w) {
                                        var p2 = (x - hEnd) / (w - hEnd);
                                        gain = 1 - duckAmount + duckAmount * p2;
                                    }
                                    gain = Math.max(0, gain);
                                    var y = h - 5 - gain * (h - 10);
                                    ctx.lineTo(x, Math.max(5, y));
                                }
                                ctx.stroke();

                                ctx.fillStyle = "#00ff8844";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Depth: " + depth.toFixed(0) + "dB", 4, 12);
                                ctx.fillText("Attack: " + attack.toFixed(1) + "ms", 4, 24);
                                ctx.fillText("Hold: " + hold.toFixed(0) + "ms", w / 2 - 30, 12);
                                ctx.fillText("Release: " + release.toFixed(0) + "ms", w - 90, 24);

                                var sideDb = linearToDb(sidechainLevel);
                                var sideY = (1 - (sideDb + 60) / 60) * (h - 10) + 5;
                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, sideY);
                                ctx.lineTo(w, sideY);
                                ctx.stroke();

                                var thrY = (1 - (thresholdDb + 60) / 60) * (h - 10) + 5;
                                ctx.strokeStyle = "#ffff0044";
                                ctx.lineWidth = 1;
                                ctx.setLineDash([3, 3]);
                                ctx.beginPath();
                                ctx.moveTo(0, thrY);
                                ctx.lineTo(w, thrY);
                                ctx.stroke();
                                ctx.setLineDash([]);

                                var isDucking = sideDb > thresholdDb;
                                var currentReduction = isDucking ? depth * (1 - (thresholdDb - sideDb) / depth) : 0;
                                currentReduction = Math.max(0, Math.min(depth, currentReduction));
                                reduction = currentReduction;

                                ctx.fillStyle = isDucking ? "#ff8800" : "#888";
                                ctx.font = "bold 10px monospace";
                                ctx.textAlign = "right";
                                ctx.fillText("GR: " + reduction.toFixed(1) + "dB", w - 4, h - 8);
                            }

                            Timer {
                                interval: 60
                                running: true
                                repeat: true
                                onTriggered: duckCanvas.requestPaint()
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

                    Text { text: "DUCKER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Threshold"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -60; to: 0; value: threshold; Layout.fillWidth: true; onMoved: threshold = value }
                        Text { text: threshold.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Depth"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 30; value: depth; Layout.fillWidth: true; onMoved: depth = value }
                        Text { text: depth.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Ratio"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 1; to: 20; value: ratio; stepSize: 0.5; Layout.fillWidth: true; onMoved: ratio = value }
                        Text { text: ratio.toFixed(1) + ":1"; color: "#E10600"; font.pixelSize: 10 }
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
                        Slider { from: 10; to: 1000; value: release; Layout.fillWidth: true; onMoved: release = value; enabled: !autoRelease }
                        Text { text: release.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Makeup"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 12; value: makeupGain; Layout.fillWidth: true; onMoved: makeupGain = value }
                        Text { text: "+" + makeupGain.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    RowLayout {
                        Text { text: "Trigger"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Internal", "Sidechain 1", "Sidechain 2", "Sidechain Bus"]
                            currentIndex: 0
                            onActivated: triggerSource = currentText
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: duckOnPeak
                        onCheckedChanged: duckOnPeak = checked
                        Text {
                            text: "Duck on Peaks"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: autoRelease
                        onCheckedChanged: autoRelease = checked
                        Text {
                            text: "Auto Release"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: bypass
                        onCheckedChanged: bypass = checked
                        Text {
                            text: "Bypass"
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
                                AudioBridge.applyCompressor(threshold, ratio, attack, release * 0.1, dbToLinear(-depth));
                            }
                        }
                    }
                }
            }
        }
    }
}


