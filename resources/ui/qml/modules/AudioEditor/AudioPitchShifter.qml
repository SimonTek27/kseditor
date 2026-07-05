import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real semitones: 0
    property real cents: 0
    property real formantPreserve: 100
    property real timeStretch: 1.0
    property real overlap: 40
    property real windowSize: 60
    property bool preserveFormants: true
    property bool syncTempo: false
    property string algorithm: "WSOLA"

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

                    Text { text: "PITCH / TIME DISPLAY"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: pitchCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var totalCents = semitones * 100 + cents;
                                var shiftFactor = Math.pow(2, totalCents / 1200);
                                var stretchFactor = timeStretch;

                                var cx = w / 2, cy = h / 2;
                                var r = Math.min(cx, cy) - 20;

                                ctx.strokeStyle = "#2a2a2a";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.arc(cx, cy, r, 0, Math.PI * 2);
                                ctx.stroke();

                                for (var a = 0; a < 12; a++) {
                                    var angle = (a / 12) * Math.PI * 2 - Math.PI / 2;
                                    var lx = cx + Math.cos(angle) * (r - 4);
                                    var ly = cy + Math.sin(angle) * (r - 4);
                                    ctx.fillStyle = a === 0 ? "#888" : "#444";
                                    ctx.font = "7px monospace";
                                    ctx.textAlign = "center";
                                    ctx.textBaseline = "middle";
                                    var noteNames = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"];
                                    ctx.fillText(noteNames[a], lx, ly);
                                }

                                var shiftAngle = (totalCents / 1200) * Math.PI * 2;
                                var pointerAngle = -Math.PI / 2 + shiftAngle;
                                var pointerLen = r * 0.7;

                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 2;
                                ctx.beginPath();
                                ctx.moveTo(cx, cy);
                                ctx.lineTo(cx + Math.cos(pointerAngle) * pointerLen,
                                           cy + Math.sin(pointerAngle) * pointerLen);
                                ctx.stroke();

                                ctx.fillStyle = "#E10600";
                                ctx.beginPath();
                                ctx.arc(cx, cy, 4, 0, Math.PI * 2);
                                ctx.fill();

                                var timeAngle = -Math.PI / 2 + (1 - stretchFactor) * Math.PI;
                                ctx.strokeStyle = "#00ff88";
                                ctx.lineWidth = 1.5;
                                ctx.beginPath();
                                ctx.moveTo(cx, cy);
                                ctx.lineTo(cx + Math.cos(timeAngle) * pointerLen * 0.5,
                                           cy + Math.sin(timeAngle) * pointerLen * 0.5);
                                ctx.stroke();

                                ctx.fillStyle = "#888";
                                ctx.font = "9px monospace";
                                ctx.textAlign = "left";
                                ctx.textBaseline = "top";
                                var text = "Pitch: " + (totalCents >= 0 ? "+" : "") + (totalCents / 100).toFixed(2) + " st";
                                ctx.fillText(text, 4, 4);
                                ctx.textBaseline = "bottom";
                                ctx.fillText("Speed: " + (stretchFactor * 100).toFixed(0) + "%", 4, h - 4);

                                ctx.textAlign = "right";
                                ctx.textBaseline = "top";
                                ctx.fillText("Formants: " + (preserveFormants ? "ON" : "OFF"), w - 4, 4);
                            }

                            Timer {
                                interval: 100
                                running: true
                                repeat: true
                                onTriggered: pitchCanvas.requestPaint()
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

                    Text { text: "PITCH SHIFTER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Semitones"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -12; to: 12; value: semitones; stepSize: 0.5; Layout.fillWidth: true; onMoved: semitones = value }
                        Text { text: (semitones >= 0 ? "+" : "") + semitones.toFixed(1); color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Cents"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -100; to: 100; value: cents; Layout.fillWidth: true; onMoved: cents = value }
                        Text { text: (cents >= 0 ? "+" : "") + cents.toFixed(0) + "ct"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Time Stretch"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0.25; to: 4.0; value: timeStretch; stepSize: 0.01; Layout.fillWidth: true; onMoved: timeStretch = value }
                        Text { text: (timeStretch * 100).toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "QUALITY"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Formant"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: formantPreserve; Layout.fillWidth: true; onMoved: formantPreserve = value; enabled: preserveFormants }
                        Text { text: formantPreserve.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Window"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 10; to: 200; value: windowSize; Layout.fillWidth: true; onMoved: windowSize = value }
                        Text { text: windowSize.toFixed(0) + "ms"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Overlap"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 10; to: 80; value: overlap; Layout.fillWidth: true; onMoved: overlap = value }
                        Text { text: overlap.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: preserveFormants
                        onCheckedChanged: preserveFormants = checked
                        Text {
                            text: "Preserve Formants"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: syncTempo
                        onCheckedChanged: syncTempo = checked
                        Text {
                            text: "Sync to Tempo"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    RowLayout {
                        Text { text: "Algorithm"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["WSOLA", "Phase Vocoder", "Rubber Band", "Speex"]
                            currentIndex: 0
                            onActivated: algorithm = currentText
                        }
                    }

                    Item { Layout.fillHeight: true }

                    KsButton {
                        height: 32
                        text: "Apply to Audio"
                        bgcolor: "#E10600"
                        color: "#121212"
                        Layout.fillWidth: true
                        onClicked: {
                            if (AudioBridge) {
                                var totalCents = semitones * 100 + cents;
                                AudioBridge.setPitch(totalCents / 100);
                            }
                        }
                    }
                }
            }
        }
    }
}


