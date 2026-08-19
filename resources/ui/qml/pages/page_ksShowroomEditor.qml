import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../widgets"

Rectangle {
    id: root
    color: "#121212"

    property real baseUiScale: 1.18
    property real uiZoom: 1.0
    property real uiScale: baseUiScale * uiZoom

    property string statusText: "Ready"
    property string currentFile: ShowroomEditor ? ShowroomEditor.showroomName || "Default" : "Default"
    property int activeRibbonTab: 0
    property string activePanel: "camera"
    property bool isEditingCamera: false
    property bool previewGenerated: false

    Component.onCompleted: {
        if (ShowroomEditor) {
            ShowroomEditor.statusMessage.connect(function(msg) { statusText = msg; })
        }
    }

    FileDialog {
        id: openDialog
        title: "Load Showroom Config"
        nameFilters: ["INI files (*.ini)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            ShowroomEditor.loadShowroom(path)
            currentFile = path.split("/").pop().split("\\").pop()
            statusText = "Loaded showroom config"
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save Showroom Config"
        nameFilters: ["INI files (*.ini)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            ShowroomEditor.saveShowroom(path)
            currentFile = path.split("/").pop().split("\\").pop()
            statusText = "Saved showroom config"
        }
    }

    FileDialog {
        id: exportPreviewDialog
        title: "Export Preview Image"
        nameFilters: ["PNG files (*.png)", "JPG files (*.jpg)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            if (ShowroomEditor) ShowroomEditor.generatePreview(path)
            statusText = "Preview saved: " + path
            previewGenerated = true
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
                    onClicked: openDialog.open()
                }
                AppButton {
                    text: "Export"
                    flat: true
                    height: 30
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: saveDialog.open()
                }
                Rectangle {
                    width: 1; height: 22; color: "#333333"
                }
                    AppButton {
                        height: 30
                        text: "Preview"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        onClicked: exportPreviewDialog.open()
                    }
                    AppButton {
                        height: 30
                        text: "Simulate"
                        flat: true
                        bgcolor: ShowroomEditor && ShowroomEditor.autoRotate ? "#E10600" : "transparent"
                        color: ShowroomEditor && ShowroomEditor.autoRotate ? "#121212" : "#ffffff"
                        onClicked: {
                            if (ShowroomEditor) ShowroomEditor.autoRotate = !ShowroomEditor.autoRotate
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
                { title: "Camera", panel: "camera", groups: [
                    { name: "Position", buttons: [
                        { label: "Distance", icon: "\u2190", cmd: "panel_camera" },
                        { label: "Height", icon: "\u2191", cmd: "panel_camera" },
                        { label: "Angle", icon: "\u2193", cmd: "panel_camera" }
                    ]},
                    { name: "View", buttons: [
                        { label: "FOV", icon: "\u25CB", cmd: "panel_camera" },
                        { label: "Speed", icon: "\u2192", cmd: "panel_camera" },
                        { label: "Rotate", icon: "\u27F3", cmd: "panel_camera" }
                    ]}
                ]},
                { title: "Lighting", panel: "lighting", groups: [
                    { name: "Sun", buttons: [
                        { label: "Color", icon: "\u2600", cmd: "panel_lighting" },
                        { label: "Intensity", icon: "\u263F", cmd: "panel_lighting" }
                    ]},
                    { name: "Ambient", buttons: [
                        { label: "Color", icon: "\u263C", cmd: "panel_lighting" },
                        { label: "Intensity", icon: "\u2191", cmd: "panel_lighting" }
                    ]}
                ]},
                { title: "Background", panel: "background", groups: [
                    { name: "Settings", buttons: [
                        { label: "Path", icon: "\u2723", cmd: "panel_background" },
                        { label: "Color", icon: "\u25A0", cmd: "panel_background" },
                        { label: "Blur", icon: "\u2218", cmd: "panel_background" }
                    ]}
                ]},
                { title: "Cameras", panel: "cameras", groups: [
                    { name: "Manage", buttons: [
                        { label: "Add", icon: "\u2795", cmd: "panel_cameras" },
                        { label: "Remove", icon: "\u2718", cmd: "panel_cameras" },
                        { label: "Copy", icon: "\u270D", cmd: "panel_cameras" }
                    ]},
                    { name: "Presets", buttons: [
                        { label: "Front", icon: "F", cmd: "panel_cameras" },
                        { label: "Side", icon: "S", cmd: "panel_cameras" },
                        { label: "Rear", icon: "R", cmd: "panel_cameras" }
                    ]}
                ]},
                { title: "Lights", panel: "lights", groups: [
                    { name: "Type", buttons: [
                        { label: "Point", icon: "\u25CF", cmd: "panel_lights" },
                        { label: "Dir", icon: "\u2192", cmd: "panel_lights" },
                        { label: "Spot", icon: "\u2981", cmd: "panel_lights" }
                    ]},
                    { name: "Props", buttons: [
                        { label: "Color", icon: "\u25A0", cmd: "panel_lights" },
                        { label: "Power", icon: "\u26A1", cmd: "panel_lights" },
                        { label: "Size", icon: "\u2218", cmd: "panel_lights" }
                    ]}
                ]},
                { title: "Actions", panel: "actions", groups: [
                    { name: "Actions", buttons: [
                        { label: "Reset", icon: "\u27F3", cmd: "panel_actions" },
                        { label: "Validate", icon: "\u2714", cmd: "panel_actions" },
                        { label: "Generate", icon: "\u2723", cmd: "panel_actions" }
                    ]}
                ]}
            ]

            function panelForTab(index) {
                if (index < 0 || index >= ribbonDefs.length) return ""
                return ribbonDefs[index].panel
            }

            function ribbonCommand(cmd) {
                if (cmd === "new") { if (ShowroomEditor) { ShowroomEditor.resetToDefaults(); statusText = "Reset to defaults" } }
                else if (cmd === "open") { openDialog.open() }
                else if (cmd === "save") { if (ShowroomEditor && ShowroomEditor.showroomName) ShowroomEditor.saveShowroom(""); else saveDialog.open() }
                else if (cmd === "import") { openDialog.open() }
                else if (cmd === "export") { saveDialog.open() }
                else if (cmd.startsWith("panel_")) {
                    var panel = cmd.substring(6)
                    if (panel !== "") activePanel = panel
                    updateRibbonIndex()
                }
            }

            function updateRibbonIndex() {
                for (var i = 0; i < ribbonDefs.length; i++) {
                    if (ribbonDefs[i].panel === activePanel) {
                        activeRibbonTab = i
                        return
                    }
                }
                activeRibbonTab = 0
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
                            }

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

                    AppButton {
                        height: 30
                        text: "Camera"
                        bgcolor: activePanel === "camera" ? "#E10600" : "#3e3e42"
                        color: activePanel === "camera" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "camera"; activeRibbonTab = 1; }
                    }
                    AppButton {
                        height: 30
                        text: "Lighting"
                        bgcolor: activePanel === "lighting" ? "#E10600" : "#3e3e42"
                        color: activePanel === "lighting" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "lighting"; activeRibbonTab = 2; }
                    }
                    AppButton {
                        height: 30
                        text: "Background"
                        bgcolor: activePanel === "background" ? "#E10600" : "#3e3e42"
                        color: activePanel === "background" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "background"; activeRibbonTab = 3; }
                    }
                    AppButton {
                        height: 30
                        text: "Cameras"
                        bgcolor: activePanel === "cameras" ? "#E10600" : "#3e3e42"
                        color: activePanel === "cameras" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "cameras"; activeRibbonTab = 4; }
                    }
                    AppButton {
                        height: 30
                        text: "Lights"
                        bgcolor: activePanel === "lights" ? "#E10600" : "#3e3e42"
                        color: activePanel === "lights" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "lights"; activeRibbonTab = 5; }
                    }
                    AppButton {
                        height: 30
                        text: "Actions"
                        bgcolor: activePanel === "actions" ? "#E10600" : "#3e3e42"
                        color: activePanel === "actions" ? "#121212" : "#ffffff"
                        onClicked: { activePanel = "actions"; activeRibbonTab = 6; }
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
                        text: "Preview"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: exportPreviewDialog.open()
                    }
                    AppButton {
                        height: 30
                        text: "Generate"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: {
                            if (ShowroomEditor) {
                                if (ShowroomEditor.showroomName) {
                                    ShowroomEditor.statusMessage("Preview generated")
                                } else {
                                    ShowroomEditor.statusMessage("Load a showroom first")
                                }
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
                            if (ShowroomEditor) ShowroomEditor.statusMessage("Advanced showroom settings")
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

                Rectangle {
                    anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 8
                    width: 160; height: 48; radius: 4
                    color: "#cc1e1e1e"; border.color: "#444"; border.width: 1

                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 6; spacing: 2
                        Text { text: "Showroom Camera"; color: "#E10600"; font.pixelSize: 10; font.bold: true }
                        Text { text: ShowroomEditor ? (ShowroomEditor.autoRotate ? "Auto Rotate ON" : "Auto Rotate OFF") : "No scene"; color: "#999"; font.pixelSize: 9 }
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.margins: 8
                    width: 220; height: 52; radius: 4
                    color: "#cc1e1e1e"; border.color: "#444"; border.width: 1

                    RowLayout {
                        anchors.fill: parent; anchors.margins: 6; spacing: 8
                        ColumnLayout { spacing: 1
                            Text { text: "Scene: " + (ShowroomEditor ? ShowroomEditor.showroomName || "none" : "none"); color: "#E10600"; font.pixelSize: 9; font.bold: true; elide: Text.ElideRight }
                            Text { text: "Camera Angle: " + (ShowroomEditor ? Math.round(ShowroomEditor.cameraAngle) : 0) + "°"; color: "#999"; font.pixelSize: 9 }
                        }
                        Rectangle { width: 1; height: 28; color: "#444" }
                        ColumnLayout { spacing: 1
                            Text { text: "Camera Distance: " + (ShowroomEditor ? ShowroomEditor.cameraDistance.toFixed(1) : 0) + "m"; color: "#999"; font.pixelSize: 9 }
                            Text { text: "Status: " + statusText; color: statusText === "Ready" ? "#44ff44" : "#ffaa00"; font.pixelSize: 9 }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: ShowroomEditor && ShowroomEditor.showroomName ? ShowroomEditor.showroomName + "\nshowroom loaded" : "Drop INI / KN5 here\nto preview showroom"
                    color: "#666"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    lineHeight: 1.5
                    opacity: ShowroomEditor && ShowroomEditor.showroomName ? 0.3 : 0.5
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

                    // --- Camera Properties Section ---
                    ColumnLayout { visible: activePanel === "camera"; spacing: 4
                        Text {
                            text: "CAMERA SETTINGS"
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
                                    Text { text: "Distance:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Slider {
                                        id: distSlider
                                        from: 1; to: 20; value: ShowroomEditor ? ShowroomEditor.cameraDistance : 5
                                        stepSize: 0.1
                                        Layout.fillWidth: true
                                        onMoved: { if (ShowroomEditor) ShowroomEditor.cameraDistance = value }
                                    }
                                    Text {
                                        text: (ShowroomEditor ? ShowroomEditor.cameraDistance.toFixed(1) : "0") + "m"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "Height:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Slider {
                                        id: heightSlider
                                        from: 0; to: 10; value: ShowroomEditor ? ShowroomEditor.cameraHeight : 1
                                        stepSize: 0.1
                                        Layout.fillWidth: true
                                        onMoved: { if (ShowroomEditor) ShowroomEditor.cameraHeight = value }
                                    }
                                    Text {
                                        text: (ShowroomEditor ? ShowroomEditor.cameraHeight.toFixed(1) : "0") + "m"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "Angle:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Slider {
                                        id: angleSlider
                                        from: 0; to: 90; value: ShowroomEditor ? ShowroomEditor.cameraAngle : 30
                                        stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: { if (ShowroomEditor) ShowroomEditor.cameraAngle = value }
                                    }
                                    Text {
                                        text: (ShowroomEditor ? Math.round(ShowroomEditor.cameraAngle) : 30) + "°"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "FOV:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Slider {
                                        id: fovSlider
                                        from: 20; to: 120; value: ShowroomEditor ? ShowroomEditor.cameraFov : 60
                                        stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: { if (ShowroomEditor) ShowroomEditor.cameraFov = value }
                                    }
                                    Text {
                                        text: (ShowroomEditor ? Math.round(ShowroomEditor.cameraFov) : 60) + "°"
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "Speed:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Slider {
                                        id: speedSlider
                                        from: 0; to: 5; value: ShowroomEditor ? ShowroomEditor.rotateSpeed : 1
                                        stepSize: 0.1
                                        Layout.fillWidth: true
                                        onMoved: { if (ShowroomEditor) ShowroomEditor.rotateSpeed = value }
                                    }
                                    Text {
                                        text: (ShowroomEditor ? ShowroomEditor.rotateSpeed.toFixed(1) : "0")
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    CheckBox {
                                        checked: ShowroomEditor ? ShowroomEditor.autoRotate : false
                                        onClicked: { if (ShowroomEditor) ShowroomEditor.autoRotate = checked }
                                    }
                                    Text { text: "Auto Rotate"; color: "#888"; font.pixelSize: 10 }
                                }
                            }
                        }
                    }

                    // --- Lighting Properties Section ---
                    ColumnLayout { visible: activePanel === "lighting"; spacing: 4
                        Text {
                            text: "LIGHTING"
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
                                    Text { text: "Sun Color:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Rectangle {
                                        width: 24; height: 24; radius: 4
                                        color: ShowroomEditor ? ShowroomEditor.sunColor : "#ffffff"
                                        border.color: "#555"; border.width: 1
                                        MouseArea { anchors.fill: parent; onClicked: sunColorDialog.open() }
                                    }
                                    ColorDialog {
                                        id: sunColorDialog
                                        color: ShowroomEditor ? ShowroomEditor.sunColor : "#ffffff"
                                        onAccepted: { if (ShowroomEditor) ShowroomEditor.sunColor = color.toString() }
                                    }
                                }

                                RowLayout {
                                    Text { text: "Sun Int:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Slider {
                                        id: sunIntSlider
                                        from: 0; to: 3; value: ShowroomEditor ? ShowroomEditor.sunIntensity : 1
                                        stepSize: 0.1
                                        Layout.fillWidth: true
                                        onMoved: { if (ShowroomEditor) ShowroomEditor.sunIntensity = value }
                                    }
                                    Text {
                                        text: (ShowroomEditor ? ShowroomEditor.sunIntensity.toFixed(1) : "0")
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }

                                RowLayout {
                                    Text { text: "Amb Color:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Rectangle {
                                        width: 24; height: 24; radius: 4
                                        color: ShowroomEditor ? ShowroomEditor.ambientColor : "#ffffff"
                                        border.color: "#555"; border.width: 1
                                        MouseArea { anchors.fill: parent; onClicked: ambColorDialog.open() }
                                    }
                                    ColorDialog {
                                        id: ambColorDialog
                                        color: ShowroomEditor ? ShowroomEditor.ambientColor : "#ffffff"
                                        onAccepted: { if (ShowroomEditor) ShowroomEditor.ambientColor = color.toString() }
                                    }
                                }

                                RowLayout {
                                    Text { text: "Amb Int:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Slider {
                                        id: ambIntSlider
                                        from: 0; to: 2; value: ShowroomEditor ? ShowroomEditor.ambientIntensity : 0.5
                                        stepSize: 0.1
                                        Layout.fillWidth: true
                                        onMoved: { if (ShowroomEditor) ShowroomEditor.ambientIntensity = value }
                                    }
                                    Text {
                                        text: (ShowroomEditor ? ShowroomEditor.ambientIntensity.toFixed(1) : "0")
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }
                        }
                    }

                    // --- Background Properties Section ---
                    ColumnLayout { visible: activePanel === "background"; spacing: 4
                        Text {
                            text: "BACKGROUND SETTINGS"
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
                                    Text { text: "Path:"; color: "#888"; Layout.preferredWidth: 70 }
                                    TextField {
                                        id: bgPathField
                                        text: ShowroomEditor ? ShowroomEditor.backgroundPath : ""
                                        Layout.fillWidth: true
                                        font.pixelSize: 11
                                        onEditingFinished: { if (ShowroomEditor) ShowroomEditor.backgroundPath = text }
                                    }
                                    AppButton {
                                        height: 24
                                        text: "..."
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: bgFileDialog.open()
                                    }
                                }

                                FileDialog {
                                    id: bgFileDialog
                                    title: "Select Background"
                                    nameFilters: ["Images (*.png *.jpg *.hdr)", "All files (*)"]
                                    onAccepted: {
                                        var p = selectedFile.toString().replace("file:///", "")
                                        bgPathField.text = p
                                        if (ShowroomEditor) ShowroomEditor.backgroundPath = p
                                    }
                                }

                                RowLayout {
                                    Text { text: "Color:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Rectangle {
                                        width: 24; height: 24; radius: 4
                                        color: ShowroomEditor ? ShowroomEditor.backgroundColor : "#000000"
                                        border.color: "#555"; border.width: 1
                                        MouseArea { anchors.fill: parent; onClicked: bgColorDialog.open() }
                                    }
                                    ColorDialog {
                                        id: bgColorDialog
                                        color: ShowroomEditor ? ShowroomEditor.backgroundColor : "#000000"
                                        onAccepted: { if (ShowroomEditor) ShowroomEditor.backgroundColor = color.toString() }
                                    }
                                }

                                RowLayout {
                                    Text { text: "Blur:"; color: "#888"; Layout.preferredWidth: 70 }
                                    Slider {
                                        from: 0; to: 1; value: 0.5; stepSize: 0.05
                                    }
                                    Text { text: "0.5"; color: "#E10600"; font.pixelSize: 10 }
                                }
                            }
                        }
                    }

                    // --- Cameras List Section ---
                    ColumnLayout { visible: activePanel === "cameras"; spacing: 4
                        Text {
                            text: "CAMERAS"
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

                                Repeater {
                                    model: ShowroomEditor ? ShowroomEditor.getCameras() : []

                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 28
                                        color: ShowroomEditor.selectedCameraIndex === index ? "#3a3a3a" : "#1e1e1e"
                                        radius: 3

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            spacing: 4

                                            Rectangle { width: 8; height: 8; radius: 4; color: modelData.isActive ? "#4CAF50" : "#666" }
                                            Text { text: modelData.name; color: "#ccc"; font.pixelSize: 10; Layout.fillWidth: true }
                                            Text { text: "FOV:" + Math.round(modelData.fov); color: "#666"; font.pixelSize: 9 }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: { ShowroomEditor.selectedCameraIndex = index; }
                                            onDoubleClicked: { ShowroomEditor.selectedCameraIndex = index; }
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 6
                                    AppButton {
                                        height: 26
                                        text: "Add"
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        Layout.fillWidth: true
                                        onClicked: {
                                            if (ShowroomEditor) {
                                                var count = ShowroomEditor.getCameras().length
                                                ShowroomEditor.addCamera({"name": "Camera " + count, "fov": 60})
                                                statusText = "Added camera"
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 26
                                        text: "Remove"
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        Layout.fillWidth: true
                                        enabled: ShowroomEditor ? ShowroomEditor.selectedCameraIndex >= 0 : false
                                        onClicked: {
                                            if (ShowroomEditor) {
                                                ShowroomEditor.removeCamera(ShowroomEditor.selectedCameraIndex)
                                                ShowroomEditor.selectedCameraIndex = -1
                                                statusText = "Removed camera"
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // --- Lights List Section ---
                    ColumnLayout { visible: activePanel === "lights"; spacing: 4
                        Text {
                            text: "LIGHTS"
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

                                Repeater {
                                    model: ShowroomEditor ? ShowroomEditor.getLights() : []

                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 28
                                        color: ShowroomEditor.selectedLightIndex === index ? "#3a3a3a" : "#1e1e1e"
                                        radius: 3

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            spacing: 4

                                            Rectangle { width: 8; height: 8; radius: 4; color: modelData.isActive ? "#4CAF50" : "#666" }
                                            Text { text: modelData.name; color: "#ccc"; font.pixelSize: 10; Layout.fillWidth: true }
                                            Text { text: modelData.type; color: "#666"; font.pixelSize: 9 }
                                            Rectangle { width: 12; height: 12; radius: 2; color: modelData.color }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: { ShowroomEditor.selectedLightIndex = index; }
                                            onDoubleClicked: { ShowroomEditor.selectedLightIndex = index; }
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 6
                                    AppButton {
                                        height: 26
                                        text: "Add"
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        Layout.fillWidth: true
                                        onClicked: {
                                            if (ShowroomEditor) {
                                                var count = ShowroomEditor.getLights().length
                                                ShowroomEditor.addLight({"name": "Light " + count, "type": "point"})
                                                statusText = "Added light"
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 26
                                        text: "Remove"
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        Layout.fillWidth: true
                                        enabled: ShowroomEditor ? ShowroomEditor.selectedLightIndex >= 0 : false
                                        onClicked: {
                                            if (ShowroomEditor) {
                                                ShowroomEditor.removeLight(ShowroomEditor.selectedLightIndex)
                                                ShowroomEditor.selectedLightIndex = -1
                                                statusText = "Removed light"
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // --- Actions Section ---
                    ColumnLayout { visible: activePanel === "actions"; spacing: 4
                        Text {
                            text: "ACTIONS"
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

                                AppButton {
                                    Layout.fillWidth: true
                                    height: 30
                                    text: "Reset to Defaults"
                                    bgcolor: "#3e3e42"
                                    color: "#ffffff"
                                    onClicked: {
                                        if (ShowroomEditor) {
                                            ShowroomEditor.resetToDefaults()
                                            statusText = "Reset to defaults"
                                        }
                                    }
                                }
                                AppButton {
                                    Layout.fillWidth: true
                                    height: 30
                                    text: "Validate Config"
                                    bgcolor: "#E10600"
                                    color: "#121212"
                                    onClicked: {
                                        if (ShowroomEditor) {
                                            var result = ShowroomEditor.validateConfig()
                                            statusText = result.valid ? "Config is valid" : "Error: " + result.error
                                        }
                                    }
                                }
                            }
                        }
                    }

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
                                    width: 10; height: 10; radius: 5
                                    color: ShowroomEditor && ShowroomEditor.isValid ? "#E10600" : "#ef4444"
                                }
                                Text {
                                    text: ShowroomEditor && ShowroomEditor.isValid ? "Valid" : "Check params"
                                    color: ShowroomEditor && ShowroomEditor.isValid ? "#E10600" : "#ef4444"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            AppButton {
                                Layout.fillWidth: true
                                height: 30
                                text: "Validate"
                                bgcolor: ShowroomEditor && ShowroomEditor.isValid ? "#E10600" : "#3e3e42"
                                color: ShowroomEditor && ShowroomEditor.isValid ? "#121212" : "#aaaaaa"
                                onClicked: {
                                    if (ShowroomEditor) {
                                        var result = ShowroomEditor.validateConfig()
                                        statusText = result.valid ? "Config is valid" : "Error: " + result.error
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
                    text: ShowroomEditor ? (ShowroomEditor.showroomName || "No scene loaded") : "ksEditor v1.0 - Showroom"
                    color: "#666"
                    font.pixelSize: 10
                }
            }
        }
    }
    }

    Connections {
        target: ShowroomEditor
        function onShowroomLoaded(name) { statusText = "Loaded: " + name }
        function onShowroomSaved(name) { statusText = "Saved: " + name }
        function onPreviewGenerated(path) { statusText = "Preview saved: " + path; previewGenerated = true }
    }

    Shortcut { sequence: "Ctrl+O"; onActivated: openDialog.open() }
    Shortcut { sequence: "Ctrl+S"; onActivated: { if (ShowroomEditor && ShowroomEditor.showroomName) ShowroomEditor.saveShowroom(""); else saveDialog.open() } }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: saveDialog.open() }
}
