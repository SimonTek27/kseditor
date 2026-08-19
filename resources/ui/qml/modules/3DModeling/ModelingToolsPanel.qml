import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: modelingTools
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property real filletRadius: 0.05
    property real chamferDist: 0.05
    property real pushPullDist: 0.1
    property real offsetDist: 0.05
    property real sectionNx: 0
    property real sectionNy: 1
    property real sectionNz: 0
    property int sectionPreviewId: -1
    property real sectionOffsetX: 0
    property real sectionOffsetY: 0
    property real sectionOffsetZ: 0
    property int arrayCount: 4
    property real arrayOx: 0.2
    property real arrayOy: 0
    property real arrayOz: 0
    property int cplaneMode: 1
    property bool facePicking: false
    property int pickedFace: -1
    property int dimPickCount: 0

    property int nurbsRows: 4
    property int nurbsCols: 4
    property real nurbsAmplitude: 1.0
    property int nurbsUSeg: 32
    property int nurbsVSeg: 32
    property vector3d nurbsLimits: Qt.vector3d(0, 0, 0)
    property var instancesList: []
    property string exportPath: ""
    property int exportViewAxis: 2
    property int combObjectId: -1
    property int cvSelectedRow: -1
    property int cvSelectedCol: -1

    function applyFillet() {
        if (objectId < 0) return
        var sel = Modeler.selectedSubEdges()
        Modeler.filletEdges(sel.length > 0 ? sel : [], filletRadius)
    }
    function applyChamfer() {
        if (objectId < 0) return
        var sel = Modeler.selectedSubEdges()
        Modeler.chamferEdges(sel.length > 0 ? sel : [], chamferDist)
    }

    function doSection() {
        if (objectId < 0) return
        var o = Modeler.getCPlane()
        Modeler.cutByPlane(o.origin.x, o.origin.y, o.origin.z,
                          sectionNx !== 0 || sectionNy !== 0 || sectionNz !== 0 ? sectionNx : o.normal.x,
                          sectionNx !== 0 || sectionNy !== 0 || sectionNz !== 0 ? sectionNy : o.normal.y,
                          sectionNx !== 0 || sectionNy !== 0 || sectionNz !== 0 ? sectionNz : o.normal.z)
    }

    function moveSectionPlane(dx, dy, dz) {
        if (sectionPreviewId < 0 || !Modeler.updateSectionPreview) return
        sectionOffsetX += dx
        sectionOffsetY += dy
        sectionOffsetZ += dz
        var o = Modeler.getCPlane()
        Modeler.updateSectionPreview(sectionPreviewId,
                                     o.origin.x + sectionOffsetX,
                                     o.origin.y + sectionOffsetY,
                                     o.origin.z + sectionOffsetZ,
                                     sectionNx !== 0 || sectionNy !== 0 || sectionNz !== 0 ? sectionNx : o.normal.x,
                                     sectionNx !== 0 || sectionNy !== 0 || sectionNz !== 0 ? sectionNy : o.normal.y,
                                     sectionNx !== 0 || sectionNy !== 0 || sectionNz !== 0 ? sectionNz : o.normal.z)
    }

    function doArray() {
        if (objectId < 0 || arrayCount <= 1) return
        if (cplaneMode === 0)
            Modeler.linearArray(arrayCount, arrayOx, arrayOy, arrayOz)
        else
            Modeler.radialArray(arrayCount, 0, 1, 0, 360)
    }

    function doPushPull() {
        if (objectId < 0) return
        var sel = Modeler.selectedSubFaces()
        Modeler.pushPullFaces(sel.length > 0 ? sel : [], pushPullDist)
    }
    function doOffsetFaces() {
        if (objectId < 0) return
        var sel = Modeler.selectedSubFaces()
        Modeler.offsetSelectedFaces(sel.length > 0 ? sel : [], offsetDist)
    }

    function setCPlaneFromView() {
        if (!Modeler.setCPlane) return
        if (cplaneMode === 0) Modeler.setCPlane(0, 0, 0, 0, 0, 1, 0, 1, 0)
        else if (cplaneMode === 1) Modeler.setCPlane(0, 0, 0, 0, 1, 0, 0, 0, 1)
        else Modeler.setCPlane(0, 0, 0, 1, 0, 0, 0, 0, 1)
    }

    function toggleFacePick() {
        facePicking = !facePicking
        if (facePicking) {
            Modeler.setSubobjectMode(2)
            Modeler.clearSubSelection()
        }
    }

    function buildPatchRows() {
        var rows = []
        for (var i = 0; i < nurbsRows; ++i) {
            var row = []
            for (var j = 0; j < nurbsCols; ++j) {
                var u = i / (nurbsRows - 1) - 0.5
                var v = j / (nurbsCols - 1) - 0.5
                var h = nurbsAmplitude * Math.sin(Math.PI * (i / (nurbsRows - 1))) * Math.sin(Math.PI * (j / (nurbsCols - 1)))
                row.push([u, 0, v])
                row[j][1] = h
            }
            rows.push(row)
        }
        return rows
    }

    function createNurbsSurface() {
        if (!Modeler.nurbsSurfaceCreate) return
        Modeler.nurbsSurfaceCreate(buildPatchRows(), 3, 3)
    }

    function loftCurvesToNurbs() {
        if (!Modeler.nurbsSurfaceLoftCurves) return
        var ids = Modeler.curveIds ? Modeler.curveIds() : []
        if (ids.length < 2) return
        Modeler.nurbsSurfaceLoftCurves(ids, 3, 3)
    }

    function refreshInstances() {
        if (objectId < 0 || !Modeler) { instancesList = []; syncInstancesModel(); return }
        instancesList = Modeler.getInstances ? Modeler.getInstances(objectId) : []
        syncInstancesModel()
    }

    function doExportSVG() {
        if (objectId < 0 || !Modeler.exportHiddenLineSVG) return
        var base = exportPath.length > 0 ? exportPath : "ks_export"
        var name = Modeler.selectedObject ? Modeler.selectedObject.name : "object"
        var file = base + "_" + name + ".svg"
        Modeler.exportHiddenLineSVG(file, exportViewAxis, 0.3)
    }

    function moveSelectedCVLocal(dx, dy, dz) {
        var r = Modeler.nurbsSelectedRow !== undefined ? Modeler.nurbsSelectedRow : cvSelectedRow
        var c = Modeler.nurbsSelectedCol !== undefined ? Modeler.nurbsSelectedCol : cvSelectedCol
        if (objectId < 0 || r < 0 || c < 0 || !Modeler.nurbsSurfaceMoveCV) return
        var rows = Modeler.nurbsSurfaceCvPositions(objectId)
        if (r >= rows.length) return
        var row = rows[r]
        if (c >= row.length) return
        var p = row[c]
        Modeler.nurbsSurfaceMoveCV(objectId, r, c,
                                   p[0] + dx, p[1] + dy, p[2] + dz)
    }

    function selectCVFromPoint(screenX, screenY) {
        if (objectId < 0 || !Modeler.nurbsSurfaceCvPositions || !Modeler.pickCvNearScreen) return
        Modeler.pickCvNearScreen(objectId, screenX, screenY, viewport ? viewport.width : 800, viewport ? viewport.height : 600)
    }

    function refreshCvList() {
        if (objectId < 0 || !Modeler.nurbsSurfaceCvPositions) return
        var rows = Modeler.nurbsSurfaceCvPositions(objectId)
        var first = -1
        for (var r = 0; r < rows.length; ++r) {
            for (var c = 0; c < rows[r].length; ++c) {
                if (first < 0) { first = r; }
            }
        }
        if (cvRow.value >= rows.length) cvRow.value = Math.max(0, rows.length - 1)
        if (cvSelectedRow < 0 && rows.length > 0) { cvSelectedRow = 0; cvSelectedCol = 0 }
    }

    ListModel { id: dimensionsModel }    function refreshDimensions() {
        dimensionsModel.clear()
        if (!Modeler || !Modeler.dimensions) return
        var dims = Modeler.dimensions()
        var typeCount = [0, 0, 0]
        for (var i = 0; i < dims.length; ++i) {
            var d = dims[i]
            var type = d.type === "angle" ? 1 : (d.type === "radius" ? 2 : 0)
            dimensionsModel.append({
                "dimLabel": (d.label && d.label.length > 0 ? d.label : d.type),
                "dimValue": d.value !== undefined ? d.value : -1,
                "dimType": type,
                "dimIndex": typeCount[type]++,
                "dimActive": d.active !== undefined ? d.active : true
            })
        }
    }
    Component.onCompleted: refreshDimensions()

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            pickedFace = -1
            facePicking = false
            refreshInstances()
            refreshDimensions()
        }
        function onSceneChanged() {
            if (objectId >= 0) refreshInstances()
            refreshDimensions()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "MODELING TOOLS"
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
            text: objectId >= 0 ? "Target: " + (Modeler.selectedObject ? Modeler.selectedObject.name : "") : "Select a mesh object first"
            color: objectId >= 0 ? "#aaa" : "#E10600"
            font.pixelSize: 11
            font.bold: objectId >= 0
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 8

                Text { text: "FILLET / CHAMFER"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 2 }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 6
                    rowSpacing: 4
                    Text { text: "Radius"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: 0; to: 10; stepSize: 1; value: filletRadius * 100; onValueChanged: filletRadius = value / 100 }
                    Text { text: "Distance"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: 0; to: 10; stepSize: 1; value: chamferDist * 100; onValueChanged: chamferDist = value / 100 }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton { text: "Fillet"; height: 26; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10; enabled: objectId >= 0; onClicked: applyFillet() }
                    AppButton { text: "Chamfer"; height: 26; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; enabled: objectId >= 0; onClicked: applyChamfer() }
                }
                Text { text: "Applies to selected edges (or all edges if none selected)."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                Text { text: "PUSH / PULL FACES"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 2 }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 6
                    rowSpacing: 4
                    Text { text: "Distance"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: -500; to: 500; stepSize: 5; value: pushPullDist * 100; onValueChanged: pushPullDist = value / 100 }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton { text: facePicking ? "Pick face..." : "Select Faces"; height: 26; Layout.fillWidth: true; bgcolor: facePicking ? "#E10600" : "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: toggleFacePick() }
                    AppButton { text: "Push/Pull"; height: 26; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10; enabled: objectId >= 0; onClicked: doPushPull() }
                }
                AppButton { text: "Offset Faces"; height: 26; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; enabled: objectId >= 0; onClicked: doOffsetFaces() }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                Text { text: "SECTION (CUT BY PLANE)"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 2 }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 4
                    rowSpacing: 4
                    Text { text: "Nx"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { id: sx; Layout.fillWidth: true; height: 24; from: -10; to: 10; stepSize: 1; onValueChanged: sectionNx = value / 10 }
                    Item { width: 8 }
                    Text { text: "Ny"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { id: sy; Layout.fillWidth: true; height: 24; from: -10; to: 10; stepSize: 1; onValueChanged: sectionNy = value / 10 }
                    Item { width: 8 }
                    Text { text: "Nz"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { id: sz; Layout.fillWidth: true; height: 24; from: -10; to: 10; stepSize: 1; onValueChanged: sectionNz = value / 10 }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    AppButton { text: "Preview"; height: 24; Layout.fillWidth: true; bgcolor: "#2c2c30"; color: "#8ac8ff"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.createSectionPreview) sectionPreviewId = Modeler.createSectionPreview(0, 0, 0, sectionNx, sectionNy, sectionNz) } }
                    AppButton { text: "Hide"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#ff8a6a"; font.pixelSize: 9; enabled: sectionPreviewId >= 0; onClicked: { if (Modeler.deleteSectionPreview) { Modeler.deleteSectionPreview(sectionPreviewId); sectionPreviewId = -1 } } }
                }
                Text { text: "Plane offset (move cut plane):"; color: "#888"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    AppButton { text: "-Z"; height: 22; bgcolor: "#3e3e42"; color: "#8888ff"; font.pixelSize: 9; enabled: sectionPreviewId >= 0; onClicked: moveSectionPlane(0, 0, -0.05) }
                    AppButton { text: "-Y"; height: 22; bgcolor: "#3e3e42"; color: "#88ff88"; font.pixelSize: 9; enabled: sectionPreviewId >= 0; onClicked: moveSectionPlane(0, -0.05, 0) }
                    AppButton { text: "-X"; height: 22; bgcolor: "#3e3e42"; color: "#ff8888"; font.pixelSize: 9; enabled: sectionPreviewId >= 0; onClicked: moveSectionPlane(-0.05, 0, 0) }
                    AppButton { text: "+X"; height: 22; bgcolor: "#3e3e42"; color: "#ff8888"; font.pixelSize: 9; enabled: sectionPreviewId >= 0; onClicked: moveSectionPlane(0.05, 0, 0) }
                    AppButton { text: "+Y"; height: 22; bgcolor: "#3e3e42"; color: "#88ff88"; font.pixelSize: 9; enabled: sectionPreviewId >= 0; onClicked: moveSectionPlane(0, 0.05, 0) }
                    AppButton { text: "+Z"; height: 22; bgcolor: "#3e3e42"; color: "#8888ff"; font.pixelSize: 9; enabled: sectionPreviewId >= 0; onClicked: moveSectionPlane(0, 0, 0.05) }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    AppButton { text: "Apply to Object"; height: 26; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10; enabled: sectionPreviewId >= 0; onClicked: { if (Modeler.applySectionPreview) Modeler.applySectionPreview(sectionPreviewId, objectId); sectionPreviewId = -1 } }
                    AppButton { text: "Replace (Old)"; height: 26; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; enabled: objectId >= 0; onClicked: doSection() }
                }
                Text { text: "Preview creates a separate section profile without touching the target. Move the plane with the offset buttons, then Apply to commit."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                Text { text: "CONSTRUCTION PLANE + SNAP"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 2 }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton { text: "XY"; height: 24; checkable: true; autoExclusive: true; bgcolor: cplaneMode === 0 ? "#E10600" : "#3e3e42"; color: cplaneMode === 0 ? "#121212" : "#fff"; checked: cplaneMode === 0; onClicked: { cplaneMode = 0; setCPlaneFromView() } }
                    AppButton { text: "XZ"; height: 24; checkable: true; autoExclusive: true; bgcolor: cplaneMode === 1 ? "#E10600" : "#3e3e42"; color: cplaneMode === 1 ? "#121212" : "#fff"; checked: cplaneMode === 1; onClicked: { cplaneMode = 1; setCPlaneFromView() } }
                    AppButton { text: "YZ"; height: 24; checkable: true; autoExclusive: true; bgcolor: cplaneMode === 2 ? "#E10600" : "#3e3e42"; color: cplaneMode === 2 ? "#121212" : "#fff"; checked: cplaneMode === 2; onClicked: { cplaneMode = 2; setCPlaneFromView() } }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton { text: "Snap: Vertex"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { if (Modeler.snapTypes) Modeler.snapTypes(1) } }
                    AppButton { text: "Snap: Edge"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { if (Modeler.snapTypes) Modeler.snapTypes(2) } }
                    AppButton { text: "Snap: Face"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { if (Modeler.snapTypes) Modeler.snapTypes(4) } }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                Text { text: "ARRAY"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 2 }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 6
                    rowSpacing: 4
                    Text { text: "Count"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: 2; to: 200; stepSize: 1; value: arrayCount; onValueChanged: arrayCount = value }
                    Text { text: "Offset X"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: -200; to: 200; stepSize: 5; value: arrayOx * 100; onValueChanged: arrayOx = value / 100 }
                    Text { text: "Offset Y"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: -200; to: 200; stepSize: 5; value: arrayOy * 100; onValueChanged: arrayOy = value / 100 }
                    Text { text: "Offset Z"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: -200; to: 200; stepSize: 5; value: arrayOz * 100; onValueChanged: arrayOz = value / 100 }
                }
                AppButton { text: "Linear Array"; height: 26; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10; enabled: objectId >= 0 && arrayCount > 1; onClicked: doArray() }
                AppButton { text: "Radial Array (360\u00b0, Y axis)"; height: 26; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; enabled: objectId >= 0 && arrayCount > 1; onClicked: { if (Modeler.radialArray) Modeler.radialArray(arrayCount, 0, 1, 0, 360) } }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 4
                    rowSpacing: 4
                    Text { text: "Grid W"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { id: gridX; Layout.fillWidth: true; height: 24; from: 2; to: 50; stepSize: 1; value: 4; }
                    Item { width: 8 }
                    Text { text: "Grid H"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { id: gridY; Layout.fillWidth: true; height: 24; from: 2; to: 50; stepSize: 1; value: 3; }
                    Item { width: 8 }
                    Text { text: "Space"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { id: gridS; Layout.fillWidth: true; height: 24; from: -500; to: 500; stepSize: 5; value: 20; }
                }
                AppButton { text: "Grid Array"; height: 26; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; enabled: objectId >= 0; onClicked: { if (Modeler.gridArray) Modeler.gridArray(gridX.value, gridY.value, gridS.value / 100, 0, 0) } }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                Text { text: "NURBS SURFACE"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 2 }
                Text { text: "Create a real NURBS surface (Cox-de Boor) from a control-point grid, then tessellate to mesh."; color: "#888"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 4
                    rowSpacing: 4
                    Text { text: "Rows U"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: 2; to: 20; stepSize: 1; value: nurbsRows; onValueChanged: nurbsRows = value }
                    Item { width: 8 }
                    Text { text: "Cols V"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: 2; to: 20; stepSize: 1; value: nurbsCols; onValueChanged: nurbsCols = value }
                    Item { width: 8 }
                    Text { text: "Height"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: -200; to: 200; stepSize: 5; value: nurbsAmplitude * 100; onValueChanged: nurbsAmplitude = value / 100 }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton { text: "Create Patch"; height: 26; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10; onClicked: createNurbsSurface() }
                    AppButton { text: "Loft Curves"; height: 26; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: loftCurvesToNurbs() }
                }
                Text { text: "Tessellation quality (re-applies to selected NURBS object):"; color: "#888"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    SpinBox { Layout.fillWidth: true; height: 24; from: 4; to: 128; stepSize: 4; value: nurbsUSeg; onValueChanged: nurbsUSeg = value }
                    Text { text: "x"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { Layout.fillWidth: true; height: 24; from: 4; to: 128; stepSize: 4; value: nurbsVSeg; onValueChanged: nurbsVSeg = value }
                    AppButton { text: "Re-tessellate"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.nurbsSurfaceTessellate) Modeler.nurbsSurfaceTessellate(objectId, nurbsUSeg, nurbsVSeg) } }
                }
                Text { text: "CV EDIT (toggle CV overlay in viewport, click a control point, then nudge):"; color: "#888"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    AppButton { text: "Show/Hide CV Overlay"; height: 22; Layout.fillWidth: true; bgcolor: "#2c2c30"; color: "#8ac8ff"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.setNurbsCvVisible) Modeler.setNurbsCvVisible(Modeler.nurbsCvVisible !== undefined ? !Modeler.nurbsCvVisible : true) } }
                }
                Text { text: "CV row / col:"; color: "#888"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    SpinBox { id: cvRow; Layout.fillWidth: true; height: 24; from: 0; to: 31; value: 0; }
                    SpinBox { id: cvCol; Layout.fillWidth: true; height: 24; from: 0; to: 31; value: 0; }
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 4
                    AppButton { text: "-X"; height: 24; bgcolor: "#3e3e42"; color: "#ff8888"; font.pixelSize: 9; onClicked: moveSelectedCVLocal(-1, 0, 0) }
                    AppButton { text: "-Y"; height: 24; bgcolor: "#3e3e42"; color: "#88ff88"; font.pixelSize: 9; onClicked: moveSelectedCVLocal(0, -1, 0) }
                    AppButton { text: "-Z"; height: 24; bgcolor: "#3e3e42"; color: "#8888ff"; font.pixelSize: 9; onClicked: moveSelectedCVLocal(0, 0, -1) }
                    AppButton { text: "+X"; height: 24; bgcolor: "#3e3e42"; color: "#ff8888"; font.pixelSize: 9; onClicked: moveSelectedCVLocal(1, 0, 0) }
                    AppButton { text: "+Y"; height: 24; bgcolor: "#3e3e42"; color: "#88ff88"; font.pixelSize: 9; onClicked: moveSelectedCVLocal(0, 1, 0) }
                    AppButton { text: "+Z"; height: 24; bgcolor: "#3e3e42"; color: "#8888ff"; font.pixelSize: 9; onClicked: moveSelectedCVLocal(0, 0, 1) }
                }
                Text { text: "Extend edge (keeps curvature):"; color: "#888"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 4
                    rowSpacing: 4
                    Text { text: "Distance"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { id: extDist; Layout.fillWidth: true; height: 24; from: -500; to: 500; stepSize: 5; value: 20; }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    AppButton { text: "U-"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.nurbsSurfaceExtend) Modeler.nurbsSurfaceExtend(objectId, 0, extDist.value / 100) } }
                    AppButton { text: "U+"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.nurbsSurfaceExtend) Modeler.nurbsSurfaceExtend(objectId, 1, extDist.value / 100) } }
                    AppButton { text: "V-"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.nurbsSurfaceExtend) Modeler.nurbsSurfaceExtend(objectId, 2, extDist.value / 100) } }
                    AppButton { text: "V+"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.nurbsSurfaceExtend) Modeler.nurbsSurfaceExtend(objectId, 3, extDist.value / 100) } }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    AppButton { text: "Comb U"; height: 22; Layout.fillWidth: true; bgcolor: "#2c2c30"; color: "#7fe07f"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.nurbsSurfaceCurvatureComb) { combObjectId = Modeler.nurbsSurfaceCurvatureComb(objectId, 0, combCount.value, combScale.value / 100) } } }
                    AppButton { text: "Comb V"; height: 22; Layout.fillWidth: true; bgcolor: "#2c2c30"; color: "#7fe07f"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.nurbsSurfaceCurvatureComb) { combObjectId = Modeler.nurbsSurfaceCurvatureComb(objectId, 1, combCount.value, combScale.value / 100) } } }
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 4
                    Text { text: "Comb count"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { id: combCount; Layout.fillWidth: true; height: 24; from: 4; to: 64; stepSize: 4; value: 24; }
                    Text { text: "Scale %"; color: "#888"; font.pixelSize: 10 }
                    SpinBox { id: combScale; Layout.fillWidth: true; height: 24; from: 1; to: 1000; stepSize: 5; value: 10; }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    AppButton { text: "Hide Comb"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#ff8a6a"; font.pixelSize: 9; enabled: combObjectId >= 0; onClicked: { if (Modeler.setObjectVisibility) Modeler.setObjectVisibility(combObjectId, false) } }
                    AppButton { text: "Show Comb"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#8affaa"; font.pixelSize: 9; enabled: combObjectId >= 0; onClicked: { if (Modeler.setObjectVisibility) Modeler.setObjectVisibility(combObjectId, true) } }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                Text { text: "DIMENSIONS"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 2 }
                Text { text: "Sub-object select vertices, then add distance dimension."; color: "#888"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton { text: "Sub Obj: V"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { if (Modeler.setSubobjectMode) Modeler.setSubobjectMode(0) } }
                    AppButton { text: "E"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { if (Modeler.setSubobjectMode) Modeler.setSubobjectMode(1) } }
                    AppButton { text: "F"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { if (Modeler.setSubobjectMode) Modeler.setSubobjectMode(2) } }
                    AppButton { text: "Obj"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { if (Modeler.setSubobjectMode) Modeler.setSubobjectMode(3) } }
                    AppButton { text: "B"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { if (Modeler.setSubobjectMode) Modeler.setSubobjectMode(5) } }
                    AppButton { text: "El"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { if (Modeler.setSubobjectMode) Modeler.setSubobjectMode(6) } }
                }
                AppButton { text: "Add Distance Dimension (V0-V1)"; height: 26; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10; enabled: objectId >= 0; onClicked: { if (Modeler.addDistanceDimension) Modeler.addDistanceDimension(0, 1, "D", objectId) } }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton { text: "Add Radius"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.addRadiusDimension) Modeler.addRadiusDimension(0, [], "R", objectId) } }
                    AppButton { text: "Add Diameter"; height: 24; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; enabled: objectId >= 0; onClicked: { if (Modeler.addDiameterDimension) Modeler.addDiameterDimension(0, [], "Dia", objectId) } }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton { text: "Refresh"; height: 22; Layout.fillWidth: true; bgcolor: "#2c2c30"; color: "#aaa"; font.pixelSize: 9; onClicked: refreshDimensions() }
                    AppButton { text: "Clear All"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { if (Modeler.clearDimensions) Modeler.clearDimensions(); refreshDimensions() } }
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(dimensionsModel.count * 22, 88)
                    model: dimensionsModel
                    visible: dimensionsModel.count > 0
                    clip: true
                    delegate: RowLayout {
                        width: parent.width
                        spacing: 4
                        Text {
                            text: dimLabel + ": " + (dimValue ? Number(dimValue).toFixed(3) : "n/a")
                            color: dimActive ? "#9f9" : "#666"
                            font.pixelSize: 9
                            Layout.fillWidth: true
                        }
                        AppButton {
                            text: dimActive ? "Hide" : "Show"
                            height: 20
                            width: 30
                            bgcolor: "#2c2c30"
                            color: dimActive ? "#aaa" : "#8affaa"
                            font.pixelSize: 8
                            onClicked: { if (Modeler.setDimensionVisible) Modeler.setDimensionVisible(dimType, dimIndex, !dimActive); refreshDimensions() }
                        }
                        AppButton {
                            text: "X"
                            height: 20
                            width: 24
                            bgcolor: "#3e3e42"
                            color: "#fff"
                            font.pixelSize: 10
                            onClicked: { if (Modeler.removeDimension) Modeler.removeDimension(dimType, dimIndex); refreshDimensions() }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                Text { text: "LIVE INSTANCES"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 2 }
                Text { text: "Create lightweight copies that share the master mesh. Edits to the master propagate to every instance."; color: "#888"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    AppButton { text: "Make Instance"; height: 26; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10; enabled: objectId >= 0; onClicked: { if (Modeler.createInstance) Modeler.createInstance(objectId); refreshInstances() } }
                    AppButton { text: "Realize"; height: 26; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; enabled: objectId >= 0; onClicked: { if (Modeler.realizeInstance) Modeler.realizeInstance(objectId); refreshInstances() } }
                }
                Rectangle { height: 1; color: "#2a2a2a"; Layout.fillWidth: true }
                Text { text: objectId >= 0 && Modeler ? ((Modeler.isInstance(objectId) ? "This object IS an instance of master " + Modeler.masterOfInstance(objectId) : "Master instances: " + Modeler.instanceCount(objectId)) + "  |  count: " + Modeler.instanceCount(objectId)) : ""; color: "#888"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                ListModel {
                    id: instancesModel
                }
                function syncInstancesModel() {
                    var m = instancesModel
                    m.clear()
                    for (var i = 0; i < instancesList.length; ++i)
                        m.append({ "instanceId": instancesList[i] })
                }
                Component.onCompleted: syncInstancesModel()
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(instancesModel.count * 24, 96)
                    model: instancesModel
                    visible: instancesModel.count > 0
                    clip: true
                    delegate: RowLayout {
                        width: parent.width
                        spacing: 4
                        AppButton {
                            text: "Instance " + instanceId
                            height: 22
                            Layout.fillWidth: true
                            bgcolor: "#2c2c30"
                            color: "#ddd"
                            font.pixelSize: 9
                            onClicked: { if (Modeler.selectObject) Modeler.selectObject(instanceId) }
                        }
                        AppButton {
                            text: "X"
                            height: 22
                            width: 26
                            bgcolor: "#3e3e42"
                            color: "#fff"
                            font.pixelSize: 10
                            onClicked: { if (Modeler.deleteInstance) Modeler.deleteInstance(instanceId); refreshInstances() }
                        }
                    }
                }
                AppButton { text: "Refresh"; height: 22; Layout.fillWidth: true; bgcolor: "#2c2c30"; color: "#aaa"; font.pixelSize: 9; visible: instancesModel.count > 0; onClicked: refreshInstances() }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                Text { text: "EXPORT"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 2 }
                Text { text: "Hidden-line technical SVG (visible edges solid, hidden dashed)."; color: "#888"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 4
                    rowSpacing: 4
                    Text { text: "Base path"; color: "#888"; font.pixelSize: 10 }
                    TextField {
                        Layout.fillWidth: true; height: 24
                        text: exportPath
                        color: "#ddd"
                        background: Rectangle { color: "#2a2a2e"; radius: 3 }
                        placeholderText: "e.g. C:/tmp/ks"
                        placeholderTextColor: "#555"
                        onTextChanged: exportPath = text
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    AppButton { text: "View X"; height: 22; Layout.fillWidth: true; bgcolor: exportViewAxis === 0 ? "#E10600" : "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: exportViewAxis = 0 }
                    AppButton { text: "View Y"; height: 22; Layout.fillWidth: true; bgcolor: exportViewAxis === 1 ? "#E10600" : "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: exportViewAxis = 1 }
                    AppButton { text: "View Z"; height: 22; Layout.fillWidth: true; bgcolor: exportViewAxis === 2 ? "#E10600" : "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: exportViewAxis = 2 }
                }
                AppButton { text: "Export Hidden-Line SVG"; height: 26; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10; enabled: objectId >= 0; onClicked: doExportSVG() }

                Item { height: 8 }
            }
        }
    }
}