import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window 2.15
import "../../widgets"
import ksEditor.TexturePainter 1.0

Rectangle {
    id: paintPanel
    width: 300
    height: 600
    color: "#1e1e1e"
    border.color: "#333"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        spacing: 2

        // Header
        Rectangle {
            height: 28; Layout.fillWidth: true; color: "#252526"
            Text { anchors.centerIn: parent; text: "TEXTURE PAINT"; color: "#E10600"; font.pixelSize: 11; font.bold: true }
        }

        ScrollView { Layout.fillWidth: true; Layout.fillHeight: true
            ColumnLayout {
                width: parent.width; spacing: 4

                // Canvas info
                Text { text: "Canvas: " + TexturePainter.canvasWidth + "x" + TexturePainter.canvasHeight; color: "#888"; font.pixelSize: 9; leftPadding: 8 }

                // Brush type
                Text { text: "BRUSH"; color: "#888"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                ComboBox {
                    id: brushTypeCombo; Layout.fillWidth: true; height: 22
                    model: TexturePainter.brushTypeNames()
                    currentIndex: TexturePainter.brushType
                    onActivated: TexturePainter.setBrushType(index)
                }

                // Size
                RowLayout { Layout.fillWidth: true
                    Text { text: "Size"; color: "#aaa"; font.pixelSize: 9; width: 40 }
                    Slider { Layout.fillWidth: true; height: 18; from: 1; to: 500; value: TexturePainter.brushSize; onMoved: TexturePainter.setBrushSize(value) }
                    Text { text: TexturePainter.brushSize; color: "#ccc"; font.pixelSize: 9; width: 35 }
                }

                // Strength
                RowLayout { Layout.fillWidth: true
                    Text { text: "Strength"; color: "#aaa"; font.pixelSize: 9; width: 40 }
                    Slider { Layout.fillWidth: true; height: 18; from: 0; to: 1; value: TexturePainter.brushStrength; onMoved: TexturePainter.setBrushStrength(value) }
                    Text { text: TexturePainter.brushStrength.toFixed(2); color: "#ccc"; font.pixelSize: 9; width: 35 }
                }

                // Hardness
                RowLayout { Layout.fillWidth: true
                    Text { text: "Hardness"; color: "#aaa"; font.pixelSize: 9; width: 40 }
                    Slider { Layout.fillWidth: true; height: 18; from: 0; to: 1; value: TexturePainter.brushHardness; onMoved: TexturePainter.setBrushHardness(value) }
                    Text { text: TexturePainter.brushHardness.toFixed(2); color: "#ccc"; font.pixelSize: 9; width: 35 }
                }

                // Color
                Text { text: "COLOR"; color: "#888"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                RowLayout { Layout.fillWidth: true
                    Rectangle {
                        width: 28; height: 28; radius: 3
                        color: TexturePainter.brushColor
                        border.color: "#555"; border.width: 1
                        MouseArea {
                            anchors.fill: parent
                            onClicked: colorDialog.open()
                        }
                    }
                    ColorDialog {
                        id: colorDialog
                        title: "Choose Brush Color"
                        selectedColor: TexturePainter.brushColor
                        onAccepted: TexturePainter.setBrushColor(selectedColor)
                    }
                    KsButton { text: "Black"; height: 22; bgcolor: "#3e3e42"; color: "#fff"
                        onClicked: TexturePainter.setBrushColor("#000000") }
                    KsButton { text: "White"; height: 22; bgcolor: "#3e3e42"; color: "#fff"
                        onClicked: TexturePainter.setBrushColor("#ffffff") }
                }

                Rectangle { height: 6; color: "transparent" }

                // Layers
                Text { text: "LAYERS (" + TexturePainter.layerCount + ")"; color: "#888"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                ListView {
                    id: layerList; Layout.fillWidth: true; height: 120
                    model: TexturePainter.layerCount
                    clip: true
                    spacing: 1
                    currentIndex: TexturePainter.currentLayer
                    delegate: Rectangle {
                        height: 28; width: layerList.width; color: ListView.isCurrentItem ? "#3a3a3e" : "#252526"
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 4; spacing: 2
                            Rectangle {
                                width: 24; height: 24; color: "transparent"; border.color: "#555"
                                Image {
                                    anchors.fill: parent
                                    source: TexturePainter.layerImage(index)
                                    fillMode: Image.PreserveAspectFit
                                }
                            }
                            Text {
                                text: {
                                    var names = TexturePainter.layerNames()
                                    return index < names.length ? names[index] : "Layer " + (index + 1)
                                }
                                color: "#ccc"; font.pixelSize: 9; Layout.fillWidth: true
                            }
                            KsButton { text: "V"; height: 18; width: 22; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 8
                                onClicked: TexturePainter.setLayerVisible(index, !(true)) }
                        }
                        MouseArea { anchors.fill: parent; onClicked: TexturePainter.setCurrentLayer(index) }
                    }
                }

                RowLayout { Layout.fillWidth: true
                    KsButton { text: "+"; height: 22; bgcolor: "#3e3e42"; color: "#fff"
                        onClicked: TexturePainter.addLayer() }
                    KsButton { text: "-"; height: 22; bgcolor: "#3e3e42"; color: "#fff"
                        onClicked: TexturePainter.removeLayer(TexturePainter.currentLayer) }
                    KsButton { text: "Up"; height: 22; bgcolor: "#3e3e42"; color: "#fff"
                        onClicked: TexturePainter.moveLayer(TexturePainter.currentLayer, TexturePainter.currentLayer - 1) }
                    KsButton { text: "Dn"; height: 22; bgcolor: "#3e3e42"; color: "#fff"
                        onClicked: TexturePainter.moveLayer(TexturePainter.currentLayer, TexturePainter.currentLayer + 1) }
                }

                // Blend mode
                Text { text: "BLEND MODE"; color: "#888"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                ComboBox {
                    Layout.fillWidth: true; height: 22
                    model: TexturePainter.blendModeNames()
                    currentIndex: {
                        var names = TexturePainter.blendModeNames()
                        return 0 // Normal
                    }
                    onActivated: TexturePainter.setLayerBlendMode(TexturePainter.currentLayer, index)
                }

                Rectangle { height: 6; color: "transparent" }

                // Filters
                Text { text: "FILTERS"; color: "#888"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                Flow { Layout.fillWidth: true; spacing: 3
                    KsButton { text: "Blur"; height: 22; bgcolor: "#3e3e42"; color: "#fff"; onClicked: TexturePainter.applyBlur(5) }
                    KsButton { text: "Sharpen"; height: 22; bgcolor: "#3e3e42"; color: "#fff"; onClicked: TexturePainter.applySharpen(0.5) }
                    KsButton { text: "Noise"; height: 22; bgcolor: "#3e3e42"; color: "#fff"; onClicked: TexturePainter.applyNoise(0.1) }
                    KsButton { text: "Emboss"; height: 22; bgcolor: "#3e3e42"; color: "#fff"; onClicked: TexturePainter.applyEmboss() }
                    KsButton { text: "Invert"; height: 22; bgcolor: "#3e3e42"; color: "#fff"; onClicked: TexturePainter.applyInvert() }
                }

                // Undo/Redo
                Text { text: "UNDO"; color: "#888"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                RowLayout { Layout.fillWidth: true
                    KsButton { id: undoBtn; text: "Undo"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; enabled: false
                        onClicked: TexturePainter.undo() }
                    KsButton { id: redoBtn; text: "Redo"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"; enabled: false
                        onClicked: TexturePainter.redo() }
                }

                Connections { target: TexturePainter; onUndoStackChanged: { undoBtn.enabled = TexturePainter.canUndo(); redoBtn.enabled = TexturePainter.canRedo() } }
                Component.onCompleted: { undoBtn.enabled = TexturePainter.canUndo(); redoBtn.enabled = TexturePainter.canRedo() }

                // Fill tools
                Text { text: "FILL"; color: "#888"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                RowLayout { Layout.fillWidth: true
                    KsButton { text: "Flood Fill"; height: 22; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"
                        onClicked: { /* click on canvas to flood fill */ } }
                    KsButton { text: "Clear Canvas"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"
                        onClicked: TexturePainter.clearCanvas() }
                }

                // Load/Save
                Text { text: "FILE"; color: "#888"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                RowLayout { Layout.fillWidth: true
                    KsButton { text: "Load"; height: 22; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#fff"
                        onClicked: loadDialog.open() }
                    KsButton { text: "Save"; height: 22; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"
                        onClicked: saveDialog.open() }
                }

                Item { height: 10 }
            }
        }
    }

    FileDialog {
        id: loadDialog; title: "Load Texture"
        nameFilters: ["Images (*.png *.jpg *.bmp *.tga)"]
        onAccepted: TexturePainter.loadTexture(selectedFile.toString().replace("file:///", ""))
    }
    FileDialog {
        id: saveDialog; title: "Save Texture"
        nameFilters: ["PNG (*.png)", "JPEG (*.jpg)"]
        onAccepted: TexturePainter.saveTexture(selectedFile.toString().replace("file:///", ""))
    }
}

