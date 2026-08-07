import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Dialogs
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

    property string currentFile: "untitled.ks3d"
    property real baseUiScale: 1.18
    property real uiZoom: 1.0
    property real uiScale: baseUiScale * uiZoom
    property int viewMode: 0
    property string selectedObject: ""
    property int activeViewport: 3
    property bool quadView: true

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

    property var selectedObj: Modeler ? Modeler.selectedObject : null

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
                { label: "Grid", icon: "\u2317", cmd: "grid" },
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
                { label: "Weld", icon: "\u2695", cmd: "weld" },
                { label: "Symmetry", icon: "\u2194", cmd: "symmetry" }
            ]}
        ]},
        { title: "Tools", groups: [
            { name: "Tools", buttons: [
                { label: "Materials", icon: "M", cmd: "material" },
                { label: "Paint", icon: "\u270E", cmd: "paint" }
            ]}
        ]},
        { title: "Panels", groups: [
            { name: "Panels", buttons: [
                { label: "Outliner", icon: "\u2630", cmd: "outliner" },
                { label: "Props", icon: "\u2699", cmd: "props" },
                { label: "Timeline", icon: "\u23F1", cmd: "timeline" }
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
        else if (c === "printcheck") { if (Modeler.checkMesh) Modeler.checkMesh() }
        else if (c === "exportstl") { if (Modeler.exportSTL) Modeler.exportSTL() }
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
        else if (c === "boolop") { boolOpOverlay.visible = !boolOpOverlay.visible }
        else if (c === "material") { materialEditorOverlay.visible = !materialEditorOverlay.visible }
        else if (c === "paint") { openPaintPanel() }
        else if (c === "outliner") { outlinerDock.visible = !outlinerDock.visible }
        else if (c === "props") { propsOverlay.visible = !propsOverlay.visible }
        else if (c === "timeline") { timelineOverlay.visible = !timelineOverlay.visible }
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
                            { icon: "\u2317", cmd: "grid", tip: "Grid (G)", active: Modeler.gridVisible },
                            { icon: "\u2194", cmd: "symmetry", tip: "Symmetry" },
                            { icon: "M", cmd: "material", tip: "Materials" },
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

                            Rectangle {
                                Layout.preferredWidth: 66; Layout.preferredHeight: 20
                                radius: 2
                                color: quadTgl.containsMouse ? "#2d2d30" : "transparent"
                                border.color: "#3f3f46"
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: quadView ? "◫ Quad" : "▣ Single"
                                    color: quadView ? "#569cd6" : "#aaa"
                                    font.pixelSize: 9; font.bold: true
                                }
                                MouseArea { id: quadTgl; anchors.fill: parent; hoverEnabled: true
                                    onClicked: quadView = !quadView
                                }
                                ToolTip { visible: quadTgl.containsMouse; text: quadView ? "Switch to single view" : "Switch to quad view" }
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
                            DirectionalLight { eulerRotation: Qt.vector3d(-45, 45, 0); brightness: 1.2 }
                            DirectionalLight { brightness: 0.35; eulerRotation: Qt.vector3d(0, 0, 0) }

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
                                    property int objId: model.objectId
                                    geometry: SceneMeshGeometry {
                                        objectId: objId
                                    }
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
                        visible: quadView || activeViewport === 3
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
                            DirectionalLight { eulerRotation: Qt.vector3d(-45, 45, 0); brightness: 1.2 }
                            DirectionalLight { brightness: 0.35; eulerRotation: Qt.vector3d(0, 0, 0) }

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
                                    property int objId: model.objectId
                                    objectName: "obj" + model.objectId
                                    pickable: true
                                    geometry: SceneMeshGeometry {
                                        objectId: objId
                                    }
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
                                Model { objectName: "gizmoX"; pickable: true; source: "#Cylinder"; position: Qt.vector3d(0.5, 0, 0); scale: dragAxis === 0 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02); eulerRotation: Qt.vector3d(0, 0, -90); materials: [DefaultMaterial { diffuseColor: dragAxis === 0 ? "#ff8888" : "#ff4444" }] }
                                Model { objectName: "gizmoX"; pickable: true; source: "#Cone"; position: Qt.vector3d(1.0, 0, 0); scale: Qt.vector3d(0.06, 0.2, 0.06); eulerRotation: Qt.vector3d(0, 0, -90); materials: [DefaultMaterial { diffuseColor: dragAxis === 0 ? "#ff8888" : "#ff4444" }] }
                                Model { objectName: "gizmoY"; pickable: true; source: "#Cylinder"; position: Qt.vector3d(0, 0.5, 0); scale: dragAxis === 1 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02); materials: [DefaultMaterial { diffuseColor: dragAxis === 1 ? "#88ff88" : "#44ff44" }] }
                                Model { objectName: "gizmoY"; pickable: true; source: "#Cone"; position: Qt.vector3d(0, 1.0, 0); scale: Qt.vector3d(0.06, 0.2, 0.06); materials: [DefaultMaterial { diffuseColor: dragAxis === 1 ? "#88ff88" : "#44ff44" }] }
                                Model { objectName: "gizmoZ"; pickable: true; source: "#Cylinder"; position: Qt.vector3d(0, 0, 0.5); scale: dragAxis === 2 ? Qt.vector3d(0.03, 1.2, 0.03) : Qt.vector3d(0.02, 1, 0.02); eulerRotation: Qt.vector3d(90, 0, 0); materials: [DefaultMaterial { diffuseColor: dragAxis === 2 ? "#8888ff" : "#4444ff" }] }
                                Model { objectName: "gizmoZ"; pickable: true; source: "#Cone"; position: Qt.vector3d(0, 0, 1.0); scale: Qt.vector3d(0.06, 0.2, 0.06); eulerRotation: Qt.vector3d(90, 0, 0); materials: [DefaultMaterial { diffuseColor: dragAxis === 2 ? "#8888ff" : "#4444ff" }] }
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
                                    else if (name.indexOf("obj") === 0) {
                                        var id = parseInt(name.substring(3))
                                        if (Modeler.selectObject) Modeler.selectObject(id)
                                    }
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
                        visible: quadView || activeViewport === 1
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
                            DirectionalLight { eulerRotation: Qt.vector3d(-45, 45, 0); brightness: 1.2 }
                            DirectionalLight { brightness: 0.35; eulerRotation: Qt.vector3d(0, 0, 0) }

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
                                    property int objId: model.objectId
                                    geometry: SceneMeshGeometry {
                                        objectId: objId
                                    }
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
                        visible: quadView || activeViewport === 2
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
                            DirectionalLight { eulerRotation: Qt.vector3d(-45, 45, 0); brightness: 1.2 }
                            DirectionalLight { brightness: 0.35; eulerRotation: Qt.vector3d(0, 0, 0) }

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
                                    property int objId: model.objectId
                                    geometry: SceneMeshGeometry {
                                        objectId: objId
                                    }
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
                visible: true
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
                        { label: "Grid", prop: Modeler.gridVisible, set: function(v){ if (Modeler.gridVisible !== undefined) Modeler.gridVisible = v } },
                        { label: "Wireframe", prop: Modeler.viewMode === 1, set: function(v){ if (Modeler.viewMode !== undefined) Modeler.viewMode = v ? 1 : 0 } },
                        { label: "Snap", prop: pageKsModeler.snapGrid, set: function(v){ pageKsModeler.snapGrid = v } }
                    ]
                    delegate: Rectangle {
                        Layout.preferredWidth: 44; Layout.preferredHeight: 18
                        radius: 3
                        color: modelData.prop ? "#264f78" : (snapHover.containsMouse ? "#3e3e42" : "transparent")
                        border.color: modelData.prop ? "#569cd6" : "#3f3f46"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: modelData.prop ? "#569cd6" : "#888"; font.pixelSize: 9
                        }
                        MouseArea { id: snapHover; anchors.fill: parent; hoverEnabled: true
                            onClicked: { if (modelData.set) modelData.set(!modelData.prop) }
                        }
                    }
                }

                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "Objects: " + (sceneModel ? sceneModel.count : 0); color: "#888"; font.pixelSize: 10 }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "View: " + viewportNames[activeViewport]; color: "#888"; font.pixelSize: 10 }
                Rectangle { width: 1; height: 16; color: "#2d2d30" }
                Text { text: "Tool: " + (Modeler.gizmoMode === 0 ? "Select" : Modeler.gizmoMode === 1 ? "Move" : Modeler.gizmoMode === 2 ? "Rotate" : "Scale"); color: "#888"; font.pixelSize: 10 }
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

    Rectangle { id: boolOpOverlay; visible: false; z: 10; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; width: 260; height: Math.min(parent.height * 0.85, 400); color: "#1e1e1e"; border.color: "#333"; border.width: 1; radius: 4
        Loader {
            anchors.fill: parent
            source: "../modules/3DModeling/BoolOpPanel.qml"
            onItemChanged: if (item) item.closePanel.connect(function() { boolOpOverlay.visible = false })
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
        nameFilters: ["Supported Files (*.kn5 *.fbx *.glb *.gltf *.obj)", "KN5 (*.kn5)", "FBX (*.fbx)", "GLB/GLTF (*.glb *.gltf)", "OBJ (*.obj)"]
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
        nameFilters: ["KN5 (*.kn5)", "FBX (*.fbx)", "OBJ (*.obj)", "GLB (*.glb)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            var ok = Modeler.exportFile ? Modeler.exportFile(path) : false
            commandEcho = ok ? "Exported: " + path : "Export failed"
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
