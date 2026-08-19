import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: setPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property string currentSet: ""
    property var setModel: []

    function refreshSets() {
        setModel = Modeler.selectionSetNames ? Modeler.selectionSetNames() : []
        setsList.model = setModel
    }

    function selectedCount() {
        var names = Modeler.selectedObjectNames ? Modeler.selectedObjectNames() : []
        return names.length
    }

    function recall() {
        if (currentSet !== "" && Modeler.recallSelectionSet)
            Modeler.recallSelectionSet(currentSet)
    }

    Connections {
        target: Modeler
        function onSelectionSetsChanged() { refreshSets() }
        function onSelectionChanged() { selStatusText.text = "Selected: " + setPanel.selectedCount() + " object(s)" }
    }

    Component.onCompleted: {
        refreshSets()
        selStatusText.text = "Selected: " + setPanel.selectedCount() + " object(s)"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "SELECTION SETS"
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
            id: selStatusText
            color: "#aaa"
            font.pixelSize: 10
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "SET NAME"; color: "#888"; font.pixelSize: 10; font.bold: true }
        TextField {
            id: nameField
            Layout.fillWidth: true
            height: 26
            placeholderText: "set name"
            selectByMouse: true
            color: "#ffffff"
            background: Rectangle { color: "#2d2d30"; radius: 3; border.color: "#444"; border.width: 1 }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            AppButton {
                Layout.fillWidth: true
                height: 28
                text: "Save Selection"
                bgcolor: "#E10600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 10
                enabled: setPanel.selectedCount() > 0 && nameField.text.trim() !== ""
                onClicked: { Modeler.createSelectionSet(nameField.text); nameField.text = "" }
            }
            AppButton {
                Layout.fillWidth: true
                height: 28
                text: "Add Selected"
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                enabled: setPanel.selectedCount() > 0 && currentSet !== ""
                onClicked: Modeler.addSelectionToSet(currentSet)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "SETS (" + setModel.length + ")"; color: "#888"; font.pixelSize: 10; font.bold: true }

        ListView {
            id: setsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            model: []

            delegate: Rectangle {
                width: setsList.width
                height: 30
                radius: 3
                color: currentSet === modelData ? "#3a3a3e" : "#2d2d30"
                border.color: currentSet === modelData ? "#E10600" : "#333"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 4

                    Text {
                        text: modelData
                        color: "#ffffff"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Text {
                        text: (Modeler.selectionSetMemberCount ? Modeler.selectionSetMemberCount(modelData) : 0) + " obj"
                        color: "#999"
                        font.pixelSize: 9
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        currentSet = modelData
                        nameField.text = modelData
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            AppButton {
                Layout.fillWidth: true
                height: 28
                text: "Recall"
                bgcolor: "#E10600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 10
                enabled: currentSet !== ""
                onClicked: setPanel.recall()
            }
            AppButton {
                Layout.fillWidth: true
                height: 28
                text: "Clear Members"
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                enabled: currentSet !== ""
                onClicked: Modeler.clearSelectionSet(currentSet)
            }
            AppButton {
                Layout.fillWidth: true
                height: 28
                text: "Delete"
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                enabled: currentSet !== ""
                onClicked: { Modeler.deleteSelectionSet(currentSet); currentSet = ""; nameField.text = "" }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Text { text: "Rename to:"; color: "#888"; font.pixelSize: 10 }
            TextField {
                id: renameField
                Layout.fillWidth: true
                height: 26
                placeholderText: "new name"
                selectByMouse: true
                color: "#ffffff"
                background: Rectangle { color: "#2d2d30"; radius: 3; border.color: "#444"; border.width: 1 }
            }
            AppButton {
                text: "Rename"
                height: 28
                width: 70
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                enabled: currentSet !== "" && renameField.text.trim() !== ""
                onClicked: {
                    if (Modeler.renameSelectionSet(currentSet, renameField.text)) {
                        currentSet = renameField.text
                        nameField.text = renameField.text
                    }
                    renameField.text = ""
                }
            }
        }

        Text {
            text: "Hint: use Ctrl+click (or Shift+click) on objects in the outliner to multi-select, "
                 + "then save the selection as a named set. Sets persist in the .ks3d file."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
