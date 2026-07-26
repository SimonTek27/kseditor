import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window 2.15
import QtQuick3D
import Qt.labs.platform 1.1
import "../../widgets"
import ksEditor.Modeler 1.0
import ksEditor.Symmetry 1.0

ApplicationWindow {
    id: studio
    width: 1400
    height: 900
    minimumWidth: 1024
    minimumHeight: 600
    title: "KS Modeler Studio - " + currentFile
    color: "#111111"
    visible: true

    property string currentFile: "untitled.zm"
    property int viewMode: 0
    property string selectedObject: ""
    property int gizmoMode: 1

    property real camDistance: Modeler.camDistance
    property real camTheta: Modeler.camTheta
    property real camPhi: Modeler.camPhi
    property real camTargetX: Modeler.camTargetX
    property real camTargetY: Modeler.camTargetY
    property real camTargetZ: Modeler.camTargetZ

    property alias sceneModel: Modeler.sceneModel
    property int totalVerts: 0
    property int totalTris: 0
    property int dragAxis: -1
    property bool animLoop: true

    property bool canUndo: Modeler.canUndo()
    property bool canRedo: Modeler.canRedo()

    function notifySelection(name, id) {
        selectedObject = name
    }

    function updateStats() {
        totalVerts = sceneModel.totalVertices
        totalTris = sceneModel.totalTriangles
    }

    function openSymmetryPanel() {
        symmetryPanel.visible = !symmetryPanel.visible
    }

    function openPaintPanel() {
        paintPanelOverlay.visible = !paintPanelOverlay.visible
    }

    function closePaintPanel() {
        paintPanelOverlay.visible = false
    }

    menuBar: MenuBar {
        Menu {
            title: "File"
            Action { text: "New Scene"; shortcut: "Ctrl+N"; onTriggered: Modeler.newScene() }
            Action { text: "Open..."; shortcut: "Ctrl+O"; onTriggered: openDialog.open() }
            Action { text: "Save"; shortcut: "Ctrl+S"; onTriggered: Modeler.saveScene(currentFile) }
            Action { text: "Save As..."; shortcut: "Ctrl+Shift+S"; onTriggered: saveDialog.open() }
            MenuSeparator {}
            Action { text: "Import..."; shortcut: "Ctrl+I"; onTriggered: importDialog.open() }
            MenuSeparator {}
            Action { text: "Export KN5"; onTriggered: Modeler.exportKN5() }
            Action { text: "Export FBX"; onTriggered: Modeler.exportFBX() }
            Action { text: "Export OBJ"; onTriggered: Modeler.exportOBJ() }
            MenuSeparator {}
            Action { text: "Exit"; shortcut: "Ctrl+Q"; onTriggered: Qt.quit() }
        }

        Menu {
            title: "Edit"
            Action { text: "Undo"; shortcut: "Ctrl+Z"; onTriggered: Modeler.undo(); enabled: canUndo }
            Action { text: "Redo"; shortcut: "Ctrl+Shift+Z"; onTriggered: Modeler.redo(); enabled: canRedo }
            MenuSeparator {}
            Action { text: "Delete Selected"; shortcut: "Delete"; onTriggered: Modeler.deleteSelected(); enabled: Modeler.hasSelection }
            Action { text: "Select All"; shortcut: "Ctrl+A"; onTriggered: Modeler.selectAll() }
            Action { text: "Deselect All"; shortcut: "Ctrl+D"; onTriggered: Modeler.deselectAll() }
            MenuSeparator {}
            Action { text: "Focus on Selection"; shortcut: "F"; onTriggered: Modeler.focusOnSelected() }
        }

        Menu {
            title: "View"
            Action { text: "Top View"; shortcut: "1"; onTriggered: Modeler.setCameraView("top") }
            Action { text: "Right View"; shortcut: "3"; onTriggered: Modeler.setCameraView("right") }
            Action { text: "Front View"; shortcut: "7"; onTriggered: Modeler.setCameraView("front") }
            Action { text: "Perspective View"; shortcut: "5"; onTriggered: Modeler.setCameraView("persp") }
            MenuSeparator {}
            Action { text: "Toggle Grid"; shortcut: "G"; onTriggered: Modeler.gridVisible = !Modeler.gridVisible; checkable: true; checked: Modeler.gridVisible }
            MenuSeparator {}
            Action { text: "Wireframe Mode"; onTriggered: Modeler.viewMode = 1; checkable: true; checked: Modeler.viewMode === 1 }
            Action { text: "Solid Mode"; onTriggered: Modeler.viewMode = 0; checkable: true; checked: Modeler.viewMode === 0 }
            Action { text: "X-Ray Mode"; onTriggered: Modeler.viewMode = 2; checkable: true; checked: Modeler.viewMode === 2 }
            MenuSeparator {}
            Action { text: "Scene Outliner"; onTriggered: outlinerOverlay.visible = !outlinerOverlay.visible; checkable: true; checked: outlinerOverlay.visible }
            Action { text: "Properties"; onTriggered: propsOverlay.visible = !propsOverlay.visible; checkable: true; checked: propsOverlay.visible }
            Action { text: "Animation Timeline"; onTriggered: timelineOverlay.visible = !timelineOverlay.visible; checkable: true; checked: timelineOverlay.visible }
            Action { text: "Material Editor"; onTriggered: materialEditorOverlay.visible = !materialEditorOverlay.visible; checkable: true; checked: materialEditorOverlay.visible }
            Action { text: "Symmetry Editor"; shortcut: "Shift+S"; onTriggered: openSymmetryPanel(); checkable: true; checked: symmetryPanel.visible }
        }

        Menu {
            title: "Tools"
            Action { text: "Select (Q)"; onTriggered: Modeler.setGizmoMode(0) }
            Action { text: "Move (W)"; onTriggered: Modeler.setGizmoMode(1) }
            Action { text: "Rotate (E)"; onTriggered: Modeler.setGizmoMode(2) }
            Action { text: "Scale (R)"; onTriggered: Modeler.setGizmoMode(3) }
            MenuSeparator {}
            Action { text: "Boolean Operations"; onTriggered: boolOpPanel.visible = !boolOpPanel.visible }
            Action { text: "Symmetry Editor"; shortcut: "Shift+S"; onTriggered: openSymmetryPanel() }
            Action { text: "Texture Paint"; onTriggered: openPaintPanel() }
        }

        Menu {
            title: "Help"
            Action { text: "About KS Modeler"; onTriggered: aboutDialog.open() }
            Action { text: "Keyboard Shortcuts"; onTriggered: shortcutDialog.open() }
        }
    }

    header: Rectangle {
        height: 36
        color: "#252526"
        border.color: "#333333"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 4

            Text { text: "KS MODELER"; color: "#E10600"; font.pixelSize: 12; font.bold: true; font.letterSpacing: 2; rightPadding: 12 }

            Rectangle { width: 1; height: 20; color: "#444" }

            RowLayout {
                spacing: 2
                ToolButton {
                    text: "\u2611"; font.pixelSize: 14
                    ToolTip.visible: hovered; ToolTip.text: "New Scene"; ToolTip.delay: 500
                    onClicked: Modeler.newScene()
                    background: Rectangle { color: parent.hovered ? "#3a3a3e" : "transparent"; radius: 3 }
                }
                ToolButton {
                    text: "\u{1F4C2}"; font.pixelSize: 14
                    ToolTip.visible: hovered; ToolTip.text: "Open"; ToolTip.delay: 500
                    onClicked: openDialog.open()
                    background: Rectangle { color: parent.hovered ? "#3a3a3e" : "transparent"; radius: 3 }
                }
                ToolButton {
                    text: "\u{1F4BE}"; font.pixelSize: 14
                    ToolTip.visible: hovered; ToolTip.text: "Save"; ToolTip.delay: 500
                    onClicked: Modeler.saveScene(currentFile)
                    background: Rectangle { color: parent.hovered ? "#3a3a3e" : "transparent"; radius: 3 }
                }
            }

            Rectangle { width: 1; height: 20; color: "#444" }

            RowLayout {
                spacing: 2
                ToolButton {
                    text: "\u21A9"; font.pixelSize: 14
                    enabled: canUndo
                    opacity: enabled ? 1 : 0.4
                    ToolTip.visible: hovered; ToolTip.text: "Undo (Ctrl+Z)"; ToolTip.delay: 500
                    onClicked: Modeler.undo()
                    background: Rectangle { color: parent.hovered && parent.enabled ? "#3a3a3e" : "transparent"; radius: 3 }
                }
                ToolButton {
                    text: "\u21AA"; font.pixelSize: 14
                    enabled: canRedo
                    opacity: enabled ? 1 : 0.4
                    ToolTip.visible: hovered; ToolTip.text: "Redo (Ctrl+Shift+Z)"; ToolTip.delay: 500
                    onClicked: Modeler.redo()
                    background: Rectangle { color: parent.hovered && parent.enabled ? "#3a3a3e" : "transparent"; radius: 3 }
                }
            }

            Rectangle { width: 1; height: 20; color: "#444" }

            RowLayout {
                spacing: 2
                ToolButton {
                    text: "\u25B2"; font.pixelSize: 12
                    ToolTip.visible: hovered; ToolTip.text: "Move (W)"; ToolTip.delay: 500
                    onClicked: { gizmoMode = 1; Modeler.setGizmoMode(1) }
                    background: Rectangle { color: gizmoMode === 1 ? "#E10600" : (parent.hovered ? "#3a3a3e" : "transparent"); radius: 3 }
                }
                ToolButton {
                    text: "\u21BB"; font.pixelSize: 12
                    ToolTip.visible: hovered; ToolTip.text: "Rotate (E)"; ToolTip.delay: 500
                    onClicked: { gizmoMode = 2; Modeler.setGizmoMode(2) }
                    background: Rectangle { color: gizmoMode === 2 ? "#E10600" : (parent.hovered ? "#3a3a3e" : "transparent"); radius: 3 }
                }
                ToolButton {
                    text: "\u2194"; font.pixelSize: 12
                    ToolTip.visible: hovered; ToolTip.text: "Scale (R)"; ToolTip.delay: 500
                    onClicked: { gizmoMode = 3; Modeler.setGizmoMode(3) }
                    background: Rectangle { color: gizmoMode === 3 ? "#E10600" : (parent.hovered ? "#3a3a3e" : "transparent"); radius: 3 }
                }
            }

            Rectangle { width: 1; height: 20; color: "#444" }

            RowLayout {
                spacing: 2
                ToolButton {
                    text: "\u25A0"; font.pixelSize: 12
                    ToolTip.visible: hovered; ToolTip.text: "Toggle Grid (G)"; ToolTip.delay: 500
                    onClicked: Modeler.gridVisible = !Modeler.gridVisible
                    background: Rectangle { color: parent.hovered ? "#3a3a3e" : "transparent"; radius: 3 }
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: currentFile
                color: "#888"
                font.pixelSize: 10
                font.italic: true
                rightPadding: 8
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                width: 260
                Layout.fillHeight: true
                color: "#181818"
            border.color: "#262626"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6

                Text {
                    text: "ASSETS"
                    color: "#ffffff"
                    font.pixelSize: 12
                    font.bold: true
                }

                TextField {
                    id: searchField
                    placeholderText: "Search assets..."
                    Layout.fillWidth: true
                    font.pixelSize: 10
                    color: "#ccc"
                    background: Rectangle { color: "#252526"; radius: 3; border.color: "#444"; border.width: 1 }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    AppButton { text: "Materials"; height: 24; font.pixelSize: 10; bgcolor: "#E10600"; color: "#121212"; Layout.fillWidth: true }
                    AppButton { text: "Textures"; height: 24; font.pixelSize: 10; bgcolor: "#3e3e42"; color: "#ffffff"; Layout.fillWidth: true }
                    AppButton { text: "Objects"; height: 24; font.pixelSize: 10; bgcolor: "#3e3e42"; color: "#ffffff"; Layout.fillWidth: true }
                }

                ListView {
                    id: assetList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: assetModel

                    delegate: Rectangle {
                        height: 28
                        width: parent.width
                        color: ListView.isCurrentItem ? "#224466" : "transparent"

                        Rectangle {
                            width: 16; height: 16; radius: 2
                            color: model.assetIcon
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.leftMargin: 4
                        }
                        Text {
                            text: model.assetName
                            color: "#dddddd"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            anchors.left: parent.left; anchors.leftMargin: 26
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: model.assetType
                            color: "#666666"
                            font.pixelSize: 9
                            anchors.right: parent.right; anchors.rightMargin: 6
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: assetList.currentIndex = index
                            onDoubleClicked: {
                                if (model.assetPath !== "")
                                    console.log("Open asset:", model.assetPath)
                            }
                        }
                    }

                    ScrollBar.vertical: ScrollBar { active: true }
                }
            }
        }

        Rectangle {
            id: viewport
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#101010"
            border.color: "#202020"

            View3D {
                id: view3d
                anchors.fill: parent
                environment: SceneEnvironment {
                    clearColor: "#1a1a1e"
                    backgroundMode: SceneEnvironment.Color
                    antialiasingMode: SceneEnvironment.MSAA
                    antialiasingQuality: SceneMedium
                }

                PerspectiveCamera {
                    id: camera
                    position: Qt.vector3d(
                        camTargetX + camDistance * Math.cos(camTheta * Math.PI / 180) * Math.cos(camPhi * Math.PI / 180),
                        camTargetY + camDistance * Math.sin(camPhi * Math.PI / 180),
                        camTargetZ + camDistance * Math.sin(camTheta * Math.PI / 180) * Math.cos(camPhi * Math.PI / 180)
                    )
                    lookAt: Qt.vector3d(camTargetX, camTargetY, camTargetZ)
                    clipNear: 0.1
                    clipFar: 1000
                }

                DirectionalLight {
                    eulerRotation: Qt.vector3d(-45, 45, 0)
                    brightness: 1.2
                    castShadows: true
                }

                DirectionalLight {
                    eulerRotation: Qt.vector3d(45, -45, 0)
                    brightness: 0.4
                }

                AmbientLight {
                    brightness: 0.3
                }

                Model {
                    source: "#Rectangle"
                    visible: Modeler.gridVisible
                    scale: Qt.vector3d(20, 20, 1)
                    eulerRotation: Qt.vector3d(90, 0, 0)
                    position: Qt.vector3d(0, -0.01, 0)
                    materials: [
                        DefaultMaterial {
                            diffuseColor: "#2a2a2e"
                            diffuseMap: GridTexture {
                                width: 512; height: 512
                                lineColor: "#3a3a3e"
                                backgroundColor: "#2a2a2e"
                                gridSize: 20
                            }
                        }
                    ]
                }

                Model {
                    source: "#Cylinder"
                    scale: Qt.vector3d(0.02, 2, 0.02)
                    position: Qt.vector3d(1, 0, 0)
                    materials: [DefaultMaterial { diffuseColor: "#ff4444" }]
                }
                Model {
                    source: "#Cylinder"
                    scale: Qt.vector3d(0.02, 2, 0.02)
                    position: Qt.vector3d(0, 1, 0)
                    materials: [DefaultMaterial { diffuseColor: "#44ff44" }]
                }
                Model {
                    source: "#Cylinder"
                    scale: Qt.vector3d(0.02, 2, 0.02)
                    position: Qt.vector3d(0, 0, 1)
                    materials: [DefaultMaterial { diffuseColor: "#4444ff" }]
                }

                Node {
                    id: sceneObjectsRoot

                    Repeater {
                        id: sceneRepeater
                        model: sceneModel

                        delegate: Model {
                            id: sceneObjModel
                            property int objId: model.objectId

                            geometry: SceneMeshGeometry {
                                id: meshGeom
                                objectId: sceneObjModel.objId
                            }

                            position: model.objectPosition !== undefined ? model.objectPosition : Qt.vector3d(0, 0, 0)
                            eulerRotation: model.objectRotation !== undefined ? model.objectRotation : Qt.vector3d(0, 0, 0)
                            scale: model.objectScale !== undefined ? model.objectScale : Qt.vector3d(1, 1, 1)
                            visible: model.objectVisible !== undefined ? model.objectVisible : true

                            property Material matShaded: PrincipledMaterial {
                                baseColor: model.baseColor
                                metalness: model.metallic
                                roughness: model.roughness
                                opacity: model.opacity
                                emissiveFactor: model.objectSelected ? Qt.vector3d(0.12, 0.18, 0.35) : Qt.vector3d(0, 0, 0)
                            }

                            property Material matWire: DefaultMaterial {
                                lighting: DefaultMaterial.NoLighting
                                diffuseColor: model.objectSelected ? "#5588bb" : "#445566"
                                opacity: 0.9
                            }

                            property Material matXray: PrincipledMaterial {
                                baseColor: model.baseColor
                                metalness: model.metallic
                                roughness: model.roughness
                                opacity: 0.2
                                emissiveFactor: model.objectSelected ? Qt.vector3d(0.3, 0.4, 0.6) : Qt.vector3d(0, 0, 0)
                            }

                            materials: Modeler.viewMode === 0 ? [matShaded] : (Modeler.viewMode === 1 ? [matWire] : [matXray])

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton
                                onClicked: {
                                    notifySelection(model.objectName, model.objectId)
                                }
                            }

                            Connections {
                                target: Modeler
                                function onSceneChanged() {
                                    meshGeom.rebuild()
                                }
                            }
                        }
                    }
                }

                Node {
                    id: gizmoNode
                    visible: Modeler.gizmoMode > 0 && Modeler.hasSelection
                    position: Modeler.gizmoPosition

                    property string _moveHead: "#Cone"
                    property string _rotHead: "#Torus"
                    property string _scaleHead: "#Cube"
                    property string _headSrc: gizmoNode.visible ? (Modeler.gizmoMode === 1 ? _moveHead : (Modeler.gizmoMode === 2 ? _rotHead : _scaleHead)) : ""

                    Model {
                        id: gizmoX
                        pickable: true
                        source: "#Cylinder"
                        position: Qt.vector3d(0.5, 0, 0)
                        scale: dragAxis === 0 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02)
                        eulerRotation: Qt.vector3d(0, 0, -90)
                        materials: [DefaultMaterial { diffuseColor: dragAxis === 0 ? "#ff8888" : "#ff4444" }]
                        onPressed: (pick) => { if (pick.button === Qt.LeftButton) dragAxis = 0 }
                    }
                    Model {
                        pickable: true
                        source: gizmoNode._headSrc
                        position: Modeler.gizmoMode === 2 ? Qt.vector3d(0, 0, 0) : Qt.vector3d(1.0, 0, 0)
                        scale: {
                            if (dragAxis === 0) {
                                if (Modeler.gizmoMode === 2) return Qt.vector3d(0.35, 0.35, 0.08)
                                return Qt.vector3d(0.08, 0.25, 0.08)
                            }
                            if (Modeler.gizmoMode === 2) return Qt.vector3d(0.3, 0.3, 0.06)
                            return Qt.vector3d(0.06, 0.2, 0.06)
                        }
                        eulerRotation: {
                            if (Modeler.gizmoMode === 1 || Modeler.gizmoMode === 3) return Qt.vector3d(0, 0, -90)
                            return Qt.vector3d(0, 90, 0)
                        }
                        materials: [DefaultMaterial { diffuseColor: dragAxis === 0 ? "#ff8888" : "#ff4444" }]
                        onPressed: (pick) => { if (pick.button === Qt.LeftButton) dragAxis = 0 }
                    }

                    Model {
                        id: gizmoY
                        pickable: true
                        source: "#Cylinder"
                        position: Qt.vector3d(0, 0.5, 0)
                        scale: dragAxis === 1 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02)
                        materials: [DefaultMaterial { diffuseColor: dragAxis === 1 ? "#88ff88" : "#44ff44" }]
                        onPressed: (pick) => { if (pick.button === Qt.LeftButton) dragAxis = 1 }
                    }
                    Model {
                        pickable: true
                        source: gizmoNode._headSrc
                        position: Modeler.gizmoMode === 2 ? Qt.vector3d(0, 0, 0) : Qt.vector3d(0, 1.0, 0)
                        scale: {
                            if (dragAxis === 1) {
                                if (Modeler.gizmoMode === 2) return Qt.vector3d(0.35, 0.35, 0.08)
                                return Qt.vector3d(0.08, 0.25, 0.08)
                            }
                            if (Modeler.gizmoMode === 2) return Qt.vector3d(0.3, 0.3, 0.06)
                            return Qt.vector3d(0.06, 0.2, 0.06)
                        }
                        eulerRotation: {
                            if (Modeler.gizmoMode === 1 || Modeler.gizmoMode === 3) return Qt.vector3d(0, 0, 0)
                            return Qt.vector3d(90, 0, 0)
                        }
                        materials: [DefaultMaterial { diffuseColor: dragAxis === 1 ? "#88ff88" : "#44ff44" }]
                        onPressed: (pick) => { if (pick.button === Qt.LeftButton) dragAxis = 1 }
                    }

                    Model {
                        id: gizmoZ
                        pickable: true
                        source: "#Cylinder"
                        position: Qt.vector3d(0, 0, 0.5)
                        scale: dragAxis === 2 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02)
                        eulerRotation: Qt.vector3d(90, 0, 0)
                        materials: [DefaultMaterial { diffuseColor: dragAxis === 2 ? "#8888ff" : "#4444ff" }]
                        onPressed: (pick) => { if (pick.button === Qt.LeftButton) dragAxis = 2 }
                    }
                    Model {
                        pickable: true
                        source: gizmoNode._headSrc
                        position: Modeler.gizmoMode === 2 ? Qt.vector3d(0, 0, 0) : Qt.vector3d(0, 0, 1.0)
                        scale: {
                            if (dragAxis === 2) {
                                if (Modeler.gizmoMode === 2) return Qt.vector3d(0.35, 0.35, 0.08)
                                return Qt.vector3d(0.08, 0.25, 0.08)
                            }
                            if (Modeler.gizmoMode === 2) return Qt.vector3d(0.3, 0.3, 0.06)
                            return Qt.vector3d(0.06, 0.2, 0.06)
                        }
                        eulerRotation: {
                            if (Modeler.gizmoMode === 1 || Modeler.gizmoMode === 3) return Qt.vector3d(90, 0, 0)
                            return Qt.vector3d(0, 0, 0)
                        }
                        materials: [DefaultMaterial { diffuseColor: dragAxis === 2 ? "#8888ff" : "#4444ff" }]
                        onPressed: (pick) => { if (pick.button === Qt.LeftButton) dragAxis = 2 }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                    property point lastPos

                    onPressed: (mouse) => {
                        lastPos = Qt.point(mouse.x, mouse.y)
                    }

                    onPositionChanged: (mouse) => {
                        var dx = mouse.x - lastPos.x
                        var dy = mouse.y - lastPos.y
                        lastPos = Qt.point(mouse.x, mouse.y)

                        if (dragAxis >= 0 && (mouse.buttons & Qt.LeftButton)) {
                            var gScale = camDistance * 0.005
                            var gx = 0, gy = 0, gz = 0
                            if (dragAxis === 0) gx = dx * gScale
                            else if (dragAxis === 1) gy = dy * gScale
                            else if (dragAxis === 2) gz = dx * gScale
                            if (Modeler.gizmoMode === 1) {
                                Modeler.translateSelected(gx, gy, gz)
                            } else if (Modeler.gizmoMode === 2) {
                                Modeler.rotateSelected(gx * 90, gy * 90, gz * 90)
                            } else if (Modeler.gizmoMode === 3) {
                                var sScale = 1 + dx * 0.01
                                Modeler.scaleSelected(
                                    dragAxis === 0 ? sScale : 1,
                                    dragAxis === 1 ? sScale : 1,
                                    dragAxis === 2 ? sScale : 1
                                )
                            }
                        } else if (mouse.buttons & Qt.LeftButton) {
                            Modeler.camTheta = camTheta - dx * 0.5
                            Modeler.camPhi = Math.max(-89, Math.min(89, camPhi + dy * 0.5))
                        } else if (mouse.buttons & Qt.MiddleButton) {
                            var panSpeed = camDistance * 0.002
                            Modeler.camTargetX = camTargetX - dx * panSpeed
                            Modeler.camTargetY = camTargetY + dy * panSpeed
                        } else if (mouse.buttons & Qt.RightButton) {
                            Modeler.camDistance = Math.max(0.5, Math.min(100, camDistance * (1 + dy * 0.01)))
                        }
                    }

                    onReleased: (mouse) => {
                        dragAxis = -1
                    }

                    onWheel: (wheel) => {
                        Modeler.camDistance = Math.max(0.5, Math.min(100, camDistance * (1 + wheel.angleDelta.y * 0.001)))
                    }
                }

                Node {
                    id: boneOverlay
                    visible: Modeler.boneCount() > 0

                    Repeater {
                        model: Modeler.boneCount()

                        Model {
                            source: "#Sphere"
                            scale: Qt.vector3d(0.08, 0.08, 0.08)
                            position: {
                                Modeler.boneVersion
                                var p = Modeler.getBoneWorldPosition(index)
                                return Qt.vector3d(p.x, p.y, p.z)
                            }
                            materials: [
                                DefaultMaterial {
                                    diffuseColor: Modeler.selectedBoneIndex() === index ? "#ffaa00" : "#4488ff"
                                    lighting: DefaultMaterial.NoLighting
                                }
                            ]
                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton
                                onClicked: Modeler.selectBone(index)
                            }
                        }
                    }

                    Repeater {
                        model: Modeler.boneCount()

                        Model {
                            source: "#Cone"
                            visible: {
                                Modeler.boneVersion
                                var pid = Modeler.getBoneParentId(index)
                                return pid >= 0 && pid < Modeler.boneCount() && pid !== index
                            }
                            scale: Qt.vector3d(0.04, 0.04, 0.04)
                            position: {
                                Modeler.boneVersion
                                var p = Modeler.getBoneWorldPosition(index)
                                return Qt.vector3d(p.x, p.y, p.z)
                            }
                            materials: [
                                DefaultMaterial {
                                    diffuseColor: "#4488ff"
                                    lighting: DefaultMaterial.NoLighting
                                }
                            ]
                        }
                    }
                }
            }

            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 4
                width: viewLabel.contentWidth + 12; height: 20
                color: "#202020"; border.color: "#303030"; radius: 2

                Text {
                    id: viewLabel
                    anchors.centerIn: parent
                    text: Modeler.currentViewName
                    color: "#bbbbbb"
                    font.pixelSize: 10
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.margins: 8
                width: 200; height: 140
                color: "#1a1a1e"; border.color: "#333"; border.width: 1; radius: 4
                opacity: 0.85

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 6; spacing: 2

                    Text { text: "VIEWPORT INFO"; color: "#E10600"; font.pixelSize: 9; font.bold: true }
                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "Camera: " + Math.round(camTheta) + "\u00b0 / " + Math.round(camPhi) + "\u00b0"; color: "#888"; font.pixelSize: 9 }
                    Text { text: "Distance: " + camDistance.toFixed(1); color: "#888"; font.pixelSize: 9 }
                    Text { text: "Objects: " + sceneModel.rowCount(); color: "#888"; font.pixelSize: 9 }
                    Text { text: "Verts: " + totalVerts; color: "#888"; font.pixelSize: 9 }
                    Text { text: "Tris: " + totalTris; color: "#888"; font.pixelSize: 9 }
                }
            }

            Loader {
                id: toolsPanelLoader
                anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
                width: 300
                source: "modeler_Tools.qml"
                onItemChanged: {
                    if (item) {
                        item.toolSelected.connect(function(tool) {
                            if (tool === "paint") openPaintPanel()
                            else closePaintPanel()
                            paintPanelOverlay.visible = (tool === "paint")
                        })
                    }
                }
            }

            Rectangle {
                anchors.top: parent.top; anchors.right: parent.right; anchors.topMargin: 8; anchors.rightMargin: 308
                width: 28; height: 28; radius: 4; z: 10
                color: symmetryPanel.visible ? "#E10600" : "#3e3e42"
                border.color: symmetryPanel.visible ? "#ff4444" : "#555"; border.width: 1

                Text { anchors.centerIn: parent; text: "<>"; color: "#ffffff"; font.pixelSize: 14; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: openSymmetryPanel(); hoverEnabled: true }
                ToolTip.visible: hovered; ToolTip.text: "Symmetry (Shift+S)"; ToolTip.delay: 800
            }

            Rectangle {
                anchors.top: parent.top; anchors.right: parent.right; anchors.topMargin: 8; anchors.rightMargin: 344
                width: 28; height: 28; radius: 4; z: 10
                color: materialEditorOverlay.visible ? "#E10600" : "#3e3e42"
                border.color: materialEditorOverlay.visible ? "#ff4444" : "#555"; border.width: 1

                Text { anchors.centerIn: parent; text: "M"; color: "#ffffff"; font.pixelSize: 14; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: materialEditorOverlay.visible = !materialEditorOverlay.visible; hoverEnabled: true }
                ToolTip.visible: hovered; ToolTip.text: "Material Editor"; ToolTip.delay: 800
            }

            Rectangle {
                anchors.top: parent.top; anchors.right: parent.right; anchors.topMargin: 8; anchors.rightMargin: 380
                width: 28; height: 28; radius: 4; z: 10
                color: outlinerOverlay.visible ? "#E10600" : "#3e3e42"
                border.color: outlinerOverlay.visible ? "#ff4444" : "#555"; border.width: 1

                Text { anchors.centerIn: parent; text: "O"; color: "#ffffff"; font.pixelSize: 14; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: outlinerOverlay.visible = !outlinerOverlay.visible; hoverEnabled: true }
                ToolTip.visible: hovered; ToolTip.text: "Scene Outliner"; ToolTip.delay: 800
            }

            Rectangle {
                anchors.top: parent.top; anchors.right: parent.right; anchors.topMargin: 8; anchors.rightMargin: 416
                width: 28; height: 28; radius: 4; z: 10
                color: propsOverlay.visible ? "#E10600" : "#3e3e42"
                border.color: propsOverlay.visible ? "#ff4444" : "#555"; border.width: 1

                Text { anchors.centerIn: parent; text: "P"; color: "#ffffff"; font.pixelSize: 14; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: propsOverlay.visible = !propsOverlay.visible; hoverEnabled: true }
                ToolTip.visible: hovered; ToolTip.text: "Properties"; ToolTip.delay: 800
            }

            Rectangle {
                anchors.top: parent.top; anchors.right: parent.right; anchors.topMargin: 8; anchors.rightMargin: 452
                width: 28; height: 28; radius: 4; z: 10
                color: timelineOverlay.visible ? "#E10600" : "#3e3e42"
                border.color: timelineOverlay.visible ? "#ff4444" : "#555"; border.width: 1

                Text { anchors.centerIn: parent; text: "T"; color: "#ffffff"; font.pixelSize: 14; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: timelineOverlay.visible = !timelineOverlay.visible; hoverEnabled: true }
                ToolTip.visible: hovered; ToolTip.text: "Animation Timeline"; ToolTip.delay: 800
            }
        }
    }

        Rectangle {
            height: 36
            color: "#1a1a1a"
            Layout.fillWidth: true
            visible: Modeler.animationName.length > 0

            RowLayout {
                anchors.fill: parent; anchors.margins: 3; spacing: 4

                ToolButton {
                    text: Modeler.isAnimating ? "\u23F8" : "\u23F5"
                    font.pixelSize: 14
                    implicitWidth: 28; implicitHeight: 28; flat: true
                    onClicked: Modeler.togglePlayPause()
                    ToolTip.visible: hovered; ToolTip.text: Modeler.isAnimating ? "Pause" : "Play"
                    background: Rectangle { color: parent.hovered ? "#333" : "transparent"; radius: 3 }
                    contentItem: Text { text: parent.text; color: "#ccc"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                ToolButton {
                    text: "\u23F9"; font.pixelSize: 14
                    implicitWidth: 28; implicitHeight: 28; flat: true
                    onClicked: Modeler.stopAnimation()
                    background: Rectangle { color: parent.hovered ? "#333" : "transparent"; radius: 3 }
                    contentItem: Text { text: parent.text; color: "#ccc"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                ToolButton {
                    text: "+"; font.pixelSize: 14
                    implicitWidth: 22; implicitHeight: 22; flat: true
                    ToolTip.visible: hovered; ToolTip.text: "New animation"
                    onClicked: {
                        var count = Modeler.animationNames().length
                        Modeler.addAnimation("Animation " + (count + 1), 5.0)
                        Modeler.setCurrentAnimationByName("Animation " + (count + 1))
                    }
                    background: Rectangle { color: parent.hovered ? "#333" : "transparent"; radius: 3 }
                    contentItem: Text { text: parent.text; color: "#888"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                ComboBox {
                    id: animSelector
                    model: Modeler.animationNames()
                    currentIndex: {
                        var idx = -1; var names = Modeler.animationNames()
                        for (var i = 0; i < names.length; ++i) { if (names[i] === Modeler.animationName) { idx = i; break } }
                        return idx
                    }
                    implicitWidth: 100; font.pixelSize: 11; flat: true
                    onActivated: Modeler.setCurrentAnimationByName(currentText)
                    background: Rectangle { color: parent.hovered ? "#333" : "transparent"; radius: 3 }
                    contentItem: Text { text: parent.currentText || "(no animation)"; color: "#aaa"; font.bold: true; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter }
                    indicator: Text { text: "\u25BC"; color: "#666"; font.pixelSize: 8; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 4 }
                }
                Text {
                    text: Math.floor(Modeler.animationTime) + " / " + Math.floor(Modeler.animationDuration)
                    color: "#999"; font.pixelSize: 11; font.family: "monospace"; width: 70
                }
                Slider {
                    id: timelineSlider
                    Layout.fillWidth: true; implicitHeight: 20
                    from: 0; to: Math.max(0.01, Modeler.animationDuration); value: 0
                    onMoved: Modeler.setAnimationTime(value)
                    background: Rectangle {
                        implicitHeight: 4; color: "#333"; radius: 2
                        Rectangle { width: parent.width * (parent.parent.value / parent.parent.to); height: parent.height; color: "#5555cc"; radius: 2 }
                    }
                    handle: Rectangle { implicitWidth: 10; implicitHeight: 16; radius: 2; color: "#7777dd"; x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width); y: (parent.height - height) / 2 }
                    Connections {
                        target: Modeler
                        function onAnimationTimeChanged() {
                            timelineSlider.to = Math.max(0.01, Modeler.animationDuration)
                            if (!timelineSlider.pressed) timelineSlider.value = Modeler.animationTime
                        }
                        function onAnimationNameChanged() { animSelector.model = Modeler.animationNames() }
                    }
                }
                ToolButton {
                    text: "\u25CF"; font.pixelSize: 14
                    implicitWidth: 28; implicitHeight: 28; flat: true
                    enabled: Modeler.hasSelection && Modeler.animationName.length > 0
                    onClicked: Modeler.addKeyframeForSelectedObject(Modeler.animationName)
                    ToolTip.visible: hovered; ToolTip.text: "Add keyframe (K)"
                    background: Rectangle { color: parent.hovered && parent.enabled ? "#993333" : "transparent"; radius: 3 }
                    contentItem: Text { text: parent.text; color: parent.enabled ? "#cc5555" : "#444"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                ToolButton {
                    text: "\u21BA"; font.pixelSize: 14
                    implicitWidth: 28; implicitHeight: 28; flat: true
                    checkable: true; checked: animLoop
                    onClicked: { animLoop = checked; Modeler.setAnimationLoop(checked) }
                    ToolTip.visible: hovered; ToolTip.text: "Loop"
                    background: Rectangle { color: parent.checked ? "#334466" : (parent.hovered ? "#333" : "transparent"); radius: 3 }
                    contentItem: Text { text: parent.text; color: parent.checked ? "#88aaff" : "#888"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                Text {
                    text: Modeler.animationFps + " fps"
                    color: "#666"; font.pixelSize: 10; width: 40
                }
            }
        }

        Rectangle {
            height: 22
            color: "#181818"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent; anchors.margins: 4

                Text { text: "Ready"; color: "#10b981"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "Camera: " + Math.round(camTheta) + "\u00b0 / " + Math.round(camPhi) + "\u00b0  Distance: " + camDistance.toFixed(1); color: "#666"; font.pixelSize: 10 }
                Item { width: 8 }
                Text { text: "Objects: " + sceneModel.rowCount(); color: "#666"; font.pixelSize: 10 }
                Item { width: 8 }
                Text { text: "Verts: " + totalVerts + "  Tris: " + totalTris; color: "#666"; font.pixelSize: 10 }
            }
        }
    }

    Rectangle {
        id: materialEditorOverlay
        visible: false; z: 10
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8
        width: 320; height: Math.min(parent.height * 0.9, 620)
        color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4

        MaterialEditor {
            anchors.fill: parent
            onCloseRequested: materialEditorOverlay.visible = false
        }
    }

    Rectangle {
        id: outlinerOverlay
        visible: false; z: 10
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8
        width: 320; height: Math.min(parent.height * 0.85, 500)
        color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4

        SceneOutliner {
            anchors.fill: parent
            onCloseRequested: outlinerOverlay.visible = false
        }
    }

    Rectangle {
        id: propsOverlay
        visible: false; z: 10
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8
        width: 300; height: Math.min(parent.height * 0.85, 520)
        color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4

        PropertiesPanel {
            anchors.fill: parent
            onCloseRequested: propsOverlay.visible = false
        }
    }

    Rectangle {
        id: timelineOverlay
        visible: false; z: 10
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8
        width: 420; height: Math.min(parent.height * 0.7, 400)
        color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4

        AnimationTimeline {
            anchors.fill: parent
            onCloseRequested: timelineOverlay.visible = false
        }
    }

    SymmetryPanel {
        id: symmetryPanel
        visible: false; z: 10
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8
        onClosePanel: symmetryPanel.visible = false
    }

    BoolOpPanel {
        id: boolOpPanel
        visible: false; z: 10
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8
        onClosePanel: boolOpPanel.visible = false
    }

    Rectangle {
        id: paintPanelOverlay
        visible: false; z: 10
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8
        width: 280; height: Math.min(parent.height * 0.85, 600)
        color: "transparent"

        Loader {
            anchors.fill: parent
            source: "paint_TexturePanel.qml"
        }

        Rectangle {
            anchors.top: parent.top; anchors.right: parent.right
            width: 18; height: 18; radius: 2; color: "#E10600"
            Text { anchors.centerIn: parent; text: "X"; color: "#fff"; font.pixelSize: 10; font.bold: true }
            MouseArea { anchors.fill: parent; onClicked: closePaintPanel() }
        }
    }

    Dialog {
        id: aboutDialog
        title: "About KS Modeler"
        standardButtons: Dialog.Ok
        modal: true
        anchors.centerIn: parent
        width: 320; height: 200

        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 8

            Text { text: "KS Modeler Studio"; color: "#E10600"; font.pixelSize: 18; font.bold: true }
            Text { text: "Kunos Simulazioni 3D Model Editor"; color: "#888"; font.pixelSize: 11 }
            Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }
            Text { text: "Integrated modeling suite for Assetto Corsa"; color: "#aaa"; font.pixelSize: 10; wrapMode: Text.WordWrap }
            Text { text: "Version 1.0"; color: "#666"; font.pixelSize: 9 }
        }
    }

    Dialog {
        id: shortcutDialog
        title: "Keyboard Shortcuts"
        standardButtons: Dialog.Close
        modal: true
        anchors.centerIn: parent
        width: 400; height: 420

        ColumnLayout {
            anchors.fill: parent; anchors.margins: 12; spacing: 4

            Text { text: "KEYBOARD SHORTCUTS"; color: "#E10600"; font.pixelSize: 13; font.bold: true; bottomPadding: 6 }

            Repeater {
                model: [
                    "Q - Select mode", "W - Move (Translate)", "E - Rotate", "R - Scale",
                    "G - Toggle Grid", "F - Focus on Selection",
                    "Delete - Delete Selected",
                    "K - Add Keyframe", "Space - Play/Pause Animation",
                    "Shift+S - Symmetry Editor",
                    "1 - Top View", "3 - Right View", "5 - Perspective", "7 - Front View",
                    "Ctrl+Z - Undo", "Ctrl+Shift+Z - Redo",
                    "Ctrl+N - New Scene", "Ctrl+O - Open",
                    "Ctrl+S - Save", "Ctrl+Shift+S - Save As"
                ]

                Text {
                    text: modelData
                    color: "#bbb"; font.pixelSize: 10; font.family: "monospace"
                    leftPadding: 8
                }
            }
        }
    }

    FileDialog {
        id: openDialog
        title: "Open Scene"
        nameFilters: ["KS Scene (*.zm *.kn5)", "All Files (*)"]
        onAccepted: {
            currentFile = file.toString().replace("file:///", "")
            Modeler.loadScene(currentFile)
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save Scene As"
        nameFilters: ["KS Scene (*.zm)", "KN5 (*.kn5)"]
        onAccepted: {
            currentFile = file.toString().replace("file:///", "")
            Modeler.saveScene(currentFile)
        }
    }

    FileDialog {
        id: importDialog
        title: "Import Model"
        nameFilters: ["3D Models (*.fbx *.obj *.gltf *.glb *.dae *.3ds)", "All Files (*)"]
        onAccepted: {
            var path = file.toString().replace("file:///", "")
            Modeler.importModel(path)
        }
    }

    Shortcut { sequence: "Q"; onActivated: { gizmoMode = 0; Modeler.setGizmoMode(0) } }
    Shortcut { sequence: "W"; onActivated: { gizmoMode = 1; Modeler.setGizmoMode(1) } }
    Shortcut { sequence: "E"; onActivated: { gizmoMode = 2; Modeler.setGizmoMode(2) } }
    Shortcut { sequence: "R"; onActivated: { gizmoMode = 3; Modeler.setGizmoMode(3) } }
    Shortcut { sequence: "G"; onActivated: Modeler.gridVisible = !Modeler.gridVisible }
    Shortcut { sequence: "F"; onActivated: Modeler.focusOnSelected() }
    Shortcut { sequence: "Delete"; onActivated: Modeler.deleteSelected() }
    Shortcut { sequence: "K"; onActivated: { if (Modeler.animationName.length > 0 && Modeler.hasSelection) Modeler.addKeyframeForSelectedObject(Modeler.animationName) } }
    Shortcut { sequence: "Space"; onActivated: { if (Modeler.animationName.length > 0) Modeler.togglePlayPause() } }
    Shortcut { sequence: "Shift+S"; onActivated: openSymmetryPanel() }
    Shortcut { sequence: "1"; onActivated: Modeler.setCameraView("top") }
    Shortcut { sequence: "3"; onActivated: Modeler.setCameraView("right") }
    Shortcut { sequence: "5"; onActivated: Modeler.setCameraView("persp") }
    Shortcut { sequence: "7"; onActivated: Modeler.setCameraView("front") }

    Component.onCompleted: {
        sceneModel.countChanged.connect(updateStats)
        updateStats()
    }
}
