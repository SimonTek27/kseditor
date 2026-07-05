import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real correlation: 0.85
    property real leftLevel: AudioBridge ? AudioBridge.leftPeak : 0
    property real rightLevel: AudioBridge ? AudioBridge.rightPeak : 0
    property real stereoBalance: 50
    property real phaseAngle: 0
    property bool freeze: false
    property bool showGrid: true
    property bool showPhase: true
    property bool showCorrelation: true
    property string viewMode: "Goniometer"
    property real inputLevel: Math.max(leftLevel, rightLevel)

    function dbToLinear(db) { return Math.pow(10, db / 20); }
    function linearToDb(lin) { return lin > 0 ? 20 * Math.log(lin) / Math.LN10 : -120; }

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

                    Text { text: "PHASE SCOPE"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#000000"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: scopeCanvas
                            anchors.fill: parent

                            property var trail: []
                            property int trailLen: 120

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#000000";
                                ctx.fillRect(0, 0, w, h);

                                var cx = w / 2, cy = h / 2;
                                var r = Math.min(cx, cy) - 8;

                                if (showGrid) {
                                    ctx.strokeStyle = "#1a1a1a";
                                    ctx.lineWidth = 1;
                                    ctx.beginPath();
                                    ctx.arc(cx, cy, r, 0, Math.PI * 2);
                                    ctx.stroke();

                                    ctx.strokeStyle = "#111";
                                    ctx.beginPath();
                                    ctx.arc(cx, cy, r * 0.5, 0, Math.PI * 2);
                                    ctx.stroke();

                                    ctx.strokeStyle = "#111";
                                    ctx.lineWidth = 1;
                                    ctx.beginPath();
                                    ctx.moveTo(cx - r, cy);
                                    ctx.lineTo(cx + r, cy);
                                    ctx.moveTo(cx, cy - r);
                                    ctx.lineTo(cx, cy + r);
                                    ctx.stroke();

                                    ctx.strokeStyle = "#151515";
                                    ctx.beginPath();
                                    ctx.moveTo(cx - r * 0.7, cy - r * 0.7);
                                    ctx.lineTo(cx + r * 0.7, cy + r * 0.7);
                                    ctx.moveTo(cx + r * 0.7, cy - r * 0.7);
                                    ctx.lineTo(cx - r * 0.7, cy + r * 0.7);
                                    ctx.stroke();

                                    ctx.fillStyle = "#333";
                                    ctx.font = "8px monospace";
                                    ctx.textAlign = "center";
                                    ctx.fillText("L", cx - r + 10, cy + 3);
                                    ctx.fillText("R", cx + r - 10, cy + 3);
                                    ctx.fillText("M", cx, cy - r + 10);
                                    ctx.fillText("S", cx, cy + r - 6);
                                }

                                var lLevel = leftLevel;
                                var rLevel = rightLevel;
                                var lNorm = Math.min(1, lLevel * 3);
                                var rNorm = Math.min(1, rLevel * 3);

                                if (!freeze) {
                                    var x = cx + (rNorm - lNorm) * r * 0.7;
                                    var y = cy - (lNorm + rNorm) * r * 0.35;
                                    trail.push({x: x, y: y, a: 1.0});
                                    if (trail.length > trailLen) trail.shift();
                                }

                                for (var i = 0; i < trail.length; i++) {
                                    var t = trail[i];
                                    var alpha = (i / trail.length) * 0.8 + 0.1;
                                    ctx.fillStyle = ctx.createRadialGradient(t.x, t.y, 0, t.x, t.y, 3);
                                    ctx.fillStyle = "rgba(225, 6, 0, " + alpha + ")";
                                    ctx.beginPath();
                                    ctx.arc(t.x, t.y, 2 + alpha * 2, 0, Math.PI * 2);
                                    ctx.fill();
                                }

                                if (trail.length > 1) {
                                    ctx.strokeStyle = "#E1060066";
                                    ctx.lineWidth = 0.8;
                                    ctx.beginPath();
                                    ctx.moveTo(trail[0].x, trail[0].y);
                                    for (var j = 1; j < trail.length; j++) {
                                        ctx.lineTo(trail[j].x, trail[j].y);
                                    }
                                    ctx.stroke();
                                }

                                var instantX = cx + (rNorm - lNorm) * r * 0.7;
                                var instantY = cy - (lNorm + rNorm) * r * 0.35;

                                ctx.fillStyle = "#E10600";
                                ctx.beginPath();
                                ctx.arc(instantX, instantY, 4, 0, Math.PI * 2);
                                ctx.fill();

                                ctx.strokeStyle = "#ffffff44";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(cx, cy);
                                ctx.lineTo(instantX, instantY);
                                ctx.stroke();

                                phaseAngle = Math.atan2(instantY - cy, instantX - cx) * 180 / Math.PI;
                                correlation = (lNorm > 0 && rNorm > 0)
                                    ? (lNorm - Math.abs(lNorm - rNorm)) / (lNorm + rNorm)
                                    : 0;

                                if (showCorrelation) {
                                    ctx.fillStyle = "#00ff88";
                                    ctx.font = "bold 11px monospace";
                                    ctx.textAlign = "left";
                                    ctx.fillText("Φ: " + correlation.toFixed(2), 6, 16);
                                    ctx.fillStyle = "#888";
                                    ctx.font = "9px monospace";
                                    ctx.fillText("Phase: " + phaseAngle.toFixed(1) + "°", 6, 30);
                                    ctx.textAlign = "right";
                                    var corrStr = correlation > 0.7 ? "In Phase" : correlation > 0.3 ? "Partial" : correlation > -0.3 ? "Mono" : "Out of Phase";
                                    ctx.fillStyle = correlation > 0.7 ? "#00ff88" : correlation > -0.3 ? "#ffaa00" : "#E10600";
                                    ctx.fillText(corrStr, w - 6, 16);
                                    ctx.fillStyle = "#666";
                                    var dbStr = linearToDb(Math.max(lLevel, rLevel)).toFixed(1) + "dBFS";
                                    ctx.fillText(dbStr, w - 6, 30);
                                }
                            }

                            Timer {
                                interval: 40
                                running: true
                                repeat: true
                                onTriggered: scopeCanvas.requestPaint()
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

                    Text { text: "STEREO CORRELATOR"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        height: 60
                        color: "#0e0e0e"
                        Layout.fillWidth: true
                        radius: 4

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2
                            Text { text: "Correlation"; color: "#888"; font.pixelSize: 8; horizontalAlignment: Text.AlignHCenter }
                            Text {
                                text: correlation.toFixed(3)
                                color: correlation > 0.7 ? "#00ff88" : correlation > 0.3 ? "#ffaa00" : correlation > -0.3 ? "#ff8800" : "#E10600"
                                font.pixelSize: 22; font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Text {
                                text: correlation > 0.7 ? "In Phase" : correlation > 0.3 ? "Partial" : correlation > -0.3 ? "Mono" : "Out of Phase"
                                color: correlation > 0.7 ? "#00ff88" : "#ffaa00"
                                font.pixelSize: 9
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "METERING"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Left"; color: "#aaa"; font.pixelSize: 9; Layout.preferredWidth: 35 }
                        Rectangle {
                            Layout.fillWidth: true; height: 12
                            color: "#0e0e0e"; radius: 2
                            Rectangle {
                                width: parent.width * Math.min(1, leftLevel * 3); height: 12
                                color: "#E10600"; radius: 2
                            }
                        }
                        Text { text: linearToDb(leftLevel).toFixed(1) + "dB"; color: "#888"; font.pixelSize: 8; Layout.preferredWidth: 55 }
                    }
                    RowLayout {
                        Text { text: "Right"; color: "#aaa"; font.pixelSize: 9; Layout.preferredWidth: 35 }
                        Rectangle {
                            Layout.fillWidth: true; height: 12
                            color: "#0e0e0e"; radius: 2
                            Rectangle {
                                width: parent.width * Math.min(1, rightLevel * 3); height: 12
                                color: "#00ff88"; radius: 2
                            }
                        }
                        Text { text: linearToDb(rightLevel).toFixed(1) + "dB"; color: "#888"; font.pixelSize: 8; Layout.preferredWidth: 55 }
                    }

                    RowLayout {
                        Text { text: "Balance"; color: "#aaa"; font.pixelSize: 9 }
                        Rectangle {
                            Layout.fillWidth: true; height: 12
                            color: "#0e0e0e"; radius: 2
                            Rectangle {
                                width: parent.width; height: 12
                                color: "#ffaa00"; radius: 2
                                property real bal: leftLevel + rightLevel > 0
                                    ? (leftLevel - rightLevel) / (leftLevel + rightLevel) * parent.width / 2 + parent.width / 2
                                    : parent.width / 2
                                x: bal - width / 2
                                width: 4
                            }
                        }
                        Text { text: ((leftLevel - rightLevel) / Math.max(0.001, leftLevel + rightLevel) * 100).toFixed(0) + "%"; color: "#888"; font.pixelSize: 8; Layout.preferredWidth: 45 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Phase"; color: "#aaa"; font.pixelSize: 9 }
                        Text { text: phaseAngle.toFixed(1) + "°"; color: "#E10600"; font.pixelSize: 11; font.bold: true }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "View"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Goniometer", "Lissajous", "Phase Meter"]
                            currentIndex: 0
                            onActivated: viewMode = currentText
                        }
                    }

                    CheckBox {
                        checked: showGrid
                        onCheckedChanged: showGrid = checked
                        Text {
                            text: "Show Grid"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: showPhase
                        onCheckedChanged: showPhase = checked
                        Text {
                            text: "Show Phase Line"
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
                }
            }
        }
    }
}


