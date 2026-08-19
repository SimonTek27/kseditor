import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: projPaintPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property bool stencilLoaded: false
    property var stencilInfo: ({})

    function refresh() {
        stencilInfo = Modeler.projectionStencilInfo ? Modeler.projectionStencilInfo() : {}
        stencilLoaded = stencilInfo && stencilInfo.width > 0
        pinInfo.text = objectId >= 0 ? "Target: " + (Modeler.selectedObject ? Modeler.selectedObject.name : "") : "No object selected"
    }

    function objLabel() {
        return objectId >= 0 ? "Target: " + (Modeler.selectedObject ? Modeler.selectedObject.name : "") : "No object selected"
    }

    Connections {
        target: Modeler
        function onProjectionStencilChanged() { refresh() }
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            refresh()
        }
        function onSceneChanged() { refresh() }
    }

    Component.onCompleted: refresh()

    FileDialog {
        id: stencilDialog
        title: "Load Stencil Image"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.bmp *.tga)", "All files (*)"]
        onAccepted: {
            if (Modeler.projectionLoadStencil)
                Modeler.projectionLoadStencil(selectedFile.toString())
            refresh()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text { text: "PROJECTION PAINT"; color: "#E10600"; font.pixelSize: 13; font.bold: true; Layout.fillWidth: true }
            AppButton { text: "X"; height: 24; width: 26; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10; font.bold: true; onClicked: closePanel() }
        }

        Text { id: pinInfo; text: objLabel(); color: objectId >= 0 ? "#aaa" : "#E10600"; font.pixelSize: 10; font.bold: objectId >= 0; wrapMode: Text.WordWrap }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "STENCIL"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: stencilLoaded ? "Replace Stencil Image" : "Load Stencil Image"
            bgcolor: "#E10600"
            color: "#121212"
            font.bold: true
            font.pixelSize: 11
            onClicked: stencilDialog.open()
        }

        Text {
            text: {
                if (!stencilLoaded) return "No stencil loaded"
                return "Size: " + (stencilInfo.width || 0) + "x" + (stencilInfo.height || 0)
            }
            color: stencilLoaded ? "#888" : "#E10600"
            font.pixelSize: 9
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "STENCIL TRANSFORM"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        Text { text: "Position X"; color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: posX
            Layout.fillWidth: true
            from: -10.0; to: 10.0; stepSize: 0.01
            value: stencilInfo.positionX || 0
            onMoved: if (Modeler.projectionSetStencilPosition) Modeler.projectionSetStencilPosition(value, posY.value, posZ.value)
            background: Rectangle {
                x: posX.leftPadding; y: posX.topPadding + posX.availableHeight / 2 - 2
                width: posX.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: posX.visualPosition * parent.width; height: 4; radius: 2; color: "#E10600" }
            }
            handle: Rectangle {
                x: posX.leftPadding + posX.visualPosition * (posX.availableWidth - width)
                y: posX.topPadding + posX.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7; color: "#ffffff"
            }
        }

        Text { text: "Position Y"; color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: posY
            Layout.fillWidth: true
            from: -10.0; to: 10.0; stepSize: 0.01
            value: stencilInfo.positionY || 0
            onMoved: if (Modeler.projectionSetStencilPosition) Modeler.projectionSetStencilPosition(posX.value, value, posZ.value)
            background: Rectangle {
                x: posY.leftPadding; y: posY.topPadding + posY.availableHeight / 2 - 2
                width: posY.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: posY.visualPosition * parent.width; height: 4; radius: 2; color: "#E10600" }
            }
            handle: Rectangle {
                x: posY.leftPadding + posY.visualPosition * (posY.availableWidth - width)
                y: posY.topPadding + posY.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7; color: "#ffffff"
            }
        }

        Text { text: "Position Z"; color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: posZ
            Layout.fillWidth: true
            from: -10.0; to: 10.0; stepSize: 0.01
            value: stencilInfo.positionZ || 0
            onMoved: if (Modeler.projectionSetStencilPosition) Modeler.projectionSetStencilPosition(posX.value, posY.value, value)
            background: Rectangle {
                x: posZ.leftPadding; y: posZ.topPadding + posZ.availableHeight / 2 - 2
                width: posZ.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: posZ.visualPosition * parent.width; height: 4; radius: 2; color: "#E10600" }
            }
            handle: Rectangle {
                x: posZ.leftPadding + posZ.visualPosition * (posZ.availableWidth - width)
                y: posZ.topPadding + posZ.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7; color: "#ffffff"
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "OPTIONS"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton {
                text: "Use Alpha"
                height: 24
                checkable: true
                autoExclusive: false
                bgcolor: stencilInfo.useAlpha ? "#2a6e2a" : "#3e3e42"
                color: "#fff"
                font.pixelSize: 9
                checked: stencilInfo.useAlpha
                onClicked: {
                    if (Modeler.projectionSetStencilUseAlpha)
                        Modeler.projectionSetStencilUseAlpha(!stencilInfo.useAlpha)
                }
            }
            AppButton {
                text: "Tile/Loop"
                height: 24
                checkable: true
                autoExclusive: false
                bgcolor: stencilInfo.loop ? "#2a6e2a" : "#3e3e42"
                color: "#fff"
                font.pixelSize: 9
                checked: stencilInfo.loop
                onClicked: {
                    if (Modeler.projectionSetStencilLoop)
                        Modeler.projectionSetStencilLoop(!stencilInfo.loop)
                }
            }
        }

        Text { text: "OPACITY"; color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: opacitySlider
            Layout.fillWidth: true
            from: 0.0; to: 1.0; stepSize: 0.01
            value: stencilInfo.opacity || 1.0
            onMoved: if (Modeler.projectionSetStencilOpacity) Modeler.projectionSetStencilOpacity(value)
            background: Rectangle {
                x: opacitySlider.leftPadding; y: opacitySlider.topPadding + opacitySlider.availableHeight / 2 - 2
                width: opacitySlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: opacitySlider.visualPosition * parent.width; height: 4; radius: 2; color: "#ff6600" }
            }
            handle: Rectangle {
                x: opacitySlider.leftPadding + opacitySlider.visualPosition * (opacitySlider.availableWidth - width)
                y: opacitySlider.topPadding + opacitySlider.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7; color: "#ffffff"
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "BRUSH MODE"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton {
                text: "Project"
                height: 24
                checkable: true
                autoExclusive: true
                bgcolor: brushMode === 0 ? "#E10600" : "#3e3e42"
                color: brushMode === 0 ? "#121212" : "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                checked: brushMode === 0
                onClicked: brushMode = 0
            }
            AppButton {
                text: "Clone"
                height: 24
                checkable: true
                autoExclusive: true
                bgcolor: brushMode === 1 ? "#E10600" : "#3e3e42"
                color: brushMode === 1 ? "#121212" : "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                checked: brushMode === 1
                onClicked: brushMode = 1
            }
        }

        property int brushMode: 0

        Text { text: "STRENGTH"; color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: strengthSlider
            Layout.fillWidth: true
            from: 0.0; to: 1.0; stepSize: 0.01
            value: 0.8
            background: Rectangle {
                x: strengthSlider.leftPadding; y: strengthSlider.topPadding + strengthSlider.availableHeight / 2 - 2
                width: strengthSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: strengthSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#7a5cf0" }
            }
            handle: Rectangle {
                x: strengthSlider.leftPadding + strengthSlider.visualPosition * (strengthSlider.availableWidth - width)
                y: strengthSlider.topPadding + strengthSlider.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7; color: "#ffffff"
            }
        }

        Text { text: "RADIUS"; color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: radiusSlider
            Layout.fillWidth: true
            from: 0.01; to: 5.0; stepSize: 0.01
            value: 1.0
            background: Rectangle {
                x: radiusSlider.leftPadding; y: radiusSlider.topPadding + radiusSlider.availableHeight / 2 - 2
                width: radiusSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: radiusSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#ff6600" }
            }
            handle: Rectangle {
                x: radiusSlider.leftPadding + radiusSlider.visualPosition * (radiusSlider.availableWidth - width)
                y: radiusSlider.topPadding + radiusSlider.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7; color: "#ffffff"
            }
        }

        Item { Layout.fillHeight: true }

        Text { text: "Tip: Load a stencil image, position it in the viewport, then paint over it. Project mode stamps the stencil onto the mesh. Clone copies from one UV area to another."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
}
