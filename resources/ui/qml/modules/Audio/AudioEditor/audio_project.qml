import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ksEditor.Audio 1.0

Rectangle {
    id: audioProject
    width: 500
    height: 400
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property string projectName: "Untitled"
    property string projectPath: ""
    property bool modified: false
    property var fileList: []

    FileDialog {
        id: importDialog
        title: "Import Audio File"
        nameFilters: ["Audio files (*.wav *.mp3 *.ogg *.flac *.aiff)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            if (AudioBridge) AudioBridge.loadAudio(path)
            fileList.push({ name: path.split('/').pop().split('\\').pop(), path: path })
            fileList = fileList.slice()
            modified = true
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save Project As..."
        onAccepted: {
            projectPath = selectedFile.toString().replace("file:///", "")
            modified = false
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 10

        RowLayout {
            Text { text: "Project:"; color: "#aaaaaa" }
            TextField { id: nameField; Layout.fillWidth: true; text: projectName; onTextChanged: modified = true }
            Button { text: "New"; onClicked: { projectName = "Untitled"; projectPath = ""; fileList = []; modified = false } }
            Button { text: "Open"; onClicked: saveDialog.open() }
            Button { text: "Save"; onClicked: { if (projectPath) modified = false; else saveDialog.open() } }
        }

        Rectangle {
            color: "#181818"; border.color: "#2a2a2a"; border.width: 1
            Layout.fillWidth: true; Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 5
                Text { text: "Project Files (" + fileList.length + ")"; color: "#888888"; font.pixelSize: 12 }

                ListView {
                    id: projectFilesList
                    model: fileList
                    delegate: Item {
                        width: 450; height: 28
                        Rectangle {
                            width: 440; height: 24; color: "#2a2a2a"; radius: 2
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 5
                                Text { text: modelData.name; color: "#cccccc"; font.pixelSize: 12 }
                                Text { text: "Audio"; color: "#666666"; font.pixelSize: 10 }
                                Item { Layout.fillWidth: true }
                                Button { text: "\u25B6"; width: 20; height: 20; font.pixelSize: 8
                                    onClicked: { if (AudioBridge) { AudioBridge.loadAudio(modelData.path); AudioBridge.play() } } }
                                Button { text: "\u2715"; width: 20; height: 20; font.pixelSize: 8
                                    onClicked: { fileList.splice(index, 1); fileList = fileList.slice(); modified = true } }
                            }
                        }
                    }
                }

                Button { text: "Add Files..."; width: 100; height: 28; onClicked: importDialog.open() }
            }
        }

        RowLayout {
            Text { text: AudioBridge ? AudioBridge.getSampleRate() + " Hz" : ""; color: "#666666"; font.pixelSize: 11 }
            Text { text: AudioBridge ? (AudioBridge.getChannelCount() > 1 ? "Stereo" : "Mono") : ""; color: "#666666"; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            Text { text: modified ? "Modified" : "Saved"; color: modified ? "#ffaa00" : "#00aa00"; font.pixelSize: 11 }
        }
    }
}
