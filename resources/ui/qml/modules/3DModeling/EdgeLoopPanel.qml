import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: edgeLoopPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property var pickedEdge: {v0: -1, v1: -1}
    property bool picking: false
    property var loopEdges: []
    property var ringEdges: []
    property int loopCount: 0
    property int ringCount: 0

    function startPick() {
        if (objectId < 0) return
        picking = true
        pickedEdge = {v0: -1, v1: -1}
    }

    function applyPick(wx, wy, wz) {
        if (!picking || objectId < 0) return
        picking = false
        var edge = Modeler.findClosestEdge(objectId, wx, wy, wz)
        if (edge.v0 >= 0 && edge.v1 >= 0) {
            pickedEdge = edge
            statusMsg.text = "Picked edge " + edge.v0 + " - " + edge.v1 + " (use Loop / Ring)"
        } else {
            statusMsg.text = "No edge picked"
        }
    }

    function doLoop() {
        if (objectId < 0 || pickedEdge.v0 < 0) return
        loopEdges = Modeler.edgeLoop(objectId, pickedEdge.v0, pickedEdge.v1)
        loopCount = loopEdges.length
        ringEdges = []
        ringCount = 0
        statusMsg.text = "Loop: " + loopCount + " edges"
    }

    function doRing() {
        if (objectId < 0 || pickedEdge.v0 < 0) return
        ringEdges = Modeler.edgeRing(objectId, pickedEdge.v0, pickedEdge.v1)
        ringCount = ringEdges.length
        loopEdges = []
        loopCount = 0
        statusMsg.text = "Ring: " + ringCount + " edges"
    }

    function clearSel() {
        loopEdges = []
        ringEdges = []
        loopCount = 0
        ringCount = 0
        pickedEdge = {v0: -1, v1: -1}
    }

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            clearSel()
            picking = false
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "EDGE LOOP / RING"
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

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text {
            text: objectId >= 0 ? "Target: " + Modeler.selectedObject.name : "Select a mesh object"
            color: objectId >= 0 ? "#aaa" : "#E10600"
            font.pixelSize: 11
            font.bold: objectId >= 0
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        AppButton {
            text: picking ? "Click an edge in the User viewport..." : "Pick Edge"
            height: 28
            Layout.fillWidth: true
            bgcolor: picking ? "#E10600" : "#3e3e42"
            color: picking ? "#121212" : "#fff"
            font.pixelSize: 11
            font.bold: picking
            enabled: objectId >= 0
            onClicked: startPick()
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: "Edge Loop"
                height: 26
                Layout.fillWidth: true
                bgcolor: "#E10600"; color: "#fff"
                font.pixelSize: 10
                enabled: pickedEdge.v0 >= 0
                onClicked: doLoop()
                ToolTip.visible: hovered; ToolTip.text: "Series of edges crossing quads (opposite edges)"
            }

            AppButton {
                text: "Edge Ring"
                height: 26
                Layout.fillWidth: true
                bgcolor: "#3e3e42"; color: "#fff"
                font.pixelSize: 10
                enabled: pickedEdge.v0 >= 0
                onClicked: doRing()
                ToolTip.visible: hovered; ToolTip.text: "Series of edges sharing a vertex around quads"
            }

            AppButton {
                text: "Clear"
                height: 26
                bgcolor: "transparent"; color: "#aaa"
                font.pixelSize: 10
                onClicked: clearSel()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#333"
        }

        Text {
            id: statusMsg
            text: loopCount > 0 ? ("Edge loop: " + loopCount + " edges highlighted")
                 : (ringCount > 0 ? ("Edge ring: " + ringCount + " edges highlighted")
                 : "Pick an edge, then select Loop or Ring. Highlighting appears in the User viewport.")
            color: "#888"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
