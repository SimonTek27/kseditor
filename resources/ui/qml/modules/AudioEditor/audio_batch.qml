import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: audioBatch
    width: 550
    height: 450
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property var files: []
    property bool isProcessing: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        Text {
            text: "Batch Converter"
            color: "#ffffff"
            font.pixelSize: 16
            font.bold: true
        }

        RowLayout {
            Text {
                text: "Input Format:"
                color: "#aaaaaa"
            }

            ComboBox {
                width: 80
                model: ["WAV", "MP3", "OGG", "FLAC", "AIFF", "Any"]
                currentIndex: 5
            }

            Text {
                text: "Output Format:"
                color: "#aaaaaa"
                marginLeft: 20
            }

            ComboBox {
                width: 80
                model: ["WAV", "MP3", "OGG", "FLAC"]
                currentIndex: 0
            }
        }

        RowLayout {
            Text {
                text: "Output Folder:"
                color: "#aaaaaa"
            }

            TextField {
                id: outputFolder
                width: 280
                placeholderText: "Select output folder..."
            }

            Button {
                text: "Browse"
                width: 60
                onClicked: console.log("Browse folder")
            }
        }

        Rectangle {
            color: "#181818"
            border.color: "#2a2a2a"
            border.width: 1
            Layout.fillWidth: true
            Layout.minimumHeight: 200

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 5

                RowLayout {
                    Button {
                        text: "Add Files"
                        width: 80
                        onClicked: addFiles()
                    }

                    Button {
                        text: "Add Folder"
                        width: 80
                        onClicked: addFolder()
                    }

                    Button {
                        text: "Clear"
                        width: 60
                        onClicked: files = []
                    }

                    Item { Layout.fillWidth: true }

                    CheckBox {
                        text: "Include subfolders"
                    }
                }

                ListView {
                    id: fileList
                    model: files
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    delegate: Item {
                        width: 500
                        height: 24

                        Rectangle {
                            width: 490
                            height: 20
                            color: "#2a2a2a"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 5

                                Text {
                                    text: modelData
                                    color: "#cccccc"
                                    font.pixelSize: 11
                                }

                                Item { Layout.fillWidth: true }

                                Button {
                                    text: "✕"
                                    width: 20
                                    height: 16
                                    onClicked: removeFile(index)
                                }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            CheckBox {
                text: "Overwrite existing"
            }

            CheckBox {
                text: "Delete source after conversion"
            }
        }

        RowLayout {
            CheckBox {
                text: "Preserve folder structure"
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Text {
                text: "Files: " + files.length
                color: "#888888"
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                width: 80
                onClicked: isProcessing = false
            }

            Button {
                text: isProcessing ? "Stop" : "Convert"
                width: 100
                onClicked: {
                    isProcessing = !isProcessing;
                    if (isProcessing) startConversion();
                }
            }
        }
    }

    function addFiles() {
        files.push("sample1.wav");
        files.push("sample2.wav");
        files = files;
    }

    function addFolder() {
        files.push("folder1/sample.wav");
        files = files;
    }

    function removeFile(idx) {
        files.splice(idx, 1);
        files = files;
    }

    function startConversion() {
        console.log("Converting " + files.length + " files...");
    }
}