import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../widgets"
import ksEditor.Physics 1.0

Rectangle {
    id: root
    color: "#121212"

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
                    text: "Import"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: importDialog.open()
                }
                AppButton {
                    text: "Export"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: exportDialog.open()
                }
                Rectangle {
                    width: 1
                    height: 20
                    color: "#444444"
                }
                AppButton {
                    text: "Validate"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: {
                        if (Physics) {
                            isValid = Physics.validate()
                        }
                    }
                }
                Rectangle {
                    width: 1
                    height: 20
                    color: "#444444"
                }
                AppButton {
                    text: "Simulate"
                    flat: true
                    height: 32
                    bgcolor: Physics && Physics.isSimulating ? "#E10600" : "transparent"
                    color: "#ffffff"
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

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 4

                    Text {
                        text: "PANELS"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    AppButton {
                        height: 28
                        text: "Suspension"
                        bgcolor: activePanel === "suspension" ? "#E10600" : "#3e3e42"
                        color: activePanel === "suspension" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "suspension"
                    }
                    AppButton {
                        height: 28
                        text: "Wheels"
                        bgcolor: activePanel === "wheels" ? "#E10600" : "#3e3e42"
                        color: activePanel === "wheels" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "wheels"
                    }
                    AppButton {
                        height: 28
                        text: "Tires"
                        bgcolor: activePanel === "tires" ? "#E10600" : "#3e3e42"
                        color: activePanel === "tires" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "tires"
                    }
                    AppButton {
                        height: 28
                        text: "Aerodynamics"
                        bgcolor: activePanel === "aero" ? "#E10600" : "#3e3e42"
                        color: activePanel === "aero" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "aero"
                    }
                    AppButton {
                        height: 28
                        text: "Engine"
                        bgcolor: activePanel === "engine" ? "#E10600" : "#3e3e42"
                        color: activePanel === "engine" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "engine"
                    }
                    AppButton {
                        height: 28
                        text: "Transmission"
                        bgcolor: activePanel === "trans" ? "#E10600" : "#3e3e42"
                        color: activePanel === "trans" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "trans"
                    }
                    AppButton {
                        height: 28
                        text: "Brakes"
                        bgcolor: activePanel === "brakes" ? "#E10600" : "#3e3e42"
                        color: activePanel === "brakes" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "brakes"
                    }
                    AppButton {
                        height: 28
                        text: "Steering"
                        bgcolor: activePanel === "steering" ? "#E10600" : "#3e3e42"
                        color: activePanel === "steering" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "steering"
                    }
                    AppButton {
                        height: 28
                        text: "Driver"
                        bgcolor: activePanel === "driver" ? "#E10600" : "#3e3e42"
                        color: activePanel === "driver" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "driver"
                    }
                    AppButton {
                        height: 28
                        text: "AI Config"
                        bgcolor: activePanel === "ai" ? "#E10600" : "#3e3e42"
                        color: activePanel === "ai" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "ai"
                    }

                    Rectangle { height: 10 }

                    Text {
                        text: "TOOLS"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    AppButton {
                        height: 28
                        text: "Colliders"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: colliderExportDialog.open()
                    }
                    AppButton {
                        height: 28
                        text: "LODs"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: lodsImportDialog.open()
                    }
                    AppButton {
                        height: 28
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
                        height: 28
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
                        height: 32
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

                            var gridSize = 30
                            var cols = Math.ceil(width / gridSize)
                            var rows = Math.ceil(height / gridSize)
                            var cx = width / 2
                            var cy = height / 2

                            ctx.strokeStyle = "#555555"
                            ctx.lineWidth = 1
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

                            // Axis indicator (bottom-left)
                            var ax = 40
                            var ay = height - 50
                            var alen = 25

                            ctx.strokeStyle = "#ff4444"
                            ctx.lineWidth = 3
                            ctx.beginPath()
                            ctx.moveTo(ax, ay)
                            ctx.lineTo(ax + alen, ay)
                            ctx.stroke()
                            ctx.fillStyle = "#ff4444"
                            ctx.font = "bold 11px monospace"
                            ctx.fillText("X", ax + alen + 4, ay + 4)

                            ctx.strokeStyle = "#44ff44"
                            ctx.beginPath()
                            ctx.moveTo(ax, ay)
                            ctx.lineTo(ax, ay - alen)
                            ctx.stroke()
                            ctx.fillStyle = "#44ff44"
                            ctx.fillText("Y", ax + 4, ay - alen - 4)

                            ctx.strokeStyle = "#4444ff"
                            ctx.beginPath()
                            ctx.moveTo(ax, ay)
                            ctx.lineTo(ax - alen, ay - alen)
                            ctx.stroke()
                            ctx.fillStyle = "#4444ff"
                            ctx.fillText("Z", ax - alen - 16, ay - alen + 4)
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
                    ColumnLayout { visible: activePanel === "suspension"; spacing: 0
                        Text {
                            text: "SUSPENSION PHYSICS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
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
                                    text: "FRONT"
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
                                        id: frontSpringField
                                        text: Physics ? Physics.suspensionFrontSpring : "0"
                                        Layout.fillWidth: true
                                        onEditingFinished: if (Physics) Physics.suspensionFrontSpring = parseFloat(text)
                                    }
                                    Text { text: "N/m"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Comp:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField { text: Physics ? (Physics.suspensionFrontSpring * 0.3).toFixed(1) : "8"; width: 50 }
                                    Text { text: "Reb:"; color: "#bbbbbb" }
                                    TextField { text: Physics ? (Physics.suspensionFrontSpring * 0.5).toFixed(1) : "10"; width: 50 }
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

                    ColumnLayout { visible: activePanel === "wheels"; spacing: 0
                        Text {
                            text: "WHEEL PHYSICS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
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

                                RowLayout {
                                    Text { text: "Diameter:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.wheelDiameter : "26"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.wheelDiameter = parseFloat(text)
                                    }
                                    Text { text: "in"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Width:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.wheelWidth : "12"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.wheelWidth = parseFloat(text)
                                    }
                                    Text { text: "in"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Mass:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.wheelMass : "12"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.wheelMass = parseFloat(text)
                                    }
                                    Text { text: "kg"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Rim:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Single Piece", "Multi Piece", "Forged", "Carbon"]
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "tires"; spacing: 0
                        Text {
                            text: "TIRE PHYSICS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
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

                                RowLayout {
                                    Text { text: "Compound:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
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
                                    Text { text: "Tread:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider {
                                        from: 0; to: 100; value: Physics ? Physics.tireTreadRemaining : 100
                                        onValueChanged: if (Physics) Physics.tireTreadRemaining = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.tireTreadRemaining : 100) + "%"
                                        color: "#E10600"; font.pixelSize: 11
                                    }
                                }

                                Text {
                                    text: "THERMAL"
                                    color: "#666"
                                    font.pixelSize: 10
                                    font.bold: true
                                }

                                RowLayout {
                                    Text { text: "Optimal:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.tireOptimalTemp : "90"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.tireOptimalTemp = parseFloat(text)
                                    }
                                    Text { text: "°C"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Max:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.tireMaxTemp : "120"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.tireMaxTemp = parseFloat(text)
                                    }
                                    Text { text: "°C"; color: "#666"; font.pixelSize: 10 }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "aero"; spacing: 0
                        Text {
                            text: "AERODYNAMICS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
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

                                RowLayout {
                                    Text { text: "Drag:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        id: dragField
                                        text: Physics ? Physics.drag : "0.4"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.drag = parseFloat(text)
                                    }
                                }

                                RowLayout {
                                    Text { text: "Downforce:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        id: downforceField
                                        text: Physics ? Physics.frontDownforce : "1200"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.frontDownforce = parseFloat(text)
                                    }
                                    Text { text: "N"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Balance:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider {
                                        from: 30; to: 70; value: Physics ? Physics.brakeBalance * 100 : 42
                                        onValueChanged: if (Physics) Physics.brakeBalance = value / 100
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.brakeBalance * 100 : 42) + "%"
                                        color: "#E10600"; font.pixelSize: 11
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "engine"; spacing: 0
                        Text {
                            text: "ENGINE PHYSICS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
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

                                RowLayout {
                                    Text { text: "Max RPM:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        id: maxRpmField
                                        text: Physics ? Physics.redlineRPM : "8000"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.redlineRPM = parseInt(text)
                                    }
                                }

                                RowLayout {
                                    Text { text: "Power:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        id: powerField
                                        text: Physics ? Math.round(Physics.maxPowerKw * 1.34102) : "268"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.maxPowerKw = parseFloat(text) / 1.34102
                                    }
                                    Text { text: "HP"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Torque:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        id: torqueField
                                        text: Physics ? Physics.maxTorqueNm : "400"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.maxTorqueNm = parseFloat(text)
                                    }
                                    Text { text: "Nm"; color: "#666"; font.pixelSize: 10 }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "trans"; spacing: 0
                        Text {
                            text: "TRANSMISSION"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
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

                                RowLayout {
                                    Text { text: "Type:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Sequential", "H-Pattern", "Dual Clutch", "CVT"]
                                        currentIndex: Physics ? Physics.transmissionType : 0
                                        onCurrentIndexChanged: if (Physics) Physics.transmissionType = currentIndex
                                    }
                                }

                                RowLayout {
                                    Text { text: "Gears:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.gearCount : "7"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.gearCount = Math.max(1, Math.min(10, parseInt(text)))
                                    }
                                }

                                RowLayout {
                                    Text { text: "Final:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        id: finalDriveField
                                        text: Physics ? Physics.finalDrive : "3.7"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.finalDrive = parseFloat(text)
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "brakes"; spacing: 0
                        Text {
                            text: "BRAKE PHYSICS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
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

                                RowLayout {
                                    Text { text: "Type:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Steel", "Carbon Ceramic", "Carbon Carbon"]
                                    }
                                }

                                RowLayout {
                                    Text { text: "Pressure:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.brakePressure : "150"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.brakePressure = parseFloat(text)
                                    }
                                    Text { text: "bar"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Bias:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider {
                                        from: 50; to: 70; value: Physics ? Physics.brakeBalance * 100 : 58
                                        onValueChanged: if (Physics) Physics.brakeBalance = value / 100
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.brakeBalance * 100 : 58) + "%"
                                        color: "#E10600"; font.pixelSize: 11
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "steering"; spacing: 0
                        Text {
                            text: "STEERING"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
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

                                RowLayout {
                                    Text { text: "Ratio:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider {
                                        from: 10; to: 30; value: Physics ? Physics.steeringRatio : 15
                                        onValueChanged: if (Physics) Physics.steeringRatio = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.steeringRatio : 15) + ":1"
                                        color: "#E10600"; font.pixelSize: 11
                                    }
                                }

                                RowLayout {
                                    Text { text: "Lock:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.steeringLockAngle : "45"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.steeringLockAngle = parseFloat(text)
                                    }
                                    Text { text: "°"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    CheckBox {
                                        checked: Physics ? Physics.powerSteering : true
                                        onClicked: if (Physics) Physics.powerSteering = checked
                                    }
                                    Text { text: "Power Steering"; color: "#bbbbbb"; font.pixelSize: 11 }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "driver"; spacing: 0
                        Text {
                            text: "DRIVER"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
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

                                RowLayout {
                                    Text { text: "Position:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.driverPositionX : "0.1"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.driverPositionX = parseFloat(text)
                                    }
                                    TextField {
                                        text: Physics ? Physics.driverPositionY : "0.5"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.driverPositionY = parseFloat(text)
                                    }
                                    TextField {
                                        text: Physics ? Physics.driverPositionZ : "-0.2"
                                        width: 50
                                        onEditingFinished: if (Physics) Physics.driverPositionZ = parseFloat(text)
                                    }
                                }

                                RowLayout {
                                    Text { text: "Mass:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.driverMass : "75"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.driverMass = parseFloat(text)
                                    }
                                    Text { text: "kg"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Height:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField {
                                        text: Physics ? Physics.driverHeight : "1.8"
                                        width: 60
                                        onEditingFinished: if (Physics) Physics.driverHeight = parseFloat(text)
                                    }
                                    Text { text: "m"; color: "#666"; font.pixelSize: 10 }
                                }
                            }
                        }
                    }

                    ColumnLayout { visible: activePanel === "ai"; spacing: 0
                        Text {
                            text: "AI CONFIG"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
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

                                RowLayout {
                                    Text { text: "Aggression:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider {
                                        from: 0; to: 100; value: Physics ? Physics.aiAggression : 50
                                        onValueChanged: if (Physics) Physics.aiAggression = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.aiAggression : 50) + "%"
                                        color: "#E10600"; font.pixelSize: 11
                                    }
                                }

                                RowLayout {
                                    Text { text: "Skill:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider {
                                        from: 0; to: 100; value: Physics ? Physics.aiSkill : 80
                                        onValueChanged: if (Physics) Physics.aiSkill = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.aiSkill : 80) + "%"
                                        color: "#E10600"; font.pixelSize: 11
                                    }
                                }

                                RowLayout {
                                    Text { text: "Consistency:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider {
                                        from: 0; to: 100; value: Physics ? Physics.aiConsistency : 90
                                        onValueChanged: if (Physics) Physics.aiConsistency = value
                                    }
                                    Text {
                                        text: Math.round(Physics ? Physics.aiConsistency : 90) + "%"
                                        color: "#E10600"; font.pixelSize: 11
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

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            Text {
                                text: "VALIDATION"
                                color: "#666"
                                font.pixelSize: 10
                                font.bold: true
                            }

                            RowLayout {
                                Rectangle {
                                    width: 12
                                    height: 12
                                    radius: 6
                                    color: isValid ? "#E10600" : "#ef4444"
                                }
                                Text {
                                    text: isValid ? "Valid" : "Invalid"
                                    color: isValid ? "#E10600" : "#ef4444"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }

                            RowLayout {
                                AppButton {
                                    height: 28
                                    text: "Validate"
                                    bgcolor: "#E10600"
                                    color: "#121212"
                                    Layout.fillWidth: true
                                    onClicked: {
                                        if (Physics) {
                                            isValid = Physics.validate()
                                        }
                                    }
                                }
                                AppButton {
                                    height: 28
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
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text { text: statusText; color: "#E10600"; font.pixelSize: 10 }
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
