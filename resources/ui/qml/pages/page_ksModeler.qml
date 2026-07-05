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

    property alias viewportArea: viewport
    property string currentFile: "untitled.zm"
    property int viewMode: 0
    property string selectedObject: ""

    // Camera state (mirrors Modeler bridge for orbit interaction)
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

    // Undo/Redo state
    property bool canUndo: Modeler.canUndo()
    property bool canRedo: Modeler.canRedo()

    function notifySelection(name, id) {
        selectedObject = name
        if (typeof mainWindow !== 'undefined' && mainWindow.onObjectSelected) {
            mainWindow.onObjectSelected(name, 0, 0, 0, 0, 0, 0, 1, 1, 1)
        }
    }

    function updateStats() {
        totalVerts = sceneModel.totalVertices
        totalTris = sceneModel.totalTriangles
    }

    // Keyboard handler per shortcuts
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Z && event.modifiers & Qt.ControlModifier) {
            if (event.modifiers & Qt.ShiftModifier) {
                Modeler.redo()
            } else {
                Modeler.undo()
            }
            event.accepted = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar con Undo/Redo
        Rectangle {
            height: 36
            color: "#252526"
            Layout.fillWidth: true
            border.color: "#333333"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4

                Text { text: "EDIT"; color: "#E10600"; font.pixelSize: 11; font.bold: true }

                // Undo button
                Rectangle {
                    width: 28
                    height: 28
                    radius: 2
                    color: pageKsModeler.canUndo ? "#3e3e42" : "#1a1a1e"
                    border.color: pageKsModeler.canUndo ? "#555" : "#333"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "↶"
                        color: pageKsModeler.canUndo ? "#ffffff" : "#666"
                        font.pixelSize: 14
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: if (pageKsModeler.canUndo) Modeler.undo()
                        hoverEnabled: true
                    }

                    ToolTip.visible: hovered && parent.enabled
                    ToolTip.text: "Undo (Ctrl+Z)"
                    ToolTip.delay: 500
                }

                // Redo button
                Rectangle {
                    width: 28
                    height: 28
                    radius: 2
                    color: pageKsModeler.canRedo ? "#3e3e42" : "#1a1a1e"
                    border.color: pageKsModeler.canRedo ? "#555" : "#333"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "↷"
                        color: pageKsModeler.canRedo ? "#ffffff" : "#666"
                        font.pixelSize: 14
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: if (pageKsModeler.canRedo) Modeler.redo()
                        hoverEnabled: true
                    }

                    ToolTip.visible: hovered && parent.enabled
                    ToolTip.text: "Redo (Ctrl+Shift+Z)"
                    ToolTip.delay: 500
                }

                Rectangle { width: 1; height: 20; color: "#444" } // Separator

                Item { Layout.fillWidth: true }
            }
        }

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
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Button { text: "Materials"; flat: true; font.pixelSize: 11; Layout.fillWidth: true; checked: true; checkable: true }
                        Button { text: "Textures"; flat: true; font.pixelSize: 11; Layout.fillWidth: true; checkable: true }
                        Button { text: "Objects"; flat: true; font.pixelSize: 11; Layout.fillWidth: true; checkable: true }
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
                                width: 16
                                height: 16
                                radius: 2
                                color: model.assetIcon
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 4
                            }
                            Text {
                                text: model.assetName
                                color: "#dddddd"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                anchors.left: parent.left
                                anchors.leftMargin: 26
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: model.assetType
                                color: "#666666"
                                font.pixelSize: 9
                                anchors.right: parent.right
                                anchors.rightMargin: 6
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: assetList.currentIndex = index
                                onDoubleClicked: {
                                    if (model.assetPath !== "") {
                                        console.log("Open asset:", model.assetPath)
                                    }
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

                    // Grid floor
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
                                    width: 512
                                    height: 512
                                    lineColor: "#3a3a3e"
                                    backgroundColor: "#2a2a2e"
                                    gridSize: 20
                                }
                            }
                        ]
                    }

                    // Coordinate axes
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

                    // Scene objects rendered dynamically from C++ scene graph
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

                    // Transform gizmo (axis arrows)
                    Node {
                        id: gizmoNode
                        visible: Modeler.gizmoMode > 0 && Modeler.hasSelection
                        position: Modeler.gizmoPosition

                        // Helper: source mesh per gizmo mode
                        property string _moveHead: "#Cone"
                        property string _rotHead: "#Torus"
                        property string _scaleHead: "#Cube"
                        property string _headSrc: gizmoNode.visible ? (Modeler.gizmoMode === 1 ? _moveHead : (Modeler.gizmoMode === 2 ? _rotHead : _scaleHead)) : ""

                        // X axis - red
                        Model {
                            id: gizmoX
                            pickable: true
                            source: "#Cylinder"
                            position: Qt.vector3d(0.5, 0, 0)
                            scale: dragAxis === 0 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02)
                            eulerRotation: Qt.vector3d(0, 0, -90)
                            materials: [DefaultMaterial { diffuseColor: dragAxis === 0 ? "#ff8888" : "#ff4444" }]
                            onPressed: (pick) => { if (pick.button === Qt.LeftButton) { dragAxis = 0; } }
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
                            onPressed: (pick) => { if (pick.button === Qt.LeftButton) { dragAxis = 0; } }
                        }

                        // Y axis - green
                        Model {
                            id: gizmoY
                            pickable: true
                            source: "#Cylinder"
                            position: Qt.vector3d(0, 0.5, 0)
                            scale: dragAxis === 1 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02)
                            materials: [DefaultMaterial { diffuseColor: dragAxis === 1 ? "#88ff88" : "#44ff44" }]
                            onPressed: (pick) => { if (pick.button === Qt.LeftButton) { dragAxis = 1; } }
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
                            onPressed: (pick) => { if (pick.button === Qt.LeftButton) { dragAxis = 1; } }
                        }

                        // Z axis - blue
                        Model {
                            id: gizmoZ
                            pickable: true
                            source: "#Cylinder"
                            position: Qt.vector3d(0, 0, 0.5)
                            scale: dragAxis === 2 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02)
                            eulerRotation: Qt.vector3d(90, 0, 0)
                            materials: [DefaultMaterial { diffuseColor: dragAxis === 2 ? "#8888ff" : "#4444ff" }]
                            onPressed: (pick) => { if (pick.button === Qt.LeftButton) { dragAxis = 2; } }
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
                            onPressed: (pick) => { if (pick.button === Qt.LeftButton) { dragAxis = 2; } }
                        }
                    }

                    // Mouse orbit controls
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
                                // Gizmo drag
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

                    // Bone visualization overlay
                    Node {
                        id: boneOverlay
                        visible: Modeler.boneCount() > 0

                        // Joint spheres at each bone position
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

                        // Bone direction indicators (cones from child toward parent)
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
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 4
                    width: viewLabel.contentWidth + 12
                    height: 20
                    color: "#202020"
                    border.color: "#303030"
                    radius: 2

                    Text {
                        id: viewLabel
                        anchors.centerIn: parent
                        text: Modeler.currentViewName
                        color: "#bbbbbb"
                        font.pixelSize: 10
                    }
                }
            }
        }

        // Animation timeline
        Rectangle {
            height: 36
            color: "#1a1a1a"
            Layout.fillWidth: true
            visible: Modeler.animationName.length > 0

            RowLayout {
                anchors.fill: parent
                anchors.margins: 3
                spacing: 4

                // Transport controls
                Button {
                    text: Modeler.isAnimating ? "\u23F8" : "\u23F5"
                    font.pixelSize: 14
                    implicitWidth: 28; implicitHeight: 28
                    flat: true
                    onClicked: Modeler.togglePlayPause()
                    ToolTip.visible: hovered; ToolTip.text: Modeler.isAnimating ? "Pause" : "Play"
                    background: Rectangle {
                        color: parent.hovered ? "#333333" : "transparent"
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text; color: "#cccccc"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }
                Button {
                    text: "\u23F9"
                    font.pixelSize: 14
                    implicitWidth: 28; implicitHeight: 28
                    flat: true
                    onClicked: Modeler.stopAnimation()
                    background: Rectangle {
                        color: parent.hovered ? "#333333" : "transparent"
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text; color: "#cccccc"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }

                // Add animation button
                Button {
                    text: "+"
                    font.pixelSize: 14
                    implicitWidth: 22; implicitHeight: 22
                    flat: true
                    ToolTip.visible: hovered; ToolTip.text: "New animation"
                    onClicked: {
                        var count = Modeler.animationNames().length
                        Modeler.addAnimation("Animation " + (count + 1), 5.0)
                        Modeler.setCurrentAnimationByName("Animation " + (count + 1))
                    }
                    background: Rectangle {
                        color: parent.hovered ? "#333333" : "transparent"
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text; color: "#888888"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }

                // Animation selector
                ComboBox {
                    id: animSelector
                    model: Modeler.animationNames()
                    currentIndex: {
                        var idx = -1
                        var names = Modeler.animationNames()
                        for (var i = 0; i < names.length; ++i) {
                            if (names[i] === Modeler.animationName) { idx = i; break }
                        }
                        return idx
                    }
                    implicitWidth: 100
                    font.pixelSize: 11
                    flat: true
                    onActivated: Modeler.setCurrentAnimationByName(currentText)
                    background: Rectangle {
                        color: parent.hovered ? "#333333" : "transparent"
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.currentText || "(no animation)"
                        color: "#aaaaaa"
                        font.bold: true
                        font.pixelSize: 11
                        verticalAlignment: Text.AlignVCenter
                    }
                    indicator: Text {
                        text: "\u25BC"
                        color: "#666666"
                        font.pixelSize: 8
                        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: 4
                    }
                }

                // Time display
                Text {
                    text: Math.floor(Modeler.animationTime) + " / " + Math.floor(Modeler.animationDuration)
                    color: "#999999"
                    font.pixelSize: 11
                    font.family: "monospace"
                    implicitWidth: 70
                }

                // Timeline slider
                Slider {
                    id: timelineSlider
                    Layout.fillWidth: true
                    implicitHeight: 20
                    from: 0; to: Math.max(0.01, Modeler.animationDuration)
                    value: 0
                    onMoved: Modeler.setAnimationTime(value)
                    background: Rectangle {
                        implicitHeight: 4
                        color: "#333333"
                        radius: 2
                        Rectangle {
                            width: parent.width * (parent.parent.value / parent.parent.to)
                            height: parent.height
                            color: "#5555cc"
                            radius: 2
                        }
                    }
                    handle: Rectangle {
                        implicitWidth: 10; implicitHeight: 16
                        radius: 2
                        color: "#7777dd"
                        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                        y: (parent.height - height) / 2
                    }
                }

                Connections {
                    target: Modeler
                    function onAnimationTimeChanged() {
                        timelineSlider.to = Math.max(0.01, Modeler.animationDuration)
                        if (!timelineSlider.pressed)
                            timelineSlider.value = Modeler.animationTime
                    }
                    function onAnimationNameChanged() {
                        animSelector.model = Modeler.animationNames()
                    }
                }

                // Keyframe button
                Button {
                    text: "\u25CF"
                    font.pixelSize: 14
                    implicitWidth: 28; implicitHeight: 28
                    flat: true
                    enabled: Modeler.hasSelection && Modeler.animationName.length > 0
                    onClicked: Modeler.addKeyframeForSelectedObject(Modeler.animationName)
                    ToolTip.visible: hovered; ToolTip.text: "Add keyframe (K)"
                    background: Rectangle {
                        color: parent.hovered && parent.enabled ? "#993333" : "transparent"
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text; color: parent.enabled ? "#cc5555" : "#444444"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }

                // Loop toggle
                Button {
                    text: "\u21BA"
                    font.pixelSize: 14
                    implicitWidth: 28; implicitHeight: 28
                    flat: true
                    checkable: true; checked: animLoop
                    onClicked: { animLoop = checked; Modeler.setAnimationLoop(checked); }
                    ToolTip.visible: hovered; ToolTip.text: "Loop"
                    background: Rectangle {
                        color: parent.checked ? "#334466" : (parent.hovered ? "#333333" : "transparent")
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text; color: parent.checked ? "#88aaff" : "#888888"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }

                // FPS label
                Text {
                    text: Modeler.animationFps + " fps"
                    color: "#666666"
                    font.pixelSize: 10
                    implicitWidth: 40
                }
            }
        }

        Rectangle {
            height: 22
            color: "#181818"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text {
                    text: "Ready"
                    color: "#10b981"
                    font.pixelSize: 10
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "Camera: " + Math.round(camTheta) + "° / " + Math.round(camPhi) + "°  Distance: " + camDistance.toFixed(1)
                    color: "#666666"
                    font.pixelSize: 10
                }
                Item { width: 8 }
                Text {
                    text: "Objects: " + sceneModel.rowCount()
                    color: "#666666"
                    font.pixelSize: 10
                }
                Item { width: 8 }
                Text {
                    text: "Verts: " + totalVerts + "  Tris: " + totalTris
                    color: "#666666"
                    font.pixelSize: 10
                }
            }
        }
    }

    // Keyboard shortcuts
    Shortcut { sequence: "Q"; onActivated: Modeler.setGizmoMode(0) }
    Shortcut { sequence: "W"; onActivated: Modeler.setGizmoMode(1) }
    Shortcut { sequence: "E"; onActivated: Modeler.setGizmoMode(2) }
    Shortcut { sequence: "R"; onActivated: Modeler.setGizmoMode(3) }
    Shortcut { sequence: "G"; onActivated: Modeler.gridVisible = !Modeler.gridVisible }
    Shortcut { sequence: "F"; onActivated: Modeler.focusOnSelected() }
    Shortcut { sequence: "Delete"; onActivated: Modeler.deleteSelected() }
    Shortcut { sequence: "K"; onActivated: { if (Modeler.animationName.length > 0 && Modeler.hasSelection) Modeler.addKeyframeForSelectedObject(Modeler.animationName) } }
    Shortcut { sequence: "Space"; onActivated: { if (Modeler.animationName.length > 0) Modeler.togglePlayPause() } }
    Shortcut { sequence: "Shift+S"; onActivated: openSymmetryPanel() }
    Shortcut { sequence: "1"; onActivated: Modeler.setCameraView("top") }
    Shortcut { sequence: "3"; onActivated: Modeler.setCameraView("right") }
    Shortcut { sequence: "7"; onActivated: Modeler.setCameraView("front") }
    Shortcut { sequence: "5"; onActivated: Modeler.setCameraView("persp") }

    // ── Symmetry toolbar button ───────────────────────────────────────
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 8
        width: 28
        height: 28
        radius: 4
        color: symmetryPanel.visible ? "#E10600" : "#3e3e42"
        border.color: symmetryPanel.visible ? "#ff4444" : "#555"
        border.width: 1
        z: 10

        Text {
            anchors.centerIn: parent
            text: "<>"
            color: "#ffffff"
            font.pixelSize: 14
            font.bold: true
        }

        MouseArea {
            anchors.fill: parent
            onClicked: openSymmetryPanel()
            hoverEnabled: true
        }

        ToolTip.visible: hovered
        ToolTip.text: "Symmetry Editor (Shift+S)"
        ToolTip.delay: 800
    }

    // ── Material Editor button ──────────────────────────────────────────
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.rightMargin: 44
        width: 28
        height: 28
        radius: 4
        color: materialEditorOverlay.visible ? "#E10600" : "#3e3e42"
        border.color: materialEditorOverlay.visible ? "#ff4444" : "#555"
        border.width: 1
        z: 10

        Text {
            anchors.centerIn: parent
            text: "M"
            color: "#ffffff"
            font.pixelSize: 14
            font.bold: true
        }

        MouseArea {
            anchors.fill: parent
            onClicked: materialEditorOverlay.visible = !materialEditorOverlay.visible
            hoverEnabled: true
        }

        ToolTip.visible: hovered
        ToolTip.text: "Material Editor"
        ToolTip.delay: 800
    }

    // ── Scene Outliner button ────────────────────────────────────────────
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.rightMargin: 80
        width: 28; height: 28; radius: 4; z: 10
        color: outlinerOverlay.visible ? "#E10600" : "#3e3e42"
        border.color: outlinerOverlay.visible ? "#ff4444" : "#555"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "O"
            color: "#ffffff"
            font.pixelSize: 14; font.bold: true
        }

        MouseArea {
            anchors.fill: parent
            onClicked: outlinerOverlay.visible = !outlinerOverlay.visible
            hoverEnabled: true
        }

        ToolTip.visible: hovered
        ToolTip.text: "Scene Outliner"
        ToolTip.delay: 800
    }

    // ── Properties button ────────────────────────────────────────────────
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.rightMargin: 116
        width: 28; height: 28; radius: 4; z: 10
        color: propsOverlay.visible ? "#E10600" : "#3e3e42"
        border.color: propsOverlay.visible ? "#ff4444" : "#555"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "P"
            color: "#ffffff"
            font.pixelSize: 14; font.bold: true
        }

        MouseArea {
            anchors.fill: parent
            onClicked: propsOverlay.visible = !propsOverlay.visible
            hoverEnabled: true
        }

        ToolTip.visible: hovered
        ToolTip.text: "Properties"
        ToolTip.delay: 800
    }

    // ── Timeline button ──────────────────────────────────────────────────
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 8
        anchors.rightMargin: 152
        width: 28; height: 28; radius: 4; z: 10
        color: timelineOverlay.visible ? "#E10600" : "#3e3e42"
        border.color: timelineOverlay.visible ? "#ff4444" : "#555"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "T"
            color: "#ffffff"
            font.pixelSize: 14; font.bold: true
        }

        MouseArea {
            anchors.fill: parent
            onClicked: timelineOverlay.visible = !timelineOverlay.visible
            hoverEnabled: true
        }

        ToolTip.visible: hovered
        ToolTip.text: "Animation Timeline"
        ToolTip.delay: 800
    }

    // ── Material Editor Panel (floating overlay) ────────────────────────
    Rectangle {
        id: materialEditorOverlay
        visible: false
        z: 10
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        width: 320
        height: Math.min(parent.height * 0.9, 620)
        color: "#1e1e1e"
        border.color: "#333"
        border.width: 1
        radius: 4

        MaterialEditor {
            anchors.fill: parent
            onCloseRequested: materialEditorOverlay.visible = false
        }
    }

    // ── Scene Outliner Panel (floating overlay) ─────────────────────────
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

    // ── Properties Panel (floating overlay) ─────────────────────────────
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

    // ── Animation Timeline Panel (floating overlay) ─────────────────────
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

    // ── Symmetry Panel (floating overlay) ──────────────────────────────
    SymmetryPanel {
        id: symmetryPanel
        visible: false
        z: 10
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        onClosePanel: symmetryPanel.visible = false
    }

    // ── Texture Paint Panel (floating overlay) ─────────────────────────
    Rectangle {
        id: paintPanelOverlay
        visible: false
        z: 10
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        width: 280
        height: Math.min(parent.height * 0.85, 600)
        color: "transparent"

        Loader {
            id: paintPanelLoader
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

    // ── Tools Panel (right sidebar) ────────────────────────────────────
    Item {
        id: toolsPanelWrap
        visible: true
        z: 5
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 300

        Loader {
            id: toolsPanelLoader
            anchors.fill: parent
            source: "../modules/3DModeling/modeler_Tools.qml"
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

    Component.onCompleted: {
        sceneModel.countChanged.connect(updateStats)
        updateStats()
    }
}
