import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: sculptLayersPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property var layerModel: []
    property int currentLayer: -1

    function refresh() {
        layerModel = Modeler.sculptLayerList ? Modeler.sculptLayerList() : []
        currentLayer = Modeler.sculptLayerCurrent ? Modeler.sculptLayerCurrent() : -1
        layersList.model = layerModel
        pinInfo.text = objectId >= 0 ? "Target: " + (Modeler.selectedObject ? Modeler.selectedObject.name : "") : "No object selected"
    }

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1

    function objLabel() {
        return objectId >= 0 ? "Target: " + (Modeler.selectedObject ? Modeler.selectedObject.name : "") : "No object selected"
    }

    Connections {
        target: Modeler
        function onSculptLayersChanged() { refresh() }
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            refresh()
        }
        function onSceneChanged() { refresh() }
    }

    Component.onCompleted: refresh()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text { text: "SCULPT LAYERS"; color: "#E10600"; font.pixelSize: 13; font.bold: true; Layout.fillWidth: true }
            AppButton { text: "X"; height: 24; width: 26; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10; font.bold: true; onClicked: closePanel() }
        }

        Text { id: pinInfo; text: objLabel(); color: objectId >= 0 ? "#aaa" : "#E10600"; font.pixelSize: 10; font.bold: objectId >= 0; wrapMode: Text.WordWrap }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "LAYERS"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        ListView {
            id: layersList
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            clip: true
            spacing: 2
            highlight: Rectangle { color: "#2a2a2e" }
            delegate: Rectangle {
                width: layersList.width
                height: 42
                color: "transparent"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 1
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Rectangle {
                            width: 10; height: 10; radius: 2
                            color: modelData && modelData.index === currentLayer ? "#E10600" : "#3e3e42"
                        }
                        Text {
                            text: modelData ? modelData.name : ""
                            color: modelData && modelData.index === currentLayer ? "#E10600" : "#aaa"
                            font.pixelSize: 10
                            font.bold: modelData && modelData.index === currentLayer
                            Layout.fillWidth: true
                        }
                        Text {
                            text: modelData ? Math.round(modelData.opacity * 100) + "%" : ""
                            color: "#888"
                            font.pixelSize: 9
                        }
                        AppButton {
                            text: modelData && modelData.visible ? "Vis" : "Hid"
                            height: 20
                            width: 34
                            bgcolor: modelData && modelData.visible ? "#2a6e2a" : "#3e3e42"
                            color: "#fff"
                            font.pixelSize: 8
                            onClicked: {
                                if (modelData && Modeler.sculptLayerSetVisible)
                                    Modeler.sculptLayerSetVisible(modelData.index, !modelData.visible)
                            }
                        }
                        AppButton {
                            text: modelData && modelData.locked ? "Lock" : "Unlk"
                            height: 20
                            width: 34
                            bgcolor: modelData && modelData.locked ? "#6e2a2a" : "#3e3e42"
                            color: "#fff"
                            font.pixelSize: 8
                            onClicked: {
                                if (modelData && Modeler.sculptLayerSetLocked)
                                    Modeler.sculptLayerSetLocked(modelData.index, !modelData.locked)
                            }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text { text: "Blend:"; color: "#666"; font.pixelSize: 8 }
                        Text {
                            text: {
                                if (!modelData) return ""
                                var mode = modelData.blendMode
                                if (mode === 0) return "Add"
                                if (mode === 1) return "Sub"
                                return "Rep"
                            }
                            color: "#aaa"
                            font.pixelSize: 8
                        }
                        Item { Layout.fillWidth: true }
                        Text { text: "Opacity:"; color: "#666"; font.pixelSize: 8 }
                        Text {
                            text: modelData ? modelData.opacity.toFixed(2) : ""
                            color: "#aaa"
                            font.pixelSize: 8
                            Layout.preferredWidth: 30
                        }
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: layersList.currentIndex = index
                    onDoubleClicked: {
                        if (modelData && Modeler.sculptLayerSetCurrent)
                            Modeler.sculptLayerSetCurrent(modelData.index)
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "layer name"
                color: "#ddd"
                font.pixelSize: 10
                background: Rectangle { color: "#2a2a2e"; border.color: "#444"; radius: 3 }
            }
            AppButton {
                text: "Add"
                height: 26
                bgcolor: "#E10600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 10
                onClicked: {
                    if (Modeler.sculptLayerAdd) {
                        var name = nameField.text || ("Layer " + (layerModel.length + 1))
                        Modeler.sculptLayerAdd(name)
                        nameField.text = ""
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton {
                text: "Rename"
                height: 24
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    var m = layerModel[layersList.currentIndex]
                    if (m && Modeler.sculptLayerRename)
                        Modeler.sculptLayerRename(m.index, nameField.text || m.name)
                }
            }
            AppButton {
                text: "Delete"
                height: 24
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    var m = layerModel[layersList.currentIndex]
                    if (m && Modeler.sculptLayerRemove)
                        Modeler.sculptLayerRemove(m.index)
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "BLEND MODE"; color: "#888"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton {
                text: "Additive"
                height: 24
                checkable: true
                autoExclusive: true
                bgcolor: layerModel.length > 0 && layerModel[layersList.currentIndex] && layerModel[layersList.currentIndex].blendMode === 0 ? "#E10600" : "#3e3e42"
                color: layerModel.length > 0 && layerModel[layersList.currentIndex] && layerModel[layersList.currentIndex].blendMode === 0 ? "#121212" : "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    var m = layerModel[layersList.currentIndex]
                    if (m && Modeler.sculptLayerSetBlendMode) Modeler.sculptLayerSetBlendMode(m.index, 0)
                }
            }
            AppButton {
                text: "Subtractive"
                height: 24
                checkable: true
                autoExclusive: true
                bgcolor: layerModel.length > 0 && layerModel[layersList.currentIndex] && layerModel[layersList.currentIndex].blendMode === 1 ? "#E10600" : "#3e3e42"
                color: layerModel.length > 0 && layerModel[layersList.currentIndex] && layerModel[layersList.currentIndex].blendMode === 1 ? "#121212" : "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    var m = layerModel[layersList.currentIndex]
                    if (m && Modeler.sculptLayerSetBlendMode) Modeler.sculptLayerSetBlendMode(m.index, 1)
                }
            }
            AppButton {
                text: "Replace"
                height: 24
                checkable: true
                autoExclusive: true
                bgcolor: layerModel.length > 0 && layerModel[layersList.currentIndex] && layerModel[layersList.currentIndex].blendMode === 2 ? "#E10600" : "#3e3e42"
                color: layerModel.length > 0 && layerModel[layersList.currentIndex] && layerModel[layersList.currentIndex].blendMode === 2 ? "#121212" : "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    var m = layerModel[layersList.currentIndex]
                    if (m && Modeler.sculptLayerSetBlendMode) Modeler.sculptLayerSetBlendMode(m.index, 2)
                }
            }
        }

        Text { text: "OPACITY"; color: "#888"; font.pixelSize: 10; font.bold: true }

        Text {
            text: {
                var m = layerModel[layersList.currentIndex]
                return m ? "Opacity: " + m.opacity.toFixed(2) : "Opacity: 1.00"
            }
            color: "#aaa"
            font.pixelSize: 9
        }

        Slider {
            id: opacitySlider
            Layout.fillWidth: true
            from: 0.0; to: 1.0; stepSize: 0.01
            value: layerModel.length > 0 && layerModel[layersList.currentIndex] ? layerModel[layersList.currentIndex].opacity : 1.0
            onMoved: {
                var m = layerModel[layersList.currentIndex]
                if (m && Modeler.sculptLayerSetOpacity) Modeler.sculptLayerSetOpacity(m.index, value)
            }
            background: Rectangle {
                x: opacitySlider.leftPadding
                y: opacitySlider.topPadding + opacitySlider.availableHeight / 2 - 2
                width: opacitySlider.availableWidth
                height: 4
                radius: 2
                color: "#3e3e42"
                Rectangle {
                    width: opacitySlider.visualPosition * parent.width
                    height: 4
                    radius: 2
                    color: "#E10600"
                }
            }
            handle: Rectangle {
                x: opacitySlider.leftPadding + opacitySlider.visualPosition * (opacitySlider.availableWidth - width)
                y: opacitySlider.topPadding + opacitySlider.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7
                color: "#ffffff"
            }
        }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "Bake Current Layer"
            bgcolor: "#7a5cf0"
            color: "#ffffff"
            font.bold: true
            font.pixelSize: 11
            enabled: layerModel.length > 0
            onClicked: {
                if (Modeler.sculptLayerBakeCurrent) {
                    Modeler.sculptLayerBakeCurrent()
                    refresh()
                }
            }
        }

        Item { Layout.fillHeight: true }

        Text { text: "Tip: Sculpt strokes go to the current layer. Use blend modes and opacity to composite layers. Bake merges to base mesh."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
}
