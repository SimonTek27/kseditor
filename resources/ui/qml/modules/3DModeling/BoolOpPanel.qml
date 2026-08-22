import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: boolOpPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property int selectedTargetId: -1
    property var meshList: []
    property var stackList: []
    property int newOpType: 0

    signal stackChanged()

    function refreshMeshList() {
        var objects = Modeler ? Modeler.getMeshObjects() : []
        meshList = []
        for (var i = 0; i < objects.length; ++i) {
            if (objects[i].id !== objectId) meshList.push(objects[i])
        }
        var stillValid = false
        for (var j = 0; j < meshList.length; ++j)
            if (meshList[j].id === selectedTargetId) { stillValid = true; break }
        if (!stillValid && meshList.length > 0)
            selectedTargetId = meshList[0].id
    }

    function refreshStack() {
        if (objectId >= 0)
            stackList = Modeler.booleanStack(objectId)
        else
            stackList = []
    }

    function refresh() {
        refreshMeshList()
        refreshStack()
    }

    function addOperation() {
        if (objectId >= 0 && selectedTargetId >= 0)
            Modeler.booleanAdd(objectId, newOpType, selectedTargetId)
    }

    function moveOp(index, delta) {
        if (objectId < 0) return
        var to = index + delta
        if (to < 0 || to >= stackList.length) return
        Modeler.booleanMove(objectId, index, to)
    }

    function removeOp(index) {
        if (objectId >= 0) Modeler.booleanRemove(objectId, index)
    }

    function cycleOp(index) {
        if (objectId < 0) return
        var cur = stackList[index].operation
        Modeler.booleanSetOperation(objectId, index, (cur + 1) % 4)
    }

    function toggleOp(index, enabled) {
        if (objectId >= 0) Modeler.booleanSetEnabled(objectId, index, enabled)
    }

    onVisibleChanged: {
        if (visible) refresh()
    }

    Connections {
        target: Modeler
        function onBooleanStackChanged(id) { if (id === boolOpPanel.objectId) boolOpPanel.refreshStack() }
        function onSelectionChanged() { boolOpPanel.refresh() }
        function onSceneChanged() { boolOpPanel.refreshMeshList() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "BOOLEAN STACK"
                color: "#E10600"
                font.pixelSize: 13
                font.bold: true
                Layout.fillWidth: true
            }

            AppButton {
                text: "X"
                height: 24
                width: 26
                bgcolor: "#3e3e42"
                color: "#ffffff"
                font.pixelSize: 10
                font.bold: true
                onClicked: closePanel()
            }
        }

        Text {
            text: "Non-destructive boolean (A = selected object, B = operand).\nEditable stack with live viewport preview \u2014 Apply bakes to static mesh."
            color: "#888"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Text {
            text: "New operation:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: 4
            rowSpacing: 4

            AppButton { text: "Union";  height: 26; checkable: true; autoExclusive: true; bgcolor: newOpType === 0 ? "#E10600" : "#3e3e42"; color: newOpType === 0 ? "#121212" : "#fff"; font.pixelSize: 10; checked: newOpType === 0; onClicked: newOpType = 0 }
            AppButton { text: "Diff";   height: 26; checkable: true; autoExclusive: true; bgcolor: newOpType === 1 ? "#E10600" : "#3e3e42"; color: newOpType === 1 ? "#121212" : "#fff"; font.pixelSize: 10; checked: newOpType === 1; onClicked: newOpType = 1 }
            AppButton { text: "Inter";  height: 26; checkable: true; autoExclusive: true; bgcolor: newOpType === 2 ? "#E10600" : "#3e3e42"; color: newOpType === 2 ? "#121212" : "#fff"; font.pixelSize: 10; checked: newOpType === 2; onClicked: newOpType = 2 }
            AppButton { text: "Xor";    height: 26; checkable: true; autoExclusive: true; bgcolor: newOpType === 3 ? "#E10600" : "#3e3e42"; color: newOpType === 3 ? "#121212" : "#fff"; font.pixelSize: 10; checked: newOpType === 3; onClicked: newOpType = 3 }
        }

        Text {
            text: "Operand B (other mesh):"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: Math.min(meshList.length * 24 + 4, 90)
            color: "#2a2a2e"
            radius: 4
            clip: true

            ListView {
                anchors.fill: parent
                anchors.margins: 2
                model: meshList
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 22
                    color: modelData.id === selectedTargetId ? "#E10600" : "transparent"
                    radius: 2

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.name + " (ID: " + modelData.id + ")"
                        color: modelData.id === selectedTargetId ? "#121212" : "#ccc"
                        font.pixelSize: 10
                        elide: Text.ElideRight
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
                text: objectId < 0 ? "Select a base object first" : "No other meshes in scene"
                color: "#666"
                font.pixelSize: 11
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                Layout.fillWidth: true
                height: 28
                text: "Add Operation"
                bgcolor: "#E10600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 10
                enabled: objectId >= 0 && selectedTargetId >= 0
                onClicked: addOperation()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text {
            text: "Stack (" + stackList.length + "):"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#2a2a2e"
            radius: 4
            clip: true

            ListView {
                id: stackView
                anchors.fill: parent
                anchors.margins: 2
                clip: true
                spacing: 2
                model: stackList

                delegate: Rectangle {
                    id: row
                    width: stackView.width
                    height: 30
                    radius: 3
                    color: modelData.enabled ? "#333" : "#28282a"
                    border.color: "#444"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 4

                        AppButton {
                            width: 86
                            height: 22
                            text: modelData.operation === 0 ? "Union" :
                                  modelData.operation === 1 ? "Diff" :
                                  modelData.operation === 2 ? "Inter" : "Xor"
                            bgcolor: modelData.enabled ? "#E10600" : "#3e3e42"
                            color: modelData.enabled ? "#121212" : "#888"
                            font.pixelSize: 9
                            font.bold: true
                            onClicked: cycleOp(index)
                            ToolTip.visible: hovered
                            ToolTip.text: "Click to cycle operation type"
                        }

                        Text {
                            text: modelData.operandName
                            color: modelData.enabled ? "#ddd" : "#777"
                            font.pixelSize: 9
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        AppButton {
                            width: 18
                            height: 20
                            text: "\u25B2"
                            bgcolor: "#3e3e42"
                            color: "#fff"
                            font.pixelSize: 7
                            onClicked: moveOp(index, -1)
                        }
                        AppButton {
                            width: 18
                            height: 20
                            text: "\u25BC"
                            bgcolor: "#3e3e42"
                            color: "#fff"
                            font.pixelSize: 7
                            onClicked: moveOp(index, 1)
                        }
                        AppButton {
                            width: 40
                            height: 20
                            text: "CAGE"
                            bgcolor: modelData.operandId === (Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1) ? "#2aa8ff" : "#3e3e42"
                            color: "#fff"
                            font.pixelSize: 7
                            font.bold: true
                            onClicked: Modeler.booleanSelectOperand(objectId, index)
                            ToolTip.visible: hovered
                            ToolTip.text: "Edit this operand (cage). Geometry edits re-run the boolean stack live."
                        }
                        AppButton {
                            width: 18
                            height: 20
                            text: "\u2715"
                            bgcolor: "#5a2a2a"
                            color: "#fff"
                            font.pixelSize: 8
                            onClicked: removeOp(index)
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: { }
                        onDoubleClicked: toggleOp(index, !modelData.enabled)
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: stackList.length === 0
                text: "No boolean operations. Select object + operand and press Add."
                color: "#666"
                font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                Layout.fillWidth: true
                height: 28
                text: "Apply (Bake)"
                bgcolor: "#ff6600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 10
                enabled: stackList.length > 0
                onClicked: { Modeler.booleanApply(objectId); closePanel() }
            }

            AppButton {
                Layout.fillWidth: true
                height: 28
                text: "Re-eval"
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                enabled: stackList.length > 0
                onClicked: { Modeler.booleanEvaluate(objectId) }
                ToolTip.visible: hovered; ToolTip.text: "Manually re-run the boolean stack (auto re-runs on edits)"
            }

            AppButton {
                Layout.fillWidth: true
                height: 28
                text: "Clear"
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                enabled: stackList.length > 0
                onClicked: Modeler.booleanClear(objectId)
            }
        }

        Text {
            text: "Hint: double-click a row to toggle enable. Press CAGE to edit an operand; its edits re-run the stack live."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
