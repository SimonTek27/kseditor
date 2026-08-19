import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: factoryPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property string currentFactory: ""
    property var factoryList: []

    function refreshFactories() {
        factoryList = Modeler.factoryNames ? Modeler.factoryNames() : []
        factoryView.model = factoryList
        userCountText.text = "User factories: " + (Modeler.factoryUserCount ? Modeler.factoryUserCount() : 0)
    }

    Connections {
        target: Modeler
        function onFactoryChanged() { refreshFactories() }
    }

    Component.onCompleted: refreshFactories()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "SCENE FACTORIES"
                color: "#E10600"
                font.pixelSize: 13
                font.bold: true
                Layout.fillWidth: true
            }

            AppButton {
                text: "X"
                height: 24
                width: 26
                bgcolor: "#3e3e42"
                color: "#ffffff"
                font.pixelSize: 10
                font.bold: true
                onClicked: closePanel()
            }
        }

        Text {
            text: "Parameterized templates (XSI-style): create primitives, cameras, "
                 + "lights and transform groups, or save a selection as a reusable factory."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            TextField {
                id: factoryNameField
                Layout.fillWidth: true
                height: 26
                placeholderText: "factory name"
                selectByMouse: true
                color: "#ffffff"
                background: Rectangle { color: "#2d2d30"; radius: 3; border.color: "#444"; border.width: 1 }
            }
            AppButton {
                text: "Save Selection"
                height: 28
                width: 110
                bgcolor: "#E10600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 9
                enabled: Modeler && Modeler.hasSelection && factoryNameField.text.trim() !== ""
                onClicked: { Modeler.factorySaveFromSelection(factoryNameField.text); factoryNameField.text = "" }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text {
            id: userCountText
            color: "#aaa"
            font.pixelSize: 10
            font.bold: true
        }

        ListView {
            id: factoryView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            model: []

            delegate: Rectangle {
                width: factoryView.width
                height: 34
                radius: 3
                color: currentFactory === modelData ? "#3a3a3e" : "#2d2d30"
                border.color: currentFactory === modelData ? "#E10600" : "#333"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 4

                    Rectangle {
                        width: 12; height: 12; radius: 2
                        color: {
                            var t = Modeler.factoryType(modelData)
                            if (t.indexOf("Mesh") >= 0) return "#E10600"
                            if (t === "Camera") return "#00ccff"
                            if (t === "Light") return "#ffdd00"
                            if (t === "Node") return "#ff6600"
                            return "#888"
                        }
                    }

                    Text {
                        text: modelData
                        color: "#ffffff"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Text {
                        text: Modeler.factoryType(modelData)
                        color: "#999"
                        font.pixelSize: 9
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        currentFactory = modelData
                        var p = Modeler.factoryParams(modelData)
                        paramText.text = p && p.user ? "Object: " + p.objectName + "  |  verts: " + p.vertexCount
                                                     : "Built-in primitive"
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            TextField {
                id: sizeField
                Layout.fillWidth: true
                height: 26
                placeholderText: "size (built-in param)"
                selectByMouse: true
                color: "#ffffff"
                background: Rectangle { color: "#2d2d30"; radius: 3; border.color: "#444"; border.width: 1 }
            }
            AppButton {
                text: "Create"
                height: 28
                width: 80
                bgcolor: "#E10600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 10
                enabled: currentFactory !== ""
                onClicked: {
                    var s = parseFloat(sizeField.text)
                    if (isNaN(s)) s = 0
                    Modeler.factoryCreate(currentFactory, s, s, s)
                }
            }
        }

        Text {
            id: paramText
            text: "Select a factory above."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        AppButton {
            Layout.fillWidth: true
            height: 26
            text: "Delete User Factory"
            bgcolor: "#3e3e42"
            color: "#fff"
            font.pixelSize: 10
            enabled: currentFactory !== "" && Modeler.factoryParams(currentFactory).user
            onClicked: {
                Modeler.factoryDelete(currentFactory)
                currentFactory = ""
                paramText.text = "Select a factory above."
            }
        }

        Text {
            text: "Factories persist in the .ks3d file (user templates)."
            color: "#666"
            font.pixelSize: 9
        }
    }
}
