import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real threshold: -30
    property real frequency: 7000
    property real bandwidth: 2000
    property real reduction: 6
    property real attack: 1
    property real release: 50
    property real range: 12
    property bool listenMode: false
    property bool autoThreshold: true
    property bool splitBand: true
    property string detectionMode: "Wide"
    property real inputLevel: AudioBridge ? Math.max(AudioBridge.leftPeak, AudioBridge.rightPeak) : 0
    property real sibilanceReduction: 0

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

                    Text { text: "SIBILANCE SPECTRUM"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: deessCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var logMin = Math.log(100);
                                var logMax = Math.log(16000);
                                var logRng = logMax - logMin;
                                function f2x(f) { return (Math.log(f) - logMin) / logRng * (w - 10) + 5; }

                                var fcX = f2x(frequency);
                                var bwLow = f2x(Math.max(100, frequency - bandwidth / 2));
                                var bwHigh = f2x(Math.min(16000, frequency + bandwidth / 2));

                                ctx.fillStyle = "#E1060018";
                                ctx.fillRect(bwLow, 5, bwHigh - bwLow, h - 10);

                                var peakDb = linearToDb(inputLevel);
                                var bandEnergy = peakDb > -50 ? 1 : 0;

                                for (var b = 0; b < w; b += 4) {
                                    var freq = Math.exp(logMin + (b / w) * logRng);
                                    var energy = 0;
                                    if (freq > 3000) energy = (freq - 3000) / 13000;
                                    var noise = Math.sin(b * 0.1) * 0.3 + Math.sin(b * 0.07) * 0.2;
                                    var specAmp = Math.max(0, (energy + noise * 0.2) * (1 + peakDb / 40));
                                    specAmp = Math.min(1, specAmp);
                                    var specH = specAmp * (h - 12);
                                    ctx.fillStyle = freq >= frequency - bandwidth / 2 && freq <= frequency + bandwidth / 2
                                        ? "#E1060044" : "#ffffff08";
                                    ctx.fillRect(b, h - 6 - specH, 3, specH);
                                }

                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 2;
                                ctx.beginPath();
                                ctx.moveTo(fcX, 5);
                                ctx.lineTo(fcX, h - 5);
                                ctx.stroke();

                                ctx.setLineDash([3, 3]);
                                ctx.strokeStyle = "#ffff0044";
                                ctx.lineWidth = 1;
                                var thrY = (1 - (threshold + 60) / 60) * (h - 10) + 5;
                                ctx.beginPath();
                                ctx.moveTo(0, thrY);
                                ctx.lineTo(w, thrY);
                                ctx.stroke();
                                ctx.setLineDash([]);

                                sibilanceReduction = bandEnergy > 0.5 + threshold / 60
                                    ? reduction * (bandEnergy - 0.5 - threshold / 60)
                                    : 0;

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Center: " + frequency.toFixed(0) + "Hz", 4, 12);
                                ctx.fillText("BW: " + bandwidth.toFixed(0) + "Hz", 4, 24);
                                ctx.fillStyle = "#ff8800";
                                ctx.fillText("Reduction: " + sibilanceReduction.toFixed(1) + "dB", w - 100, 12);
                                ctx.fillStyle = "#666";
                                ctx.textAlign = "center";
                                ctx.fillText("Sibilance Band", fcX, h - 2);
                            }

                            Timer {
                                interval: 80
                                running: true
                                repeat: true
                                onTriggered: deessCanvas.requestPaint()
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

                    Text { text: "DE-ESSER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Frequency"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 2000; to: 12000; value: frequency; Layout.fillWidth: true; onMoved: frequency = value }
                        Text { text: frequency.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Bandwidth"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 200; to: 6000; value: bandwidth; Layout.fillWidth: true; onMoved: bandwidth = value }
                        Text { text: bandwidth.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Threshold"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -60; to: 0; value: threshold; Layout.fillWidth: true; onMoved: threshold = value; enabled: !autoThreshold }
                        Text { text: threshold.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Reduction"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 24; value: reduction; Layout.fillWidth: true; onMoved: reduction = value }
                        Text { text: reduction.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Range"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 3; to: 24; value: range; Layout.fillWidth: true; onMoved: range = value }
                        Text { text: range.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Attack"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.1; to: 10; value: attack; Layout.fillWidth: true; onMoved: attack = value }
                        Text { text: attack.toFixed(1) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Release"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 10; to: 200; value: release; Layout.fillWidth: true; onMoved: release = value }
                        Text { text: release.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Detection"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Wide", "Split", "Dual"]
                            currentIndex: 0
                            onActivated: detectionMode = currentText
                        }
                    }

                    CheckBox {
                        checked: autoThreshold
                        onCheckedChanged: autoThreshold = checked
                        Text {
                            text: "Auto Threshold"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: splitBand
                        onCheckedChanged: splitBand = checked
                        Text {
                            text: "Split Band"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: listenMode
                        onCheckedChanged: listenMode = checked
                        Text {
                            text: "Listen (Solo Sibilance)"
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
                                AudioBridge.applyCompressor(threshold, 10, attack, release, dbToLinear(-reduction));
                            }
                        }
                    }
                }
            }
        }
    }
}


