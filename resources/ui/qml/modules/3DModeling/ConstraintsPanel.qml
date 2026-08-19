import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: constraintsPanel
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

    function addConstraint() {
        if (objectId >= 0 && addTargetId >= 0) {
            if (addType === 4)
                Modeler.constraintAddPath(objectId, addTargetId, 64, 0.5, false)
            else if (addType === 5)
                Modeler.constraintAddAttachment(objectId, addTargetId, 0, 0, 0, 0)
            else
                Modeler.constraintAdd(objectId, addType, addTargetId, 0,0,0, 0,0,0)
            addTargetId = -1
        }
    }

    function refresh() {
        // force re-eval of list
    }

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            addingTarget = false
            addTargetId = -1
        }
        function onConstraintChanged() {
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
                text: "CONSTRAINTS"
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

            AppButton { text: "Point"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 0 ? "#E10600" : "#3e3e42"; color: addType === 0 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 0; onClicked: addType = 0 }
            AppButton { text: "Orientation"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 1 ? "#E10600" : "#3e3e42"; color: addType === 1 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 1; onClicked: addType = 1 }
            AppButton { text: "Aim"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 2 ? "#E10600" : "#3e3e42"; color: addType === 2 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 2; onClicked: addType = 2 }
            AppButton { text: "Parent"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 3 ? "#E10600" : "#3e3e42"; color: addType === 3 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 3; onClicked: addType = 3 }
            AppButton { text: "Path"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 4 ? "#E10600" : "#3e3e42"; color: addType === 4 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 4; onClicked: addType = 4 }
            AppButton { text: "Attach"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 5 ? "#E10600" : "#3e3e42"; color: addType === 5 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 5; onClicked: addType = 5 }
            AppButton { text: "Link"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 6 ? "#E10600" : "#3e3e42"; color: addType === 6 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 6; onClicked: addType = 6 }
            AppButton { text: "Spring"; height: 26; checkable: true; autoExclusive: true; bgcolor: addType === 7 ? "#E10600" : "#3e3e42"; color: addType === 7 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 7; onClicked: addType = 7 }
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
                onClicked: addConstraint()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "CONSTRAINT LIST"; color: "#888"; font.pixelSize: 10; font.bold: true }

        Column {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Repeater {
                id: constraintRepeater
                model: objectId >= 0 ? Modeler.constraintList(objectId) : []

                delegate: Rectangle {
                    Layout.fillWidth: true
                    height: (data.type === 4 || data.type === 7) ? 92 : 58
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

                            // Enabled checkbox
                            CheckBox {
                                checked: data.enabled
                                onCheckedChanged: {
                                    if (objectId >= 0) Modeler.constraintSetEnabled(objectId, index, checked)
                                }
                                Layout.alignment: Qt.AlignVCenter
                            }

                            // Type label
                            Text {
                                text: ["Point", "Orientation", "Aim", "Parent", "Path", "Attachment", "Link", "Spring"][data.type]
                                color: "#fff"
                                font.pixelSize: 10
                                font.bold: true
                                Layout.alignment: Qt.AlignVCenter
                            }

                            // Target name
                            Text {
                                text: "→ " + data.targetName
                                color: "#aaa"
                                font.pixelSize: 9
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                            }

                            // Edit offset button
                            AppButton {
                                text: "Offset"
                                height: 22
                                width: 50
                                bgcolor: "#3e3e42"
                                color: "#fff"
                                font.pixelSize: 9
                                onClicked: {
                                    // Simple: open a small dialog or just set to zero for now
                                    Modeler.constraintSetOffset(objectId, index, 0, 0, 0)
                                }
                            }

                            // Remove button
                            AppButton {
                                text: "✕"
                                height: 22
                                width: 24
                                bgcolor: "#5a1a1a"
                                color: "#ff8888"
                                font.pixelSize: 10
                                font.bold: true
                                onClicked: {
                                    Modeler.constraintRemove(objectId, index)
                                }
                            }
                        }

                        // Path: param T slider + follow toggle
                        RowLayout {
                            Layout.fillWidth: true
                            visible: data.type === 4
                            spacing: 6

                            Text { text: "T"; color: "#888"; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter }
                            Slider {
                                Layout.fillWidth: true
                                from: 0
                                to: 1
                                stepSize: 0.01
                                value: data.param
                                onMoved: if (objectId >= 0) Modeler.constraintSetParam(objectId, index, value)
                            }
                            CheckBox {
                                text: "Follow"
                                checked: data.follow
                                onToggled: if (objectId >= 0) Modeler.constraintSetFollow(objectId, index, checked)
                            }
                        }

                        // Spring: stiffness / damping
                        RowLayout {
                            Layout.fillWidth: true
                            visible: data.type === 7
                            spacing: 6

                            Text { text: "k"; color: "#888"; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter }
                            TextField {
                                Layout.preferredWidth: 60
                                text: data.stiffness.toFixed(1)
                                color: "#fff"
                                font.pixelSize: 10
                                onEditingFinished: if (objectId >= 0) Modeler.constraintSetSpringParams(objectId, index, parseFloat(text), data.damping)
                            }
                            Text { text: "d"; color: "#888"; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter }
                            TextField {
                                Layout.preferredWidth: 60
                                text: data.damping.toFixed(1)
                                color: "#fff"
                                font.pixelSize: 10
                                onEditingFinished: if (objectId >= 0) Modeler.constraintSetSpringParams(objectId, index, data.stiffness, parseFloat(text))
                            }
                        }
                    }
                }
            }
        }
    }
}