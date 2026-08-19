import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: wirePanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property int addDriverId: -1
    property bool pickingDriver: false
    property string driverProp: "position.x"
    property string drivenProp: "position.y"

    property var propOptions: ["position.x", "position.y", "position.z",
                               "rotation.x", "rotation.y", "rotation.z",
                               "scale.x", "scale.y", "scale.z",
                               "visibility", "opacity", "metallic", "roughness"]

    function startPickDriver() {
        pickingDriver = true
        addDriverId = -1
    }

    function confirmPickDriver(id) {
        if (pickingDriver && id !== objectId) {
            addDriverId = id
            pickingDriver = false
        }
    }

    function addWire() {
        if (objectId >= 0 && addDriverId >= 0) {
            Modeler.wireAdd(addDriverId, driverProp, objectId, drivenProp, 1.0, 0.0)
            addDriverId = -1
        }
    }

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            pickingDriver = false
            addDriverId = -1
        }
        function onWireChanged() {
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
                text: "WIRE PARAMETERS"
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
            text: objectId >= 0 ? "Driven: " + Modeler.selectedObject.name
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

        Text { text: "ADD NEW WIRE (driven = selected)"; color: "#888"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: pickingDriver ? "Click driver object..." : "Driver (click)"
                height: 26
                Layout.fillWidth: true
                bgcolor: pickingDriver ? "#E10600" : "#3e3e42"
                color: pickingDriver ? "#121212" : "#fff"
                font.pixelSize: 10
                font.bold: pickingDriver
                onClicked: startPickDriver()
            }

            AppButton {
                text: "Add"
                height: 26
                width: 60
                bgcolor: (objectId >= 0 && addDriverId >= 0) ? "#E10600" : "#3e3e42"
                color: (objectId >= 0 && addDriverId >= 0) ? "#121212" : "#888"
                font.pixelSize: 10
                font.bold: true
                enabled: objectId >= 0 && addDriverId >= 0
                onClicked: addWire()
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 6
            rowSpacing: 4

            Text { text: "Driver prop"; color: "#888"; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter }
            ComboBox {
                Layout.fillWidth: true
                model: propOptions
                currentIndex: Math.max(0, propOptions.indexOf(driverProp))
                font.pixelSize: 10
                onActivated: driverProp = propOptions[currentIndex]
            }
            Text { text: "Driven prop"; color: "#888"; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter }
            ComboBox {
                Layout.fillWidth: true
                model: propOptions
                currentIndex: Math.max(0, propOptions.indexOf(drivenProp))
                font.pixelSize: 10
                onActivated: drivenProp = propOptions[currentIndex]
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "WIRE LIST (drives selected)"; color: "#888"; font.pixelSize: 10; font.bold: true }

        Column {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Repeater {
                id: wireRepeater
                model: objectId >= 0 ? Modeler.wireList(objectId) : []

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
                                if (objectId >= 0) Modeler.wireSetEnabled(objectId, index, checked)
                            }
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Text {
                            text: data.driverName + "." + data.driverProp
                            color: "#fff"
                            font.pixelSize: 9
                            elide: Text.ElideRight
                            Layout.preferredWidth: 130
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Text {
                            text: "→ " + data.drivenProp
                            color: "#aaa"
                            font.pixelSize: 9
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Text {
                            text: data.scale.toFixed(2) + "x"
                            color: "#888"
                            font.pixelSize: 9
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
                                Modeler.wireRemove(objectId, index)
                            }
                        }
                    }
                }
            }
        }
    }
}
