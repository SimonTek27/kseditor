import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../widgets"
import ksEditor.FontCreator 1.0

Rectangle {
    id: root
    width: 1280
    height: 720
    color: "#121212"

    readonly property color cBg: "#121212"
    readonly property color cSurface: "#1e1e1e"
    readonly property color cBorder: "#333333"
    readonly property color cAccent: "#E10600"
    readonly property color cText: "#ffffff"
    readonly property color cMuted: "#888888"

    property string currentGlyph: "A"
    property int fontSize: FontCreator ? FontCreator.fontSize : 32
    property string fontFamily: FontCreator ? FontCreator.currentFont : "Arial"
    property string exportFormat: "BMFont"
    property string currentFile: FontCreator ? FontCreator.currentFont + ".acf" : "font.acf"
    property bool isGenerating: FontCreator ? FontCreator.isGenerating : false
    property int rightTabIndex: 0

    property var glyphSet: FontCreator ? FontCreator.getCommonCharsets() : ["ASCII", "Latin-1", "Numbers", "Alphanumeric", "Full ASCII + Symbols"]

    FileDialog {
        id: fontOpenDialog
        title: "Open Font Preset"
        nameFilters: ["Font presets (*.acf *.json)", "All files (*)"]
        onAccepted: {
            if (FontCreator) {
                var path = selectedFile.toString().replace("file:///", "")
                if (path.endsWith(".acf")) FontCreator.loadPreset(path)
                else FontCreator.importFromJSON(path)
            }
        }
    }

    FileDialog {
        id: fontSaveDialog
        title: "Save Font Preset"
        nameFilters: ["Font presets (*.acf)", "JSON files (*.json)", "All files (*)"]
        onAccepted: {
            if (FontCreator) {
                var path = selectedFile.toString().replace("file:///", "")
                if (path.endsWith(".acf")) FontCreator.savePreset(path)
                else FontCreator.exportToJSON(path)
            }
        }
    }

    FileDialog {
        id: fontExportDialog
        title: "Export Font Atlas"
        nameFilters: ["PNG files (*.png)", "All files (*)"]
        onAccepted: {
            if (FontCreator) {
                FontCreator.generateAtlas(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Toolbar ---
        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10

                AppButton {
                    text: "Open"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: fontOpenDialog.open()
                }
                AppButton {
                    text: "Save"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: fontSaveDialog.open()
                }
                Rectangle {
                    width: 1
                    height: 20
                    color: "#444444"
                }
                AppButton {
                    text: "Generate Atlas"
                    flat: true
                    height: 32
                    bgcolor: isGenerating ? "#E10600" : "transparent"
                    color: "#ffffff"
                    onClicked: fontExportDialog.open()
                }

                Rectangle { width: 1; height: 20; color: "#444444" }

                AppButton {
                    text: "Extract Kerning"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: if (FontCreator) FontCreator.extractKerning()
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: currentFile
                    color: "#aaaaaa"
                    font.pixelSize: 12
                }
            }
        }

        // --- Main Content ---
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // --- Left Panel: Glyph List ---
            Rectangle {
                width: 200
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        height: 44
                        color: "#252526"

                        Text {
                            anchors.centerIn: parent
                            text: "GLYPH SET"
                            color: "#E10600"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        color: "#252526"
                        border.color: "#333333"
                        border.width: 1
                        Row { anchors { fill: parent; margins: 8 } spacing: 6
                            Text { text: "Search glyph..."; color: "#666"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1e1e1e"

                        Flow {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4
                            Repeater {
                                model: root.glyphSet.length
                                delegate: Rectangle {
                                    width: 36; height: 36; radius: 4
                                    color: root.glyphSet[index] === root.currentGlyph ? "#E10600" : "#2d2d2d"
                                    border.color: "#444444"; border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: root.glyphSet[index]
                                        color: root.glyphSet[index] === root.currentGlyph ? "#121212" : "#ffffff"
                                        font.pixelSize: 14
                                        font.family: root.fontFamily
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: root.currentGlyph = root.glyphSet[index]
                                    }
                                }
                            }
                        }
                    }

                    // Charset selector
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: 8
                        spacing: 6

                        Repeater {
                            model: FontCreator ? FontCreator.getCommonCharsets() : ["ASCII", "Latin-1", "Full"]
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                height: 28; radius: 4
                                color: "#2d2d2d"; border.color: "#444444"; border.width: 1
                                Text { anchors.centerIn: parent; text: modelData; color: "#888888"; font.pixelSize: 11 }
                                MouseArea { anchors.fill: parent; onClicked: { if (FontCreator) FontCreator.setCharset(modelData) } }
                            }
                        }
                    }
                }
            }

            // --- Center: Glyph Editor ---
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    // Bitmap grid editor / preview
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#2a2a2a"

                        Rectangle {
                            anchors.fill: parent; anchors.margins: 24
                            color: "#1e1e1e"; border.color: "#333333"; border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: root.currentGlyph
                                color: "#E10600"
                                font.pixelSize: 120
                                font.bold: true
                                font.family: root.fontFamily
                            }
                        }
                    }

                    // Right panel: tabbed (Glyph Info / Kerning / Font Settings)
                    Rectangle {
                        width: 280
                        Layout.fillHeight: true
                        color: "#1e1e1e"
                        border.color: "#333333"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            // Tab bar
                            RowLayout {
                                Layout.fillWidth: true
                                height: 32
                                spacing: 0

                                AppButton {
                                    Layout.fillWidth: true
                                    height: 32
                                    flat: true
                                    text: "Glyph"
                                    bgcolor: root.rightTabIndex === 0 ? "#E10600" : "#252526"
                                    color: root.rightTabIndex === 0 ? "#121212" : "#ffffff"
                                    onClicked: root.rightTabIndex = 0
                                }
                                AppButton {
                                    Layout.fillWidth: true
                                    height: 32
                                    flat: true
                                    text: "Kerning"
                                    bgcolor: root.rightTabIndex === 1 ? "#E10600" : "#252526"
                                    color: root.rightTabIndex === 1 ? "#121212" : "#ffffff"
                                    onClicked: root.rightTabIndex = 1
                                }
                                AppButton {
                                    Layout.fillWidth: true
                                    height: 32
                                    flat: true
                                    text: "Settings"
                                    bgcolor: root.rightTabIndex === 2 ? "#E10600" : "#252526"
                                    color: root.rightTabIndex === 2 ? "#121212" : "#ffffff"
                                    onClicked: root.rightTabIndex = 2
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "transparent"

                                // ── Tab 0: Glyph Info ──
                                ColumnLayout {
                                    visible: root.rightTabIndex === 0
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 10

                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 70
                                        radius: 8
                                        color: "#252526"
                                        border.color: "#E10600"
                                        border.width: 1

                                        Text {
                                            anchors.centerIn: parent
                                            text: root.currentGlyph
                                            color: "#E10600"
                                            font.pixelSize: 42
                                            font.bold: true
                                        }
                                    }

                                    ColumnLayout {
                                        spacing: 5
                                        RowLayout {
                                            Text { text: "Unicode:"; color: "#888"; Layout.preferredWidth: 70 }
                                            Text { text: "U+" + root.currentGlyph.charCodeAt(0).toString(16).toUpperCase().padStart(4,"0"); color: "#ffffff"; font.pixelSize: 11 }
                                        }
                                        RowLayout {
                                            Text { text: "Width:"; color: "#888"; Layout.preferredWidth: 70 }
                                            SpinBox { Layout.fillWidth: true; from: 0; to: 256; value: 12 }
                                        }
                                        RowLayout {
                                            Text { text: "H Pad:"; color: "#888"; Layout.preferredWidth: 70 }
                                            SpinBox { Layout.fillWidth: true; from: 0; to: 50; value: 0 }
                                        }
                                        RowLayout {
                                            Text { text: "V Pad:"; color: "#888"; Layout.preferredWidth: 70 }
                                            SpinBox { Layout.fillWidth: true; from: 0; to: 50; value: 0 }
                                        }
                                    }
                                }

                                // ── Tab 1: Kerning ──
                                ColumnLayout {
                                    visible: root.rightTabIndex === 1
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 8

                                    RowLayout {
                                        spacing: 6
                                        Text { text: "Pair:"; color: "#bbb"; font.pixelSize: 11 }
                                        TextField {
                                            id: kernLeftInput
                                            Layout.preferredWidth: 30
                                            height: 24
                                            maximumLength: 1
                                            placeholderText: "A"
                                        }
                                        Text { text: "+"; color: "#888"; font.pixelSize: 11 }
                                        TextField {
                                            id: kernRightInput
                                            Layout.preferredWidth: 30
                                            height: 24
                                            maximumLength: 1
                                            placeholderText: "V"
                                        }
                                        Text { text: "="; color: "#888"; font.pixelSize: 11 }
                                        SpinBox {
                                            id: kernValueSpin
                                            Layout.preferredWidth: 60
                                            height: 24
                                            from: -20; to: 20; value: 0
                                        }
                                        AppButton {
                                            text: "Set"
                                            height: 24
                                            flat: true
                                            bgcolor: "#E10600"
                                            color: "#121212"
                                            onClicked: {
                                                if (FontCreator && kernLeftInput.text.length > 0 && kernRightInput.text.length > 0) {
                                                    FontCreator.setKerningPair(kernLeftInput.text.charCodeAt(0), kernRightInput.text.charCodeAt(0), kernValueSpin.value)
                                                }
                                            }
                                        }
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        color: "#1e1e1e"
                                        border.color: "#333"
                                        border.width: 1

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            spacing: 2

                                            Text {
                                                text: "KERNING PAIRS (" + (FontCreator ? FontCreator.getKerningPairs().length : 0) + ")"
                                                color: "#E10600"
                                                font.pixelSize: 10
                                                font.bold: true
                                            }

                                            ListView {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                clip: true
                                                spacing: 1
                                                model: FontCreator ? FontCreator.getKerningPairs() : []

                                                delegate: Rectangle {
                                                    width: parent.width
                                                    height: 28
                                                    color: index % 2 === 0 ? "#252526" : "#1e1e1e"
                                                    radius: 2

                                                    RowLayout {
                                                        anchors.fill: parent
                                                        anchors.margins: 6
                                                        spacing: 6

                                                        Text {
                                                            text: String.fromCharCode(modelData.left) + " + " + String.fromCharCode(modelData.right)
                                                            color: "white"
                                                            font.pixelSize: 12
                                                            font.bold: true
                                                            Layout.preferredWidth: 60
                                                        }
                                                        Text {
                                                            text: modelData.kerning > 0 ? "+" + modelData.kerning : "" + modelData.kerning
                                                            color: modelData.kerning > 0 ? "#4caf50" : (modelData.kerning < 0 ? "#f44336" : "#888")
                                                            font.pixelSize: 12
                                                            Layout.fillWidth: true
                                                        }
                                                        SpinBox {
                                                            Layout.preferredWidth: 60
                                                            height: 22
                                                            from: -20; to: 20
                                                            value: modelData.kerning
                                                            onValueChanged: {
                                                                if (FontCreator) {
                                                                    FontCreator.setKerningPair(modelData.left, modelData.right, value)
                                                                }
                                                            }
                                                        }
                                                        AppButton {
                                                            text: "X"
                                                            height: 22
                                                            width: 24
                                                            flat: true
                                                            bgcolor: "transparent"
                                                            color: "#f44336"
                                                            onClicked: {
                                                                if (FontCreator) FontCreator.removeKerningPair(modelData.left, modelData.right)
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    RowLayout {
                                        spacing: 6
                                        AppButton {
                                            text: "Extract"
                                            height: 26
                                            flat: true
                                            bgcolor: "#3e3e42"
                                            color: "#ffffff"
                                            onClicked: if (FontCreator) FontCreator.extractKerning()
                                        }
                                        AppButton {
                                            text: "Clear All"
                                            height: 26
                                            flat: true
                                            bgcolor: "#3e3e42"
                                            color: "#ffffff"
                                            onClicked: if (FontCreator) FontCreator.clearKerningPairs()
                                        }
                                    }
                                }

                                // ── Tab 2: Settings ──
                                ColumnLayout {
                                    visible: root.rightTabIndex === 2
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 12

                                    Text { text: "FONT SETTINGS"; color: "white"; font.pixelSize: 14; font.bold: true }

                                    ColumnLayout {
                                        spacing: 4
                                        RowLayout {
                                            Text { text: "Size:"; color: "#888"; Layout.preferredWidth: 50 }
                                            Text { text: (FontCreator ? FontCreator.fontSize : root.fontSize) + "px"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                        Slider {
                                            Layout.fillWidth: true
                                            from: 8; to: 128
                                            value: FontCreator ? FontCreator.fontSize : root.fontSize
                                            onValueChanged: { if (FontCreator) FontCreator.fontSize = value }
                                        }
                                    }

                                    Rectangle { height: 5 }

                                    Text { text: "UNICODE RANGES"; color: "white"; font.pixelSize: 14; font.bold: true }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        color: "#1e1e1e"
                                        border.color: "#333"
                                        border.width: 1
                                        clip: true

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 4

                                            Repeater {
                                                model: FontCreator ? FontCreator.getAvailableRanges() : []

                                                delegate: RowLayout {
                                                    Layout.fillWidth: true
                                                    height: 24
                                                    spacing: 6

                                                    CheckBox {
                                                        id: rangeCheck
                                                        checked: {
                                                            var ranges = FontCreator ? FontCreator.getEnabledRanges() : []
                                                            return ranges.indexOf(modelData) >= 0
                                                        }
                                                        onCheckedChanged: {
                                                            if (FontCreator)
                                                                FontCreator.enableRange(modelData, checked)
                                                        }
                                                    }

                                                    Text {
                                                        text: modelData
                                                        color: rangeCheck.checked ? "#E10600" : "#bbb"
                                                        font.pixelSize: 11
                                                        Layout.fillWidth: true
                                                    }
                                                }
                                            }

                                            Item { Layout.fillHeight: true }

                                            RowLayout {
                                                spacing: 6
                                                AppButton {
                                                    text: "Apply Ranges"
                                                    height: 26
                                                    flat: true
                                                    bgcolor: "#E10600"
                                                    color: "#121212"
                                                    onClicked: {
                                                        if (FontCreator) FontCreator.applyCombinedCharset()
                                                    }
                                                }
                                                AppButton {
                                                    text: "Clear"
                                                    height: 26
                                                    flat: true
                                                    bgcolor: "#3e3e42"
                                                    color: "#ffffff"
                                                    onClicked: {
                                                        if (FontCreator) FontCreator.clearRanges()
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    Item { Layout.fillHeight: true }

                                    Text { text: "EXPORT FORMAT"; color: "white"; font.pixelSize: 14; font.bold: true }

                                    ColumnLayout {
                                        spacing: 4
                                        Repeater {
                                            model: ["BMFont", "JSON", "PNG", "AC INI"]
                                            delegate: Rectangle {
                                                Layout.fillWidth: true; height: 28; radius: 4
                                                color: root.exportFormat === modelData ? "#E10600" : "#2d2d2d"
                                                border.color: "#444444"; border.width: 1
                                                Text { anchors.centerIn: parent; text: modelData; color: root.exportFormat === modelData ? "#121212" : "#ffffff"; font.pixelSize: 11 }
                                                MouseArea { anchors.fill: parent; onClicked: root.exportFormat = modelData }
                                            }
                                        }
                                    }

                                    Item { Layout.fillHeight: true }

                                    AppButton {
                                        height: 36
                                        text: "Export " + root.exportFormat
                                        bgcolor: "#E10600"
                                        color: "#121212"
                                        onClicked: { if (FontCreator) FontCreator.generateAtlas(Qt.resolvedUrl("font_atlas.png")) }
                                    }
                                }
                            }
                        }
                    }
                }

                // Bottom: kerning preview + atlas stats
                Rectangle {
                    Layout.fillWidth: true
                    height: 100
                    color: "#1e1e1e"
                    border.color: "#333333"
                    border.width: 1

                    RowLayout {
                        anchors { fill: parent; margins: 12 }
                        spacing: 20

                        ColumnLayout {
                            spacing: 4
                            Text { text: "KERNING PREVIEW"; color: "white"; font.pixelSize: 12; font.bold: true }
                            Rectangle {
                                width: 500; height: 50; radius: 4
                                color: "#0a0a0a"; border.color: "#333333"; border.width: 1
                                Row {
                                    anchors.centerIn: parent
                                    spacing: -1
                                    Repeater {
                                        model: "AVATAR"
                                        delegate: Text {
                                            text: modelData
                                            color: "#E10600"
                                            font.pixelSize: 28
                                            font.bold: true
                                            font.family: root.fontFamily
                                            font.letterSpacing: {
                                                if (index > 0 && FontCreator) {
                                                    var l = root.glyphSet.indexOf("AVATAR"[index-1])
                                                    var r = root.glyphSet.indexOf(modelData)
                                                    return -FontCreator.getKerningOffset("AVATAR"[index-1].charCodeAt(0), modelData.charCodeAt(0)) / 10.0
                                                }
                                                return 0
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            spacing: 6
                            Text { text: "STATS"; color: "#666"; font.pixelSize: 10 }
                            Text { text: "Glyphs: " + (FontCreator ? FontCreator.getGlyphs().length : 95); color: "#bbb"; font.pixelSize: 11 }
                            Text { text: "Kerning: " + (FontCreator ? FontCreator.getKerningPairs().length : 0) + " pairs"; color: "#bbb"; font.pixelSize: 11 }
                            Text { text: "Atlas: " + (FontCreator ? FontCreator.atlasWidth : 4096) + "\u00D7" + (FontCreator ? FontCreator.atlasHeight : 64); color: "#bbb"; font.pixelSize: 11 }
                        }
                    }
                }
            }
        }

        // --- Status Bar ---
        Rectangle {
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                Text { text: FontCreator && FontCreator.statusMessage ? FontCreator.statusMessage : "Ready"; color: "#E10600"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "ksEditor v1.0 - Font"; color: "#666"; font.pixelSize: 10 }
            }
        }
    }
}

