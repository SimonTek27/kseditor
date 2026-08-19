import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: modPanel
    width: 340
    height: 520
    color: "#1e1e1e"
    border.color: "#E10600"
    border.width: 1
    radius: 6

    signal closePanel()

    property var stackModel: []
    property int selectedIndex: -1

    function refresh() {
        stackModel = Modeler.modifierStackList()
    }

    onVisibleChanged: {
        if (visible) refresh()
    }

    Connections {
        target: Modeler
        function onModifierStackChanged() {
            refresh()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "MODIFIER STACK"
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

        Item { height: 2 }

        Text {
            text: "Modifiers:  " + (stackModel ? stackModel.length : 0)
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(220, modPanel.height * 0.45)
            color: "#252526"
            border.color: "#333"
            border.width: 1
            radius: 3

            ListView {
                id: modList
                anchors.fill: parent
                anchors.margins: 2
                clip: true
                spacing: 2
                model: stackModel

                delegate: Rectangle {
                    id: modRow
                    width: modList.width
                    height: 30
                    radius: 3
                    color: selectedIndex === index ? "#3a3a3e" : (modelData.enabled ? "#2c2c2e" : "#222224")
                    border.color: selectedIndex === index ? "#E10600" : "#333"
                    border.width: 1

                    MouseArea {
                        anchors.fill: parent
                        onClicked: selectedIndex = index
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 4
                        spacing: 6

                        Rectangle {
                            width: 14
                            height: 14
                            radius: 3
                            color: modelData.enabled ? "#E10600" : "#555"
                            border.color: "#000"
                            border.width: 1

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    Modeler.modifierStackSetEnabled(index, !modelData.enabled)
                                    selectedIndex = index
                                }
                            }
                        }

                        Text {
                            text: (stackModel.length - index) + ". " + modelData.type
                            color: modelData.enabled ? "#ffffff" : "#777"
                            font.pixelSize: 11
                            font.bold: modModelData.enabled
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        AppButton {
                            text: "\u2191"
                            height: 22
                            width: 24
                            bgcolor: "#3e3e42"
                            color: "#ffffff"
                            font.pixelSize: 11
                            enabled: index > 0
                            onClicked: {
                                Modeler.modifierStackMove(index, index - 1)
                                selectedIndex = index - 1
                            }
                        }

                        AppButton {
                            text: "\u2193"
                            height: 22
                            width: 24
                            bgcolor: "#3e3e42"
                            color: "#ffffff"
                            font.pixelSize: 11
                            enabled: index < stackModel.length - 1
                            onClicked: {
                                Modeler.modifierStackMove(index, index + 1)
                                selectedIndex = index + 1
                            }
                        }

                        AppButton {
                            text: "\u2715"
                            height: 22
                            width: 24
                            bgcolor: "#E10600"
                            color: "#ffffff"
                            font.pixelSize: 10
                            onClicked: {
                                Modeler.modifierStackRemove(index)
                                if (selectedIndex >= stackModel.length)
                                    selectedIndex = stackModel.length - 1
                            }
                        }
                    }
                }
            }
        }

        Text {
            text: "Add Modifier:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            ComboBox {
                id: addCombo
                Layout.fillWidth: true
                height: 28
                model: Modeler.modifierStackTypes()
                currentIndex: 0
                font.pixelSize: 11
            }

            AppButton {
                text: "Add"
                height: 28
                width: 56
                bgcolor: "#E10600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 11
                onClicked: {
                    if (addCombo.currentIndex >= 0 && addCombo.currentText) {
                        Modeler.modifierStackAdd(addCombo.currentText)
                        selectedIndex = Modeler.modifierStackCount() - 1
                    }
                }
            }
        }

        Item { height: 2 }

        Text {
            text: "Parameters"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            spacing: 6
            visible: selectedIndex >= 0 && selectedIndex < stackModel.length
                     && stackModel[selectedIndex].type === "Subdivision"

            AppButton {
                text: "Crease Selected Edges"
                height: 28
                Layout.fillWidth: true
                bgcolor: "#c06000"
                color: "#ffffff"
                font.pixelSize: 9
                onClicked: Modeler.subdivisionCreaseSelectedEdges(selectedIndex)
            }

            AppButton {
                text: "Pin Selected Verts"
                height: 28
                Layout.fillWidth: true
                bgcolor: "#2c6cb0"
                color: "#ffffff"
                font.pixelSize: 9
                onClicked: Modeler.subdivisionPinSelectedVertices(selectedIndex)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            spacing: 6
            visible: selectedIndex >= 0 && selectedIndex < stackModel.length
                     && stackModel[selectedIndex].type === "Subdivision"

            AppButton {
                text: "Clear Creases / Pins"
                height: 28
                Layout.fillWidth: true
                bgcolor: "#3e3e42"
                color: "#ffffff"
                font.pixelSize: 9
                onClicked: Modeler.subdivisionClearPinnedVertices(selectedIndex)
            }

            AppButton {
                text: "Apply (Bake)"
                height: 28
                Layout.fillWidth: true
                bgcolor: "#E10600"
                color: "#ffffff"
                font.pixelSize: 9
                onClicked: Modeler.modifierStackFreeze()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(120, modPanel.height * 0.25)
            color: "#252526"
            border.color: "#333"
            border.width: 1
            radius: 3

            ScrollView {
                anchors.fill: parent
                anchors.margins: 2
                clip: true

                ColumnLayout {
                    width: parent.width
                    spacing: 4

                    Repeater {
                        id: paramRepeater
                        model: selectedIndex >= 0 && selectedIndex < stackModel.length
                               ? Modeler.modifierStackParamNames(selectedIndex)
                               : []

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Text {
                                text: modelData
                                color: "#aaa"
                                font.pixelSize: 10
                                Layout.preferredWidth: 110
                                elide: Text.ElideRight
                            }

                            TextField {
                                id: paramField
                                Layout.fillWidth: true
                                height: 24
                                color: "#ffffff"
                                font.pixelSize: 10
                                selectByMouse: true
                                placeholderText: "value"
                                text: selectedIndex >= 0 ? (stackModel[selectedIndex].params[modelData] || "") : ""

                                onEditingFinished: {
                                    if (selectedIndex >= 0 && selectedIndex < stackModel.length)
                                        Modeler.modifierStackSetParam(selectedIndex, modelData, text)
                                }
                            }
                        }
                    }
                }
            }
        }

        Item { height: 4 }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AppButton {
                height: 30
                text: "Freeze"
                bgcolor: "#ff6600"
                color: "#121212"
                Layout.fillWidth: true
                font.bold: true
                enabled: Modeler.modifierStackCount() > 0
                onClicked: {
                    Modeler.modifierStackFreeze()
                    selectedIndex = -1
                }
            }

            AppButton {
                height: 30
                text: "Clear"
                bgcolor: "#3e3e42"
                color: "#ffffff"
                Layout.fillWidth: true
                font.bold: true
                enabled: Modeler.modifierStackCount() > 0
                onClicked: {
                    Modeler.modifierStackClear()
                    selectedIndex = -1
                }
            }
        }
    }
}
