import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.Physics 1.0

Rectangle {
    id: physicsTools
    width: 800
    height: 600
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property string activeTool: "colliders"

    FileDialog {
        id: colliderFileDialog
        title: "Open Collider Mesh"
        nameFilters: ["FBX files (*.fbx)", "KN5 files (*.kn5)", "All files (*)"]
        onAccepted: { if (Physics) Physics.importLOD(selectedFile.toString().replace("file:///", "")) }
    }

    FileDialog {
        id: lodFileDialog
        title: "Open LOD File"
        nameFilters: ["FBX files (*.fbx)", "KN5 files (*.kn5)", "All files (*)"]
        onAccepted: { if (Physics) Physics.importLOD(selectedFile.toString().replace("file:///", "")) }
    }

    FileDialog {
        id: exportDataDialog
        title: "Export Data"
        nameFilters: ["CSV files (*.csv)", "JSON files (*.json)", "All files (*)"]
        onAccepted: {
            if (Physics) Physics.exportData("csv", selectedFile.toString().replace("file:///", ""))
        }
    }

    FileDialog {
        id: colliderMeshDialog
        title: "Open Collider Mesh"
        nameFilters: ["FBX files (*.fbx)", "KN5 files (*.kn5)", "All files (*)"]
        onAccepted: { if (Physics) Physics.importLOD(selectedFile.toString().replace("file:///", "")) }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 36
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8

                Text {
                    text: "PHYSICS TOOLS"
                    color: "#E10600"
                    font.pixelSize: 12
                    font.bold: true
                }
                Item { Layout.fillWidth: true }

                KsButton {
                    text: "Run All"
                    height: 24
                    font.pixelSize: 10
                    bgcolor: "#E10600"
                    color: "#121212"
                    onClicked: { if (Physics) Physics.runAllTools() }
                }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Rectangle {
                width: 140
                color: "#252526"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    Text { text: "TOOLS"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                    KsButton { height: 26; text: "Colliders"; font.pixelSize: 10; bgcolor: activeTool === "colliders" ? "#E10600" : "#3e3e42"; color: activeTool === "colliders" ? "#121212" : "#ffffff"; onClicked: { activeTool = "colliders" } }
                    KsButton { height: 26; text: "LODs"; font.pixelSize: 10; bgcolor: activeTool === "lods" ? "#E10600" : "#3e3e42"; color: activeTool === "lods" ? "#121212" : "#ffffff"; onClicked: { activeTool = "lods" } }
                    KsButton { height: 26; text: "Mirrors"; font.pixelSize: 10; bgcolor: activeTool === "mirrors" ? "#E10600" : "#3e3e42"; color: activeTool === "mirrors" ? "#121212" : "#ffffff"; onClicked: { activeTool = "mirrors" } }
                    KsButton { height: 26; text: "Exhaust"; font.pixelSize: 10; bgcolor: activeTool === "exhaust" ? "#E10600" : "#3e3e42"; color: activeTool === "exhaust" ? "#121212" : "#ffffff"; onClicked: { activeTool = "exhaust" } }
                    KsButton { height: 26; text: "Lights"; font.pixelSize: 10; bgcolor: activeTool === "lights" ? "#E10600" : "#3e3e42"; color: activeTool === "lights" ? "#121212" : "#ffffff"; onClicked: { activeTool = "lights" } }
                    KsButton { height: 26; text: "Interior"; font.pixelSize: 10; bgcolor: activeTool === "interior" ? "#E10600" : "#3e3e42"; color: activeTool === "interior" ? "#121212" : "#ffffff"; onClicked: { activeTool = "interior" } }
                    KsButton { height: 26; text: "Data Export"; font.pixelSize: 10; bgcolor: activeTool === "export" ? "#E10600" : "#3e3e42"; color: activeTool === "export" ? "#121212" : "#ffffff"; onClicked: { activeTool = "export" } }
                    KsButton { height: 26; text: "Paint Config"; font.pixelSize: 10; bgcolor: activeTool === "paint" ? "#E10600" : "#3e3e42"; color: activeTool === "paint" ? "#121212" : "#ffffff"; onClicked: { activeTool = "paint" } }

                    Item { Layout.fillHeight: true }

                    KsButton {
                        text: "Batch Process"
                        height: 28
                        bgcolor: "#ff6600"
                        color: "#121212"
                        onClicked: {
                            if (Physics) Physics.batchProcess(["colliders", "lods", "mirrors", "lights"])
                        }
                    }
                }
            }

            StackLayout {
                id: toolPanels
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: activeTool === "colliders" ? 0
                    : activeTool === "lods" ? 1
                    : activeTool === "mirrors" ? 2
                    : activeTool === "exhaust" ? 3
                    : activeTool === "lights" ? 4
                    : activeTool === "interior" ? 5
                    : activeTool === "export" ? 6
                    : 7

                // 0 — Collider Generator
                ColumnLayout {
                    spacing: 8
                    Text { text: "COLLIDER GENERATOR"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                    RowLayout {
                        KsButton {
                            text: "Generate Colliders"
                            height: 28
                            bgcolor: "#E10600"
                            color: "#121212"
                            onClicked: {
                                if (Physics) Physics.generateColliders(qualityCombo.currentText, modeCombo.currentText, simplifyChk.checked, optimizeChk.checked)
                            }
                        }
                        KsButton {
                            text: "Auto-Generate"
                            height: 28
                            bgcolor: "transparent"
                            color: "#ffffff"
                            onClicked: { if (Physics) Physics.autoGenerateColliders() }
                        }
                    }

                    RowLayout {
                        Text { text: "Quality:"; color: "#888888"; width: 70 }
                        ComboBox { id: qualityCombo; width: 120; model: ["Low", "Medium", "High", "Ultra"] }
                    }

                    RowLayout {
                        Text { text: "Mode:"; color: "#888888"; width: 70 }
                        ComboBox { id: modeCombo; width: 120; model: ["Convex", "Mesh", "Mixed"] }
                    }

                    RowLayout {
                        CheckBox { id: simplifyChk; checked: true }
                        Text { text: "Simplify Geometry"; color: "#888888" }
                    }
                    RowLayout {
                        CheckBox { id: optimizeChk; checked: true }
                        Text { text: "Optimize Vertices"; color: "#888888" }
                    }
                    Item { Layout.fillHeight: true }
                }

                // 1 — LOD Manager
                ColumnLayout {
                    spacing: 8
                    Text { text: "LOD MANAGER"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                    RowLayout {
                        KsButton {
                            text: "Import LOD"
                            height: 28
                            bgcolor: "#E10600"
                            color: "#121212"
                            onClicked: lodFileDialog.open()
                        }
                        KsButton {
                            text: "Auto-Generate LODs"
                            height: 28
                            bgcolor: "transparent"
                            color: "#ffffff"
                            onClicked: { if (Physics) Physics.statusMessage("Auto-generating LODs...") }
                        }
                    }

                    GroupBox {
                        title: "LOD Levels"
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent; spacing: 4
                            RowLayout { Text { text: "LOD 0 (High):"; color: "#aaa"; width: 100 }; ComboBox { Layout.fillWidth: true; model: ["Import from FBX", "Auto"] } }
                            RowLayout { Text { text: "LOD 1 (Medium):"; color: "#aaa"; width: 100 }; ComboBox { Layout.fillWidth: true; model: ["50% Tris", "60% Tris", "Auto"] } }
                            RowLayout { Text { text: "LOD 2 (Low):"; color: "#aaa"; width: 100 }; ComboBox { Layout.fillWidth: true; model: ["25% Tris", "30% Tris", "Auto"] } }
                            RowLayout { Text { text: "LOD 3 (Ultra Low):"; color: "#aaa"; width: 100 }; ComboBox { Layout.fillWidth: true; model: ["10% Tris", "15% Tris", "Auto"] } }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // 2 — Mirror Setup
                ColumnLayout {
                    spacing: 8
                    Text { text: "MIRROR SETUP"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                    RowLayout {
                        KsButton {
                            text: "Apply Mirror Setup"
                            height: 28
                            bgcolor: "#E10600"
                            color: "#121212"
                            onClicked: {
                                if (Physics) Physics.setupMirrors(mirrorCount.value, mirrorAngle.value, mirrorDist.value)
                            }
                        }
                    }

                    RowLayout { Text { text: "Mirror Count:"; color: "#aaa"; width: 100 }; SpinBox { id: mirrorCount; value: 2; from: 0; to: 4 } }
                    RowLayout { Text { text: "Angle (°):"; color: "#aaa"; width: 100 }; SpinBox { id: mirrorAngle; value: 30; from: 0; to: 90 } }
                    RowLayout { Text { text: "Distance (mm):"; color: "#aaa"; width: 100 }; SpinBox { id: mirrorDist; value: 800; from: 100; to: 2000 } }
                    Item { Layout.fillHeight: true }
                }

                // 3 — Exhaust Config
                ColumnLayout {
                    spacing: 8
                    Text { text: "EXHAUST CONFIGURATION"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                    RowLayout {
                        KsButton {
                            text: "Apply Exhaust Config"
                            height: 28
                            bgcolor: "#E10600"
                            color: "#121212"
                            onClicked: {
                                if (Physics) Physics.setupExhaust(exhaustType.currentText, exhaustLength.value, exhaustDiameter.value)
                            }
                        }
                    }

                    RowLayout { Text { text: "Type:"; color: "#aaa"; width: 100 }; ComboBox { id: exhaustType; Layout.fillWidth: true; model: ["Single", "Dual", "Quad", "Side"] } }
                    RowLayout { Text { text: "Length (mm):"; color: "#aaa"; width: 100 }; SpinBox { id: exhaustLength; value: 600; from: 100; to: 2000 } }
                    RowLayout { Text { text: "Diameter (mm):"; color: "#aaa"; width: 100 }; SpinBox { id: exhaustDiameter; value: 60; from: 20; to: 200 } }
                    Item { Layout.fillHeight: true }
                }

                // 4 — Light Setup
                ColumnLayout {
                    spacing: 8
                    Text { text: "LIGHT SETUP"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                    RowLayout {
                        KsButton {
                            text: "Apply Lights"
                            height: 28
                            bgcolor: "#E10600"
                            color: "#121212"
                            onClicked: {
                                if (Physics) Physics.setupLights(lightCount.value, lightType.currentText, lightBrightness.value)
                            }
                        }
                    }

                    RowLayout { Text { text: "Count:"; color: "#aaa"; width: 100 }; SpinBox { id: lightCount; value: 4; from: 0; to: 20 } }
                    RowLayout { Text { text: "Type:"; color: "#aaa"; width: 100 }; ComboBox { id: lightType; Layout.fillWidth: true; model: ["Headlight", "Taillight", "Turn Signal", "Brake Light", "Fog Light"] } }
                    RowLayout { Text { text: "Brightness:"; color: "#aaa"; width: 100 }; Slider { id: lightBrightness; from: 0; to: 100; value: 80; Layout.fillWidth: true } }
                    Item { Layout.fillHeight: true }
                }

                // 5 — Interior Setup
                ColumnLayout {
                    spacing: 8
                    Text { text: "INTERIOR SETUP"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                    RowLayout {
                        KsButton {
                            text: "Apply Interior"
                            height: 28
                            bgcolor: "#E10600"
                            color: "#121212"
                            onClicked: {
                                if (Physics) Physics.setupInterior(interiorMat.currentText, interiorQuality.value)
                            }
                        }
                    }

                    RowLayout { Text { text: "Material:"; color: "#aaa"; width: 100 }; ComboBox { id: interiorMat; Layout.fillWidth: true; model: ["Leather", "Alcantara", "Plastic", "Carbon", "Custom"] } }
                    RowLayout { Text { text: "Quality:"; color: "#aaa"; width: 100 }; Slider { id: interiorQuality; from: 0; to: 100; value: 70; Layout.fillWidth: true } }
                    Item { Layout.fillHeight: true }
                }

                // 6 — Data Export
                ColumnLayout {
                    spacing: 8
                    Text { text: "DATA EXPORT"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                    RowLayout {
                        KsButton {
                            text: "Export CSV"
                            height: 28
                            bgcolor: "#E10600"
                            color: "#121212"
                            onClicked: exportDataDialog.open()
                        }
                        KsButton {
                            text: "Export JSON"
                            height: 28
                            bgcolor: "transparent"
                            color: "#ffffff"
                            onClicked: exportDataDialog.open()
                        }
                    }

                    RowLayout {
                        Text { text: "Include:"; color: "#aaa"; width: 80 }
                        ColumnLayout {
                            CheckBox { text: "Physics Data"; checked: true }
                            CheckBox { text: "Suspension"; checked: true }
                            CheckBox { text: "Aerodynamics"; checked: true }
                            CheckBox { text: "Drivetrain"; checked: true }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // 7 — Paint Config
                ColumnLayout {
                    spacing: 8
                    Text { text: "PAINT CONFIGURATION"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                    RowLayout {
                        KsButton {
                            text: "Apply Paint"
                            height: 28
                            bgcolor: "#E10600"
                            color: "#121212"
                            onClicked: {
                                if (Physics) Physics.paintConfig(paintRegion.currentText, paintColor.text)
                            }
                        }
                    }

                    RowLayout { Text { text: "Region:"; color: "#aaa"; width: 100 }; ComboBox { id: paintRegion; Layout.fillWidth: true; model: ["Body", "Wheels", "Spoiler", "Mirrors", "Interior"] } }
                    RowLayout { Text { text: "Color:"; color: "#aaa"; width: 100 }; TextField { id: paintColor; text: "#E10600"; Layout.fillWidth: true } }
                    RowLayout { Text { text: "Finish:"; color: "#aaa"; width: 100 }; ComboBox { Layout.fillWidth: true; model: ["Gloss", "Matte", "Metallic", "Satin"] } }
                    Item { Layout.fillHeight: true }
                }
            }

            Rectangle {
                width: 140
                color: "#252526"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8

                    Text { text: "STATUS"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                    RowLayout { Text { text: "Colliders:"; color: "#888888"; font.pixelSize: 10 }; Text { text: "OK"; color: "#E10600"; font.pixelSize: 10 } }
                    RowLayout { Text { text: "LODs:"; color: "#888888"; font.pixelSize: 10 }; Text { text: "OK"; color: "#E10600"; font.pixelSize: 10 } }
                    RowLayout { Text { text: "Mirrors:"; color: "#888888"; font.pixelSize: 10 }; Text { text: "3"; color: "#ffffff"; font.pixelSize: 10 } }

                    Item { Layout.fillHeight: true }

                    KsButton {
                        text: "Close"
                        height: 28
                        bgcolor: "#ef4444"
                        color: "#ffffff"
                        onClicked: physicsTools.visible = false
                    }
                }
            }
        }
    }
}


