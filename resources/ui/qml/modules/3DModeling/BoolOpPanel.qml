import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0
import ksEditor.BoolOp 1.0

Rectangle {
    id: boolOpPanel
    visible: false
    width: 260
    height: 400
    color: "#1e1e1e"
    border.color: "#E10600"
    border.width: 1
    radius: 6

    property string activeOperation: "Union"
    property int selectedTargetId: -1
    property var meshList: []

    signal closePanel()

    onVisibleChanged: {
        if (visible) refreshMeshList()
    }

    function refreshMeshList() {
        var objects = Modeler.getMeshObjects()
        meshList = objects
        if (objects.length > 0 && selectedTargetId < 0) {
            selectedTargetId = objects[0].id
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Text {
            text: "BOOLEAN OPERATIONS"
            color: "#E10600"
            font.pixelSize: 13
            font.bold: true
        }

        Text {
            text: "Operation:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        ButtonGroup { id: opGroup }

        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true

            Repeater {
                model: ["Union (A ∪ B)", "Difference (A - B)", "Intersection (A ∩ B)", "Symmetric Diff (A △ B)"]
                AppButton {
                    height: 26
                    text: modelData
                    checkable: true
                    autoExclusive: true
                    Layout.fillWidth: true
                    font.pixelSize: 11
                    bgcolor: checked ? "#E10600" : "#3e3e42"
                    color: checked ? "#121212" : "#ffffff"
                    ButtonGroup.group: opGroup
                    onClicked: activeOperation = modelData.split(" ")[0]
                }
            }
        }

        Item { height: 4 }

        Text {
            text: "Target Mesh:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: Math.min(meshList.length * 26 + 4, 120)
            color: "#2a2a2e"
            radius: 4
            clip: true

            ListView {
                anchors.fill: parent
                anchors.margins: 2
                model: meshList
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 24
                    color: modelData.id === selectedTargetId ? "#E10600" : "transparent"
                    radius: 2

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.name + " (ID: " + modelData.id + ")"
                        color: modelData.id === selectedTargetId ? "#121212" : "#ccc"
                        font.pixelSize: 11
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: selectedTargetId = modelData.id
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: meshList.length === 0
                text: "No other meshes in scene"
                color: "#666"
                font.pixelSize: 11
            }
        }

        Item { height: 4 }

        Text {
            text: "Status: " + (BoolOp.available ? "CGAL Ready" : "CGAL Unavailable")
            color: BoolOp.available ? "#4CAF50" : "#E10600"
            font.pixelSize: 10
        }

        Item { height: 4 }

        RowLayout {
            spacing: 8
            Layout.fillWidth: true

            AppButton {
                height: 32
                text: "Apply"
                bgcolor: "#E10600"
                color: "#121212"
                Layout.fillWidth: true
                font.bold: true
                enabled: selectedTargetId >= 0

                onClicked: {
                    var opMap = {
                        "Union": 0,
                        "Difference": 1,
                        "Intersection": 2,
                        "Symmetric": 3
                    }
                    var opIdx = opMap[activeOperation] || 0
                    Modeler.booleanOperation(opIdx, selectedTargetId)
                    closePanel()
                }
            }

            AppButton {
                height: 32
                text: "Cancel"
                bgcolor: "transparent"
                color: "#ffffff"
                Layout.fillWidth: true
                onClicked: closePanel()
            }
        }
    }
}
