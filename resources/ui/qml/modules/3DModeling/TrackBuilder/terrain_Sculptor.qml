import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

Rectangle {
    id: terrainSculptor
    color: "#1a1a1a"

    property string brushType: "raise"
    property real brushRadius: 5.0
    property real brushStrength: 0.5
    property real brushHardness: 0.5
    property string terrainFile: ""
    property int terrainWidth: 256
    property int terrainHeight: 256
    property bool isSculpting: false

    FileDialog {
        id: terrainOpenDialog
        title: "Open Terrain Heightmap"
        nameFilters: ["PNG files (*.png)", "All files (*)"]
        onAccepted: {
            if (TerrainEditor) {
                TerrainEditor.loadTerrain(selectedFile.toString().replace("file:///", ""))
                terrainFile = selectedFile.toString().replace("file:///", "")
            }
        }
    }

    FileDialog {
        id: terrainSaveDialog
        title: "Save Terrain Heightmap"
        nameFilters: ["PNG files (*.png)", "OBJ files (*.obj)", "All files (*)"]
        onAccepted: {
            if (TerrainEditor) {
                var path = selectedFile.toString().replace("file:///", "")
                if (path.endsWith(".png")) TerrainEditor.exportToPNG(path)
                else if (path.endsWith(".obj")) TerrainEditor.exportToOBJ(path)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar
        Rectangle {
            height: 32
            color: "#252525"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Text { text: "Terrain Sculptor"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                Text { text: terrainWidth + "x" + terrainHeight; color: "#888"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }

                Button {
                    text: "Open"
                    flat: true
                    font.pixelSize: 11
                    color: "#aaa"
                    onClicked: terrainOpenDialog.open()
                }
                Button {
                    text: "Save"
                    flat: true
                    font.pixelSize: 11
                    color: "#aaa"
                    onClicked: terrainSaveDialog.open()
                }
                Button {
                    text: "Undo"
                    flat: true
                    font.pixelSize: 11
                    color: TerrainEditor && TerrainEditor.canUndo ? "#aaa" : "#555"
                    enabled: TerrainEditor && TerrainEditor.canUndo
                    onClicked: TerrainEditor.undo()
                }
                Button {
                    text: "Redo"
                    flat: true
                    font.pixelSize: 11
                    color: TerrainEditor && TerrainEditor.canRedo ? "#aaa" : "#555"
                    enabled: TerrainEditor && TerrainEditor.canRedo
                    onClicked: TerrainEditor.redo()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Left panel - Brush tools
            Rectangle {
                width: 200
                color: "#1e1e1e"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 15

                    Text { text: "BRUSH TYPE"; color: "#666"; font.pixelSize: 10; font.bold: true }

                    // Brush type buttons
                    Repeater {
                        model: [
                            { type: "raise", label: "Raise", color: "#27ae60" },
                            { type: "lower", label: "Lower", color: "#e74c3c" },
                            { type: "smooth", label: "Smooth", color: "#3498db" },
                            { type: "flatten", label: "Flatten", color: "#f39c12" },
                            { type: "noise", label: "Noise", color: "#9b59b6" },
                            { type: "paint", label: "Paint", color: "#1abc9c" }
                        ]
                        delegate: Button {
                            text: modelData.label
                            flat: true
                            Layout.fillWidth: true
                            height: 28
                            background: Rectangle {
                                color: terrainSculptor.brushType === modelData.type ? modelData.color : "#2d2d2d"
                                radius: 4
                                border.color: "#444"
                                border.width: 1
                            }
                            contentItem: Text {
                                text: parent.text
                                color: terrainSculptor.brushType === modelData.type ? "#121212" : "#fff"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                terrainSculptor.brushType = modelData.type
                                if (TerrainEditor) TerrainEditor.setBrushType(modelData.type)
                            }
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "BRUSH SETTINGS"; color: "#666"; font.pixelSize: 10; font.bold: true }

                    // Radius
                    ColumnLayout {
                        spacing: 4
                        RowLayout {
                            Text { text: "Radius:"; color: "#aaa"; width: 60 }
                            Text { text: brushRadius.toFixed(1); color: "#E10600"; font.pixelSize: 11 }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 1; to: 50
                            value: brushRadius
                            onValueChanged: {
                                terrainSculptor.brushRadius = value
                                if (TerrainEditor) TerrainEditor.setBrushRadius(value)
                            }
                        }
                    }

                    // Strength
                    ColumnLayout {
                        spacing: 4
                        RowLayout {
                            Text { text: "Strength:"; color: "#aaa"; width: 60 }
                            Text { text: (brushStrength * 100).toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 11 }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 0; to: 1
                            value: brushStrength
                            onValueChanged: {
                                terrainSculptor.brushStrength = value
                                if (TerrainEditor) TerrainEditor.setBrushStrength(value)
                            }
                        }
                    }

                    // Hardness
                    ColumnLayout {
                        spacing: 4
                        RowLayout {
                            Text { text: "Hardness:"; color: "#aaa"; width: 60 }
                            Text { text: (brushHardness * 100).toFixed(0) + "%"; color: "#E10600"; font.pixelSize: 11 }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 0; to: 1
                            value: brushHardness
                            onValueChanged: terrainSculptor.brushHardness = value
                        }
                    }

                    Item { Layout.fillHeight: true }

                    // Brush preview
                    Rectangle {
                        Layout.fillWidth: true
                        height: 128
                        radius: 4
                        color: "#1a1a1a"
                        border.color: "#444"
                        border.width: 1

                        Canvas {
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)

                                var centerX = width / 2
                                var centerY = height / 2
                                var radius = (brushRadius / 50) * (width / 2 - 10)

                                var brushColor
                                switch (brushType) {
                                    case "raise": brushColor = "rgba(0, 255, 0, 0.3)"
                                    case "lower": brushColor = "rgba(255, 0, 0, 0.3)"
                                    case "smooth": brushColor = "rgba(0, 128, 255, 0.3)"
                                    case "flatten": brushColor = "rgba(255, 255, 0, 0.3)"
                                    case "noise": brushColor = "rgba(255, 0, 255, 0.3)"
                                    case "paint": brushColor = "rgba(0, 255, 255, 0.3)"
                                    default: brushColor = "rgba(255, 255, 255, 0.3)"
                                }

                                ctx.fillStyle = brushColor
                                ctx.beginPath()
                                ctx.arc(centerX, centerY, radius, 0, Math.PI * 2)
                                ctx.fill()

                                ctx.strokeStyle = "#fff"
                                ctx.lineWidth = 2
                                ctx.beginPath()
                                ctx.arc(centerX, centerY, radius, 0, Math.PI * 2)
                                ctx.stroke()

                                ctx.fillStyle = "#fff"
                                ctx.font = "10px sans-serif"
                                ctx.textAlign = "center"
                                ctx.fillText(brushType.toUpperCase(), centerX, height - 10)
                            }
                        }
                    }
                }
            }

            // Center - Terrain viewport
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#2a2a2a"

                Canvas {
                    id: terrainCanvas
                    anchors.fill: parent
                    anchors.margins: 10

                    property var terrainData: []
                    property real zoom: 1.0
                    property real offsetX: 0
                    property real offsetY: 0

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)

                        if (terrainData.length === 0) {
                            ctx.fillStyle = "#333"
                            ctx.fillRect(0, 0, width, height)
                            ctx.fillStyle = "#666"
                            ctx.font = "14px sans-serif"
                            ctx.textAlign = "center"
                            ctx.fillText("No terrain loaded", width / 2, height / 2)
                            return
                        }

                        var cellWidth = (width * zoom) / terrainWidth
                        var cellHeight = (height * zoom) / terrainHeight

                        for (var y = 0; y < terrainHeight; y++) {
                            for (var x = 0; x < terrainWidth; x++) {
                                var idx = y * terrainWidth + x
                                var h = terrainData[idx] || 0
                                var gray = Math.floor(h * 255)
                                ctx.fillStyle = "rgb(" + gray + "," + gray + "," + gray + ")"
                                ctx.fillRect(
                                    x * cellWidth + offsetX,
                                    y * cellHeight + offsetY,
                                    cellWidth + 1,
                                    cellHeight + 1
                                )
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            terrainSculptor.isSculpting = true
                            applyBrushAt(mouseX, mouseY)
                        }
                        onReleased: terrainSculptor.isSculpting = false
                        onPositionChanged: {
                            if (terrainSculptor.isSculpting) {
                                applyBrushAt(mouseX, mouseY)
                            }
                        }

                        function applyBrushAt(mx, my) {
                            if (!TerrainEditor) return

                            var cellWidth = (terrainCanvas.width * terrainCanvas.zoom) / terrainWidth
                            var cellHeight = (terrainCanvas.height * terrainCanvas.zoom) / terrainHeight

                            var tx = (mx - terrainCanvas.offsetX) / cellWidth
                            var ty = (my - terrainCanvas.offsetY) / cellHeight

                            TerrainEditor.applyBrush(tx, ty, brushStrength * 0.1)
                            terrainCanvas.updateTerrainData()
                        }
                    }

                    function updateTerrainData() {
                        if (!TerrainEditor) return

                        var img = TerrainEditor.getHeightmapImage()
                        if (img) {
                            terrainWidth = TerrainEditor.terrainWidth
                            terrainHeight = TerrainEditor.terrainHeight
                            terrainData = []
                            for (var y = 0; y < terrainHeight; y++) {
                                for (var x = 0; x < terrainWidth; x++) {
                                    var idx = y * terrainWidth + x
                                    terrainData[idx] = TerrainEditor.getHeight(x, y)
                                }
                            }
                            terrainCanvas.requestPaint()
                        }
                    }
                }
            }

            // Right panel - Terrain info
            Rectangle {
                width: 220
                color: "#1e1e1e"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 15

                    Text { text: "TERRAIN INFO"; color: "white"; font.pixelSize: 14; font.bold: true }

                    ColumnLayout {
                        spacing: 6

                        RowLayout {
                            Text { text: "Size:"; color: "#888"; Layout.preferredWidth: 70 }
                            Text { text: terrainWidth + "x" + terrainHeight; color: "#fff"; font.pixelSize: 11 }
                        }
                        RowLayout {
                            Text { text: "Vertices:"; color: "#888"; Layout.preferredWidth: 70 }
                            Text { text: (terrainWidth * terrainHeight).toLocaleString(); color: "#fff"; font.pixelSize: 11 }
                        }
                        RowLayout {
                            Text { text: "Triangles:"; color: "#888"; Layout.preferredWidth: 70 }
                            Text { text: ((terrainWidth - 1) * (terrainHeight - 1) * 2).toLocaleString(); color: "#fff"; font.pixelSize: 11 }
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "HEIGHT STATS"; color: "white"; font.pixelSize: 14; font.bold: true }

                    ColumnLayout {
                        spacing: 6

                        RowLayout {
                            Text { text: "Min:"; color: "#888"; Layout.preferredWidth: 70 }
                            Text { text: "0.00"; color: "#27ae60"; font.pixelSize: 11 }
                        }
                        RowLayout {
                            Text { text: "Max:"; color: "#888"; Layout.preferredWidth: 70 }
                            Text { text: "1.00"; color: "#e74c3c"; font.pixelSize: 11 }
                        }
                        RowLayout {
                            Text { text: "Average:"; color: "#888"; Layout.preferredWidth: 70 }
                            Text { text: "0.50"; color: "#3498db"; font.pixelSize: 11 }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    // Export buttons
                    Button {
                        text: "Export PNG"
                        Layout.fillWidth: true
                        height: 32
                        background: Rectangle { color: "#2d2d2d"; radius: 4; border.color: "#444"; border.width: 1 }
                        contentItem: Text { text: parent.text; color: "#fff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: {
                            if (TerrainEditor) TerrainEditor.exportToPNG(Qt.resolvedUrl("terrain.png"))
                        }
                    }
                    Button {
                        text: "Export OBJ"
                        Layout.fillWidth: true
                        height: 32
                        background: Rectangle { color: "#2d2d2d"; radius: 4; border.color: "#444"; border.width: 1 }
                        contentItem: Text { text: parent.text; color: "#fff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        onClicked: {
                            if (TerrainEditor) TerrainEditor.exportToOBJ(Qt.resolvedUrl("terrain.obj"))
                        }
                    }
                }
            }
        }

        // Status bar
        Rectangle {
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text { text: isSculpting ? "Sculpting..." : "Ready"; color: "#E10600"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "Brush: " + brushType + " (r:" + brushRadius.toFixed(1) + " s:" + (brushStrength * 100).toFixed(0) + "%)"; color: "#666"; font.pixelSize: 10 }
            }
        }
    }
}
