import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: morphPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property var targetNames: []
    property int targetCount: 0
    property int currentIndex: -1
    property var weights: []

    function refresh() {
        if (objectId < 0) {
            targetNames = []
            targetCount = 0
            currentIndex = -1
            weights = []
            return
        }
        targetNames = Modeler.getShapeKeyNames ? Modeler.getShapeKeyNames() : []
        targetCount = Modeler.getShapeKeyCount ? Modeler.getShapeKeyCount() : 0
        weights = []
        for (var i = 0; i < targetCount; i++) {
            weights.push(Modeler.getShapeKeyWeight ? Modeler.getShapeKeyWeight(i) : 0)
        }
        targetList.model = targetNames
        pinInfo.text = objectId >= 0 ? "Target: " + (Modeler.selectedObject ? Modeler.selectedObject.name : "") : "No object selected"
    }

    function objLabel() {
        return objectId >= 0 ? "Target: " + (Modeler.selectedObject ? Modeler.selectedObject.name : "") : "No object selected"
    }

    Connections {
        target: Modeler
        function onShapeKeysChanged() { refresh() }
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
            Text { text: "MORPH TARGETS"; color: "#E10600"; font.pixelSize: 13; font.bold: true; Layout.fillWidth: true }
            AppButton { text: "X"; height: 24; width: 26; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10; font.bold: true; onClicked: closePanel() }
        }

        Text { id: pinInfo; text: objLabel(); color: objectId >= 0 ? "#aaa" : "#E10600"; font.pixelSize: 10; font.bold: objectId >= 0; wrapMode: Text.WordWrap }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "TARGETS (" + targetCount + ")"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        ListView {
            id: targetList
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            clip: true
            spacing: 2
            highlight: Rectangle { color: "#2a2a2e" }
            delegate: Rectangle {
                width: targetList.width
                height: 30
                color: "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 6
                    Rectangle {
                        width: 10; height: 10; radius: 2
                        color: index === currentIndex ? "#E10600" : "#3e3e42"
                    }
                    Text {
                        text: modelData || ""
                        color: index === currentIndex ? "#E10600" : "#aaa"
                        font.pixelSize: 10
                        font.bold: index === currentIndex
                        Layout.fillWidth: true
                    }
                    Text {
                        text: weights.length > index ? Math.round(weights[index] * 100) + "%" : "0%"
                        color: "#888"
                        font.pixelSize: 9
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        targetList.currentIndex = index
                        currentIndex = index
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "CREATE / CAPTURE"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "morph name"
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
                    if (Modeler.addShapeKey) {
                        var name = nameField.text || ("Morph " + (targetCount + 1))
                        Modeler.addShapeKey(name)
                        nameField.text = ""
                    }
                }
            }
            AppButton {
                text: "Capture"
                height: 26
                bgcolor: "#ff6600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 10
                onClicked: {
                    if (Modeler.captureShapeKey) {
                        var name = nameField.text || ("Morph " + (targetCount + 1))
                        Modeler.captureShapeKey(name)
                        nameField.text = ""
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "BLEND WEIGHT"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        Text {
            text: {
                if (currentIndex < 0 || currentIndex >= targetNames.length) return "No target selected"
                var w = weights.length > currentIndex ? weights[currentIndex] : 0
                return targetNames[currentIndex] + ": " + (w * 100).toFixed(1) + "%"
            }
            color: "#aaa"
            font.pixelSize: 9
        }

        Slider {
            id: weightSlider
            Layout.fillWidth: true
            from: -1.0; to: 1.0; stepSize: 0.01
            value: currentIndex >= 0 && currentIndex < weights.length ? weights[currentIndex] : 0
            onMoved: {
                if (currentIndex >= 0 && Modeler.setShapeKeyWeight)
                    Modeler.setShapeKeyWeight(currentIndex, value)
            }
            background: Rectangle {
                x: weightSlider.leftPadding
                y: weightSlider.topPadding + weightSlider.availableHeight / 2 - 2
                width: weightSlider.availableWidth
                height: 4
                radius: 2
                color: "#3e3e42"
                Rectangle {
                    width: weightSlider.visualPosition * parent.width
                    height: 4
                    radius: 2
                    color: "#E10600"
                }
            }
            handle: Rectangle {
                x: weightSlider.leftPadding + weightSlider.visualPosition * (weightSlider.availableWidth - width)
                y: weightSlider.topPadding + weightSlider.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7
                color: "#ffffff"
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
                    if (currentIndex >= 0 && Modeler.renameShapeKey)
                        Modeler.renameShapeKey(currentIndex, nameField.text || targetNames[currentIndex])
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
                    if (currentIndex >= 0 && Modeler.removeShapeKey)
                        Modeler.removeShapeKey(currentIndex)
                }
            }
        }

        AppButton {
            Layout.fillWidth: true
            height: 28
            text: "Reset All Weights"
            bgcolor: "#3e3e42"
            color: "#fff"
            font.pixelSize: 10
            enabled: targetCount > 0
            onClicked: {
                if (Modeler.resetShapeKeys) Modeler.resetShapeKeys()
                refresh()
            }
        }

        Item { Layout.fillHeight: true }

        Text { text: "Tip: Add morph targets to capture mesh states. Use the weight slider to blend between base and morphed shapes. Capture saves current deformation."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
}
