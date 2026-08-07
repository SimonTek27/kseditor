import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window 2.15
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: modelerTools
    width: 300
    height: 550
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property string activeTool: "select"
    property bool proportionalMode: false
    property real proportionalFalloff: 0.5

    signal toolSelected(string tool)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 36
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10

                Text { text: "TOOLS"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                Item { Layout.fillWidth: true }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4

                Text { text: "SELECTION"; color: "#666"; font.pixelSize: 10; font.bold: true }

                Rectangle {
                    height: 28
                    Layout.fillWidth: true
                    color: activeTool === "select" ? "#E10600" : "#3e3e42"
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4

                        Text { 
                            text: "Select" 
                            color: activeTool === "select" ? "#121212" : "#ffffff"
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }

                        Text {
                            text: Modeler.getShortcutKey("select")
                            color: activeTool === "select" ? "#121212" : "#999"
                            font.pixelSize: 8
                            font.italic: true
                            rightPadding: 4
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: activeTool = "select"
                        hoverEnabled: true
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: Modeler.getShortcutDescription("select")
                    ToolTip.delay: 500
                }

                Rectangle {
                    height: 28
                    Layout.fillWidth: true
                    color: activeTool === "loop" ? "#E10600" : "#3e3e42"
                    radius: 2

                    Text { 
                        text: "Loop Select"
                        color: activeTool === "loop" ? "#121212" : "#ffffff"
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 8
                        font.pixelSize: 11
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: activeTool = "loop"
                        hoverEnabled: true
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: "Loop Select"
                    ToolTip.delay: 500
                }

                Rectangle {
                    height: 28
                    Layout.fillWidth: true
                    color: activeTool === "ring" ? "#E10600" : "#3e3e42"
                    radius: 2

                    Text { 
                        text: "Ring Select"
                        color: activeTool === "ring" ? "#121212" : "#ffffff"
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 8
                        font.pixelSize: 11
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: activeTool = "ring"
                        hoverEnabled: true
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: "Ring Select"
                    ToolTip.delay: 500
                }

                Rectangle {
                    height: 28
                    Layout.fillWidth: true
                    color: activeTool === "similar" ? "#E10600" : "#3e3e42"
                    radius: 2

                    Text { 
                        text: "Similar"
                        color: activeTool === "similar" ? "#121212" : "#ffffff"
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 8
                        font.pixelSize: 11
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: activeTool = "similar"
                        hoverEnabled: true
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: "Select similar elements"
                    ToolTip.delay: 500
                }

                Rectangle { height: 10 }

                Text { text: "EDITING"; color: "#666"; font.pixelSize: 10; font.bold: true }
                AppButton { height: 28; text: "Extrude"; bgcolor: "#E10600"; color: "#121212"
                    onClicked: { if (Modeler.extrudeFaces) Modeler.extrudeFaces([], 1.0) } }
                AppButton { height: 28; text: "Inset"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.insetFaces) Modeler.insetFaces([], 0.5) } }
                AppButton { height: 28; text: "Bevel"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.bevelEdges) Modeler.bevelEdges([], 0.1, 1) } }
                AppButton { height: 28; text: "Loop Cut"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.subdivideFaces) Modeler.subdivideFaces([], 2) } }
                AppButton { height: 28; text: "Knife"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.knifeCut) Modeler.knifeCut(0, 0, 0, 1, 1, 1) } }
                AppButton { height: 28; text: "Weld"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.weldVertices) Modeler.weldVertices(0.01) } }

                Rectangle { height: 10 }

                Text { text: "TRANSFORM"; color: "#666"; font.pixelSize: 10; font.bold: true }

                Rectangle {
                    height: 28
                    Layout.fillWidth: true
                    color: activeTool === "move" ? "#E10600" : "#3e3e42"
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4

                        Text {
                            text: "Move"
                            color: activeTool === "move" ? "#121212" : "#ffffff"
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }

                        Text {
                            text: Modeler.getShortcutKey("move")
                            color: activeTool === "move" ? "#121212" : "#999"
                            font.pixelSize: 8
                            font.italic: true
                            rightPadding: 4
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: activeTool = "move"
                        hoverEnabled: true
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: Modeler.getShortcutDescription("move")
                    ToolTip.delay: 500
                }

                Rectangle {
                    height: 28
                    Layout.fillWidth: true
                    color: activeTool === "rotate" ? "#E10600" : "#3e3e42"
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4

                        Text {
                            text: "Rotate"
                            color: activeTool === "rotate" ? "#121212" : "#ffffff"
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }

                        Text {
                            text: Modeler.getShortcutKey("rotate")
                            color: activeTool === "rotate" ? "#121212" : "#999"
                            font.pixelSize: 8
                            font.italic: true
                            rightPadding: 4
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: activeTool = "rotate"
                        hoverEnabled: true
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: Modeler.getShortcutDescription("rotate")
                    ToolTip.delay: 500
                }

                Rectangle {
                    height: 28
                    Layout.fillWidth: true
                    color: activeTool === "scale" ? "#E10600" : "#3e3e42"
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4

                        Text {
                            text: "Scale"
                            color: activeTool === "scale" ? "#121212" : "#ffffff"
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }

                        Text {
                            text: Modeler.getShortcutKey("scale")
                            color: activeTool === "scale" ? "#121212" : "#999"
                            font.pixelSize: 8
                            font.italic: true
                            rightPadding: 4
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: activeTool = "scale"
                        hoverEnabled: true
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: Modeler.getShortcutDescription("scale")
                    ToolTip.delay: 500
                }

                RowLayout {
                    AppButton { height: 24; text: "Mirror X"; bgcolor: "transparent"; color: "#ffffff"; Layout.fillWidth: true
                        onClicked: { if (Modeler.mirrorMesh) Modeler.mirrorMesh(0) } }
                    AppButton { height: 24; text: "Y"; bgcolor: "transparent"; color: "#ffffff"; width: 30
                        onClicked: { if (Modeler.mirrorMesh) Modeler.mirrorMesh(1) } }
                    AppButton { height: 24; text: "Z"; bgcolor: "transparent"; color: "#ffffff"; width: 30
                        onClicked: { if (Modeler.mirrorMesh) Modeler.mirrorMesh(2) } }
                }

                AppButton { height: 28; text: "Align"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.setSelectedPosition) Modeler.setSelectedPosition(0, 0, 0) } }
                AppButton {
                    id: propBtn
                    height: 28; text: "Proportional Edit (" + Modeler.getShortcutKey("toggle_proportional") + ")";
                    bgcolor: Modeler.isProportionalEditing() ? "#ff6600" : "transparent";
                    color: "#ffffff";
                    onClicked: { Modeler.setProportionalEditing(!Modeler.isProportionalEditing()); }
                }

                Rectangle {
                    height: proportionalSettings.height
                    width: parent.width
                    color: "#252526"
                    visible: Modeler.isProportionalEditing()
                    clip: true

                    ColumnLayout {
                        id: proportionalSettings
                        width: parent.width
                        spacing: 4

                        Text { text: "FALLOFF"; color: "#888"; font.pixelSize: 9; font.bold: true }

                        ComboBox {
                            Layout.fillWidth: true; height: 22
                            model: ["Smooth", "Linear", "Sharp", "Root", "Sphere", "Constant"]
                            currentIndex: Modeler.proportionalFalloffType()
                            onActivated: Modeler.setProportionalFalloffType(index)
                        }

                        Text { text: "RADIUS"; color: "#888"; font.pixelSize: 9; font.bold: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Slider {
                                Layout.fillWidth: true; height: 20
                                from: 0.01; to: 10.0; stepSize: 0.01
                                value: Modeler.proportionalRadius()
                                onMoved: Modeler.setProportionalRadius(value)
                            }
                            Text {
                                width: 50
                                text: Modeler.proportionalRadius().toFixed(2)
                                color: "#ccc"; font.pixelSize: 10
                            }
                        }

                        Text { text: "CENTER"; color: "#888"; font.pixelSize: 9; font.bold: true }

                        RowLayout {
                            Layout.fillWidth: true
                            AppButton {
                                height: 22; text: "Pick"; Layout.fillWidth: true
                                bgcolor: Modeler.hasProportionalCenter() ? "#E10600" : "#3e3e42"
                                color: "#ffffff"
                                onClicked: { /* QML viewport will call pick on click */ }
                            }
                            AppButton {
                                height: 22; text: "Clear"
                                bgcolor: "#3e3e42"; color: "#ffffff"
                                onClicked: Modeler.clearProportionalCenter()
                                enabled: Modeler.hasProportionalCenter()
                            }
                        }

                        Rectangle { height: 6; color: "transparent" }
                    }
                }

                Rectangle { height: 10 }

                Text { text: "MESH OPS"; color: "#666"; font.pixelSize: 10; font.bold: true }
                AppButton { height: 28; text: "Subsurf"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.subdivideFaces) Modeler.subdivideFaces([], 2) } }
                AppButton { height: 28; text: "Decimate"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.removeDoubles) Modeler.removeDoubles(0.1) } }
                AppButton { height: 28; text: "Remesh"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.triangulateMesh) Modeler.triangulateMesh() } }
                AppButton { height: 28; text: "Spin"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.duplicateSelected) Modeler.duplicateSelected() } }
                AppButton { height: 28; text: "Boolean"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { boolOpPanel.visible = !boolOpPanel.visible } }
                AppButton { height: 28; text: "Symmetry"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { symmetryPanel.visible = !symmetryPanel.visible } }

                Rectangle { height: 10 }

                Text { text: "DEFORM"; color: "#ff8800"; font.pixelSize: 10; font.bold: true }
                AppButton {
                    height: 28; text: "Twist";
                    bgcolor: activeTool === "twist" ? "#E10600" : "#3e3e42";
                    color: activeTool === "twist" ? "#121212" : "#ffffff";
                    onClicked: { activeTool = "twist"; if (Modeler.applySimpleDeform) Modeler.applySimpleDeform(0, 0, 45, 1.0); modelerTools.toolSelected("twist"); }
                }
                AppButton {
                    height: 28; text: "Bend";
                    bgcolor: activeTool === "bend" ? "#E10600" : "#3e3e42";
                    color: activeTool === "bend" ? "#121212" : "#ffffff";
                    onClicked: { activeTool = "bend"; if (Modeler.applySimpleDeform) Modeler.applySimpleDeform(1, 0, 45, 1.0); modelerTools.toolSelected("bend"); }
                }
                AppButton {
                    height: 28; text: "Stretch";
                    bgcolor: activeTool === "stretch" ? "#E10600" : "#3e3e42";
                    color: activeTool === "stretch" ? "#121212" : "#ffffff";
                    onClicked: { activeTool = "stretch"; if (Modeler.applySimpleDeform) Modeler.applySimpleDeform(2, 0, 45, 1.0); modelerTools.toolSelected("stretch"); }
                }
                AppButton {
                    height: 28; text: "Lattice";
                    bgcolor: activeTool === "lattice" ? "#E10600" : "#3e3e42";
                    color: activeTool === "lattice" ? "#121212" : "#ffffff";
                    onClicked: { activeTool = "lattice"; if (Modeler.applyLattice) Modeler.applyLattice(4, 4, 4, 1.0); modelerTools.toolSelected("lattice"); }
                }
                AppButton {
                    height: 28; text: "Cage Deform";
                    bgcolor: activeTool === "cage" ? "#E10600" : "#3e3e42";
                    color: activeTool === "cage" ? "#121212" : "#ffffff";
                    onClicked: { activeTool = "cage"; if (Modeler.applyCageDeform) Modeler.applyCageDeform("", 1.0, true); modelerTools.toolSelected("cage"); }
                }

                Rectangle { height: 10 }

                Text { text: "UV"; color: "#666"; font.pixelSize: 10; font.bold: true }
                AppButton { height: 28; text: "Unwrap"; bgcolor: "#E10600"; color: "#121212"
                    onClicked: { if (Modeler.unwrapUVs) Modeler.unwrapUVs("lscm") } }
                AppButton { height: 28; text: "Project"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.projectUVPlanar) Modeler.projectUVPlanar(0) } }
                AppButton { height: 28; text: "Mark Seam"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.dissolveEdges) Modeler.dissolveEdges([]) } }

                Rectangle { height: 10 }

                Text { text: "SCULPT"; color: "#666"; font.pixelSize: 10; font.bold: true }
                AppButton { height: 28; text: "Draw"; bgcolor: "#E10600"; color: "#121212"
                    onClicked: activeTool = "draw" }
                AppButton { height: 28; text: "Smooth"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: activeTool = "smooth" }
                AppButton { height: 28; text: "Grab"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: activeTool = "grab" }
                AppButton { height: 28; text: "Flatten"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: activeTool = "flatten" }
                AppButton { height: 28; text: "Crease"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: activeTool = "crease" }

                Rectangle { height: 10 }

                Text { text: "AC TRACK TOOLS"; color: "#ff6600"; font.pixelSize: 10; font.bold: true }
                AppButton { height: 28; text: "Generate Track Mesh"; bgcolor: "#ff6600"; color: "#ffffff"
                    onClicked: { if (Modeler.generateTrackMesh) Modeler.generateTrackMesh() } }
                AppButton { height: 28; text: "AI Line"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.generateAILine) Modeler.generateAILine() } }
                AppButton { height: 28; text: "Smooth Track"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.smoothTrackPoints) Modeler.smoothTrackPoints(3) } }
                AppButton { height: 28; text: "Generate Terrain"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.generateTerrain) Modeler.generateTerrain(200, 20) } }
                AppButton { height: 28; text: "Import GPX"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.importGPX) Modeler.importGPX("") } }
                AppButton { height: 28; text: "Import KML"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.importKML) Modeler.importKML("") } }
                AppButton {
                    height: 28; text: "Texture Paint";
                    bgcolor: activeTool === "paint" ? "#E10600" : "transparent";
                    color: "#ffffff";
                    onClicked: {
                        activeTool = activeTool === "paint" ? "select" : "paint"
                        modelerTools.toolSelected(activeTool)
                    }
                }

                Rectangle { height: 10 }

                Text { text: "AC CAR TOOLS"; color: "#ff6600"; font.pixelSize: 10; font.bold: true }
                AppButton { height: 28; text: "Flip Normals"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.flipNormals) Modeler.flipNormals() } }
                AppButton { height: 28; text: "Recalc Normals"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.recalculateNormals) Modeler.recalculateNormals() } }

                Rectangle { height: 10 }

                Text { text: "EXPORT"; color: "#666"; font.pixelSize: 10; font.bold: true }
                AppButton { height: 28; text: "Export KN5"; bgcolor: "#E10600"; color: "#121212"
                    onClicked: { if (Modeler.exportKN5) Modeler.exportKN5("export.kn5") } }
                AppButton { height: 28; text: "Export FBX"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.exportFBX) Modeler.exportFBX("export.fbx") } }
                AppButton { height: 28; text: "Export GLB"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.exportGLB) Modeler.exportGLB("export.glb") } }

                Rectangle { height: 10 }

                Text { text: "VIEW"; color: "#666"; font.pixelSize: 10; font.bold: true }
                AppButton { height: 28; text: "Wireframe"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.viewMode !== undefined) Modeler.viewMode = 1 } }
                AppButton { height: 28; text: "Solid"; bgcolor: "#E10600"; color: "#121212"
                    onClicked: { if (Modeler.viewMode !== undefined) Modeler.viewMode = 0 } }
                AppButton { height: 28; text: "Textured"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.viewMode !== undefined) Modeler.viewMode = 2 } }

                Item { height: 10 }
            }
        }
    }
}

