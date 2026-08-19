import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../widgets"
import ksEditor.Physics 1.0

Rectangle {
    id: root
    color: "#121212"

    property int activeRibbonTab: 0

    // Synchronize activeRibbonTab with activePanel
    Component.onCompleted: {
        updateRibbonIndex()
    }

    function updateRibbonIndex() {
        switch (activePanel) {
            case "suspension": activeRibbonTab = 1; break
            case "wheels": activeRibbonTab = 2; break
            case "tires": activeRibbonTab = 3; break
            case "aero": activeRibbonTab = 4; break
            case "engine": activeRibbonTab = 5; break
            case "transmission": activeRibbonTab = 6; break
            case "brakes": activeRibbonTab = 7; break
            case "steering": activeRibbonTab = 8; break
            case "driver": activeRibbonTab = 9; break
            case "ai": activeRibbonTab = 10; break
            case "electronics": activeRibbonTab = 11; break
            case "drs": activeRibbonTab = 12; break
            case "turbo": activeRibbonTab = 13; break
            case "hybrid": activeRibbonTab = 14; break
            case "damage": activeRibbonTab = 15; break
            case "fuel": activeRibbonTab = 16; break
        }
    }

    property real baseUiScale: 1.18
    property real uiZoom: 1.0
    property real uiScale: baseUiScale * uiZoom

    property string activePanel: "suspension"
    property bool isValid: Physics ? Physics.isValid : true
    property string currentFile: Physics ? Physics.currentFile : "LOD0.FBX"
    property string statusText: "Ready"

    Component.onCompleted: {
        if (Physics) {
            Physics.statusMessage.connect(function(msg) { statusText = msg; })
        }
    }

    FileDialog {
        id: importDialog
        title: "Import Physics File"
        nameFilters: ["Physics files (*.ini *.json *.fbx *.kn5)", "All files (*)"]
        onAccepted: {
            if (Physics) {
                Physics.loadFile(selectedFile.toString().replace("file:///", ""))
                currentFile = Physics.currentFile.split("/").pop().split("\\").pop()
            }
        }
    }

    FileDialog {
        id: exportDialog
        title: "Export Physics File"
        nameFilters: ["INI files (*.ini)", "JSON files (*.json)", "All files (*)"]
        onAccepted: {
            if (Physics) {
                Physics.saveFile(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: colliderExportDialog
        title: "Export Collider Mesh"
        nameFilters: ["FBX files (*.fbx)", "KN5 files (*.kn5)", "All files (*)"]
        onAccepted: {
            if (Physics) {
                Physics.generateColliders("high", "convex", true, true)
                Physics.statusMessage("Colliders generated")
            }
        }
    }

    FileDialog {
        id: lodsImportDialog
        title: "Import LOD Files"
        nameFilters: ["FBX files (*.fbx)", "KN5 files (*.kn5)", "All files (*)"]
        onAccepted: {
            if (Physics) {
                var path = selectedFile.toString().replace("file:///", "")
                Physics.importLOD(path)
                Physics.statusMessage("LOD imported: " + path.split("/").pop().split("\\").pop())
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
                    onClicked: importDialog.open()
                }
                AppButton {
                    text: "Export"
                    flat: true
                    height: 30
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: exportDialog.open()
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
                    bgcolor: isValid ? "#E10600" : "#3e3e42"
                    color: isValid ? "#121212" : "#aaaaaa"
                    onClicked: {
                        if (Physics) {
                            isValid = Physics.validate()
                        }
                    }
                }
                Rectangle {
                    width: 1
                    height: 22
                    color: "#333333"
                }
                AppButton {
                    text: "Simulate"
                    flat: true
                    height: 30
                    bgcolor: Physics && Physics.isSimulating ? "#E10600" : "transparent"
                    color: Physics && Physics.isSimulating ? "#121212" : "#ffffff"
                    onClicked: {
                        if (Physics) {
                            if (Physics.isSimulating) Physics.stopSimulation()
                            else Physics.startSimulation()
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

        // ── QML Ribbon Bar ────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 96
            color: "#1e1e1e"

            property var ribbonDefs: [
                { title: "File", panel: "", groups: [
                    { name: "File", buttons: [
                        { label: "New", icon: "\u2795", cmd: "new" },
                        { label: "Open", icon: "\u2601", cmd: "open" },
                        { label: "Save", icon: "\u2913", cmd: "save" },
                        { label: "Import", icon: "\u2B07", cmd: "import" },
                        { label: "Export", icon: "\u2B06", cmd: "export" }
                    ]}
                ]},
                { title: "Suspension", panel: "suspension", groups: [
                    { name: "Springs", buttons: [
                        { label: "Front", icon: "\u2191", cmd: "panel_suspension" },
                        { label: "Rear", icon: "\u2193", cmd: "panel_suspension" }
                    ]},
                    { name: "Dampers", buttons: [
                        { label: "Bump", icon: "\u25BC", cmd: "panel_suspension" },
                        { label: "Rebound", icon: "\u25B2", cmd: "panel_suspension" }
                    ]}
                ]},
                { title: "Wheels", panel: "wheels", groups: [
                    { name: "Config", buttons: [
                        { label: "Diameter", icon: "\u25CB", cmd: "panel_wheels" },
                        { label: "Width", icon: "\u2500", cmd: "panel_wheels" },
                        { label: "Mass", icon: "\u2696", cmd: "panel_wheels" }
                    ]}
                ]},
                { title: "Tires", panel: "tires", groups: [
                    { name: "Compound", buttons: [
                        { label: "Soft", icon: "S", cmd: "panel_tires" },
                        { label: "Medium", icon: "M", cmd: "panel_tires" },
                        { label: "Hard", icon: "H", cmd: "panel_tires" }
                    ]},
                    { name: "Thermal", buttons: [
                        { label: "Temp", icon: "\u2668", cmd: "panel_tires" },
                        { label: "Tread", icon: "\u25D4", cmd: "panel_tires" }
                    ]}
                ]},
                { title: "Aerodynamics", panel: "aero", groups: [
                    { name: "Forces", buttons: [
                        { label: "Drag", icon: "\u2190", cmd: "panel_aero" },
                        { label: "Downforce", icon: "\u2193", cmd: "panel_aero" }
                    ]}
                ]},
                { title: "Engine", panel: "engine", groups: [
                    { name: "Output", buttons: [
                        { label: "Power", icon: "\u26A1", cmd: "panel_engine" },
                        { label: "Torque", icon: "\u21BB", cmd: "panel_engine" },
                        { label: "RPM", icon: "\u27F3", cmd: "panel_engine" }
                    ]}
                ]},
                { title: "Transmission", panel: "transmission", groups: [
                    { name: "Gears", buttons: [
                        { label: "Type", icon: "\u2699", cmd: "panel_transmission" },
                        { label: "Count", icon: "\u2460", cmd: "panel_transmission" },
                        { label: "Final", icon: "\u2192", cmd: "panel_transmission" }
                    ]}
                ]},
                { title: "Brakes", panel: "brakes", groups: [
                    { name: "Config", buttons: [
                        { label: "Pressure", icon: "\u25A0", cmd: "panel_brakes" },
                        { label: "Bias", icon: "\u25CE", cmd: "panel_brakes" }
                    ]}
                ]},
                { title: "Steering", panel: "steering", groups: [
                    { name: "Config", buttons: [
                        { label: "Ratio", icon: "\u2194", cmd: "panel_steering" },
                        { label: "Lock", icon: "\u21C4", cmd: "panel_steering" },
                        { label: "Power", icon: "\u26A1", cmd: "panel_steering" }
                    ]}
                ]},
                { title: "Driver", panel: "driver", groups: [
                    { name: "Position", buttons: [
                        { label: "Pos XYZ", icon: "\u25CE", cmd: "panel_driver" },
                        { label: "Mass", icon: "\u2696", cmd: "panel_driver" }
                    ]}
                ]},
                { title: "AI Config", panel: "ai", groups: [
                    { name: "AI", buttons: [
                        { label: "Skill", icon: "\u2605", cmd: "panel_ai" },
                        { label: "Aggro", icon: "\u2620", cmd: "panel_ai" },
                        { label: "Consist", icon: "\u2696", cmd: "panel_ai" }
                    ]}
                ]},
                { title: "Electronics", panel: "electronics", groups: [
                    { name: "Systems", buttons: [
                        { label: "ABS", icon: "\u2714", cmd: "panel_electronics" },
                        { label: "TC", icon: "\u26A1", cmd: "panel_electronics" },
                        { label: "ESC", icon: "\u26D4", cmd: "panel_electronics" },
                        { label: "Launch", icon: "\u2191", cmd: "panel_electronics" }
                    ]}
                ]},
                { title: "DRS", panel: "drs", groups: [
                    { name: "Config", buttons: [
                        { label: "Enable", icon: "\u2714", cmd: "panel_drs" },
                        { label: "Speed", icon: "\u25B6", cmd: "panel_drs" },
                        { label: "Drag", icon: "\u2190", cmd: "panel_drs" }
                    ]}
                ]},
                { title: "Turbo", panel: "turbo", groups: [
                    { name: "Config", buttons: [
                        { label: "Boost", icon: "\u2191", cmd: "panel_turbo" },
                        { label: "Lag", icon: "\u23F3", cmd: "panel_turbo" },
                        { label: "Wastegate", icon: "\u26D4", cmd: "panel_turbo" }
                    ]}
                ]},
                { title: "Hybrid", panel: "hybrid", groups: [
                    { name: "ERS", buttons: [
                        { label: "Enable", icon: "\u2714", cmd: "panel_hybrid" },
                        { label: "Mode", icon: "\u2699", cmd: "panel_hybrid" },
                        { label: "MGU-K", icon: "\u26A1", cmd: "panel_hybrid" },
                        { label: "MGU-H", icon: "\u2668", cmd: "panel_hybrid" }
                    ]},
                    { name: "Battery", buttons: [
                        { label: "Capacity", icon: "\u25A0", cmd: "panel_hybrid" },
                        { label: "Attack", icon: "\u2621", cmd: "panel_hybrid" }
                    ]}
                ]},
                { title: "Damage", panel: "damage", groups: [
                    { name: "Model", buttons: [
                        { label: "Enable", icon: "\u2714", cmd: "panel_damage" },
                        { label: "Aero", icon: "\u2190", cmd: "panel_damage" },
                        { label: "Engine", icon: "\u2699", cmd: "panel_damage" },
                        { label: "Body", icon: "\u25A0", cmd: "panel_damage" }
                    ]}
                ]},
                { title: "Fuel", panel: "fuel", groups: [
                    { name: "Tank", buttons: [
                        { label: "Load", icon: "\u2696", cmd: "panel_fuel" },
                        { label: "Capacity", icon: "\u25A0", cmd: "panel_fuel" },
                        { label: "Consumption", icon: "\u21BB", cmd: "panel_fuel" }
                    ]}
                ]}
            ]

            function panelForTab(index) {
                if (index < 0 || index >= ribbonDefs.length) return ""
                return ribbonDefs[index].panel
            }

            function ribbonCommand(cmd) {
                if (cmd === "new") { if (Physics) Physics.newProject(); currentFile = "NewCar" }
                else if (cmd === "open") { importDialog.open() }
                else if (cmd === "save") { exportDialog.open() }
                else if (cmd === "import") { importDialog.open() }
                else if (cmd === "export") { exportDialog.open() }
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
                        text: "PANELS"
                        color: "#666"
                        font.pixelSize: 11
                        font.bold: true
                        padding: 4
                    }

                    // Panel buttons - also update ribbon
                    AppButton {
                        height: 30
                        text: "Suspension"
                        bgcolor: activePanel === "suspension" ? "#E10600" : "#3e3e42"
                        color: activePanel === "suspension" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "suspension"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Wheels"
                        bgcolor: activePanel === "wheels" ? "#E10600" : "#3e3e42"
                        color: activePanel === "wheels" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "wheels"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Tires"
                        bgcolor: activePanel === "tires" ? "#E10600" : "#3e3e42"
                        color: activePanel === "tires" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "tires"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Aerodynamics"
                        bgcolor: activePanel === "aero" ? "#E10600" : "#3e3e42"
                        color: activePanel === "aero" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "aero"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Engine"
                        bgcolor: activePanel === "engine" ? "#E10600" : "#3e3e42"
                        color: activePanel === "engine" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "engine"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Transmission"
                        bgcolor: activePanel === "transmission" ? "#E10600" : "#3e3e42"
                        color: activePanel === "transmission" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "transmission"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Brakes"
                        bgcolor: activePanel === "brakes" ? "#E10600" : "#3e3e42"
                        color: activePanel === "brakes" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "brakes"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Steering"
                        bgcolor: activePanel === "steering" ? "#E10600" : "#3e3e42"
                        color: activePanel === "steering" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "steering"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Driver"
                        bgcolor: activePanel === "driver" ? "#E10600" : "#3e3e42"
                        color: activePanel === "driver" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "driver"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "AI Config"
                        bgcolor: activePanel === "ai" ? "#E10600" : "#3e3e42"
                        color: activePanel === "ai" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "ai"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Electronics"
                        bgcolor: activePanel === "electronics" ? "#E10600" : "#3e3e42"
                        color: activePanel === "electronics" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "electronics"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "DRS"
                        bgcolor: activePanel === "drs" ? "#E10600" : "#3e3e42"
                        color: activePanel === "drs" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "drs"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Turbo"
                        bgcolor: activePanel === "turbo" ? "#E10600" : "#3e3e42"
                        color: activePanel === "turbo" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "turbo"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Hybrid"
                        bgcolor: activePanel === "hybrid" ? "#E10600" : "#3e3e42"
                        color: activePanel === "hybrid" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "hybrid"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Damage"
                        bgcolor: activePanel === "damage" ? "#E10600" : "#3e3e42"
                        color: activePanel === "damage" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "damage"; updateRibbonIndex() }
                    }
                    AppButton {
                        height: 30
                        text: "Fuel"
                        bgcolor: activePanel === "fuel" ? "#E10600" : "#3e3e42"
                        color: activePanel === "fuel" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "fuel"; updateRibbonIndex() }
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
                        text: "Colliders"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: colliderExportDialog.open()
                    }
                    AppButton {
                        height: 30
                        text: "LODs"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: lodsImportDialog.open()
                    }
                    AppButton {
                        height: 30
                        text: "Mirrors"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: {
                            if (Physics) {
                                Physics.setupMirrors(2, 15.0, 3.0)
                                Physics.statusMessage("Mirrors configured: 2 mirrors, 15 deg angle")
                            }
                        }
                    }
                    AppButton {
                        height: 30
                        text: "Exhaust"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: {
                            if (Physics) {
                                Physics.setupExhaust("dual", 0.8, 0.1)
                                Physics.statusMessage("Exhaust configured: dual type")
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    AppButton {
                        height: 34
                        text: "Advanced"
                        bgcolor: "#E10600"
                        color: "#121212"
                        onClicked: {
                            if (Physics) Physics.statusMessage("Advanced physics settings")
                        }
                    }
                }
            }

            // --- Center: 3D Preview (GIANT Editor-style) ---
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#3a3a3a"
                clip: true

                // Grid overlay
                Item {
                    anchors.fill: parent
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
                        Text { text: "Free Camera"; color: "#E10600"; font.pixelSize: 10; font.bold: true }
                        Text { text: "Perspective | Gizmo: Local"; color: "#999"; font.pixelSize: 9 }
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
                            Text { text: "Car: " + (Physics ? Physics.carName : "none"); color: "#E10600"; font.pixelSize: 9; font.bold: true; elide: Text.ElideRight }
                            Text { text: "Total Mass: " + (Physics ? Physics.totalMass.toFixed(1) + " kg" : "—"); color: "#999"; font.pixelSize: 9 }
                        }
                        Rectangle { width: 1; height: 28; color: "#444" }
                        ColumnLayout { spacing: 1
                            Text { text: "Susp: " + (Physics ? Physics.suspensionFrontSpring.toFixed(0) + "/" + Physics.suspensionRearSpring.toFixed(0) : "—"); color: "#999"; font.pixelSize: 9 }
                            Text { text: "Status: " + (Physics && Physics.isValid ? "Valid" : "Check params"); color: Physics && Physics.isValid ? "#44ff44" : "#ffaa00"; font.pixelSize: 9 }
                        }
                    }
                }

                // Drop zone hint (center)
                Text {
                    anchors.centerIn: parent
                    text: Physics && Physics.carName ? Physics.carName + "\nconfiguration loaded" : "Drop FBX / KN5 here\nto preview 3D model"
                    color: "#666"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    lineHeight: 1.5
                    opacity: Physics && Physics.carName ? 0.3 : 0.5
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

                    // --- Properties Section ---
                    ColumnLayout { visible: activePanel === "suspension"; spacing: 4
                        Text {
                            text: "SUSPENSION PHYSICS"
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
                                    text: "FRONT"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    Text { text: "Type:"; color: "#888"; Layout.preferredWidth: 60 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Pushrod", "Pullrod", "MacPherson", "Double Wishbone"]
                                    }
                                }

                                RowLayout {
                                    Text { text: "Spring:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        id: frontSpringField
                                        text: Physics ? Physics.suspensionFrontSpring : "0"
                                        Layout.fillWidth: true
                                        onEditingFinished: if (Physics) Physics.suspensionFrontSpring = parseFloat(text)
                                        textMargins: Qt.binding(function() { return [4, 0, 4, 0]; })
                                    }
                                    Text { text: "N/m"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Comp:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField { text: Physics ? (Physics.suspensionFrontSpring * 0.3).toFixed(1) : "8"; width: 45 }
                                    Text { text: "Reb:"; color: "#888" }
                                    TextField { text: Physics ? (Physics.suspensionFrontSpring * 0.5).toFixed(1) : "10"; width: 45 }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            color: "#252526"
                            border.color: "#3e3e42"
                            border.width: 1

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 8

                                Text {
                                    text: "REAR"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    Text { text: "Type:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Pushrod", "Pullrod", "MacPherson", "Double Wishbone"]
                                    }
                                }

                                RowLayout {
                                    Text { text: "Spring:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        id: rearSpringField
                                        text: Physics ? Physics.suspensionRearSpring : "0"
                                        Layout.fillWidth: true
                                        onEditingFinished: if (Physics) Physics.suspensionRearSpring = parseFloat(text)
                                    }
                                    Text { text: "N/m"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Comp:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField { text: Physics ? (Physics.suspensionRearSpring * 0.3).toFixed(1) : "7"; width: 50 }
                                    Text { text: "Reb:"; color: "#bbbbbb" }
                                    TextField { text: Physics ? (Physics.suspensionRearSpring * 0.5).toFixed(1) : "9"; width: 50 }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "wheels"; spacing: 4
                        Text {
                            text: "WHEEL PHYSICS"
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
                                    Text { text: "Diameter:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.wheelDiameter : "26"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.wheelDiameter = parseFloat(text)
                                    }
                                    Text { text: "in"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Width:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.wheelWidth : "12"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.wheelWidth = parseFloat(text)
                                    }
                                    Text { text: "in"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Mass:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.wheelMass : "12"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.wheelMass = parseFloat(text)
                                    }
                                    Text { text: "kg"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Rim:"; color: "#888"; Layout.preferredWidth: 60 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Single Piece", "Multi Piece", "Forged", "Carbon"]
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "tires"; spacing: 4
                        Text {
                            text: "TIRE PHYSICS"
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
                                    Text { text: "Compound:"; color: "#888"; Layout.preferredWidth: 60 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Soft", "Medium", "Hard", "Inter", "Wet"]
                                        currentIndex: {
                                            if (!Physics) return 1
                                            var idx = model.indexOf(Physics.tireCompound)
                                            return idx >= 0 ? idx : 1
                                        }
                                        onCurrentTextChanged: if (Physics) Physics.tireCompound = currentText
                                    }
                                }

                                RowLayout {
                                    Text { text: "Tread:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 0; to: 100; value: Physics ? Physics.tireTreadRemaining : 100
                                        onValueChanged: if (Physics) Physics.tireTreadRemaining = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.tireTreadRemaining : 100) + "%"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                Text {
                                    text: "THERMAL"
                                    color: "#888"
                                    font.pixelSize: 9
                                    font.bold: true
                                    padding: 2
                                }

                                RowLayout {
                                    Text { text: "Optimal:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.tireOptimalTemp : "90"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.tireOptimalTemp = parseFloat(text)
                                    }
                                    Text { text: "°C"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Max:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.tireMaxTemp : "120"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.tireMaxTemp = parseFloat(text)
                                    }
                                    Text { text: "°C"; color: "#666"; font.pixelSize: 9 }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "aero"; spacing: 4
                        Text {
                            text: "AERODYNAMICS"
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
                                    Text { text: "Drag:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        id: dragField
                                        text: Physics ? Physics.drag : "0.4"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.drag = parseFloat(text)
                                    }
                                }

                                RowLayout {
                                    Text { text: "Downforce:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        id: downforceField
                                        text: Physics ? Physics.frontDownforce : "1200"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.frontDownforce = parseFloat(text)
                                    }
                                    Text { text: "N"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Downforce R:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.rearDownforce : "1500"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.rearDownforce = parseFloat(text)
                                    }
                                    Text { text: "N"; color: "#666"; font.pixelSize: 9 }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "engine"; spacing: 4
                        Text {
                            text: "ENGINE PHYSICS"
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
                                    Text { text: "Max RPM:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        id: maxRpmField
                                        text: Physics ? Physics.redlineRPM : "8000"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.redlineRPM = parseInt(text)
                                    }
                                }

                                RowLayout {
                                    Text { text: "Power:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        id: powerField
                                        text: Physics ? Math.round(Physics.maxPowerKw * 1.34102) : "268"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.maxPowerKw = parseFloat(text) / 1.34102
                                    }
                                    Text { text: "HP"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Torque:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        id: torqueField
                                        text: Physics ? Physics.maxTorqueNm : "400"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.maxTorqueNm = parseFloat(text)
                                    }
                                    Text { text: "Nm"; color: "#666"; font.pixelSize: 9 }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "transmission"; spacing: 4
                        Text {
                            text: "TRANSMISSION"
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
                                    Text { text: "Type:"; color: "#888"; Layout.preferredWidth: 60 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Sequential", "H-Pattern", "Dual Clutch", "CVT"]
                                        currentIndex: Physics ? Physics.transmissionType : 0
                                        onCurrentIndexChanged: if (Physics) Physics.transmissionType = currentIndex
                                    }
                                }

                                RowLayout {
                                    Text { text: "Gears:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.gearCount : "7"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.gearCount = Math.max(1, Math.min(10, parseInt(text)))
                                    }
                                }

                                RowLayout {
                                    Text { text: "Final:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        id: finalDriveField
                                        text: Physics ? Physics.finalDrive : "3.7"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.finalDrive = parseFloat(text)
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "brakes"; spacing: 4
                        Text {
                            text: "BRAKE PHYSICS"
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
                                    Text { text: "Type:"; color: "#888"; Layout.preferredWidth: 60 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Steel", "Carbon Ceramic", "Carbon Carbon"]
                                    }
                                }

                                RowLayout {
                                    Text { text: "Pressure:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.brakePressure : "150"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.brakePressure = parseFloat(text)
                                    }
                                    Text { text: "bar"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Bias:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 50; to: 70; value: Physics ? Physics.brakeBalance * 100 : 58
                                        onValueChanged: if (Physics) Physics.brakeBalance = value / 100
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.brakeBalance * 100 : 58) + "%"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "steering"; spacing: 4
                        Text {
                            text: "STEERING"
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
                                    Text { text: "Ratio:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 10; to: 30; value: Physics ? Physics.steeringRatio : 15
                                        onValueChanged: if (Physics) Physics.steeringRatio = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.steeringRatio : 15) + ":1"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "Lock:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.steeringLockAngle : "45"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.steeringLockAngle = parseFloat(text)
                                    }
                                    Text { text: "°"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    CheckBox {
                                        checked: Physics ? Physics.powerSteering : true
                                        onClicked: if (Physics) Physics.powerSteering = checked
                                    }
                                    Text { text: "Power Steering"; color: "#888"; font.pixelSize: 9 }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "driver"; spacing: 4
                        Text {
                            text: "DRIVER"
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
                                    Text { text: "Position:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.driverPositionX : "0.1"
                                        width: 45
                                        onEditingFinished: if (Physics) Physics.driverPositionX = parseFloat(text)
                                    }
                                    TextField {
                                        text: Physics ? Physics.driverPositionY : "0.5"
                                        width: 45
                                        onEditingFinished: if (Physics) Physics.driverPositionY = parseFloat(text)
                                    }
                                    TextField {
                                        text: Physics ? Physics.driverPositionZ : "-0.2"
                                        width: 45
                                        onEditingFinished: if (Physics) Physics.driverPositionZ = parseFloat(text)
                                    }
                                }

                                RowLayout {
                                    Text { text: "Mass:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.driverMass : "75"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.driverMass = parseFloat(text)
                                    }
                                    Text { text: "kg"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Height:"; color: "#888"; Layout.preferredWidth: 60 }
                                    TextField {
                                        text: Physics ? Physics.driverHeight : "1.8"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.driverHeight = parseFloat(text)
                                    }
                                    Text { text: "m"; color: "#666"; font.pixelSize: 9 }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "ai"; spacing: 4
                        Text {
                            text: "AI CONFIG"
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
                                    Text { text: "Aggression:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 0; to: 100; value: Physics ? Physics.aiAggression : 50
                                        onValueChanged: if (Physics) Physics.aiAggression = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.aiAggression : 50) + "%"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "Skill:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 0; to: 100; value: Physics ? Physics.aiSkill : 80
                                        onValueChanged: if (Physics) Physics.aiSkill = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.aiSkill : 80) + "%"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "Consistency:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 0; to: 100; value: Physics ? Physics.aiConsistency : 90
                                        onValueChanged: if (Physics) Physics.aiConsistency = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.aiConsistency : 90) + "%"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }
                        }
                    }

                    // --- Electronics Panel ---
                    ColumnLayout { visible: activePanel === "electronics"; spacing: 4
                        Text {
                            text: "ELECTRONICS"
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
                                        checked: Physics ? Physics.absEnabled : true
                                        onClicked: if (Physics) Physics.absEnabled = checked
                                    }
                                    Text { text: "ABS"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "ABS Level:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Slider {
                                        from: 0; to: 10; stepSize: 1; value: 5
                                    }
                                    Text { text: "5"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "TC Mode:"; color: "#888"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"]
                                        currentIndex: 5
                                    }
                                }

                                RowLayout {
                                    Text { text: "TC Level:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Slider {
                                        from: 0; to: 10; stepSize: 1; value: 5
                                    }
                                    Text { text: "5"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    CheckBox { checked: true }
                                    Text { text: "Auto Clutch"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    CheckBox { checked: true }
                                    Text { text: "Auto Blip"; color: "#888"; font.pixelSize: 10 }
                                }
                            }
                        }
                    }

                    // --- DRS Panel ---
                    ColumnLayout { visible: activePanel === "drs"; spacing: 4
                        Text {
                            text: "DRS CONFIGURATION"
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
                                        checked: Physics ? Physics.drsEnabled : false
                                        onClicked: if (Physics) Physics.drsEnabled = checked
                                    }
                                    Text { text: "DRS Enabled"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    CheckBox {
                                        checked: Physics ? Physics.drsAutoActivate : false
                                        onClicked: if (Physics) Physics.drsAutoActivate = checked
                                    }
                                    Text { text: "Auto Activate"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Speed:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 100; to: 300; value: Physics ? Physics.drsSpeedThreshold : 180
                                        onValueChanged: if (Physics) Physics.drsSpeedThreshold = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.drsSpeedThreshold : 180) + " km/h"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "Drag Red.:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 0; to: 50; value: Physics ? Physics.drsDragReduction * 100 : 25
                                        onValueChanged: if (Physics) Physics.drsDragReduction = value / 100
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.drsDragReduction * 100 : 25) + "%"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Rectangle {
                                        width: 10; height: 10; radius: 5
                                        color: Physics && Physics.drsActive ? "#44ff44" : "#666"
                                    }
                                    Text { text: "Active: " + (Physics && Physics.drsActive ? "Yes" : "No"); color: "#888"; font.pixelSize: 10 }
                                }
                            }
                        }
                    }

                    // --- Turbo Panel ---
                    ColumnLayout { visible: activePanel === "turbo"; spacing: 4
                        Text {
                            text: "TURBO CHARGER"
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
                                    Text { text: "Type:"; color: "#888"; Layout.preferredWidth: 60 }
                                    ComboBox {
                                        id: turboTypeCombo
                                        Layout.fillWidth: true
                                        model: ["None", "Single Turbo", "Twin Turbo", "Electric Turbo"]
                                        currentIndex: Physics ? Physics.turboType : 1
                                        onCurrentIndexChanged: { if (Physics) Physics.turboType = currentIndex }
                                    }
                                }

                                RowLayout {
                                    Text { text: "Boost:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        id: turboBoostSlider
                                        from: 0; to: 3.0; stepSize: 0.01
                                        value: Physics ? Physics.turboBoostPressure : 1.5
                                        onMoved: { if (Physics) Physics.turboBoostPressure = value }
                                    }
                                    Text { text: (Physics ? Physics.turboBoostPressure : 1.5).toFixed(2) + " bar"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Threshold:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        id: turboThresholdSlider
                                        from: 1000; to: 6000; stepSize: 100
                                        value: Physics ? Physics.turboThreshold : 3000
                                        onMoved: { if (Physics) Physics.turboThreshold = value }
                                    }
                                    Text { text: (Physics ? Physics.turboThreshold : 3000) + " RPM"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Lag:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        id: turboLagSlider
                                        from: 0.1; to: 3.0; stepSize: 0.1
                                        value: Physics ? Physics.turboLag : 0.5
                                        onMoved: { if (Physics) Physics.turboLag = value }
                                    }
                                    Text { text: (Physics ? Physics.turboLag : 0.5).toFixed(2) + " s"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Wastegate:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        id: turboWastegateSlider
                                        from: 0.5; to: 3.0; stepSize: 0.1
                                        value: Physics ? Physics.turboWastegate : 2.0
                                        onMoved: { if (Physics) Physics.turboWastegate = value }
                                    }
                                    Text { text: (Physics ? Physics.turboWastegate : 2.0).toFixed(2) + " bar"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Count:"; color: "#888"; Layout.preferredWidth: 60 }
                                    ComboBox {
                                        id: turboCountCombo
                                        Layout.fillWidth: true
                                        model: ["1", "2"]
                                        currentIndex: Physics ? Physics.turboCount - 1 : 0
                                        onCurrentIndexChanged: { if (Physics) Physics.turboCount = currentIndex + 1 }
                                    }
                                }
                            }
                        }
                    }

                    // --- Hybrid/ERS Panel ---
                    ColumnLayout { visible: activePanel === "hybrid"; spacing: 4
                        Text {
                            text: "HYBRID / ERS"
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
                                        checked: Physics ? Physics.ersEnabled : false
                                        onClicked: if (Physics) Physics.ersEnabled = checked
                                    }
                                    Text { text: "ERS Enabled"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Architecture:"; color: "#888"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["None", "F1 2014", "F1 2026", "LMP1", "LMDh", "Road Mild", "Road Full", "Road Plug-In", "Electric"]
                                        currentIndex: Physics ? Physics.ersArchitecture : 0
                                        onCurrentIndexChanged: if (Physics) Physics.ersArchitecture = currentIndex
                                    }
                                }

                                RowLayout {
                                    Text { text: "Mode:"; color: "#888"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Qualifying", "Attack", "Balanced", "Low", "Medium", "High", "Overtake", "Auto"]
                                        currentIndex: Physics ? Math.min(Physics.ersDeploymentMode, 7) : 7
                                        onCurrentIndexChanged: if (Physics) Physics.ersDeploymentMode = currentIndex
                                    }
                                }
                            }
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

                                Text { text: "MGU-K"; color: "#E10600"; font.bold: true; font.pixelSize: 11 }

                                RowLayout {
                                    Text { text: "Power:"; color: "#888"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.ersMgukPower : "120"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.ersMgukPower = parseFloat(text)
                                    }
                                    Text { text: "kW"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Regen:"; color: "#888"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.ersMgukRegen : "120"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.ersMgukRegen = parseFloat(text)
                                    }
                                    Text { text: "kW"; color: "#666"; font.pixelSize: 9 }
                                }
                            }
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

                                Text { text: "MGU-H"; color: "#E10600"; font.bold: true; font.pixelSize: 11 }

                                RowLayout {
                                    Text { text: "Power:"; color: "#888"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.ersMguhPower : "120"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.ersMguhPower = parseFloat(text)
                                    }
                                    Text { text: "kW"; color: "#666"; font.pixelSize: 9 }
                                }
                            }
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

                                Text { text: "BATTERY"; color: "#E10600"; font.bold: true; font.pixelSize: 11 }

                                RowLayout {
                                    Text { text: "Capacity:"; color: "#888"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.ersBatteryCapacity : "4.0"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.ersBatteryCapacity = parseFloat(text)
                                    }
                                    Text { text: "MJ"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "Per Lap:"; color: "#888"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.ersPerLapEnergy : "4.0"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.ersPerLapEnergy = parseFloat(text)
                                    }
                                    Text { text: "MJ"; color: "#666"; font.pixelSize: 9 }
                                }

                                RowLayout {
                                    Text { text: "SoC:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Text {
                                        text: (Physics ? Physics.ersBatterySoc : 50).toFixed(1) + "%"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "Temp:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Text {
                                        text: (Physics ? Physics.ersBatteryTemp : 25).toFixed(1) + "\u00B0C"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "Deployed:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Text {
                                        text: (Physics ? Physics.ersEnergyDeployed : 0).toFixed(2) + " MJ"
                                        color: "#999"; font.pixelSize: 10
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            AppButton {
                                height: 30
                                text: "Activate Attack Mode"
                                bgcolor: Physics && Physics.ersAttackActive ? "#E10600" : "#3e3e42"
                                color: Physics && Physics.ersAttackActive ? "#121212" : "#ffffff"
                                Layout.fillWidth: true
                                enabled: Physics ? Physics.ersAttackAvailable : false
                                onClicked: if (Physics) Physics.activateErsAttack()
                            }

                            Rectangle {
                                width: 10; height: 10; radius: 5
                                color: Physics && Physics.ersAttackActive ? "#44ff44" : "#666"
                            }
                            Text {
                                text: Physics && Physics.ersAttackActive ? "ACTIVE" : "READY"
                                color: Physics && Physics.ersAttackActive ? "#44ff44" : "#666"
                                font.pixelSize: 10; font.bold: true
                            }
                        }
                    }

                    // --- Damage Panel ---
                    ColumnLayout { visible: activePanel === "damage"; spacing: 4
                        Text {
                            text: "DAMAGE MODEL"
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
                                        checked: Physics ? Physics.damageEnabled : false
                                        onClicked: if (Physics) Physics.damageEnabled = checked
                                    }
                                    Text { text: "Enable Damage Model"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Aero:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 12
                                        color: "#1a1a1e"
                                        radius: 2
                                        Rectangle {
                                            width: (Physics ? Physics.aeroDamage : 0) / 100 * parent.width
                                            height: parent.height
                                            color: "#E10600"
                                            radius: 2
                                        }
                                    }
                                    Text { text: (Physics ? Physics.aeroDamage : 0).toFixed(1) + "%"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Engine:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 12
                                        color: "#1a1a1e"
                                        radius: 2
                                        Rectangle {
                                            width: (Physics ? Physics.engineDamage : 100) / 100 * parent.width
                                            height: parent.height
                                            color: Physics && Physics.engineDamage < 30 ? "#ff4444" : "#44ff44"
                                            radius: 2
                                        }
                                    }
                                    Text { text: (Physics ? Physics.engineDamage : 100).toFixed(1) + "%"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Body:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 12
                                        color: "#1a1a1e"
                                        radius: 2
                                        Rectangle {
                                            width: (Physics ? Physics.bodyDamage : 100) / 100 * parent.width
                                            height: parent.height
                                            color: Physics && Physics.bodyDamage < 30 ? "#ff4444" : "#44ff44"
                                            radius: 2
                                        }
                                    }
                                    Text { text: (Physics ? Physics.bodyDamage : 100).toFixed(1) + "%"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    spacing: 8
                                    Rectangle {
                                        width: 10; height: 10; radius: 5
                                        color: Physics && Physics.isEliminated ? "#ff4444" : "#666"
                                    }
                                    Text {
                                        text: Physics && Physics.isEliminated ? "ELIMINATED" : "OK"
                                        color: Physics && Physics.isEliminated ? "#ff4444" : "#44ff44"
                                        font.pixelSize: 10; font.bold: true
                                    }
                                }

                                AppButton {
                                    Layout.fillWidth: true
                                    text: "Reset Damage"
                                    height: 28
                                    bgcolor: "#E10600"
                                    color: "#ffffff"
                                    onClicked: if (Physics) Physics.resetDamage()
                                }
                            }
                        }
                    }

                    // --- Fuel Panel ---
                    ColumnLayout { visible: activePanel === "fuel"; spacing: 4
                        Text {
                            text: "FUEL"
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
                                    Text { text: "Load:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 0; to: Physics ? Physics.fuelCapacity : 110; stepSize: 1
                                        value: Physics ? Physics.fuelKg : 80
                                        onMoved: { if (Physics) Physics.fuelKg = value }
                                    }
                                    Text { text: (Physics ? Physics.fuelKg : 80).toFixed(0) + " kg"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Capacity:"; color: "#888"; Layout.preferredWidth: 60 }
                                    Slider {
                                        from: 10; to: 200; stepSize: 1
                                        value: Physics ? Physics.fuelCapacity : 110
                                        onMoved: { if (Physics) Physics.fuelCapacity = value }
                                    }
                                    Text { text: (Physics ? Physics.fuelCapacity : 110).toFixed(0) + " L"; color: "#E10600"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    CheckBox {
                                        checked: Physics ? Physics.fuelConsumptionEnabled : true
                                        onClicked: if (Physics) Physics.fuelConsumptionEnabled = checked
                                    }
                                    Text { text: "Enable Consumption"; color: "#888"; font.pixelSize: 10 }
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
                                    color: isValid ? "#E10600" : "#ef4444"
                                }
                                Text {
                                    text: isValid ? "Valid" : "Invalid"
                                    color: isValid ? "#E10600" : "#ef4444"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            RowLayout {
                                AppButton {
                                    height: 30
                                    text: "Validate"
                                    bgcolor: isValid ? "#E10600" : "#3e3e42"
                                    color: isValid ? "#121212" : "#aaaaaa"
                                    Layout.fillWidth: true
                                    onClicked: {
                                        if (Physics) {
                                            isValid = Physics.validate()
                                        }
                                    }
                                }
                                AppButton {
                                    height: 30
                                    text: "Export"
                                    bgcolor: "#ff6600"
                                    color: "#ffffff"
                                    Layout.fillWidth: true
                                    onClicked: exportDialog.open()
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
                    text: Physics ? Physics.carName + " | ksEditor Physics" : "ksEditor v1.0 - Physics"
                    color: "#666"
                    font.pixelSize: 10
                }
            }
        }
    }
    }
}
