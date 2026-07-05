import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: carEditor
    width: 1280
    height: 720
    color: "#121212"

    property string activePanel: "basic"
    property string carType: Modeler ? Modeler.currentCarType : "GT3"
    property var carData: ({})
    property string currentFile: Modeler ? Modeler.currentFile : "car.ini"

    function setCarData(data) { carData = data }

    FileDialog {
        id: carImportDialog
        title: "Import Car Model"
        nameFilters: ["3D models (*.fbx *.obj *.kn5 *.glb)", "All files (*)"]
        onAccepted: {
            if (Modeler) {
                Modeler.importFile(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: carExportDialog
        title: "Export Car"
        nameFilters: ["KN5 files (*.kn5)", "FBX files (*.fbx)", "GLB files (*.glb)", "OBJ files (*.obj)", "All files (*)"]
        onAccepted: {
            if (Modeler) {
                Modeler.exportFile(selectedFile.toString().replace("file:///", ""))
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

                KsButton {
                    text: "Import"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: carImportDialog.open()
                }
                KsButton {
                    text: "Export"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: carExportDialog.open()
                }
                Rectangle {
                    width: 1
                    height: 20
                    color: "#444444"
                }
                KsButton {
                    text: "Save"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: {
                        if (Modeler) Modeler.saveProject()
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

                    KsButton {
                        height: 28
                        text: "Basic"
                        bgcolor: activePanel === "basic" ? "#E10600" : "#3e3e42"
                        color: activePanel === "basic" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Engine"
                        bgcolor: activePanel === "engine" ? "#E10600" : "#3e3e42"
                        color: activePanel === "engine" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Transmission"
                        bgcolor: activePanel === "trans" ? "#E10600" : "#3e3e42"
                        color: activePanel === "trans" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Suspension"
                        bgcolor: activePanel === "susp" ? "#E10600" : "#3e3e42"
                        color: activePanel === "susp" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Aerodynamics"
                        bgcolor: activePanel === "aero" ? "#E10600" : "#3e3e42"
                        color: activePanel === "aero" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Brakes"
                        bgcolor: activePanel === "brakes" ? "#E10600" : "#3e3e42"
                        color: activePanel === "brakes" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Tires"
                        bgcolor: activePanel === "tires" ? "#E10600" : "#3e3e42"
                        color: activePanel === "tires" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Electronics"
                        bgcolor: activePanel === "electronics" ? "#E10600" : "#3e3e42"
                        color: activePanel === "electronics" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Fuel"
                        bgcolor: activePanel === "fuel" ? "#E10600" : "#3e3e42"
                        color: activePanel === "fuel" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "CSP Lights"
                        bgcolor: activePanel === "cspLights" ? "#E10600" : "#3e3e42"
                        color: activePanel === "cspLights" ? "#121212" : "#ffffff"
                    }

                    Rectangle { height: 10 }

                    Text {
                        text: "ANALYSIS"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton {
                        height: 28
                        text: "Tire Mgmt"
                        bgcolor: activePanel === "tireMgmt" ? "#E10600" : "#3e3e42"
                        color: activePanel === "tireMgmt" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Fuel Calc"
                        bgcolor: activePanel === "fuelCalc" ? "#E10600" : "#3e3e42"
                        color: activePanel === "fuelCalc" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Lap Time"
                        bgcolor: activePanel === "lapTime" ? "#E10600" : "#3e3e42"
                        color: activePanel === "lapTime" ? "#121212" : "#ffffff"
                    }
                    KsButton {
                        height: 28
                        text: "Telemetry"
                        bgcolor: activePanel === "telemetry" ? "#E10600" : "#3e3e42"
                        color: activePanel === "telemetry" ? "#121212" : "#ffffff"
                    }

                    Rectangle { height: 10 }

                    Text {
                        text: "STRATEGY"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton { height: 28; text: "Pit Strategy"; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 28; text: "Race Strategy"; bgcolor: "transparent"; color: "#ffffff" }

                    Item { Layout.fillHeight: true }

                    Text {
                        text: "CAR TYPE"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    ComboBox {
                        Layout.fillWidth: true
                        model: ["GT3", "GT4", "Formula", "Touring", "Prototype", "Electric", "Hybrid"]
                    }
                }
            }

            // --- Center: Editor View ---
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#2a2a2a"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    if (activePanel === "basic") {
                        Text {
                            text: "BASIC SETUP"
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
                                spacing: 10

                                RowLayout {
                                    Text { text: "Model:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField { text: "Porsche 911 GT3"; Layout.fillWidth: true }
                                }

                                RowLayout {
                                    Text { text: "Class:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["GT3", "GT4", "GTE", "GT2"]
                                    }
                                }

                                RowLayout {
                                    Text { text: "Power:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField { text: "500"; width: 60 }
                                    Text { text: "HP"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Weight:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField { text: "1245"; width: 60 }
                                    Text { text: "kg"; color: "#666"; font.pixelSize: 10 }
                                }
                            }
                        }
                    }

                    if (activePanel === "engine") {
                        Text {
                            text: "ENGINE SETUP"
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
                                spacing: 10

                                Text {
                                    text: "ENGINE MAP"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    Text { text: "Map:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Qualifying", "Race", "Fuel Save", "Custom"]
                                    }
                                }

                                RowLayout {
                                    Text { text: "Throttle:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 0; to: 100; value: 95; Layout.fillWidth: true }
                                    Text { text: "95%"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                Rectangle { height: 10 }

                                Text {
                                    text: "TURBO / HYBRID"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    Text { text: "Boost:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 0; to: 100; value: 80; Layout.fillWidth: true }
                                    Text { text: "80%"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "MGU-H:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 0; to: 100; value: 70; Layout.fillWidth: true }
                                    Text { text: "70%"; color: "#E10600"; font.pixelSize: 11 }
                                }
                            }
                        }
                    }

                    if (activePanel === "trans") {
                        Text {
                            text: "TRANSMISSION SETUP"
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
                                    text: "GEAR RATIOS"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    Text { text: "1st:"; color: "#888"; width: 40 }
                                    TextField { width: 50; text: "3.15" }
                                    Text { text: "2nd:"; color: "#888"; width: 40 }
                                    TextField { width: 50; text: "2.12" }
                                    Text { text: "3rd:"; color: "#888"; width: 40 }
                                    TextField { width: 50; text: "1.76" }
                                    Text { text: "4th:"; color: "#888"; width: 40 }
                                    TextField { width: 50; text: "1.52" }
                                }
                                RowLayout {
                                    Text { text: "5th:"; color: "#888"; width: 40 }
                                    TextField { width: 50; text: "1.32" }
                                    Text { text: "6th:"; color: "#888"; width: 40 }
                                    TextField { width: 50; text: "1.18" }
                                    Text { text: "7th:"; color: "#888"; width: 40 }
                                    TextField { width: 50; text: "1.00" }
                                }

                                Rectangle { height: 10 }

                                Text {
                                    text: "DIFFERENTIAL"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    Text { text: "Final:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField { text: "3.44"; width: 60 }
                                }

                                RowLayout {
                                    Text { text: "Preload:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true }
                                    Text { text: "50%"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Accel:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 0; to: 100; value: 60; Layout.fillWidth: true }
                                    Text { text: "60%"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Decel:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 0; to: 100; value: 40; Layout.fillWidth: true }
                                    Text { text: "40%"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    CheckBox { checked: true }
                                    Text { text: "Clutch Blip"; color: "#bbbbbb" }
                                }
                            }
                        }
                    }

                    if (activePanel === "susp") {
                        Text {
                            text: "SUSPENSION SETUP"
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
                                spacing: 10

                                Text {
                                    text: "FRONT"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    Text { text: "Ride Height:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                    Slider { from: 50; to: 150; value: 85; Layout.fillWidth: true }
                                    Text { text: "85mm"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Spring:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                    Slider { from: 50; to: 200; value: 120; Layout.fillWidth: true }
                                    Text { text: "120 N/mm"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Compression:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                    Slider { from: 1; to: 20; value: 8; Layout.fillWidth: true }
                                    Text { text: "8"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Rebound:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                    Slider { from: 1; to: 20; value: 10; Layout.fillWidth: true }
                                    Text { text: "10"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                Rectangle { height: 10 }

                                Text {
                                    text: "REAR"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    Text { text: "Ride Height:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                    Slider { from: 50; to: 150; value: 90; Layout.fillWidth: true }
                                    Text { text: "90mm"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Spring:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                    Slider { from: 50; to: 200; value: 90; Layout.fillWidth: true }
                                    Text { text: "90 N/mm"; color: "#E10600"; font.pixelSize: 11 }
                                }
                            }
                        }
                    }

                    if (activePanel === "aero") {
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
                                spacing: 10

                                Text {
                                    text: "FRONT WING"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    Text { text: "Angle:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: -5; to: 15; value: 5; Layout.fillWidth: true }
                                    Text { text: "5°"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                Rectangle { height: 5 }

                                Text {
                                    text: "REAR WING"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    Text { text: "Angle:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: -5; to: 25; value: 15; Layout.fillWidth: true }
                                    Text { text: "15°"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Mode:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Fixed", "DRS", "Manual"]
                                    }
                                }

                                Rectangle { height: 10 }

                                Text {
                                    text: "DOWNFORCE"
                                    color: "#E10600"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                RowLayout {
                                    Text { text: "Front:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Text { text: "850 N"; color: "#ffffff"; font.pixelSize: 12 }
                                }

                                RowLayout {
                                    Text { text: "Rear:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Text { text: "1200 N"; color: "#ffffff"; font.pixelSize: 12 }
                                }

                                RowLayout {
                                    Text { text: "Total:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Text { text: "2050 N"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                                }
                            }
                        }
                    }

                    if (activePanel === "brakes") {
                        Text {
                            text: "BRAKE SETUP"
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
                                spacing: 10

                                RowLayout {
                                    Text { text: "Balance:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 50; to: 70; value: 58; Layout.fillWidth: true }
                                    Text { text: "58%"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Pressure:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 50; to: 100; value: 90; Layout.fillWidth: true }
                                    Text { text: "90%"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    CheckBox { checked: true }
                                    Text { text: "ABS"; color: "#bbbbbb" }
                                }

                                RowLayout {
                                    Text { text: "ABS Level:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 1; to: 10; value: 6; Layout.fillWidth: true }
                                    Text { text: "6"; color: "#E10600"; font.pixelSize: 11 }
                                }
                            }
                        }
                    }

                    if (activePanel === "tires") {
                        Text {
                            text: "TIRE SETUP"
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
                                spacing: 10

                                RowLayout {
                                    Text { text: "Compound:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Soft", "Medium", "Hard", "Inter", "Wet"]
                                    }
                                }

                                RowLayout {
                                    Text { text: "Front:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField { text: "2.1"; width: 50 }
                                    Text { text: "bar"; color: "#666"; font.pixelSize: 10 }
                                    Text { text: "L/R"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Rear:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField { text: "1.9"; width: 50 }
                                    Text { text: "bar"; color: "#666"; font.pixelSize: 10 }
                                    Text { text: "L/R"; color: "#888"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Camber F:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: -5; to: 0; value: -2.5; Layout.fillWidth: true }
                                    Text { text: "-2.5°"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Camber R:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: -5; to: 0; value: -2.0; Layout.fillWidth: true }
                                    Text { text: "-2.0°"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Toe F:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: -0.5; to: 0.5; value: 0.1; Layout.fillWidth: true }
                                    Text { text: "0.1°"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Toe R:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: -0.5; to: 0.5; value: 0.2; Layout.fillWidth: true }
                                    Text { text: "0.2°"; color: "#E10600"; font.pixelSize: 11 }
                                }
                            }
                        }
                    }

                    if (activePanel === "electronics") {
                        Text {
                            text: "ELECTRONICS"
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
                                spacing: 10

                                RowLayout {
                                    CheckBox { checked: true }
                                    Text { text: "Traction Control"; color: "#bbbbbb" }
                                }

                                RowLayout {
                                    Text { text: "TC Level:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                    Slider { from: 1; to: 10; value: 5; Layout.fillWidth: true }
                                    Text { text: "5"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                Rectangle { height: 10 }

                                RowLayout {
                                    CheckBox { checked: true }
                                    Text { text: "ABS"; color: "#bbbbbb" }
                                }

                                RowLayout {
                                    Text { text: "ABS Level:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                    Slider { from: 1; to: 10; value: 6; Layout.fillWidth: true }
                                    Text { text: "6"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                Rectangle { height: 10 }

                                RowLayout {
                                    CheckBox { checked: false }
                                    Text { text: "Launch Control"; color: "#bbbbbb" }
                                }

                                RowLayout {
                                    CheckBox { checked: true }
                                    Text { text: "Engine Braking"; color: "#bbbbbb" }
                                }
                            }
                        }
                    }

                    if (activePanel === "fuel") {
                        Text {
                            text: "FUEL SETUP"
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
                                spacing: 10

                                RowLayout {
                                    Text { text: "Load:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 30; to: 100; value: 80; Layout.fillWidth: true }
                                    Text { text: "80%"; color: "#E10600"; font.pixelSize: 11 }
                                }

                                RowLayout {
                                    Text { text: "Amount:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    TextField { text: "80"; width: 60 }
                                    Text { text: "L"; color: "#666"; font.pixelSize: 10 }
                                }

                                RowLayout {
                                    Text { text: "Race Mode:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        Layout.fillWidth: true
                                        model: ["Full", "Stint", "Qualifying"]
                                    }
                                }
                            }
                        }
                    }

                    if (activePanel === "tireMgmt") {
                        Text {
                            text: "TIRE MANAGEMENT"
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
                                spacing: 10

                                RowLayout {
                                    Text { text: "Wear Rate:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Text { text: "Medium"; color: "#ff6600"; font.pixelSize: 12 }
                                }

                                RowLayout {
                                    Text { text: "Degradation:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    Slider { from: 0; to: 100; value: 30; Layout.fillWidth: true }
                                    Text { text: "30%"; color: "#E10600"; font.pixelSize: 11 }
                                }
                            }
                        }
                    }

                    if (activePanel === "cspLights") {
                        Text {
                            text: "CSP LIGHTS & EFFECTS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                            ColumnLayout {
                                width: parent ? parent.width : 800
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
                                            text: "EMISSIVE LIGHTS"
                                            color: "#E10600"
                                            font.bold: true
                                            font.pixelSize: 12
                                        }

                                        RowLayout {
                                            Text { text: "Brake Light:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "20,3,0,20"; Layout.fillWidth: true }
                                            color: "#888"; font.pixelSize: 10
                                        }

                                        RowLayout {
                                            Text { text: "Off Color:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "10,0,0"; Layout.fillWidth: true }
                                        }

                                        RowLayout {
                                            CheckBox { checked: false }
                                            Text { text: "Bind to Headlights"; color: "#bbbbbb" }
                                        }

                                        Rectangle { height: 10 }

                                        Text {
                                            text: "TURN SIGNALS"
                                            color: "#E10600"
                                            font.bold: true
                                            font.pixelSize: 12
                                        }

                                        RowLayout {
                                            Text { text: "Left Color:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "52,40,0"; Layout.fillWidth: true }
                                        }

                                        RowLayout {
                                            Text { text: "Right Color:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            TextField { text: "240,70,20"; Layout.fillWidth: true }
                                        }

                                        RowLayout {
                                            Text { text: "Flash Rate:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            Slider { from: 0.5; to: 5; value: 2.78; Layout.fillWidth: true }
                                            Text { text: "2.78 Hz"; color: "#E10600"; font.pixelSize: 11 }
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
                                        spacing: 10

                                        Text {
                                            text: "BRAKE DISC FX"
                                            color: "#E10600"
                                            font.bold: true
                                            font.pixelSize: 12
                                        }

                                        Text {
                                            text: "FRONT"
                                            color: "#888888"
                                            font.pixelSize: 11
                                        }

                                        RowLayout {
                                            Text { text: "Ambient:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            Slider { from: 0; to: 3; value: 1.6; Layout.fillWidth: true }
                                            Text { text: "1.6"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            Text { text: "Reflection:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            Slider { from: 0; to: 10; value: 2; Layout.fillWidth: true }
                                            Text { text: "2.0"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            Text { text: "Int. Radius:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            TextField { text: "0.105"; width: 60 }
                                        }

                                        Rectangle { height: 10 }

                                        Text {
                                            text: "REAR"
                                            color: "#888888"
                                            font.pixelSize: 11
                                        }

                                        RowLayout {
                                            Text { text: "Ambient:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            Slider { from: 0; to: 3; value: 1.6; Layout.fillWidth: true }
                                            Text { text: "1.6"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            Text { text: "Reflection:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            Slider { from: 0; to: 10; value: 2; Layout.fillWidth: true }
                                            Text { text: "2.0"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            Text { text: "Int. Radius:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                            TextField { text: "0.101"; width: 60 }
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
                                        spacing: 10

                                        Text {
                                            text: "CONDITIONS"
                                            color: "#E10600"
                                            font.bold: true
                                            font.pixelSize: 12
                                        }

                                        RowLayout {
                                            Text { text: "Night:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            ComboBox {
                                                Layout.fillWidth: true
                                                model: ["NIGHT_SMOOTH", "NIGHT", "ALWAYS_ON", "CUSTOM"]
                                            }
                                        }

                                        RowLayout {
                                            CheckBox { checked: false }
                                            Text { text: "Use Flashing"; color: "#bbbbbb" }
                                        }

                                        RowLayout {
                                            Text { text: "Flashing Rate:"; color: "#bbbbbb"; Layout.preferredWidth: 80 }
                                            Slider { from: 0; to: 10; value: 2; Layout.fillWidth: true }
                                            Text { text: "2 Hz"; color: "#E10600"; font.pixelSize: 11 }
                                        }

                                        RowLayout {
                                            CheckBox { checked: false }
                                            Text { text: "Sync Lights"; color: "#bbbbbb" }
                                        }
                                    }
                                }

                                Item { height: 20 }
                            }
                        }
                    }
                }
            }

            // --- Right Panel: Quick Actions ---
            Rectangle {
                width: 200
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    Text {
                        text: "QUICK ACTIONS"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton { height: 28; text: "Reset All"; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 28; text: "Copy Setup"; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 28; text: "Paste Setup"; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 28; text: "Save Preset"; bgcolor: "#E10600"; color: "#121212" }
                    KsButton { height: 28; text: "Load Preset"; bgcolor: "transparent"; color: "#ffffff" }

                    Rectangle { height: 10 }

                    Text {
                        text: "COMPARISON"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton { height: 28; text: "Compare"; bgcolor: "transparent"; color: "#ffffff" }

                    Item { Layout.fillHeight: true }

                    KsButton {
                        height: 36
                        text: "Export Data"
                        bgcolor: "#E10600"
                        color: "#121212"
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
                Text { text: "ksEditor v1.0 - Car"; color: "#666"; font.pixelSize: 10 }
            }
        }
    }
}
