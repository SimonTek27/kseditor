import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform 1.1
import ksEditor.AudioModule 1.0
import "../../../widgets"

Item {
    id: exportPanel
    anchors.fill: parent

    readonly property color cAccent: "#E10600"
    readonly property color cMuted: "#666666"
    readonly property color cText: "#cccccc"
    readonly property color cBorder: "#333333"

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        Rectangle {
            height: 32; color: "#252526"; Layout.fillWidth: true
            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "EXPORT"; color: cAccent; font.pixelSize: 11; font.bold: true
            }
        }

        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: "#1a1a1a"; border.color: cBorder; border.width: 1

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 24; spacing: 12

                Text { text: "EXPORT PROJECT"; color: cText; font.pixelSize: 16; font.bold: true }

                Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                Text { text: "Export Format"; color: "#888"; font.pixelSize: 10; font.bold: true }

                GridLayout {
                    columns: 2; columnSpacing: 12; rowSpacing: 8

                    Text { text: "Format:"; color: "#888"; font.pixelSize: 9 }
                    ComboBox { Layout.fillWidth: true; height: 22; model: ["WAV (*.wav)", "OGG (*.ogg)", "MP3 (*.mp3)", "FLAC (*.flac)"]; font.pixelSize: 9 }

                    Text { text: "Sample Rate:"; color: "#888"; font.pixelSize: 9 }
                    ComboBox { Layout.fillWidth: true; height: 22; model: ["44100 Hz", "22050 Hz", "48000 Hz", "96000 Hz"]; font.pixelSize: 9 }

                    Text { text: "Bit Depth:"; color: "#888"; font.pixelSize: 9 }
                    ComboBox { Layout.fillWidth: true; height: 22; model: ["16-bit", "24-bit", "32-bit float"]; font.pixelSize: 9 }

                    Text { text: "Quality:"; color: "#888"; font.pixelSize: 9 }
                    Slider { from: 0; to: 100; value: 80; Layout.fillWidth: true; height: 16 }
                }

                Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                Text { text: "Export Scope"; color: "#888"; font.pixelSize: 10; font.bold: true }

                RowLayout {
                    spacing: 16
                    RadioButton { text: "Entire Project"; font.pixelSize: 9; checked: true }
                    RadioButton { text: "Selected Tracks"; font.pixelSize: 9 }
                    RadioButton { text: "Selection Only"; font.pixelSize: 9 }
                }

                Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                Text { text: "Options"; color: "#888"; font.pixelSize: 10; font.bold: true }

                RowLayout {
                    spacing: 16
                    CheckBox { text: "Normalize"; font.pixelSize: 9; checked: true }
                    CheckBox { text: "Dither"; font.pixelSize: 9 }
                    CheckBox { text: "Write Metadata"; font.pixelSize: 9; checked: true }
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    spacing: 12; Layout.fillWidth: true

                    AppButton { text: "Export..."; height: 36; bgcolor: cAccent; color: "#121212"; font.bold: true; Layout.fillWidth: true; font.pixelSize: 11
                        onClicked: exportFileDialog.open() }

                    AppButton { text: "Export All Banks"; height: 36; bgcolor: "#3e3e42"; color: cText; Layout.fillWidth: true; font.pixelSize: 11
                        onClicked: AudioModule.onBuildBanks() }

                    AppButton { text: "Cancel"; height: 36; bgcolor: "transparent"; color: cMuted; font.pixelSize: 11 }
                }
            }
        }
    }

    FileDialog {
        id: exportFileDialog
        title: "Export Audio"
        nameFilters: ["WAV (*.wav)", "OGG (*.ogg)", "MP3 (*.mp3)", "FLAC (*.flac)"]
        onAccepted: {
            var path = file.toString().replace("file:///", "")
            AudioModule.onExportAsset()
        }
    }
}
