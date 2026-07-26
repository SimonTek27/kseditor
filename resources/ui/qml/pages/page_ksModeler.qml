import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D 6.5
import ksEditor.Modeler 1.0
import "../modules/3DModeling"

Rectangle {
    id: pageKsModeler
    width: 1280
    height: 720
    color: "#111111"
    focus: true

    property string currentFile: "untitled.zm"
    property int viewMode: 0
    property string selectedObject: ""
    property int activeViewport: 3

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
    property bool animLoop: true

    property bool canUndo: Modeler.canUndo ? Modeler.canUndo() : false
    property bool canRedo: Modeler.canRedo ? Modeler.canRedo() : false

    property var viewportNames: ["Top", "Front", "Right", "User"]
    property var viewportViewModes: ["top", "front", "right", "persp"]

    function notifySelection(name, id) {
        selectedObject = name
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
                if (mode === 1) { if (Modeler.translateSelected) Modeler.translateSelected(gx, gy, gz) }
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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 36
            color: "#2d2d30"
            Layout.fillWidth: true
            border.color: "#3f3f46"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 2

                Repeater {
                    model: [
                        { label: "New", icon: "\u2795", tip: "New Scene" },
                        { label: "Open", icon: "\u2601", tip: "Open" },
                        { label: "Save", icon: "\u2913", tip: "Save" },
                        { label: "Import", icon: "\u2B07", tip: "Import" },
                        { label: "Export", icon: "\u2B06", tip: "Export" }
                    ]
                    delegate: Rectangle {
                        width: 55; height: 28; radius: 3
                        color: tbMa.containsMouse ? "#3e3e42" : "transparent"
                        Column {
                            anchors.centerIn: parent; spacing: -2
                            Text { text: modelData.icon; color: "#ccc"; font.pixelSize: 13; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: modelData.label; color: "#999"; font.pixelSize: 8; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                        MouseArea { id: tbMa; anchors.fill: parent; hoverEnabled: true
                            onClicked: {
                                if (index === 0 && Modeler.newScene) Modeler.newScene()
                                else if (index === 2 && Modeler.saveScene) Modeler.saveScene(currentFile)
                            }
                        }
                        ToolTip { visible: tbMa.containsMouse; text: modelData.tip }
                    }
                }

                Rectangle { width: 1; height: 24; color: "#3f3f46" }

                Repeater {
                    model: [
                        { label: "Undo", icon: "\u21B6", tip: "Undo (Ctrl+Z)" },
                        { label: "Redo", icon: "\u21B7", tip: "Redo (Ctrl+Shift+Z)" }
                    ]
                    delegate: Rectangle {
                        width: 50; height: 28; radius: 3
                        color: tbMa2.containsMouse ? "#3e3e42" : "transparent"
                        opacity: (index === 0 ? pageKsModeler.canUndo : pageKsModeler.canRedo) ? 1.0 : 0.4
                        Column {
                            anchors.centerIn: parent; spacing: -2
                            Text { text: modelData.icon; color: "#ccc"; font.pixelSize: 13; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: modelData.label; color: "#999"; font.pixelSize: 8; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                        MouseArea { id: tbMa2; anchors.fill: parent; hoverEnabled: true
                            enabled: index === 0 ? pageKsModeler.canUndo : pageKsModeler.canRedo
                            onClicked: { if (index === 0 && Modeler.undo) Modeler.undo(); else if (index === 1 && Modeler.redo) Modeler.redo() }
                        }
                    }
                }

                Rectangle { width: 1; height: 24; color: "#3f3f46" }

                Repeater {
                    model: [
                        { label: "Sel", icon: "\u25CE", mode: 0, tip: "Select (Q)" },
                        { label: "Move", icon: "\u2194", mode: 1, tip: "Move (W)" },
                        { label: "Rot", icon: "\u21BB", mode: 2, tip: "Rotate (E)" },
                        { label: "Scl", icon: "\u2195", mode: 3, tip: "Scale (R)" }
                    ]
                    delegate: Rectangle {
                        width: 50; height: 28; radius: 3
                        color: Modeler.gizmoMode === modelData.mode ? "#264f78" : (tbMa3.containsMouse ? "#3e3e42" : "transparent")
                        border.color: Modeler.gizmoMode === modelData.mode ? "#569cd6" : "transparent"
                        border.width: 1
                        Column {
                            anchors.centerIn: parent; spacing: -2
                            Text { text: modelData.icon; color: Modeler.gizmoMode === modelData.mode ? "#569cd6" : "#ccc"; font.pixelSize: 13; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: modelData.label; color: "#999"; font.pixelSize: 8; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                        MouseArea { id: tbMa3; anchors.fill: parent; hoverEnabled: true
                            onClicked: { if (Modeler.setGizmoMode) Modeler.setGizmoMode(modelData.mode) }
                        }
                        ToolTip { visible: tbMa3.containsMouse; text: modelData.tip }
                    }
                }

                Rectangle { width: 1; height: 24; color: "#3f3f46" }

                Repeater {
                    model: [
                        { label: "Grid", icon: "\u2317", tip: "Toggle Grid (G)" },
                        { label: "Sym", icon: "\u2194", tip: "Symmetry (Shift+S)" },
                        { label: "Mat", icon: "M", tip: "Material Editor" }
                    ]
                    delegate: Rectangle {
                        width: 50; height: 28; radius: 3
                        color: tbMa4.containsMouse ? "#3e3e42" : "transparent"
                        Column {
                            anchors.centerIn: parent; spacing: -2
                            Text { text: modelData.icon; color: "#ccc"; font.pixelSize: 13; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: modelData.label; color: "#999"; font.pixelSize: 8; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                        MouseArea { id: tbMa4; anchors.fill: parent; hoverEnabled: true
                            onClicked: {
                                if (index === 0 && Modeler.gridVisible !== undefined) Modeler.gridVisible = !Modeler.gridVisible
                                else if (index === 1) openSymmetryPanel()
                                else if (index === 2) materialEditorOverlay.visible = !materialEditorOverlay.visible
                            }
                        }
                        ToolTip { visible: tbMa4.containsMouse; text: modelData.tip }
                    }
                }

                Item { Layout.fillWidth: true }

                Repeater {
                    model: [
                        { label: "Outliner", tip: "Scene Outliner" },
                        { label: "Props", tip: "Properties" },
                        { label: "Timeline", tip: "Animation Timeline" }
                    ]
                    delegate: Rectangle {
                        width: 60; height: 28; radius: 3
                        color: tbMa5.containsMouse ? "#3e3e42" : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: "#999"; font.pixelSize: 9
                        }
                        MouseArea { id: tbMa5; anchors.fill: parent; hoverEnabled: true
                            onClicked: {
                                if (index === 0) outlinerOverlay.visible = !outlinerOverlay.visible
                                else if (index === 1) propsOverlay.visible = !propsOverlay.visible
                                else if (index === 2) timelineOverlay.visible = !timelineOverlay.visible
                            }
                        }
                        ToolTip { visible: tbMa5.containsMouse; text: modelData.tip }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

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

                    Text { text: "COMMANDS"; color: "#888"; font.pixelSize: 10; font.bold: true; leftPadding: 4 }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ListView {
                            id: commandsList
                            model: ListModel {
                                id: commandsModel
                                ListElement { category: "Create"; expanded: true; tools: "" }
                                ListElement { category: "Display"; expanded: false; tools: "" }
                                ListElement { category: "Modify"; expanded: false; tools: "" }
                                ListElement { category: "Select"; expanded: false; tools: "" }
                                ListElement { category: "Local Axes"; expanded: false; tools: "" }
                                ListElement { category: "Surface"; expanded: false; tools: "" }
                            }
                            spacing: 1

                            delegate: Column {
                                width: commandsList.width

                                Rectangle {
                                    width: parent.width
                                    height: 24
                                    color: catMouse.containsMouse ? "#2d2d30" : "transparent"
                                    radius: 2

                                    Row {
                                        anchors.fill: parent
                                        anchors.leftMargin: 4
                                        spacing: 4
                                        Text { text: model.expanded ? "\u25BC" : "\u25B6"; color: "#888"; font.pixelSize: 8; anchors.verticalCenter: parent.verticalCenter }
                                        Text { text: model.category.toUpperCase(); color: "#cccccc"; font.pixelSize: 11; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                                    }

                                    MouseArea {
                                        id: catMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: commandsModel.setProperty(index, "expanded", !model.expanded)
                                    }
                                }

                                Repeater {
                                    model: {
                                        if (!model.expanded) return []
                                        if (index === 0) return [
                                            { tool: "Box", icon: "\u25A0" }, { tool: "Sphere", icon: "\u25CB" },
                                            { tool: "Cylinder", icon: "\u25AD" }, { tool: "Cone", icon: "\u25B3" },
                                            { tool: "Torus", icon: "\u25CE" }, { tool: "Plane", icon: "\u25A1" }
                                        ]
                                        if (index === 1) return [
                                            { tool: "Wireframe", icon: "\u25A1" }, { tool: "Solid", icon: "\u25A0" },
                                            { tool: "Textured", icon: "\u25A3" }
                                        ]
                                        if (index === 2) return [
                                            { tool: "Move", icon: "\u2194" }, { tool: "Rotate", icon: "\u21BB" },
                                            { tool: "Scale", icon: "\u2195" }, { tool: "Extrude", icon: "\u21E7" },
                                            { tool: "Bevel", icon: "\u2261" }, { tool: "Weld", icon: "\u2228" }
                                        ]
                                        if (index === 3) return [
                                            { tool: "Box Select", icon: "\u25A1" }, { tool: "Circle Select", icon: "\u25CB" },
                                            { tool: "All", icon: "\u25A0" }, { tool: "None", icon: "\u25CB" }
                                        ]
                                        if (index === 4) return [
                                            { tool: "Copy", icon: "\u2398" }, { tool: "Paste", icon: "\u2399" },
                                            { tool: "Reset", icon: "\u21BA" }
                                        ]
                                        if (index === 5) return [
                                            { tool: "Assign Material", icon: "\u25A3" },
                                            { tool: "UV Map", icon: "\u25A1" }
                                        ]
                                        return []
                                    }

                                    delegate: Rectangle {
                                        width: commandsList.width
                                        height: 22
                                        color: toolMouse.containsMouse ? "#264f78" : "transparent"
                                        radius: 2

                                        Row {
                                            anchors.fill: parent
                                            anchors.leftMargin: 24
                                            spacing: 6
                                            Text { text: modelData.icon; color: "#aaa"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                                            Text { text: modelData.tool; color: "#cccccc"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                                        }

                                        MouseArea { id: toolMouse; anchors.fill: parent; hoverEnabled: true
                                            onClicked: console.log("Tool:", modelData.tool)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle { height: 1; color: "#2d2d30"; Layout.fillWidth: true }

                    Text { text: "TRANSFORM"; color: "#888"; font.pixelSize: 10; font.bold: true; leftPadding: 4 }

                    GridLayout {
                        columns: 2; Layout.fillWidth: true; rowSpacing: 2; columnSpacing: 4
                        Text { text: "X:"; color: "#ff4444"; font.pixelSize: 10 }
                        TextField { text: "0.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                        Text { text: "Y:"; color: "#44ff44"; font.pixelSize: 10 }
                        TextField { text: "0.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                        Text { text: "Z:"; color: "#4444ff"; font.pixelSize: 10 }
                        TextField { text: "0.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 20; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                    }

                    Rectangle { height: 1; color: "#2d2d30"; Layout.fillWidth: true }
                    Text { text: "SELECTION"; color: "#888"; font.pixelSize: 10; font.bold: true; leftPadding: 4 }
                    Text { text: selectedObject !== "" ? selectedObject : "(none)"; color: "#aaa"; font.pixelSize: 10; leftPadding: 4 }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#111111"

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 1
                    columns: 2
                    rows: 2
                    columnSpacing: 1
                    rowSpacing: 1

                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#1a1a1e"
                        border.color: activeViewport === 0 ? "#E10600" : "#2d2d30"
                        border.width: activeViewport === 0 ? 2 : 1

                        View3D {
                            id: vpTop
                            anchors.fill: parent
                            visible: true
                            environment: SceneEnvironment {
                                clearColor: "#1a1a1e"
                                backgroundMode: SceneEnvironment.Color
                                antialiasingMode: SceneEnvironment.MSAA
                                antialiasingQuality: SceneMedium
                            }
                            OrthographicCamera {
                                id: camTop
                                position: Qt.vector3d(0, 20, 0)
                                eulerRotation: Qt.vector3d(-90, 0, 0)
                                scale: Qt.vector3d(camDistance * 0.01, camDistance * 0.01, 1)
                            }

                            Repeater {
                                model: [
                                    { x: 0, y: -0.01, z: 0, sx: 20, sy: 20, sz: 1, rx: 90, ry: 0, rz: 0, col: "#2a2a2e" },
                                    { x: 1, y: 0, z: 0, sx: 0.02, sy: 2, sz: 0.02, rx: 0, ry: 0, rz: 0, col: "#ff4444" },
                                    { x: 0, y: 0, z: 0.5, sx: 0.02, sy: 1, sz: 0.02, rx: 0, ry: 0, rz: 0, col: "#4444ff" }
                                ]
                                delegate: Model {
                                    source: index === 0 ? "#Rectangle" : "#Cylinder"
                                    position: Qt.vector3d(modelData.x, modelData.y, modelData.z)
                                    scale: Qt.vector3d(modelData.sx, modelData.sy, modelData.sz)
                                    eulerRotation: Qt.vector3d(modelData.rx, modelData.ry, modelData.rz)
                                    visible: index === 0 ? (Modeler.gridVisible !== undefined ? Modeler.gridVisible : true) : true
                                    materials: [DefaultMaterial { diffuseColor: modelData.col }]
                                }
                            }

                            Repeater {
                                model: sceneModel
                                delegate: Model {
                                    source: model.meshSource || "#Cube"
                                    position: model.objectPosition !== undefined ? model.objectPosition : Qt.vector3d(0, 0, 0)
                                    eulerRotation: model.objectRotation !== undefined ? model.objectRotation : Qt.vector3d(0, 0, 0)
                                    scale: model.objectScale !== undefined ? model.objectScale : Qt.vector3d(1, 1, 1)
                                    visible: model.objectVisible !== undefined ? model.objectVisible : true
                                    materials: [PrincipledMaterial {
                                        baseColor: model.baseColor || "#4488cc"
                                        metalness: model.metallic || 0.0
                                        roughness: model.roughness || 0.5
                                    }]
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
                        border.color: activeViewport === 3 ? "#E10600" : "#2d2d30"
                        border.width: activeViewport === 3 ? 2 : 1

                        View3D {
                            id: vpUser
                            anchors.fill: parent
                            visible: true
                            environment: SceneEnvironment {
                                clearColor: "#1a1a1e"
                                backgroundMode: SceneEnvironment.Color
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
                                clipNear: 0.1
                                clipFar: 1000
                            }

                            Repeater {
                                model: [
                                    { x: 0, y: -0.01, z: 0, sx: 20, sy: 20, sz: 1, rx: 90, ry: 0, rz: 0, col: "#2a2a2e" },
                                    { x: 1, y: 0, z: 0, sx: 0.02, sy: 2, sz: 0.02, rx: 0, ry: 0, rz: 0, col: "#ff4444" },
                                    { x: 0, y: 1, z: 0, sx: 0.02, sy: 2, sz: 0.02, rx: 0, ry: 0, rz: 0, col: "#44ff44" },
                                    { x: 0, y: 0, z: 1, sx: 0.02, sy: 2, sz: 0.02, rx: 0, ry: 0, rz: 0, col: "#4444ff" }
                                ]
                                delegate: Model {
                                    source: index === 0 ? "#Rectangle" : "#Cylinder"
                                    position: Qt.vector3d(modelData.x, modelData.y, modelData.z)
                                    scale: Qt.vector3d(modelData.sx, modelData.sy, modelData.sz)
                                    eulerRotation: Qt.vector3d(modelData.rx, modelData.ry, modelData.rz)
                                    visible: index === 0 ? (Modeler.gridVisible !== undefined ? Modeler.gridVisible : true) : true
                                    materials: [DefaultMaterial { diffuseColor: modelData.col }]
                                }
                            }

                            Repeater {
                                model: sceneModel
                                delegate: Model {
                                    source: model.meshSource || "#Cube"
                                    position: model.objectPosition !== undefined ? model.objectPosition : Qt.vector3d(0, 0, 0)
                                    eulerRotation: model.objectRotation !== undefined ? model.objectRotation : Qt.vector3d(0, 0, 0)
                                    scale: model.objectScale !== undefined ? model.objectScale : Qt.vector3d(1, 1, 1)
                                    visible: model.objectVisible !== undefined ? model.objectVisible : true
                                    materials: [PrincipledMaterial {
                                        baseColor: model.baseColor || "#4488cc"
                                        metalness: model.metallic || 0.0
                                        roughness: model.roughness || 0.5
                                    }]
                                }
                            }

                            Node {
                                visible: {
                                    var mode = Modeler.gizmoMode !== undefined ? Modeler.gizmoMode : 0
                                    var hasSel = Modeler.hasSelection !== undefined ? Modeler.hasSelection : false
                                    return mode > 0 && hasSel
                                }
                                position: Modeler.gizmoPosition || Qt.vector3d(0, 0, 0)
                                Model { source: "#Cylinder"; position: Qt.vector3d(0.5, 0, 0); scale: dragAxis === 0 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02); eulerRotation: Qt.vector3d(0, 0, -90); materials: [DefaultMaterial { diffuseColor: dragAxis === 0 ? "#ff8888" : "#ff4444" }] }
                                Model { source: "#Cone"; position: Qt.vector3d(1.0, 0, 0); scale: Qt.vector3d(0.06, 0.2, 0.06); eulerRotation: Qt.vector3d(0, 0, -90); materials: [DefaultMaterial { diffuseColor: dragAxis === 0 ? "#ff8888" : "#ff4444" }] }
                                Model { source: "#Cylinder"; position: Qt.vector3d(0, 0.5, 0); scale: dragAxis === 1 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02); materials: [DefaultMaterial { diffuseColor: dragAxis === 1 ? "#88ff88" : "#44ff44" }] }
                                Model { source: "#Cone"; position: Qt.vector3d(0, 1.0, 0); scale: Qt.vector3d(0.06, 0.2, 0.06); materials: [DefaultMaterial { diffuseColor: dragAxis === 1 ? "#88ff88" : "#44ff44" }] }
                                Model { source: "#Cylinder"; position: Qt.vector3d(0, 0, 0.5); scale: dragAxis === 2 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02); eulerRotation: Qt.vector3d(90, 0, 0); materials: [DefaultMaterial { diffuseColor: dragAxis === 2 ? "#8888ff" : "#4444ff" }] }
                                Model { source: "#Cone"; position: Qt.vector3d(0, 0, 1.0); scale: Qt.vector3d(0.06, 0.2, 0.06); eulerRotation: Qt.vector3d(90, 0, 0); materials: [DefaultMaterial { diffuseColor: dragAxis === 2 ? "#8888ff" : "#4444ff" }] }
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
                            onPressed: (mouse) => {
                                activateViewport(3)
                                _lastPos = Qt.point(mouse.x, mouse.y)
                                var result = vpUser.pick(mouse.x, mouse.y)
                                if (result.objectHit) {
                                    var name = result.objectHit.objectName
                                    if (name.indexOf("gizmoX") >= 0) dragAxis = 0
                                    else if (name.indexOf("gizmoY") >= 0) dragAxis = 1
                                    else if (name.indexOf("gizmoZ") >= 0) dragAxis = 2
                                }
                            }
                            onPositionChanged: (mouse) => { setupViewportMouse(this, 3) }
                            onReleased: { dragAxis = -1 }
                            onWheel: (wheel) => { if (Modeler.camDistance !== undefined) Modeler.camDistance = Math.max(0.5, Math.min(100, camDistance * (1 + wheel.angleDelta.y * 0.001))) }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#1a1a1e"
                        border.color: activeViewport === 1 ? "#E10600" : "#2d2d30"
                        border.width: activeViewport === 1 ? 2 : 1

                        View3D {
                            id: vpFront
                            anchors.fill: parent
                            visible: true
                            environment: SceneEnvironment {
                                clearColor: "#1a1a1e"
                                backgroundMode: SceneEnvironment.Color
                                antialiasingMode: SceneEnvironment.MSAA
                                antialiasingQuality: SceneMedium
                            }
                            OrthographicCamera {
                                id: camFront
                                position: Qt.vector3d(0, 0, 20)
                                eulerRotation: Qt.vector3d(0, 0, 0)
                                scale: Qt.vector3d(camDistance * 0.01, camDistance * 0.01, 1)
                            }

                            Repeater {
                                model: [
                                    { x: 0, y: -0.01, z: 0, sx: 20, sy: 20, sz: 1, rx: 90, ry: 0, rz: 0, col: "#2a2a2e" },
                                    { x: 1, y: 0, z: 0, sx: 0.02, sy: 2, sz: 0.02, rx: 0, ry: 0, rz: 0, col: "#ff4444" },
                                    { x: 0, y: 1, z: 0, sx: 0.02, sy: 2, sz: 0.02, rx: 0, ry: 0, rz: 0, col: "#44ff44" }
                                ]
                                delegate: Model {
                                    source: index === 0 ? "#Rectangle" : "#Cylinder"
                                    position: Qt.vector3d(modelData.x, modelData.y, modelData.z)
                                    scale: Qt.vector3d(modelData.sx, modelData.sy, modelData.sz)
                                    eulerRotation: Qt.vector3d(modelData.rx, modelData.ry, modelData.rz)
                                    visible: index === 0 ? (Modeler.gridVisible !== undefined ? Modeler.gridVisible : true) : true
                                    materials: [DefaultMaterial { diffuseColor: modelData.col }]
                                }
                            }

                            Repeater {
                                model: sceneModel
                                delegate: Model {
                                    source: model.meshSource || "#Cube"
                                    position: model.objectPosition !== undefined ? model.objectPosition : Qt.vector3d(0, 0, 0)
                                    eulerRotation: model.objectRotation !== undefined ? model.objectRotation : Qt.vector3d(0, 0, 0)
                                    scale: model.objectScale !== undefined ? model.objectScale : Qt.vector3d(1, 1, 1)
                                    visible: model.objectVisible !== undefined ? model.objectVisible : true
                                    materials: [PrincipledMaterial {
                                        baseColor: model.baseColor || "#4488cc"
                                        metalness: model.metallic || 0.0
                                        roughness: model.roughness || 0.5
                                    }]
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
                        border.color: activeViewport === 2 ? "#E10600" : "#2d2d30"
                        border.width: activeViewport === 2 ? 2 : 1

                        View3D {
                            id: vpRight
                            anchors.fill: parent
                            visible: true
                            environment: SceneEnvironment {
                                clearColor: "#1a1a1e"
                                backgroundMode: SceneEnvironment.Color
                                antialiasingMode: SceneEnvironment.MSAA
                                antialiasingQuality: SceneMedium
                            }
                            OrthographicCamera {
                                id: camRight
                                position: Qt.vector3d(20, 0, 0)
                                eulerRotation: Qt.vector3d(0, 90, 0)
                                scale: Qt.vector3d(camDistance * 0.01, camDistance * 0.01, 1)
                            }

                            Repeater {
                                model: [
                                    { x: 0, y: -0.01, z: 0, sx: 20, sy: 20, sz: 1, rx: 90, ry: 0, rz: 0, col: "#2a2a2e" },
                                    { x: 0, y: 1, z: 0, sx: 0.02, sy: 2, sz: 0.02, rx: 0, ry: 0, rz: 0, col: "#44ff44" },
                                    { x: 0, y: 0, z: 1, sx: 0.02, sy: 2, sz: 0.02, rx: 0, ry: 0, rz: 0, col: "#4444ff" }
                                ]
                                delegate: Model {
                                    source: index === 0 ? "#Rectangle" : "#Cylinder"
                                    position: Qt.vector3d(modelData.x, modelData.y, modelData.z)
                                    scale: Qt.vector3d(modelData.sx, modelData.sy, modelData.sz)
                                    eulerRotation: Qt.vector3d(modelData.rx, modelData.ry, modelData.rz)
                                    visible: index === 0 ? (Modeler.gridVisible !== undefined ? Modeler.gridVisible : true) : true
                                    materials: [DefaultMaterial { diffuseColor: modelData.col }]
                                }
                            }

                            Repeater {
                                model: sceneModel
                                delegate: Model {
                                    source: model.meshSource || "#Cube"
                                    position: model.objectPosition !== undefined ? model.objectPosition : Qt.vector3d(0, 0, 0)
                                    eulerRotation: model.objectRotation !== undefined ? model.objectRotation : Qt.vector3d(0, 0, 0)
                                    scale: model.objectScale !== undefined ? model.objectScale : Qt.vector3d(1, 1, 1)
                                    visible: model.objectVisible !== undefined ? model.objectVisible : true
                                    materials: [PrincipledMaterial {
                                        baseColor: model.baseColor || "#4488cc"
                                        metalness: model.metallic || 0.0
                                        roughness: model.roughness || 0.5
                                    }]
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

            Rectangle {
                width: propsOverlay.visible ? 220 : 0
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#2d2d30"
                border.width: propsOverlay.visible ? 1 : 0
                visible: propsOverlay.visible

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 4

                    RowLayout {
                        Text { text: "PROPERTIES"; color: "#ccc"; font.bold: true; font.pixelSize: 11 }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            width: 16; height: 16; radius: 2; color: ppCloseMouse.containsMouse ? "#E10600" : "#3e3e42"
                            Text { anchors.centerIn: parent; text: "X"; color: "#fff"; font.pixelSize: 9 }
                            MouseArea { id: ppCloseMouse; anchors.fill: parent; hoverEnabled: true; onClicked: propsOverlay.visible = false }
                        }
                    }

                    Rectangle { height: 1; color: "#2d2d30"; Layout.fillWidth: true }
                    Text { text: "Name:"; color: "#888"; font.pixelSize: 10 }
                    TextField { text: selectedObject; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 22; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }

                    Text { text: "Position"; color: "#888"; font.pixelSize: 10; font.bold: true }
                    GridLayout { columns: 2; Layout.fillWidth: true
                        Text { text: "X:"; color: "#ff4444"; font.pixelSize: 10 }
                        TextField { text: "0.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 18; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                        Text { text: "Y:"; color: "#44ff44"; font.pixelSize: 10 }
                        TextField { text: "0.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 18; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                        Text { text: "Z:"; color: "#4444ff"; font.pixelSize: 10 }
                        TextField { text: "0.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 18; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                    }

                    Text { text: "Rotation"; color: "#888"; font.pixelSize: 10; font.bold: true }
                    GridLayout { columns: 2; Layout.fillWidth: true
                        Text { text: "X:"; color: "#ff4444"; font.pixelSize: 10 }
                        TextField { text: "0.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 18; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                        Text { text: "Y:"; color: "#44ff44"; font.pixelSize: 10 }
                        TextField { text: "0.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 18; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                        Text { text: "Z:"; color: "#4444ff"; font.pixelSize: 10 }
                        TextField { text: "0.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 18; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                    }

                    Text { text: "Scale"; color: "#888"; font.pixelSize: 10; font.bold: true }
                    GridLayout { columns: 2; Layout.fillWidth: true
                        Text { text: "X:"; color: "#ff4444"; font.pixelSize: 10 }
                        TextField { text: "1.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 18; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                        Text { text: "Y:"; color: "#44ff44"; font.pixelSize: 10 }
                        TextField { text: "1.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 18; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                        Text { text: "Z:"; color: "#4444ff"; font.pixelSize: 10 }
                        TextField { text: "1.00"; Layout.fillWidth: true; font.pixelSize: 10; implicitHeight: 18; color: "#ccc"; background: Rectangle { color: "#2d2d30"; radius: 2 } }
                    }
                }
            }
        }

        Rectangle {
            height: 24
            color: "#1e1e1e"
            Layout.fillWidth: true
            border.color: "#2d2d30"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 12

                Text { text: "Ready"; color: "#10b981"; font.pixelSize: 10 }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "View: " + viewportNames[activeViewport]; color: "#888"; font.pixelSize: 10 }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "Camera: " + Math.round(camTheta) + "\u00B0 / " + Math.round(camPhi) + "\u00B0  Dist: " + camDistance.toFixed(1); color: "#888"; font.pixelSize: 10 }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "Objects: " + (sceneModel ? (sceneModel.count || (sceneModel.rowCount ? sceneModel.rowCount() : 0)) : 0); color: "#888"; font.pixelSize: 10 }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "Verts: " + totalVerts + "  Tris: " + totalTris; color: "#888"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "Tool: " + (Modeler.gizmoMode === 0 ? "Select" : Modeler.gizmoMode === 1 ? "Move" : Modeler.gizmoMode === 2 ? "Rotate" : "Scale"); color: "#888"; font.pixelSize: 10 }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: currentFile; color: "#666"; font.pixelSize: 10 }
            }
        }
    }

    Rectangle { id: materialEditorOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.9, 620); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Rectangle { anchors.fill: parent; color: "transparent"
            Text { anchors.centerIn: parent; text: "Material Editor\n(Load MaterialEditor.qml)"; color: "#666"; horizontalAlignment: Text.AlignHCenter }
        }
    }

    Rectangle { id: outlinerOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 320; height: Math.min(parent.height * 0.85, 500); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Rectangle { anchors.fill: parent; color: "transparent"
            Text { anchors.centerIn: parent; text: "Scene Outliner\n(Load SceneOutliner.qml)"; color: "#666"; horizontalAlignment: Text.AlignHCenter }
        }
    }

    Rectangle { id: propsOverlay; visible: false; z: 10 }
    Rectangle { id: timelineOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 420; height: Math.min(parent.height * 0.7, 400); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Rectangle { anchors.fill: parent; color: "transparent"
            Text { anchors.centerIn: parent; text: "Animation Timeline\n(Load AnimationTimeline.qml)"; color: "#666"; horizontalAlignment: Text.AlignHCenter }
        }
    }

    function openSymmetryPanel() { symmetryPanel.visible = !symmetryPanel.visible }
    function openPaintPanel() { paintPanelOverlay.visible = !paintPanelOverlay.visible }
    function closePaintPanel() { paintPanelOverlay.visible = false }

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
        activateViewport(3)
    }
}
