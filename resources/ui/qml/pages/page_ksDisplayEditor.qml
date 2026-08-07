import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../widgets"
import ksEditor.DisplayEditor 1.0

Rectangle {
    id: root
    color: "#121212"

    property real baseUiScale: 1.18
    property real uiZoom: 1.0
    property real uiScale: baseUiScale * uiZoom

    readonly property color cBg: "#121212"
    readonly property color cSurface: "#1e1e1e"
    readonly property color cBorder: "#333333"
    readonly property color cAccent: "#E10600"
    readonly property color cText: "#ffffff"
    readonly property color cMuted: "#888888"

    property string displayType: "7seg"
    property string previewText: "88.8"
    property int segColumns: 4
    property int segRows: 1
    property color segOnColor: "#E10600"
    property color segOffColor: "#1a1a1a"
    property string currentChar: "8"
    property string currentFile: DisplayEditor ? DisplayEditor.currentFile : "display.ini"
    property int elementCount: DisplayEditor ? DisplayEditor.elementCount : 0
    property bool showGrid: true
    property bool showGuides: true
    property real canvasZoom: 1.0

    FileDialog {
        id: displayOpenDialog
        title: "Open Display"
        nameFilters: ["Display files (*.ini *.lua *.json)", "All files (*)"]
        onAccepted: {
            if (DisplayEditor) {
                DisplayEditor.loadFromFile(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: displaySaveDialog
        title: "Save Display"
        nameFilters: ["INI files (*.ini)", "Lua files (*.lua)", "JSON files (*.json)", "All files (*)"]
        onAccepted: {
            if (DisplayEditor) {
                DisplayEditor.saveToFile(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: exportImageDialog
        title: "Export as Image"
        nameFilters: ["PNG files (*.png)", "All files (*)"]
        onAccepted: {
            if (DisplayEditor) {
                var err = DisplayEditor.exportAsImage(selectedFile.toString().replace("file:///", ""))
                if (err !== "") {
                    console.log("Export failed:", err)
                }
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

        // --- Toolbar ---
        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10

                AppButton {
                    text: "New"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: {
                        if (DisplayEditor) DisplayEditor.clearElements()
                    }
                }
                AppButton {
                    text: "Open"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: displayOpenDialog.open()
                }
                AppButton {
                    text: "Save"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: displaySaveDialog.open()
                }
                AppButton {
                    text: "Export Image"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: exportImageDialog.open()
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: elementCount + " elements | " + currentFile
                    color: "#aaaaaa"
                    font.pixelSize: 12
                }
            }
        }

        // --- Main Content ---
        RowLayout {
            anchors.fill: parent
            spacing: 0

            // --- Left: display type + segment palette ---
            Rectangle {
                width: 220
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors { fill: parent; margins: 15 }
                    spacing: 15

                    Text {
                        text: "DISPLAY EDITOR"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 14
                    }

                    // Display type selector
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "DISPLAY TYPE"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 12
                        }

                        AppButton {
                            height: 28
                            text: "7-Segment"
                            bgcolor: displayType === "7seg" ? "#E10600" : "#3e3e42"
                            color: displayType === "7seg" ? "#121212" : "#ffffff"
                            onClicked: { root.displayType = "7seg"; if (DisplayEditor) DisplayEditor.setDisplayName("7-Segment Display") }
                        }
                        AppButton {
                            height: 28
                            text: "14-Segment"
                            bgcolor: displayType === "14seg" ? "#E10600" : "#3e3e42"
                            color: displayType === "14seg" ? "#121212" : "#ffffff"
                            onClicked: { root.displayType = "14seg"; if (DisplayEditor) DisplayEditor.setDisplayName("14-Segment Display") }
                        }
                        AppButton {
                            height: 28
                            text: "16-Segment"
                            bgcolor: displayType === "16seg" ? "#E10600" : "#3e3e42"
                            color: displayType === "16seg" ? "#121212" : "#ffffff"
                            onClicked: { root.displayType = "16seg"; if (DisplayEditor) DisplayEditor.setDisplayName("16-Segment Display") }
                        }
                        AppButton {
                            height: 28
                            text: "LCD Matrix"
                            bgcolor: displayType === "lcd" ? "#E10600" : "#3e3e42"
                            color: displayType === "lcd" ? "#121212" : "#ffffff"
                            onClicked: { root.displayType = "lcd"; if (DisplayEditor) DisplayEditor.setDisplayName("LCD Matrix Display") }
                        }
                    }

                    Rectangle { height: 10 }

                    // Preview text
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "PREVIEW"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 12
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 40
                            radius: 4
                            color: "#252526"
                            border.color: "#333333"
                            border.width: 1

                            Text {
                                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                                text: root.previewText
                                color: "#ffffff"
                                font.pixelSize: 18
                            }
                        }
                    }

                    // Color controls
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "SEGMENT COLOUR"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 12
                        }

                        RowLayout {
                            Text { text: "ON:"; color: "#888888"; width: 30 }
                            Rectangle {
                                id: segOnColorRect; width: 24; height: 24; radius: 4; color: segOnColor
                                border.color: "#555"; border.width: 1
                                MouseArea { anchors.fill: parent; onClicked: segOnColorDialog.open() }
                            }
                            ColorDialog { id: segOnColorDialog; color: segOnColor; onAccepted: segOnColor = color.toString() }
                        }

                        RowLayout {
                            Text { text: "OFF:"; color: "#888888"; width: 30 }
                            Rectangle {
                                id: segOffColorRect; width: 24; height: 24; radius: 4; color: segOffColor
                                border.color: "#555"; border.width: 1
                                MouseArea { anchors.fill: parent; onClicked: segOffColorDialog.open() }
                            }
                            ColorDialog { id: segOffColorDialog; color: segOffColor; onAccepted: segOffColor = color.toString() }
                        }
                    }

                    // Character set
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "CHARACTER SET"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 12
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 80
                            radius: 4
                            color: "#252526"
                            border.color: "#333333"
                            border.width: 1

                            Text {
                                anchors { fill: parent; margins: 8 }
                                text: "0123456789\n.,-+/\\°%"
                                color: "#888888"
                                font.pixelSize: 12
                                wrapMode: Text.Wrap
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    // Export
                    AppButton {
                        height: 36
                        text: "Export DDS + INI"
                        bgcolor: "#E10600"
                        color: "#121212"
                        onClicked: { if (DisplayEditor) DisplayEditor.saveToFile(currentFile) }
                    }
                }
            }

            // --- Center: segment display canvas ---
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Canvas toolbar
                Rectangle {
                    height: 40
                    color: "#1e1e1e"
                    border.color: "#333333"
                    border.width: 1

                    RowLayout {
                        anchors { fill: parent; margins: 10 }
                        spacing: 10

                        Text {
                            text: "DISPLAY CANVAS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 12
                        }

                        Item { Layout.fillWidth: true }

                        AppButton {
                            height: 28
                            text: "Grid"
                            bgcolor: showGrid ? "#E10600" : "transparent"
                            color: showGrid ? "#121212" : "#ffffff"
                            onClicked: showGrid = !showGrid
                        }
                        AppButton {
                            height: 28
                            text: "Guides"
                            bgcolor: showGuides ? "#E10600" : "transparent"
                            color: showGuides ? "#121212" : "#ffffff"
                            onClicked: showGuides = !showGuides
                        }
                        AppButton {
                            height: 28
                            text: "Zoom +"
                            bgcolor: "transparent"
                            color: "#ffffff"
                            onClicked: canvasZoom = Math.min(canvasZoom + 0.25, 4.0)
                        }
                        AppButton {
                            height: 28
                            text: "Zoom -"
                            bgcolor: "transparent"
                            color: "#ffffff"
                            onClicked: canvasZoom = Math.max(canvasZoom - 0.25, 0.25)
                        }
                    }
                }

                // Canvas
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#0a0a0a"

                    // Simulated 7-segment display
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 16

                        Repeater {
                            model: root.previewText.length
                            delegate: Item {
                                width: 60
                                height: 90

                                property string ch: root.previewText[index] || " "

                                property var segsOn: {
                                    var map = {
                                        "0": "abcdef",
                                        "1": "bc",
                                        "2": "abdeg",
                                        "3": "abcdg",
                                        "4": "bcfg",
                                        "5": "acdfg",
                                        "6": "acdefg",
                                        "7": "abc",
                                        "8": "abcdefg",
                                        "9": "abcdfg",
                                        ".": ".",
                                        "-": "g"
                                    };
                                    return (map[ch] || "").split("");
                                }

                                function seg(s) { return segsOn.indexOf(s) >= 0 ? root.segOnColor : root.segOffColor }

                                Rectangle { x: 8; y: 2; width: 44; height: 8; radius: 3; color: parent.seg("a") }
                                Rectangle { x: 52; y: 8; width: 8; height: 34; radius: 3; color: parent.seg("b") }
                                Rectangle { x: 52; y: 48; width: 8; height: 34; radius: 3; color: parent.seg("c") }
                                Rectangle { x: 8; y: 80; width: 44; height: 8; radius: 3; color: parent.seg("d") }
                                Rectangle { x: 0; y: 48; width: 8; height: 34; radius: 3; color: parent.seg("e") }
                                Rectangle { x: 0; y: 8; width: 8; height: 34; radius: 3; color: parent.seg("f") }
                                Rectangle { x: 8; y: 41; width: 44; height: 8; radius: 3; color: parent.seg("g") }
                            }
                        }
                    }

                    Text {
                        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 16 }
                        text: "Click digits in the bottom panel to edit individual glyphs"
                        color: "#666"
                        font.pixelSize: 11
                    }
                }

                // Bottom: character map
                Rectangle {
                    height: 80
                    color: "#1e1e1e"
                    border.color: "#333333"
                    border.width: 1

                    RowLayout {
                        anchors { fill: parent; margins: 10 }
                        spacing: 4

                        Text { text: "Chars:"; color: "#888888" }

                        Repeater {
                            model: "0123456789.-+/ °%ABCDEFHIJLNOPSTU"
                            delegate: Rectangle {
                                width: 28
                                height: 44
                                radius: 4
                                color: root.currentChar === modelData[index] ? "#E10600" + "33" : "#2d2d2d"
                                border.color: root.currentChar === modelData[index] ? "#E10600" : "#444444"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData[index] || ""
                                    color: root.currentChar === modelData[index] ? "#121212" : "#ffffff"
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        root.currentChar = modelData[index];
                                        root.previewText = modelData[index];
                                    }
                                }
                            }
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

                Text { text: "Ready"; color: "#E10600"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "ksEditor v1.0 - Display"; color: "#666"; font.pixelSize: 10 }
            }
        }
    }
    }
}
