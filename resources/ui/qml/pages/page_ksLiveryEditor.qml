import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ksEditor.TexturePainter 1.0
import ksEditor.Content 1.0

Rectangle {
    id: root
    color: "#1e1e1e"

    property string currentTool: "brush"
    property string currentSkin: ""
    property string currentCar: ""
    property real zoomLevel: 1.0
    property real canvasOffsetX: 0
    property real canvasOffsetY: 0
    property color fgColor: "#000000"
    property color bgColor: "#ffffff"
    property real brushSize: 20
    property real brushOpacity: 100
    property real brushFlow: 100
    property int currentLayer: 0
    property bool showRulers: true
    property alias modelSkins: skinListModel
    property alias modelLayers: layerListModel

    ListModel { id: skinListModel }
    ListModel { id: layerListModel }

    function loadSkins() {
        if (currentCar === "") return
        var skins = Content.listSkins(currentCar)
        skinListModel.clear()
        for (var i = 0; i < skins.length; ++i) {
            skinListModel.append(skins[i])
        }
    }

    function setTool(tool) {
        currentTool = tool
        toolPanel.currentIndex = -1
        for (var i = 0; i < toolPanel.children.length; ++i) {
            var btn = toolPanel.children[i]
            if (btn.objectName === tool) {
                toolPanel.currentIndex = i
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Menu bar
        MenuBar {
            Layout.fillWidth: true
            font.pixelSize: 11
            background: Rectangle { color: "#323232" }

            Menu {
                title: "File"
                Action { text: "New"; shortcut: "Ctrl+N"; onTriggered: { TexturePainter.clearCanvas(); statusMsg.text = "New canvas" } }
                Action { text: "Open..."; shortcut: "Ctrl+O"; onTriggered: openTextureDialog.open() }
                Action { text: "Save"; shortcut: "Ctrl+S"; onTriggered: saveTextureDialog.open() }
                Action { text: "Save As..."; shortcut: "Ctrl+Shift+S"; onTriggered: saveTextureDialog.open() }
                MenuSeparator {}
                Action { text: "Import..."; shortcut: "Ctrl+I"; onTriggered: openTextureDialog.open() }
                Action { text: "Export..."; shortcut: "Ctrl+E"; onTriggered: saveTextureDialog.open() }
                MenuSeparator {}
                Action { text: "Close"; shortcut: "Ctrl+W"; onTriggered: statusMsg.text = "Close livery" }
            }

            Menu {
                title: "Edit"
                Action { text: "Undo"; shortcut: "Ctrl+Z"; enabled: TexturePainter.canUndo()
                    onTriggered: TexturePainter.undo() }
                Action { text: "Redo"; shortcut: "Ctrl+Shift+Z"; enabled: TexturePainter.canRedo()
                    onTriggered: TexturePainter.redo() }
                MenuSeparator {}
                Action { text: "Fill..."; onTriggered: TexturePainter.floodFill(256, 256, 0.1) }
                Action { text: "Stroke..."; onTriggered: statusMsg.text = "Stroke" }
            }

            Menu {
                title: "Select"
                Action { text: "All"; shortcut: "Ctrl+A"; onTriggered: TexturePainter.setSelection(0, 0, TexturePainter.canvasWidth, TexturePainter.canvasHeight) }
                Action { text: "None"; shortcut: "Ctrl+D"; onTriggered: TexturePainter.clearSelection() }
                Action { text: "Inverse"; shortcut: "Ctrl+Shift+I"; onTriggered: statusMsg.text = "Inverse selection" }
            }

            Menu {
                title: "View"
                Action { text: "Zoom In"; shortcut: "Ctrl++"; onTriggered: zoomLevel = Math.min(4, zoomLevel + 0.25) }
                Action { text: "Zoom Out"; shortcut: "Ctrl+-"; onTriggered: zoomLevel = Math.max(0.25, zoomLevel - 0.25) }
                Action { text: "Fit to Screen"; shortcut: "Ctrl+0"; onTriggered: zoomLevel = 1.0 }
                MenuSeparator {}
                Action { text: "Show Rulers"; checkable: true; checked: showRulers; onTriggered: showRulers = checked }
            }

            Menu {
                title: "Image"
                Action { text: "Canvas Size..."; onTriggered: statusMsg.text = "Canvas size: " + TexturePainter.canvasWidth + "x" + TexturePainter.canvasHeight }
                Action { text: "Flatten Image"; onTriggered: TexturePainter.clearCanvas() }
                Action { text: "Invert Colors"; onTriggered: TexturePainter.applyInvert() }
            }

            Menu {
                title: "Layer"
                Action { text: "New Layer"; shortcut: "Ctrl+Shift+N"; onTriggered: TexturePainter.addLayer("Layer " + (TexturePainter.layerCount + 1)) }
                Action { text: "Duplicate Layer"; onTriggered: statusMsg.text = "Duplicate layer" }
                Action { text: "Delete Layer"; onTriggered: TexturePainter.removeLayer(currentLayer) }
                MenuSeparator {}
                Action { text: "Merge Down"; onTriggered: statusMsg.text = "Merge down" }
                Action { text: "Flatten Image"; onTriggered: statusMsg.text = "Flatten" }
            }

            Menu {
                title: "Colors"
                Action { text: "Hue/Saturation..."; onTriggered: TexturePainter.applyHueSaturation(0, 0, 0) }
                Action { text: "Levels..."; onTriggered: TexturePainter.applyLevels(0, 1.0, 255) }
            }

            Menu {
                title: "Filters"
                Action { text: "Blur..."; onTriggered: TexturePainter.applyBlur(5.0) }
                Action { text: "Sharpen..."; onTriggered: TexturePainter.applySharpen(0.5) }
                Action { text: "Add Noise..."; onTriggered: TexturePainter.applyNoise(0.1) }
                Action { text: "Emboss..."; onTriggered: TexturePainter.applyEmboss(1.0) }
            }

            Menu {
                title: "Help"
                Action { text: "About Livery Editor"; onTriggered: statusMsg.text = "KSEditor Livery Editor v1.0" }
            }
        }

        // Tool options bar
        Rectangle {
            height: 32; Layout.fillWidth: true; color: "#2a2a2a"
            border.color: "#3a3a3a"; border.width: 1
            RowLayout {
                anchors.fill: parent; anchors.margins: 4; spacing: 4
                visible: currentTool === "brush" || currentTool === "pencil" || currentTool === "eraser"

                Text { text: "Size:"; color: "#999"; font.pixelSize: 10 }
                Slider { from: 1; to: 500; value: brushSize; Layout.preferredWidth: 80; height: 16
                    onValueChanged: { brushSize = value; if (TexturePainter) TexturePainter.setBrushSize(value) }
                }
                Text { text: Math.round(brushSize); color: "#ccc"; font.pixelSize: 10; width: 30 }

                Text { text: "Opacity:"; color: "#999"; font.pixelSize: 10 }
                Slider { from: 0; to: 100; value: brushOpacity; Layout.preferredWidth: 60; height: 16
                    onValueChanged: brushOpacity = value
                }
                Text { text: Math.round(brushOpacity) + "%"; color: "#ccc"; font.pixelSize: 10; width: 32 }

                Text { text: "Flow:"; color: "#999"; font.pixelSize: 10 }
                Slider { from: 0; to: 100; value: brushFlow; Layout.preferredWidth: 60; height: 16
                    onValueChanged: brushFlow = value
                }
                Text { text: Math.round(brushFlow) + "%"; color: "#ccc"; font.pixelSize: 10; width: 32 }
            }
        }

        // Main content
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            // Left: Tool palette
            Rectangle {
                width: 42; Layout.fillHeight: true; color: "#2d2d2d"
                border.color: "#3a3a3a"; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 3; spacing: 2

                    GridLayout {
                        columns: 2; columnSpacing: 2; rowSpacing: 2; Layout.fillWidth: true

                        Repeater {
                            model: [
                                { icon: "\u25A2", tool: "move", tip: "Move (V)" },
                                { icon: "\u25AD", tool: "marquee", tip: "Rect Select (M)" },
                                { icon: "\u25E2", tool: "lasso", tip: "Lasso (L)" },
                                { icon: "\u260C", tool: "wand", tip: "Magic Wand (W)" },
                                { icon: "\u2702", tool: "crop", tip: "Crop (C)" },
                                { icon: "\u2B1C", tool: "eyedropper", tip: "Eyedropper (I)" },
                                { icon: "\u270F", tool: "brush", tip: "Brush (B)" },
                                { icon: "\u270E", tool: "pencil", tip: "Pencil (P)" },
                                { icon: "\u2B2F", tool: "eraser", tip: "Eraser (E)" },
                                { icon: "\u25CB", tool: "airbrush", tip: "Airbrush (A)" },
                                { icon: "\u25A3", tool: "fill", tip: "Fill (G)" },
                                { icon: "\u2B21", tool: "gradient", tip: "Gradient" },
                                { icon: "\u00A9", tool: "clone", tip: "Clone Stamp (S)" },
                                { icon: "\u273E", tool: "heal", tip: "Healing (J)" },
                                { icon: "\u2B1A", tool: "blur", tip: "Blur" },
                                { icon: "\u2701", tool: "dodge", tip: "Dodge/Burn (O)" },
                                { icon: "\u25CF", tool: "sponge", tip: "Sponge" },
                                { icon: "\u2261", tool: "text", tip: "Text (T)" },
                                { icon: "\u25A0", tool: "shape", tip: "Shape (U)" },
                                { icon: "\u2798", tool: "line", tip: "Line" },
                                { icon: "\u25A1", tool: "path", tip: "Path" },
                                { icon: "\u2316", tool: "colorpicker", tip: "Color Picker" },
                                { icon: "\u2795", tool: "zoom", tip: "Zoom (Z)" },
                                { icon: "\u270B", tool: "hand", tip: "Hand (H)" },
                            ]
                            Rectangle {
                                width: 18; height: 18; radius: 2
                                color: currentTool === modelData.tool ? "#0066cc" : (toolMa.containsMouse ? "#3a3a3e" : "transparent")
                                border.color: currentTool === modelData.tool ? "#3399ff" : "transparent"
                                Text { anchors.centerIn: parent; text: modelData.icon; color: currentTool === modelData.tool ? "#fff" : "#aaa"; font.pixelSize: 11 }
                                MouseArea { id: toolMa; anchors.fill: parent; hoverEnabled: true; onClicked: setTool(modelData.tool) }
                                ToolTip { visible: toolMa.containsMouse; text: modelData.tip; delay: 600 }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    Rectangle { height: 1; color: "#444"; Layout.fillWidth: true }

                    Rectangle {
                        width: 36; height: 36; radius: 2; border.color: "#555"; border.width: 1
                        color: fgColor
                        MouseArea { anchors.fill: parent; onClicked: { fgDialog.open() } }
                    }
                    Rectangle {
                        width: 18; height: 18; radius: 1; border.color: "#555"; border.width: 1
                        anchors.left: parent.left; anchors.leftMargin: 18
                        color: bgColor
                        MouseArea { anchors.fill: parent; onClicked: { bgDialog.open() } }
                    }
                    Rectangle {
                        width: 36; height: 14; radius: 2; color: "#3a3a3e"
                        Text { anchors.centerIn: parent; text: "\u21C5"; color: "#ccc"; font.pixelSize: 10 }
                        MouseArea { anchors.fill: parent; onClicked: { var t = fgColor; fgColor = bgColor; bgColor = t } }
                    }
                }
            }

            // Center: Canvas
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; color: "#353535"
                clip: true

                Rectangle {
                    id: canvas
                    x: canvasOffsetX; y: canvasOffsetY
                    width: 512 * zoomLevel; height: 512 * zoomLevel
                    color: "#191919"
                    border.color: "#555"; border.width: 1

                    Rectangle {
                        anchors.fill: parent; anchors.margins: 1
                        color: "#ffffff"

                        Canvas {
                            id: paintCanvas
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d")
                                if (TexturePainter && TexturePainter.hasTexture()) {
                                    ctx.drawImage(TexturePainter.canvasImage(), 0, 0, width, height)
                                } else {
                                    ctx.fillStyle = "#ffffff"
                                    ctx.fillRect(0, 0, width, height)
                                    ctx.strokeStyle = "#dddddd"
                                    ctx.lineWidth = 1
                                    for (var x = 0; x < width; x += 32) {
                                        ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke()
                                    }
                                    for (var y = 0; y < height; y += 32) {
                                        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke()
                                    }
                                    ctx.fillStyle = "#aaaaaa"
                                    ctx.font = "14px sans-serif"
                                    ctx.textAlign = "center"
                                    ctx.fillText("Canvas - Load or create a texture", width/2, height/2)
                                }
                            }
                            Connections {
                                target: TexturePainter
                                function onTextureChanged() { paintCanvas.requestPaint() }
                                function onCanvasChanged() { paintCanvas.requestPaint() }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            property point lastPos

                            onPressed: (mouse) => {
                                lastPos = Qt.point(mouse.x, mouse.y)
                                if (mouse.button === Qt.LeftButton && currentTool === "brush") {
                                    var texX = mouse.x / zoomLevel
                                    var texY = mouse.y / zoomLevel
                                    TexturePainter.paintAt(texX, texY)
                                }
                            }

                            onPositionChanged: (mouse) => {
                                if (mouse.buttons & Qt.LeftButton) {
                                    if (currentTool === "brush") {
                                        var texX = mouse.x / zoomLevel
                                        var texY = mouse.y / zoomLevel
                                        TexturePainter.paintTo(texX, texY)
                                    } else if (currentTool === "hand") {
                                        canvasOffsetX += mouse.x - lastPos.x
                                        canvasOffsetY += mouse.y - lastPos.y
                                    }
                                }
                                lastPos = Qt.point(mouse.x, mouse.y)
                            }

                            onWheel: (wheel) => {
                                zoomLevel = Math.max(0.1, Math.min(32, zoomLevel * (1 + wheel.angleDelta.y * 0.001)))
                            }
                        }
                    }
                }
            }

            // Right: Layers + Channels + History
            Rectangle {
                width: 240; Layout.fillHeight: true; color: "#2d2d2d"
                border.color: "#3a3a3a"; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; spacing: 0

                    // Tab bar
                    Rectangle {
                        height: 24; Layout.fillWidth: true; color: "#252526"
                        RowLayout {
                            anchors.fill: parent; spacing: 0
                            Repeater {
                                model: ["Layers", "Channels", "Paths", "History"]
                                Rectangle {
                                    Layout.fillWidth: true; height: 24
                                    color: index === 0 ? "#2d2d2d" : "#252526"
                                    border.color: index === 0 ? "#444" : "#252526"; border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: index === 0 ? "#ccc" : "#666"; font.pixelSize: 10 }
                                    MouseArea { anchors.fill: parent; onClicked: {} }
                                }
                            }
                        }
                    }

                    // Layer list
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true; color: "#2d2d2d"

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 4; spacing: 2

                            ListView {
                                id: layerView
                                Layout.fillWidth: true; Layout.fillHeight: true
                                clip: true
                                spacing: 2
                                model: layerListModel
                                currentIndex: currentLayer

                                delegate: Rectangle {
                                    height: 36; width: parent.width
                                    color: ListView.isCurrentItem ? "#383838" : "transparent"
                                    border.color: "#3a3a3a"; border.width: 1
                                    radius: 2

                                    RowLayout {
                                        anchors.fill: parent; anchors.margins: 4; spacing: 4

                                        Rectangle {
                                            width: 28; height: 28; color: "transparent"
                                            border.color: "#555"; border.width: 1
                                            Image {
                                                anchors.fill: parent
                                                source: model.thumbnail || ""
                                                fillMode: Image.PreserveAspectFit
                                            }
                                        }

                                        ColumnLayout {
                                            spacing: 1
                                            Text { text: model.name || "Layer " + (index + 1); color: "#ccc"; font.pixelSize: 10 }
                                            Text { text: model.mode || "Normal"; color: "#666"; font.pixelSize: 8 }
                                        }

                                        Item { Layout.fillWidth: true }

                                        Rectangle {
                                            width: 16; height: 16; radius: 2
                                            color: model.visible !== false ? "#3a6a3a" : "#3a3a3e"
                                            Text { anchors.centerIn: parent; text: model.visible !== false ? "\u2713" : ""; color: "#8f8"; font.pixelSize: 9 }
                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: { layerListModel.setProperty(index, "visible", !(model.visible !== false)) }
                                            }
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            currentLayer = index
                                            layerView.currentIndex = index
                                        }
                                    }
                                }
                            }

                            Rectangle { height: 1; color: "#3a3a3a"; Layout.fillWidth: true }

                            RowLayout {
                                Layout.fillWidth: true; spacing: 2
                                Rectangle { height: 22; Layout.fillWidth: true; color: "#3a3a3e"; radius: 3
                                    Text { anchors.centerIn: parent; text: "+"; color: "#ccc"; font.pixelSize: 14 }
                                    MouseArea { anchors.fill: parent; hoverEnabled: true
                                        onClicked: {
                                            layerListModel.append({name: "Layer " + (layerListModel.count + 1), mode: "Normal", visible: true, thumbnail: ""})
                                            if (TexturePainter) TexturePainter.addLayer()
                                        }
                                    }
                                }
                                Rectangle { height: 22; Layout.fillWidth: true; color: "#3a3a3e"; radius: 3
                                    Text { anchors.centerIn: parent; text: "\u2212"; color: "#ccc"; font.pixelSize: 14 }
                                    MouseArea { anchors.fill: parent; hoverEnabled: true
                                        onClicked: {
                                            if (currentLayer >= 0 && currentLayer < layerListModel.count) {
                                                layerListModel.remove(currentLayer)
                                                if (TexturePainter) TexturePainter.removeLayer(currentLayer)
                                            }
                                        }
                                    }
                                }
                                Rectangle { height: 22; width: 22; color: "#3a3a3e"; radius: 3
                                    Text { anchors.centerIn: parent; text: "\u25B2"; color: "#ccc"; font.pixelSize: 10 }
                                    MouseArea { anchors.fill: parent; hoverEnabled: true
                                        onClicked: {
                                            if (currentLayer > 0) {
                                                layerListModel.move(currentLayer, currentLayer - 1, 1)
                                                currentLayer--
                                                if (TexturePainter) TexturePainter.moveLayer(currentLayer + 1, currentLayer)
                                            }
                                        }
                                    }
                                }
                                Rectangle { height: 22; width: 22; color: "#3a3a3e"; radius: 3
                                    Text { anchors.centerIn: parent; text: "\u25BC"; color: "#ccc"; font.pixelSize: 10 }
                                    MouseArea { anchors.fill: parent; hoverEnabled: true
                                        onClicked: {
                                            if (currentLayer < layerListModel.count - 1) {
                                                layerListModel.move(currentLayer, currentLayer + 1, 1)
                                                if (TexturePainter) TexturePainter.moveLayer(currentLayer, currentLayer + 1)
                                                currentLayer++
                                            }
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true; spacing: 2
                                Rectangle { height: 22; Layout.fillWidth: true; color: "#3a3a3e"; radius: 3
                                    Text { anchors.centerIn: parent; text: "Normal"; color: "#ccc"; font.pixelSize: 10 }
                                }
                                Rectangle { height: 22; width: 50; color: "#3a3a3e"; radius: 3
                                    Text { anchors.centerIn: parent; text: "100%"; color: "#ccc"; font.pixelSize: 10 }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Status bar
        Rectangle {
            height: 22; Layout.fillWidth: true; color: "#2a2a2a"
            RowLayout {
                anchors.fill: parent; anchors.margins: 4
                Text { id: statusMsg; text: currentTool.toUpperCase() + " TOOL"; color: "#0066cc"; font.pixelSize: 9; font.bold: true }
                Rectangle { width: 1; height: 14; color: "#444" }
                Text { text: "Size: " + Math.round(brushSize); color: "#888"; font.pixelSize: 9 }
                Rectangle { width: 1; height: 14; color: "#444" }
                Text { text: "Opacity: " + Math.round(brushOpacity) + "%"; color: "#888"; font.pixelSize: 9 }
                Rectangle { width: 1; height: 14; color: "#444" }
                Text { text: "Zoom: " + (zoomLevel * 100).toFixed(0) + "%"; color: "#888"; font.pixelSize: 9 }
                Item { Layout.fillWidth: true }
                Text { text: currentCar ? currentCar + " / " + currentSkin : "No car loaded"; color: "#666"; font.pixelSize: 9 }
                Rectangle { width: 1; height: 14; color: "#444" }
                Text { text: "ksEditor Livery v1.0"; color: "#555"; font.pixelSize: 9 }
            }
        }
    }

    ColorDialog {
        id: fgDialog; title: "Foreground Color"
        selectedColor: fgColor
        onAccepted: {
            fgColor = selectedColor
            if (TexturePainter) TexturePainter.setBrushColor(selectedColor)
        }
    }

    ColorDialog {
        id: bgDialog; title: "Background Color"
        selectedColor: bgColor
        onAccepted: bgColor = selectedColor
    }

    FileDialog {
        id: openTextureDialog
        title: "Open Texture"
        nameFilters: ["Image files (*.png *.jpg *.jpeg *.tga *.bmp)", "All Files (*)"]
        onAccepted: {
            var path = file.toString().replace("file:///", "")
            TexturePainter.loadTexture(path)
            statusMsg.text = "Opened: " + path.split("/").pop()
        }
    }

    FileDialog {
        id: saveTextureDialog
        title: "Save Texture"
        nameFilters: ["PNG (*.png)", "JPEG (*.jpg)", "TGA (*.tga)", "BMP (*.bmp)"]
        onAccepted: {
            var path = file.toString().replace("file:///", "")
            TexturePainter.saveTexture(path)
            statusMsg.text = "Saved: " + path.split("/").pop()
        }
    }

    Component.onCompleted: {
        if (TexturePainter) {
            TexturePainter.setBrushColor(fgColor)
            TexturePainter.setBrushSize(brushSize)
        }
        loadSkins()
    }
}
