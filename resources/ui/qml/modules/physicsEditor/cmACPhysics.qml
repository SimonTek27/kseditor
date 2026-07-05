import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ksEditor.Physics 1.0

Rectangle {
    id: cmACPhysics
    width: 750
    height: 550
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property bool isLoaded: false
    property string carModel: ""
    property var physicsData: ({})
    property bool debugMode: false

    FileDialog {
        id: cmImportDialog
        title: "Open CM Physics Car"
        nameFilters: ["Car folders (*)", "INI files (*.ini)", "JSON files (*.json)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            if (Physics.importFromCar(path)) {
                isLoaded = true
                carModel = Physics.carName
                carModelField.text = carModel
                Physics.setCurrentFile(path)
            }
        }
    }

    FileDialog {
        id: cmExportDialog
        title: "Export CM Physics Config"
        nameFilters: ["JSON files (*.json)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            Physics.saveProject(path)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5

        RowLayout {
            Text {
                text: qsTr("CM Physics (Custom Extension)")
                color: "#ffffff"
                font.pixelSize: 14
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Button {
                text: debugMode ? "Debug: ON" : "Debug: OFF"
                width: 80
                height: 24
                onClicked: debugMode = !debugMode
            }
        }

        RowLayout {
            Text {
                text: "Car Model:"
                color: "#aaaaaa"
            }

            TextField {
                id: carModelField
                width: 200
                text: carModel
                placeholderText: "e.g., my_car_mod"
                onTextChanged: carModel = text
            }

            Button {
                text: "Load"
                width: 60
                onClicked: cmImportDialog.open()
            }

            Button {
                text: "Save"
                width: 60
                onClicked: savePhysics()
            }

            Button {
                text: "Validate"
                width: 70
                onClicked: validatePhysics()
            }
        }

        TabBar {
            id: cmTabBar
            width: parent.width
            currentIndex: 0

            TabButton { text: "Core" }
            TabButton { text: "Suspension" }
            TabButton { text: "Aerodynamics" }
            TabButton { text: "Engine" }
            TabButton { text: "Transmission" }
            TabButton { text: "Tyres" }
        }

        Rectangle {
            color: "#181818"
            border.color: "#2a2a2a"
            border.width: 1
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                anchors.fill: parent

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Text {
                        text: "Core Physics"
                        color: "#00aaff"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    RowLayout {
                        Text {
                            text: "Dry Mass (kg):"
                            color: "#888888"
                            width: 130
                        }
                        TextField { width: 80; text: "1250" }
                        Text { text: "Payload (kg):"; color: "#666666" }
                        TextField { width: 80; text: "80" }
                    }

                    RowLayout {
                        Text {
                            text: "Mass Distribution:"
                            color: "#888888"
                            width: 130
                        }
                        TextField { width: 50; text: "42" }
                        Text { text: "% front"; color: "#666666" }
                    }

                    RowLayout {
                        Text {
                            text: "CG Height (mm):"
                            color: "#888888"
                            width: 130
                        }
                        TextField { width: 80; text: "350" }
                    }

                    RowLayout {
                        Text {
                            text: "Inertia (kg·m²):"
                            color: "#888888"
                            width: 130
                        }
                        TextField { width: 60; text: "1000" }
                        Text { text: "X"; color: "#666666" }
                        TextField { width: 60; text: "1500" }
                        Text { text: "Y"; color: "#666666" }
                        TextField { width: 60; text: "800" }
                        Text { text: "Z"; color: "#666666" }
                    }

                    Item { height: 10 }

                    Text {
                        text: "Extended Physics Features"
                        color: "#ffaa00"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    RowLayout {
                        CheckBox {
                            text: "Turbo Boost"
                            checked: true
                        }
                        CheckBox {
                            text: "ABS"
                            checked: true
                        }
                        CheckBox {
                            text: "TC"
                            checked: true
                        }
                        CheckBox {
                            text: "EBS"
                            checked: false
                        }
                    }

                    RowLayout {
                        CheckBox {
                            text: "Brake Ducts"
                            checked: true
                        }
                        CheckBox {
                            text: "ERS/Hybrid"
                            checked: false
                        }
                        CheckBox {
                            text: "KERS"
                            checked: false
                        }
                        CheckBox {
                            text: "Push to Pass"
                            checked: false
                        }
                    }

                    RowLayout {
                        CheckBox {
                            text: "Drift Mode"
                            checked: false
                        }
                        CheckBox {
                            text: "Launch Control"
                            checked: true
                        }
                        CheckBox {
                            text: "Fuel Consumption"
                            checked: true
                        }
                        CheckBox {
                            text: "Tyre Wear"
                            checked: true
                        }
                    }

                    Item { height: 10 }

                    Text {
                        text: "Advanced Integration"
                        color: "#aa55ff"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    RowLayout {
                        Text {
                            text: "Physics DLL:"
                            color: "#888888"
                            width: 130
                        }
                        TextField {
                            width: 200
                            text: "cm_physics.dll"
                            placeholderText: "custom_physics.dll"
                        }
                    }

                    RowLayout {
                        Text {
                            text: "Controller:"
                            color: "#888888"
                            width: 130
                        }
                        ComboBox {
                            width: 150
                            model: ["Default", "Custom", "External"]
                        }
                    }

                    RowLayout {
                        Text {
                            text: "Data Telemetry:"
                            color: "#888888"
                            width: 130
                        }
                        ComboBox {
                            width: 150
                            model: ["None", "Basic", "Extended", "Custom"]
                        }
                    }

                    RowLayout {
                        Text {
                            text: "UDP Port:"
                            color: "#888888"
                            width: 130
                        }
                        TextField { width: 80; text: "9999" }
                    }

                    if (debugMode) {
                        Item { height: 10 }

                        Text {
                            text: "Debug Settings"
                            color: "#ff4444"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        RowLayout {
                            CheckBox {
                                text: "Show Forces"
                            }
                            CheckBox {
                                text: "Show Velocities"
                            }
                            CheckBox {
                                text: "Show Pressures"
                            }
                        }

                        RowLayout {
                            Text {
                                text: "Debug Scale:"
                                color: "#888888"
                            }
                            Slider {
                                width: 100
                                from: 0.1
                                to: 10
                                value: 1
                            }
                        }

                        TextArea {
                            width: 600
                            height: 80
                            text: "Debug log output...\nPhysics tick: 16ms\nForces: Fx=1234 N, Fy=567 N, Fz=8900 N\nVelocities: Vx=45 m/s, Vy=0.1 m/s"
                            readOnly: true
                            color: "#00ff00"
                            background: "#0a0a0a"
                        }
                    }
                }
            }
        }

        RowLayout {
            Text {
                text: isLoaded ? "Loaded: " + carModel : "No car loaded"
                color: isLoaded ? "#00aa00" : "#666666"
                font.pixelSize: 11
            }

            Text {
                text: "Extension: CM Physics v1.0"
                color: "#666666"
                font.pixelSize: 11
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Export"
                width: 80
                onClicked: exportConfig()
            }
        }
    }

    function loadPhysics() {
        cmImportDialog.open();
    }

    function savePhysics() {
        if (carModel.length > 0) {
            Physics.exportToCar(carModel);
            console.log("CM physics saved to: " + carModel);
        } else {
            cmExportDialog.open();
        }
    }

    function validatePhysics() {
        var result = Physics.validate();
        console.log("CM physics validation: " + (result ? "PASSED" : "FAILED"));
    }

    function exportConfig() {
        cmExportDialog.open();
    }
}