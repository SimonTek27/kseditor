import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ksEditor.Physics 1.0

Rectangle {
    id: ksKunosACPhysics
    width: 700
    height: 500
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property bool isLoaded: Physics && Physics.carName.length > 0
    property string carModel: Physics ? Physics.carName : ""
    property bool advancedMode: false

    FileDialog {
        id: ksImportDialog
        title: "Open Car Physics"
        nameFilters: ["Car folders (*)", "INI files (*.ini)", "JSON files (*.json)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            if (Physics && Physics.importFromCar(path)) {
                isLoaded = true
                carModel = Physics.carName
                carModelField.text = carModel
                massField.text = Physics.totalMass
                Physics.setCurrentFile(path)
            }
        }
    }

    FileDialog {
        id: ksExportDialog
        title: "Export Car Physics"
        nameFilters: ["INI files (*.ini)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            if (Physics) Physics.exportToCar(path)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5

        RowLayout {
            Text {
                text: qsTr("Standard Physics (Vanilla)")
                color: "#ffffff"
                font.pixelSize: 14
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Button {
                text: advancedMode ? "Basic" : "Advanced"
                width: 70
                height: 24
                onClicked: advancedMode = !advancedMode
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
                text: Physics ? Physics.carName : ""
                placeholderText: "e.g., ksFerrari_FXX_K"
                onTextChanged: if (Physics) Physics.carName = text
            }

            Button {
                text: "Load"
                width: 60
                onClicked: ksImportDialog.open()
            }

            Button {
                text: "Save"
                width: 60
                onClicked: savePhysics()
            }
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
                        text: "Basic Settings"
                        color: "#00aa00"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    RowLayout {
                        Text {
                            text: "Mass (kg):"
                            color: "#888888"
                            width: 120
                        }
                        TextField {
                            id: massField
                            width: 80
                            text: Physics ? Physics.totalMass : "1500"
                            onEditingFinished: if (Physics) Physics.totalMass = parseFloat(text)
                        }
                    }

                    RowLayout {
                        Text {
                            text: "Power (hp):"
                            color: "#888888"
                            width: 120
                        }
                        TextField {
                            width: 80
                            text: Physics ? Math.round(Physics.maxPowerKw * 1.34102) : "500"
                            onEditingFinished: if (Physics) Physics.maxPowerKw = parseFloat(text) / 1.34102
                        }
                    }

                    RowLayout {
                        Text {
                            text: "Torque (Nm):"
                            color: "#888888"
                            width: 120
                        }
                        TextField {
                            width: 80
                            text: Physics ? Physics.maxTorqueNm : "400"
                            onEditingFinished: if (Physics) Physics.maxTorqueNm = parseFloat(text)
                        }
                    }

                    Item { height: 10 }

                    Text {
                        text: "Suspension"
                        color: "#00aa00"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    RowLayout {
                        Text {
                            text: "Front Type:"
                            color: "#888888"
                            width: 120
                        }
                        ComboBox {
                            width: 100
                            model: ["Pushrod", "Pullrod", "McPherson", "Double Wishbone"]
                        }
                    }

                    RowLayout {
                        Text {
                            text: "Rear Type:"
                            color: "#888888"
                            width: 120
                        }
                        ComboBox {
                            width: 100
                            model: ["Pushrod", "Pullrod", "McPherson", "Double Wishbone"]
                        }
                    }

                    RowLayout {
                        Text {
                            text: "Spring Rate (N/m):"
                            color: "#888888"
                            width: 120
                        }
                        TextField {
                            width: 80
                            text: Physics ? Physics.suspensionFrontSpring : "100000"
                            onEditingFinished: if (Physics) Physics.suspensionFrontSpring = parseFloat(text)
                        }
                    }

                    RowLayout {
                        Text {
                            text: "Damping (Bump/Rebound):"
                            color: "#888888"
                            width: 120
                        }
                        TextField { width: 35; text: "3000" }
                        TextField { width: 35; text: "2500" }
                    }

                    Item { height: 10 }

                    Text {
                        text: "Aerodynamics"
                        color: "#00aa00"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    RowLayout {
                        Text {
                            text: "Downforce (N):"
                            color: "#888888"
                            width: 120
                        }
                        TextField {
                            width: 80
                            text: Physics ? Physics.frontDownforce : "8000"
                            onEditingFinished: if (Physics) Physics.frontDownforce = parseFloat(text)
                        }
                        Text { text: "Front"; color: "#666666" }
                        TextField {
                            width: 80
                            text: Physics ? Physics.rearDownforce : "3000"
                            onEditingFinished: if (Physics) Physics.rearDownforce = parseFloat(text)
                        }
                        Text { text: "Rear"; color: "#666666" }
                    }

                    RowLayout {
                        Text {
                            text: "Drag Coefficient:"
                            color: "#888888"
                            width: 120
                        }
                        TextField {
                            width: 80
                            text: Physics ? Physics.drag : "0.35"
                            onEditingFinished: if (Physics) Physics.drag = parseFloat(text)
                        }
                    }

                    if (advancedMode) {
                        Item { height: 10 }

                        Text {
                            text: "Advanced Settings"
                            color: "#ffaa00"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        RowLayout {
                            Text {
                                text: "Anti-Roll Front:"
                                color: "#888888"
                                width: 120
                            }
                            TextField { width: 80; text: "50000" }
                        }

                        RowLayout {
                            Text {
                                text: "Anti-Roll Rear:"
                                color: "#888888"
                                width: 120
                            }
                            TextField { width: 80; text: "40000" }
                        }

                        RowLayout {
                            Text {
                                text: "Camber (deg):"
                                color: "#888888"
                                width: 120
                            }
                            TextField { width: 35; text: "-2.5" }
                            Text { text: "Front"; color: "#666666" }
                            TextField { width: 35; text: "-1.5" }
                            Text { text: "Rear"; color: "#666666" }
                        }

                        RowLayout {
                            Text {
                                text: "Toe (deg):"
                                color: "#888888"
                                width: 120
                            }
                            TextField { width: 35; text: "0.1" }
                            Text { text: "Front"; color: "#666666" }
                            TextField { width: 35; text: "0.2" }
                            Text { text: "Rear"; color: "#666666" }
                        }

                        RowLayout {
                            Text {
                                text: "Caster (deg):"
                                color: "#888888"
                                width: 120
                            }
                            TextField { width: 80; text: "5.0" }
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

            Item { Layout.fillWidth: true }

            Button {
                text: "Export ini"
                width: 80
                onClicked: exportIni()
            }
        }
    }

    function loadPhysics() {
        ksImportDialog.open();
    }

    function savePhysics() {
        if (carModel.length > 0) {
            if (Physics) Physics.exportToCar(carModel);
            console.log("Physics saved to: " + carModel);
        } else {
            ksExportDialog.open();
        }
    }

    function exportIni() {
        if (carModel.length > 0) {
            if (Physics) Physics.exportToCar(carModel);
            console.log("Physics exported to INI: " + carModel);
        } else {
            ksExportDialog.open();
        }
    }
}
