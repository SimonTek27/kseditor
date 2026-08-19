import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.Modeler 1.0

// Slate-style material node editor: draggable node graph with sockets,
// connections, a node palette, an inspector and GLSL codegen.
Rectangle {
    id: rootPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property var nodesModel: []
    property string selectedNodeId: ""
    property string statusText: "Material Node Editor"
    property real canvasZoom: 1.0
    property bool nodeDragging: false
    property bool pendingConnect: false
    property string pendingFromNode: ""
    property string pendingFromSocket: ""

    readonly property int nodeWidth: 200
    readonly property int headerH: 28
    readonly property int rowH: 26

    // ---- helpers -----------------------------------------------------------
    function displayName(s) {
        if (!s) return ""
        return s.replace(/([A-Z])/g, " $1").trim()
    }

    function nodeById(nid) {
        for (var i = 0; i < rootPanel.nodesModel.length; ++i)
            if (rootPanel.nodesModel[i].id === nid) return rootPanel.nodesModel[i]
        return null
    }

    function bodyHeight(n) {
        if (!n) return 80
        var rows = Math.max(n.inputs ? n.inputs.length : 0, n.outputs ? n.outputs.length : 0, 1)
        return rootPanel.headerH + rows * rootPanel.rowH
    }

    function socketPoint(nodeId, socketId) {
        var n = nodeById(nodeId)
        if (!n) return null
        for (var i = 0; i < n.inputs.length; ++i) {
            if (n.inputs[i].id === socketId)
                return { x: n.x, y: n.y + rootPanel.headerH + i * rootPanel.rowH + rootPanel.rowH * 0.5 }
        }
        for (var j = 0; j < n.outputs.length; ++j) {
            if (n.outputs[j].id === socketId)
                return { x: n.x + rootPanel.nodeWidth, y: n.y + rootPanel.headerH + j * rootPanel.rowH + rootPanel.rowH * 0.5 }
        }
        return null
    }

    function socketColor(type) {
        switch (type) {
        case 0: return "#9be08a"   // Float
        case 1: return "#6ac8ff"   // Float2
        case 2: return "#6ac8ff"   // Float3
        case 3: return "#ffcf6a"   // Color
        case 4: return "#ff9a9a"   // Image
        case 5: return "#b088ff"   // Shader
        case 6: return "#9be08a"   // Int
        default: return "#9be08a"
        }
    }

    function sourceOfSocket(socketId) {
        for (var i = 0; i < rootPanel.nodesModel.length; ++i) {
            var n = rootPanel.nodesModel[i]
            for (var j = 0; j < n.outputs.length; ++j)
                if (n.outputs[j].id === socketId) return n
        }
        return null
    }

    function refreshGraph() {
        var raw = Modeler.matNodeEditorGraph()
        var arr = []
        if (raw && raw.length > 0) {
            try { arr = JSON.parse(raw).nodes || [] } catch (e) { arr = [] }
        }
        rootPanel.nodesModel = arr
        connCanvas.requestPaint()
        gridCanvas.requestPaint()
    }

    function addNode(type) {
        var cx = canvasFlick.contentX / rootPanel.canvasZoom + canvasFlick.width / 2 / rootPanel.canvasZoom - 100
        var cy = canvasFlick.contentY / rootPanel.canvasZoom + canvasFlick.height / 2 / rootPanel.canvasZoom - 40
        var resp = Modeler.matNodeEditorCreateNode(type, Math.max(0, cx), Math.max(0, cy))
        if (resp && resp.length > 0) {
            try {
                var obj = JSON.parse(resp)
                rootPanel.selectedNodeId = obj.id
            } catch (e) { }
        }
        refreshGraph()
    }

    function deleteSelected() {
        if (!rootPanel.selectedNodeId) return
        Modeler.matNodeEditorDeleteNode(rootPanel.selectedNodeId)
        rootPanel.selectedNodeId = ""
        refreshGraph()
    }

    function setSocketValue(nodeId, socketId, value) {
        Modeler.matNodeEditorSetSocketValue(nodeId, socketId, value)
        refreshGraph()
    }

    // ---- state -------------------------------------------------------------
    Connections {
        target: Modeler
        function onMatNodeGraphChanged() { rootPanel.refreshGraph() }
    }

    Component.onCompleted: {
        refreshGraph()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6

        // ---- header + palette ------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: "#1b1d22"
            radius: 3
            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4
                Text {
                    text: "MATERIAL NODE EDITOR"
                    color: "#8fa3c0"
                    font.pixelSize: 11
                    font.bold: true
                    Layout.leftMargin: 6
                }
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: "#333"
                }
                Text {
                    text: "Add:"
                    color: "#778"
                    font.pixelSize: 11
                }
                Flickable {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    contentWidth: paletteFlow.implicitWidth
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    Flow {
                        id: paletteFlow
                        spacing: 3
                        Repeater {
                            model: {
                                var raw = Modeler.matNodeEditorAvailableTypes()
                                var list = []
                                if (raw && raw.length > 0) {
                                    try { list = JSON.parse(raw) } catch (e) { list = [] }
                                }
                                return list
                            }
                            delegate: AppButton {
                                text: rootPanel.displayName(modelData)
                                bgcolor: "#2a2f3a"
                                color: "#cfd6e4"
                                height: 24
                                implicitWidth: Math.max(78, text.length * 6.2 + 14)
                                font.pixelSize: 10
                                onClicked: rootPanel.addNode(modelData)
                            }
                        }
                    }
                }
                AppButton {
                    text: "Clear"
                    bgcolor: "#3a2226"
                    color: "#e0b0b0"
                    height: 24
                    onClicked: { Modeler.matNodeEditorClear(); rootPanel.selectedNodeId = "" }
                }
                AppButton {
                    text: "Shader"
                    bgcolor: "#1e4a2e"
                    color: "#bfe6cf"
                    height: 24
                    onClicked: {
                        rootPanel.statusText = "Generated shader (" +
                            Modeler.matNodeEditorGraph().length + " bytes JSON, see status)"
                        Modeler.matNodeEditorGenerateShader()
                        shaderPreview.open()
                    }
                }
                AppButton {
                    text: "Push→Mat"
                    bgcolor: "#2e2e5a"
                    color: "#bfbfe6"
                    height: 24
                    onClicked: {
                        var glsl = Modeler.matNodeEditorGenerateShader()
                        var vs = "#version 330 core\nlayout(location = 0) in vec3 inPos;\nlayout(location = 1) in vec3 inNorm;\nlayout(location = 2) in vec2 inUV;\nout vec2 vTexCoord;\nuniform mat4 modelViewProjection;\nvoid main() {\n    vTexCoord = inUV;\n    gl_Position = modelViewProjection * vec4(inPos, 1.0);\n}\n"
                        Modeler.createShader(vs, glsl)
                        rootPanel.statusText = "Pushed generated shader to current material"
                    }
                }
                AppButton {
                    text: "✕"
                    bgcolor: "#5a2222"
                    color: "#e6bfbf"
                    width: 28
                    height: 24
                    onClicked: rootPanel.closePanel()
                }
            }
        }

        // ---- canvas + inspector ----------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6

            Rectangle {
                id: canvasView
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#14161a"
                radius: 3
                clip: true

                Flickable {
                    id: canvasFlick
                    anchors.fill: parent
                    contentWidth: 2400
                    contentHeight: 1600
                    interactive: !rootPanel.nodeDragging
                    boundsBehavior: Flickable.DragAndOvershootBounds

                    Item {
                        id: contentItem
                        width: 2400
                        height: 1600
                        scale: rootPanel.canvasZoom
                        transformOrigin: Item.TopLeft

                        Canvas {
                            id: gridCanvas
                            anchors.fill: parent
                            z: 0
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                ctx.strokeStyle = "#1e222a"
                                ctx.lineWidth = 1
                                var spacing = 32
                                ctx.beginPath()
                                for (var x = 0; x <= width; x += spacing) {
                                    ctx.moveTo(x, 0); ctx.lineTo(x, height)
                                }
                                for (var y = 0; y <= height; y += spacing) {
                                    ctx.moveTo(0, y); ctx.lineTo(width, y)
                                }
                                ctx.stroke()
                            }
                        }

                        Canvas {
                            id: connCanvas
                            anchors.fill: parent
                            z: 1
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                for (var i = 0; i < rootPanel.nodesModel.length; ++i) {
                                    var n = rootPanel.nodesModel[i]
                                    for (var k = 0; k < n.inputs.length; ++k) {
                                        var inp = n.inputs[k]
                                        if (!inp.isConnected || !inp.connectedSocketId) continue
                                        var src = rootPanel.sourceOfSocket(inp.connectedSocketId)
                                        if (!src) continue
                                        var p1 = rootPanel.socketPoint(src.id, inp.connectedSocketId)
                                        var p2 = rootPanel.socketPoint(n.id, inp.id)
                                        if (!p1 || !p2) continue
                                        ctx.strokeStyle = rootPanel.socketColor(n.inputs[k].type)
                                        ctx.lineWidth = 2
                                        ctx.beginPath()
                                        ctx.moveTo(p1.x, p1.y)
                                        ctx.bezierCurveTo(p1.x + 60, p1.y, p2.x - 60, p2.y, p2.x, p2.y)
                                        ctx.stroke()
                                        // direction dot
                                        ctx.fillStyle = rootPanel.socketColor(n.inputs[k].type)
                                        ctx.beginPath()
                                        ctx.arc(p2.x, p2.y, 3, 0, Math.PI * 2)
                                        ctx.fill()
                                    }
                                }
                                if (rootPanel.pendingConnect) {
                                    var pp = rootPanel.socketPoint(rootPanel.pendingFromNode, rootPanel.pendingFromSocket)
                                    if (pp) {
                                        ctx.strokeStyle = "#ffffff"
                                        ctx.lineWidth = 1.5
                                        ctx.setLineDash([4, 4])
                                        ctx.beginPath()
                                        ctx.arc(pp.x, pp.y, 8, 0, Math.PI * 2)
                                        ctx.stroke()
                                        ctx.setLineDash([])
                                    }
                                }
                            }
                        }

                        Repeater {
                            id: nodeRepeater
                            model: rootPanel.nodesModel
                            delegate: nodeDelegate
                        }
                    }
                }

                // zoom controls
                WheelHandler {
                    id: wheel
                    acceptedModifiers: Qt.ControlModifier
                    onWheel: (event) => {
                        var f = event.angleDelta.y > 0 ? 1.15 : 0.87
                        rootPanel.canvasZoom = Math.max(0.25, Math.min(3.0, rootPanel.canvasZoom * f))
                    }
                }
                Column {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 8
                    spacing: 4
                    AppButton { text: "+"; width: 26; height: 22; bgcolor: "#2a2f3a"; color: "#cfd6e4"
                        onClicked: rootPanel.canvasZoom = Math.min(3.0, rootPanel.canvasZoom * 1.2) }
                    AppButton { text: "−"; width: 26; height: 22; bgcolor: "#2a2f3a"; color: "#cfd6e4"
                        onClicked: rootPanel.canvasZoom = Math.max(0.25, rootPanel.canvasZoom / 1.2) }
                }

                Shortcut {
                    sequence: "Delete"
                    onActivated: rootPanel.deleteSelected()
                }
            }

            // ---- inspector ------------------------------------------------------
            Rectangle {
                id: inspector
                Layout.preferredWidth: 232
                Layout.fillHeight: true
                color: "#1b1d22"
                radius: 3

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: selectedNodeId ? ("NODE: " + rootPanel.displayName(selectedNode.name)) : "INSPECTOR"
                            color: "#8fa3c0"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        AppButton {
                            text: "Del"
                            bgcolor: "#5a2222"
                            color: "#e6bfbf"
                            height: 22
                            visible: rootPanel.selectedNodeId.length > 0
                            onClicked: rootPanel.deleteSelected()
                        }
                    }

                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentHeight: inspCol.implicitHeight
                        Column {
                            id: inspCol
                            width: parent.width
                            spacing: 4

                            property var selectedNode: rootPanel.nodesModel && rootPanel.selectedNodeId.length > 0 ?
                                                       rootPanel.nodeById(rootPanel.selectedNodeId) : null

                            Rectangle {
                                visible: !inspCol.selectedNode
                                width: parent.width
                                height: 60
                                color: "#20232a"
                                radius: 3
                                Text {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    color: "#778"
                                    font.pixelSize: 10
                                    wrapMode: Text.WordWrap
                                    text: "Select a node to edit its inputs.\nDrag from an output socket to an input socket to connect.\nClick a connected input to disconnect."
                                }
                            }

                            // texture loader for image-ish nodes
                            Repeater {
                                model: inspCol.selectedNode &&
                                       (inspCol.selectedNode.factoryType === "ImageTexture" ||
                                        inspCol.selectedNode.factoryType === "NormalMap" ||
                                        inspCol.selectedNode.factoryType === "CubeMap" ||
                                        inspCol.selectedNode.factoryType === "Texture") ? [1] : []
                                delegate: Column {
                                    width: parent.width
                                    spacing: 3
                                    Text { text: "Texture Map"; color: "#9ab"; font.pixelSize: 10 }
                                    RowLayout {
                                        width: parent.width
                                        spacing: 3
                                        Text {
                                            Layout.fillWidth: true
                                            color: "#9aa"
                                            font.pixelSize: 10
                                            elide: Text.ElideMiddle
                                            text: inspCol.selectedNode && inspCol.selectedNode.texturePath ?
                                                  inspCol.selectedNode.texturePath : "(none)"
                                        }
                                        AppButton {
                                            text: "Load"
                                            bgcolor: "#2a2f3a"
                                            color: "#cfd6e4"
                                            height: 20
                                            onClicked: texDialog.open()
                                        }
                                    }
                                }
                            }

                            Repeater {
                                model: inspCol.selectedNode ? inspCol.selectedNode.inputs : []
                                delegate: Item {
                                    id: inputRow
                                    width: parent.width
                                    height: Math.max(22, editorItem.height)
                                    property var socketData: modelData
                                    property var socketNode: inspCol.selectedNode

                                    Column {
                                        id: editorItem
                                        width: parent.width
                                        spacing: 2
                                        Text {
                                            text: inputRow.socketData.name
                                            color: "#c8d2e0"
                                            font.pixelSize: 10
                                        }
                                        // Float
                                        RowLayout {
                                            visible: inputRow.socketData.type === 0 || inputRow.socketData.type === 6
                                            width: parent.width
                                            spacing: 4
                                            Slider {
                                                id: floatSlider
                                                Layout.fillWidth: true
                                                from: 0; to: 1; stepSize: 0.005
                                                value: { var v = Number(inputRow.socketData.value); return isNaN(v) ? 0 : Math.max(0, Math.min(1, v)) }
                                                onMoved: rootPanel.setSocketValue(inputRow.socketNode.id, inputRow.socketData.id, floatSlider.value)
                                            }
                                            TextInput {
                                                width: 44
                                                color: "#cfd6e4"
                                                font.pixelSize: 10
                                                text: { var v = Number(inputRow.socketData.value); return isNaN(v) ? "0" : v.toFixed(3) }
                                                onEditingFinished: {
                                                    var v = parseFloat(text)
                                                    if (!isNaN(v)) rootPanel.setSocketValue(inputRow.socketNode.id, inputRow.socketData.id, v)
                                                }
                                            }
                                        }
                                        // Color
                                        RowLayout {
                                            visible: inputRow.socketData.type === 3
                                            width: parent.width
                                            spacing: 4
                                            Rectangle {
                                                id: colorSwatch
                                                width: 24; height: 18
                                                radius: 2
                                                border.color: "#666"
                                                color: {
                                                    var v = inputRow.socketData.value
                                                    if (v && v.constructor === Object) {
                                                        var a = v.c
                                                        if (a) return Qt.rgba(a[0], a[1], a[2], a.length > 3 ? a[3] : 1)
                                                    }
                                                    return "#888"
                                                }
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: colorDialog.open()
                                                }
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                color: "#9aa"
                                                font.pixelSize: 9
                                                text: inputRow.socketData.isConnected ? "linked" : "click swatch"
                                            }
                                        }
                                        // Vector / Float3
                                        RowLayout {
                                            visible: inputRow.socketData.type === 1 || inputRow.socketData.type === 2
                                            width: parent.width
                                            spacing: 3
                                            property var v: {
                                                var val = inputRow.socketData.value
                                                if (val && val.constructor === Object && val.v) return val.v
                                                return [0, 0, 0]
                                            }
                                            Repeater {
                                                model: ["X", "Y", "Z"]
                                                delegate: RowLayout {
                                                    Layout.fillWidth: true
                                                    spacing: 2
                                                    Text { text: modelData; color: "#7a8698"; font.pixelSize: 9 }
                                                    TextInput {
                                                        Layout.fillWidth: true
                                                        color: "#cfd6e4"
                                                        font.pixelSize: 9
                                                        text: String(Math.round((parent.parent.v[model.index] || 0) * 100) / 100)
                                                        onEditingFinished: {
                                                            var arr = parent.parent.v.slice()
                                                            var val = parseFloat(text)
                                                            if (!isNaN(val)) arr[model.index] = val
                                                            rootPanel.setSocketValue(inputRow.socketNode.id, inputRow.socketData.id, arr)
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        Text {
                                            visible: inputRow.socketData.isConnected
                                            color: "#6b7"
                                            font.pixelSize: 9
                                            text: "connected"
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- status bar --------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 22
            color: "#1b1d22"
            radius: 3
            Text {
                anchors.fill: parent
                anchors.margins: 6
                color: "#8a97a8"
                font.pixelSize: 10
                elide: Text.ElideRight
                text: rootPanel.statusText + "  |  nodes: " + rootPanel.nodesModel.length
                      + "  |  " + (rootPanel.pendingConnect ? "click a target input socket to connect" : "ctrl+wheel to zoom, drag empty space to pan")
            }
        }
    }

    // ---- node delegate ---------------------------------------------------------
    Component {
        id: nodeDelegate
        Item {
            id: nodeItem
            property var nodeData: modelData
            property real baseX: 0
            property real baseY: 0
            property real dragDX: 0
            property real dragDY: 0
            width: rootPanel.nodeWidth
            height: rootPanel.bodyHeight(nodeData)

            x: baseX + dragDX
            y: baseY + dragDY

            onNodeDataChanged: {
                if (nodeData) { baseX = nodeData.x; baseY = nodeData.y; dragDX = 0; dragDY = 0 }
            }
            Component.onCompleted: {
                if (nodeData) { baseX = nodeData.x; baseY = nodeData.y }
            }

            // selection border + background
            Rectangle {
                anchors.fill: parent
                radius: 4
                color: "transparent"
                border.width: 1.5
                border.color: nodeItem.isSelectedItem ? "#4a9eff" : "#3a3f48"
                z: 2
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: 3
                    color: "#23262d"
                    Rectangle {
                        id: nodeHeader
                        width: parent.width
                        height: rootPanel.headerH
                        radius: 3
                        color: nodeItem.isSelectedItem ? "#2b3a52" : "#2a2f38"
                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - 16
                            color: "#dfe6f0"
                            font.pixelSize: 11
                            font.bold: true
                            elide: Text.ElideRight
                            text: rootPanel.displayName(nodeData.name)
                        }
                    }
                }
            }

            property bool isSelectedItem: nodeData.id === rootPanel.selectedNodeId

            // drag
            MouseArea {
                id: nodeDragArea
                anchors.fill: parent
                z: 3
                cursorShape: Qt.OpenHandCursor
                property real lastX: 0
                property real lastY: 0
                onPressed: (mouse) => {
                    nodeDragArea.lastX = mouse.x
                    nodeDragArea.lastY = mouse.y
                    rootPanel.selectedNodeId = nodeData.id
                    rootPanel.nodeDragging = true
                    canvasFlick.interactive = false
                }
                onPositionChanged: (mouse) => {
                    nodeItem.dragDX += mouse.x - nodeDragArea.lastX
                    nodeItem.dragDY += mouse.y - nodeDragArea.lastY
                    nodeDragArea.lastX = mouse.x
                    nodeDragArea.lastY = mouse.y
                    nodeData.x = Math.max(0, nodeItem.baseX + nodeItem.dragDX)
                    nodeData.y = Math.max(0, nodeItem.baseY + nodeItem.dragDY)
                    Modeler.matNodeEditorMoveNode(nodeData.id, nodeData.x, nodeData.y)
                    connCanvas.requestPaint()
                }
                onReleased: {
                    nodeItem.baseX = nodeData.x
                    nodeItem.baseY = nodeData.y
                    nodeItem.dragDX = 0
                    nodeItem.dragDY = 0
                    rootPanel.nodeDragging = false
                    canvasFlick.interactive = true
                    connCanvas.requestPaint()
                }
                onClicked: rootPanel.selectedNodeId = nodeData.id
            }

            // socket rows (inputs left, outputs right)
            Repeater {
                model: Math.max(nodeData.inputs.length, nodeData.outputs.length, 1)
                delegate: Item {
                    x: 0
                    y: rootPanel.headerH + model.index * rootPanel.rowH
                    width: rootPanel.nodeWidth
                    height: rootPanel.rowH
                    z: 4

                    // input socket
                    Repeater {
                        model: model.index < nodeData.inputs.length ? [nodeData.inputs[model.index]] : []
                        delegate: Item {
                            id: inputSocketArea
                            width: 16
                            height: 16
                            x: 0
                            y: (rootPanel.rowH - 16) / 2
                            property var socketData: modelData
                            Rectangle {
                                id: inputDot
                                width: 12; height: 12
                                radius: 6
                                x: -3
                                y: 2
                                color: rootPanel.socketColor(inputSocketArea.socketData.type)
                                border.color: "white"
                                border.width: rootPanel.pendingConnect &&
                                             inputSocketArea.socketData.id === rootPanel.pendingFromSocket ? 2 : 1
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (rootPanel.pendingConnect) {
                                        if (inputSocketArea.socketData.id === rootPanel.pendingFromSocket) {
                                            rootPanel.pendingConnect = false
                                            return
                                        }
                                        if (inputSocketArea.socketData.id !== rootPanel.pendingFromSocket) {
                                            // reconnect: drop previous link on this input if any
                                            if (inputSocketArea.socketData.isConnected && inputSocketArea.socketData.connectedSocketId) {
                                                var oldSrc = rootPanel.sourceOfSocket(inputSocketArea.socketData.connectedSocketId)
                                                if (oldSrc)
                                                    Modeler.matNodeEditorDisconnect(oldSrc.id,
                                                        inputSocketArea.socketData.connectedSocketId,
                                                        nodeData.id, inputSocketArea.socketData.id)
                                            }
                                            Modeler.matNodeEditorConnect(rootPanel.pendingFromNode,
                                                rootPanel.pendingFromSocket, nodeData.id, inputSocketArea.socketData.id)
                                        }
                                        rootPanel.pendingConnect = false
                                        rootPanel.refreshGraph()
                                    } else if (inputSocketArea.socketData.isConnected &&
                                               inputSocketArea.socketData.connectedSocketId) {
                                        var src = rootPanel.sourceOfSocket(inputSocketArea.socketData.connectedSocketId)
                                        if (src)
                                            Modeler.matNodeEditorDisconnect(src.id,
                                                inputSocketArea.socketData.connectedSocketId,
                                                nodeData.id, inputSocketArea.socketData.id)
                                        rootPanel.refreshGraph()
                                    }
                                }
                            }
                            Text {
                                anchors.left: parent.right
                                anchors.leftMargin: 4
                                anchors.verticalCenter: parent.verticalCenter
                                color: "#9fb0c5"
                                font.pixelSize: 9
                                text: inputSocketArea.socketData.name
                                width: 90
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // output socket
                    Repeater {
                        model: model.index < nodeData.outputs.length ? [nodeData.outputs[model.index]] : []
                        delegate: Item {
                            id: outputSocketArea
                            width: 16
                            height: 16
                            x: rootPanel.nodeWidth - 16
                            y: (rootPanel.rowH - 16) / 2
                            property var socketData: modelData
                            Rectangle {
                                id: outputDot
                                width: 12; height: 12
                                radius: 6
                                x: 7
                                y: 2
                                color: rootPanel.socketColor(outputSocketArea.socketData.type)
                                border.color: "white"
                                border.width: rootPanel.pendingConnect &&
                                             outputSocketArea.socketData.id === rootPanel.pendingFromSocket ? 2 : 1
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (rootPanel.pendingConnect &&
                                        outputSocketArea.socketData.id === rootPanel.pendingFromSocket) {
                                        rootPanel.pendingConnect = false
                                    } else {
                                        rootPanel.pendingConnect = true
                                        rootPanel.pendingFromNode = nodeData.id
                                        rootPanel.pendingFromSocket = outputSocketArea.socketData.id
                                    }
                                    connCanvas.requestPaint()
                                }
                            }
                            Text {
                                anchors.right: parent.left
                                anchors.rightMargin: 4
                                anchors.verticalCenter: parent.verticalCenter
                                color: "#9fb0c5"
                                font.pixelSize: 9
                                text: outputSocketArea.socketData.name
                                width: 70
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }
                }
            }
        }
    }

    // ---- dialogs --------------------------------------------------------------
    ColorDialog {
        id: colorDialog
        onAccepted: {
            var sel = rootPanel.nodeById(rootPanel.selectedNodeId)
            if (sel) {
                for (var i = 0; i < sel.inputs.length; ++i) {
                    var s = sel.inputs[i]
                    if (s.type === 3) {
                        rootPanel.setSocketValue(sel.id, s.id, color)
                        break
                    }
                }
            }
        }
    }

    FileDialog {
        id: texDialog
        title: "Load texture map"
        fileMode: FileDialog.OpenFile
        nameFilters: ["Images (*.png *.jpg *.jpeg *.tga *.bmp *.exr *.hdr)", "All Files (*)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            if (rootPanel.selectedNodeId)
                Modeler.matNodeEditorSetTexture(rootPanel.selectedNodeId, path)
        }
    }

    // shader preview popover
    Popup {
        id: shaderPreview
        anchors.centerIn: parent
        width: Math.min(640, parent.width * 0.7)
        height: Math.min(420, parent.height * 0.7)
        modal: true
        focus: true
        background: Rectangle { color: "#1b1d22"; border.color: "#3a3f48"; radius: 4 }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            RowLayout {
                Layout.fillWidth: true
                Text { text: "Generated GLSL"; color: "#8fa3c0"; font.bold: true; font.pixelSize: 11; Layout.fillWidth: true }
                AppButton { text: "✕"; width: 28; height: 22; bgcolor: "#5a2222"; color: "#e6bfbf"; onClicked: shaderPreview.close() }
            }
            Flickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                TextArea {
                    id: shaderText
                    width: parent.width
                    height: contentHeight
                    readOnly: true
                    font.family: "Consolas"
                    font.pixelSize: 10
                    color: "#cfe3cf"
                    background: Rectangle { color: "#101216" }
                }
            }
            AppButton {
                text: "Copy"
                bgcolor: "#1e4a2e"
                color: "#bfe6cf"
                height: 24
                onClicked: shaderText.selectAll() && shaderText.copy()
            }
        }
        onOpened: shaderText.text = Modeler.matNodeEditorGenerateShader()
    }
}
