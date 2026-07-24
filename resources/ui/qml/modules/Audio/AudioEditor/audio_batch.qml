import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ksEditor.Audio 1.0

Rectangle {
    id: audioBatch
    width: 550
    height: 450
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property var files: []
    property bool isProcessing: false
    property string statusText: ""

    FileDialog {
        id: fileDialog
        title: "Select Audio Files"
        nameFilters: ["Audio files (*.wav *.mp3 *.ogg *.flac *.aiff)", "All files (*)"]
        selectMultiple: true
        onAccepted: {
            for (var i = 0; i < selectedFiles.length; i++) {
                var path = selectedFiles[i].toString().replace("file:///", "")
                if (files.indexOf(path) < 0) files.push(path)
            }
            files = files.slice()
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Select Folder"
        onAccepted: {
            var folder = selectedFolder.toString().replace("file:///", "")
            files.push(folder + "/*")
            files = files.slice()
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 10

        Text { text: "Batch Converter"; color: "#ffffff"; font.pixelSize: 16; font.bold: true }

        RowLayout {
            Text { text: "Input Format:"; color: "#aaaaaa" }
            ComboBox { width: 80; model: ["WAV", "MP3", "OGG", "FLAC", "AIFF", "Any"]; currentIndex: 5 }
            Text { text: "Output Format:"; color: "#aaaaaa"; marginLeft: 20 }
            ComboBox { id: outFormat; width: 80; model: ["WAV", "MP3", "OGG", "FLAC"]; currentIndex: 0 }
        }

        RowLayout {
            Text { text: "Output Folder:"; color: "#aaaaaa" }
            TextField { id: outputFolder; Layout.fillWidth: true; placeholderText: "Select output folder..." }
            Button { text: "Browse"; width: 60; onClicked: folderDialog.open() }
        }

        Rectangle {
            color: "#181818"; border.color: "#2a2a2a"; border.width: 1
            Layout.fillWidth: true; Layout.minimumHeight: 200

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 5
                RowLayout {
                    Button { text: "Add Files"; width: 80; onClicked: fileDialog.open() }
                    Button { text: "Add Folder"; width: 80; onClicked: folderDialog.open() }
                    Button { text: "Clear"; width: 60; onClicked: files = [] }
                    Item { Layout.fillWidth: true }
                    CheckBox { text: "Include subfolders" }
                }

                ListView {
                    id: fileList; model: files; Layout.fillWidth: true; Layout.fillHeight: true
                    delegate: Item {
                        width: 500; height: 24
                        Rectangle {
                            width: 490; height: 20; color: "#2a2a2a"
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 5
                                Text { text: modelData; color: "#cccccc"; font.pixelSize: 11; elide: Text.ElideMiddle; Layout.fillWidth: true }
                                Button { text: "\u2715"; width: 20; height: 16; font.pixelSize: 8
                                    onClicked: { files.splice(index, 1); files = files.slice() } }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            CheckBox { text: "Overwrite existing" }
            CheckBox { text: "Delete source after conversion" }
        }

        Text { text: statusText; color: "#888888"; font.pixelSize: 10; visible: statusText !== "" }

        Item { Layout.fillHeight: true }

        RowLayout {
            Text { text: "Files: " + files.length; color: "#888888" }
            Item { Layout.fillWidth: true }
            Button { text: "Cancel"; width: 80; onClicked: { isProcessing = false; statusText = "" } }
            Button {
                text: isProcessing ? "Stop" : "Convert"; width: 100
                onClicked: { isProcessing = !isProcessing; if (isProcessing) startConversion() }
            }
        }
    }

    function startConversion() {
        var outFmt = outFormat.currentText.toLowerCase()
        var converted = 0
        for (var i = 0; i < files.length; i++) {
            var inp = files[i]
            if (inp.endsWith("*")) continue
            var outPath = (outputFolder.text || "converted") + "/" + inp.split('/').pop().split('\\').pop()
            outPath = outPath.substring(0, outPath.lastIndexOf('.')) + "." + outFmt
            if (AudioBridge) AudioBridge.convertFormat(inp, outPath, outFormat.currentIndex === 0 ? 100 : 75)
            converted++
        }
        statusText = "Converted " + converted + " file(s) to " + outFmt
        isProcessing = false
    }
}
