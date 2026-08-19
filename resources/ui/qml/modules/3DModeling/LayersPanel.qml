import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

// Layer system panel (Max-style scene layers): organize objects, toggle
// visibility, assign the selected object to the current layer.
Rectangle {
    id: layersPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int currentIndex: 0
    property string newName: "Layer " + (Modeler.layerCount ? (Modeler.layerCount() + 1) : 1)

    function refresh() {
        currentIndex = Modeler.currentLayerIndex ? Modeler.currentLayerIndex() : 0
        layerList.model = Modeler.layerNames ? Modeler.layerNames() : []
    }

    Connections {
        target: Modeler
        function onSceneChanged() { refresh() }
        function onSelectionChanged() { refresh() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "LAYERS"
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

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            TextField {
                id: nameField
                Layout.fillWidth: true
                height: 26
                text: newName
                font.pixelSize: 11
                onEditingFinished: newName = text
            }

            AppButton {
                text: "Add"
                height: 26
                bgcolor: "#E10600"; color: "#fff"
                font.pixelSize: 11; font.bold: true
                onClicked: {
                    if (Modeler.addLayer) {
                        Modeler.addLayer(newName.trim() === "" ? ("Layer " + (Modeler.layerCount() + 1)) : newName.trim())
                    }
                    newName = "Layer " + (Modeler.layerCount ? (Modeler.layerCount() + 1) : 1)
                    refresh()
                }
            }
        }

        ListView {
            id: layerList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2

            model: Modeler.layerNames ? Modeler.layerNames() : []

            delegate: Rectangle {
                width: layerList.width
                height: 26
                color: layerList.currentIndex === index ? "#2a2a2e" : "#1e1e1e"
                radius: 3

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (Modeler.setCurrentLayerIndex) Modeler.setCurrentLayerIndex(index)
                        layerList.currentIndex = index
                        refresh()
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 6

                    Rectangle {
                        width: 10; height: 10; radius: 5
                        color: "#9AA0A6"
                    }

                    Text {
                        text: modelData
                        color: "#fff"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    AppButton {
                        text: visibleTxt
                        height: 20
                        width: 64
                        bgcolor: visibleOn ? "#3e3e42" : "transparent"
                        color: visibleOn ? "#fff" : "#999"
                        font.pixelSize: 9
                        property bool visibleOn: Modeler.isLayerVisible ? Modeler.isLayerVisible(index) : true
                        property string visibleTxt: visibleOn ? "Visible" : "Hidden"
                        onClicked: { if (Modeler.setLayerVisible) Modeler.setLayerVisible(index, !visibleOn) }
                    }

                    AppButton {
                        text: "Assign"
                        height: 20
                        width: 60
                        bgcolor: "#3e3e42"
                        color: "#fff"
                        font.pixelSize: 9
                        onClicked: {
                            if (Modeler.assignSelectionToLayer) {
                                var n = Modeler.assignSelectionToLayer(index)
                                statusMsg.text = n > 0 ? "Assigned to layer" : "Select an object first"
                            }
                        }
                    }

                    AppButton {
                        text: "Del"
                        height: 20
                        width: 34
                        bgcolor: "transparent"
                        color: "#E10600"
                        font.pixelSize: 9
                        onClicked: { if (Modeler.removeLayer) Modeler.removeLayer(index) }
                    }
                }
            }
        }

        Text {
            id: statusMsg
            text: "Left-click a layer to make it current. Assign the selected object with the Assign button."
            color: "#888"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}