import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: sculptPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property int brushMode: 0
    property real brushRadius: parent && parent.parent ? Math.max(0.02, Math.min(5.0, Math.max(parent.parent.width, parent.parent.height) * 0.01)) : 0.3
    property real brushStrength: 0.2
    property real brushFalloff: 2.0
    property int pinnedCount: 0
    property int multiresLevel: 0
    property int sculptLayer: -1

    function applyBrushMode(mode, label) {
        brushMode = mode
        var root = sculptPanel.parent
        while (root && root.objectName !== "pageModelerRoot") root = root.parent
        if (root && root.sculptMode !== undefined) root.sculptMode = mode
    }

    function setRadius(r) {
        brushRadius = r
        var root = sculptPanel.parent
        while (root && root.objectName !== "pageModelerRoot") root = root.parent
        if (root && root.sculptRadius !== undefined) root.sculptRadius = r
    }

    function setStrength(s) {
        brushStrength = s
        var root = sculptPanel.parent
        while (root && root.objectName !== "pageModelerRoot") root = root.parent
        if (root && root.sculptStrength !== undefined) root.sculptStrength = s
    }

    function setFalloff(f) {
        brushFalloff = f
        var root = sculptPanel.parent
        while (root && root.objectName !== "pageModelerRoot") root = root.parent
        if (root && root.sculptFalloff !== undefined) root.sculptFalloff = f
    }

    function setMultiresLevel(level) {
        brushRadius = Math.max(0.02, Math.min(5.0, level * 0.5))
        brushStrength = 0.5
        brushFalloff = 2.0
        brushMode = 0
        sculptMode = -1
        if (root && root.multiresSetCurrentLevel !== undefined) root.multiresSetCurrentLevel(level)
        if (root && root.multiresLevelCount !== undefined) {
            var count = root.multiresLevelCount()
            if (level >= 0 && level < count) {
                root.multiresSetCurrentLevel(level)
                sculptLayer = root.multiresLevelCount() > 0 ? 0 : -1
            }
        }
    }

    function setSculptLayer(layerIdx) {
        sculptLayer = layerIdx
        if (root && root.multiresManager !== undefined && root.multiresManager.layers !== undefined) {
            var count = root.multiresManager.layers.length
            if (layerIdx >= 0 && layerIdx < count) {
                if (root && root.sculptLayer !== undefined) root.sculptLayer = layerIdx
            }
        }
    }

    function pinSelected(pinned) {
        if (objectId < 0) return
        var sel = Modeler.selectedSubVertices ? Modeler.selectedSubVertices : null
        if (!sel || sel.length === 0) {
            if (Modeler.statusMessage) Modeler.statusMessage("Select vertices first (Vertex mode) to pin them")
            return
        }
        Modeler.setSculptPins(objectId, sel, pinned)
        pinnedCount = Modeler.sculptPinnedCount(objectId)
    }

    function refreshPins() {
        pinnedCount = objectId >= 0 && Modeler.sculptPinnedCount ? Modeler.sculptPinnedCount(objectId) : 0
    }

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            refreshPins()
        }
        function onSceneChanged() { refreshPins() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "SCULPT"
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
            text: objectId >= 0 ? "Target: " + Modeler.selectedObject.name
                                : "No object selected (click an object to sculpt)"
            color: objectId >= 0 ? "#aaa" : "#E10600"
            font.pixelSize: 10
            font.bold: objectId >= 0
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "BRUSH (click & drag on the object in the User viewport)"; color: "#888"; font.pixelSize: 10; font.bold: true }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: 4
            rowSpacing: 4

            AppButton { text: "Draw"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 0 ? "#E10600" : "#3e3e42"; color: brushMode === 0 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 0; onClicked: applyBrushMode(0, "Draw") }
            AppButton { text: "Smooth"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 1 ? "#E10600" : "#3e3e42"; color: brushMode === 1 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 1; onClicked: applyBrushMode(1, "Smooth") }
            AppButton { text: "Grab"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 2 ? "#E10600" : "#3e3e42"; color: brushMode === 2 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 2; onClicked: applyBrushMode(2, "Grab") }
            AppButton { text: "Flatten"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 3 ? "#E10600" : "#3e3e42"; color: brushMode === 3 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 3; onClicked: applyBrushMode(3, "Flatten") }
            AppButton { text: "Crease"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 4 ? "#E10600" : "#3e3e42"; color: brushMode === 4 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 4; onClicked: applyBrushMode(4, "Crease") }
            AppButton { text: "Inflate"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 5 ? "#E10600" : "#3e3e42"; color: brushMode === 5 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 5; onClicked: applyBrushMode(5, "Inflate") }
            AppButton { text: "Pinch"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 6 ? "#E10600" : "#3e3e42"; color: brushMode === 6 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 6; onClicked: applyBrushMode(6, "Pinch") }
            AppButton { text: "Smear"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 7 ? "#E10600" : "#3e3e42"; color: brushMode === 7 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 7; onClicked: applyBrushMode(7, "Smear") }
            AppButton { text: "Negate"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 8 ? "#E10600" : "#3e3e42"; color: brushMode === 8 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 8; onClicked: applyBrushMode(8, "Negate") }
            AppButton { text: "Folds"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 9 ? "#E10600" : "#3e3e42"; color: brushMode === 9 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 9; onClicked: applyBrushMode(9, "Folds") }
            AppButton { text: "Pores"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 10 ? "#E10600" : "#3e3e42"; color: brushMode === 10 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 10; onClicked: applyBrushMode(10, "Pores") }
            AppButton { text: "Bulge"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 11 ? "#E10600" : "#3e3e42"; color: brushMode === 11 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 11; onClicked: applyBrushMode(11, "Bulge") }
            AppButton { text: "Slash"; height: 26; checkable: true; autoExclusive: true; bgcolor: brushMode === 12 ? "#E10600" : "#3e3e42"; color: brushMode === 12 ? "#121212" : "#fff"; font.pixelSize: 10; checked: brushMode === 12; onClicked: applyBrushMode(12, "Slash") }
        }

        Text { text: "Radius: " + brushRadius.toFixed(3); color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: radiusSlider
            Layout.fillWidth: true
            from: 0.02; to: 2.0; stepSize: 0.01
            value: brushRadius
            onMoved: setRadius(value)
            background: Rectangle { x: radiusSlider.leftPadding; y: radiusSlider.topPadding + radiusSlider.availableHeight / 2 - 2; width: radiusSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: radiusSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#E10600" }
            }
            handle: Rectangle { x: radiusSlider.leftPadding + radiusSlider.visualPosition * (radiusSlider.availableWidth - width); y: radiusSlider.topPadding + radiusSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        Text { text: "Strength: " + brushStrength.toFixed(2); color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: strengthSlider
            Layout.fillWidth: true
            from: 0.0; to: 1.0; stepSize: 0.01
            value: brushStrength
            onMoved: setStrength(value)
            background: Rectangle { x: strengthSlider.leftPadding; y: strengthSlider.topPadding + strengthSlider.availableHeight / 2 - 2; width: strengthSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: strengthSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#ff6600" }
            }
            handle: Rectangle { x: strengthSlider.leftPadding + strengthSlider.visualPosition * (strengthSlider.availableWidth - width); y: strengthSlider.topPadding + strengthSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        Text { text: "Falloff curve: " + brushFalloff.toFixed(2) + " (2 = smoothstep)"; color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: falloffSlider
            Layout.fillWidth: true
            from: 0.25; to: 8.0; stepSize: 0.25
            value: brushFalloff
            onMoved: setFalloff(value)
            background: Rectangle { x: falloffSlider.leftPadding; y: falloffSlider.topPadding + falloffSlider.availableHeight / 2 - 2; width: falloffSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: falloffSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#7a5cf0" }
            }
            handle: Rectangle { x: falloffSlider.leftPadding + falloffSlider.visualPosition * (falloffSlider.availableWidth - width); y: falloffSlider.topPadding + falloffSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "MULTIRESOLUTION"; color: "#E10600"; font.pixelSize: 10; font.bold: true }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton { text: "Add Level"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: setMultiresLevel(root.multiresLevelCount() > 0 ? root.multiresLevelCount() - 1 : 0) }
            AppButton { text: "Remove Level"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: setMultiresLevel(root.multiresLevelCount() > 1 ? root.multiresLevelCount() - 2 : 0) }
            AppButton { text: "Bake"; height: 24; bgcolor: "#e10600"; color: "#121212"; font.pixelSize: 9; onClicked: root.multiresBakeCurrent ? root.multiresBakeCurrent() : void(0) }
        }
        Text { text: "Level: " + (root.multiresCurrentLevel ? root.multiresCurrentLevel() : 0) + "/" + (root.multiresLevelCount ? root.multiresLevelCount() : 0); color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: multiresSlider
            Layout.fillWidth: true
            from: 0; to: 6; stepSize: 1
            value: multiresLevel
            onMoved: setMultiresLevel(value)
            enabled: root.multiresLevelCount ? root.multiresLevelCount() > 1 : false
            background: Rectangle { x: multiresSlider.leftPadding; y: multiresSlider.topPadding + multiresSlider.availableHeight / 2 - 2; width: multiresSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: multiresSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#E10600" }
            }
            handle: Rectangle { x: multiresSlider.leftPadding + multiresSlider.visualPosition * (multiresSlider.availableWidth - width); y: multiresSlider.topPadding + multiresSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        Text { text: "SCULPT LAYERS"; color: "#E10600"; font.pixelSize: 10; font.bold: true; marginTop: 8 }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton { text: "Add Layer"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: setSculptLayer(root.multiresManager ? root.multiresManager.layers.length : -1) }
            AppButton { text: "Remove Layer"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: setSculptLayer(root.multiresManager ? root.multiresManager.layers.length - 1 : -1) }
        }
        Text { text: "Layer: " + (sculptLayer >= 0 ? sculptLayer : "none") + (root.multiresManager ? "/" + root.multiresManager.layers.length : ""); color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: layerOpacitySlider
            Layout.fillWidth: true
            from: 0.0; to: 1.0; stepSize: 0.05
            value: root.multiresManager ? root.multiresManager.layers[sculptLayer].opacity : 1.0
            enabled: sculptLayer >= 0 && root.multiresManager && root.multiresManager.layers.length > sculptLayer
            onMoved: root.multiresManager ? root.multiresManager.setLayerOpacity(sculptLayer, value) : void(0)
            background: Rectangle { x: layerOpacitySlider.leftPadding; y: layerOpacitySlider.topPadding + layerOpacitySlider.availableHeight / 2 - 2; width: layerOpacitySlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: layerOpacitySlider.visualPosition * parent.width; height: 4; radius: 2; color: "#7a5cf0" }
            }
            handle: Rectangle { x: layerOpacitySlider.leftPadding + layerOpacitySlider.visualPosition * (layerOpacitySlider.availableWidth - width); y: layerOpacitySlider.topPadding + layerOpacitySlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }
        Text { text: "Opacity: " + (root.multiresManager ? root.multiresManager.layers[sculptLayer].opacity.toFixed(2) : "N/A"); color: "#aaa"; font.pixelSize: 9 }
        ComboBox {
            id: blendModeCombo
            enabled: sculptLayer >= 0 && root.multiresManager && root.multiresManager.layers.length > sculptLayer
            model: ["Additive", "Subtractive", "Replace"]
            currentIndex: root.multiresManager ? root.multiresManager.layers[sculptLayer].blendMode : 0
            onActivated: root.multiresManager ? root.multiresManager.setLayerBlendMode(sculptLayer, currentIndex) : void(0)
        }
        CheckBox {
            text: "Lock Layer"
            enabled: sculptLayer >= 0 && root.multiresManager && root.multiresManager.layers.length > sculptLayer
            checked: root.multiresManager ? root.multiresManager.layers[sculptLayer].locked : false
            onCheckedChanged: root.multiresManager ? root.multiresManager.setLayerLocked(sculptLayer, checked) : void(0)
        }

        Text { text: "MORPH TARGETS"; color: "#E10600"; font.pixelSize: 10; font.bold: true; marginTop: 8 }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton { text: "Add Morph Target"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: root.morphTargetEditor ? root.morphTargetEditor.addMorphTarget("Target " + (root.morphTargetEditor.targetCount() + 1) : void(0) }
            }
            AppButton { text: "Remove Morph Target"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; onClicked: root.morphTargetEditor ? root.morphTargetEditor.removeMorphTarget(root.morphTargetEditor.currentTarget()) : void(0) }
        }
        Text { text: "Current: " + (root.morphTargetEditor ? root.morphTargetEditor.currentTargetName() : "none"); color: "#aaa"; font.pixelSize: 9 }
        ComboBox {
            id: morphTargetCombo
            enabled: root.morphTargetEditor && root.morphTargetEditor.targetCount() > 0
            model: root.morphTargetEditor ? root.morphTargetEditor.targetNames() : []
            currentIndex: root.morphTargetEditor ? root.morphTargetEditor.currentTarget() : -1
            onActivated: root.morphTargetEditor ? root.morphTargetEditor.setCurrentTarget(currentIndex) : void(0)
        }
        Slider {
            id: morphWeightSlider
            Layout.fillWidth: true
            from: 0.0; to: 1.0; stepSize: 0.01
            value: root.morphTargetEditor ? root.morphTargetEditor.morphTargetWeight(root.morphTargetEditor.currentTarget()) : 0.0
            enabled: root.morphTargetEditor && root.morphTargetEditor.targetCount() > 0 && root.morphTargetEditor.currentTarget() >= 0
            onMoved: root.morphTargetEditor ? root.morphTargetEditor.setMorphTargetWeight(root.morphTargetEditor.currentTarget(), value) : void(0)
            background: Rectangle { x: morphWeightSlider.leftPadding; y: morphWeightSlider.topPadding + morphWeightSlider.availableHeight / 2 - 2; width: morphWeightSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: morphWeightSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#E10600" }
            }
            handle: Rectangle { x: morphWeightSlider.leftPadding + morphWeightSlider.visualPosition * (morphWeightSlider.availableWidth - width); y: morphWeightSlider.topPadding + morphWeightSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }
        AppButton {
            text: "Sculpt to Target"
            enabled: root.morphTargetEditor && root.morphTargetEditor.targetCount() > 0 && root.morphTargetEditor.currentTarget() >= 0
            height: 30
            bgcolor: "#E10600"
            color: "#121212"
            font.bold: true
            font.pixelSize: 10
            onClicked: root.morphTargetEditor ? root.morphTargetEditor.sculptBrushToTarget(root.morphTargetEditor.currentTarget(), QVector3D(0,0,0), sculptRadius, sculptStrength, sculptMode, QVector3D(), QVector3D()) : void(0)
        }

        Text { text: "PIN / LOCK vertices (select in Vertex mode first)"; color: "#888"; font.pixelSize: 10; font.bold: true }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton { text: "Pin Sel"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; Layout.fillWidth: true; onClicked: pinSelected(true) }
            AppButton { text: "Unpin Sel"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; Layout.fillWidth: true; onClicked: pinSelected(false) }
            AppButton { text: "Clear"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; Layout.fillWidth: true; onClicked: { if (objectId >= 0 && Modeler.clearSculptPins) Modeler.clearSculptPins(objectId); refreshPins() } }
        }
        Text {
            text: pinnedCount > 0 ? pinnedCount + " vertex(es) locked (brushes skip them)" : "No pinned vertices"
            color: pinnedCount > 0 ? "#E10600" : "#666"
            font.pixelSize: 9
            font.bold: pinnedCount > 0
        }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: brushMode >= 0 ? "Active: " + ["Draw", "Smooth", "Grab", "Flatten", "Crease", "Inflate", "Pinch", "Smear", "Negate", "Folds", "Pores", "Bulge", "Slash"][brushMode] + " (drag in viewport)" : "Select a brush"
            bgcolor: brushMode >= 0 ? "#E10600" : "#3e3e42"
            color: "#121212"
            font.bold: true
            font.pixelSize: 11
            onClicked: applyBrushMode(brushMode, "current")
        }

        Item { Layout.fillHeight: true }

        Text {
            text: "Tip: use a denser mesh (Subdivide in Modify tab) for finer detail. Hold still to repeat the brush on one spot."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
