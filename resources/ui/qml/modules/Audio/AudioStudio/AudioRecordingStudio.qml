import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform 1.1
import ksEditor.Audio 1.0
import "../../widgets"

Item {
    id: recordingStudio
    anchors.fill: parent

    readonly property color cAccent: "#E10600"
    readonly property color cMuted: "#666666"
    readonly property color cText: "#cccccc"
    readonly property color cBorder: "#333333"

    property bool isRecording: false
    property real inputLevel: 0
    property real recordTime: 0

    Timer {
        id: recordTimer
        interval: 100; repeat: true
        onTriggered: { if (isRecording) recordTime += 0.1 }
    }

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        Rectangle {
            height: 32; color: "#252526"; Layout.fillWidth: true
            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "RECORDING STUDIO"; color: cAccent; font.pixelSize: 11; font.bold: true
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            Rectangle {
                Layout.preferredWidth: 200; Layout.fillHeight: true
                color: "#1e1e1e"; border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 10; spacing: 8

                    Text { text: "INPUT SETTINGS"; color: cMuted; font.pixelSize: 9; font.bold: true }

                    Text { text: "Input Device:"; color: "#888"; font.pixelSize: 9 }
                    ComboBox { Layout.fillWidth: true; height: 22; model: ["Default Input", "Microphone", "Line In", "SPDIF"]; font.pixelSize: 9 }

                    Text { text: "Sample Rate:"; color: "#888"; font.pixelSize: 9 }
                    ComboBox { Layout.fillWidth: true; height: 22; model: ["44100 Hz", "48000 Hz", "96000 Hz"]; font.pixelSize: 9 }

                    Text { text: "Bit Depth:"; color: "#888"; font.pixelSize: 9 }
                    ComboBox { Layout.fillWidth: true; height: 22; model: ["16-bit", "24-bit", "32-bit float"]; font.pixelSize: 9 }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    Text { text: "INPUT LEVEL"; color: cMuted; font.pixelSize: 9; font.bold: true }

                    Rectangle { Layout.fillWidth: true; height: 20; color: "#0e0e0e"; radius: 3
                        Rectangle { height: parent.height; width: parent.width * Math.min(1, inputLevel); color: isRecording ? cAccent : "#3a3a3a"; radius: 3 }
                    }
                    Text { text: "Gain:"; color: "#888"; font.pixelSize: 9 }
                    Slider { Layout.fillWidth: true; from: 0; to: 200; value: 100; height: 16 }

                    Item { Layout.fillHeight: true }

                    AppButton {
                        height: 36; text: isRecording ? "\u25A0  STOP RECORDING" : "\u25CF  START RECORDING"
                        bgcolor: isRecording ? "#ff4c4c" : cAccent; color: "#121212"; font.bold: true
                        onClicked: {
                            isRecording = !isRecording
                            if (isRecording) {
                                recordTime = 0
                                recordTimer.start()
                                if (AudioBridge) AudioBridge.startRecording()
                            } else {
                                recordTimer.stop()
                                if (AudioBridge) AudioBridge.stopRecording()
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: "#1a1a1a"; border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16; spacing: 10

                    Text { text: "RECORDINGS"; color: cMuted; font.pixelSize: 10; font.bold: true }

                    Text { text: "Duration: " + Math.floor(recordTime / 60).toFixed(0).padStart(2, "0") + ":" + (recordTime % 60).toFixed(1).padStart(4, "0"); color: cAccent; font.pixelSize: 24; font.bold: true; font.family: "monospace" }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: 0
                        delegate: Rectangle {
                            width: ListView.view.width; height: 32
                            color: "#161616"; radius: 2
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 4
                                Text { text: "Recording"; color: cText; font.pixelSize: 10 }
                                Item { Layout.fillWidth: true }
                                AppButton { text: "Play"; height: 22; bgcolor: "transparent"; color: cText; font.pixelSize: 8 }
                                AppButton { text: "Rename"; height: 22; bgcolor: "transparent"; color: cText; font.pixelSize: 8 }
                                AppButton { text: "Delete"; height: 22; bgcolor: "transparent"; color: "#ff6666"; font.pixelSize: 8 }
                            }
                        }
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        Text {
                            anchors.centerIn: parent
                            text: "No recordings yet.\nPress Record to begin."; color: cMuted; font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            visible: parent.count === 0
                        }
                    }
                }
            }
        }
    }
}
