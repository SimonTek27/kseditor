import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: propsPanel
    anchors.fill: parent
    color: "transparent"

    signal closeRequested()

    property var obj: Modeler ? Modeler.selectedObject : null

    function updateFromSelection() {
        obj = Modeler ? Modeler.selectedObject : null
        if (obj) {
            nameField.text = obj.name || ""
            posXField.value = obj.position ? obj.position.x : 0
            posYField.value = obj.position ? obj.position.y : 0
            posZField.value = obj.position ? obj.position.z : 0
            rotXField.value = obj.rotation ? obj.rotation.x : 0
            rotYField.value = obj.rotation ? obj.rotation.y : 0
            rotZField.value = obj.rotation ? obj.rotation.z : 0
            scaleXField.value = obj.scale ? obj.scale.x : 1
            scaleYField.value = obj.scale ? obj.scale.y : 1
            scaleZField.value = obj.scale ? obj.scale.z : 1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 32
            color: "#252526"
            Layout.fillWidth: true

            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "PROPERTIES"
                color: "#E10600"
                font.pixelSize: 11
                font.bold: true
            }

            Text {
                anchors { right: parent.right; rightMargin: 30; verticalCenter: parent.verticalCenter }
                text: obj ? obj.type : "No selection"
                color: "#666"
                font.pixelSize: 9
            }

            Rectangle {
                anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                width: 18; height: 18; radius: 2; color: "#E10600"
                Text { anchors.centerIn: parent; text: "X"; color: "#fff"; font.pixelSize: 10; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: closeRequested() }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            enabled: obj !== null

            ColumnLayout {
                width: parent.width
                spacing: 4

                Item { height: 4 }

                Text {
                    text: "TRANSFORM"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10; Layout.rightMargin: 10
                    height: 22
                    color: "#252526"
                    radius: 3

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 3
                        Text { text: "Name"; color: "#888"; font.pixelSize: 9; width: 50 }
                        TextField {
                            id: nameField
                            Layout.fillWidth: true; height: 18
                            font.pixelSize: 9; color: "#fff"
                            placeholderText: "Object name"
                            onEditingFinished: { if (obj) obj.name = text }
                        }
                    }
                }

                Item { height: 2 }

                // Position
                Text {
                    text: "Position"
                    color: "#888"; font.pixelSize: 9; font.bold: true
                    leftPadding: 10
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10; Layout.rightMargin: 10
                    spacing: 4

                    Text { text: "X"; color: "#E10600"; font.pixelSize: 9; font.bold: true; width: 12 }
                    SpinBox {
                        id: posXField
                        Layout.fillWidth: true; height: 22
                        from: -10000; to: 10000; decimals: 3; stepSize: 0.1
                        editable: true
                        font.pixelSize: 9
                        onValueModified: { if (obj) obj.position = Qt.vector3d(value, posYField.value, posZField.value) }
                    }
                    Text { text: "Y"; color: "#4CAF50"; font.pixelSize: 9; font.bold: true; width: 12 }
                    SpinBox {
                        id: posYField
                        Layout.fillWidth: true; height: 22
                        from: -10000; to: 10000; decimals: 3; stepSize: 0.1
                        editable: true
                        font.pixelSize: 9
                        onValueModified: { if (obj) obj.position = Qt.vector3d(posXField.value, value, posZField.value) }
                    }
                    Text { text: "Z"; color: "#4488ff"; font.pixelSize: 9; font.bold: true; width: 12 }
                    SpinBox {
                        id: posZField
                        Layout.fillWidth: true; height: 22
                        from: -10000; to: 10000; decimals: 3; stepSize: 0.1
                        editable: true
                        font.pixelSize: 9
                        onValueModified: { if (obj) obj.position = Qt.vector3d(posXField.value, posYField.value, value) }
                    }
                }

                // Rotation
                Text {
                    text: "Rotation"
                    color: "#888"; font.pixelSize: 9; font.bold: true
                    leftPadding: 10
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10; Layout.rightMargin: 10
                    spacing: 4

                    Text { text: "X"; color: "#E10600"; font.pixelSize: 9; font.bold: true; width: 12 }
                    SpinBox {
                        id: rotXField
                        Layout.fillWidth: true; height: 22
                        from: -360; to: 360; decimals: 1; stepSize: 1
                        editable: true
                        font.pixelSize: 9
                        onValueModified: { if (obj) obj.rotation = Qt.vector3d(value, rotYField.value, rotZField.value) }
                    }
                    Text { text: "Y"; color: "#4CAF50"; font.pixelSize: 9; font.bold: true; width: 12 }
                    SpinBox {
                        id: rotYField
                        Layout.fillWidth: true; height: 22
                        from: -360; to: 360; decimals: 1; stepSize: 1
                        editable: true
                        font.pixelSize: 9
                        onValueModified: { if (obj) obj.rotation = Qt.vector3d(rotXField.value, value, rotZField.value) }
                    }
                    Text { text: "Z"; color: "#4488ff"; font.pixelSize: 9; font.bold: true; width: 12 }
                    SpinBox {
                        id: rotZField
                        Layout.fillWidth: true; height: 22
                        from: -360; to: 360; decimals: 1; stepSize: 1
                        editable: true
                        font.pixelSize: 9
                        onValueModified: { if (obj) obj.rotation = Qt.vector3d(rotXField.value, rotYField.value, value) }
                    }
                }

                // Scale
                Text {
                    text: "Scale"
                    color: "#888"; font.pixelSize: 9; font.bold: true
                    leftPadding: 10
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10; Layout.rightMargin: 10
                    spacing: 4

                    Text { text: "X"; color: "#E10600"; font.pixelSize: 9; font.bold: true; width: 12 }
                    SpinBox {
                        id: scaleXField
                        Layout.fillWidth: true; height: 22
                        from: 0.01; to: 100; decimals: 3; stepSize: 0.1
                        editable: true
                        font.pixelSize: 9
                        onValueModified: { if (obj) obj.scale = Qt.vector3d(value, scaleYField.value, scaleZField.value) }
                    }
                    Text { text: "Y"; color: "#4CAF50"; font.pixelSize: 9; font.bold: true; width: 12 }
                    SpinBox {
                        id: scaleYField
                        Layout.fillWidth: true; height: 22
                        from: 0.01; to: 100; decimals: 3; stepSize: 0.1
                        editable: true
                        font.pixelSize: 9
                        onValueModified: { if (obj) obj.scale = Qt.vector3d(scaleXField.value, value, scaleZField.value) }
                    }
                    Text { text: "Z"; color: "#4488ff"; font.pixelSize: 9; font.bold: true; width: 12 }
                    SpinBox {
                        id: scaleZField
                        Layout.fillWidth: true; height: 22
                        from: 0.01; to: 100; decimals: 3; stepSize: 0.1
                        editable: true
                        font.pixelSize: 9
                        onValueModified: { if (obj) obj.scale = Qt.vector3d(scaleXField.value, scaleYField.value, value) }
                    }
                }

                // Uniform scale lock
                RowLayout {
                    Layout.leftMargin: 10; Layout.rightMargin: 10
                    CheckBox {
                        id: uniformScale
                        height: 16
                        text: ""
                        font.pixelSize: 9
                    }
                    Text {
                        text: "Uniform Scale"
                        color: "#888"
                        font.pixelSize: 9
                    }
                }

                Item { height: 6 }

                // Selection state
                Text {
                    text: "OBJECT INFO"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10; Layout.rightMargin: 10
                    height: 22
                    color: "#252526"
                    radius: 3

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        Text { text: "ID:"; color: "#888"; font.pixelSize: 9 }
                        Text { text: obj ? obj.id : "-"; color: "#ccc"; font.pixelSize: 9; font.family: "monospace" }
                        Item { Layout.fillWidth: true }
                        Text { text: "Type:"; color: "#888"; font.pixelSize: 9 }
                        Text { text: obj ? obj.type : "-"; color: "#ccc"; font.pixelSize: 9 }
                    }
                }

                // Visibility
                RowLayout {
                    Layout.leftMargin: 10
                    CheckBox {
                        id: visibleCheck
                        height: 16
                        checked: obj ? obj.selected : false
                        font.pixelSize: 9
                    }
                    Text {
                        text: "Visible"
                        color: "#888"
                        font.pixelSize: 9
                    }
                    Item { Layout.fillWidth: true }
                    AppButton {
                        text: "Focus (F)"
                        height: 22
                        font.pixelSize: 9
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: { if (Modeler) Modeler.focusOnSelected() }
                    }
                }

                Item { height: 8 }

                // Reset transform
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10; Layout.rightMargin: 10
                    height: 26
                    color: "#252526"
                    radius: 3

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 4

                        AppButton {
                            text: "Reset Position"; height: 20; font.pixelSize: 8
                            bgcolor: "#3e3e42"; color: "#ffffff"
                            Layout.fillWidth: true
                            onClicked: { if (obj) obj.position = Qt.vector3d(0,0,0); updateFromSelection() }
                        }
                        AppButton {
                            text: "Reset Rotation"; height: 20; font.pixelSize: 8
                            bgcolor: "#3e3e42"; color: "#ffffff"
                            Layout.fillWidth: true
                            onClicked: { if (obj) obj.rotation = Qt.vector3d(0,0,0); updateFromSelection() }
                        }
                        AppButton {
                            text: "Reset Scale"; height: 20; font.pixelSize: 8
                            bgcolor: "#3e3e42"; color: "#ffffff"
                            Layout.fillWidth: true
                            onClicked: { if (obj) obj.scale = Qt.vector3d(1,1,1); updateFromSelection() }
                        }
                    }
                }

                Item { height: 80 }
            }
        }
    }

    Connections {
        target: Modeler
        function onSelectionChanged() {
            updateFromSelection()
        }
    }

    Component.onCompleted: updateFromSelection()
}


