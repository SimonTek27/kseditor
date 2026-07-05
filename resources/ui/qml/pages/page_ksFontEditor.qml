import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import ksEditor.FontCreator 1.0

// ksFontEditor - interfaccia stile Ribbon per editing bitmap/texture fonts
// Connected to FontCreatorQmlBridge C++ backend

Rectangle {
    id: pageKsFontEditor
    width: 1280
    height: 720
    color: "#111111"

    // API base verso backend
    property alias glyphView: glyphGrid
    property alias fontListView: fontList
    property alias zoomSlider: zoomSlider
    property alias boldToggle: btnBold
    property alias italicToggle: btnItalic

    property string currentFontName: FontCreator.currentFont
    property string currentFilePath: ""

    Connections {
        target: FontCreator
        onCurrentFontChanged: { currentFontName = FontCreator.currentFont; }
        onStatusMessageChanged: { statusText.text = FontCreator.statusMessage; }
        onAtlasGenerated: { statusText.text = "Atlas generated: " + pngPath; }
        onPresetLoaded: { statusText.text = "Preset loaded: " + acfPath; }
        onPresetSaved: { statusText.text = "Preset saved: " + acfPath; }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Ribbon: Tab "Font" attivo ---
        Rectangle {
            height: 60
            color: "#1b1b1b"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4

                // Tab strip (solo Font attivo per ora)
                RowLayout {
                    spacing: 6
                    Rectangle {
                        height: 22
                        width: 70
                        radius: 3
                        color: "#E10600"
                        Text {
                            anchors.centerIn: parent
                            text: "Font"
                            color: "#111111"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                }

                // Ribbon content (gruppi di comandi)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    // Gruppo File
                    ColumnLayout {
                        spacing: 2
                        Text { text: "File"; color: "#cccccc"; font.pixelSize: 10 }
                        RowLayout {
                            spacing: 4
                            Button { text: "New"; width: 60; height: 24 }
                            Button { text: "Open"; width: 60; height: 24 }
                            Button { text: "Save"; width: 60; height: 24 }
                        }
                    }

                    // Gruppo Glyph
                    ColumnLayout {
                        spacing: 2
                        Text { text: "Glyph"; color: "#cccccc"; font.pixelSize: 10 }
                        RowLayout {
                            spacing: 4
                            Button { text: "Add"; width: 60; height: 24 }
                            Button { text: "Remove"; width: 70; height: 24 }
                            Button { text: "Auto"; width: 60; height: 24 }
                        }
                    }

                    // Gruppo Style
                    ColumnLayout {
                        spacing: 2
                        Text { text: "Style"; color: "#cccccc"; font.pixelSize: 10 }
                        RowLayout {
                            spacing: 4
                            Button { id: btnBold; text: "B"; checkable: true; width: 32; height: 24 }
                            Button { id: btnItalic; text: "I"; checkable: true; width: 32; height: 24 }
                            Button { text: "Outline"; checkable: true; width: 70; height: 24 }
                        }
                    }

                    // Gruppo Metrics
                    ColumnLayout {
                        spacing: 2
                        Text { text: "Metrics"; color: "#cccccc"; font.pixelSize: 10 }
                        GridLayout {
                            columns: 2
                            rowSpacing: 2
                            columnSpacing: 4
                            Text { text: "Size"; color: "#aaaaaa"; font.pixelSize: 10 }
                            SpinBox {
                                id: fontSizeSpin
                                from: 4; to: 256; value: FontCreator.fontSize
                                editable: true
                                onValueChanged: FontCreator.setFontSize(value)
                            }
                            Text { text: "Spacing"; color: "#aaaaaa"; font.pixelSize: 10 }
                            SpinBox { from: -10; to: 50; value: 0; editable: true }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: currentFontName
                        color: "#aaaaaa"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // --- Corpo principale ---
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Sinistra: elenco font/progetti
            Rectangle {
                width: 220
                Layout.fillHeight: true
                color: "#181818"
                border.color: "#262626"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    Text {
                        text: "FONTS"
                        color: "#ffffff"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    ListView {
                        id: fontList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: ["ksHUD", "ksTiming", "ksMenu", "CustomFont"]

                        delegate: Rectangle {
                            height: 22
                            width: parent.width
                            color: ListView.isCurrentItem ? "#225566" : "transparent"

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 6
                                text: modelData
                                color: "#dddddd"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            // Centro: editor glifi
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#101010"
                border.color: "#202020"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    // Info/zoom
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: "Glyphs"; color: "#ffffff"; font.pixelSize: 12; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Text { text: "Zoom"; color: "#aaaaaa"; font.pixelSize: 10 }
                        Slider { id: zoomSlider; width: 150; from: 1; to: 16; value: 4 }
                    }

                    // Generation controls
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Button {
                            text: "Generate Atlas"
                            height: 28
                            bgcolor: "#E10600"
                            color: "#111111"
                            enabled: !FontCreator.isGenerating
                            onClicked: FontCreator.generateAtlas("font_atlas.png")
                        }
                        Button {
                            text: "Load Preset"
                            height: 28
                            bgcolor: "transparent"
                            color: "#ffffff"
                            onClicked: {
                                // In a real app, open a file dialog
                                FontCreator.loadPreset("font.preset.acf")
                            }
                        }
                        Button {
                            text: "Save Preset"
                            height: 28
                            bgcolor: "transparent"
                            color: "#ffffff"
                            onClicked: {
                                FontCreator.savePreset("font.preset.acf")
                            }
                        }
                        Button {
                            text: "Export JSON"
                            height: 28
                            bgcolor: "transparent"
                            color: "#ffffff"
                            onClicked: {
                                FontCreator.exportToJSON("font.json")
                            }
                        }
                        Item { Layout.fillWidth: true }
                        ProgressBar {
                            id: progressBar
                            width: 200
                            visible: FontCreator.isGenerating
                            from: 0
                            to: 100
                            value: 0
                        }
                    }

                    // Griglia glifi (preview)
                    GridView {
                        id: glyphGrid
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        cellWidth: 64
                        cellHeight: 64
                        model: 96  // placeholder: ASCII da 32 a 127

                        delegate: Rectangle {
                            width: 60
                            height: 60
                            color: GridView.isCurrentItem ? "#1f3b5d" : "#151515"
                            border.color: "#333333"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 2
                                spacing: 2

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    color: "#000000"
                                    border.color: "#444444"
                                    radius: 2

                                    Text {
                                        anchors.centerIn: parent
                                        text: String.fromCharCode(32 + index)
                                        color: "#ffffff"
                                        font.pixelSize: 20
                                    }
                                }

                                Text {
                                    text: "U+" + (32 + index).toString(16).toUpperCase()
                                    color: "#aaaaaa"
                                    font.pixelSize: 8
                                    horizontalAlignment: Text.AlignHCenter
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }

            // Destra: dettagli glifo selezionato
            Rectangle {
                width: 260
                Layout.fillHeight: true
                color: "#181818"
                border.color: "#262626"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Text {
                        text: "GLYPH PROPERTIES"
                        color: "#ffffff"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 4
                        rowSpacing: 2

                        Text { text: "Char"; color: "#bbbbbb"; font.pixelSize: 10 }
                        TextField { text: "A"; font.pixelSize: 10 }

                        Text { text: "Code"; color: "#bbbbbb"; font.pixelSize: 10 }
                        TextField { text: "0x0041"; font.pixelSize: 10 }

                        Text { text: "Advance"; color: "#bbbbbb"; font.pixelSize: 10 }
                        SpinBox { from: 0; to: 512; value: 16; editable: true }

                        Text { text: "Left Bearing"; color: "#bbbbbb"; font.pixelSize: 10 }
                        SpinBox { from: -64; to: 64; value: 0; editable: true }

                        Text { text: "Right Bearing"; color: "#bbbbbb"; font.pixelSize: 10 }
                        SpinBox { from: -64; to: 64; value: 0; editable: true }
                    }

                    Rectangle { height: 1; Layout.fillWidth: true; color: "#333333" }

                    Text { text: "Preview"; color: "#cccccc"; font.pixelSize: 10 }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        color: "#000000"
                        border.color: "#444444"

                        Text {
                            anchors.centerIn: parent
                            text: "Sample Text"
                            color: "#ffffff"
                            font.pixelSize: 20
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // --- Status bar ---
        Rectangle {
            height: 22
            color: "#181818"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text {
                    id: statusText
                    text: "Font editor ready"
                    color: "#10b981"
                    font.pixelSize: 10
                }
                Item { Layout.fillWidth: true }
                Text { text: currentFilePath; color: "#777777"; font.pixelSize: 10; elide: Text.ElideLeft }
            }
        }
    }
}
