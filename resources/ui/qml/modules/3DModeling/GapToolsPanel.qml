import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

// Fase-1 gap tools: Shell (solidity), Polygonal Bridge, Smoothing groups.
Rectangle {
    id: gapToolsPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property real shellThickness: 0.05
    property int shellDirection: 1
    property bool shellFlip: false
    property int bridgeSegments: 1
    property real smoothAngle: 30.0
    property int smoothCount: 0

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            smoothCount = 0
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
                text: "SHELL / BRIDGE / SMOOTH"
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

        // ---- Shell ----
        GroupBox {
            title: "Shell (solidity)"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text { text: "Thickness"; color: "#aaa"; font.pixelSize: 11 }
                    Slider {
                        id: shellSlider
                        Layout.fillWidth: true
                        from: 0.001; to: 1.0; value: shellThickness; stepSize: 0.001
                        onMoved: shellThickness = value
                    }
                    Text { text: shellThickness.toFixed(3); color: "#fff"; font.pixelSize: 11 }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton {
                        text: "Outward"
                        height: 24; Layout.fillWidth: true
                        bgcolor: shellDirection === 1 ? "#E10600" : "#3e3e42"
                        color: shellDirection === 1 ? "#121212" : "#fff"
                        font.pixelSize: 10
                        onClicked: shellDirection = 1
                    }
                    AppButton {
                        text: "Inward"
                        height: 24; Layout.fillWidth: true
                        bgcolor: shellDirection === -1 ? "#E10600" : "#3e3e42"
                        color: shellDirection === -1 ? "#121212" : "#fff"
                        font.pixelSize: 10
                        onClicked: shellDirection = -1
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    CheckBox {
                        id: shellFlipBox
                        text: "Flip outer normals"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                        onToggled: shellFlip = checked
                    }
                    AppButton {
                        text: "Apply Shell"
                        height: 26
                        bgcolor: "#E10600"; color: "#fff"
                        font.pixelSize: 11; font.bold: true
                        enabled: objectId >= 0
                        onClicked: {
                            if (Modeler.applyShell) {
                                var ok = Modeler.applyShell(shellThickness, shellDirection, shellFlip)
                            }
                        }
                    }
                }
            }
        }

        // ---- Bridge ----
        GroupBox {
            title: "Polygonal Bridge"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text { text: "Segments"; color: "#aaa"; font.pixelSize: 11 }
                    SpinBox {
                        id: bridgeSegBox
                        Layout.fillWidth: true
                        from: 1; to: 16; value: bridgeSegments
                        onValueModified: bridgeSegments = value
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton {
                        text: "Bridge Selected Loops"
                        height: 26; Layout.fillWidth: true
                        bgcolor: "#E10600"; color: "#fff"
                        font.pixelSize: 10; font.bold: true
                        enabled: objectId >= 0
                        onClicked: { if (Modeler.bridgeSelectedLoops) Modeler.bridgeSelectedLoops(bridgeSegments) }
                        ToolTip.visible: hovered; ToolTip.text: "Select two border edge loops (Edge or Border sub-object mode), then press"
                    }
                    AppButton {
                        text: "Bridge Faces"
                        height: 26; Layout.fillWidth: true
                        bgcolor: "#3e3e42"; color: "#fff"
                        font.pixelSize: 10
                        enabled: objectId >= 0
                        onClicked: { if (Modeler.bridgeSelectedFaces) Modeler.bridgeSelectedFaces(bridgeSegments) }
                        ToolTip.visible: hovered; ToolTip.text: "Select two faces (Face sub-object mode), then press"
                    }
                }
            }
        }

        // ---- Smoothing groups ----
        GroupBox {
            title: "Smoothing groups"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text { text: "Angle"; color: "#aaa"; font.pixelSize: 11 }
                    Slider {
                        id: smoothSlider
                        Layout.fillWidth: true
                        from: 0.0; to: 90.0; value: smoothAngle; stepSize: 0.5
                        onMoved: smoothAngle = value
                    }
                    Text { text: smoothAngle.toFixed(0) + "°"; color: "#fff"; font.pixelSize: 11 }
                }

                AppButton {
                    text: smoothCount > 0 ? ("Auto-Smooth: " + smoothCount + " groups") : "Auto-Smooth"
                    height: 26; Layout.fillWidth: true
                    bgcolor: "#E10600"; color: "#fff"
                    font.pixelSize: 11; font.bold: true
                    enabled: objectId >= 0
                    onClicked: {
                        if (Modeler.smoothGroupsAuto)
                            smoothCount = Modeler.smoothGroupsAuto(smoothAngle)
                    }
                }

                Text {
                    text: "Each group colors faces within the angle threshold. Group 0 = hard edge."
                    color: "#888"
                    font.pixelSize: 9
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // ---- Hole fill / mesh extraction ----
        GroupBox {
            title: "Hole fill / Extract"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text { text: "Max hole edges"; color: "#aaa"; font.pixelSize: 11 }
                    SpinBox {
                        id: holeEdgeBox
                        Layout.fillWidth: true
                        from: 3; to: 64; value: 16
                    }
                    AppButton {
                        text: "Fill Holes"
                        height: 26; Layout.fillWidth: true
                        bgcolor: "#E10600"; color: "#fff"
                        font.pixelSize: 10; font.bold: true
                        enabled: objectId >= 0
                        onClicked: { if (Modeler.fillMeshHoles) Modeler.fillMeshHoles(holeEdgeBox.value) }
                        ToolTip.visible: hovered; ToolTip.text: "Caps every open boundary loop of the selected mesh with a triangulated patch"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton {
                        text: "Extract Faces"
                        height: 26; Layout.fillWidth: true
                        bgcolor: "#3e3e42"; color: "#fff"
                        font.pixelSize: 10
                        enabled: objectId >= 0
                        onClicked: { if (Modeler.extractSelectedFaces) Modeler.extractSelectedFaces(0, true) }
                        ToolTip.visible: hovered; ToolTip.text: "Builds a standalone object from the selected faces (Face sub-object mode)"
                    }
                    AppButton {
                        text: "Extract Solid"
                        height: 26; Layout.fillWidth: true
                        bgcolor: "#3e3e42"; color: "#fff"
                        font.pixelSize: 10
                        enabled: objectId >= 0
                        onClicked: { if (Modeler.extractSelectedFaces) Modeler.extractSelectedFaces(0.05, true) }
                        ToolTip.visible: hovered; ToolTip.text: "Extracts the selected faces as a shelled solid with thickness"
                    }
                }

                Text {
                    text: "Select faces first (Face sub-object mode). Fill Holes caps open borders with a fan patch."
                    color: "#888"
                    font.pixelSize: 9
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
    }
}