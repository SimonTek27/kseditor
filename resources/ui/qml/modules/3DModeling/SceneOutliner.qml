import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: outliner
    anchors.fill: parent
    color: "transparent"

    signal closeRequested()

    property alias sceneModel: Modeler.sceneModel
    property int objectCount: sceneModel.count
    property int selectedCount: 0

    function refreshSelection() {
        var count = 0
        for (var i = 0; i < sceneModel.count; ++i) {
            var idx = outlinerList.model.index(i, 0)
            if (sceneModel.data(idx, 256 + 7))
                count++
        }
        selectedCount = count
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 32
            color: "#252526"
            Layout.fillWidth: true

            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "SCENE OUTLINER"
                color: "#E10600"
                font.pixelSize: 11
                font.bold: true
            }

            Text {
                anchors { right: parent.right; rightMargin: 30; verticalCenter: parent.verticalCenter }
                text: objectCount + " objects"
                color: "#666"
                font.pixelSize: 9
            }

            Rectangle {
                anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                width: 18; height: 18; radius: 2; color: "#E10600"
                Text { anchors.centerIn: parent; text: "X"; color: "#fff"; font.pixelSize: 10; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: closeRequested() }
            }
        }

        Rectangle {
            height: 24
            color: "#181818"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4

                Text {
                    text: "Name"
                    color: "#888"
                    font.pixelSize: 9
                    font.bold: true
                    Layout.preferredWidth: 140
                    leftPadding: 4
                }
                Text {
                    text: "Type"
                    color: "#888"
                    font.pixelSize: 9
                    font.bold: true
                    Layout.preferredWidth: 50
                }
                Text {
                    text: "Polys"
                    color: "#888"
                    font.pixelSize: 9
                    font.bold: true
                    Layout.preferredWidth: 50
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        ListView {
            id: outlinerList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: sceneModel

            delegate: Rectangle {
                height: 24
                width: parent.width
                color: {
                    if (model.objectSelected) return "#224466"
                    if (mouseArea.containsMouse) return "#1a1a1a"
                    return "transparent"
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 2
                    spacing: 2

                    Rectangle {
                        width: 14; height: 14; radius: 2
                        color: model.objectVisible ? "#4CAF50" : "#555"
                        border.color: model.objectVisible ? "#66bb6a" : "#666"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: model.objectVisible ? "\u2713" : ""
                            color: "#fff"
                            font.pixelSize: 8
                            font.bold: true
                        }
                    }

                    Rectangle {
                        width: 12; height: 12; radius: 2
                        color: {
                            switch (model.objectType) {
                                case "Mesh": return "#E10600"
                                case "Node": return "#ff6600"
                                case "Light": return "#ffdd00"
                                case "Camera": return "#00ccff"
                                case "Bone": return "#aa66ff"
                                default: return "#888"
                            }
                        }
                    }

                    Text {
                        text: model.objectName
                        color: model.objectSelected ? "#ffffff" : "#cccccc"
                        font.pixelSize: 10
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        leftPadding: 2
                    }

                    Text {
                        text: model.objectType
                        color: "#666"
                        font.pixelSize: 8
                        Layout.preferredWidth: 44
                    }

                    Text {
                        text: model.hasMesh ? model.triangleCount : "-"
                        color: "#666"
                        font.pixelSize: 8
                        Layout.preferredWidth: 44
                        horizontalAlignment: Text.AlignRight
                        rightPadding: 4
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (Modeler) Modeler.selectObject(model.objectId)
                    }
                }
            }

            ScrollBar.vertical: ScrollBar { active: true }
        }

        Rectangle {
            height: 24
            color: "#181818"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4

                AppButton {
                    text: "Select All"
                    height: 20
                    font.pixelSize: 9
                    bgcolor: "#3e3e42"
                    color: "#ffffff"
                    onClicked: {
                        for (var i = 0; i < sceneModel.count; ++i) {
                            var idx = sceneModel.index(i, 0)
                            var id = sceneModel.data(idx, 258)
                            if (id > 0) Modeler.selectObject(id)
                        }
                    }
                }
                AppButton {
                    text: "Deselect"
                    height: 20
                    font.pixelSize: 9
                    bgcolor: "#3e3e42"
                    color: "#ffffff"
                    onClicked: { if (Modeler) Modeler.deselectAll() }
                }
                AppButton {
                    text: "Delete"
                    height: 20
                    font.pixelSize: 9
                    bgcolor: "#E10600"
                    color: "#121212"
                    enabled: Modeler && Modeler.hasSelection
                    onClicked: { if (Modeler) Modeler.deleteSelected() }
                }
                Item { Layout.fillWidth: true }
            }
        }
    }
}


