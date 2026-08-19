import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: fgPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property var groupModel: []

    function refreshGroups() {
        groupModel = Modeler.faceGroupList ? Modeler.faceGroupList() : []
        groupsList.model = groupModel
        pinInfo.text = fgPanel.objLabel()
    }

    function objLabel() {
        if (objectId < 0) return "No object selected"
        return "Target: " + Modeler.selectedObject.name
    }

    function assign(remove) {
        if (objectId < 0) return
        var idx = groupsList.currentIndex
        if (remove) idx = -1
        if (Modeler.faceGroupAssignSelected) {
            var n = Modeler.faceGroupAssignSelected(idx)
            if (Modeler.statusMessage) Modeler.statusMessage("Face groups: " + n + " face(s) assigned")
        }
    }

    Connections {
        target: Modeler
        function onFaceGroupsChanged() { refreshGroups() }
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
            refreshGroups()
        }
        function onSceneChanged() { refreshGroups() }
    }

    Component.onCompleted: refreshGroups()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text { text: "FACE GROUPS"; color: "#E10600"; font.pixelSize: 13; font.bold: true; Layout.fillWidth: true }
            AppButton { text: "X"; height: 24; width: 26; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10; font.bold: true; onClicked: closePanel() }
        }

        Text { id: pinInfo; text: objLabel(); color: objectId >= 0 ? "#aaa" : "#E10600"; font.pixelSize: 10; font.bold: objectId >= 0; wrapMode: Text.WordWrap }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "GROUPS (assign/remove face groups, select faces in Face mode first)"; color: "#888"; font.pixelSize: 10; font.bold: true }

        ListView {
            id: groupsList
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            clip: true
            spacing: 2
            highlight: Rectangle { color: "#2a2a2e" }
            delegate: Rectangle {
                width: groupsList.width
                height: 26
                color: "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 6
                    Rectangle { width: 12; height: 12; radius: 3; color: modelData && modelData.color ? "#" + modelData.color.toString(16).padStart(6, "0") : "#888" }
                    Text { text: modelData ? modelData.name : ""; color: "#ddd"; font.pixelSize: 10; Layout.fillWidth: true; elide: Text.ElideRight }
                    Text { text: modelData ? modelData.count + " f" : ""; color: "#888"; font.pixelSize: 9 }
                    AppButton { text: "Vis"; height: 20; width: 34; bgcolor: modelData && modelData.visible ? "#2a6e2a" : "#3e3e42"; color: "#fff"; font.pixelSize: 8; onClicked: if (Modeler.faceGroupSetVisible) Modeler.faceGroupSetVisible(modelData.index, !modelData.visible) }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: groupsList.currentIndex = modelData.index
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: "group name"
                color: "#ddd"
                font.pixelSize: 10
                background: Rectangle { color: "#2a2a2e"; border.color: "#444"; radius: 3 }
            }
            AppButton { text: "Add"; height: 26; bgcolor: "#E10600"; color: "#121212"; font.bold: true; font.pixelSize: 10; onClicked: if (Modeler.faceGroupCreate) { Modeler.faceGroupCreate(nameField.text, 0xE10600); nameField.text = "" } }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton { text: "Assign Sel"; height: 26; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; Layout.fillWidth: true; onClicked: assign(false) }
            AppButton { text: "Remove Sel"; height: 26; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; Layout.fillWidth: true; onClicked: assign(true) }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton { text: "Rename"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; Layout.fillWidth: true; onClicked: { var m = groupsList.model[groupsList.currentIndex]; if (m && Modeler.faceGroupRename) Modeler.faceGroupRename(m.index, nameField.text || m.name) } }
            AppButton { text: "Color"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; Layout.fillWidth: true; onClicked: { var m = groupsList.model[groupsList.currentIndex]; if (m && Modeler.faceGroupSetColor) Modeler.faceGroupSetColor(m.index, 0x00aaff) } }
            AppButton { text: "Delete"; height: 24; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 9; Layout.fillWidth: true; onClicked: { var m = groupsList.model[groupsList.currentIndex]; if (m && Modeler.faceGroupRemove) Modeler.faceGroupRemove(m.index) } }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "Tip: in Face sub-object mode select faces, then Assign Sel to the highlighted group. Vis toggles group visibility (masking)."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
}
