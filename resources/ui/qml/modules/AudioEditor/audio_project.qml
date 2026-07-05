import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        RowLayout {
            Text {
                text: "Project:"
                color: "#aaaaaa"
            }

            TextField {
                id: nameField
                width: 200
                text: projectName
                onTextChanged: modified = true
            }

            Button {
                text: "New"
                width: 50
                onClicked: newProject()
            }

            Button {
                text: "Open"
                width: 50
                onClicked: openProject()
            }

            Button {
                text: "Save"
                width: 50
                onClicked: saveProject()
            }
        }

        Rectangle {
            color: "#181818"
            border.color: "#2a2a2a"
            border.width: 1
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 5

                Text {
                    text: "Project Files"
                    color: "#888888"
                    font.pixelSize: 12
                }

                ListView {
                    id: projectFilesList
                    model: ListModel {
                        ListElement { name: "engine.wav"; type: "Audio" }
                        ListElement { name: "tire_screech.wav"; type: "Audio" }
                        ListElement { name: "turbo.wav"; type: "Audio" }
                    }
                    delegate: Item {
                        width: 450
                        height: 28

                        Rectangle {
                            width: 440
                            height: 24
                            color: "#2a2a2a"
                            radius: 2

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 5

                                Text {
                                    text: model.name
                                    color: "#cccccc"
                                    font.pixelSize: 12
                                }

                                Text {
                                    text: model.type
                                    color: "#666666"
                                    font.pixelSize: 10
                                }

                                Item { Layout.fillWidth: true }

                                Button {
                                    text: "✕"
                                    width: 20
                                    height: 20
                                    onClicked: removeFile(index)
                                }
                            }
                        }
                    }
                }

                Button {
                    text: "Add Files..."
                    width: 100
                    height: 28
                    onClicked: addFiles()
                }
            }
        }

        RowLayout {
            Text {
                text: "Sample Rate: 48000 Hz"
                color: "#666666"
                font.pixelSize: 11
            }

            Text {
                text: "Channels: Stereo"
                color: "#666666"
                font.pixelSize: 11
            }

            Item { Layout.fillWidth: true }

            Text {
                text: modified ? "Modified" : "Saved"
                color: modified ? "#ffaa00" : "#00aa00"
                font.pixelSize: 11
            }
        }
    }

    function newProject() {
        projectName = "Untitled";
        projectPath = "";
        modified = false;
    }

    function openProject() {
        console.log("Open project dialog");
    }

    function saveProject() {
        modified = false;
    }

    function addFiles() {
        console.log("Add files dialog");
    }

    function removeFile(idx) {
        console.log("Remove file: " + idx);
    }
}