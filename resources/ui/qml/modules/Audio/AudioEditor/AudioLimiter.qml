import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real ceiling: -0.5
    property real threshold: -6
    property real attack: 0.1
    property real release: 50
    property real lookahead: 2
    property real knee: 0
    property bool autoRelease: true
    property bool oversample2x: false
    property bool oversample4x: false
    property bool truePeak: true
    property real inputLevel: AudioBridge ? Math.max(AudioBridge.leftPeak, AudioBridge.rightPeak) : 0
    property real gainReduction: 0

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

                    Text { text: "LIMITER / CLIPPER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: limCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var thrDb = threshold;
                                var ceilDb = ceiling;
                                var kneeDb = knee;

                                ctx.strokeStyle = "#555";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, 0);
                                ctx.lineTo(w, h);
                                ctx.stroke();

                                var thrY = (1 - (thrDb + 24) / 48) * (h - 10) + 5;
                                var ceilY = (1 - (ceilDb + 24) / 48) * (h - 10) + 5;

                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 2;
                                ctx.beginPath();

                                var kneeStart = thrDb - kneeDb;
                                for (var x = 0; x <= w; x += 2) {
                                    var inDb = (x / w) * 48 - 24;
                                    var outDb;
                                    if (inDb <= kneeStart) {
                                        outDb = inDb;
                                    } else if (inDb <= thrDb) {
                                        var kPos = (inDb - kneeStart) / (thrDb - kneeStart);
                                        if (kneeDb <= 0) {
                                            outDb = inDb;
                                        } else {
                                            var k = Math.sin(Math.PI / 2 * kPos);
                                            outDb = inDb * (1 - k * 0.3);
                                        }
                                    } else {
                                        var overshoot = inDb - thrDb;
                                        var reduction = Math.min(overshoot, 24);
                                        outDb = thrDb - (thrDb - ceilDb) * (1 - Math.exp(-overshoot / 6));
                                    }
                                    if (outDb > ceilDb) outDb = ceilDb;
                                    var sy = (1 - (outDb + 24) / 48) * (h - 10) + 5;
                                    ctx.lineTo(x, Math.max(5, Math.min(h - 5, sy)));
                                }
                                ctx.stroke();

                                ctx.setLineDash([3, 3]);
                                ctx.strokeStyle = "#ffff0044";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, thrY);
                                ctx.lineTo(w, thrY);
                                ctx.stroke();

                                ctx.strokeStyle = "#ff444488";
                                ctx.beginPath();
                                ctx.moveTo(0, ceilY);
                                ctx.lineTo(w, ceilY);
                                ctx.stroke();
                                ctx.setLineDash([]);

                                var peakDb = linearToDb(inputLevel);
                                var peakX = (peakDb + 24) / 48 * w;
                                var outDb2 = peakDb;
                                if (peakDb > thrDb) {
                                    var overshoot2 = peakDb - thrDb;
                                    outDb2 = thrDb - (thrDb - ceilDb) * (1 - Math.exp(-overshoot2 / 6));
                                }
                                if (outDb2 > ceilDb) outDb2 = ceilDb;
                                var outY = (1 - (outDb2 + 24) / 48) * (h - 10) + 5;

                                gainReduction = Math.max(0, peakDb - outDb2);

                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 2;
                                ctx.beginPath();
                                ctx.moveTo(peakX, 0);
                                ctx.lineTo(peakX, h);
                                ctx.stroke();

                                ctx.fillStyle = "#E10600";
                                ctx.beginPath();
                                ctx.arc(peakX, Math.max(5, Math.min(h - 5, outY)), 5, 0, Math.PI * 2);
                                ctx.fill();

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Threshold: " + thrDb.toFixed(1) + "dB", 4, 12);
                                ctx.fillText("Ceiling: " + ceilDb.toFixed(1) + "dBFS", 4, 24);
                                ctx.fillStyle = "#ff8800";
                                ctx.fillText("GR: " + gainReduction.toFixed(1) + "dB", w - 70, 12);
                            }

                            Timer {
                                interval: 60
                                running: true
                                repeat: true
                                onTriggered: limCanvas.requestPaint()
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

                    Text { text: "LIMITER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Ceiling"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -12; to: 0; value: ceiling; stepSize: 0.1; Layout.fillWidth: true; onMoved: ceiling = value }
                        Text { text: ceiling.toFixed(1) + "dBFS"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Threshold"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -24; to: 0; value: threshold; Layout.fillWidth: true; onMoved: threshold = value }
                        Text { text: threshold.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Attack"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.01; to: 5; value: attack; stepSize: 0.01; Layout.fillWidth: true; onMoved: attack = value }
                        Text { text: attack.toFixed(2) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Release"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 1; to: 200; value: release; Layout.fillWidth: true; onMoved: release = value; enabled: !autoRelease }
                        Text { text: release.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Lookahead"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 10; value: lookahead; stepSize: 0.1; Layout.fillWidth: true; onMoved: lookahead = value }
                        Text { text: lookahead.toFixed(1) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Knee"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 6; value: knee; Layout.fillWidth: true; onMoved: knee = value }
                        Text { text: knee.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

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
                        checked: truePeak
                        onCheckedChanged: truePeak = checked
                        Text {
                            text: "True Peak"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Text { text: "OVERSAMPLING"; color: "#666"; font.pixelSize: 9 }
                    Row {
                        spacing: 4
                        AppButton {
                            height: 24; text: "1x"
                            bgcolor: !oversample2x && !oversample4x ? "#E10600" : "#3e3e42"
                            color: !oversample2x && !oversample4x ? "#121212" : "#ffffff"
                            onClicked: { oversample2x = false; oversample4x = false; }
                        }
                        AppButton {
                            height: 24; text: "2x"
                            bgcolor: oversample2x ? "#E10600" : "#3e3e42"
                            color: oversample2x ? "#121212" : "#ffffff"
                            onClicked: { oversample2x = true; oversample4x = false; }
                        }
                        AppButton {
                            height: 24; text: "4x"
                            bgcolor: oversample4x ? "#E10600" : "#3e3e42"
                            color: oversample4x ? "#121212" : "#ffffff"
                            onClicked: { oversample2x = false; oversample4x = true; }
                        }
                    }

                    Rectangle {
                        height: 50
                        color: "#0e0e0e"
                        Layout.fillWidth: true
                        radius: 4
                        ColumnLayout {
                            anchors.centerIn: parent
                            Text { text: "Gain Reduction"; color: "#888"; font.pixelSize: 8; horizontalAlignment: Text.AlignHCenter }
                            Text {
                                text: gainReduction.toFixed(1) + " dB"
                                color: gainReduction > 6 ? "#ff4444" : "#ffaa00"
                                font.pixelSize: 18; font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                            }
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
                                AudioBridge.applyCompressor(threshold, 20, attack, release, dbToLinear(0));
                            }
                        }
                    }
                }
            }
        }
    }
}


