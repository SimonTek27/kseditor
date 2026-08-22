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
                AppButton { height: 28; text: "Fillet Chain"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: {
                        var es = Modeler.selectedSubEdges ? Modeler.selectedSubEdges() : [];
                        var rs = [];
                        for (var i = 0; i < Math.max(es.length, 1); ++i)
                            rs.push(0.06 + 0.015 * (i % 3));
                        if (Modeler.filletChain) Modeler.filletChain(es.length ? es : [], rs, 1, 40.0)
                    }
                    ToolTip.visible: hovered; ToolTip.text: "Bevel selected edges with a tapering chain radius (Plasticity fillet-chain)" }
                AppButton { height: 28; text: "Loop Cut"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { var id = Modeler.selectedObject ? Modeler.selectedObject.id : -1; if (id >= 0 && Modeler.loopCut) Modeler.loopCut(id, 2, 0.5, 0.0) } }
                AppButton { height: 28; text: "Knife"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { var id = Modeler.selectedObject ? Modeler.selectedObject.id : -1; if (id >= 0 && Modeler.knifeCutSelected) Modeler.knifeCutSelected(id) } }
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

                Rectangle {
                    height: actionCenter.height
                    width: parent.width
                    color: "#252526"
                    clip: true

                    ColumnLayout {
                        id: actionCenter
                        width: parent.width
                        spacing: 4

                        Text { text: "ACTION CENTER"; color: "#ff6600"; font.pixelSize: 9; font.bold: true }

                        ComboBox {
                            id: acMode
                            Layout.fillWidth: true; height: 22
                            model: ["Translate", "Rotate", "Uniform Scale", "Axis Scale"]
                        }

                        ComboBox {
                            id: acAxis
                            Layout.fillWidth: true; height: 22
                            model: ["X", "Y", "Z"]
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 3
                            columnSpacing: 4
                            rowSpacing: 4

                            Text { text: "X"; color: "#888"; font.pixelSize: 9; Layout.alignment: Qt.AlignVCenter }
                            Text { text: "Y"; color: "#888"; font.pixelSize: 9; Layout.alignment: Qt.AlignVCenter }
                            Text { text: "Z"; color: "#888"; font.pixelSize: 9; Layout.alignment: Qt.AlignVCenter }

                            SpinBox {
                                id: acX
                                Layout.fillWidth: true; height: 22
                                from: -10000; to: 10000; value: 0; editable: true; stepSize: 10
                            }
                            SpinBox {
                                id: acY
                                Layout.fillWidth: true; height: 22
                                from: -10000; to: 10000; value: 0; editable: true; stepSize: 10
                            }
                            SpinBox {
                                id: acZ
                                Layout.fillWidth: true; height: 22
                                from: -10000; to: 10000; value: 0; editable: true; stepSize: 10
                            }
                        }

                        AppButton {
                            height: 26; Layout.fillWidth: true
                            text: "Apply"
                            bgcolor: "#ff6600"; color: "#121212"
                            font.pixelSize: 11; font.bold: true
                            enabled: Modeler.selectedObject !== undefined
                            onClicked: {
                                if (!Modeler.transformVerticesAround) return
                                var pivot = Modeler.hasProportionalCenter()
                                        ? Modeler.proportionalCenter()
                                        : Modeler.gizmoPosition
                                var mode = acMode.currentIndex
                                var axis = acAxis.currentIndex
                                var r = Modeler.isProportionalEditing() ? Modeler.proportionalRadius() : 0
                                var tx = 0, ty = 0, tz = 0
                                if (mode === 0) { // Translate (mesh units)
                                    tx = acX.value; ty = acY.value; tz = acZ.value
                                } else if (mode === 1) { // Rotate (degrees on the chosen axis)
                                    if (axis === 0) tx = acX.value
                                    else if (axis === 1) ty = acY.value
                                    else tz = acZ.value
                                } else if (mode === 2) { // Uniform scale factor = 1 + v/100
                                    tx = 1 + acX.value / 100
                                } else { // Axis scale: factor on the chosen axis, others 1
                                    tx = ty = tz = 1
                                    if (axis === 0) tx = 1 + acX.value / 100
                                    else if (axis === 1) ty = 1 + acY.value / 100
                                    else tz = 1 + acZ.value / 100
                                }
                                Modeler.transformVerticesAround(mode, pivot.x, pivot.y, pivot.z,
                                                                tx, ty, tz, r)
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
                AppButton { height: 28; text: "Resolve Overlaps"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.resolveUVOverlaps) Modeler.resolveUVOverlaps() }
                    ToolTip.visible: hovered; ToolTip.text: "Detect overlapping UV islands and re-pack them (3ds Max-style overlap resolution)" }
                AppButton { height: 28; text: "Pack"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.packUVs) Modeler.packUVs(0.01, 1024) } }

                Rectangle { height: 10 }

                Text { text: "SKETCH (revolve)"; color: "#ff6600"; font.pixelSize: 10; font.bold: true }
                AppButton { height: 28; text: "Lathe X"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.revolveSketch) Modeler.revolveSketch(24, 360.0, true, 0) }
                    ToolTip.visible: hovered; ToolTip.text: "Revolve the selected sketch profile around the X axis" }
                AppButton { height: 28; text: "Lathe Y"; bgcolor: "#E10600"; color: "#121212"
                    onClicked: { if (Modeler.revolveSketch) Modeler.revolveSketch(24, 360.0, true, 1) }
                    ToolTip.visible: hovered; ToolTip.text: "Revolve the selected sketch profile around the Y axis" }
                AppButton { height: 28; text: "Lathe Z"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.revolveSketch) Modeler.revolveSketch(24, 360.0, true, 2) }
                    ToolTip.visible: hovered; ToolTip.text: "Revolve the selected sketch profile around the Z axis" }
                AppButton { height: 28; text: "Tessellate"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { var id = Modeler.selectedObject ? Modeler.selectedObject.id : -1; if (id >= 0 && Modeler.nurbsSurfaceTessellate) Modeler.nurbsSurfaceTessellate(id); }
                    ToolTip.visible: hovered; ToolTip.text: "Tessellate the selected NURBS surface object" }

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
                AppButton { height: 28; text: "Import LXO"; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (Modeler.importLXO) Modeler.importLXO("") }
                    ToolTip.visible: hovered; ToolTip.text: "Import a Modo .lxo object model" }

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

