import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: vertexEdgeSlidePanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property int mode: 0 // 0=vertex, 1=edge
    property int pickedVertex: -1
    property var pickedEdge: {v0: -1, v1: -1}
    property bool picking: false

    function setMode(m) {
        mode = m
        pickedVertex = -1
        pickedEdge = {v0: -1, v1: -1}
    }

    function startPick() {
        picking = true
        pickedVertex = -1
        pickedEdge = {v0: -1, v1: -1}
    }

    function applyPick(wx, wy, wz) {
        if (!picking || objectId < 0) return
        picking = false
        if (mode === 0) {
            var vid = Modeler.findClosestVertex(objectId, wx, wy, wz)
            if (vid >= 0) pickedVertex = vid
        } else {
            var edge = Modeler.findClosestEdge(objectId, wx, wy, wz)
            if (edge.v0 >= 0 && edge.v1 >= 0) pickedEdge = edge
        }
    }

    function slideVertex() {
        if (pickedVertex >= 0 && objectId >= 0) {
            // In a real implementation, this would use a gizmo or second click
            // For now, slide toward cursor position - but we need a target position
            // Simplified: just trigger a slide with a small offset
            Modeler.vertexSlide(objectId, pickedVertex, 0, 0, 0) // placeholder
        }
    }

    function slideEdge(factor) {
        if (pickedEdge.v0 >= 0 && objectId >= 0) {
            Modeler.edgeSlide(objectId, pickedEdge.v0, pickedEdge.v1, factor)
        }
    }

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            pickedVertex = -1
            pickedEdge = {v0: -1, v1: -1}
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
                text: "VERTEX / EDGE SLIDE"
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
            text: objectId >= 0 ? "Target: " + Modeler.selectedObject.name
                                : "No object selected"
            color: objectId >= 0 ? "#aaa" : "#E10600"
            font.pixelSize: 10
            font.bold: objectId >= 0
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "MODE"; color: "#888"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: "Vertex"
                height: 28
                checkable: true
                autoExclusive: true
                checked: mode === 0
                onClicked: setMode(0)
                bgcolor: mode === 0 ? "#E10600" : "#3e3e42"
                color: mode === 0 ? "#121212" : "#fff"
                font.pixelSize: 10
                Layout.fillWidth: true
            }

            AppButton {
                text: "Edge"
                height: 28
                checkable: true
                autoExclusive: true
                checked: mode === 1
                onClicked: setMode(1)
                bgcolor: mode === 1 ? "#E10600" : "#3e3e42"
                color: mode === 1 ? "#121212" : "#fff"
                font.pixelSize: 10
                Layout.fillWidth: true
            }
        }

        Text { text: "PICK IN VIEWPORT"; color: "#888"; font.pixelSize: 10; font.bold: true }

        AppButton {
            text: picking ? "Click vertex/edge in User viewport..." : "Pick " + (mode === 0 ? "Vertex" : "Edge")
            height: 30
            Layout.fillWidth: true
            bgcolor: picking ? "#E10600" : "#3e3e42"
            color: picking ? "#121212" : "#fff"
            font.pixelSize: 10
            font.bold: picking
            onClicked: startPick()
        }

        Text {
            text: mode === 0
                ? (pickedVertex >= 0 ? "Picked vertex: " + pickedVertex : "No vertex picked")
                : (pickedEdge.v0 >= 0 ? "Picked edge: " + pickedEdge.v0 + " - " + pickedEdge.v1 : "No edge picked")
            color: (mode === 0 ? (pickedVertex >= 0 ? "#aaa" : "#888") : (pickedEdge.v0 >= 0 ? "#aaa" : "#888"))
            font.pixelSize: 10
            font.bold: (mode === 0 ? pickedVertex >= 0 : pickedEdge.v0 >= 0)
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "SLIDE"; color: "#888"; font.pixelSize: 10; font.bold: true }

        // Vertex slide: slider for direction? Actually needs a target position.
        // For now, provide a simple "slide along edge toward cursor" via gizmo or 2-click.
        // Simplified UI: use slider to slide edge
        Item {
            visible: mode === 1 && pickedEdge.v0 >= 0
            Layout.fillWidth: true

            Text { text: "Edge Slide Factor: " + edgeSlideFactor.toFixed(2); color: "#aaa"; font.pixelSize: 9 }
            Slider {
                id: edgeSlideFactor
                Layout.fillWidth: true
                from: -1.0; to: 1.0; stepSize: 0.01
                value: 0.0
                onValueChanged: slideEdge(value)
                background: Rectangle { x: edgeSlideFactor.leftPadding; y: edgeSlideFactor.topPadding + edgeSlideFactor.availableHeight / 2 - 2; width: edgeSlideFactor.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                    Rectangle { width: edgeSlideFactor.visualPosition * parent.width; height: 4; radius: 2; color: "#ff6600" }
                }
                handle: Rectangle { x: edgeSlideFactor.leftPadding + edgeSlideFactor.visualPosition * (edgeSlideFactor.availableWidth - width); y: edgeSlideFactor.topPadding + edgeSlideFactor.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
            }
        }

        Item {
            visible: mode === 0 && pickedVertex >= 0
            Layout.fillWidth: true

            Text { text: "Vertex Slide: use Gizmo (Move) or click-drag in viewport (TODO: sub-object gizmo)"; color: "#777"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }

        Item { Layout.fillHeight: true }

        Text {
            text: "Note: Vertex slide requires sub-object gizmo or 2-click target. Edge slide uses factor slider."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}