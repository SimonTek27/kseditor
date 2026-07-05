import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real crossoverLow: 250
    property real crossoverHigh: 4000
    property real lowGain: 0
    property real midGain: 0
    property real highGain: 0
    property int filterOrder: 2
    property bool soloLow: false
    property bool soloMid: false
    property bool soloHigh: false
    property bool muteLow: false
    property bool muteMid: false
    property bool muteHigh: false
    property bool linkSlopes: true

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

                    Text { text: "CROSSOVER SPECTRUM"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: splitCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var logMin = Math.log(20);
                                var logMax = Math.log(20000);
                                var logRange = logMax - logMin;
                                function freqToX(freq) { return ((Math.log(freq) - logMin) / logRange) * (w - 10) + 5; }

                                var xOverLow = freqToX(crossoverLow);
                                var xOverHigh = freqToX(crossoverHigh);

                                var order = filterOrder;

                                ctx.fillStyle = "#E1060022";
                                ctx.beginPath();
                                ctx.rect(5, 5, xOverLow - 5, h - 10);
                                ctx.fill();

                                ctx.fillStyle = "#00ff8822";
                                ctx.beginPath();
                                ctx.rect(xOverLow, 5, xOverHigh - xOverLow, h - 10);
                                ctx.fill();

                                ctx.fillStyle = "#4488ff22";
                                ctx.beginPath();
                                ctx.rect(xOverHigh, 5, w - 5 - xOverHigh, h - 10);
                                ctx.fill();

                                for (var b = 0; b < 3; b++) {
                                    var bandColor = b === 0 ? "#E10600" : b === 1 ? "#00ff88" : "#4488ff";
                                    var solo = b === 0 ? soloLow : b === 1 ? soloMid : soloHigh;
                                    if (solo) {
                                        ctx.strokeStyle = bandColor;
                                        ctx.lineWidth = 3;
                                    } else {
                                        ctx.strokeStyle = bandColor + "44";
                                        ctx.lineWidth = 1;
                                    }
                                }

                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = soloLow ? 2 : 1;
                                ctx.beginPath();
                                ctx.moveTo(5, h - 5);
                                for (var x1 = 5; x1 <= xOverLow; x1 += 2) {
                                    var freq1 = Math.exp(logMin + ((x1 - 5) / (w - 10)) * logRange);
                                    var attenLow = freq1 > crossoverLow ? -24 * order * (Math.log(freq1 / crossoverLow) / Math.LN2) : 0;
                                    var y1 = h - 5 - (1 - Math.min(1, -attenLow / 24)) * (h - 10) * 0.8;
                                    ctx.lineTo(x1, Math.max(5, y1));
                                }
                                ctx.stroke();

                                ctx.strokeStyle = "#00ff88";
                                ctx.lineWidth = soloMid ? 2 : 1;
                                ctx.beginPath();
                                ctx.moveTo(xOverLow, h - 5);
                                for (var x2 = xOverLow; x2 <= xOverHigh; x2 += 2) {
                                    var freq2 = Math.exp(logMin + ((x2 - 5) / (w - 10)) * logRange);
                                    var attenLow2 = freq2 < crossoverLow ? -24 * order * (Math.log(crossoverLow / freq2) / Math.LN2) : 0;
                                    var attenHigh2 = freq2 > crossoverHigh ? -24 * order * (Math.log(freq2 / crossoverHigh) / Math.LN2) : 0;
                                    var attenBand = Math.min(attenLow2, attenHigh2);
                                    if (attenBand > 0) attenBand = 0;
                                    var y2 = h - 5 - (1 - Math.min(1, -attenBand / 24)) * (h - 10) * 0.8;
                                    ctx.lineTo(x2, Math.max(5, y2));
                                }
                                ctx.stroke();

                                ctx.strokeStyle = "#4488ff";
                                ctx.lineWidth = soloHigh ? 2 : 1;
                                ctx.beginPath();
                                ctx.moveTo(xOverHigh, h - 5);
                                for (var x3 = xOverHigh; x3 <= w - 5; x3 += 2) {
                                    var freq3 = Math.exp(logMin + ((x3 - 5) / (w - 10)) * logRange);
                                    var attenHigh = freq3 < crossoverHigh ? -24 * order * (Math.log(crossoverHigh / freq3) / Math.LN2) : 0;
                                    var y3 = h - 5 - (1 - Math.min(1, -attenHigh / 24)) * (h - 10) * 0.8;
                                    ctx.lineTo(x3, Math.max(5, y3));
                                }
                                ctx.stroke();

                                ctx.strokeStyle = "#ffff0044";
                                ctx.lineWidth = 1;
                                ctx.setLineDash([3, 3]);
                                ctx.beginPath();
                                ctx.moveTo(xOverLow, 5);
                                ctx.lineTo(xOverLow, h - 5);
                                ctx.moveTo(xOverHigh, 5);
                                ctx.lineTo(xOverHigh, h - 5);
                                ctx.stroke();
                                ctx.setLineDash([]);

                                ctx.fillStyle = "#ff0";
                                ctx.font = "7px monospace";
                                ctx.textAlign = "center";
                                ctx.fillText("Low", 5 + (xOverLow - 5) / 2, h / 2);
                                ctx.fillText("Mid", xOverLow + (xOverHigh - xOverLow) / 2, h / 2);
                                ctx.fillText("High", xOverHigh + (w - 5 - xOverHigh) / 2, h / 2);

                                ctx.fillStyle = "#888";
                                ctx.textAlign = "center";
                                ctx.fillText(crossoverLow.toFixed(0) + "Hz", xOverLow, h - 2);
                                ctx.fillText(crossoverHigh.toFixed(0) + "Hz", xOverHigh, h - 2);
                            }

                            Timer {
                                interval: 100
                                running: true
                                repeat: true
                                onTriggered: splitCanvas.requestPaint()
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

                    Text { text: "MULTI-BAND SPLITTER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Text { text: "CROSSOVER FREQUENCIES"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Low-Mid"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 20; to: 2000; value: crossoverLow; Layout.fillWidth: true; onMoved: crossoverLow = value }
                        Text { text: crossoverLow.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Mid-High"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 200; to: 16000; value: crossoverHigh; Layout.fillWidth: true; onMoved: crossoverHigh = value }
                        Text { text: crossoverHigh.toFixed(0) + "Hz"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "BAND GAINS"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Rectangle { width: 10; height: 10; color: "#E10600"; radius: 2 }
                        Text { text: "Low"; color: "#aaa"; font.pixelSize: 9; Layout.preferredWidth: 30 }
                        Slider { from: -24; to: 24; value: lowGain; Layout.fillWidth: true; onMoved: lowGain = value }
                        Text { text: lowGain.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Rectangle { width: 10; height: 10; color: "#00ff88"; radius: 2 }
                        Text { text: "Mid"; color: "#aaa"; font.pixelSize: 9; Layout.preferredWidth: 30 }
                        Slider { from: -24; to: 24; value: midGain; Layout.fillWidth: true; onMoved: midGain = value }
                        Text { text: midGain.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Rectangle { width: 10; height: 10; color: "#4488ff"; radius: 2 }
                        Text { text: "High"; color: "#aaa"; font.pixelSize: 9; Layout.preferredWidth: 30 }
                        Slider { from: -24; to: 24; value: highGain; Layout.fillWidth: true; onMoved: highGain = value }
                        Text { text: highGain.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "FILTER ORDER"; color: "#666"; font.pixelSize: 9 }

                    Row {
                        spacing: 4
                        Repeater {
                            model: [1, 2, 3, 4]
                            delegate: KsButton {
                                height: 24; width: 36
                                text: modelData + "x"
                                bgcolor: filterOrder === modelData ? "#E10600" : "#3e3e42"
                                color: filterOrder === modelData ? "#121212" : "#ffffff"
                                onClicked: filterOrder = modelData
                            }
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "SOLO / MUTE"; color: "#666"; font.pixelSize: 9 }

                    Row {
                        spacing: 8
                        ColumnLayout {
                            CheckBox { checked: soloLow; onCheckedChanged: soloLow = checked }
                            Text { text: "Solo L"; color: "#aaa"; font.pixelSize: 8 }
                        }
                        ColumnLayout {
                            CheckBox { checked: soloMid; onCheckedChanged: soloMid = checked }
                            Text { text: "Solo M"; color: "#aaa"; font.pixelSize: 8 }
                        }
                        ColumnLayout {
                            CheckBox { checked: soloHigh; onCheckedChanged: soloHigh = checked }
                            Text { text: "Solo H"; color: "#aaa"; font.pixelSize: 8 }
                        }
                        Item { width: 10 }
                        ColumnLayout {
                            CheckBox { checked: muteLow; onCheckedChanged: muteLow = checked }
                            Text { text: "Mute L"; color: "#aaa"; font.pixelSize: 8 }
                        }
                        ColumnLayout {
                            CheckBox { checked: muteMid; onCheckedChanged: muteMid = checked }
                            Text { text: "Mute M"; color: "#aaa"; font.pixelSize: 8 }
                        }
                        ColumnLayout {
                            CheckBox { checked: muteHigh; onCheckedChanged: muteHigh = checked }
                            Text { text: "Mute H"; color: "#aaa"; font.pixelSize: 8 }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    KsButton {
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


