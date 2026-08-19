import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: skinWrapPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property int addCageId: -1
    property bool pickingCage: false

    function startPickCage() {
        pickingCage = true
        addCageId = -1
    }

    function confirmPickCage(id) {
        if (pickingCage && id !== objectId) {
            addCageId = id
            pickingCage = false
        }
    }

    function addWrap() {
        if (objectId >= 0 && addCageId >= 0) {
            Modeler.skinWrapAdd(objectId, addCageId)
            addCageId = -1
        }
    }

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            pickingCage = false
            addCageId = -1
        }
        function onSkinWrapChanged() {
            // list updates via bindings
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "SKIN WRAP"
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
            text: objectId >= 0 ? "Skin: " + Modeler.selectedObject.name
                                : "No object selected"
            color: objectId >= 0 ? "#aaa" : "#E10600"
            font.pixelSize: 10
            font.bold: objectId >= 0
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "ADD NEW (skin = selected)"; color: "#888"; font.pixelSize: 10; font.bold: true }

        Text {
            text: "Bind the selected mesh to a cage mesh. Skin vertices follow the cage as it deforms/moves."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: pickingCage ? "Click cage object..." : "Cage (click)"
                height: 26
                Layout.fillWidth: true
                bgcolor: pickingCage ? "#E10600" : "#3e3e42"
                color: pickingCage ? "#121212" : "#fff"
                font.pixelSize: 10
                font.bold: pickingCage
                onClicked: startPickCage()
            }

            AppButton {
                text: "Add"
                height: 26
                width: 60
                bgcolor: (objectId >= 0 && addCageId >= 0) ? "#E10600" : "#3e3e42"
                color: (objectId >= 0 && addCageId >= 0) ? "#121212" : "#888"
                font.pixelSize: 10
                font.bold: true
                enabled: objectId >= 0 && addCageId >= 0
                onClicked: addWrap()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "WRAP LIST"; color: "#888"; font.pixelSize: 10; font.bold: true }

        Column {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Repeater {
                id: wrapRepeater
                model: objectId >= 0 ? Modeler.skinWrapList(objectId) : []

                delegate: Rectangle {
                    Layout.fillWidth: true
                    height: 46
                    color: "#2a2a2e"
                    border.color: "#333"
                    border.width: 1
                    radius: 3

                    property variant data: modelData
                    property int index: index

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        CheckBox {
                            checked: data.enabled
                            onCheckedChanged: {
                                if (objectId >= 0) Modeler.skinWrapSetEnabled(objectId, index, checked)
                            }
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Text {
                            text: "Cage: " + data.cageName
                            color: "#fff"
                            font.pixelSize: 10
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                        }

                        AppButton {
                            text: "Rebind"
                            height: 22
                            width: 52
                            bgcolor: "#3e3e42"
                            color: "#fff"
                            font.pixelSize: 9
                            onClicked: {
                                Modeler.skinWrapRebind(objectId, index)
                            }
                        }

                        AppButton {
                            text: "✕"
                            height: 22
                            width: 24
                            bgcolor: "#5a1a1a"
                            color: "#ff8888"
                            font.pixelSize: 10
                            font.bold: true
                            onClicked: {
                                Modeler.skinWrapRemove(objectId, index)
                            }
                        }
                    }
                }
            }
        }
    }
}