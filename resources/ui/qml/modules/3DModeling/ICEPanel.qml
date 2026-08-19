import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

// Slate-style particle system editor
Rectangle {
    id: icePanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property var graph: ({nodes: [], connections: []})
    property bool playing: false
    property int aliveCount: 0
    property string selectedNodeId: ""
    property var selectedNode: ({id: "", title: "", type: "", properties: {}})
    property var propertyList: []
    property int cacheLength: 0
    property int scrubFrame: 0
    property real canvasScale: 1.0

    // Node type definitions with display names
    property var nodeTypes: [
        { type: "ICE.EmitterPoint", label: "Emitter (Point)", color: "#e10600" },
        { type: "ICE.EmitterSphere", label: "Emitter (Sphere)", color: "#e10600" },
        { type: "ICE.EmitterMesh", label: "Emitter (Mesh)", color: "#e10600" },
        { type: "ICE.EmitterCircle", label: "Emitter (Circle)", color: "#e10600" },
        { type: "ICE.ForceGravity", label: "Gravity", color: "#ff6600" },
        { type: "ICE.ForceWind", label: "Wind", color: "#ff6600" },
        { type: "ICE.ForceTurbulence", label: "Turbulence", color: "#ff6600" },
        { type: "ICE.ForceDrag", label: "Drag", color: "#ff6600" },
        { type: "ICE.ForceVortex", label: "Vortex", color: "#ff6600" },
        { type: "ICE.ForceAttractor", label: "Attractor", color: "#ff6600" },
        { type: "ICE.CollisionPlane", label: "Collision Plane", color: "#e1d500" },
        { type: "ICE.CollisionSphere", label: "Collision Sphere", color: "#e1d500" },
        { type: "ICE.CollisionMesh", label: "Collision Mesh", color: "#e1d500" },
        { type: "ICE.FilterAge", label: "Filter Age", color: "#4ecdc4" },
        { type: "ICE.FilterVelocity", label: "Filter Velocity", color: "#4ecdc4" },
        { type: "ICE.FilterRandom", label: "Filter Random", color: "#4ecdc4" },
        { type: "ICE.FilterPosition", label: "Filter Box", color: "#4ecdc4" },
        { type: "ICE.OpMultiply", label: "Multiply", color: "#a85590" },
        { type: "ICE.OpLerp", label: "Lerp", color: "#a85590" },
        { type: "ICE.OpVectorMath", label: "Vector Math", color: "#a85590" },
        { type: "ICE.OpCurve", label: "Curve Falloff", color: "#a85590" },
        { type: "ICE.PropColor", label: "Set Color", color: "#1e88e5" },
        { type: "ICE.PropSize", label: "Set Size", color: "#1e88e5" },
        { type: "ICE.PropLifetime", label: "Set Lifetime", color: "#1e88e5" },
        { type: "ICE.PropMass", label: "Set Mass", color: "#1e88e5" },
        { type: "ICE.OutputRibbons", label: "Output (Ribbons)", color: "#cddc39" },
        { type: "ICE.OutputMesh", label: "Output (Mesh)", color: "#cddc39" }
    ]

    function displayName(type) {
        for (var i = 0; i < nodeTypes.length; ++i)
            if (nodeTypes[i].type === type) return nodeTypes[i].label
        return type
    }

    function nodeColor(type) {
        for (var i = 0; i < nodeTypes.length; ++i)
            if (nodeTypes[i].type === type) return nodeTypes[i].color
        return "#fff"
    }

    // ---- Helpers -------------------------------------------------------

    function findNode(id) {
        if (!graph.hasOwnProperty("nodes")) return ({id: "", title: "", type: "", properties: {}})
        for (var i = 0; i < graph.nodes.length; ++i)
            if (graph.nodes[i].id === id) return graph.nodes[i]
        return ({id: "", title: "", type: "", properties: {}})
    }

    function bodyHeight(n) {
        if (!n) return 80
        var rows = Math.max(n.properties ? Object.keys(n.properties).length : 0, 1)
        return 28 + rows * 26
    }

    function refreshGraph() {
        graph = Modeler.iceGetGraph(objectId)
        aliveCount = Modeler.iceGetAliveCount(objectId)
        cacheLength = Modeler.iceCacheLength(objectId)
        if (selectedNodeId) {
            var n = findNode(selectedNodeId)
            if (n.id) selectedNode = n
        }
        rebuildProperties()
        graphCanvas.requestPaint()
    }

    function rebuildProperties() {
        propertyList = []
        var props = selectedNode.properties !== undefined ? selectedNode.properties : {}
        for (var key in props) {
            var val = props[key]
            propertyList.push({
                key: key,
                value: Array.isArray(val) ? val.slice() : val,
                isVec: Array.isArray(val),
                vecSize: val ? val.length : 0
            })
        }
    }

    function selectNode(id) {
        selectedNodeId = id
        selectedNode = findNode(id)
        rebuildProperties()
        graphCanvas.requestPaint()
    }

    function setSocketValue(nodeId, socketId, value) {
        Modeler.matNodeEditorSetSocketValue(nodeId, socketId, value)
        refreshGraph()
    }

    // ---- Node dragging / connection logic (live) ---------------------

    function tryConnect(fromNodeId, fromPort, toNodeId, toPort) {
        if (objectId < 0) return
        Modeler.iceConnect(objectId, fromNodeId, fromPort, toNodeId, toPort)
        refreshGraph()
    }

    // ---- Status / shader -------------------------------------------------

    function generateShader() {
        if (!selectedNode) return
        Modeler.matNodeEditorGenerateShader()
        refreshGraph()
    }

    // ---- UI Update on model change ------------------------------------

    Connections {
        target: Modeler
        function onIceChanged(id) {
            if (id === objectId) refreshGraph()
        }
        function onIceParticlesUpdated(id, count) {
            if (id === objectId) aliveCount = count
        }
    }

    // ---- UI ------------------------------------------------------------

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6

        // ---- Header / controls ------------------------------------------

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: "#1e1e1e"
            radius: 3
            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 4
                Text {
                    text: "PARTICLE SYSTEM EDITOR"
                    color: "#b0bec5"
                    font.pixelSize: 11
                    font.bold: true
                    Layout.leftMargin: 6
                }
                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: "#333"
                }
                AppButton {
                    text: "Close"
                    bgcolor: "#3e3e42"
                    color: "#e0e0e0"
                    height: 24
                    onClicked: closePanel()
                }
            }

            // ---- Toolbar --------------------------------------------------

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                spacing: 4

                AppButton {
                    text: "Create System"
                    bgcolor: "#e10600"
                    color: "#121212"
                    height: 26
                    font.pixelSize: 9
                    onClicked: { if (objectId >= 0) Modeler.iceCreate(objectId) }

                AppButton {
                    text: playing ? "Pause" : "Play"
                    bgcolor: playing ? "#ff6600" : "#3e3e42"
                    color: "#fff"
                    height: 26
                    font.pixelSize: 9
                    enabled: objectId >= 0 && graph.hasOwnProperty("nodes") && graph.nodes.length > 0
                    onClicked: { playing = !playing; if (objectId >= 0) Modeler.icePlayPause(objectId, playing) }

                AppButton {
                    text: "Remove Sys"
                    bgcolor: "#5a2222"
                    color: "#fff"
                    height: 26
                    font.pixelSize: 9
                    enabled: objectId >= 0 && graph.hasOwnProperty("nodes") && graph.nodes.length > 0
                    onClicked: { if (objectId >= 0) { Modeler.iceRemove(objectId); selectedNodeId = ""; refreshGraph() } }

                AppButton {
                    text: "Clear links"
                    bgcolor: "#5a2222"
                    color: "#fff"
                    height: 26
                    font.pixelSize: 9
                    enabled: objectId >= 0 && graph.hasOwnProperty("connections") && graph.connections.length > 0
                    onClicked: { if (objectId >= 0) Modeler.iceClearConnections(objectId) }

                AppButton {
                    text: "Bake"
                    bgcolor: "#e10600"
                    color: "#121212"
                    height: 26
                    font.pixelSize: 9
                    enabled: objectId >= 0 && graph.hasOwnProperty("nodes") && graph.nodes.length > 0
                    onClicked: {
                        var n = parseInt(bakeFrames.text)
                        if (!(n > 0)) n = 120
                        if (objectId >= 0) Modeler.iceBake(objectId, n)
                        cacheLength = Modeler.iceCacheLength(objectId)
                        scrubFrame = 0
                    }

                TextField {
                    id: bakeFrames
                    Layout.fillWidth: true
                    height: 24
                    text: "120"
                    color: "#fff"
                    font.pixelSize: 9
                    background: Rectangle { color: "#1a1a1c"; radius: 2 }
                    selectByMouse: true
                }

                AppButton {
                    text: "Clear Cache"
                    bgcolor: "#3e3e42"
                    color: "#fff"
                    height: 26
                    font.pixelSize: 9
                    enabled: cacheLength > 0
                    onClicked: { if (objectId >= 0) Modeler.iceClearCache(objectId); cacheLength = 0; scrubFrame = 0 }
                }
            }

            Text {
                text: cacheLength > 0 ? "Frames cached: " + cacheLength + " (drag to scrub)" : "Bake simulates a range offline, then you can scrub the result."
                color: "#aaa"
                font.pixelSize: 9
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Slider {
                id: scrubSlider
                Layout.fillWidth: true
                from: 0; to: Math.max(1, cacheLength); stepSize: 1
                value: scrubFrame
                enabled: cacheLength > 0
                onMoved: {
                    scrubFrame = Math.round(value)
                    if (objectId >= 0) Modeler.iceScrubToFrame(objectId, scrubFrame)
                }
                background: Rectangle { x: scrubSlider.leftPadding; y: scrubSlider.topPadding + scrubSlider.availableHeight / 2 - 2; width: scrubSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: scrubSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#e10600" }
                }
                handle: Rectangle { x: scrubSlider.leftPadding + scrubSlider.visualPosition * (scrubSlider.availableWidth - width); y: scrubSlider.topPadding + scrubSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#fff" }
            }

            Text {
                text: cacheLength > 0 ? "ICE simulates via a node graph: Emitter → Forces → Integrate → Output." : "No system loaded"
                color: "#666"
                font.pixelSize: 9
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

        // ---- Node palette -------------------------------------------------

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: "#25262a"
            radius: 3
            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 3
                Repeater {
                    model: nodeTypes
                    delegate: AppButton {
                        text: displayName(modelData.type)
                        bgcolor: nodeColor(modelData.type)
                        color: "#fff"
                        height: 22
                        font.pixelSize: 8
                        padding: 2
                        onClicked: { if (objectId >= 0) addNode(modelData.type) }
                    }
                }
            }

            // ---- Graph canvas ----------------------------------------------

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#1e1e1e"
                radius: 3

                Flickable {
                    id: flick
                    anchors.fill: parent
                    contentWidth: 2000
                    contentHeight: 1400
                    interactive: !nodeDragging
                    onScroll: canvasScale = Math.max(0.5, Math.min(2.0, canvasScale * (event.pixelDelta.y < 0 ? 1.1 : 0.9)))

                    Item {
                        id: content
                        width: 2000
                        height: 1400
                        scale: canvasScale
                        transformOrigin: Item.TopLeft

                        // Grid background
                        Canvas {
                            anchors.fill: parent
                            z: 0
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                ctx.strokeStyle = "#2a2a30"
                                ctx.lineWidth = 1
                                var spacing = 40
                                for (var x = 0; x <= width; x += spacing) {
                                    ctx.moveTo(x, 0); ctx.lineTo(x, height)
                                }
                                for (var y = 0; y <= height; y += spacing) {
                                    ctx.moveTo(0, y); ctx.lineTo(width, y)
                                }
                                ctx.stroke()
                            }
                        }

                        // Connections Canvas (live)
                        Canvas {
                            id: connCanvas
                            anchors.fill: parent
                            z: 1
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                for (var i = 0; i < graph.connections.length; ++i) {
                                    var c = graph.connections[i]
                                    var from = nodeById(c.fromNode)
                                    var to = nodeById(c.toNode)
                                    if (!from || !to) continue
                                    ctx.strokeStyle = "#64b5f6"
                                    ctx.lineWidth = 2
                                    ctx.beginPath()
                                    ctx.moveTo(from.x + 20, from.y + 13)
                                    ctx.bezierCurveTo(from.x + 80, from.y, to.x - 80, to.y, to.x + 20, to.y + 13)
                                    ctx.stroke()
                                    // Arrow head
                                    ctx.fillStyle = "#64b5f6"
                                    ctx.beginPath()
                                    ctx.moveTo(to.x + 20, to.y + 13)
                                    ctx.lineTo(to.x + 5, to.y - 8)
                                    ctx.lineTo(to.x + 5, to.y + 8)
                                    ctx.closePath()
                                    ctx.fill()
                                }
                            }
                        }

                        // Node delegate
                        Repeater {
                            id: nodeRepeater
                            model: graph.nodes
                            delegate: nodeDelegate
                        }

                        // Live connection preview from drag
                        Canvas {
                            id: previewCanvas
                            anchors.fill: parent
                            z: 2
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                if (dragFromNodeId && dragMouse) {
                                    var from = nodeById(dragFromNodeId)
                                    if (from) {
                                        ctx.strokeStyle = "#ffcc00"
                                        ctx.lineWidth = 2
                                        ctx.setLineDash([4, 4])
                                        ctx.beginPath()
                                        ctx.moveTo(from.x + 20, from.y + 13)
                                        ctx.lineTo(dragMouse.x, dragMouse.y)
                                        ctx.stroke()
                                        ctx.setLineDash([])
                                    }
                                }
                            }
                        }
                    }

                    // Node delegate Component
                    Component {
                        id: nodeDelegate
                        Item {
                            id: nodeItem
                            property var nodeData: modelData
                            property real baseX: 0
                            property real baseY: 0
                            property real dragDX: 0
                            property real dragDY: 0
                            width: 160
                            height: bodyHeight(nodeData)

                            x: baseX + dragDX
                            y: baseY + dragDY

                            onNodeDataChanged: {
                                if (nodeData) { baseX = nodeData.x; baseY = nodeData.y; dragDX = 0; dragDY = 0 }
                            }
                            Component.onCompleted: {
                                if (nodeData) { baseX = nodeData.x; baseY = nodeData.y }
                            }

                            // Selection + background
                            Rectangle {
                                anchors.fill: parent
                                radius: 4
                                color: "transparent"
                                border.width: 1.5
                                border.color: nodeItem.isSelectedItem ? "#64b5f6" : "#3f3f44"
                                Rectangle {
                                    id: nodeHeader
                                    width: parent.width
                                    height: 26
                                    radius: 3
                                    color: nodeItem.isSelectedItem ? "#1e2a3a" : "#1e1e1f"
                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 6
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width - 12
                                        color: "#b0bec5"
                                        font.pixelSize: 9
                                        font.bold: true
                                        elide: Text.ElideRight
                                        text: nodeItem.displayName
                                    }
                                }
                            }

                            property bool isSelectedItem: nodeData.id === selectedNodeId

                            // Drag
                            MouseArea {
                                id: nodeDragArea
                                anchors.fill: parent
                                cursorShape: Qt.OpenHandCursor
                                property real lastX: 0
                                property real lastY: 0
                                property real dragStartX: 0
                                property real dragStartY: 0
                                onPressed: (mouse) => {
                                    nodeDragArea.lastX = mouse.x
                                    nodeDragArea.lastY = mouse.y
                                    nodeDragArea.dragStartX = mouse.x
                                    nodeDragArea.dragStartY = mouse.y
                                    selectedNodeId = nodeData.id
                                    nodeDragging = true
                                    flick.interactive = false
                                }
                                onPositionChanged: (mouse) => {
                                    nodeItem.dragDX += mouse.x - nodeDragArea.lastX
                                    nodeItem.dragDY += mouse.y - nodeDragArea.lastY
                                    nodeDragArea.lastX = mouse.x
                                    nodeDragArea.lastY = mouse.y
                                    nodeData.x = Math.max(0, nodeItem.baseX + nodeItem.dragDX)
                                    nodeData.y = Math.max(0, nodeItem.baseY + nodeItem.dragDY)
                                    Modeler.iceSetNodePosition(objectId, nodeData.id, nodeData.x, nodeData.y)
                                    connCanvas.requestPaint()
                                }
                                onReleased: {
                                    nodeItem.baseX = nodeData.x
                                    nodeItem.baseY = nodeData.y
                                    nodeItem.dragDX = 0
                                    nodeItem.dragDY = 0
                                    nodeDragging = false
                                    flick.interactive = true
                                    connCanvas.requestPaint()
                                }
                                onClicked: { selectedNodeId = nodeData.id }
                            }

                            // Socket rows (inputs left, outputs right)
                            Repeater {
                                model: Math.max(nodeData.properties ? Object.keys(nodeData.properties).length : 1)
                                delegate: Item {
                                    x: 0
                                    y: 26 + model.index * 24
                                    width: 160
                                    height: 24
                                    // Input socket (left)
                                    Rectangle {
                                        id: inSocket
                                        x: -6
                                        y: (24 - 10) / 2
                                        width: 10
                                        height: 10
                                        color: "#64b5f6"
                                        radius: 5
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: { tryConnect(nodeData.id, "out_" + model.index, nodeData.id, "in_" + model.index) }
                                        }
                                    }
                                    // Output socket (right)
                                    Rectangle {
                                        id: outSocket
                                        x: 150
                                        y: (24 - 10) / 2
                                        width: 10
                                        height: 10
                                        color: "#64b5f6"
                                        radius: 5
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                if (dragFromNodeId === nodeData.id) {
                                                    tryConnect(dragFromNodeId, "out_" + model.index, nodeData.id, "in_" + model.index)
                                                    dragFromNodeId = ""
                                                } else {
                                                    dragFromNodeId = nodeData.id
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ---- Inspector --------------------------------------------------

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredWidth: 240
                Layout.fillHeight: true
                color: "#25262a"
                radius: 3

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: selectedNodeId ? "NODE: " + (selectedNode.title || selectedNode.type) : "INSPECTOR"
                            color: "#b0bec5"
                            font.pixelSize: 10
                            font.bold: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        AppButton {
                            text: "Del"
                            bgcolor: "#3e3e42"
                            color: "#e0e0e0"
                            height: 22
                            visible: selectedNodeId.length > 0
                            onClicked: { selectedNodeId = "" }
                        }
                    }

                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentHeight: propList implicitHeight
                        Column {
                            id: propListCol
                            width: parent.width
                            spacing: 2

                            property var selNode: selectedNode
                            onSelectedNodeChanged: { }

                            Repeater {
                                model: propListCol.selectedNode ? propListCol.selectedNode.properties ? Object.keys(propListCol.selectedNode.properties) : [] : []
                                delegate: Item {
                                    id: propItem
                                    Layout.fillWidth: true
                                    height: propData.isVec ? 56 : 32
                                    color: "#222226"
                                    radius: 3
                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        spacing: 2

                                        Text {
                                            text: propData.key
                                            color: "#aaa"
                                            font.pixelSize: 8
                                        }
                                        if (propData.isVec) {
                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 2
                                                for (var i = 0; i < propData.vecSize; ++i) {
                                                    var vi = propData.vec[i]
                                                    Text { text: "X"+i; color: "#666"; font.pixelSize: 7; width: 12 }
                                                    TextField {
                                                        Layout.fillWidth: true
                                                        text: vi !== undefined ? vi.toFixed(3) : "0"
                                                        color: "#fff"
                                                        font.pixelSize: 8
                                                        selectByMouse: true
                                                        background: Rectangle { color: "#1a1a1c"; radius: 2 }
                                                        onEditingFinished: {
                                                            var v = propData.value.slice()
                                                            v[i] = parseFloat(text)
                                                            setProp(propData.key, v)
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            TextField {
                                                Layout.fillWidth: true
                                                text: propData.value.toFixed(4)
                                                color: "#fff"
                                                font.pixelSize: 8
                                                selectByMouse: true
                                                background: Rectangle { color: "#1a1a1c"; radius: 2 }
                                                onEditingFinished: setProp(propData.key, parseFloat(text))
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: selectedNodeId !== "" ? "PARTICLE COUNT: " + aliveCount : ""
                            color: "#777"
                            font.pixelSize: 8
                        }
                        AppButton {
                            text: "Del"
                            bgcolor: "#3e3e42"
                            color: "#e0e0e0"
                            height: 22
                            visible: selectedNodeId.length > 0
                            onClicked: { selectedNodeId = "" }
                        }
                    }
                }
            }
        }
    }