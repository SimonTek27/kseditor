import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: modelerPanels
    width: 320
    height: 500
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property string activePanel: "materials"
    property var uvChannels: ["Map 1", "Lightmap", "Detail"]
    property real lodDistance: 50
    property bool isExporting: false
    property string currentFile: Modeler ? Modeler.currentFile || "untitled.ks3d" : "untitled.ks3d"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            width: 110
            Layout.fillHeight: true
            color: "#181818"
            border.color: "#2a2a2a"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 2

                Text {
                    text: "PANELS"
                    color: "#E10600"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 4
                    bottomPadding: 4
                }

                Repeater {
                    model: [
                        { key: "materials", label: "Materials", icon: "MAT" },
                        { key: "layers", label: "Layers", icon: "L" },
                        { key: "outliner", label: "Outliner", icon: "O" },
                        { key: "properties", label: "Properties", icon: "P" },
                        { key: "general", label: "General", icon: "G" }
                    ]

                    AppButton {
                        height: 26
                        text: modelData.label
                        bgcolor: activePanel === modelData.key ? "#E10600" : "#252526"
                        color: activePanel === modelData.key ? "#121212" : "#cccccc"
                        font.pixelSize: 10
                        Layout.fillWidth: true
                        onClicked: activePanel = modelData.key
                    }
                }

                Item { Layout.fillHeight: true }

                Text {
                    text: "OBJECT INFO"
                    color: "#555"
                    font.pixelSize: 8
                    font.bold: true
                    leftPadding: 4
                }

                Text {
                    text: "V: " + (Modeler && Modeler.hasSelection ? "-" : "0")
                    color: "#777"
                    font.pixelSize: 9
                    leftPadding: 4
                }

                Text {
                    text: "F: " + (Modeler && Modeler.hasSelection ? "-" : "0")
                    color: "#777"
                    font.pixelSize: 9
                    leftPadding: 4
                }

                Text {
                    text: "E: " + (Modeler && Modeler.hasSelection ? "-" : "0")
                    color: "#777"
                    font.pixelSize: 9
                    leftPadding: 4
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1e1e1e"

            StackLayout {
                anchors.fill: parent
                currentIndex: {
                    if (activePanel === "materials") return 0
                    if (activePanel === "layers") return 1
                    if (activePanel === "outliner") return 2
                    if (activePanel === "properties") return 3
                    return 4
                }

                MaterialEditor {}

                Rectangle {
                    color: "transparent"
                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 8

                        Text { text: "LAYERS"; color: "#666"; font.pixelSize: 10; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "Layer management panel"; color: "#444"; font.pixelSize: 9 }
                    }
                }

                SceneOutliner {}

                PropertiesPanel {}

                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 4

                        Text { text: "UV CHANNELS"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 8; topPadding: 6 }
                        ComboBox { height: 22; Layout.fillWidth: true; Layout.leftMargin: 8; Layout.rightMargin: 8; model: uvChannels; font.pixelSize: 10 }
                        RowLayout {
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8
                            AppButton { height: 22; text: "Add"; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 9
                                onClicked: { if (Modeler.addPrimitiveCube) Modeler.addPrimitiveCube(0.5) } }
                            AppButton { height: 22; text: "Remove"; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 9
                                onClicked: { if (Modeler.deleteSelected) Modeler.deleteSelected() } }
                        }

                        Rectangle { height: 6; color: "transparent" }

                        Text { text: "LOD SYSTEM"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                        RowLayout {
                            Layout.leftMargin: 8; Layout.rightMargin: 8
                            Text { text: "Distance:"; color: "#888"; font.pixelSize: 9 }
                            Slider { id: lodSlider; from: 10; to: 200; value: lodDistance; Layout.fillWidth: true; height: 18
                                onMoved: lodDistance = value }
                            Text { text: lodDistance.toFixed(0) + "m"; color: "#E10600"; font.pixelSize: 9 }
                        }
                        RowLayout {
                            Layout.leftMargin: 8; Layout.rightMargin: 8
                            CheckBox { id: autoLodCheck; height: 16 }
                            Text { text: "Auto-generate LODs"; color: "#888"; font.pixelSize: 9 }
                        }
                        AppButton { height: 26; text: "Generate LODs"; Layout.fillWidth: true; Layout.leftMargin: 8; Layout.rightMargin: 8; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10
                            onClicked: { if (Modeler.generateLODs) Modeler.generateLODs(lodDistance, autoLodCheck.checked) } }

                        Rectangle { height: 6; color: "transparent" }

                        Text { text: "KN5 EXPORT"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                        RowLayout {
                            Layout.leftMargin: 8; Layout.rightMargin: 8
                            CheckBox { id: compressTexCheck; checked: true; height: 16 }
                            Text { text: "Compress Textures"; color: "#888"; font.pixelSize: 9 }
                        }
                        RowLayout {
                            Layout.leftMargin: 8; Layout.rightMargin: 8
                            CheckBox { id: genColliderCheck; checked: true; height: 16 }
                            Text { text: "Generate Collider"; color: "#888"; font.pixelSize: 9 }
                        }
                        RowLayout {
                            Layout.leftMargin: 8; Layout.rightMargin: 8
                            CheckBox { id: includeAnimCheck; height: 16 }
                            Text { text: "Include Animations"; color: "#888"; font.pixelSize: 9 }
                        }
                        AppButton { height: 30; text: "Export KN5"; Layout.fillWidth: true; Layout.leftMargin: 8; Layout.rightMargin: 8; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10; font.bold: true
                            onClicked: { if (Modeler.exportKN5) Modeler.exportKN5("export.kn5") } }

                        Rectangle { height: 6; color: "transparent" }

                        Text { text: "FILE"; color: "#666"; font.pixelSize: 9; font.bold: true; leftPadding: 8 }
                        RowLayout {
                            Layout.leftMargin: 8; Layout.rightMargin: 8
                            AppButton { height: 24; text: "Import"; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 9
                                onClicked: { if (Modeler.importFBX) Modeler.importFBX("") } }
                            AppButton { height: 24; text: "Export"; Layout.fillWidth: true; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 9
                                onClicked: { if (Modeler.exportKN5) Modeler.exportKN5("export.kn5") } }
                            AppButton { height: 24; text: "Save"; Layout.fillWidth: true; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 9
                                onClicked: { if (Modeler.newProject) console.log("Save called") } }
                        }

                        Item { height: 12 }
                    }
                }
            }
        }
    }
}


