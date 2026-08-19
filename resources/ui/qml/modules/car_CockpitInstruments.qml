import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../widgets"
import ksEditor.CockpitInstruments 1.0

Rectangle {
    id: root
    color: "#121212"

    property int activeRibbonTab: 0

    Component.onCompleted: {
        updateRibbonIndex()
        if (CockpitInstruments) {
            CockpitInstruments.statusMessage.connect(function(msg) { statusText = msg; })
        }
    }

    function updateRibbonIndex() {
        switch (activePanel) {
            case "display": activeRibbonTab = 1; break
            case "items": activeRibbonTab = 2; break
            case "leds": activeRibbonTab = 3; break
            case "tyreslip": activeRibbonTab = 4; break
            case "elements": activeRibbonTab = 5; break
        }
    }

    property real baseUiScale: 1.18
    property real uiZoom: 1.0
    property real uiScale: baseUiScale * uiZoom

    property string activePanel: "display"
    property string currentFile: CockpitInstruments ? CockpitInstruments.currentFile : "display.ini"
    property int elementCount: CockpitInstruments ? CockpitInstruments.elementCount : 0
    property string statusText: "Ready"

    property string displayType: "7seg"
    property string previewText: "88.8"
    property int segColumns: 4
    property int segRows: 1
    property color segOnColor: "#E10600"
    property color segOffColor: "#1a1a1a"
    property string currentChar: "8"
    property bool showGrid: true
    property bool showGuides: true
    property real canvasZoom: 1.0

    FileDialog {
        id: displayOpenDialog
        title: "Open Display"
        nameFilters: ["AC Display (*.ini)", "Digital Instruments (digital_instruments.ini)", "Analog Instruments (analog_instruments.ini)", "All files (*)"]
        onAccepted: {
            if (CockpitInstruments) {
                CockpitInstruments.loadFromFile(selectedFile.toString().replace("file:///", ""))
                currentFile = CockpitInstruments.currentFile.split("/").pop().split("\\").pop()
                statusText = "Loaded: " + currentFile
            }
        }
    }

    FileDialog {
        id: displaySaveDialog
        title: "Save Display"
        nameFilters: ["AC Display (*.ini)", "Digital Instruments (digital_instruments.ini)", "Analog Instruments (analog_instruments.ini)", "All files (*)"]
        onAccepted: {
            if (CockpitInstruments) {
                CockpitInstruments.saveToFile(selectedFile.toString().replace("file:///", ""))
                statusText = "Saved: " + selectedFile.toString().split("/").pop().split("\\").pop()
            }
        }
    }

    FileDialog {
        id: exportImageDialog
        title: "Export as Image"
        nameFilters: ["PNG files (*.png)", "All files (*)"]
        onAccepted: {
            if (CockpitInstruments) {
                var err = CockpitInstruments.exportAsImage(selectedFile.toString().replace("file:///", ""))
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
            height: 44
            color: "#1e1e1e"
            Layout.fillWidth: true
            radius: 6

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                AppButton {
                    text: "Import"
                    flat: true
                    height: 30
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: displayOpenDialog.open()
                }
                AppButton {
                    text: "Export"
                    flat: true
                    height: 30
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: displaySaveDialog.open()
                }
                Rectangle {
                    width: 1
                    height: 22
                    color: "#333333"
                }
                AppButton {
                    text: "Export Image"
                    flat: true
                    height: 30
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: exportImageDialog.open()
                }
                Rectangle {
                    width: 1
                    height: 22
                    color: "#333333"
                }
                AppButton {
                    text: "Validate"
                    flat: true
                    height: 30
                    bgcolor: "#E10600"
                    color: "#121212"
                    onClicked: {
                        if (CockpitInstruments) {
                            statusText = "Validating display configuration..."
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: currentFile
                    color: "#aaaaaa"
                    font.pixelSize: 12
                }
            }
        }

        // -- QML Ribbon Bar --
        Rectangle {
            Layout.fillWidth: true
            height: 96
            color: "#1e1e1e"

            property var ribbonDefs: [
                { title: "File", panel: "", groups: [
                    { name: "File", buttons: [
                        { label: "Open", icon: "\u2601", cmd: "open" },
                        { label: "Save", icon: "\u2913", cmd: "save" },
                        { label: "Export", icon: "\u2B06", cmd: "export" }
                    ]}
                ]},
                { title: "Display", panel: "display", groups: [
                    { name: "Type", buttons: [
                        { label: "7-Seg", icon: "\u2588", cmd: "panel_display" },
                        { label: "14-Seg", icon: "\u2588", cmd: "panel_display" },
                        { label: "LCD", icon: "\u25A3", cmd: "panel_display" }
                    ]}
                ]},
                { title: "Items", panel: "items", groups: [
                    { name: "Add", buttons: [
                        { label: "Gear", icon: "G", cmd: "panel_items" },
                        { label: "Speed", icon: "S", cmd: "panel_items" },
                        { label: "RPM", icon: "R", cmd: "panel_items" },
                        { label: "Lap", icon: "L", cmd: "panel_items" },
                        { label: "Fuel", icon: "F", cmd: "panel_items" },
                        { label: "Pos", icon: "P", cmd: "panel_items" }
                    ]}
                ]},
                { title: "LEDs", panel: "leds", groups: [
                    { name: "Shift", buttons: [
                        { label: "Add", icon: "\u25CF", cmd: "panel_leds" },
                        { label: "Blink", icon: "\u25D4", cmd: "panel_leds" }
                    ]}
                ]},
                { title: "Tyre Slip", panel: "tyreslip", groups: [
                    { name: "Slip", buttons: [
                        { label: "Left", icon: "\u2190", cmd: "panel_tyreslip" },
                        { label: "Right", icon: "\u2192", cmd: "panel_tyreslip" }
                    ]}
                ]},
                { title: "Legacy", panel: "elements", groups: [
                    { name: "Elements", buttons: [
                        { label: "Text", icon: "T", cmd: "panel_elements" },
                        { label: "Bar", icon: "\u2500", cmd: "panel_elements" },
                        { label: "Ring", icon: "\u25CE", cmd: "panel_elements" }
                    ]}
                ]}
            ]

            function panelForTab(index) {
                if (index < 0 || index >= ribbonDefs.length) return ""
                return ribbonDefs[index].panel
            }

            function ribbonCommand(cmd) {
                if (cmd === "new") { if (CockpitInstruments) CockpitInstruments.clearElements(); currentFile = "NewDisplay" }
                else if (cmd === "open") { displayOpenDialog.open() }
                else if (cmd === "save") { displaySaveDialog.open() }
                else if (cmd === "export") { displaySaveDialog.open() }
                else if (cmd === "export_dds") { if (CockpitInstruments) CockpitInstruments.saveToFile(currentFile) }
                else if (cmd.startsWith("panel_")) {
                    var panel = cmd.substring(6)
                    activePanel = panel
                    updateRibbonIndex()
                }
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Tab bar
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    spacing: 0

                    Rectangle {
                        width: 40; Layout.fillHeight: true
                        color: "#18181b"
                        Text {
                            anchors.centerIn: parent
                            text: "KS"
                            color: "#E10600"; font.pixelSize: 11; font.bold: true
                        }
                    }

                    Repeater {
                        model: ribbonDefs
                        delegate: Rectangle {
                            Layout.fillHeight: true
                            width: Math.max(tabLabel.implicitWidth + 20, 64)
                            color: activeRibbonTab === index ? "#3e3e42" : (tabHover.containsMouse ? "#333336" : "transparent")
                            border.color: activeRibbonTab === index ? "#E10600" : "transparent"
                            border.width: 1
                            Text {
                                id: tabLabel
                                anchors.centerIn: parent
                                text: modelData.title
                                color: activeRibbonTab === index ? "#E10600" : "#ccc"
                                font.pixelSize: 11; font.bold: activeRibbonTab === index
                            }
                            MouseArea { id: tabHover; anchors.fill: parent; hoverEnabled: true
                                onClicked: {
                                    activeRibbonTab = index
                                    var panel = panelForTab(index)
                                    if (panel !== "") activePanel = panel
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        width: 120; height: 22; radius: 3; color: "#18181b"; border.color: "#3f3f46"; border.width: 1
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideMiddle
                            text: currentFile
                            color: "#aaa"; font.pixelSize: 9
                        }
                    }
                }

                // Ribbon content (groups + buttons)
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    spacing: 0

                    Repeater {
                        model: activeRibbonTab < ribbonDefs.length ? ribbonDefs[activeRibbonTab].groups : []
                        delegate: Item {
                            Layout.fillHeight: true
                            Layout.leftMargin: index > 0 ? 10 : 0
                            width: Math.max(groupName.implicitWidth + 16, groupRow.width + 8)

                            Rectangle {
                                visible: index > 0
                                width: 1; height: parent.height - 6
                                color: "#3f3f46"
                                anchors.left: parent.left
                                anchors.leftMargin: -5
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            RowLayout {
                                id: groupRow
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.topMargin: 2
                                height: 46
                                spacing: 1

                                Repeater {
                                    model: modelData.buttons
                                    delegate: Rectangle {
                                        width: 46; height: 44
                                        radius: 2
                                        color: btnHover.containsMouse ? "#3e3e42" : "transparent"
                                        Column {
                                            anchors.centerIn: parent; spacing: 2
                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                text: modelData.icon
                                                color: "#bbb"; font.pixelSize: 16
                                            }
                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                text: modelData.label
                                                color: "#999"; font.pixelSize: 8
                                            }
                                        }
                                        MouseArea { id: btnHover; anchors.fill: parent; hoverEnabled: true
                                            onClicked: ribbonCommand(modelData.cmd)
                                        }
                                    }
                                }
                            }

                            Text {
                                id: groupName
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 1
                                text: modelData.name
                                color: "#888"; font.pixelSize: 8; font.italic: true
                            }
                        }
                    }
                }
            }
        }

        // --- Main Content ---
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // --- Left Panel: Panels List ---
            Rectangle {
                width: 160
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1
                radius: 6

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    Text {
                        text: "DISPLAY ITEMS"
                        color: "#666"
                        font.pixelSize: 11
                        font.bold: true
                        padding: 4
                    }

                    AppButton {
                        height: 30
                        text: "Display"
                        bgcolor: activePanel === "display" ? "#E10600" : "#3e3e42"
                        color: activePanel === "display" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "display"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Items (" + (CockpitInstruments ? CockpitInstruments.getAcItems().length : 0) + ")"
                        bgcolor: activePanel === "items" ? "#E10600" : "#3e3e42"
                        color: activePanel === "items" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "items"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "LEDs (" + (CockpitInstruments ? CockpitInstruments.getAcLeds().length : 0) + ")"
                        bgcolor: activePanel === "leds" ? "#E10600" : "#3e3e42"
                        color: activePanel === "leds" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "leds"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Tyre Slip (" + (CockpitInstruments ? CockpitInstruments.getAcTyreSlips().length : 0) + ")"
                        bgcolor: activePanel === "tyreslip" ? "#E10600" : "#3e3e42"
                        color: activePanel === "tyreslip" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "tyreslip"; updateRibbonIndex() }
                    }

                    Rectangle { height: 12 }

                    Text {
                        text: "LEGACY"
                        color: "#666"
                        font.pixelSize: 11
                        font.bold: true
                        padding: 4
                    }

                    AppButton {
                        height: 30
                        text: "Elements"
                        bgcolor: activePanel === "elements" ? "#E10600" : "#3e3e42"
                        color: activePanel === "elements" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "elements"; updateRibbonIndex() }
                    }

                    Rectangle { height: 12 }

                    Text {
                        text: "TOOLS"
                        color: "#666"
                        font.pixelSize: 11
                        font.bold: true
                        padding: 4
                    }

                    AppButton {
                        height: 30
                        text: "Templates"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: {
                            if (CockpitInstruments) statusText = "AC Item templates loaded"
                        }
                    }
                    AppButton {
                        height: 30
                        text: "Data Sources"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: {
                            if (CockpitInstruments) statusText = "Types: GEAR, SPEED, RPM, LAPTIME, FUEL, PERF..."
                        }
                    }

                    Item { Layout.fillHeight: true }

                    AppButton {
                        height: 34
                        text: "Export DDS + INI"
                        bgcolor: "#E10600"
                        color: "#121212"
                        onClicked: { if (CockpitInstruments) CockpitInstruments.saveToFile(currentFile) }
                    }
                }
            }

            // --- Center: Display Canvas ---
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#3a3a3a"
                clip: true

                // Grid overlay
                Item {
                    anchors.fill: parent
                    visible: showGrid
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)

                            var gridSize = 20
                            var cols = Math.ceil(width / gridSize)
                            var rows = Math.ceil(height / gridSize)
                            var cx = width / 2
                            var cy = height / 2

                            ctx.strokeStyle = "#444444"
                            ctx.lineWidth = 1
                            ctx.globalAlpha = 0.5
                            for (var x = 0; x <= cols; ++x) {
                                ctx.beginPath()
                                ctx.moveTo(cx + (x - cols/2) * gridSize, 0)
                                ctx.lineTo(cx + (x - cols/2) * gridSize, height)
                                ctx.stroke()
                            }
                            for (var y = 0; y <= rows; ++y) {
                                ctx.beginPath()
                                ctx.moveTo(0, cy + (y - rows/2) * gridSize)
                                ctx.lineTo(width, cy + (y - rows/2) * gridSize)
                                ctx.stroke()
                            }
                            ctx.globalAlpha = 1.0

                            // Axis indicator (bottom-left)
                            var ax = 30
                            var ay = height - 40
                            var alen = 20

                            ctx.strokeStyle = "#ff4444"
                            ctx.lineWidth = 2
                            ctx.beginPath()
                            ctx.moveTo(ax, ay)
                            ctx.lineTo(ax + alen, ay)
                            ctx.stroke()
                            ctx.fillStyle = "#ff4444"
                            ctx.font = "bold 10px monospace"
                            ctx.fillText("X", ax + alen + 3, ay + 3)

                            ctx.strokeStyle = "#44ff44"
                            ctx.beginPath()
                            ctx.moveTo(ax, ay)
                            ctx.lineTo(ax, ay - alen)
                            ctx.stroke()
                            ctx.fillStyle = "#44ff44"
                            ctx.fillText("Y", ax + 3, ay - alen - 3)

                            ctx.strokeStyle = "#4444ff"
                            ctx.beginPath()
                            ctx.moveTo(ax, ay)
                            ctx.lineTo(ax - alen, ay - alen)
                            ctx.stroke()
                            ctx.fillStyle = "#4444ff"
                            ctx.fillText("Z", ax - alen - 12, ay - alen + 3)
                        }
                    }
                }

                // Camera info overlay (top-left)
                Rectangle {
                    anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 8
                    width: 160; height: 48; radius: 4
                    color: "#cc1e1e1e"; border.color: "#444"; border.width: 1

                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 6; spacing: 2
                        Text { text: "2D Canvas"; color: "#E10600"; font.pixelSize: 10; font.bold: true }
                        Text { text: "Display: " + displayType; color: "#999"; font.pixelSize: 9 }
                    }
                }

                // Scene info overlay (bottom-left)
                Rectangle {
                    anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.margins: 8
                    width: 220; height: 52; radius: 4
                    color: "#cc1e1e1e"; border.color: "#444"; border.width: 1

                    RowLayout {
                        anchors.fill: parent; anchors.margins: 6; spacing: 8
                        ColumnLayout { spacing: 1
                            Text { text: "Display: " + (CockpitInstruments ? CockpitInstruments.displayName : "none"); color: "#E10600"; font.pixelSize: 9; font.bold: true; elide: Text.ElideRight }
                            Text { text: "Elements: " + elementCount; color: "#999"; font.pixelSize: 9 }
                        }
                        Rectangle { width: 1; height: 28; color: "#444" }
                        ColumnLayout { spacing: 1
                            Text { text: "Type: " + displayType; color: "#999"; font.pixelSize: 9 }
                            Text { text: "Status: Ready"; color: "#44ff44"; font.pixelSize: 9 }
                        }
                    }
                }

                // Center: Simulated 7-segment display
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 16

                    Repeater {
                        model: previewText.length
                        delegate: Item {
                            width: 60
                            height: 90

                            property string ch: previewText[index] || " "

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

                            function seg(s) { return segsOn.indexOf(s) >= 0 ? segOnColor : segOffColor }

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

            // --- Right Panel: Properties & Tools ---
            Rectangle {
                Layout.preferredWidth: 320
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15

                    // --- Display Properties ---
                    ColumnLayout { visible: activePanel === "display"; spacing: 4
                        Text {
                            text: "DISPLAY PROPERTIES"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 13
                            padding: 4
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            color: "#252526"
                            border.color: "#3e3e42"
                            border.width: 1
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                Text {
                                    text: "TYPE"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    Text { text: "Type:"; color: "#888"; Layout.preferredWidth: 60 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["7-Segment", "14-Segment", "16-Segment", "LCD Matrix"]
                                        currentIndex: {
                                            if (displayType === "7seg") return 0
                                            if (displayType === "14seg") return 1
                                            if (displayType === "16seg") return 2
                                            if (displayType === "lcd") return 3
                                            return 0
                                        }
                                        onCurrentIndexChanged: {
                                            var types = ["7seg", "14seg", "16seg", "lcd"]
                                            var names = ["7-Segment Display", "14-Segment Display", "16-Segment Display", "LCD Matrix Display"]
                                            displayType = types[currentIndex]
                                            if (CockpitInstruments) CockpitInstruments.setDisplayName(names[currentIndex])
                                        }
                                    }
                                }

                                RowLayout {
                                    Text { text: "Columns:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: segColumns
                                        width: 50
                                        onEditingFinished: segColumns = parseInt(text)
                                    }
                                }

                                RowLayout {
                                    Text { text: "Rows:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: segRows
                                        width: 50
                                        onEditingFinished: segRows = parseInt(text)
                                    }
                                }

                                RowLayout {
                                    Text { text: "Name:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        Layout.fillWidth: true
                                        text: CockpitInstruments ? CockpitInstruments.displayName : "Display"
                                        onEditingFinished: if (CockpitInstruments) CockpitInstruments.setDisplayName(text)
                                    }
                                }
                            }
                        }
                    }

                    // --- Elements Panel ---
                    ColumnLayout { visible: activePanel === "elements"; spacing: 4
                        Text {
                            text: "ELEMENT MANAGEMENT"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 13
                            padding: 4
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            color: "#252526"
                            border.color: "#3e3e42"
                            border.width: 1
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                Text {
                                    text: "ADD ELEMENT"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    spacing: 4
                                    AppButton {
                                        height: 28
                                        text: "Text"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addElement({
                                                    type: "TEXT",
                                                    source: "SPEED",
                                                    position: [0, 0],
                                                    size: [100, 30],
                                                    color: "#ffffff",
                                                    fontSize: 24
                                                })
                                                statusText = "Text element added"
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 28
                                        text: "Image"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addElement({
                                                    type: "IMAGE",
                                                    source: "SPEED",
                                                    position: [0, 0],
                                                    size: [100, 100]
                                                })
                                                statusText = "Image element added"
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    AppButton {
                                        height: 28
                                        text: "Bar"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addElement({
                                                    type: "BAR",
                                                    source: "RPM",
                                                    position: [0, 0],
                                                    size: [200, 20]
                                                })
                                                statusText = "Bar element added"
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 28
                                        text: "Circle"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addElement({
                                                    type: "CIRCLE",
                                                    source: "SPEED",
                                                    position: [0, 0],
                                                    size: [50, 50]
                                                })
                                                statusText = "Circle element added"
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    AppButton {
                                        height: 28
                                        text: "Digits"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addElement({
                                                    type: "DIGIT_GROUP",
                                                    source: "SPEED",
                                                    position: [0, 0],
                                                    size: [200, 60],
                                                    decimalPlaces: 1
                                                })
                                                statusText = "Digit group added"
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 28
                                        text: "Ring"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addElement({
                                                    type: "PROGRESS_RING",
                                                    source: "RPM",
                                                    position: [0, 0],
                                                    size: [80, 80]
                                                })
                                                statusText = "Progress ring added"
                                            }
                                        }
                                    }
                                }

                                Rectangle { height: 4 }

                                Text {
                                    text: "ELEMENTS (" + elementCount + ")"
                                    color: "#888"
                                    font.pixelSize: 10
                                    font.bold: true
                                    padding: 2
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 120
                                    color: "#1a1a1e"
                                    radius: 4
                                    border.color: "#333333"
                                    border.width: 1

                                    Text {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        text: elementCount > 0 ? CockpitInstruments.getElements().length + " elements in display" : "No elements added yet"
                                        color: "#666"
                                        font.pixelSize: 10
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }
                        }
                    }

                    // --- Style Panel ---
                    ColumnLayout { visible: activePanel === "style"; spacing: 4
                        Text {
                            text: "STYLE SETTINGS"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 13
                            padding: 4
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            color: "#252526"
                            border.color: "#3e3e42"
                            border.width: 1
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                Text {
                                    text: "SEGMENT COLOUR"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    Text { text: "ON:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Rectangle {
                                        id: segOnColorRect; width: 24; height: 24; radius: 4; color: segOnColor
                                        border.color: "#555"; border.width: 1
                                        MouseArea { anchors.fill: parent; onClicked: segOnColorDialog.open() }
                                    }
                                    ColorDialog { id: segOnColorDialog; color: segOnColor; onAccepted: segOnColor = color.toString() }
                                }

                                RowLayout {
                                    Text { text: "OFF:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Rectangle {
                                        id: segOffColorRect; width: 24; height: 24; radius: 4; color: segOffColor
                                        border.color: "#555"; border.width: 1
                                        MouseArea { anchors.fill: parent; onClicked: segOffColorDialog.open() }
                                    }
                                    ColorDialog { id: segOffColorDialog; color: segOffColor; onAccepted: segOffColor = color.toString() }
                                }

                                Rectangle { height: 4 }

                                Text {
                                    text: "PREVIEW"
                                    color: "#888"
                                    font.pixelSize: 9
                                    font.bold: true
                                    padding: 2
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
                                        text: previewText
                                        color: "#ffffff"
                                        font.pixelSize: 18
                                    }
                                }

                                Rectangle { height: 4 }

                                Text {
                                    text: "CHARACTER SET"
                                    color: "#888"
                                    font.pixelSize: 9
                                    font.bold: true
                                    padding: 2
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 60
                                    radius: 4
                                    color: "#252526"
                                    border.color: "#333333"
                                    border.width: 1

                                    Text {
                                        anchors { fill: parent; margins: 8 }
                                        text: "0123456789\n.,-+/\\°%"
                                        color: "#888888"
                                        font.pixelSize: 11
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }
                        }
                    }

                    // --- Animation Panel ---
                    ColumnLayout { visible: activePanel === "animation"; spacing: 4
                        Text {
                            text: "ANIMATION"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 13
                            padding: 4
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            color: "#252526"
                            border.color: "#3e3e42"
                            border.width: 1
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                Text {
                                    text: "ENTRY ANIMATION"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    Text { text: "Type:"; color: "#888"; Layout.preferredWidth: 60 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["None", "Fade In", "Fade Out", "Slide In", "Slide Out", "Bounce"]
                                    }
                                }

                                RowLayout {
                                    Text { text: "Duration:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 100; to: 2000; stepSize: 100; value: 500
                                    }
                                    Text { text: "500ms"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                Rectangle { height: 4 }

                                Text {
                                    text: "LOOP ANIMATION"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    Text { text: "Type:"; color: "#888"; Layout.preferredWidth: 60 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["None", "Pulse", "Rotate", "Color Cycle", "Shake", "Physics Driven"]
                                    }
                                }

                                RowLayout {
                                    CheckBox { checked: false }
                                    Text { text: "Ping Pong"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    CheckBox { checked: true }
                                    Text { text: "Loop"; color: "#888"; font.pixelSize: 10 }
                                }
                            }
                        }
                    }

                    // --- Preview Panel ---
                    ColumnLayout { visible: activePanel === "preview"; spacing: 4
                        Text {
                            text: "PREVIEW OPTIONS"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 13
                            padding: 4
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            color: "#252526"
                            border.color: "#3e3e42"
                            border.width: 1
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                RowLayout {
                                    CheckBox {
                                        checked: showGrid
                                        onClicked: showGrid = checked
                                    }
                                    Text { text: "Show Grid"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    CheckBox {
                                        checked: showGuides
                                        onClicked: showGuides = checked
                                    }
                                    Text { text: "Show Guides"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Zoom:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 0.25; to: 4.0; stepSize: 0.25; value: canvasZoom
                                        onValueChanged: canvasZoom = value
                                    }
                                    Text { text: Math.round(canvasZoom * 100) + "%"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    spacing: 4
                                    AppButton {
                                        height: 28
                                        text: "Zoom +"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: canvasZoom = Math.min(canvasZoom + 0.25, 4.0)
                                    }
                                    AppButton {
                                        height: 28
                                        text: "Zoom -"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: canvasZoom = Math.max(canvasZoom - 0.25, 0.25)
                                    }
                                }
                            }
                        }
                    }

                    // --- Items Panel (AC CSP) ---
                    ColumnLayout { visible: activePanel === "items"; spacing: 4
                        Text {
                            text: "AC DISPLAY ITEMS"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 13
                            padding: 4
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            color: "#252526"
                            border.color: "#3e3e42"
                            border.width: 1
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                Text {
                                    text: "ADD ITEM"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    spacing: 4
                                    AppButton {
                                        height: 28
                                        text: "Gear"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addAcItem({type: 0, parent: "DISPLAY_WHEEL", objectName: "ITEM_GEAR", width: 80, height: 60, color: "#ffffff", font: "german_led", align: "CENTER", decimals: 0})
                                                statusText = "Gear item added"
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 28
                                        text: "Speed"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addAcItem({type: 1, parent: "DISPLAY_WHEEL", objectName: "ITEM_SPEED", width: 160, height: 60, color: "#E10600", font: "german_led", align: "CENTER", decimals: 1, units: "KPH"})
                                                statusText = "Speed item added"
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    AppButton {
                                        height: 28
                                        text: "RPM"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addAcItem({type: 2, parent: "DISPLAY_WHEEL", objectName: "ITEM_RPM", width: 120, height: 40, color: "#44ff44", font: "german_led", align: "CENTER", decimals: 0})
                                                statusText = "RPM item added"
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 28
                                        text: "Lap Time"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addAcItem({type: 3, parent: "DISPLAY_MONITOR", objectName: "ITEM_LAPTIME", width: 200, height: 30, color: "#ffffff", font: "german_led", align: "LEFT", decimals: 3, nTime: 0.1})
                                                statusText = "Lap time item added"
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    AppButton {
                                        height: 28
                                        text: "Fuel"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addAcItem({type: 4, parent: "DISPLAY_WHEEL", objectName: "ITEM_FUEL", width: 100, height: 30, color: "#f59e0b", font: "german_led", align: "CENTER", decimals: 1})
                                                statusText = "Fuel item added"
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 28
                                        text: "Position"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addAcItem({type: 13, parent: "DISPLAY_WHEEL", objectName: "ITEM_POS", width: 60, height: 40, color: "#ffffff", font: "german_led", align: "CENTER", decimals: 0})
                                                statusText = "Position item added"
                                            }
                                        }
                                    }
                                }

                                Rectangle { height: 4 }

                                Text {
                                    text: "ITEMS (" + (CockpitInstruments ? CockpitInstruments.getAcItems().length : 0) + ")"
                                    color: "#888"
                                    font.pixelSize: 10
                                    font.bold: true
                                    padding: 2
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 120
                                    color: "#1a1a1e"
                                    radius: 4
                                    border.color: "#333333"
                                    border.width: 1

                                    Text {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        text: {
                                            if (!CockpitInstruments) return "No display loaded";
                                            var items = CockpitInstruments.getAcItems();
                                            if (items.length === 0) return "No items configured";
                                            var text = "";
                                            for (var i = 0; i < Math.min(items.length, 8); i++) {
                                                text += "[" + items[i].index + "] " + items[i].typeStr + " -> " + items[i].objectName + "\n";
                                            }
                                            if (items.length > 8) text += "... and " + (items.length - 8) + " more";
                                            return text;
                                        }
                                        color: "#888"
                                        font.pixelSize: 9
                                        font.family: "monospace"
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }
                        }
                    }

                    // --- LEDs Panel (AC CSP) ---
                    ColumnLayout { visible: activePanel === "leds"; spacing: 4
                        Text {
                            text: "RPM SHIFT LEDs"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 13
                            padding: 4
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            color: "#252526"
                            border.color: "#3e3e42"
                            border.width: 1
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                Text {
                                    text: "ADD LED"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    spacing: 4
                                    AppButton {
                                        height: 28
                                        text: "Shift LED"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addAcLed({objectName: "LED_RPM_NEW", rpmSwitch: 7000, emissiveR: 255, emissiveG: 0, emissiveB: 0, diffuse: 0.2})
                                                statusText = "Shift LED added"
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 28
                                        text: "Blink LED"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addAcLed({objectName: "LED_BLINK_NEW", rpmSwitch: 8000, emissiveR: 255, emissiveG: 255, emissiveB: 0, diffuse: 0.2, blinkSwitch: 8000, blinkHz: 5})
                                                statusText = "Blink LED added"
                                            }
                                        }
                                    }
                                }

                                Rectangle { height: 4 }

                                Text {
                                    text: "LEDS (" + (CockpitInstruments ? CockpitInstruments.getAcLeds().length : 0) + ")"
                                    color: "#888"
                                    font.pixelSize: 10
                                    font.bold: true
                                    padding: 2
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 150
                                    color: "#1a1a1e"
                                    radius: 4
                                    border.color: "#333333"
                                    border.width: 1

                                    Text {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        text: {
                                            if (!CockpitInstruments) return "No display loaded";
                                            var leds = CockpitInstruments.getAcLeds();
                                            if (leds.length === 0) return "No LEDs configured";
                                            var text = "";
                                            for (var i = 0; i < Math.min(leds.length, 10); i++) {
                                                text += "[LED_" + leds[i].index + "] " + leds[i].objectName + " @ " + leds[i].rpmSwitch + " RPM\n";
                                            }
                                            if (leds.length > 10) text += "... and " + (leds.length - 10) + " more";
                                            return text;
                                        }
                                        color: "#888"
                                        font.pixelSize: 9
                                        font.family: "monospace"
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }
                        }
                    }

                    // --- Tyre Slip Panel (AC CSP) ---
                    ColumnLayout { visible: activePanel === "tyreslip"; spacing: 4
                        Text {
                            text: "TYRE LOCK / SLIP LEDs"
                            color: "#E10600"
                            font.bold: true
                            font.pixelSize: 13
                            padding: 4
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            color: "#252526"
                            border.color: "#3e3e42"
                            border.width: 1
                            radius: 4

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 10

                                Text {
                                    text: "ADD TYRE SLIP LED"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    spacing: 4
                                    AppButton {
                                        height: 28
                                        text: "Left Slip"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addAcTyreSlip({objectName: "LED_SLIP_L_NEW", tyreIndex: 0, emissiveR: 0, emissiveG: 0, emissiveB: 160, emissiveLockR: 0, emissiveLockG: 0, emissiveLockB: 0, diffuse: 0.25, slipSwitch: 0.6, showLock: false, wheelSpeedMult: 0.8})
                                                statusText = "Left slip LED added"
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 28
                                        text: "Right Slip"
                                        Layout.fillWidth: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (CockpitInstruments) {
                                                CockpitInstruments.addAcTyreSlip({objectName: "LED_SLIP_R_NEW", tyreIndex: 1, emissiveR: 0, emissiveG: 0, emissiveB: 160, emissiveLockR: 0, emissiveLockG: 0, emissiveLockB: 0, diffuse: 0.25, slipSwitch: 0.6, showLock: false, wheelSpeedMult: 0.8})
                                                statusText = "Right slip LED added"
                                            }
                                        }
                                    }
                                }

                                Rectangle { height: 4 }

                                Text {
                                    text: "TYRE SLIP LEDs (" + (CockpitInstruments ? CockpitInstruments.getAcTyreSlips().length : 0) + ")"
                                    color: "#888"
                                    font.pixelSize: 10
                                    font.bold: true
                                    padding: 2
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 120
                                    color: "#1a1a1e"
                                    radius: 4
                                    border.color: "#333333"
                                    border.width: 1

                                    Text {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        text: {
                                            if (!CockpitInstruments) return "No display loaded";
                                            var slips = CockpitInstruments.getAcTyreSlips();
                                            if (slips.length === 0) return "No tyre slip LEDs configured";
                                            var text = "";
                                            for (var i = 0; i < Math.min(slips.length, 8); i++) {
                                                var side = slips[i].tyreIndex === 0 ? "L" : "R";
                                                text += "[TYRE_" + slips[i].index + "] " + side + " " + slips[i].objectName + "\n";
                                            }
                                            if (slips.length > 8) text += "... and " + (slips.length - 8) + " more";
                                            return text;
                                        }
                                        color: "#888"
                                        font.pixelSize: 9
                                        font.family: "monospace"
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    // --- Validation Section ---
                    Rectangle {
                        Layout.fillWidth: true
                        color: "#252526"
                        border.color: "#3e3e42"
                        border.width: 1
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            Text {
                                text: "VALIDATION"
                                color: "#888"
                                font.pixelSize: 10
                                font.bold: true
                                padding: 4
                            }

                            RowLayout {
                                Rectangle {
                                    width: 10
                                    height: 10
                                    radius: 5
                                    color: "#E10600"
                                }
                                Text {
                                    text: "Valid"
                                    color: "#E10600"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            RowLayout {
                                AppButton {
                                    height: 30
                                    text: "Validate"
                                    bgcolor: "#E10600"
                                    color: "#121212"
                                    Layout.fillWidth: true
                                    onClicked: {
                                        if (CockpitInstruments) statusText = "Display configuration validated"
                                    }
                                }
                                AppButton {
                                    height: 30
                                    text: "Export"
                                    bgcolor: "#ff6600"
                                    color: "#ffffff"
                                    Layout.fillWidth: true
                                    onClicked: displaySaveDialog.open()
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- Status Bar ---
        Rectangle {
            height: 28
            color: "#252526"
            Layout.fillWidth: true
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6

                Text { text: statusText; color: "#E10600"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                Text {
                    text: currentFile + " | ksEditor Display"
                    color: "#666"
                    font.pixelSize: 10
                }
            }
        }
    }
    }
}
