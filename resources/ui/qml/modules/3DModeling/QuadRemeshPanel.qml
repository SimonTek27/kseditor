import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: quadRemeshPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property int densityLevel: 1
    property bool livePreview: true

    Timer {
        id: applyTimer
        interval: 120
        repeat: false
        onTriggered: applyRemesh()
    }

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
        }
    }

    function applyRemesh() {
        if (objectId < 0) return
        var ok = Modeler.quadRemesh(objectId, densityLevel)
        statusMsg.text = ok ? ("Quad remesh applied (level " + densityLevel + ").") : "Quad remesh failed."
    }

    function clearRemesh() {
        if (objectId < 0) return
        Modeler.quadRemeshClear(objectId)
        statusMsg.text = "Original mesh restored."
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "QUAD REMESH"
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

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "Density"
                color: "#aaa"
                font.pixelSize: 11
            }

            Slider {
                id: densitySlider
                Layout.fillWidth: true
                from: 0
                to: 3
                stepSize: 1
                value: densityLevel
                snapMode: Slider.SnapOnRelease
                onValueChanged: {
                    densityLevel = value
                    if (livePreview) applyTimer.restart()
                }
            }

            Text {
                text: densityLevel
                color: "#fff"
                font.pixelSize: 11
                font.bold: true
                width: 14
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            CheckBox {
                id: liveBox
                checked: livePreview
                onCheckedChanged: livePreview = checked
                Text {
                    text: "Live preview (re-apply on change)"
                    color: "#999"; font.pixelSize: 9
                    anchors.left: parent.right; anchors.leftMargin: 2; anchors.verticalCenter: parent.verticalCenter
                }
            }

            AppButton {
                text: "Apply"
                height: 26
                Layout.fillWidth: true
                bgcolor: "#E10600"; color: "#fff"
                font.pixelSize: 10
                font.bold: true
                enabled: objectId >= 0
                onClicked: { applyTimer.stop(); applyRemesh() }
                ToolTip.visible: hovered; ToolTip.text: "Convert the selected mesh to quads (non-destructive, re-runnable)"
            }

            AppButton {
                text: "Restore"
                height: 26
                bgcolor: "#3e3e42"; color: "#fff"
                font.pixelSize: 10
                enabled: objectId >= 0
                onClicked: clearRemesh()
            }
        }

        Text {
            text: densityLevel === 0 ? "Merge adjacent triangles into quads."
                 : densityLevel === 1 ? "Subdivide once, then merge into quads (fine quad grid)."
                 : "Subdivide " + densityLevel + "x, then merge into quads (dense quad grid)."
            color: "#888"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        AppButton {
            text: "Restore Original"
            height: 26
            Layout.fillWidth: true
            bgcolor: "#3e3e42"; color: "#fff"
            font.pixelSize: 10
            enabled: objectId >= 0
            onClicked: clearRemesh()
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#333"
        }

        Text {
            id: statusMsg
            text: "Converts triangles to a quad-dominant mesh. Adjust density and re-apply; the original is always restored."
            color: "#888"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
