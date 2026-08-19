import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick3D 6.5
import Qt5Compat.GraphicalEffects
import ksEditor.Modeler 1.0
import "../modules/3DModeling"

Rectangle {
    id: pageKsModeler
    objectName: "pageModelerRoot"
    width: 1280
    height: 720
    color: "#111111"
    focus: true

    property string currentFile: "untitled.ks3d"
    property real baseUiScale: 1.18
    property real uiZoom: 1.0
    property real uiScale: baseUiScale * uiZoom
    property int viewMode: 0
    property string selectedObject: ""
    property int activeViewport: 3
    property bool quadView: false
    property bool editMode: Modeler ? Modeler.curveCvEdit : false

    property real camDistance: Modeler.camDistance
    property real camTheta: Modeler.camTheta
    property real camPhi: Modeler.camPhi
    property real camTargetX: Modeler.camTargetX
    property real camTargetY: Modeler.camTargetY
    property real camTargetZ: Modeler.camTargetZ

    property var sceneModel: Modeler ? Modeler.sceneModel : null
    property int totalVerts: 0
    property int totalTris: 0
    property int dragAxis: -1
    property int sculptMode: -1
    property real sculptRadius: 0.3
    property real sculptStrength: 0.2
    property real sculptFalloff: 2.0
    property bool cutMode: false
    property var cutStart: null
    property bool seamMode: false
    property bool animLoop: true

    property real acTrackWidth: 12.0
    property real acTrackCamber: 0.0
    property int acSmoothIterations: 3
    property real acTerrainSize: 200
    property real acTerrainHeight: 20

    property string envHDRSource: Modeler && Modeler.environmentHDR !== undefined ? Modeler.environmentHDR : ""

    property bool canUndo: Modeler.canUndo ? Modeler.canUndo() : false
    property bool canRedo: Modeler.canRedo ? Modeler.canRedo() : false

    property var viewportNames: ["Top", "Front", "Right", "User"]
    property var viewportViewModes: ["top", "front", "right", "persp"]

    property var selectedObj: Modeler ? Modeler.selectedObject : null
    property int iceAliveCount: 0
    property var iceParticlePositions: []
    property var iceParticleColors: []
    property var iceParticleSizes: []
    property int iceRenderObjectId: -1

    property bool snapGrid: true
    property bool snapOrtho: false
    property bool snapOsnap: false
    property bool snapPlanar: false
    property string commandHistory: ""
    property string commandEcho: "Ready"
    property int activeRibbonTab: 0

    property var ribbonDefs: [
        { title: "File", groups: [
            { name: "File", buttons: [
                { label: "New", icon: "\u2795", cmd: "new" },
                { label: "Open", icon: "\u2601", cmd: "open" },
                { label: "Save", icon: "\u2913", cmd: "save" },
                { label: "Import", icon: "\u2B07", cmd: "import" },
                { label: "Export", icon: "\u2B06", cmd: "export" }
            ]},
            { name: "History", buttons: [
                { label: "Undo", icon: "\u21B6", cmd: "undo" },
                { label: "Redo", icon: "\u21B7", cmd: "redo" }
            ]}
        ]},
        { title: "Edit", groups: [
            { name: "Gizmo", buttons: [
                { label: "Select", icon: "\u25CE", cmd: "select", mode: 0 },
                { label: "Move", icon: "\u2194", cmd: "move", mode: 1 },
                { label: "Rotate", icon: "\u21BB", cmd: "rotate", mode: 2 },
                { label: "Scale", icon: "\u2195", cmd: "scale", mode: 3 }
            ]},
            { name: "Clipboard", buttons: [
                { label: "Cut", icon: "\u2702", cmd: "cut" },
                { label: "Copy", icon: "\u2398", cmd: "copy" },
                { label: "Paste", icon: "\u2399", cmd: "paste" }
            ]},
            { name: "Selection", buttons: [
                { label: "All", icon: "\u2606", cmd: "selectall" },
                { label: "None", icon: "\u2606", cmd: "deselect" },
                { label: "Delete", icon: "\u2715", cmd: "delete" }
            ]},
            { name: "3D Print", buttons: [
                { label: "Check", icon: "\u2714", cmd: "printcheck" },
                { label: "Export STL", icon: "\u21E3", cmd: "exportstl" },
                { label: "Scale", icon: "\u2195", cmd: "printscale" },
                { label: "Hollow", icon: "\u25CB", cmd: "printhollow" },
                { label: "Supports", icon: "\u2191", cmd: "printsupports" },
                { label: "Slice", icon: "\u25A0", cmd: "printslice" }
            ]},
        { name: "Print", buttons: [
                { label: "Print Scene", icon: "\u2605", cmd: "print" },
                { label: "3D Print...", icon: "\u2605", cmd: "3dprint" }
            ]}
        ]},
        { title: "Create", groups: [
            { name: "Primitives", buttons: [
                { label: "Box", icon: "\u25A8", cmd: "box" },
                { label: "Sphere", icon: "\u25C9", cmd: "sphere" },
                { label: "Cylinder", icon: "\u25E0", cmd: "cylinder" },
                { label: "Cone", icon: "\u25BC", cmd: "cone" },
                { label: "Torus", icon: "\u25F0", cmd: "torus" },
                { label: "Plane", icon: "\u25A1", cmd: "plane" }
            ]}
        ]},
        { title: "View", groups: [
            { name: "Display", buttons: [
                { label: "Wireframe", icon: "\u25A2", cmd: "wireframe" },
                { label: "Shaded", icon: "\u25C6", cmd: "shaded" },
                { label: "Textured", icon: "\u25C4", cmd: "textured" }
            ]},
            { name: "Views", buttons: [
                { label: "Top", icon: "T", cmd: "view0" },
                { label: "Front", icon: "F", cmd: "view1" },
                { label: "Right", icon: "R", cmd: "view2" },
                { label: "Persp", icon: "P", cmd: "view3" }
            ]}
        ]},
        { title: "Modify", groups: [
            { name: "Boolean", buttons: [
                { label: "Bool Ops", icon: "\u2296", cmd: "boolop" },
                { label: "Cut Tools", icon: "\u2756", cmd: "cuttools" },
                { label: "Weld", icon: "\u2695", cmd: "weld" },
                { label: "Symmetry", icon: "\u2194", cmd: "symmetry" }
            ]},
            { name: "Curves", buttons: [
                { label: "Curve Editor", icon: "\u1F4C", cmd: "curves" },
                { label: "F-Curves", icon: "\u223D", cmd: "fcurve" },
                { label: "Mocap", icon: "\u26A1", cmd: "ac_mocap" }
            ]},
            { name: "Visibility", buttons: [
                { label: "Hide", icon: "\u25C6", cmd: "hide", n: "H" },
                { label: "Unhide Face", icon: "\u25A1", cmd: "unhideFace", n: "Ctrl+H" },
                { label: "Unhide All", icon: "\u25A1", cmd: "unhideAllFaces", n: "Shift+H" }
            ]}
        ]},
        { title: "Tools", groups: [
            { name: "Tools", buttons: [
                { label: "Materials", icon: "MAT", cmd: "material" },
                { label: "Paint", icon: "\u270E", cmd: "paint" },
                { label: "Sculpt", icon: "\u25C9", cmd: "sculpt" },
                { label: "Constraints", icon: "\u2692", cmd: "constraints" },
                { label: "Controllers", icon: "\u26A1", cmd: "controllers" },
                { label: "Wire Params", icon: "\u00D7\u2318", cmd: "wires" },
                { label: "Skin Wrap", icon: "\u229B", cmd: "skinwrap" },
                { label: "Light Lister", icon: "\u2600", cmd: "lights" },
                { label: "Dynamics", icon: "\u26AC", cmd: "dynamics" },
                { label: "Cloth", icon: "\u229B", cmd: "cloth" },
                { label: "Hair/Fur", icon: "\u26A1", cmd: "hair" },
                { label: "Vert/Edge Slide", icon: "\u2194", cmd: "vertexedgeslide" },
                { label: "Edge Loop", icon: "\u21E4", cmd: "edgeloop" },
                { label: "Quad Remesh", icon: "\u25A6", cmd: "quadremesh" },
                { label: "Mark Seam", icon: "\u2716", cmd: "seam" },
                { label: "ICE", icon: "\u2726", cmd: "ice" },
                { label: "Shell/Bridge", icon: "\u25C8", cmd: "gaptools" }
            ]}
        ]},
        { title: "Panels", groups: [
            { name: "Panels", buttons: [
                { label: "Outliner", icon: "\u2630", cmd: "outliner" },
                { label: "Props", icon: "\u2699", cmd: "props" },
                { label: "Timeline", icon: "\u23F1", cmd: "timeline" },
                { label: "Modifiers", icon: "\u2263", cmd: "modifierStack" },
                { label: "Selection Sets", icon: "\u2603", cmd: "selectionSets" },
                { label: "Face Groups", icon: "\u25C7", cmd: "faceGroups" },
                { label: "Viewport", icon: "\u2298", cmd: "viewport" },
                { label: "Factories", icon: "\u25A1", cmd: "factories" },
                { label: "NLA", icon: "\u2616", cmd: "nla" },
                { label: "Layers", icon: "\u2564", cmd: "layers" }
            ]}
        ]},
        { title: "Render", groups: [
            { name: "Environment", buttons: [
                { label: "HDRI Env", icon: "\u2606", cmd: "render_hdri" },
                { label: "Clear HDRI", icon: "\u2605", cmd: "render_hdriclear" },
                { label: "Bake", icon: "\u25A3", cmd: "bake" },
                { label: "Node Editor", icon: "\u25A8", cmd: "nodeeditor" }
            ]},
            { name: "Camera", buttons: [
                { label: "Match Camera", icon: "\u25C9", cmd: "camera_match" }
            ]},
            { name: "Output", buttons: [
                { label: "Render File", icon: "\u2B1B", cmd: "ac_renderfile" }
            ]}
        ]},
        { title: "AC", groups: [
            { name: "Track Tools", buttons: [
                { label: "AC Tools", icon: "\u2299", cmd: "actools" },
                { label: "Track Mesh", icon: "T", cmd: "ac_trackmesh" },
                { label: "AI Line", icon: "A", cmd: "ac_ailine" },
                { label: "Smooth", icon: "S", cmd: "ac_smooth" },
                { label: "Terrain", icon: "\u25B2", cmd: "ac_terrain" }
            ]},
            { name: "Geo", buttons: [
                { label: "Import GPX", icon: "G", cmd: "ac_gpximport" },
                { label: "Import KML", icon: "K", cmd: "ac_kmlimport" },
                { label: "Export GPX", icon: "g", cmd: "ac_gpxexport" },
                { label: "Export KML", icon: "k", cmd: "ac_kmlexport" },
                { label: "Export AI", icon: "a", cmd: "ac_aiexport" }
            ]}
        ]}
    ]

    function runCommand(cmd) {
        commandEcho = "Command: " + cmd
        var c = String(cmd).trim().toLowerCase()
        if (c === "new") { if (Modeler.newProject) Modeler.newProject(); currentFile = "untitled.ks3d" }
        else if (c === "open") { openSceneDialog.open() }
        else if (c === "import") { importDialog.open() }
        else if (c === "save") { saveDialog.open() }
        else if (c === "export") { exportDialog.open() }
        else if (c === "undo") { if (Modeler.undo) Modeler.undo() }
        else if (c === "redo") { if (Modeler.redo) Modeler.redo() }
        else if (c === "delete") { if (Modeler.deleteSelected) Modeler.deleteSelected() }
        else if (c === "cut") { if (Modeler.cutSelected) Modeler.cutSelected() }
        else if (c === "copy") { if (Modeler.copySelected) Modeler.copySelected() }
        else if (c === "paste") { if (Modeler.pasteClipboard) Modeler.pasteClipboard() }
        else if (c === "print") { if (Modeler.printScene) Modeler.printScene() }
        else if (c === "3dprint") { if (Modeler.sliceModel) Modeler.sliceModel() }
        else if (c === "printcheck") { if (Modeler.checkMesh) Modeler.checkMesh() }
        else if (c === "exportstl") { exportDialog.open() }
        else if (c === "printscale") { if (Modeler.scaleForPrint) Modeler.scaleForPrint() }
        else if (c === "printhollow") { if (Modeler.hollowMesh) Modeler.hollowMesh() }
        else if (c === "printsupports") { if (Modeler.generateSupports) Modeler.generateSupports() }
        else if (c === "printslice") { if (Modeler.sliceModel) Modeler.sliceModel() }
        else if (c === "selectall") { if (Modeler.selectAll) Modeler.selectAll() }
        else if (c === "deselect") { if (Modeler.deselectAll) Modeler.deselectAll() }
        else if (c === "grid") { if (Modeler.gridVisible !== undefined) Modeler.gridVisible = !Modeler.gridVisible }
        else if (c === "wireframe") { if (Modeler.viewMode !== undefined) Modeler.viewMode = 1 }
        else if (c === "shaded") { if (Modeler.viewMode !== undefined) Modeler.viewMode = 0 }
        else if (c === "textured") { if (Modeler.viewMode !== undefined) Modeler.viewMode = 2 }
        else if (c === "box") { if (Modeler.addPrimitiveCube) Modeler.addPrimitiveCube() }
        else if (c === "sphere") { if (Modeler.addPrimitiveSphere) Modeler.addPrimitiveSphere() }
        else if (c === "cylinder") { if (Modeler.addPrimitiveCylinder) Modeler.addPrimitiveCylinder() }
        else if (c === "cone") { if (Modeler.addPrimitiveCone) Modeler.addPrimitiveCone() }
        else if (c === "torus") { if (Modeler.addPrimitiveTorus) Modeler.addPrimitiveTorus() }
        else if (c === "plane") { if (Modeler.addPrimitivePlane) Modeler.addPrimitivePlane() }
        else if (c === "move") { if (Modeler.setGizmoMode) Modeler.setGizmoMode(1) }
        else if (c === "select") { if (Modeler.setGizmoMode) Modeler.setGizmoMode(0) }
        else if (c === "rotate") { if (Modeler.setGizmoMode) Modeler.setGizmoMode(2) }
        else if (c === "scale") { if (Modeler.setGizmoMode) Modeler.setGizmoMode(3) }
        else if (c === "symmetry") { openSymmetryPanel() }
        else if (c === "modifierStack") { modifierStackOverlay.visible = !modifierStackOverlay.visible }
        else if (c === "curves") { curveOverlay.visible = !curveOverlay.visible }
        else if (c === "fcurve") { fcurveOverlay.visible = !fcurveOverlay.visible }
        else if (c === "boolop") { boolOpOverlay.visible = !boolOpOverlay.visible }
        else if (c === "cuttools") { cutToolsOverlay.visible = !cutToolsOverlay.visible }
        else if (c === "material") { materialEditorOverlay.visible = !materialEditorOverlay.visible }
        else if (c === "paint") { openPaintPanel() }
        else if (c === "sculpt") { sculptOverlay.visible = !sculptOverlay.visible; if (!sculptOverlay.visible) sculptMode = -1 }
        else if (c === "constraints") { constraintsOverlay.visible = !constraintsOverlay.visible }
        else if (c === "controllers") { controllersOverlay.visible = !controllersOverlay.visible }
        else if (c === "wires") { wiresOverlay.visible = !wiresOverlay.visible }
        else if (c === "skinwrap") { skinWrapOverlay.visible = !skinWrapOverlay.visible }
        else if (c === "lights") { lightsOverlay.visible = !lightsOverlay.visible }
        else if (c === "bake") { bakeOverlay.visible = !bakeOverlay.visible }
        else if (c === "nodeeditor") { nodeEditorOverlay.visible = !nodeEditorOverlay.visible }
        else if (c === "dynamics") { dynOverlay.visible = !dynOverlay.visible }
        else if (c === "cloth") { clothOverlay.visible = !clothOverlay.visible }
        else if (c === "hair") { hairOverlay.visible = !hairOverlay.visible }
        else if (c === "nla") { nlaOverlay.visible = !nlaOverlay.visible }
        else if (c === "vertexedgeslide") { vertexEdgeSlideOverlay.visible = !vertexEdgeSlideOverlay.visible }
        else if (c === "edgeloop") { edgeLoopOverlay.visible = !edgeLoopOverlay.visible }
        else if (c === "quadremesh") { quadRemeshOverlay.visible = !quadRemeshOverlay.visible }
        else if (c === "seam") { seamMode = !seamMode; if (seamMode) { cutMode = false; sculptMode = -1 } }
        else if (c === "ice") { iceOverlay.visible = !iceOverlay.visible }
        else if (c === "outliner") { outlinerDock.visible = !outlinerDock.visible }
        else if (c === "props") { propsOverlay.visible = !propsOverlay.visible }
        else if (c === "timeline") { timelineOverlay.visible = !timelineOverlay.visible }
        else if (c === "selectionSets") { selectionSetsOverlay.visible = !selectionSetsOverlay.visible }
        else if (c === "faceGroups") { faceGroupsOverlay.visible = !faceGroupsOverlay.visible }
        else if (c === "gaptools") { gapToolsOverlay.visible = !gapToolsOverlay.visible }
        else if (c === "layers") { layersOverlay.visible = !layersOverlay.visible }
        else if (c === "viewport") { viewportOverlay.visible = !viewportOverlay.visible }
        else if (c === "factories") { factoryOverlay.visible = !factoryOverlay.visible }
        else if (c === "camera_match") { if (Modeler.matchCameraToSelection) Modeler.matchCameraToSelection() }
        else if (c === "group") { if (Modeler.groupSelected) Modeler.groupSelected("Group") }
        else if (c === "ungroup") { if (Modeler.ungroupSelected) Modeler.ungroupSelected() }
        else if (c === "actools") { acToolsOverlay.visible = !acToolsOverlay.visible }
        else if (c === "render_hdri") { hdrImportDialog.open() }
        else if (c === "render_hdriclear") {
            if (Modeler.environmentHDR !== undefined) Modeler.environmentHDR = ""
        }
        else if (c === "ac_trackmesh") {
            if (Modeler.setTrackWidth) Modeler.setTrackWidth(acTrackWidth)
            if (Modeler.setTrackCamber) Modeler.setTrackCamber(acTrackCamber)
            if (Modeler.generateTrackMesh) Modeler.generateTrackMesh()
        }
        else if (c === "ac_ailine") { if (Modeler.generateAILine) Modeler.generateAILine() }
        else if (c === "ac_smooth") { if (Modeler.smoothTrackPoints) Modeler.smoothTrackPoints(acSmoothIterations) }
        else if (c === "ac_terrain") { if (Modeler.generateTerrain) Modeler.generateTerrain(acTerrainSize, acTerrainHeight) }
        else if (c === "ac_gpximport") { gpxImportDialog.open() }
        else if (c === "ac_kmlimport") { kmlImportDialog.open() }
        else if (c === "hide") { if (Modeler.hideFace) Modeler.hideFace() }
        else if (c === "unhideFace") { if (Modeler.unhideFace) Modeler.unhideFace() }
        else if (c === "unhideAllFaces") { if (Modeler.unhideAllFaces) Modeler.unhideAllFaces() }
        else if (c === "ac_gpxexport") { gpxExportDialog.open() }
        else if (c === "ac_kmlexport") { kmlExportDialog.open() }
        else if (c === "ac_aiexport") { aiExportDialog.open() }
        else if (c === "ac_mocap") { mocapImportDialog.open() }
        else if (c === "ac_renderfile") { renderFileDialog.open() }
        else if (c === "view0") { activateViewport(0) }
        else if (c === "view1") { activateViewport(1) }
        else if (c === "view2") { activateViewport(2) }
        else if (c === "view3") { activateViewport(3) }
        else if (c === "weld") { if (Modeler.weldVertices) Modeler.weldVertices(0.01) }
        else if (c === "help") { commandEcho = "KS Modeler \u2014 3D scene editor" }
        else { commandEcho = "Unknown command: " + cmd }
        commandHistory = commandEcho
    }

    function notifySelection(name, id) {
        selectedObject = name
        selectedObj = Modeler ? Modeler.selectedObject : null
        updateTransformFields()
    }

    function updateTransformFields() {
        var obj = selectedObj
        if (obj) {
            posXField.text = obj.position ? obj.position.x.toFixed(3) : "0.000"
            posYField.text = obj.position ? obj.position.y.toFixed(3) : "0.000"
            posZField.text = obj.position ? obj.position.z.toFixed(3) : "0.000"
            rotXField.text = obj.rotation ? obj.rotation.x.toFixed(1) : "0.0"
            rotYField.text = obj.rotation ? obj.rotation.y.toFixed(1) : "0.0"
            rotZField.text = obj.rotation ? obj.rotation.z.toFixed(1) : "0.0"
            sclXField.text = obj.scale ? obj.scale.x.toFixed(3) : "1.000"
            sclYField.text = obj.scale ? obj.scale.y.toFixed(3) : "1.000"
            sclZField.text = obj.scale ? obj.scale.z.toFixed(3) : "1.000"
        }
    }

    function cullVisible(model, camPos) {
        if (model === undefined || model === null) return true
        if (model.objectVisible !== undefined && !model.objectVisible) return false
        if (Modeler && Modeler.cullingEnabled) {
            var p = model.objectWorldPosition
            if (p !== undefined && camPos !== undefined) {
                var dx = p.x - camPos.x
                var dy = p.y - camPos.y
                var dz = p.z - camPos.z
                var d = Math.sqrt(dx*dx + dy*dy + dz*dz)
                if (d > Modeler.cullDistance) return false
            }
        }
        return true
    }

    function updateStats() {
        if (sceneModel) {
            totalVerts = sceneModel.totalVertices || 0
            totalTris = sceneModel.totalTriangles || 0
        }
    }

    function activateViewport(idx) {
        activeViewport = idx
        if (Modeler.setCameraView) Modeler.setCameraView(viewportViewModes[idx])
    }

    function edgeMid(edge) {
        if (!edge || edge.length < 2) return Qt.vector3d(0, 0, 0)
        return Qt.vector3d((edge[0][0] + edge[1][0]) / 2, (edge[0][1] + edge[1][1]) / 2, (edge[0][2] + edge[1][2]) / 2)
    }

    function edgeLen(edge) {
        if (!edge || edge.length < 2) return 1
        var dx = edge[1][0] - edge[0][0], dy = edge[1][1] - edge[0][1], dz = edge[1][2] - edge[0][2]
        return Math.max(0.01, Math.sqrt(dx*dx + dy*dy + dz*dz))
    }

    // Euler rotation (Qt ZYX convention) that aligns the +Y axis to the edge direction.
    function edgeRot(edge) {
        if (!edge || edge.length < 2) return Qt.vector3d(0, 0, 0)
        var dx = edge[1][0] - edge[0][0], dy = edge[1][1] - edge[0][1], dz = edge[1][2] - edge[0][2]
        var len = Math.sqrt(dx*dx + dy*dy + dz*dz)
        if (len < 1e-5) return Qt.vector3d(0, 0, 0)
        var nx = dx / len, ny = dy / len, nz = dz / len
        var ang = Math.acos(Math.max(-1, Math.min(1, ny)))
        if (ang < 1e-5) return Qt.vector3d(0, 0, 0)
        if (ang > Math.PI - 1e-5) return Qt.vector3d(180, 0, 0)
        var axis = Qt.vector3d(nz, 0, -nx)
        var al = axis.length()
        if (al < 1e-5) return Qt.vector3d(0, 0, 0)
        axis = axis.times(1 / al)
        var s = Math.sin(ang / 2)
        var w = Math.cos(ang / 2), x = axis.x * s, y = axis.y * s, z = axis.z * s
        var roll = Math.atan2(2 * (w * x + y * z), 1 - 2 * (x * x + y * y))
        var sinp = 2 * (w * y - z * x)
        var pitch = Math.abs(sinp) >= 1 ? (sinp >= 0 ? Math.PI / 2 : -Math.PI / 2) : Math.asin(sinp)
        var yaw = Math.atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z))
        return Qt.vector3d(roll * 180 / Math.PI, pitch * 180 / Math.PI, yaw * 180 / Math.PI)
    }

    function updateRayTraceCamera() {
        if (!Modeler || Modeler.rayTraceSetCamera === undefined) return
        if (Modeler.camDistance === undefined || Modeler.cameraFov === undefined) return
        var theta = Modeler.camTheta * Math.PI / 180
        var phi = Modeler.camPhi * Math.PI / 180
        var eye = Qt.vector3d(
            Modeler.camTargetX + Modeler.camDistance * Math.cos(theta) * Math.cos(phi),
            Modeler.camTargetY + Modeler.camDistance * Math.sin(phi),
            Modeler.camTargetZ + Modeler.camDistance * Math.sin(theta) * Math.cos(phi))
        Modeler.rayTraceSetCamera(eye.x, eye.y, eye.z,
                                  Modeler.camTargetX, Modeler.camTargetY, Modeler.camTargetZ,
                                  Modeler.cameraFov)
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Z && event.modifiers & Qt.ControlModifier) {
            if (event.modifiers & Qt.ShiftModifier) {
                if (Modeler.redo) Modeler.redo()
            } else {
                if (Modeler.undo) Modeler.undo()
            }
            event.accepted = true
        }
    }

    Connections {
        target: Modeler
        function onSceneChanged() {
            if (sceneModel) {
                sceneModel.refresh()
                updateStats()
            }
        }
        function onSelectionChanged() {
            selectedObj = Modeler ? Modeler.selectedObject : null
            updateTransformFields()
        }
        function onIceParticlesUpdated(id, count) {
            iceRenderObjectId = id
            iceAliveCount = count
            iceParticlePositions = Modeler.iceGetPositions(id)
            iceParticleColors = Modeler.iceGetColors(id)
            iceParticleSizes = Modeler.iceGetSizes(id)
        }
        function onCameraChanged() {
            updateRayTraceCamera()
        }
        function onRayTraceEnabledChanged() {
            updateRayTraceCamera()
            if (rtOverlay) {
                rtOverlay.visible = Modeler.rayTraceEnabled
                var img = rtOverlay.children[0]
                if (img && img.width > 0) Modeler.rayTraceSetSize(img.width, img.height)
            }
        }
    }

    function setupViewportMouse(mouseArea, vpIdx) {
        var dx = mouseArea.mouseX - mouseArea._lastPos.x
        var dy = mouseArea.mouseY - mouseArea._lastPos.y
        mouseArea._lastPos = Qt.point(mouseArea.mouseX, mouseArea.mouseY)

        if (vpIdx === 3) {
            if (dragAxis >= 0 && (mouseArea.pressedButtons & Qt.LeftButton)) {
                var gScale = camDistance * 0.005
                var gx = 0, gy = 0, gz = 0
                if (dragAxis === 0) gx = dx * gScale
                else if (dragAxis === 1) gy = dy * gScale
                else if (dragAxis === 2) gz = dx * gScale
                var mode = Modeler.gizmoMode !== undefined ? Modeler.gizmoMode : 0
                var cvEdit = Modeler.curveCvEdit !== undefined ? Modeler.curveCvEdit : false
                var cvSel = Modeler.curveSelectedCV !== undefined ? Modeler.curveSelectedCV : -1
                if (cvEdit && cvSel >= 0 && mode === 1 && Modeler.translateSelectedCV) {
                    Modeler.translateSelectedCV(gx, gy, gz)
                }
                else if (mode === 1) { if (Modeler.translateSelected) Modeler.translateSelected(gx, gy, gz) }
                else if (mode === 2) { if (Modeler.rotateSelected) Modeler.rotateSelected(gx * 90, gy * 90, gz * 90) }
                else if (mode === 3) {
                    var sScale = 1 + dx * 0.01
                    if (Modeler.scaleSelected) Modeler.scaleSelected(
                        dragAxis === 0 ? sScale : 1, dragAxis === 1 ? sScale : 1, dragAxis === 2 ? sScale : 1)
                }
            } else if (mouseArea.pressedButtons & Qt.LeftButton) {
                if (Modeler.camTheta !== undefined) Modeler.camTheta = camTheta - dx * 0.5
                if (Modeler.camPhi !== undefined) Modeler.camPhi = Math.max(-89, Math.min(89, camPhi + dy * 0.5))
            } else if (mouseArea.pressedButtons & Qt.MiddleButton) {
                var panSpeed = camDistance * 0.002
                if (Modeler.camTargetX !== undefined) Modeler.camTargetX = camTargetX - dx * panSpeed
                if (Modeler.camTargetY !== undefined) Modeler.camTargetY = camTargetY + dy * panSpeed
            } else if (mouseArea.pressedButtons & Qt.RightButton) {
                if (Modeler.camDistance !== undefined) Modeler.camDistance = Math.max(0.5, Math.min(100, camDistance * (1 + dy * 0.01)))
            }
        } else {
            if (mouseArea.pressedButtons & (Qt.LeftButton | Qt.MiddleButton)) {
                var panSpeed2 = camDistance * 0.002
                if (Modeler.camTargetX !== undefined) Modeler.camTargetX = camTargetX - dx * panSpeed2
                if (Modeler.camTargetY !== undefined) Modeler.camTargetY = camTargetY + dy * panSpeed2
            } else if (mouseArea.pressedButtons & Qt.RightButton) {
                if (Modeler.camDistance !== undefined) Modeler.camDistance = Math.max(0.5, Math.min(100, camDistance * (1 + dy * 0.01)))
            }
        }
    }

    Item {
        width: parent.width / uiScale
        height: parent.height / uiScale
        scale: uiScale
        transformOrigin: Item.TopLeft

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            color: "#2d2d30"
            Layout.fillWidth: true
            border.color: "#3f3f46"
            border.width: 1
            height: ribbonTabs.Layout.preferredHeight + ribbonContent.Layout.preferredHeight

            MouseArea {
                anchors.fill: parent
                onPressed: { if (winBridge) winBridge.beginMove() }
                z: -1
            }

            ColumnLayout {
                id: ribbonBar
                anchors.fill: parent
                spacing: 0

                RowLayout {
                    id: ribbonTabs
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    spacing: 0

                    Rectangle {
                        width: 40; Layout.fillHeight: true
                        color: "#1f1f22"
                        MouseArea {
                            anchors.fill: parent
                            onPressed: { if (winBridge) winBridge.beginMove() }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: "KS"
                            color: "#E10600"; font.pixelSize: 13; font.bold: true
                        }
                    }

                    Repeater {
                        model: ribbonDefs
                        delegate: Rectangle {
                            Layout.fillHeight: true
                            width: Math.max(ribbonTabTxt.implicitWidth + 20, 64)
                            color: activeRibbonTab === index ? "#3e3e42" : (ribbonTabHover.containsMouse ? "#333336" : "transparent")
                            border.color: activeRibbonTab === index ? "#569cd6" : "transparent"
                            border.width: 1
                            Text {
                                id: ribbonTabTxt
                                anchors.centerIn: parent
                                text: modelData.title
                                color: activeRibbonTab === index ? "#569cd6" : "#ccc"
                                font.pixelSize: 11; font.bold: activeRibbonTab === index
                            }
                            MouseArea { id: ribbonTabHover; anchors.fill: parent; hoverEnabled: true
                                onClicked: activeRibbonTab = index
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        width: 120; height: 22; radius: 3; color: "#18181b"; border.color: "#3f3f46"; border.width: 1
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideMiddle
                            text: currentFile
                            color: "#aaa"; font.pixelSize: 9
                        }
                    }

                    Rectangle { width: 1; height: 22; color: "#3f3f46"; anchors.verticalCenter: parent.verticalCenter }

                    Repeater {
                        model: [
                            { icon: "\u003f", tip: "Help", act: function(){ if (winBridge) winBridge.showHelp() } },
                            { icon: "\u2013", tip: "Minimize", act: function(){ if (winBridge) winBridge.minimize() } },
                            { icon: "\u2750", tip: "Maximize", act: function(){ if (winBridge) winBridge.toggleMaximize() } },
                            { icon: "\u2715", tip: "Close", act: function(){ if (winBridge) winBridge.closeWindow() } }
                        ]
                        delegate: Rectangle {
                            width: 42; height: 32
                            Layout.fillHeight: true
                            color: (modelData.icon === "\u2715" && wbBtn.containsMouse) ? "#E81123"
                                 : wbBtn.containsMouse ? "#3e3e42" : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: modelData.icon; color: "#ccc"; font.pixelSize: 12
                            }
                            MouseArea { id: wbBtn; anchors.fill: parent; hoverEnabled: true
                                onClicked: modelData.act()
                            }
                            ToolTip { visible: wbBtn.containsMouse; text: modelData.tip }
                        }
                    }
                }

                RowLayout {
                    id: ribbonContent
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    spacing: 0

                    Repeater {
                        model: ribbonDefs[activeRibbonTab] ? ribbonDefs[activeRibbonTab].groups : []
                        delegate: Item {
                            Layout.fillHeight: true
                            Layout.leftMargin: index > 0 ? 10 : 0
                            width: Math.max(rgName.implicitWidth + 16, ribbonGroupRow.width + 8)

                            Rectangle {
                                visible: index > 0
                                width: 1; height: parent.height - 6
                                color: "#3f3f46"
                                anchors.left: parent.left
                                anchors.leftMargin: -5
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            RowLayout {
                                id: ribbonGroupRow
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.topMargin: 2
                                height: 46
                                spacing: 1

                                Repeater {
                                    model: modelData.buttons
                                    delegate: Rectangle {
                                        width: 46; height: 44
                                        radius: 2
                                        color: rbBtnHover.containsMouse ? "#3e3e42" : "transparent"
                                        border.color: modelData.mode !== undefined && Modeler.gizmoMode === modelData.mode ? "#569cd6" : "transparent"
                                        border.width: 1
                                        Column {
                                            anchors.centerIn: parent; spacing: 2
                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                text: modelData.icon
                                                color: modelData.mode !== undefined && Modeler.gizmoMode === modelData.mode ? "#569cd6" : "#bbb"
                                                font.pixelSize: 16
                                            }
                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                text: modelData.label
                                                color: "#999"; font.pixelSize: 8
                                            }
                                        }
                                        MouseArea { id: rbBtnHover; anchors.fill: parent; hoverEnabled: true
                                            onClicked: runCommand(modelData.cmd)
                                        }
                                    }
                                }
                            }

                            Text {
                                id: rgName
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 1
                                text: modelData.name
                                color: "#888"; font.pixelSize: 8; font.italic: true
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            height: 26
            color: "#1f1f22"
            Layout.fillWidth: true
            border.color: "#3f3f46"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 8

                Text {
                    text: "Command:"
                    color: "#569cd6"; font.pixelSize: 11; font.bold: true
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 18
                    radius: 2
                    color: "#18181b"
                    border.color: "#3f3f46"
                    border.width: 1
                    TextInput {
                        id: commandInput
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        verticalAlignment: Text.AlignVCenter
                        color: "#e0e0e0"; font.pixelSize: 11
                        clip: true
                        Keys.onReturnPressed: { runCommand(commandInput.text); commandInput.text = "" }
                        Keys.onEnterPressed: { runCommand(commandInput.text); commandInput.text = "" }
                    }
                }
                Text {
                    text: commandEcho
                    color: "#10b981"; font.pixelSize: 10
                    elide: Text.ElideRight
                    Layout.maximumWidth: 320
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                width: 44
                Layout.fillHeight: true
                color: "#242427"
                border.color: "#2d2d30"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "TOOLS"
                        color: "#5a5a60"; font.pixelSize: 8; font.bold: true
                    }

                    Repeater {
                        model: [
                            { icon: "\u25CE", mode: 0, tip: "Select (Q)", railGroup: "gizmo" },
                            { icon: "\u2194", mode: 1, tip: "Move (W)", railGroup: "gizmo" },
                            { icon: "\u21BB", mode: 2, tip: "Rotate (E)", railGroup: "gizmo" },
                            { icon: "\u2195", mode: 3, tip: "Scale (R)", railGroup: "gizmo" }
                        ]
                        delegate: Rectangle {
                            Layout.preferredWidth: 34; Layout.preferredHeight: 30
                            Layout.alignment: Qt.AlignHCenter
                            radius: 3
                            color: Modeler.gizmoMode === modelData.mode ? "#264f78" : (railGizmo.containsMouse ? "#3e3e42" : "transparent")
                            border.color: Modeler.gizmoMode === modelData.mode ? "#569cd6" : "transparent"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData.icon; color: Modeler.gizmoMode === modelData.mode ? "#569cd6" : "#ccc"
                                font.pixelSize: 14
                            }
                            MouseArea { id: railGizmo; anchors.fill: parent; hoverEnabled: true
                                onClicked: { if (Modeler.setGizmoMode) Modeler.setGizmoMode(modelData.mode) }
                            }
                            ToolTip { visible: railGizmo.containsMouse; text: modelData.tip }
                        }
                    }

                    Rectangle { Layout.preferredHeight: 1; Layout.fillWidth: true; Layout.leftMargin: 5; Layout.rightMargin: 5; color: "#2d2d30" }

                    Repeater {
                        model: [
                            { icon: "\u25A8", cmd: "box", tip: "Box" },
                            { icon: "\u25C9", cmd: "sphere", tip: "Sphere" },
                            { icon: "\u25E0", cmd: "cylinder", tip: "Cylinder" },
                            { icon: "\u25BC", cmd: "cone", tip: "Cone" },
                            { icon: "\u25F0", cmd: "torus", tip: "Torus" },
                            { icon: "\u25A1", cmd: "plane", tip: "Plane" }
                        ]
                        delegate: Rectangle {
                            Layout.preferredWidth: 34; Layout.preferredHeight: 30
                            Layout.alignment: Qt.AlignHCenter
                            radius: 3
                            color: railPrim.containsMouse ? "#3e3e42" : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: modelData.icon; color: "#bbb"
                                font.pixelSize: 14
                            }
                            MouseArea { id: railPrim; anchors.fill: parent; hoverEnabled: true
                                onClicked: runCommand(modelData.cmd)
                            }
                            ToolTip { visible: railPrim.containsMouse; text: modelData.tip }
                        }
                    }

                    Rectangle { Layout.preferredHeight: 1; Layout.fillWidth: true; Layout.leftMargin: 5; Layout.rightMargin: 5; color: "#2d2d30" }

                    Repeater {
                        model: [
                            { icon: "\u2194", cmd: "symmetry", tip: "Symmetry" },
                            { icon: "MAT", cmd: "material", tip: "Materials" },
                            { icon: "\u21B6", cmd: "undo", tip: "Undo" },
                            { icon: "\u21B7", cmd: "redo", tip: "Redo" }
                        ]
                        delegate: Rectangle {
                            Layout.preferredWidth: 34; Layout.preferredHeight: 30
                            Layout.alignment: Qt.AlignHCenter
                            radius: 3
                            color: railTgl.containsMouse ? "#3e3e42" : "transparent"
                            border.color: modelData.active === true ? "#569cd6" : "transparent"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData.icon; color: modelData.active === true ? "#569cd6" : "#bbb"
                                font.pixelSize: 14
                            }
                            MouseArea { id: railTgl; anchors.fill: parent; hoverEnabled: true
                                onClicked: runCommand(modelData.cmd)
                            }
                            ToolTip { visible: railTgl.containsMouse; text: modelData.tip }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#111111"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                        color: "#1f1f22"
                        border.color: "#2d2d30"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 2
                            spacing: 2

                            ComboBox {
                                id: editModeCombo
                                Layout.preferredWidth: 108
                                Layout.preferredHeight: 20
                                model: ["Object Mode", "Edit Mode"]
                                currentIndex: pageKsModeler.editMode ? 1 : 0
                                onActivated: {
                                    if (Modeler.curveCvEdit !== undefined)
                                        Modeler.curveCvEdit = currentIndex === 1
                                    commandEcho = currentIndex === 1 ? "Edit mode" : "Object mode"
                                }
                                contentItem: Text {
                                    leftPadding: 8
                                    rightPadding: 20
                                    text: editModeCombo.displayText
                                    color: "#ccc"
                                    font.pixelSize: 9
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                                indicator: Text {
                                    x: editModeCombo.width - width - 7
                                    y: (editModeCombo.height - height) / 2
                                    text: "\u25be"
                                    color: "#999"
                                    font.pixelSize: 10
                                }
                                background: Rectangle {
                                    radius: 2
                                    color: editModeCombo.hovered ? "#2d2d30" : "#252528"
                                    border.color: editModeCombo.activeFocus ? "#569cd6" : "#3f3f46"
                                    border.width: 1
                                }
                                ToolTip { visible: editModeCombo.hovered; text: "Choose object or edit mode" }
                            }

                            Repeater {
                                model: viewportNames
                                delegate: Rectangle {
                                    Layout.preferredWidth: 78; Layout.preferredHeight: 20
                                    radius: 2
                                    color: vpTabHover.containsMouse || activeViewport === index ? "#2d2d30" : "transparent"
                                    border.color: activeViewport === index ? "#569cd6" : "transparent"
                                    border.width: 1
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        color: activeViewport === index ? "#569cd6" : "#aaa"
                                        font.pixelSize: 9; font.bold: activeViewport === index
                                    }
                                    MouseArea { id: vpTabHover; anchors.fill: parent; hoverEnabled: true
                                        onClicked: activateViewport(index)
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                Layout.rightMargin: 6
                                text: "Tris: " + totalTris
                                color: "#5a5a60"; font.pixelSize: 9
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        anchors.margins: 1
                        columns: quadView ? 2 : 1
                        rows: quadView ? 2 : 1
                        columnSpacing: 1
                        rowSpacing: 1

                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#1a1a1e"
                        visible: quadView || activeViewport === 0
                        border.color: activeViewport === 0 ? "#E10600" : "#2d2d30"
                        border.width: activeViewport === 0 ? 2 : 1

                        View3D {
                            id: vpTop
                            anchors.fill: parent
                            visible: true
                            environment: SceneEnvironment {
                                clearColor: "#1a1a1e"
                                backgroundMode: envHDRSource ? SceneEnvironment.SkyBox : SceneEnvironment.Color
                                antialiasingMode: SceneEnvironment.MSAA
                                antialiasingQuality: SceneMedium
                            }
                            OrthographicCamera {
                                id: camTop
                                position: Qt.vector3d(0, 20, 0)
                                eulerRotation: Qt.vector3d(-90, 0, 0)
                                horizontalMagnification: camDistance * 2
                                verticalMagnification: camDistance * 2
                                clipNear: 0.1
                                clipFar: 1000
                            }
                            DirectionalLight { eulerRotation: Qt.vector3d(-45, 45, 0); brightness: 1.2 }
                            DirectionalLight { brightness: 0.35; eulerRotation: Qt.vector3d(0, 0, 0) }

                            Model {
                                source: "#Rectangle"
                                visible: Modeler.gridVisible !== undefined ? Modeler.gridVisible : true
                                scale: Qt.vector3d(20, 20, 1)
                                eulerRotation: Qt.vector3d(90, 0, 0)
                                position: Qt.vector3d(0, -0.01, 0)
                                materials: [DefaultMaterial {
                                    lighting: DefaultMaterial.NoLighting
                                    diffuseColor: "#2a2a2e"
                                    diffuseMap: Texture { source: "qrc:/modules/3DModeling/grid_512.png" }
                                }]
                            }

                            Model {
                                source: "#Cylinder"
                                scale: Qt.vector3d(0.002, 0.5, 0.002)
                                position: Qt.vector3d(0.5, 0, 0)
                                eulerRotation: Qt.vector3d(0, 0, -90)
                                materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#ff4444" }]
                            }
                            Model {
                                source: "#Cylinder"
                                scale: Qt.vector3d(0.002, 0.5, 0.002)
                                position: Qt.vector3d(0, 0, 0.5)
                                eulerRotation: Qt.vector3d(90, 0, 0)
                                materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#4444ff" }]
                            }

                            Repeater {
                                model: sceneModel
                                delegate: Model {
                                    property int objId: model.objectId
                                    geometry: SceneMeshGeometry {
                                        objectId: objId
                                    }
                                    position: model.objectWorldPosition !== undefined ? model.objectWorldPosition : Qt.vector3d(0, 0, 0)
                                    eulerRotation: model.objectWorldRotation !== undefined ? model.objectWorldRotation : Qt.vector3d(0, 0, 0)
                                    scale: model.objectWorldScale !== undefined ? model.objectWorldScale : Qt.vector3d(1, 1, 1)
                                    visible: cullVisible(model, camTop.position)
                                    materials: [PrincipledMaterial {
                                        baseColor: model.baseColor || "#4488cc"
                                        metalness: model.metallic || 0.0
                                        roughness: model.roughness || 0.5
                                        baseColorMap: model.diffuseTexture ? fabTopDiff : null
                                        normalMap: model.normalTexture ? fabTopNorm : null
                                    }]
                                    Texture {
                                        id: fabTopDiff
                                        source: model.diffuseTexture || ""
                                        enabled: model.diffuseTexture !== undefined && model.diffuseTexture.length > 0
                                    }
                                    Texture {
                                        id: fabTopNorm
                                        source: model.normalTexture || ""
                                        enabled: model.normalTexture !== undefined && model.normalTexture.length > 0
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 4
                            width: lblTop.contentWidth + 10; height: 18
                            color: activeViewport === 0 ? "#E10600" : "#2d2d30"; radius: 2; opacity: 0.9
                            Text { id: lblTop; anchors.centerIn: parent; text: "Top (1)"; color: "#fff"; font.pixelSize: 10; font.bold: true }
                        }

                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                            property point _lastPos
                            onPressed: (mouse) => { activateViewport(0); _lastPos = Qt.point(mouse.x, mouse.y) }
                            onPositionChanged: (mouse) => { setupViewportMouse(this, 0) }
                            onReleased: { dragAxis = -1 }
                            onWheel: (wheel) => { if (Modeler.camDistance !== undefined) Modeler.camDistance = Math.max(0.5, Math.min(100, camDistance * (1 + wheel.angleDelta.y * 0.001))) }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#1a1a1e"
                        visible: quadView || activeViewport === 3
                        border.color: activeViewport === 3 ? "#E10600" : "#2d2d30"
                        border.width: activeViewport === 3 ? 2 : 1

                        View3D {
                            id: vpUser
                            anchors.fill: parent
                            visible: true
                            environment: SceneEnvironment {
                                clearColor: "#1a1a1e"
                                backgroundMode: envHDRSource ? SceneEnvironment.SkyBox : SceneEnvironment.Color
                                antialiasingMode: SceneEnvironment.MSAA
                                antialiasingQuality: SceneMedium
                            }
                            PerspectiveCamera {
                                id: camUser
                                position: Qt.vector3d(
                                    camTargetX + camDistance * Math.cos(camTheta * Math.PI / 180) * Math.cos(camPhi * Math.PI / 180),
                                    camTargetY + camDistance * Math.sin(camPhi * Math.PI / 180),
                                    camTargetZ + camDistance * Math.sin(camTheta * Math.PI / 180) * Math.cos(camPhi * Math.PI / 180)
                                )
                                eulerRotation: {
                                    var dx = camTargetX - position.x;
                                    var dy = camTargetY - position.y;
                                    var dz = camTargetZ - position.z;
                                    var dist = Math.sqrt(dx*dx + dy*dy + dz*dz);
                                    if (dist < 0.001) return Qt.vector3d(0, 0, 0);
                                    var pitch = -Math.asin(dy / dist) * 180 / Math.PI;
                                    var yaw = Math.atan2(dx, dz) * 180 / Math.PI;
                                    return Qt.vector3d(pitch, yaw, 0);
                                }
                                fieldOfView: Modeler && Modeler.cameraFov !== undefined ? Modeler.cameraFov : 60
                                clipNear: 0.1
                                clipFar: 1000
                            }
                            DirectionalLight { eulerRotation: Qt.vector3d(-45, 45, 0); brightness: 1.2 }
                            DirectionalLight { brightness: 0.35; eulerRotation: Qt.vector3d(0, 0, 0) }

                            Model {
                                source: "#Rectangle"
                                visible: Modeler.gridVisible !== undefined ? Modeler.gridVisible : true
                                scale: Qt.vector3d(20, 20, 1)
                                eulerRotation: Qt.vector3d(90, 0, 0)
                                position: Qt.vector3d(0, -0.01, 0)
                                materials: [DefaultMaterial {
                                    lighting: DefaultMaterial.NoLighting
                                    diffuseColor: "#2a2a2e"
                                    diffuseMap: Texture { source: "qrc:/modules/3DModeling/grid_512.png" }
                                }]
                            }

                            Model {
                                source: "#Cylinder"
                                scale: Qt.vector3d(0.02, 2, 0.02)
                                position: Qt.vector3d(1, 0, 0)
                                eulerRotation: Qt.vector3d(0, 0, -90)
                                materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#ff4444" }]
                            }
                            Model {
                                source: "#Cylinder"
                                scale: Qt.vector3d(0.02, 2, 0.02)
                                position: Qt.vector3d(0, 1, 0)
                                materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#44ff44" }]
                            }
                            Model {
                                source: "#Cylinder"
                                scale: Qt.vector3d(0.02, 2, 0.02)
                                position: Qt.vector3d(0, 0, 1)
                                eulerRotation: Qt.vector3d(90, 0, 0)
                                materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#4444ff" }]
                            }

                            Repeater {
                                model: sceneModel
                                delegate: Model {
                                    property int objId: model.objectId
                                    objectName: "obj" + model.objectId
                                    pickable: true
                                    geometry: SceneMeshGeometry {
                                        objectId: objId
                                    }
                                    position: model.objectWorldPosition !== undefined ? model.objectWorldPosition : Qt.vector3d(0, 0, 0)
                                    eulerRotation: model.objectWorldRotation !== undefined ? model.objectWorldRotation : Qt.vector3d(0, 0, 0)
                                    scale: model.objectWorldScale !== undefined ? model.objectWorldScale : Qt.vector3d(1, 1, 1)
                                    visible: cullVisible(model, camUser.position)
                                    materials: [PrincipledMaterial {
                                        baseColor: model.baseColor || "#4488cc"
                                        metalness: model.metallic || 0.0
                                        roughness: model.roughness || 0.5
                                        baseColorMap: model.diffuseTexture ? fabUserDiff : null
                                        normalMap: model.normalTexture ? fabUserNorm : null
                                    }]
                                    Texture {
                                        id: fabUserDiff
                                        source: model.diffuseTexture || ""
                                        enabled: model.diffuseTexture !== undefined && model.diffuseTexture.length > 0
                                    }
                                    Texture {
                                        id: fabUserNorm
                                        source: model.normalTexture || ""
                                        enabled: model.normalTexture !== undefined && model.normalTexture.length > 0
                                    }
                                }
                            }

                            Node {
                                visible: {
                                    var mode = Modeler.gizmoMode !== undefined ? Modeler.gizmoMode : 0
                                    var hasSel = Modeler.hasSelection !== undefined ? Modeler.hasSelection : false
                                    return mode > 0 && hasSel
                                }
                                position: Modeler.gizmoPosition || Qt.vector3d(0, 0, 0)
                                Model { objectName: "gizmoX"; pickable: true; source: "#Cylinder"; position: Qt.vector3d(0.5, 0, 0); scale: dragAxis === 0 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02); eulerRotation: Qt.vector3d(0, 0, -90); materials: [DefaultMaterial { diffuseColor: dragAxis === 0 ? "#ff8888" : "#ff4444" }] }
                                Model { objectName: "gizmoX"; pickable: true; source: "#Cone"; position: Qt.vector3d(1.0, 0, 0); scale: Qt.vector3d(0.06, 0.2, 0.06); eulerRotation: Qt.vector3d(0, 0, -90); materials: [DefaultMaterial { diffuseColor: dragAxis === 0 ? "#ff8888" : "#ff4444" }] }
                                Model { objectName: "gizmoY"; pickable: true; source: "#Cylinder"; position: Qt.vector3d(0, 0.5, 0); scale: dragAxis === 1 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02); materials: [DefaultMaterial { diffuseColor: dragAxis === 1 ? "#88ff88" : "#44ff44" }] }
                                Model { objectName: "gizmoY"; pickable: true; source: "#Cone"; position: Qt.vector3d(0, 1.0, 0); scale: Qt.vector3d(0.06, 0.2, 0.06); materials: [DefaultMaterial { diffuseColor: dragAxis === 1 ? "#88ff88" : "#44ff44" }] }
                                Model { objectName: "gizmoZ"; pickable: true; source: "#Cylinder"; position: Qt.vector3d(0, 0, 0.5); scale: dragAxis === 2 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02); eulerRotation: Qt.vector3d(90, 0, 0); materials: [DefaultMaterial { diffuseColor: dragAxis === 2 ? "#8888ff" : "#4444ff" }] }
                                Model { objectName: "gizmoZ"; pickable: true; source: "#Cone"; position: Qt.vector3d(0, 0, 1.0); scale: Qt.vector3d(0.06, 0.2, 0.06); eulerRotation: Qt.vector3d(90, 0, 0); materials: [DefaultMaterial { diffuseColor: dragAxis === 2 ? "#8888ff" : "#4444ff" }] }

                                Model {
                                    objectName: "cutStartMarker"
                                    visible: cutMode && cutStart !== null
                                    source: "#Sphere"
                                    position: cutStart !== null ? cutStart : Qt.vector3d(0, 0, 0)
                                    scale: Qt.vector3d(0.04, 0.04, 0.04)
                                    materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#E10600" }]
                                }
                            }

                            Node {
                                id: cvMarkerRoot
                                visible: (Modeler.curveCvEdit !== undefined ? Modeler.curveCvEdit : false)
                                         && Modeler.selectedObject !== undefined && Modeler.selectedObject !== null
                                position: Modeler.selectedObject && Modeler.selectedObject.position !== undefined
                                          ? Modeler.selectedObject.position : Qt.vector3d(0, 0, 0)
                                Repeater {
                                    model: cvMarkerRoot.visible ? (Modeler.curveCvPositions ? Modeler.curveCvPositions(Modeler.selectedObject.id) : []) : []
                                    delegate: Model {
                                        objectName: "cv" + index
                                        pickable: true
                                        source: "#Sphere"
                                        position: Qt.vector3d(modelData[0], modelData[1], modelData[2])
                                        scale: Qt.vector3d(0.05, 0.05, 0.05)
                                        materials: [DefaultMaterial {
                                            lighting: DefaultMaterial.NoLighting
                                            diffuseColor: (Modeler.curveSelectedCV !== undefined && Modeler.curveSelectedCV === index) ? "#E10600" : "#ffcc00"
                                        }]
                                    }
                                }
                            }

                            // Edge loop/ring highlight markers (2.3).
                            Node {
                                id: edgeLoopMarkers
                                visible: edgeLoopOverlay.visible && edgeLoopOverlay.Loader.item
                                Repeater {
                                    id: edgeLoopRepeater
                                    model: edgeLoopMarkers.visible && edgeLoopOverlay.Loader.item.loopEdges !== undefined ? edgeLoopOverlay.Loader.item.loopEdges : []
                                    delegate: Model {
                                        source: "#Cylinder"
                                        scale: Qt.vector3d(0.006, edgeLen(edgeLoopOverlay.Loader.item.loopEdges[index]) * 0.5, 0.006)
                                        position: edgeMid(edgeLoopOverlay.Loader.item.loopEdges[index])
                                        eulerRotation: edgeRot(edgeLoopOverlay.Loader.item.loopEdges[index])
                                        materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#E10600" }]
                                    }
                                }
                                Repeater {
                                    model: edgeLoopMarkers.visible && edgeLoopOverlay.Loader.item.ringEdges !== undefined ? edgeLoopOverlay.Loader.item.ringEdges : []
                                    delegate: Model {
                                        source: "#Cylinder"
                                        scale: Qt.vector3d(0.006, edgeLen(edgeLoopOverlay.Loader.item.ringEdges[index]) * 0.5, 0.006)
                                        position: edgeMid(edgeLoopOverlay.Loader.item.ringEdges[index])
                                        eulerRotation: edgeRot(edgeLoopOverlay.Loader.item.ringEdges[index])
                                        materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#ffcc00" }]
                                    }
                                }
                            }

                            Model {
                                objectName: "icePoints"
                                visible: iceOverlay.visible && iceAliveCount > 0 && (Modeler.iceSpheresEnabled === false)
                                pickable: false
                                geometry: ParticlePointsGeometry {
                                    particlePositions: iceParticlePositions
                                    particleColors: iceParticleColors
                                    particleSizes: iceParticleSizes
                                }
                                materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#ffffff" }]
                            }

                            Model {
                                objectName: "iceSpheres"
                                visible: iceOverlay.visible && iceAliveCount > 0 && (Modeler.iceSpheresEnabled !== false)
                                pickable: false
                                source: "#Sphere"
                                instancing: ParticleInstancing {
                                    particlePositions: iceParticlePositions
                                    particleColors: iceParticleColors
                                    particleSizes: iceParticleSizes
                                }
                                materials: [PrincipledMaterial {
                                    baseColor: "#ffffff"
                                    metalness: 0.1
                                    roughness: 0.6
                                }]
                            }
                        }

                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 4
                            width: lblUser.contentWidth + 10; height: 18
                            color: activeViewport === 3 ? "#E10600" : "#2d2d30"; radius: 2; opacity: 0.9
                            Text { id: lblUser; anchors.centerIn: parent; text: "User (4)"; color: "#fff"; font.pixelSize: 10; font.bold: true }
                        }

                        MouseArea {
                            id: userMouseArea
                            anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                            property point _lastPos
                            property bool sculpting: false
                            property int sculptTargetId: -1
                            onPressed: (mouse) => {
                                activateViewport(3)
                                _lastPos = Qt.point(mouse.x, mouse.y)
                                var result = vpUser.pick(mouse.x, mouse.y)
                                if (result.objectHit) {
                                    var name = result.objectHit.objectName
                                    if (name.indexOf("gizmoX") >= 0) dragAxis = 0
                                    else if (name.indexOf("gizmoY") >= 0) dragAxis = 1
                                    else if (name.indexOf("gizmoZ") >= 0) dragAxis = 2
                                    else if (name.indexOf("cv") === 0 && Modeler.curveCvEdit) {
                                        var cvIdx = parseInt(name.substring(2))
                                        if (Modeler.curveSelectedCV !== undefined) Modeler.curveSelectedCV = cvIdx
                                    }
                                    else if (name.indexOf("obj") === 0) {
                                        var id = parseInt(name.substring(3))
                                        if (vertexEdgeSlideOverlay.visible) {
                                            var item = vertexEdgeSlideOverlay.Loader.item
                                            if (item && item.picking && result.position !== undefined) {
                                                var pos = result.scenePosition !== undefined ? result.scenePosition : result.position
                                                item.applyPick(pos.x, pos.y, pos.z)
                                            }
                                        } else if (edgeLoopOverlay.visible) {
                                            var eitem = edgeLoopOverlay.Loader.item
                                            if (eitem && eitem.picking && result.position !== undefined) {
                                                var epos = result.scenePosition !== undefined ? result.scenePosition : result.position
                                                eitem.applyPick(epos.x, epos.y, epos.z)
                                            }
                                        } else if (cutMode && result.position !== undefined) {
                                            var pos = result.scenePosition !== undefined ? result.scenePosition : result.position
                                            if (Modeler.selectObject) Modeler.selectObject(id)
                                            if (cutStart === null) {
                                                cutStart = Qt.vector3d(pos.x, pos.y, pos.z)
                                            } else {
                                                Modeler.knifeCutWorld(id, cutStart.x, cutStart.y, cutStart.z, pos.x, pos.y, pos.z)
                                                cutStart = null
                                            }
                                        } else if (seamMode && result.position !== undefined) {
                                            var seamPos = result.scenePosition !== undefined ? result.scenePosition : result.position
                                            if (Modeler.selectObject) Modeler.selectObject(id)
                                            if (Modeler.markSeamFromClosestEdge) Modeler.markSeamFromClosestEdge(seamPos.x, seamPos.y, seamPos.z)
                                        } else if (sculptMode >= 0 && result.position !== undefined) {
                                            sculpting = true
                                            sculptTargetId = id
                                            if (Modeler.selectObject) Modeler.selectObject(id)
                                            var pos2 = result.scenePosition !== undefined ? result.scenePosition : result.position
                                            _lastSculpt = Qt.vector3d(pos2.x, pos2.y, pos2.z)
                                            Modeler.sculptBrush(id, pos2.x, pos2.y, pos2.z, sculptRadius, sculptStrength, sculptMode, 0, 0, 0, pos2.x, pos2.y, pos2.z, sculptFalloff)
                                        } else if (constraintsOverlay.visible && constraintsOverlay.Loader.item && constraintsOverlay.Loader.item.startAddTarget) {
                                            if (constraintsOverlay.Loader.item.addingTarget) {
                                                constraintsOverlay.Loader.item.confirmAddTarget(id)
                                            } else if (Modeler.selectObject) Modeler.selectObject(id)
                                        } else if (controllersOverlay.visible && controllersOverlay.Loader.item && controllersOverlay.Loader.item.startAddTarget) {
                                            if (controllersOverlay.Loader.item.addingTarget) {
                                                controllersOverlay.Loader.item.confirmAddTarget(id)
                                            } else if (Modeler.selectObject) Modeler.selectObject(id)
                                        } else if (wiresOverlay.visible && wiresOverlay.Loader.item && wiresOverlay.Loader.item.startPickDriver) {
                                            if (wiresOverlay.Loader.item.pickingDriver) {
                                                wiresOverlay.Loader.item.confirmPickDriver(id)
                                            } else if (Modeler.selectObject) Modeler.selectObject(id)
                                        } else if (skinWrapOverlay.visible && skinWrapOverlay.Loader.item && skinWrapOverlay.Loader.item.startPickCage) {
                                            if (skinWrapOverlay.Loader.item.pickingCage) {
                                                skinWrapOverlay.Loader.item.confirmPickCage(id)
                                            } else if (Modeler.selectObject) Modeler.selectObject(id)
                                        } else if (Modeler.subobjectMode && (Modeler.subobjectMode() === 5 || Modeler.subobjectMode() === 6) && result.position !== undefined) {
                                            if (Modeler.selectObject) Modeler.selectObject(id)
                                            var spo = result.scenePosition !== undefined ? result.scenePosition : result.position
                                            if (Modeler.subobjectMode() === 5) {
                                                if (Modeler.selectBorderUnderCursor) Modeler.selectBorderUnderCursor(spo.x, spo.y, spo.z)
                                            } else {
                                                if (Modeler.selectElementUnderCursor) Modeler.selectElementUnderCursor(spo.x, spo.y, spo.z)
                                            }
                                        } else if (Modeler.selectObject) Modeler.selectObject(id)
                                    }
                                }
                            }
                            onPositionChanged: (mouse) => {
                                if (sculpting && sculptMode >= 0 && sculptTargetId >= 0) {
                                    var r = vpUser.pick(mouse.x, mouse.y)
                                    if (r.objectHit && r.position !== undefined) {
                                        var p = r.scenePosition !== undefined ? r.scenePosition : r.position
                                        var dx = p.x - _lastSculpt.x
                                        var dy = p.y - _lastSculpt.y
                                        var dz = p.z - _lastSculpt.z
                                        Modeler.sculptBrush(sculptTargetId, p.x, p.y, p.z, sculptRadius, sculptStrength, sculptMode, dx, dy, dz, _lastSculpt.x, _lastSculpt.y, _lastSculpt.z, sculptFalloff)
                                        _lastSculpt = Qt.vector3d(p.x, p.y, p.z)
                                    }
                                } else setupViewportMouse(this, 3)
                            }
                            onReleased: (mouse) => { dragAxis = -1; sculpting = false; sculptTargetId = -1 }
                            onWheel: (wheel) => { if (Modeler.camDistance !== undefined) Modeler.camDistance = Math.max(0.5, Math.min(100, camDistance * (1 + wheel.angleDelta.y * 0.001))) }
                             property vector3d _lastSculpt: Qt.vector3d(0, 0, 0)
                        }

                        // Raytraced viewport overlay (2.6).
                        Item {
                            id: rtOverlay
                            anchors.fill: parent
                            visible: Modeler.rayTraceEnabled !== undefined ? Modeler.rayTraceEnabled : false

                            Image {
                                anchors.fill: parent
                                fillMode: Image.Stretch
                                smooth: true
                                source: visible ? ("image://raytrace/frame?rev=" + (Modeler.rayTraceFrame !== undefined ? Modeler.rayTraceFrame : 0)) : ""
                                onWidthChanged: { if (visible) Modeler.rayTraceSetSize(width, height) }
                                Component.onCompleted: { if (visible) Modeler.rayTraceSetSize(width, height) }
                            }
                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 34
                                spacing: 4
                                Repeater {
                                    model: ["Color", "Depth", "AO", "Diffuse", "Normal"]
                                    Rectangle {
                                        width: 52; height: 18; radius: 3
                                        color: Modeler.rayTracePass === index ? "#E10600" : "#1a1a1e"
                                        border.color: "#3a3a3f"
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: "#ffffff"; font.pixelSize: 8; font.bold: true
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onClicked: { if (Modeler.rayTracePass !== undefined) Modeler.rayTracePass = index }
                                        }
                                    }
                                }
                            }
                            Rectangle {
                                anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
                                anchors.margins: 8
                                width: 130; height: 20; radius: 4; color: "#1a1a1e"; opacity: 0.85
                                Text {
                                    anchors.centerIn: parent
                                    text: "Raytrace preview"
                                    color: "#E10600"; font.pixelSize: 9; font.bold: true
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#1a1a1e"
                        visible: quadView || activeViewport === 1
                        border.color: activeViewport === 1 ? "#E10600" : "#2d2d30"
                        border.width: activeViewport === 1 ? 2 : 1

                        View3D {
                            id: vpFront
                            anchors.fill: parent
                            visible: true
                            environment: SceneEnvironment {
                                clearColor: "#1a1a1e"
                                backgroundMode: envHDRSource ? SceneEnvironment.SkyBox : SceneEnvironment.Color
                                antialiasingMode: SceneEnvironment.MSAA
                                antialiasingQuality: SceneMedium
                            }
                            OrthographicCamera {
                                id: camFront
                                position: Qt.vector3d(0, 0, 20)
                                eulerRotation: Qt.vector3d(0, 0, 0)
                                horizontalMagnification: camDistance * 2
                                verticalMagnification: camDistance * 2
                                clipNear: 0.1
                                clipFar: 1000
                            }
                            DirectionalLight { eulerRotation: Qt.vector3d(-45, 45, 0); brightness: 1.2 }
                            DirectionalLight { brightness: 0.35; eulerRotation: Qt.vector3d(0, 0, 0) }

                            Model {
                                source: "#Rectangle"
                                visible: Modeler.gridVisible !== undefined ? Modeler.gridVisible : true
                                scale: Qt.vector3d(20, 20, 1)
                                eulerRotation: Qt.vector3d(90, 0, 0)
                                position: Qt.vector3d(0, -0.01, 0)
                                materials: [DefaultMaterial {
                                    lighting: DefaultMaterial.NoLighting
                                    diffuseColor: "#2a2a2e"
                                    diffuseMap: Texture { source: "qrc:/modules/3DModeling/grid_512.png" }
                                }]
                            }

                            Model {
                                source: "#Cylinder"
                                scale: Qt.vector3d(0.002, 0.5, 0.002)
                                position: Qt.vector3d(0.5, 0, 0)
                                eulerRotation: Qt.vector3d(0, 0, -90)
                                materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#ff4444" }]
                            }
                            Model {
                                source: "#Cylinder"
                                scale: Qt.vector3d(0.002, 0.5, 0.002)
                                position: Qt.vector3d(0, 0.5, 0)
                                materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#44ff44" }]
                            }

                            Repeater {
                                model: sceneModel
                                delegate: Model {
                                    property int objId: model.objectId
                                    geometry: SceneMeshGeometry {
                                        objectId: objId
                                    }
                                    position: model.objectWorldPosition !== undefined ? model.objectWorldPosition : Qt.vector3d(0, 0, 0)
                                    eulerRotation: model.objectWorldRotation !== undefined ? model.objectWorldRotation : Qt.vector3d(0, 0, 0)
                                    scale: model.objectWorldScale !== undefined ? model.objectWorldScale : Qt.vector3d(1, 1, 1)
                                    visible: cullVisible(model, camFront.position)
                                    materials: [PrincipledMaterial {
                                        baseColor: model.baseColor || "#4488cc"
                                        metalness: model.metallic || 0.0
                                        roughness: model.roughness || 0.5
                                        baseColorMap: model.diffuseTexture ? fabFrontDiff : null
                                        normalMap: model.normalTexture ? fabFrontNorm : null
                                    }]
                                    Texture {
                                        id: fabFrontDiff
                                        source: model.diffuseTexture || ""
                                        enabled: model.diffuseTexture !== undefined && model.diffuseTexture.length > 0
                                    }
                                    Texture {
                                        id: fabFrontNorm
                                        source: model.normalTexture || ""
                                        enabled: model.normalTexture !== undefined && model.normalTexture.length > 0
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 4
                            width: lblFront.contentWidth + 10; height: 18
                            color: activeViewport === 1 ? "#E10600" : "#2d2d30"; radius: 2; opacity: 0.9
                            Text { id: lblFront; anchors.centerIn: parent; text: "Front (2)"; color: "#fff"; font.pixelSize: 10; font.bold: true }
                        }

                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                            property point _lastPos
                            onPressed: (mouse) => { activateViewport(1); _lastPos = Qt.point(mouse.x, mouse.y) }
                            onPositionChanged: (mouse) => { setupViewportMouse(this, 1) }
                            onReleased: { dragAxis = -1 }
                            onWheel: (wheel) => { if (Modeler.camDistance !== undefined) Modeler.camDistance = Math.max(0.5, Math.min(100, camDistance * (1 + wheel.angleDelta.y * 0.001))) }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#1a1a1e"
                        visible: quadView || activeViewport === 2
                        border.color: activeViewport === 2 ? "#E10600" : "#2d2d30"
                        border.width: activeViewport === 2 ? 2 : 1

                        View3D {
                            id: vpRight
                            anchors.fill: parent
                            visible: true
                            environment: SceneEnvironment {
                                clearColor: "#1a1a1e"
                                backgroundMode: envHDRSource ? SceneEnvironment.SkyBox : SceneEnvironment.Color
                                antialiasingMode: SceneEnvironment.MSAA
                                antialiasingQuality: SceneMedium
                            }
                            OrthographicCamera {
                                id: camRight
                                position: Qt.vector3d(20, 0, 0)
                                eulerRotation: Qt.vector3d(0, 90, 0)
                                horizontalMagnification: camDistance * 2
                                verticalMagnification: camDistance * 2
                                clipNear: 0.1
                                clipFar: 1000
                            }
                            DirectionalLight { eulerRotation: Qt.vector3d(-45, 45, 0); brightness: 1.2 }
                            DirectionalLight { brightness: 0.35; eulerRotation: Qt.vector3d(0, 0, 0) }

                            Model {
                                source: "#Rectangle"
                                visible: Modeler.gridVisible !== undefined ? Modeler.gridVisible : true
                                scale: Qt.vector3d(20, 20, 1)
                                eulerRotation: Qt.vector3d(90, 0, 0)
                                position: Qt.vector3d(0, -0.01, 0)
                                materials: [DefaultMaterial {
                                    lighting: DefaultMaterial.NoLighting
                                    diffuseColor: "#2a2a2e"
                                    diffuseMap: Texture { source: "qrc:/modules/3DModeling/grid_512.png" }
                                }]
                            }

                            Model {
                                source: "#Cylinder"
                                scale: Qt.vector3d(0.002, 0.5, 0.002)
                                position: Qt.vector3d(0, 0.5, 0)
                                materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#44ff44" }]
                            }
                            Model {
                                source: "#Cylinder"
                                scale: Qt.vector3d(0.002, 0.5, 0.002)
                                position: Qt.vector3d(0, 0, 0.5)
                                eulerRotation: Qt.vector3d(90, 0, 0)
                                materials: [DefaultMaterial { lighting: DefaultMaterial.NoLighting; diffuseColor: "#4444ff" }]
                            }

                            Repeater {
                                model: sceneModel
                                delegate: Model {
                                    property int objId: model.objectId
                                    geometry: SceneMeshGeometry {
                                        objectId: objId
                                    }
                                    position: model.objectWorldPosition !== undefined ? model.objectWorldPosition : Qt.vector3d(0, 0, 0)
                                    eulerRotation: model.objectWorldRotation !== undefined ? model.objectWorldRotation : Qt.vector3d(0, 0, 0)
                                    scale: model.objectWorldScale !== undefined ? model.objectWorldScale : Qt.vector3d(1, 1, 1)
                                    visible: cullVisible(model, camRight.position)
                                    materials: [PrincipledMaterial {
                                        baseColor: model.baseColor || "#4488cc"
                                        metalness: model.metallic || 0.0
                                        roughness: model.roughness || 0.5
                                        baseColorMap: model.diffuseTexture ? fabRightDiff : null
                                        normalMap: model.normalTexture ? fabRightNorm : null
                                    }]
                                    Texture {
                                        id: fabRightDiff
                                        source: model.diffuseTexture || ""
                                        enabled: model.diffuseTexture !== undefined && model.diffuseTexture.length > 0
                                    }
                                    Texture {
                                        id: fabRightNorm
                                        source: model.normalTexture || ""
                                        enabled: model.normalTexture !== undefined && model.normalTexture.length > 0
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 4
                            width: lblRight.contentWidth + 10; height: 18
                            color: activeViewport === 2 ? "#E10600" : "#2d2d30"; radius: 2; opacity: 0.9
                            Text { id: lblRight; anchors.centerIn: parent; text: "Right (3)"; color: "#fff"; font.pixelSize: 10; font.bold: true }
                        }

                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                            property point _lastPos
                            onPressed: (mouse) => { activateViewport(2); _lastPos = Qt.point(mouse.x, mouse.y) }
                            onPositionChanged: (mouse) => { setupViewportMouse(this, 2) }
                            onReleased: { dragAxis = -1 }
                            onWheel: (wheel) => { if (Modeler.camDistance !== undefined) Modeler.camDistance = Math.max(0.5, Math.min(100, camDistance * (1 + wheel.angleDelta.y * 0.001))) }
                        }
                    }
                }
            }
        }

        Rectangle {
            width: 180
            Layout.fillHeight: true
            color: "#1e1e1e"
            border.color: "#2d2d30"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4

                Text { text: "TRANSFORM"; color: "#888"; font.pixelSize: 10; font.bold: true; leftPadding: 4 }

                GridLayout {
                    columns: 2; Layout.fillWidth: true; rowSpacing: 2; columnSpacing: 4
                    Text { text: "X:"; color: "#ff4444"; font.pixelSize: 10 }
                    TextField { id: posXField; text: "0.000"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 }
                        onEditingFinished: { if (selectedObj) selectedObj.position = Qt.vector3d(parseFloat(text)||0, selectedObj.position.y, selectedObj.position.z) }
                    }
                    Text { text: "Y:"; color: "#44ff44"; font.pixelSize: 10 }
                    TextField { id: posYField; text: "0.000"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 }
                        onEditingFinished: { if (selectedObj) selectedObj.position = Qt.vector3d(selectedObj.position.x, parseFloat(text)||0, selectedObj.position.z) }
                    }
                    Text { text: "Z:"; color: "#4444ff"; font.pixelSize: 10 }
                    TextField { id: posZField; text: "0.000"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 }
                        onEditingFinished: { if (selectedObj) selectedObj.position = Qt.vector3d(selectedObj.position.x, selectedObj.position.y, parseFloat(text)||0) }
                    }
                }

                GridLayout {
                    columns: 2; Layout.fillWidth: true; rowSpacing: 2; columnSpacing: 4
                    Text { text: "RX:"; color: "#ff4444"; font.pixelSize: 10 }
                    TextField { id: rotXField; text: "0.0"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 }
                        onEditingFinished: { if (selectedObj) selectedObj.rotation = Qt.vector3d(parseFloat(text)||0, selectedObj.rotation.y, selectedObj.rotation.z) }
                    }
                    Text { text: "RY:"; color: "#44ff44"; font.pixelSize: 10 }
                    TextField { id: rotYField; text: "0.0"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 }
                        onEditingFinished: { if (selectedObj) selectedObj.rotation = Qt.vector3d(selectedObj.rotation.x, parseFloat(text)||0, selectedObj.rotation.z) }
                    }
                    Text { text: "RZ:"; color: "#4444ff"; font.pixelSize: 10 }
                    TextField { id: rotZField; text: "0.0"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 }
                        onEditingFinished: { if (selectedObj) selectedObj.rotation = Qt.vector3d(selectedObj.rotation.x, selectedObj.rotation.y, parseFloat(text)||0) }
                    }
                }

                GridLayout {
                    columns: 2; Layout.fillWidth: true; rowSpacing: 2; columnSpacing: 4
                    Text { text: "SX:"; color: "#ff4444"; font.pixelSize: 10 }
                    TextField { id: sclXField; text: "1.000"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 }
                        onEditingFinished: { if (selectedObj) selectedObj.scale = Qt.vector3d(parseFloat(text)||1, selectedObj.scale.y, selectedObj.scale.z) }
                    }
                    Text { text: "SY:"; color: "#44ff44"; font.pixelSize: 10 }
                    TextField { id: sclYField; text: "1.000"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 }
                        onEditingFinished: { if (selectedObj) selectedObj.scale = Qt.vector3d(selectedObj.scale.x, parseFloat(text)||1, selectedObj.scale.z) }
                    }
                    Text { text: "SZ:"; color: "#4444ff"; font.pixelSize: 10 }
                    TextField { id: sclZField; text: "1.000"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 }
                        onEditingFinished: { if (selectedObj) selectedObj.scale = Qt.vector3d(selectedObj.scale.x, selectedObj.scale.y, parseFloat(text)||1) }
                    }
                }

                Rectangle { height: 1; color: "#2d2d30"; Layout.fillWidth: true }
                Text { text: "SELECTION"; color: "#888"; font.pixelSize: 10; font.bold: true; leftPadding: 4 }
                Text { text: selectedObject !== "" ? selectedObject : "(none)"; color: "#aaa"; font.pixelSize: 10; leftPadding: 4 }

                Item { Layout.fillHeight: true }
            }
        }

        Rectangle {
            id: outlinerDock
                width: 240
                Layout.fillHeight: true
                visible: false
                color: "#1e1e1e"
                border.color: "#2d2d30"
                border.width: 1
                clip: true

                Loader {
                    anchors.fill: parent
                    source: "../modules/3DModeling/SceneOutliner.qml"
                    onItemChanged: if (item) item.closeRequested.connect(function() { outlinerDock.visible = false })
                }
            }
        }

        Rectangle {
            height: 26
            color: "#1e1e1e"
            Layout.fillWidth: true
            border.color: "#2d2d30"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                Text {
                    text: "X " + camTargetX.toFixed(2) + "  Y " + camTargetY.toFixed(2) + "  Z " + camTargetZ.toFixed(2)
                    color: "#888"; font.pixelSize: 10
                }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "Dist: " + camDistance.toFixed(1); color: "#888"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }

                Repeater {
                    model: [
                        { label: "Grid", icon: "qrc:/icons/grid.svg", prop: Modeler.gridVisible, set: function(v){ if (Modeler.gridVisible !== undefined) Modeler.gridVisible = v } },
                        { label: "Wireframe", icon: "qrc:/icons/mesh.svg", prop: Modeler.viewMode === 1, set: function(v){ if (Modeler.viewMode !== undefined) Modeler.viewMode = v ? 1 : 0 } },
                        { label: "Snap", icon: "qrc:/icons/snap.svg", prop: pageKsModeler.snapGrid, set: function(v){ pageKsModeler.snapGrid = v; if (Modeler.setSnapEnabled) Modeler.setSnapEnabled(v) } }
                    ]
                    delegate: Rectangle {
                        Layout.preferredWidth: modelData.label === "Wireframe" ? 72 : 54
                        Layout.preferredHeight: 18
                        radius: 3
                        color: modelData.prop ? "#264f78" : (snapHover.containsMouse ? "#3e3e42" : "transparent")
                        border.color: modelData.prop ? "#569cd6" : "#3f3f46"
                        border.width: 1
                        Row {
                            anchors.centerIn: parent
                            spacing: 4
                            Item {
                                width: 11; height: 11
                                Image { id: statusToggleIcon; anchors.fill: parent; source: modelData.icon }
                                ColorOverlay {
                                    anchors.fill: statusToggleIcon
                                    source: statusToggleIcon
                                    color: modelData.prop ? "#78beff" : "#aaa"
                                }
                            }
                            Text {
                                text: modelData.label
                                color: modelData.prop ? "#78beff" : "#aaa"
                                font.pixelSize: 9
                            }
                        }
                        MouseArea { id: snapHover; anchors.fill: parent; hoverEnabled: true
                            onClicked: { if (modelData.set) modelData.set(!modelData.prop) }
                        }
                    }
                }

                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Rectangle {
                    id: snapIncBox
                    Layout.preferredWidth: 46; Layout.preferredHeight: 18
                    radius: 3
                    color: snapIncHover.containsMouse ? "#3e3e42" : "transparent"
                    border.color: "#3f3f46"; border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: "Inc " + (Modeler.snapIncrement ? Modeler.snapIncrement().toFixed(2) : "0.10")
                        color: "#888"; font.pixelSize: 8
                    }
                    MouseArea { id: snapIncHover; anchors.fill: parent; hoverEnabled: true
                        onClicked: {
                            var presets = [0.05, 0.1, 0.25, 0.5, 1.0]
                            var cur = Modeler.snapIncrement ? Modeler.snapIncrement() : 0.1
                            var idx = presets.indexOf(cur)
                            if (idx < 0) idx = 1
                            var next = presets[(idx + 1) % presets.length]
                            if (Modeler.setSnapIncrement) Modeler.setSnapIncrement(next)
                            if (Modeler.setSnapEnabled) Modeler.setSnapEnabled(true)
                            pageKsModeler.snapGrid = true
                        }
                    }
                }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Rectangle {
                    id: statusViewToggle
                    Layout.preferredWidth: 58; Layout.preferredHeight: 18
                    radius: 3
                    color: statusViewToggleHover.containsMouse ? "#3e3e42" : "transparent"
                    border.color: quadView ? "#569cd6" : "#3f3f46"
                    border.width: 1
                    Row {
                        anchors.centerIn: parent
                        spacing: 4
                        Item {
                            width: 11; height: 11
                            Image {
                                id: statusViewIcon
                                anchors.fill: parent
                                source: quadView ? "qrc:/icons/view-quad.svg" : "qrc:/icons/view-single.svg"
                            }
                            ColorOverlay {
                                anchors.fill: statusViewIcon
                                source: statusViewIcon
                                color: quadView ? "#78beff" : "#aaa"
                            }
                        }
                        Text {
                            text: quadView ? "Quad" : "Single"
                            color: quadView ? "#78beff" : "#aaa"
                            font.pixelSize: 9
                        }
                    }
                    MouseArea { id: statusViewToggleHover; anchors.fill: parent; hoverEnabled: true
                        onClicked: quadView = !quadView
                    }
                    ToolTip { visible: statusViewToggleHover.containsMouse; text: quadView ? "Switch to single view" : "Switch to quad view" }
                }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "Objects: " + (sceneModel ? sceneModel.count : 0); color: "#888"; font.pixelSize: 10 }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "View: " + viewportNames[activeViewport]; color: "#888"; font.pixelSize: 10 }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "Tool: " + (Modeler.gizmoMode === 0 ? "Select" : Modeler.gizmoMode === 1 ? "Move" : Modeler.gizmoMode === 2 ? "Rotate" : "Scale"); color: "#888"; font.pixelSize: 10 }
                Rectangle { width: 1; height: 16; color: "#2d2d30"; visible: seamMode }
                Text { text: "Mark Seam ON (click edges)"; color: "#E10600"; font.pixelSize: 10; font.bold: true; visible: seamMode }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: currentFile; color: "#666"; font.pixelSize: 10 }
            }
        }
    }
    Rectangle { id: materialEditorOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.9, 620); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/MaterialEditor.qml"
            onItemChanged: if (item) item.closeRequested.connect(function() { materialEditorOverlay.visible = false })
        }
    }

    Rectangle { id: propsOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 300; height: Math.min(parent.height * 0.85, 520); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/PropertiesPanel.qml"
            onItemChanged: if (item) item.closeRequested.connect(function() { propsOverlay.visible = false })
        }
    }

    Rectangle { id: timelineOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 420; height: Math.min(parent.height * 0.7, 400); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/AnimationTimeline.qml"
            onItemChanged: if (item) item.closeRequested.connect(function() { timelineOverlay.visible = false })
        }
    }

    Rectangle { id: symmetryOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 260; height: Math.min(parent.height * 0.85, 420); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/SymmetryPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { symmetryOverlay.visible = false })
        }
    }

    Rectangle { id: modifierStackOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 340; height: Math.min(parent.height * 0.9, 520); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/ModifierStackPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { modifierStackOverlay.visible = false })
        }
    }

    Rectangle { id: curveOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 360; height: Math.min(parent.height * 0.9, 560); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/CurvePanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { curveOverlay.visible = false })
        }
    }

    Rectangle { id: selectionSetsOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.9, 520); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/SelectionSetsPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { selectionSetsOverlay.visible = false })
        }
    }

    Rectangle { id: faceGroupsOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 340; height: Math.min(parent.height * 0.9, 560); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/FaceGroupsPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { faceGroupsOverlay.visible = false })
        }
    }

    Rectangle { id: gapToolsOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 340; height: Math.min(parent.height * 0.92, 560); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/GapToolsPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { gapToolsOverlay.visible = false })
        }
    }

    Rectangle { id: layersOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.8, 480); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/LayersPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { layersOverlay.visible = false })
        }
    }

    Rectangle { id: fcurveOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 620; height: Math.min(parent.height * 0.92, 720); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/FCurveEditorPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { fcurveOverlay.visible = false })
        }
    }

    Rectangle { id: boolOpOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 340; height: Math.min(parent.height * 0.9, 560); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/BoolOpPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { boolOpOverlay.visible = false })
        }
    }

    Rectangle { id: cutToolsOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 300; height: Math.min(parent.height * 0.85, 460); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/CutToolsPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { cutToolsOverlay.visible = false })
        }
    }

    Rectangle { id: acToolsOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 300; height: Math.min(parent.height * 0.92, 640); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/ACPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { acToolsOverlay.visible = false })
        }
    }

    Rectangle { id: viewportOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.9, 640); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/ViewportPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { viewportOverlay.visible = false })
        }
    }

    Rectangle { id: factoryOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.9, 540); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/FactoryPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { factoryOverlay.visible = false })
        }
    }

    Rectangle { id: sculptOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 300; height: Math.min(parent.height * 0.9, 520); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/SculptPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { sculptOverlay.visible = false; sculptMode = -1 })
        }
    }

    Rectangle { id: constraintsOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 360; height: Math.min(parent.height * 0.9, 600); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/ConstraintsPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { constraintsOverlay.visible = false })
        }
    }

    Rectangle { id: controllersOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 340; height: Math.min(parent.height * 0.9, 560); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/ControllersPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { controllersOverlay.visible = false })
        }
    }

    Rectangle { id: wiresOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 360; height: Math.min(parent.height * 0.9, 560); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/WireParametersPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { wiresOverlay.visible = false })
        }
    }

    Rectangle { id: skinWrapOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 340; height: Math.min(parent.height * 0.9, 540); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/SkinWrapPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { skinWrapOverlay.visible = false })
        }
    }

    Rectangle { id: lightsOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 360; height: Math.min(parent.height * 0.9, 560); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/LightPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { lightsOverlay.visible = false })
        }
    }

    Rectangle { id: bakeOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 360; height: Math.min(parent.height * 0.9, 600); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/BakePanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { bakeOverlay.visible = false })
        }
    }

    Rectangle { id: nodeEditorOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: Math.min(parent.width * 0.82, 980); height: Math.min(parent.height * 0.92, 720); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/MaterialNodePanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { nodeEditorOverlay.visible = false })
        }
    }

    Rectangle { id: nlaOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 520; height: Math.min(parent.height * 0.95, 640); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/NLAPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { nlaOverlay.visible = false })
        }
    }

    Rectangle { id: dynOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 420; height: Math.min(parent.height * 0.9, 560); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/DynamicsPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { dynOverlay.visible = false })
        }
    }

    Rectangle { id: clothOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 420; height: Math.min(parent.height * 0.95, 720); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/ClothPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { clothOverlay.visible = false })
        }
    }

    Rectangle { id: hairOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 360; height: Math.min(parent.height * 0.9, 560); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/HairPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { hairOverlay.visible = false })
        }
    }

    Rectangle { id: vertexEdgeSlideOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.85, 500); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/VertexEdgeSlidePanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { vertexEdgeSlideOverlay.visible = false })
        }
    }

    Rectangle { id: edgeLoopOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.6, 300); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            id: edgeLoopLoader
            anchors.fill: parent
            source: "../modules/3DModeling/EdgeLoopPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { edgeLoopOverlay.visible = false })
        }
    }

    Rectangle { id: quadRemeshOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.55, 300); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/QuadRemeshPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { quadRemeshOverlay.visible = false })
        }
    }

    Rectangle { id: iceOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.9, 600); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/ICEPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { iceOverlay.visible = false })
        }
    }

    Rectangle {
        id: paintPanelOverlay
        visible: false; z: 10
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8
        width: 280; height: Math.min(parent.height * 0.85, 600)
        color: "transparent"
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/paint_TexturePanel.qml"
        }
        Rectangle {
            anchors.top: parent.top; anchors.right: parent.right
            width: 18; height: 18; radius: 2; color: "#E10600"
            Text { anchors.centerIn: parent; text: "X"; color: "#fff"; font.pixelSize: 10; font.bold: true }
            MouseArea { anchors.fill: parent; onClicked: closePaintPanel() }
        }
    }
    }

    function openSymmetryPanel() { symmetryOverlay.visible = !symmetryOverlay.visible }
    function openPaintPanel() { paintPanelOverlay.visible = !paintPanelOverlay.visible }
    function closePaintPanel() { paintPanelOverlay.visible = false }

    FileDialog {
        id: openSceneDialog
        title: "Open Scene"
        nameFilters: ["KS Scene (*.ks3d)", "KN5 Scene (*.kn5)", "FBX Scene (*.fbx)", "Blender Scene (*.blend)", "All Files (*)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            var lowerPath = path.toLowerCase()
            var ok = false

            if (lowerPath.endsWith(".fbx") || lowerPath.endsWith(".blend")) {
                if (Modeler.importFile) {
                    ok = Modeler.importFile(path)
                } else if (Modeler.loadScene) {
                    ok = Modeler.loadScene(path)
                }
            } else {
                if (Modeler.loadScene) {
                    ok = Modeler.loadScene(path)
                } else if (Modeler.importFile) {
                    ok = Modeler.importFile(path)
                }
            }

            currentFile = ok ? path : currentFile
            commandEcho = ok ? "Opened: " + path : "Open failed"

            if (sceneModel)
                sceneModel.refresh()
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save Scene As"
        fileMode: FileDialog.SaveFile
        nameFilters: ["KS Scene (*.ks3d)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            if (!path.toLowerCase().endsWith(".ks3d"))
                path += ".ks3d"
            var ok = Modeler.saveScene ? Modeler.saveScene(path) : false
            currentFile = ok ? path : currentFile
            commandEcho = ok ? "Saved: " + path : "Save failed"
        }
    }

    FileDialog {
        id: importDialog
        title: "Import 3D Model"
        nameFilters: ["Supported Files (*.kn5 *.fbx *.glb *.gltf *.obj *.stl)", "KN5 (*.kn5)", "FBX (*.fbx)", "GLB/GLTF (*.glb *.gltf)", "OBJ (*.obj)", "STL (*.stl)"]
        onAccepted: {
            if (Modeler.importFile) {
                var ok = Modeler.importFile(fileUrl.toString().replace("file:///", "").replace("file://", ""))
                commandEcho = ok ? "Imported: " + fileUrl : "Import failed"
            }
            if (sceneModel) sceneModel.refresh()
        }
    }

    FileDialog {
        id: exportDialog
        title: "Export 3D Model"
        fileMode: FileDialog.SaveFile
        nameFilters: ["KN5 (*.kn5)", "FBX (*.fbx)", "OBJ (*.obj)", "GLB (*.glb)", "STL (*.stl)", "STEP (*.step)", "All Files (*)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            var filter = nameFilters ? nameFilters[0] : ""
            if (filter.includes("STEP")) {
                var ok = Modeler.exportSTEP ? Modeler.exportSTEP(path, false) : false
                commandEcho = ok ? "Exported STEP: " + path : "STEP export failed"
            } else {
                var ok = Modeler.exportFile ? Modeler.exportFile(path) : false
                commandEcho = ok ? "Exported: " + path : "Export failed"
            }
        }
    }

    FileDialog {
        id: gpxImportDialog
        title: "Import GPX"
        nameFilters: ["GPX (*.gpx)", "All Files (*)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            var ok = Modeler.importGPX ? Modeler.importGPX(path) : false
            commandEcho = ok ? "GPX imported (" + (Modeler.trackPointCount ? Modeler.trackPointCount() : 0) + " points): " + path
                             : "GPX import failed"
        }
    }

    FileDialog {
        id: mocapImportDialog
        title: "Import Motion Capture (BVH)"
        nameFilters: ["Biovision BVH (*.bvh)", "All Files (*)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            var ok = Modeler.importBVH ? Modeler.importBVH(path, "") : false
            commandEcho = ok ? "Mocap imported: " + path : "BVH import failed"
        }
    }

    FileDialog {
        id: renderFileDialog
        title: "Render Scene to PNG"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG Image (*.png)", "All Files (*)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            if (!/\.png$/i.test(path)) path += ".png"
            if (updateRayTraceCamera) updateRayTraceCamera()
            var ok = Modeler.rayTraceRenderToFile ? Modeler.rayTraceRenderToFile(path, 1280, 720, 8, Modeler.rayTracePass !== undefined ? Modeler.rayTracePass : 0) : false
            commandEcho = ok ? "Render saved: " + path : "Render to file failed"
        }
    }

    FileDialog {
        id: kmlImportDialog
        title: "Import KML"
        nameFilters: ["KML (*.kml)", "All Files (*)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            var ok = Modeler.importKML ? Modeler.importKML(path) : false
            commandEcho = ok ? "KML imported (" + (Modeler.trackPointCount ? Modeler.trackPointCount() : 0) + " points): " + path
                             : "KML import failed"
        }
    }

    FileDialog {
        id: gpxExportDialog
        title: "Export GPX"
        fileMode: FileDialog.SaveFile
        nameFilters: ["GPX (*.gpx)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            if (!path.toLowerCase().endsWith(".gpx"))
                path += ".gpx"
            var ok = Modeler.exportGPX ? Modeler.exportGPX(path) : false
            commandEcho = ok ? "GPX exported: " + path : "GPX export failed"
        }
    }

    FileDialog {
        id: kmlExportDialog
        title: "Export KML"
        fileMode: FileDialog.SaveFile
        nameFilters: ["KML (*.kml)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            if (!path.toLowerCase().endsWith(".kml"))
                path += ".kml"
            var ok = Modeler.exportKML ? Modeler.exportKML(path) : false
            commandEcho = ok ? "KML exported: " + path : "KML export failed"
        }
    }

    FileDialog {
        id: aiExportDialog
        title: "Export AI Line"
        fileMode: FileDialog.SaveFile
        nameFilters: ["AC AI Line (*.ai)", "AC AI KN5 (*_ai.kn5)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            var ok = Modeler.exportAILine ? Modeler.exportAILine(path) : false
            commandEcho = ok ? "AI line exported: " + path : "AI line export failed"
        }
    }

    FileDialog {
        id: hdrImportDialog
        title: "Load HDRI Environment"
        nameFilters: ["HDRI (*.hdr *.exr)", "All Files (*)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            if (Modeler.environmentHDR !== undefined) Modeler.environmentHDR = path
            commandEcho = "HDRI environment: " + path
        }
    }

    Shortcut { sequence: "Q"; onActivated: { if (Modeler.setGizmoMode) Modeler.setGizmoMode(0) } }
    Shortcut { sequence: "W"; onActivated: { if (Modeler.setGizmoMode) Modeler.setGizmoMode(1) } }
    Shortcut { sequence: "E"; onActivated: { if (Modeler.setGizmoMode) Modeler.setGizmoMode(2) } }
    Shortcut { sequence: "R"; onActivated: { if (Modeler.setGizmoMode) Modeler.setGizmoMode(3) } }
    Shortcut { sequence: "G"; onActivated: { if (Modeler.gridVisible !== undefined) Modeler.gridVisible = !Modeler.gridVisible } }
    Shortcut { sequence: "F"; onActivated: { if (Modeler.focusOnSelected) Modeler.focusOnSelected() } }
    Shortcut { sequence: "Delete"; onActivated: { if (Modeler.deleteSelected) Modeler.deleteSelected() } }
    Shortcut { sequence: "1"; onActivated: activateViewport(0) }
    Shortcut { sequence: "2"; onActivated: activateViewport(1) }
    Shortcut { sequence: "3"; onActivated: activateViewport(2) }
    Shortcut { sequence: "4"; onActivated: activateViewport(3) }
    Shortcut { sequence: "5"; onActivated: activateViewport(3) }

    Component.onCompleted: {
        if (sceneModel) { if (sceneModel.countChanged) sceneModel.countChanged.connect(updateStats) }
        updateStats()
        updateTransformFields()
        activateViewport(3)
    }
}
