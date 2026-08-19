import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: controllersPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property int addType: 0
    property int addTargetId: -1
    property bool addingTarget: false

    function setObjectId(id) {
        objectId = id
    }

    function startAddTarget() {
        addingTarget = true
        addTargetId = -1
    }

    function confirmAddTarget(id) {
        if (addingTarget && id !== objectId) {
            addTargetId = id
            addingTarget = false
        }
    }

    function addController() {
        if (objectId >= 0 && addTargetId >= 0) {
            if (addType === 3)
                Modeler.controllerAdd(objectId, addType, addTargetId, "position.x", 0, 0, 0, 0, 50, 2)
            else
                Modeler.controllerAdd(objectId, addType, addTargetId, "position.y", 0, 1, 1, 0, 50, 2)
            addTargetId = -1
        }
    }

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            addingTarget = false
            addTargetId = -1
        }
        function onControllerChanged() {
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
                text: "CONTROLLERS"
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
            text: objectId >= 0 ? "Target: " + Modeler.selectedObject.name
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

        Text { text: "ADD NEW"; color: "#888"; font.pixelSize: 10; font.bold: true }

        GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: 4
            rowSpacing: 4

            AppButton { text: "Noise"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 0 ? "#E10600" : "#3e3e42"; color: addType === 0 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 0; onClicked: addType = 0 }
            AppButton { text: "Spring"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 1 ? "#E10600" : "#3e3e42"; color: addType === 1 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 1; onClicked: addType = 1 }
            AppButton { text: "LookAt"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 2 ? "#E10600" : "#3e3e42"; color: addType === 2 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 2; onClicked: addType = 2 }
            AppButton { text: "Attach"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 3 ? "#E10600" : "#3e3e42"; color: addType === 3 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 3; onClicked: addType = 3 }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: addingTarget ? "Click target object..." : "Set Target (click)"
                height: 26
                Layout.fillWidth: true
                bgcolor: addingTarget ? "#E10600" : "#3e3e42"
                color: addingTarget ? "#121212" : "#fff"
                font.pixelSize: 10
                font.bold: addingTarget
                onClicked: startAddTarget()
            }

            AppButton {
                text: "Add"
                height: 26
                width: 60
                bgcolor: (objectId >= 0 && addTargetId >= 0) ? "#E10600" : "#3e3e42"
                color: (objectId >= 0 && addTargetId >= 0) ? "#121212" : "#888"
                font.pixelSize: 10
                font.bold: true
                enabled: objectId >= 0 && addTargetId >= 0
                onClicked: addController()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "CONTROLLER LIST"; color: "#888"; font.pixelSize: 10; font.bold: true }

        Column {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Repeater {
                id: controllerRepeater
                model: objectId >= 0 ? Modeler.controllerList(objectId) : []

                delegate: Rectangle {
                    Layout.fillWidth: true
                    height: (data.type === 0 || data.type === 1) ? 66 : 46
                    color: "#2a2a2e"
                    border.color: "#333"
                    border.width: 1
                    radius: 3

                    property variant data: modelData
                    property int index: index

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            CheckBox {
                                checked: data.enabled
                                onCheckedChanged: {
                                    if (objectId >= 0) Modeler.controllerSetEnabled(objectId, index, checked)
                                }
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Text {
                                text: ["Noise", "Spring", "LookAt", "Attachment"][data.type]
                                color: "#fff"
                                font.pixelSize: 10
                                font.bold: true
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Text {
                                text: data.channel
                                color: "#aaa"
                                font.pixelSize: 9
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Text {
                                text: "→ " + data.targetName
                                color: "#aaa"
                                font.pixelSize: 9
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                Layout.alignment: Qt.AlignVCenter
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
                                    Modeler.controllerRemove(objectId, index)
                                }
                            }
                        }

                        // Noise / Spring: amplitude + frequency.
                        RowLayout {
                            Layout.fillWidth: true
                            visible: data.type === 0 || data.type === 1
                            spacing: 6

                            Text { text: "A"; color: "#888"; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter }
                            TextField {
                                Layout.preferredWidth: 50
                                text: data.amplitude.toFixed(2)
                                color: "#fff"
                                font.pixelSize: 10
                                onEditingFinished: if (objectId >= 0) Modeler.controllerSetParams(objectId, index, parseFloat(text), data.frequency, data.phase, data.stiffness, data.damping)
                            }
                            Text { text: "f"; color: "#888"; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter }
                            TextField {
                                Layout.preferredWidth: 50
                                text: data.frequency.toFixed(2)
                                color: "#fff"
                                font.pixelSize: 10
                                onEditingFinished: if (objectId >= 0) Modeler.controllerSetParams(objectId, index, data.amplitude, parseFloat(text), data.phase, data.stiffness, data.damping)
                            }
                        }
                    }
                }
            }
        }
    }
}
