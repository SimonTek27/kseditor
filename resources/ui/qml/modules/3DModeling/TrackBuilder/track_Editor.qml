import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: trackEditor
    width: 1280
    height: 720
    color: "#121212"

    property string activePanel: "map"
    property string activeTool: "select"
    property var trackData: ({})
    property real trackLength: Modeler ? Modeler.getTrackLength() : 3500
    property real avgWidth: Modeler ? Modeler.getTrackWidth() : 12
    property string currentFile: Modeler ? Modeler.currentFile : "track.ini"
    property bool showGrid: true
    property bool showOverlay: true
    property real zoom: 1.0

    function setTrackData(data) { trackData = data }

    FileDialog {
        id: trackOpenDialog
        title: "Open Track Project"
        fileMode: FileDialog.OpenFolder
        onAccepted: {
            if (Modeler) {
                Modeler.openTrackProject(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: trackExportDialog
        title: "Export Track"
        nameFilters: ["KN5 files (*.kn5)", "FBX files (*.fbx)", "GLB files (*.glb)", "All files (*)"]
        onAccepted: {
            if (Modeler) {
                Modeler.exportTrack(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Toolbar ---
        Rectangle {
            height: 44
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                spacing: 15

                Text {
                    text: Modeler ? Modeler.currentTrackName : "TRE CORSA"
                    color: "#E10600"
                    font.pixelSize: 14
                    font.bold: true
                }

                Rectangle { width: 1; height: 20; color: "#444444" }

                KsButton {
                    text: "New"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: {
                        if (Modeler) Modeler.newTrackProject()
                    }
                }
                KsButton {
                    text: "Open"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: trackOpenDialog.open()
                }
                KsButton {
                    text: "Save"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: {
                        if (Modeler) Modeler.saveTrackProject()
                    }
                }
                KsButton {
                    text: "Export"
                    flat: true
                    height: 32
                    bgcolor: "#ff6600"
                    color: "#ffffff"
                    onClicked: trackExportDialog.open()
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

            // --- Left Panel: Tools ---
            Rectangle {
                width: 56
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 4

                    KsButton {
                        height: 48
                        width: 48
                        text: "↗"
                        bgcolor: activeTool === "select" ? "#E10600" : "#3e3e42"
                        color: activeTool === "select" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 48
                        width: 48
                        text: "📍"
                        bgcolor: activeTool === "place" ? "#E10600" : "#3e3e42"
                        color: activeTool === "place" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 48
                        width: 48
                        text: "🛣️"
                        bgcolor: activeTool === "road" ? "#E10600" : "#3e3e42"
                        color: activeTool === "road" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 48
                        width: 48
                        text: "🖌️"
                        bgcolor: activeTool === "paint" ? "#E10600" : "#3e3e42"
                        color: activeTool === "paint" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 48
                        width: 48
                        text: "🏔️"
                        bgcolor: activeTool === "terrain" ? "#E10600" : "#3e3e42"
                        color: activeTool === "terrain" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 48
                        width: 48
                        text: "🌲"
                        bgcolor: activeTool === "props" ? "#E10600" : "#3e3e42"
                        color: activeTool === "props" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 48
                        width: 48
                        text: "💡"
                        bgcolor: activeTool === "lights" ? "#E10600" : "#3e3e42"
                        color: activeTool === "lights" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 48
                        width: 48
                        text: "🚧"
                        bgcolor: activeTool === "barriers" ? "#E10600" : "#3e3e42"
                        color: activeTool === "barriers" ? "#121212" : "#ffffff"
                    }

                    Item { Layout.fillHeight: true }

                    KsButton {
                        height: 48
                        width: 48
                        text: "⚙️"
                        bgcolor: "transparent"
                        color: "#ffffff"
                    }
                }
            }

            // --- Left Panel: Panels List ---
            Rectangle {
                width: 180
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 4

                    Text {
                        text: "TOOLS"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton {
                        height: 28
                        text: "Satellite Import"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        font.pixelSize: 11
                    }
                    KsButton {
                        height: 28
                        text: "Terrain"
                        bgcolor: activePanel === "terrain" ? "#E10600" : "transparent"
                        color: activePanel === "terrain" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Texture Paint"
                        bgcolor: activePanel === "paint" ? "#E10600" : "transparent"
                        color: activePanel === "paint" ? "#121212" : "#ffffff"
                    }

                    Rectangle { height: 10 }

                    Text {
                        text: "TRACK"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton {
                        height: 28
                        text: "Map"
                        bgcolor: activePanel === "map" ? "#E10600" : "transparent"
                        color: activePanel === "map" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Roads"
                        bgcolor: activePanel === "roads" ? "#E10600" : "transparent"
                        color: activePanel === "roads" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Surfaces"
                        bgcolor: activePanel === "surfaces" ? "#E10600" : "transparent"
                        color: activePanel === "surfaces" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Kerbs"
                        bgcolor: activePanel === "kerbs" ? "#E10600" : "transparent"
                        color: activePanel === "kerbs" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Barriers"
                        bgcolor: activePanel === "barriers" ? "#E10600" : "transparent"
                        color: activePanel === "barriers" ? "#121212" : "#ffffff"
                    }

                    Rectangle { height: 10 }

                    Text {
                        text: "ENVIRONMENT"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton { height: 28; text: "Trees"; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 28; text: "Nature Props"; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 28; text: "Buildings"; bgcolor: "transparent"; color: "#ffffff" }

                    Rectangle { height: 10 }

                    Text {
                        text: "PHYSICS"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton { height: 28; text: "Start/Pits"; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 28; text: "Race Line"; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 28; text: "AI Lines"; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 28; text: "Physics Roads"; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 28; text: "Checkpoints"; bgcolor: "transparent"; color: "#ffffff" }

                    Rectangle { height: 10 }

                    Text {
                        text: "CSP CONFIG"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton {
                        height: 28
                        text: "Track Lights"
                        bgcolor: activePanel === "cspLights" ? "#E10600" : "transparent"
                        color: activePanel === "cspLights" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Materials"
                        bgcolor: activePanel === "cspMaterials" ? "#E10600" : "transparent"
                        color: activePanel === "cspMaterials" ? "#121212" : "#ffffff"
                    }
                }
            }

            // --- Center: Map Viewport ---
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#2a2a2a"

                // Map background (satellite simulation)
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#4a708b" }
                        GradientStop { position: 0.4; color: "#87ceeb" }
                        GradientStop { position: 0.45; color: "#3d5c3d" }
                        GradientStop { position: 1.0; color: "#2d4a2d" }
                    }
                }

                // Grid overlay
                if (showGrid) {
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"

                        Canvas {
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.strokeStyle = "#ffffff22";
                                ctx.lineWidth = 1;
                                var spacing = 50 * zoom;
                                for (var x = 0; x < width; x += spacing) {
                                    ctx.beginPath();
                                    ctx.moveTo(x, 0);
                                    ctx.lineTo(x, height);
                                    ctx.stroke();
                                }
                                for (var y = 0; y < height; y += spacing) {
                                    ctx.beginPath();
                                    ctx.moveTo(0, y);
                                    ctx.lineTo(width, y);
                                    ctx.stroke();
                                }
                            }
                        }
                    }
                }

                // Track preview (road lines simulation)
                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.strokeStyle = "#ff6600";
                        ctx.lineWidth = 8;
                        ctx.beginPath();
                        ctx.moveTo(150, 400);
                        ctx.quadraticTo(300, 350, 400, 400);
                        ctx.quadraticTo(500, 450, 600, 400);
                        ctx.quadraticTo(700, 350, 850, 400);
                        ctx.stroke();

                        ctx.strokeStyle = "#ffff0066";
                        ctx.lineWidth = 2;
                        ctx.setLineDash([10, 10]);
                        ctx.beginPath();
                        ctx.moveTo(150, 400);
                        ctx.quadraticTo(300, 350, 400, 400);
                        ctx.quadraticTo(500, 450, 600, 400);
                        ctx.quadraticTo(700, 350, 850, 400);
                        ctx.stroke();
                        ctx.setLineDash([]);
                    }
                }

                // Map controls
                RowLayout {
                    anchors { right: parent.right; rightMargin: 10; top: parent.top; topMargin: 10 }
                    spacing: 4

                    KsButton {
                        width: 32
                        height: 32
                        text: "+"
                        bgcolor: "#252526"
                        color: "#ffffff"
                    }
                    KsButton {
                        width: 32
                        height: 32
                        text: "-"
                        bgcolor: "#252526"
                        color: "#ffffff"
                    }
                    KsButton {
                        width: 32
                        height: 32
                        text: "⟲"
                        bgcolor: "#252526"
                        color: "#ffffff"
                    }
                }

                // View options
                RowLayout {
                    anchors { left: parent.left; leftMargin: 10; bottom: parent.bottom; bottomMargin: 10 }
                    spacing: 4

                    KsButton {
                        height: 28
                        text: "Satellite"
                        bgcolor: "#E10600"
                        color: "#121212"
                    }
                    KsButton {
                        height: 28
                        text: "Terrain"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Hybrid"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                    }

                    Rectangle { width: 20 }

                    KsButton {
                        height: 28
                        text: "Grid"
                        bgcolor: showGrid ? "#E10600" : "transparent"
                        color: showGrid ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Overlay"
                        bgcolor: showOverlay ? "#E10600" : "transparent"
                        color: showOverlay ? "#121212" : "#ffffff"
                    }
                }

                // Coordinates display
                Text {
                    anchors { left: parent.left; leftMargin: 10; top: parent.top; topMargin: 10 }
                    text: "X: 450.32  Y: 234.56"
                    color: "#E10600"
                    font.pixelSize: 11
                    font.family: "Courier"
                }

                // Zoom display
                Text {
                    anchors { right: parent.right; rightMargin: 10; bottom: parent.bottom; bottomMargin: 10 }
                    text: "Zoom: " + (zoom * 100).toFixed(0) + "%"
                    color: "#888888"
                    font.pixelSize: 10
                }
            }

            // --- Right Panel: Properties ---
            Rectangle {
                width: 280
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    if (activeTool === "terrain") {
                        Text {
                            text: "TERRAIN TOOLS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        Text {
                            text: "Brush Size"
                            color: "#888888"
                            font.pixelSize: 11
                        }
                        RowLayout {
                            Slider { from: 1; to: 100; value: 30; Layout.fillWidth: true }
                            Text { text: "30"; color: "#E10600" }
                        }

                        Text {
                            text: "Brush Strength"
                            color: "#888888"
                            font.pixelSize: 11
                        }
                        RowLayout {
                            Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true }
                            Text { text: "50%"; color: "#E10600" }
                        }

                        KsButton { height: 32; text: "Raise Terrain"; bgcolor: "transparent"; color: "#ffffff" }
                        KsButton { height: 32; text: "Lower Terrain"; bgcolor: "transparent"; color: "#ffffff" }
                        KsButton { height: 32; text: "Smooth"; bgcolor: "transparent"; color: "#ffffff" }
                        KsButton { height: 32; text: "Flatten"; bgcolor: "transparent"; color: "#ffffff" }
                    }

                    if (activeTool === "road") {
                        Text {
                            text: "ROAD SETTINGS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        Text {
                            text: "Width"
                            color: "#888888"
                            font.pixelSize: 11
                        }
                        RowLayout {
                            Slider { from: 5; to: 20; value: 12; Layout.fillWidth: true }
                            Text { text: "12m"; color: "#E10600" }
                        }

                        Text {
                            text: "Segments"
                            color: "#888888"
                            font.pixelSize: 11
                        }
                        RowLayout {
                            Slider { from: 1; to: 10; value: 4; Layout.fillWidth: true }
                            Text { text: "4"; color: "#E10600" }
                        }

                        Rectangle { height: 10; color: "transparent" }

                        Text {
                            text: "Spline Type"
                            color: "#888888"
                            font.pixelSize: 11
                        }
                        RowLayout {
                            KsButton { height: 28; text: "Tangent"; bgcolor: "#E10600"; color: "#121212" }
                            KsButton { height: 28; text: "Bezier"; bgcolor: "transparent"; color: "#ffffff" }
                        }

                        RowLayout {
                            CheckBox { checked: true }
                            Text { text: "Kerbs"; color: "#bbbbbb" }
                        }
                        RowLayout {
                            CheckBox { checked: false }
                            Text { text: "Barriers"; color: "#bbbbbb" }
                        }
                    }

                    if (activeTool === "paint") {
                        Text {
                            text: "TEXTURE PAINT"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        Text {
                            text: "Brush Size"
                            color: "#888888"
                            font.pixelSize: 11
                        }
                        RowLayout {
                            Slider { from: 1; to: 100; value: 50; Layout.fillWidth: true }
                            Text { text: "50"; color: "#E10600" }
                        }

                        Text {
                            text: "Texture"
                            color: "#888888"
                            font.pixelSize: 11
                        }

                        RowLayout {
                            spacing: 4
                            Rectangle { width: 40; height: 40; radius: 4; color: "#333333"; border.color: "#E10600"; border.width: 2 }
                            Rectangle { width: 40; height: 40; radius: 4; color: "#444444" }
                            Rectangle { width: 40; height: 40; radius: 4; color: "#444444" }
                            Rectangle { width: 40; height: 40; radius: 4; color: "#444444" }
                        }

                        Text {
                            text: "Automask"
                            color: "#888888"
                            font.pixelSize: 11
                        }

                        RowLayout {
                            CheckBox { checked: true }
                            Text { text: "Road"; color: "#bbbbbb" }
                        }
                        RowLayout {
                            CheckBox { checked: true }
                            Text { text: "Slopes"; color: "#bbbbbb" }
                        }
                        RowLayout {
                            CheckBox { checked: false }
                            Text { text: "Colors"; color: "#bbbbbb" }
                        }
                    }

                    if (activePanel === "cspLights") {
                        Text {
                            text: "CSP TRACK LIGHTS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                            ColumnLayout {
                                width: parent ? parent.width - 20 : 800
                                spacing: 15

                                Rectangle {
                                    Layout.fillWidth: true
                                    color: "#252526"
                                    border.color: "#3e3e42"
                                    border.width: 1

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 10

                                        Text {
                                            text: "LIGHT SERIES"
                                            color: "#E10600"
                                            font.bold: true
                                            font.pixelSize: 12
                                        }

                                        RowLayout {
                                            Text { text: "Description:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "pit_lights"; Layout.fillWidth: true }
                                        }

                                        RowLayout {
                                            Text { text: "Meshes:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "DOMEHALO, some?material"; Layout.fillWidth: true }
                                        }

                                        RowLayout {
                                            Text { text: "Color:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "1, 0.7, 0.4, 30"; Layout.fillWidth: true }
                                        }

                                        Rectangle { height: 10 }

                                        RowLayout {
                                            Text { text: "Direction:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "0, -1, 0"; width: 80 }
                                        }

                                        RowLayout {
                                            Text { text: "Offset:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "0, 0, 0"; width: 80 }
                                        }

                                        Rectangle { height: 10 }

                                        Text {
                                            text: "SPOT SETTINGS"
                                            color: "#888888"
                                            font.pixelSize: 11
                                        }

                                        RowLayout {
                                            Text { text: "Spot Angle:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            Slider { from: 30; to: 180; value: 150; Layout.fillWidth: true }
                                            Text { text: "150"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            Text { text: "Sharpness:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            Slider { from: 0; to: 1; value: 0.7; Layout.fillWidth: true }
                                            Text { text: "0.7"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        Rectangle { height: 10 }

                                        Text {
                                            text: "RANGE & FADE"
                                            color: "#888888"
                                            font.pixelSize: 11
                                        }

                                        RowLayout {
                                            Text { text: "Range:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            Slider { from: 5; to: 50; value: 20; Layout.fillWidth: true }
                                            Text { text: "20m"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            Text { text: "Fade At:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            Slider { from: 100; to: 2000; value: 700; Layout.fillWidth: true }
                                            Text { text: "700m"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            Text { text: "Fade Smooth:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            Slider { from: 0; to: 200; value: 100; Layout.fillWidth: true }
                                            Text { text: "100"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        Rectangle { height: 10 }

                                        RowLayout {
                                            Text { text: "Cluster:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            Slider { from: 1; to: 50; value: 10; Layout.fillWidth: true }
                                            Text { text: "10"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            Text { text: "Diffuse:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            Slider { from: 0; to: 1; value: 0.5; Layout.fillWidth: true }
                                            Text { text: "0.5"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                    }
                                }

                                Item { height: 20 }
                            }
                        }
                    }

                    if (activePanel === "cspMaterials") {
                        Text {
                            text: "MATERIAL ADJUSTMENTS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                            ColumnLayout {
                                width: parent ? parent.width - 20 : 800
                                spacing: 15

                                Rectangle {
                                    Layout.fillWidth: true
                                    color: "#252526"
                                    border.color: "#3e3e42"
                                    border.width: 1

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 10

                                        Text {
                                            text: "MATERIAL KEY VALUES"
                                            color: "#E10600"
                                            font.bold: true
                                            font.pixelSize: 12
                                        }

                                        RowLayout {
                                            Text { text: "Meshes:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "gr_?KSLAYER?"; Layout.fillWidth: true }
                                        }

                                        RowLayout {
                                            Text { text: "Materials:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "water"; Layout.fillWidth: true }
                                        }

                                        Rectangle { height: 10 }

                                        Text {
                                            text: "KEY 0"
                                            color: "#888888"
                                            font.pixelSize: 11
                                        }

                                        RowLayout {
                                            Text { text: "Property:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            ComboBox {
                                                Layout.fillWidth: true
                                                model: ["ksEmissive", "ksAlphaRef", "ksSpecular", "ksSpecularEXP", "ksDiffuse", "ksAmbient", "fresnelC", "fresnelMaxLevel", "fresnelEXP"]
                                            }
                                        }

                                        RowLayout {
                                            Text { text: "Value On:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            TextField { text: "255,255,255,0.01"; Layout.fillWidth: true }
                                        }

                                        RowLayout {
                                            Text { text: "Value Off:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            TextField { text: "0, 0, 0"; Layout.fillWidth: true }
                                        }

                                        Rectangle { height: 10 }

                                        Text {
                                            text: "KEY 1"
                                            color: "#888888"
                                            font.pixelSize: 11
                                        }

                                        RowLayout {
                                            Text { text: "Property:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            ComboBox {
                                                Layout.fillWidth: true
                                                model: ["", "ksEmissive", "ksAlphaRef", "ksSpecular", "ksSpecularEXP", "ksDiffuse", "ksAmbient"]
                                            }
                                        }

                                        RowLayout {
                                            Text { text: "Value On:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            TextField { text: ""; Layout.fillWidth: true }
                                        }

                                        RowLayout {
                                            Text { text: "Value Off:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            TextField { text: ""; Layout.fillWidth: true }
                                        }

                                        Rectangle { height: 10 }

                                        Text {
                                            text: "EFFECT RANGE"
                                            color: "#888888"
                                            font.pixelSize: 11
                                        }

                                        RowLayout {
                                            Text { text: "Range:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            Slider { from: 5; to: 100; value: 20; Layout.fillWidth: true }
                                            Text { text: "20m"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            Text { text: "Spot:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            Slider { from: 0; to: 360; value: 75; Layout.fillWidth: true }
                                            Text { text: "75"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            Text { text: "Sharpness:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            Slider { from: 0; to: 1; value: 0.5; Layout.fillWidth: true }
                                            Text { text: "0.5"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        Rectangle { height: 10 }

                                        RowLayout {
                                            CheckBox { checked: false }
                                            Text { text: "Alpha Test"; color: "#bbbbbb" }
                                        }
                                        RowLayout {
                                            CheckBox { checked: false }
                                            Text { text: "Alpha Blend"; color: "#bbbbbb" }
                                        }
                                    }
                                }

                                Item { height: 20 }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    // Quick actions
                    Text {
                        text: "QUICK ACTIONS"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    RowLayout {
                        KsButton { height: 28; text: "Undo"; bgcolor: "transparent"; color: "#ffffff" }
                        KsButton { height: 28; text: "Redo"; bgcolor: "transparent"; color: "#ffffff" }
                    }

                    KsButton { height: 32; text: "Auto-Save"; bgcolor: "#E10600"; color: "#121212" }
                }
            }
        }

        // --- Status Bar ---
        Rectangle {
            height: 28
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text { text: "Ready"; color: "#E10600"; font.pixelSize: 10 }
                Text { text: "|" ; color: "#444444" }
                Text { text: "Nodes: 24"; color: "#888888"; font.pixelSize: 10 }
                Text { text: "|" ; color: "#444444" }
                Text { text: "Length: " + trackLength.toFixed(0) + "m"; color: "#888888"; font.pixelSize: 10 }
                Text { text: "|" ; color: "#444444" }
                Text { text: "Area: 12.4 km²"; color: "#888888"; font.pixelSize: 10 }

                Item { Layout.fillWidth: true }

                Text { text: "TreCorsa Editor"; color: "#666"; font.pixelSize: 10 }
                Text { text: " | "; color: "#444444" }
                Text { text: "v1.0"; color: "#666"; font.pixelSize: 10 }
            }
        }
    }
}
