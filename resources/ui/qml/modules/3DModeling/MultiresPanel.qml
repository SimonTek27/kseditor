import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: multiresPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property var levelModel: []
    property int currentLevel: -1
    property int levelCount: 0

    function refresh() {
        if (objectId < 0) {
            levelModel = []
            currentLevel = -1
            levelCount = 0
            return
        }
        levelModel = Modeler.multiresLevelList ? Modeler.multiresLevelList(objectId) : []
        currentLevel = Modeler.multiresCurrentLevel ? Modeler.multiresCurrentLevel(objectId) : -1
        levelCount = Modeler.multiresLevelCount ? Modeler.multiresLevelCount(objectId) : 0
        levelsList.model = levelModel
        pinInfo.text = objectId >= 0 ? "Target: " + (Modeler.selectedObject ? Modeler.selectedObject.name : "") : "No object selected"
    }

    Connections {
        target: Modeler
        function onSceneChanged() { refresh() }
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            refresh()
        }
    }

    Component.onCompleted: refresh()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text { text: "MULTIRES"; color: "#E10600"; font.pixelSize: 13; font.bold: true; Layout.fillWidth: true }
            AppButton { text: "X"; height: 24; width: 26; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10; font.bold: true; onClicked: closePanel() }
        }

        Text { id: pinInfo; text: objectId >= 0 ? "Target: " + (Modeler.selectedObject ? Modeler.selectedObject.name : "") : "No object selected"; color: objectId >= 0 ? "#aaa" : "#E10600"; font.pixelSize: 10; font.bold: objectId >= 0; wrapMode: Text.WordWrap }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "SUBDIVISION LEVELS"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        Text { text: "Levels: " + levelCount + " | Current: " + currentLevel; color: "#888"; font.pixelSize: 9 }

        ListView {
            id: levelsList
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            clip: true
            spacing: 2
            highlight: Rectangle { color: "#2a2a2e" }
            delegate: Rectangle {
                width: levelsList.width
                height: 28
                color: "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 6
                    Rectangle {
                        width: 10; height: 10; radius: 2
                        color: modelData && modelData.level === currentLevel ? "#E10600" : "#3e3e42"
                    }
                    Text {
                        text: modelData ? "Level " + modelData.level : ""
                        color: modelData && modelData.level === currentLevel ? "#E10600" : "#aaa"
                        font.pixelSize: 10
                        font.bold: modelData && modelData.level === currentLevel
                        Layout.fillWidth: true
                    }
                    Text {
                        text: modelData ? modelData.vertices + "v" : ""
                        color: "#888"
                        font.pixelSize: 9
                    }
                    AppButton {
                        text: "Set"
                        height: 20
                        width: 34
                        bgcolor: modelData && modelData.level === currentLevel ? "#2a6e2a" : "#3e3e42"
                        color: "#fff"
                        font.pixelSize: 8
                        onClicked: {
                            if (modelData && Modeler.multiresSetCurrentLevel)
                                Modeler.multiresSetCurrentLevel(objectId, modelData.level)
                        }
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onDoubleClicked: {
                        if (modelData && Modeler.multiresSetCurrentLevel)
                            Modeler.multiresSetCurrentLevel(objectId, modelData.level)
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "ACTIONS"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton {
                text: "Add Level"
                Layout.fillWidth: true
                height: 28
                bgcolor: "#E10600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 10
                enabled: objectId >= 0
                onClicked: {
                    if (Modeler.multiresAddLevel) {
                        Modeler.multiresAddLevel(objectId)
                        refresh()
                    }
                }
            }
            AppButton {
                text: "Remove"
                Layout.fillWidth: true
                height: 28
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                enabled: objectId >= 0 && levelCount > 1
                onClicked: {
                    if (Modeler.multiresRemoveLevel) {
                        Modeler.multiresRemoveLevel(objectId)
                        refresh()
                    }
                }
            }
        }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "Subdivide Current"
            bgcolor: "#ff6600"
            color: "#121212"
            font.bold: true
            font.pixelSize: 11
            enabled: objectId >= 0
            onClicked: {
                if (Modeler.multiresSubdivide) {
                    Modeler.multiresSubdivide(objectId)
                    refresh()
                }
            }
        }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "Bake to Base Mesh"
            bgcolor: "#7a5cf0"
            color: "#ffffff"
            font.bold: true
            font.pixelSize: 11
            enabled: objectId >= 0 && levelCount > 0
            onClicked: {
                if (Modeler.multiresBake) {
                    Modeler.multiresBake(objectId)
                    refresh()
                }
            }
        }

        Item { Layout.fillHeight: true }

        Text { text: "Tip: Add levels to subdivide. Sculpt on the current level for per-level detail. Bake to commit to base mesh."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
}
