import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Scene3D 2.15
import "../widgets"

Rectangle {
    id: pageEditor
    width: 1280
    height: 720
    color: "#121212"

    property string currentPage: "home"
    property string currentFile: "LOD0.FBX"

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

                Button {
                    text: "Import"
                    flat: true
                    icon.source: "qrc:/icons/document-open.svg"
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                }
                Button {
                    text: "Export"
                    flat: true
                    icon.source: "qrc:/icons/document-save-as.svg"
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                }
                Button {
                    text: "3D Print"
                    flat: true
                    icon.source: "qrc:/icons/primitive-plane.svg"
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                }
                Rectangle {
                    width: 1
                    height: 20
                    color: "#444444"
                }
                Button {
                    text: "Primitives"
                    flat: true
                    icon.source: "qrc:/icons/primitive-box.svg"
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                }
                Button {
                    text: "Texture Browser"
                    flat: true
                    icon.source: "qrc:/icons/icon-ksproj.svg"
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
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

            // --- 3D Viewport (Center) ---
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#2a2a2a"
                clip: true

                // Simulated 3D viewport gradient
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#4a708b" }
                        GradientStop { position: 0.6; color: "#87ceeb" }
                        GradientStop { position: 0.61; color: "#333333" }
                        GradientStop { position: 1.0; color: "#1a1a1a" }
                    }
                }

                // Viewport info text
                Text {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.margins: 10
                    text: "Perspective | Gizmo: Local | Camera: Free"
                    color: "#cccccc"
                    font.pixelSize: 11
                }
            }

            // --- Properties Panel (Right) ---
            Rectangle {
                Layout.preferredWidth: 320
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 20

                    Text {
                        text: "OBJECT INSTANCE EDIT"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 14
                    }

                    // Naming Section
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
                                text: "Naming"
                                color: "#E10600"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            RowLayout {
                                width: parent.width

                                Text {
                                    text: "Object:"
                                    color: "#bbbbbb"
                                    Layout.preferredWidth: 60
                                }
                                TextField {
                                    text: "CINTURE_ON_SUB1"
                                    Layout.fillWidth: true
                                    color: "white"
                                }
                            }
                        }
                    }

                    // Transform Section
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
                                text: "Transform"
                                color: "#E10600"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            GridLayout {
                                columns: 2
                                width: parent.width

                                Text { text: "Position X:"; color: "#bbbbbb" }
                                SpinBox {
                                    editable: true
                                    from: -1000
                                    to: 1000
                                    value: 0
                                }

                                Text { text: "Rotation Y:"; color: "#bbbbbb" }
                                SpinBox {
                                    editable: true
                                    from: 0
                                    to: 360
                                    value: 91
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
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

                Text { text: "Ready"; color: "#10b981"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "ksEditor v1.0"; color: "#666"; font.pixelSize: 10 }
            }
        }
    }
}