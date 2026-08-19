import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: fcurvePanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property string activeChannel: "position.x"
    property var selectedKeys: []
    property bool playing: false
    property bool fitted: false

    onWidthChanged: {
        if (width > 10 && !fitted) { fitted = true; autoFit(); requestRepaint() }
    }

    // View state (graph). Shared frame scale/pan with dope sheet.
    property real frameScale: 20
    property real valueScale: 40
    property real panX: 40
    property real panY: 150

    property var channels: ["position.x", "position.y", "position.z",
                            "rotation.x", "rotation.y", "rotation.z",
                            "scale.x", "scale.y", "scale.z"]

    // Interaction state
    property bool panMode: false
    property real panStartX: 0
    property real panStartY: 0
    property real panOriginX: 0
    property real panOriginY: 0
    property bool draggingKey: false
    property int dragIndex: -1
    property real dragF0: 0
    property real dragV0: 0

    // ============================================================
    // Helpers
    // ============================================================
    function keysFor(ch) {
        if (objectId < 0) return []
        var k = Modeler.fcurveKeys(objectId, ch)
        return k || []
    }
    function activeKeys() { return keysFor(activeChannel) }

    function evalAt(frame) {
        if (objectId < 0) return 0
        return Modeler.fcurveEvaluate(objectId, activeChannel, frame)
    }

    function frameToX(f) { return panX + f * frameScale }
    function xToFrame(x) { return (x - panX) / frameScale }
    function valueToY(v) { return panY - v * valueScale }
    function yToValue(y) { return (panY - y) / valueScale }
    function dopeFrameToX(f) { return 60 + panX + f * frameScale }

    function valueForChannel(ch) {
        var obj = Modeler ? Modeler.selectedObject : null
        if (!obj) return 0
        var axis = ch.charAt(ch.length - 1)
        if (ch.indexOf("position.") === 0) {
            var p = obj.position
            return axis === "x" ? p.x : axis === "y" ? p.y : p.z
        }
        if (ch.indexOf("rotation.") === 0) {
            var r = obj.rotation
            return axis === "x" ? r.x : axis === "y" ? r.y : r.z
        }
        if (ch.indexOf("scale.") === 0) {
            var s = obj.scale
            return axis === "x" ? s.x : axis === "y" ? s.y : s.z
        }
        return 0
    }

    function requestRepaint() {
        if (graphCanvas) graphCanvas.requestPaint()
        if (dopeCanvas) dopeCanvas.requestPaint()
    }

    function autoFit() {
        if (!graphCanvas) return
        var keys = activeKeys()
        var minF = 0, maxF = Math.max(1, Modeler ? Modeler.animationDuration : 5)
        var minV = -1, maxV = 1
        if (keys.length > 0) {
            minF = keys[0].frame; maxF = keys[0].frame
            minV = keys[0].value; maxV = keys[0].value
            for (var i = 1; i < keys.length; ++i) {
                minF = Math.min(minF, keys[i].frame)
                maxF = Math.max(maxF, keys[i].frame)
                minV = Math.min(minV, keys[i].value)
                maxV = Math.max(maxV, keys[i].value)
            }
        }
        maxF = Math.max(maxF, Math.min(minF + 1, maxF) )
        var fPad = (maxF - minF) * 0.15
        var vPad = (maxV - minV) * 0.15
        if (fPad < 0.5) fPad = 0.5
        if (vPad < 0.1) vPad = 0.1
        minF -= fPad; maxF += fPad
        minV -= vPad; maxV += vPad

        var aw = graphCanvas.width
        var ah = graphCanvas.height
        if (aw < 1 || ah < 1) return
        frameScale = Math.max(0.5, (aw * 0.9) / Math.max(1e-3, (maxF - minF)))
        panX = aw * 0.05 - minF * frameScale
        valueScale = Math.max(0.05, (ah * 0.9) / Math.max(1e-3, (maxV - minV)))
        panY = ah * 0.95 + minV * valueScale
    }

    function hitTest(x, y) {
        var keys = activeKeys()
        for (var i = 0; i < keys.length; ++i) {
            var kx = frameToX(keys[i].frame)
            var ky = valueToY(keys[i].value)
            var dx = kx - x, dy = ky - y
            if (dx * dx + dy * dy <= 36) return { index: i, frame: keys[i].frame, value: keys[i].value }
        }
        return null
    }

    function selectKey(channel, index, frame, value, additive) {
        var sel = { channel: channel, index: index, frame: frame, value: value }
        if (additive) {
            for (var i = 0; i < selectedKeys.length; ++i) {
                if (selectedKeys[i].channel === channel && selectedKeys[i].frame === frame) {
                    selectedKeys.splice(i, 1)
                    requestRepaint()
                    return
                }
            }
            selectedKeys.push(sel)
        } else {
            selectedKeys = [sel]
        }
        requestRepaint()
    }

    function isSelected(channel, frame) {
        for (var i = 0; i < selectedKeys.length; ++i)
            if (selectedKeys[i].channel === channel && Math.abs(selectedKeys[i].frame - frame) < 0.01)
                return true
        return false
    }

    function refresh() {
        objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
        if (activeKeys().length === 0 && activeChannel === "")
            activeChannel = "position.x"
        autoFit()
        requestRepaint()
    }

    function addKeyAt(frame, value, ch) {
        if (objectId < 0) return
        var target = ch || activeChannel
        Modeler.fcurveSetKey(objectId, target, frame, value, "Cubic")
        var keys = keysFor(target)
        for (var i = 0; i < keys.length; ++i)
            if (Math.abs(keys[i].frame - frame) < 0.01) { selectKey(target, i, keys[i].frame, keys[i].value, false); return }
    }

    function deleteSelectedKeys() {
        if (objectId < 0) return
        for (var i = 0; i < selectedKeys.length; ++i)
            Modeler.fcurveRemoveKey(objectId, selectedKeys[i].channel, selectedKeys[i].frame)
        selectedKeys = []
        requestRepaint()
    }

    function applyInterp(name) {
        if (objectId < 0) return
        for (var i = 0; i < selectedKeys.length; ++i)
            Modeler.fcurveSetInterpolation(objectId, selectedKeys[i].channel, selectedKeys[i].index, name)
    }

    function clearActiveChannel() {
        if (objectId < 0) return
        var keys = activeKeys()
        for (var i = 0; i < keys.length; ++i)
            Modeler.fcurveRemoveKey(objectId, activeChannel, keys[i].frame)
        selectedKeys = []
        requestRepaint()
    }

    function applyPose() {
        if (objectId >= 0) Modeler.fcurveApplyToObject(objectId, Modeler.animationTime)
    }

    // ============================================================
    // Connections
    // ============================================================
    Connections {
        target: Modeler
        function onFcurveChanged(id) {
            if (id === fcurvePanel.objectId) {
                for (var i = 0; i < fcurvePanel.selectedKeys.length; ++i) {
                    var ch = fcurvePanel.selectedKeys[i].channel
                    var keys = fcurvePanel.keysFor(ch)
                    for (var j = 0; j < keys.length; ++j)
                        if (Math.abs(keys[j].frame - fcurvePanel.selectedKeys[i].frame) < 0.01) {
                            fcurvePanel.selectedKeys[i].index = j
                            break
                        }
                }
                fcurvePanel.requestRepaint()
            }
        }
        function onSelectionChanged() { fcurvePanel.refresh() }
        function onAnimationTimeChanged() {
            fcurvePanel.requestRepaint()
            if (liveToggle.checked && fcurvePanel.objectId >= 0)
                Modeler.fcurveApplyToObject(fcurvePanel.objectId, Modeler.animationTime)
        }
        function onPlaybackStateChanged() { fcurvePanel.playing = Modeler.fcurveIsPlaying() }
    }

    Component.onCompleted: refresh()

    // ============================================================
    // UI
    // ============================================================
    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Rectangle {
            Layout.fillWidth: true
            height: 32
            color: "#252526"

            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "F-CURVE EDITOR"
                color: "#E10600"
                font.pixelSize: 11
                font.bold: true
            }

            Text {
                anchors { left: parent.left; leftMargin: 140; verticalCenter: parent.verticalCenter }
                text: "Dope Sheet + F-Curve"
                color: "#888"
                font.pixelSize: 9
            }

            Rectangle {
                anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                width: 18; height: 18; radius: 2; color: "#E10600"
                Text { anchors.centerIn: parent; text: "X"; color: "#fff"; font.pixelSize: 10; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: closePanel() }
            }
        }

        // Channel chips
        Flow {
            id: channelFlow
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            spacing: 4

            Repeater {
                model: fcurvePanel.channels
                delegate: Rectangle {
                    width: channelText.width + 12
                    height: 20
                    radius: 3
                    color: activeChannel === modelData ? "#E10600" : "#2c2c2e"
                    border.color: activeChannel === modelData ? "#E10600" : "#333"
                    border.width: 1

                    Text {
                        id: channelText
                        anchors.centerIn: parent
                        text: modelData
                        color: activeChannel === modelData ? "#121212" : "#bbb"
                        font.pixelSize: 9
                        font.bold: activeChannel === modelData
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: { activeChannel = modelData; autoFit(); requestRepaint() }
                    }
                }
            }
        }

        // Toolbar
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            height: 28
            color: "#1c1c1e"
            radius: 3

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 3

                Text {
                    text: "Interp:"
                    color: "#888"
                    font.pixelSize: 9
                    font.bold: true
                }
                AppButton { text: "Lin";  height: 20; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 8; onClicked: applyInterp("Linear") }
                AppButton { text: "Step"; height: 20; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 8; onClicked: applyInterp("Step") }
                AppButton { text: "Cub";  height: 20; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 8; onClicked: applyInterp("Cubic") }
                AppButton { text: "EIn";  height: 20; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 8; onClicked: applyInterp("EaseIn") }
                AppButton { text: "EOut"; height: 20; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 8; onClicked: applyInterp("EaseOut") }
                AppButton { text: "EIO";  height: 20; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 8; onClicked: applyInterp("EaseInOut") }

                Item { Layout.fillWidth: true }

                AppButton { text: "Add";   height: 20; bgcolor: "#E10600"; color: "#121212"; font.bold: true; font.pixelSize: 9; onClicked: addKeyAt(Modeler.animationTime, valueForChannel(activeChannel), "") }
                AppButton { text: "Del";   height: 20; bgcolor: "#5a2a2a"; color: "#fff"; font.pixelSize: 9; onClicked: deleteSelectedKeys() }
                AppButton { text: playing ? "Pause" : "Play"; height: 20; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: Modeler.fcurvePlayPause() }
                AppButton { text: "Reset"; height: 20; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: { autoFit(); requestRepaint() } }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            height: 22
            color: "#1c1c1e"
            radius: 3

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 8

                CheckBox {
                    id: liveToggle
                    text: "Live apply"
                    font.pixelSize: 9
                    checked: true
                    contentItem: Text {
                        leftPadding: liveToggle.indicator.width + 4
                        text: liveToggle.text
                        font.pixelSize: 9
                        color: "#aaa"
                        verticalAlignment: Text.AlignVCenter
                    }
                    indicator: Rectangle {
                        implicitWidth: 14; implicitHeight: 14
                        radius: 2
                        color: liveToggle.checked ? "#E10600" : "#333"
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: "#121212"
                            font.pixelSize: 9
                            visible: liveToggle.checked
                        }
                    }
                }
                CheckBox {
                    id: snapToggle
                    text: "Snap frames"
                    font.pixelSize: 9
                    checked: true
                    contentItem: Text {
                        leftPadding: snapToggle.indicator.width + 4
                        text: snapToggle.text
                        font.pixelSize: 9
                        color: "#aaa"
                        verticalAlignment: Text.AlignVCenter
                    }
                    indicator: Rectangle {
                        implicitWidth: 14; implicitHeight: 14
                        radius: 2
                        color: snapToggle.checked ? "#E10600" : "#333"
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: "#121212"
                            font.pixelSize: 9
                            visible: snapToggle.checked
                        }
                    }
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "Frame: " + (Modeler ? Modeler.animationTime.toFixed(1) : "0.0")
                    color: "#E10600"
                    font.pixelSize: 9
                    font.bold: true
                }
                Text {
                    text: hoverReadout.text
                    color: "#888"
                    font.pixelSize: 9
                }
            }
        }

        // ============================================================
        // F-Curve graph
        // ============================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            Layout.fillHeight: true
            color: "#18181a"
            border.color: "#333"
            border.width: 1
            radius: 3
            clip: true

            Canvas {
                id: graphCanvas
                anchors.fill: parent
                antialiasing: true

                onPaint: {
                    var ctx = getContext("2d")
                    var w = width, h = height
                    ctx.reset()
                    ctx.clearRect(0, 0, w, h)
                    ctx.fillStyle = "#18181a"
                    ctx.fillRect(0, 0, w, h)

                    // Grid
                    var stepF = 1
                    var steps = [1, 2, 5, 10, 20, 50, 100]
                    for (var s = 0; s < steps.length; ++s) {
                        if (steps[s] * frameScale >= 50) { stepF = steps[s]; break }
                    }
                    var f0 = Math.floor(xToFrame(0) / stepF) * stepF
                    var f1 = Math.ceil(xToFrame(w) / stepF) * stepF
                    ctx.strokeStyle = "#222"
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    for (var f = f0; f <= f1; f += stepF) {
                        var x = frameToX(f)
                        ctx.moveTo(x, 0)
                        ctx.lineTo(x, h)
                    }
                    ctx.stroke()

                    var valStep = 1
                    var vsteps = [0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100]
                    for (var vs = 0; vs < vsteps.length; ++vs) {
                        if (vsteps[vs] * valueScale >= 50) { valStep = vsteps[vs]; break }
                    }
                    var v0 = Math.floor(yToValue(h) / valStep) * valStep
                    var v1 = Math.ceil(yToValue(0) / valStep) * valStep
                    ctx.beginPath()
                    for (var val = v0; val <= v1; val += valStep) {
                        var y = valueToY(val)
                        ctx.moveTo(0, y)
                        ctx.lineTo(w, y)
                    }
                    ctx.stroke()

                    // Zero lines
                    ctx.strokeStyle = "#444"
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    var yz = valueToY(0)
                    if (yz >= 0 && yz <= h) { ctx.moveTo(0, yz); ctx.lineTo(w, yz) }
                    ctx.stroke()

                    var keys = activeKeys()

                    // Curve
                    if (keys.length > 0) {
                        var fA = Math.max(xToFrame(0), keys[0].frame)
                        var fB = Math.min(xToFrame(w), keys[keys.length - 1].frame)
                        var step = Math.max(0.01, 2 / frameScale)
                        ctx.strokeStyle = "#E10600"
                        ctx.lineWidth = 1.5
                        ctx.beginPath()
                        var started = false
                        for (var tf = fA; tf <= fB; tf += step) {
                            var cv = fcurvePanel.evalAt(tf)
                            var px = frameToX(tf)
                            var py = valueToY(cv)
                            if (!started) { ctx.moveTo(px, py); started = true }
                            else ctx.lineTo(px, py)
                        }
                        ctx.stroke()
                    }

                    // Playhead
                    var ph = frameToX(Modeler ? Modeler.animationTime : 0)
                    if (ph >= 0 && ph <= w) {
                        ctx.strokeStyle = "#E1B000"
                        ctx.lineWidth = 1
                        ctx.beginPath()
                        ctx.moveTo(ph, 0)
                        ctx.lineTo(ph, h)
                        ctx.stroke()
                    }

                    // Keys
                    for (var i = 0; i < keys.length; ++i) {
                        var kx = frameToX(keys[i].frame)
                        var ky = valueToY(keys[i].value)
                        var sel = isSelected(activeChannel, keys[i].frame)
                        ctx.fillStyle = sel ? "#E10600" : (keys[i].locked ? "#E1A500" : "#e0e0e0")
                        ctx.strokeStyle = sel ? "#ffffff" : "#222"
                        ctx.lineWidth = 1
                        var r = sel ? 4 : 3
                        ctx.beginPath()
                        ctx.rect(kx - r, ky - r, r * 2, r * 2)
                        ctx.fill()
                        ctx.stroke()
                    }
                }
            }

            MouseArea {
                id: graphMouse
                anchors.fill: parent
                hoverEnabled: true

                onWheel: {
                    var fx = fcurvePanel.xToFrame(mouseX)
                    var vy = fcurvePanel.yToValue(mouseY)
                    var factor = wheel.angleDelta.y > 0 ? 1.2 : 1 / 1.2
                    fcurvePanel.frameScale = Math.max(0.1, Math.min(500, fcurvePanel.frameScale * factor))
                    fcurvePanel.valueScale = Math.max(0.05, Math.min(500, fcurvePanel.valueScale * factor))
                    fcurvePanel.panX = mouseX - fx * fcurvePanel.frameScale
                    fcurvePanel.panY = mouseY + vy * fcurvePanel.valueScale
                    fcurvePanel.requestRepaint()
                }
                onPositionChanged: {
                    hoverReadout.text = ""
                    if (fcurvePanel.panMode) {
                        fcurvePanel.panX = fcurvePanel.panOriginX + (mouse.x - fcurvePanel.panStartX)
                        fcurvePanel.panY = fcurvePanel.panOriginY + (mouse.y - fcurvePanel.panStartY)
                        fcurvePanel.requestRepaint()
                        return
                    }
                    if (fcurvePanel.draggingKey) {
                        var f = fcurvePanel.xToFrame(mouse.x)
                        var v = fcurvePanel.yToValue(mouse.y)
                        if (snapToggle.checked) f = Math.round(f)
                        var idx = fcurvePanel.dragIndex
                        Modeler.fcurveMoveKey(objectId, activeChannel, idx, f)
                        var ks = activeKeys()
                        for (var i = 0; i < ks.length; ++i) {
                            if (Math.abs(ks[i].frame - f) < 0.01) {
                                Modeler.fcurveSetValue(objectId, activeChannel, i, v)
                                fcurvePanel.dragIndex = i
                                break
                            }
                        }
                        fcurvePanel.dragF0 = f
                        fcurvePanel.dragV0 = v
                        // Keep selection in sync
                        for (var j = 0; j < fcurvePanel.selectedKeys.length; ++j) {
                            if (fcurvePanel.selectedKeys[j].channel === activeChannel &&
                                Math.abs(fcurvePanel.selectedKeys[j].frame - fcurvePanel.dragF0) < 0.01)
                                fcurvePanel.selectedKeys[j].frame = f
                        }
                    } else {
                        var hit = fcurvePanel.hitTest(mouse.x, mouse.y)
                        if (hit) hoverReadout.text = "(" + hit.frame.toFixed(1) + ", " + hit.value.toFixed(2) + ")"
                        else hoverReadout.text = "(" + fcurvePanel.xToFrame(mouse.x).toFixed(1) + ", " + fcurvePanel.yToValue(mouse.y).toFixed(2) + ")"
                    }
                }
                onPressed: {
                    if (mouse.button === Qt.MiddleButton || mouse.button === Qt.RightButton) {
                        fcurvePanel.panMode = true
                        fcurvePanel.panStartX = mouse.x
                        fcurvePanel.panStartY = mouse.y
                        fcurvePanel.panOriginX = fcurvePanel.panX
                        fcurvePanel.panOriginY = fcurvePanel.panY
                        return
                    }
                    if (mouse.button !== Qt.LeftButton) return
                    var hit = fcurvePanel.hitTest(mouse.x, mouse.y)
                    if (hit) {
                        fcurvePanel.selectKey(activeChannel, hit.index, hit.frame, hit.value, mouse.modifiers & Qt.ShiftModifier)
                        fcurvePanel.draggingKey = true
                        fcurvePanel.dragIndex = hit.index
                        fcurvePanel.dragF0 = hit.frame
                        fcurvePanel.dragV0 = hit.value
                    } else {
                        if (!(mouse.modifiers & Qt.ShiftModifier)) fcurvePanel.selectedKeys = []
                        fcurvePanel.requestRepaint()
                    }
                }
                onReleased: {
                    fcurvePanel.panMode = false
                    fcurvePanel.draggingKey = false
                }
                onDoubleClicked: {
                    if (objectId >= 0) {
                        var f = fcurvePanel.xToFrame(mouse.x)
                        var v = fcurvePanel.yToValue(mouse.y)
                        if (snapToggle.checked) f = Math.round(f)
                        fcurvePanel.addKeyAt(f, v, "")
                    }
                }
            }
        }

        // ============================================================
        // Dope Sheet
        // ============================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            height: 150
            color: "#18181a"
            border.color: "#333"
            border.width: 1
            radius: 3
            clip: true

            Canvas {
                id: dopeCanvas
                anchors.fill: parent
                antialiasing: true

                onPaint: {
                    var ctx = getContext("2d")
                    var w = width, h = height
                    ctx.reset()
                    ctx.clearRect(0, 0, w, h)
                    ctx.fillStyle = "#18181a"
                    ctx.fillRect(0, 0, w, h)

                    var gutter = 60
                    var rulerH = 16
                    var rowH = (h - rulerH) / fcurvePanel.channels.length
                    var stepF = 1
                    var steps = [1, 2, 5, 10, 20, 50, 100]
                    for (var s = 0; s < steps.length; ++s)
                        if (steps[s] * frameScale >= 60) { stepF = steps[s]; break }

                    // Ruler
                    ctx.fillStyle = "#222"
                    ctx.fillRect(0, 0, w, rulerH)
                    ctx.strokeStyle = "#333"
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    var f0 = Math.floor((fcurvePanel.xToFrame(-gutter)) / stepF) * stepF
                    var f1 = Math.ceil(fcurvePanel.xToFrame(w) / stepF) * stepF
                    for (var f = f0; f <= f1; f += stepF) {
                        var x = fcurvePanel.dopeFrameToX(f)
                        ctx.moveTo(x, 0)
                        ctx.lineTo(x, rulerH)
                    }
                    ctx.stroke()
                    ctx.fillStyle = "#888"
                    ctx.font = "8px sans-serif"
                    ctx.textAlign = "center"
                    for (var fl = f0; fl <= f1; fl += stepF) {
                        var xl = fcurvePanel.dopeFrameToX(fl)
                        if (xl >= gutter && xl <= w - 4)
                            ctx.fillText(String(fl), xl, rulerH - 4)
                    }

                    // Rows
                    for (var r = 0; r < fcurvePanel.channels.length; ++r) {
                        var ch = fcurvePanel.channels[r]
                        var yTop = rulerH + r * rowH
                        if (r % 2 === 0) {
                            ctx.fillStyle = "#1d1d1f"
                            ctx.fillRect(0, yTop, w, rowH)
                        }
                        var isActive = ch === activeChannel
                        // Label
                        ctx.fillStyle = isActive ? "#E10600" : "#777"
                        ctx.font = "bold 8px sans-serif"
                        ctx.textAlign = "left"
                        ctx.fillText(ch, 6, yTop + rowH / 2 + 3)

                        // Keys
                        var klist = fcurvePanel.keysFor(ch)
                        for (var i = 0; i < klist.length; ++i) {
                            var kx = fcurvePanel.dopeFrameToX(klist[i].frame)
                            var ky = yTop + rowH / 2
                            var sel = isSelected(ch, klist[i].frame)
                            ctx.fillStyle = sel ? "#E10600" : (klist[i].locked ? "#E1A500" : "#c0c0c0")
                            ctx.strokeStyle = sel ? "#fff" : "#222"
                            ctx.lineWidth = 1
                            ctx.beginPath()
                            ctx.moveTo(kx, ky - 4)
                            ctx.lineTo(kx + 4, ky)
                            ctx.lineTo(kx, ky + 4)
                            ctx.lineTo(kx - 4, ky)
                            ctx.closePath()
                            ctx.fill()
                            ctx.stroke()
                        }
                    }

                    // Row separators
                    ctx.strokeStyle = "#262626"
                    ctx.beginPath()
                    for (var rr = 0; rr <= fcurvePanel.channels.length; ++rr) {
                        var yy = rulerH + rr * rowH
                        ctx.moveTo(0, yy)
                        ctx.lineTo(w, yy)
                    }
                    ctx.stroke()

                    // Playhead
                    var ph = fcurvePanel.dopeFrameToX(Modeler ? Modeler.animationTime : 0)
                    if (ph >= 0 && ph <= w) {
                        ctx.strokeStyle = "#E1B000"
                        ctx.lineWidth = 1
                        ctx.beginPath()
                        ctx.moveTo(ph, 0)
                        ctx.lineTo(ph, h)
                        ctx.stroke()
                    }
                }
            }

            MouseArea {
                id: dopeMouse
                anchors.fill: parent

                function rowAt(y) {
                    var rulerH = 16
                    var rowH = (dopeCanvas.height - rulerH) / fcurvePanel.channels.length
                    var r = Math.floor((y - rulerH) / rowH)
                    if (r < 0 || r >= fcurvePanel.channels.length) return -1
                    return r
                }
                function hitDiamond(ch, x, y, rowH) {
                    var klist = fcurvePanel.keysFor(ch)
                    for (var i = 0; i < klist.length; ++i) {
                        var kx = fcurvePanel.dopeFrameToX(klist[i].frame)
                        if (Math.abs(kx - x) < 6) return i
                    }
                    return -1
                }

                onPressed: {
                    var rulerH = 16
                    var rowH = (dopeCanvas.height - rulerH) / fcurvePanel.channels.length
                    if (mouse.y < rulerH) {
                        // Scrub on ruler
                        var f = fcurvePanel.xToFrame(mouse.x - 60)
                        if (snapToggle.checked) f = Math.round(f)
                        Modeler.setAnimationTime(Math.max(0, f))
                        return
                    }
                    var r = rowAt(mouse.y)
                    if (r < 0) return
                    var ch = fcurvePanel.channels[r]
                    var hi = hitDiamond(ch, mouse.x, mouse.y, rowH)
                    if (hi >= 0) {
                        var klist = fcurvePanel.keysFor(ch)
                        fcurvePanel.selectKey(ch, hi, klist[hi].frame, klist[hi].value, mouse.modifiers & Qt.ShiftModifier)
                    } else {
                        fcurvePanel.activeChannel = ch
                        if (!(mouse.modifiers & Qt.ShiftModifier)) fcurvePanel.selectedKeys = []
                        fcurvePanel.requestRepaint()
                    }
                }
                onDoubleClicked: {
                    var r = rowAt(mouse.y)
                    if (r < 0) return
                    var ch = fcurvePanel.channels[r]
                    fcurvePanel.activeChannel = ch
                    var f = fcurvePanel.xToFrame(mouse.x - 60)
                    if (snapToggle.checked) f = Math.round(f)
                    fcurvePanel.addKeyAt(f, fcurvePanel.valueForChannel(ch), ch)
                }
            }
        }
    }

    Text {
        id: hoverReadout
        visible: false
    }
}
