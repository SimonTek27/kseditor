import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform 1.1
import ksEditor.AudioModule 1.0
import "../../../widgets"

Item {
    id: batchProcessor
    anchors.fill: parent

    readonly property color cAccent: "#E10600"
    readonly property color cMuted: "#666666"
    readonly property color cText: "#cccccc"
    readonly property color cBorder: "#333333"

    property var fileList: []

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        Rectangle {
            height: 32; color: "#252526"; Layout.fillWidth: true
            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "BATCH PROCESSOR"; color: cAccent; font.pixelSize: 11; font.bold: true
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: "#1a1a1a"; border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16; spacing: 10

                    RowLayout {
                        spacing: 8
                        AppButton { text: "Add Files"; height: 28; bgcolor: "#3e3e42"; color: cText; onClicked: addFilesDialog.open(); font.pixelSize: 10 }
                        AppButton { text: "Clear List"; height: 28; bgcolor: "transparent"; color: cText; font.pixelSize: 10 }
                        Item { Layout.fillWidth: true }
                        Text { text: fileList.length + " files"; color: cMuted; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: fileList
                        delegate: Rectangle {
                            width: ListView.view.width; height: 28; color: index % 2 === 0 ? "#161616" : "#121212"; radius: 2
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 4; spacing: 8
                                Text { text: (index + 1) + "."; color: cMuted; font.pixelSize: 9; width: 24 }
                                Text { text: modelData; color: cText; font.pixelSize: 10; Layout.fillWidth: true; elide: Text.ElideMiddle }
                                Text { text: "44100 Hz / 16-bit / Stereo"; color: cMuted; font.pixelSize: 8 }
                                Rectangle { width: 14; height: 14; radius: 2; color: "#3a3a3a"
                                    Text { anchors.centerIn: parent; text: "X"; color: cMuted; font.pixelSize: 8 }
                                    MouseArea { anchors.fill: parent; onClicked: fileList.splice(index, 1) } }
                            }
                        }
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        Text {
                            anchors.centerIn: parent
                            text: "No files added.\nClick 'Add Files' to begin."; color: cMuted; font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            visible: parent.count === 0
                        }
                    }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    GridLayout {
                        columns: 4; columnSpacing: 8; rowSpacing: 6; Layout.fillWidth: true

                        Text { text: "Output Format:"; color: "#888"; font.pixelSize: 9 }
                        ComboBox { Layout.fillWidth: true; height: 22; model: ["WAV", "OGG", "MP3", "FLAC"]; font.pixelSize: 9 }
                        Text { text: "Sample Rate:"; color: "#888"; font.pixelSize: 9 }
                        ComboBox { Layout.fillWidth: true; height: 22; model: ["44100 Hz", "22050 Hz", "48000 Hz", "96000 Hz"]; font.pixelSize: 9 }

                        Text { text: "Bit Depth:"; color: "#888"; font.pixelSize: 9 }
                        ComboBox { Layout.fillWidth: true; height: 22; model: ["16-bit", "24-bit", "32-bit float"]; font.pixelSize: 9 }
                        Text { text: "Channels:"; color: "#888"; font.pixelSize: 9 }
                        ComboBox { Layout.fillWidth: true; height: 22; model: ["Mono", "Stereo", "Surround"]; font.pixelSize: 9 }
                    }

                    RowLayout {
                        spacing: 8
                        CheckBox { text: "Normalize"; font.pixelSize: 9 }
                        CheckBox { text: "Remove Silence"; font.pixelSize: 9; checked: true }
                        CheckBox { text: "Apply EQ Preset"; font.pixelSize: 9 }
                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        spacing: 8; Layout.fillWidth: true
                        AppButton { text: "Run Batch"; height: 34; bgcolor: cAccent; color: "#121212"; font.bold: true; Layout.fillWidth: true; font.pixelSize: 11; enabled: fileList.length > 0 }
                        AppButton { text: "Output Folder..."; height: 34; bgcolor: "#3e3e42"; color: cText; font.pixelSize: 10 }
                    }
                }
            }
        }
    }

    FileDialog {
        id: addFilesDialog
        title: "Select Audio Files"
        nameFilters: ["Audio files (*.wav *.mp3 *.ogg *.flac *.aiff)", "All files (*)"]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            for (var i = 0; i < files.length; ++i) {
                var path = files[i].toString().replace("file:///", "")
                fileList.push(path.split("/").pop())
            }
            fileList = fileList
        }
    }
}
