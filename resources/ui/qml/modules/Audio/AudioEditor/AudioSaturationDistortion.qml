import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: root
    color: "#121212"

    property string satType: "Soft Clip"
    property real drive: 30
    property real mix: 70
    property real outputGain: 0
    property real bias: 0
    property real lowBoost: 0
    property real highBoost: 0
    property real harmonics: 50
    property int oversample: 1
    property bool asymmetric: false
    property bool dcFilter: true
    property real inputLevel: AudioBridge ? Math.max(AudioBridge.leftPeak, AudioBridge.rightPeak) : 0

    function dbToLinear(db) { return Math.pow(10, db / 20); }

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

                    Text { text: "WAVESHAPER"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        border.color: "#333"
                        border.width: 1
                        radius: 4

                        Canvas {
                            id: satCanvas
                            anchors.fill: parent

                            onPaint: {
                                var ctx = getContext("2d");
                                var w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = "#1a1a1a";
                                ctx.fillRect(0, 0, w, h);

                                ctx.strokeStyle = "#333";
                                ctx.lineWidth = 1;
                                ctx.strokeRect(0.5, 0.5, w-1, h-1);

                                var driveNorm = drive / 100 * 4 + 1;
                                var biasNorm = bias / 100;

                                ctx.strokeStyle = "#444";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, h / 2);
                                ctx.lineTo(w, h / 2);
                                ctx.stroke();
                                ctx.beginPath();
                                ctx.moveTo(w / 2, 0);
                                ctx.lineTo(w / 2, h);
                                ctx.stroke();

                                ctx.fillStyle = "#888";
                                ctx.font = "7px monospace";
                                ctx.textAlign = "center";
                                ctx.fillText("0", w / 2, h / 2 + 10);
                                ctx.fillText("In", w - 8, h / 2 + 10);
                                ctx.fillText("Out", 8, 10);

                                var modes = ["Soft Clip","Hard Clip","Tape","Tube","Fuzz","Rectify","Foldback","Sine Fold"];
                                var modeIdx = modes.indexOf(satType);
                                if (modeIdx < 0) modeIdx = 0;
                                var harmAmount = harmonics / 100;

                                ctx.strokeStyle = "#555";
                                ctx.lineWidth = 1;
                                ctx.beginPath();
                                ctx.moveTo(0, h - h * (1 - biasNorm) * 0.5 - h * 0.25);
                                ctx.lineTo(w, h - h * 0.75 - h * 0.5 * biasNorm);
                                ctx.stroke();

                                ctx.strokeStyle = "#E10600";
                                ctx.lineWidth = 2;
                                ctx.beginPath();

                                for (var x = 0; x <= w; x += 2) {
                                    var input = (x / w) * 2 - 1;
                                    var output = input * (asymmetric ? (1 + biasNorm * 0.3) : 1);

                                    if (modeIdx === 0) {
                                        output = Math.tanh(output * driveNorm);
                                    } else if (modeIdx === 1) {
                                        output = Math.max(-1 / driveNorm, Math.min(1 / driveNorm, output * driveNorm));
                                        if (output > 0.9 / driveNorm) output = 0.9 / driveNorm + (output - 0.9 / driveNorm) * 0.1;
                                        if (output < -0.9 / driveNorm) output = -0.9 / driveNorm + (output + 0.9 / driveNorm) * 0.1;
                                    } else if (modeIdx === 2) {
                                        var shaped = output * driveNorm * 0.5;
                                        var tape = shaped - shaped * shaped * shaped * 0.1 + (shaped * shaped) * 0.05;
                                        output = Math.tanh(tape * 1.5);
                                    } else if (modeIdx === 3) {
                                        var tube = output * driveNorm * 0.7;
                                        tube = tube + tube * tube * 0.1 + tube * tube * tube * 0.05;
                                        var asymmetry = 0.85 - 0.3 * biasNorm;
                                        if (tube > 0) tube = Math.pow(tube, 0.8) * asymmetry;
                                        else tube = -Math.pow(-tube, 0.8) * (2 - asymmetry);
                                        output = Math.tanh(tube * 1.3);
                                    } else if (modeIdx === 4) {
                                        var fuzz = output * driveNorm * 2;
                                        fuzz = Math.tanh(fuzz);
                                        fuzz = fuzz * (1 + Math.abs(fuzz) * 0.5);
                                        output = fuzz / (1 + Math.abs(fuzz) * 0.3);
                                    } else if (modeIdx === 5) {
                                        output = Math.abs(output * driveNorm) * 2 - 1;
                                    } else if (modeIdx === 6) {
                                        var folded = output * driveNorm;
                                        if (folded > 1) folded = 2 - folded;
                                        if (folded < -1) folded = -2 - folded;
                                        output = folded * 0.7;
                                    } else if (modeIdx === 7) {
                                        output = Math.sin(output * Math.PI * driveNorm * 0.5) * 0.8;
                                    }

                                    if (asymmetric && modeIdx < 5) {
                                        var asymMix = 0.1 + biasNorm * 0.3;
                                        if (output > 0) output = output * (1 - asymMix * 0.3);
                                        else output = output * (1 + asymMix * 0.3);
                                    }

                                    output += output * output * output * harmAmount * 0.1;

                                    output = Math.max(-1, Math.min(1, output));

                                    var y = h / 2 - output * (h / 2 - 8);
                                    if (x === 0) ctx.moveTo(x, Math.max(4, Math.min(h - 4, y)));
                                    else ctx.lineTo(x, Math.max(4, Math.min(h - 4, y)));
                                }
                                ctx.stroke();

                                ctx.fillStyle = "#888";
                                ctx.font = "8px monospace";
                                ctx.textAlign = "left";
                                ctx.fillText("Type: " + satType, 4, 12);
                                ctx.textAlign = "right";
                                ctx.fillText("Drive: " + drive.toFixed(0) + "%", w - 4, 12);
                            }

                            Timer {
                                interval: 100
                                running: true
                                repeat: true
                                onTriggered: satCanvas.requestPaint()
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

                    Text { text: "SATURATION / DISTORTION"; color: "#888"; font.bold: true; font.pixelSize: 10 }

                    RowLayout {
                        Text { text: "Type"; color: "#aaa"; font.pixelSize: 9 }
                        ComboBox {
                            Layout.fillWidth: true
                            model: ["Soft Clip", "Hard Clip", "Tape", "Tube", "Fuzz", "Rectify", "Foldback", "Sine Fold"]
                            currentIndex: 0
                            onActivated: satType = currentText
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Drive"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: drive; Layout.fillWidth: true; onMoved: drive = value }
                        Text { text: drive.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Mix"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: mix; Layout.fillWidth: true; onMoved: mix = value }
                        Text { text: mix.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Output"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -12; to: 12; value: outputGain; Layout.fillWidth: true; onMoved: outputGain = value }
                        Text { text: (outputGain >= 0 ? "+" : "") + outputGain.toFixed(1) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "Bias"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -100; to: 100; value: bias; Layout.fillWidth: true; onMoved: bias = value }
                        Text { text: bias.toFixed(0); color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "TONE"; color: "#666"; font.pixelSize: 9 }

                    RowLayout {
                        Text { text: "Low Boost"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -12; to: 12; value: lowBoost; Layout.fillWidth: true; onMoved: lowBoost = value }
                        Text { text: (lowBoost >= 0 ? "+" : "") + lowBoost.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }
                    RowLayout {
                        Text { text: "High Boost"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: -12; to: 12; value: highBoost; Layout.fillWidth: true; onMoved: highBoost = value }
                        Text { text: (highBoost >= 0 ? "+" : "") + highBoost.toFixed(0) + "dB"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    RowLayout {
                        Text { text: "Harmonics"; color: "#aaa"; font.pixelSize: 9 }
                        Slider { from: 0; to: 100; value: harmonics; Layout.fillWidth: true; onMoved: harmonics = value }
                        Text { text: harmonics.toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 10 }
                    }

                    Text { text: "OVERSAMPLE"; color: "#666"; font.pixelSize: 9 }
                    Row {
                        spacing: 4
                        AppButton {
                            height: 24; text: "1x"
                            bgcolor: oversample === 1 ? "#E10600" : "#3e3e42"
                            color: oversample === 1 ? "#121212" : "#ffffff"
                            onClicked: oversample = 1
                        }
                        AppButton {
                            height: 24; text: "2x"
                            bgcolor: oversample === 2 ? "#E10600" : "#3e3e42"
                            color: oversample === 2 ? "#121212" : "#ffffff"
                            onClicked: oversample = 2
                        }
                        AppButton {
                            height: 24; text: "4x"
                            bgcolor: oversample === 4 ? "#E10600" : "#3e3e42"
                            color: oversample === 4 ? "#121212" : "#ffffff"
                            onClicked: oversample = 4
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    CheckBox {
                        checked: asymmetric
                        onCheckedChanged: asymmetric = checked
                        Text {
                            text: "Asymmetric"
                            color: "#aaa"; font.pixelSize: 10
                            anchors.left: parent.right; anchors.leftMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    CheckBox {
                        checked: dcFilter
                        onCheckedChanged: dcFilter = checked
                        Text {
                            text: "DC Filter"
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


