import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: lightPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int addType: 0
    property variant lightModel: []
    property int editingObjectId: -1
    property int iesObjectId: -1
    property string iesFilterLabel: ""

    function refresh() {
        lightModel = Modeler && Modeler.lightList ? Modeler.lightList() : []
    }

    Connections {
        target: Modeler
        function onLightsChanged() {
            lightPanel.refresh()
        }
        function onSceneChanged() {
            lightPanel.refresh()
        }
    }

    ColorDialog {
        id: lightColorDialog
        title: "Light Color"
        onAccepted: {
            if (editingObjectId >= 0) {
                Modeler.lightSetColor(editingObjectId,
                                      selectedColor.r / 255, selectedColor.g / 255, selectedColor.b / 255)
            }
            editingObjectId = -1
        }
        onRejected: editingObjectId = -1
    }

    FileDialog {
        id: iesDialog
        title: "Select IES profile"
        fileMode: FileDialog.OpenFile
        nameFilters: ["IES Photometric (*.ies)", "All Files (*)"]
        onAccepted: {
            var path = fileUrl.toString().replace("file:///", "").replace("file://", "")
            if (iesObjectId >= 0)
                Modeler.lightSetIesProfile(iesObjectId, path)
            iesObjectId = -1
        }
        onRejected: iesObjectId = -1
    }

    Component.onCompleted: refresh()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "LIGHT LISTER"
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

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "ADD NEW LIGHT"; color: "#888"; font.pixelSize: 10; font.bold: true }

        GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: 4
            rowSpacing: 4

            AppButton { text: "Dir"; height: 26; bgcolor: addType === 0 ? "#E10600" : "#3e3e42"; color: addType === 0 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 0; onClicked: addType = 0 }
            AppButton { text: "Point"; height: 26; bgcolor: addType === 1 ? "#E10600" : "#3e3e42"; color: addType === 1 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 1; onClicked: addType = 1 }
            AppButton { text: "Spot"; height: 26; bgcolor: addType === 2 ? "#E10600" : "#3e3e42"; color: addType === 2 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 2; onClicked: addType = 2 }
            AppButton { text: "Area"; height: 26; bgcolor: addType === 3 ? "#E10600" : "#3e3e42"; color: addType === 3 ? "#121212" : "#fff"; font.pixelSize: 10; checked: addType === 3; onClicked: addType = 3 }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: "Add Light"
                height: 26
                Layout.fillWidth: true
                bgcolor: "#E10600"
                color: "#121212"
                font.pixelSize: 10
                font.bold: true
                onClicked: Modeler.lightCreate(addType, "")
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "SCENE LIGHTS (" + lightModel.length + ")"; color: "#888"; font.pixelSize: 10; font.bold: true }

        Column {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4
            clip: true

            Repeater {
                model: lightModel

                delegate: Rectangle {
                    width: lightPanel.width - 20
                    height: (data.type === 2) ? 128 : 106
                    color: "#2a2a2e"
                    border.color: "#333"
                    border.width: 1
                    radius: 3

                    property variant data: modelData
                    property int lightIndex: index

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            CheckBox {
                                checked: data.enabled
                                onCheckedChanged: if (data.objectId >= 0) Modeler.lightSetEnabled(data.objectId, checked)
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Text {
                                text: data.name
                                color: "#fff"
                                font.pixelSize: 10
                                font.bold: true
                                Layout.alignment: Qt.AlignVCenter
                                Layout.preferredWidth: 90
                                elide: Text.ElideMiddle
                            }

                            ComboBox {
                                Layout.preferredWidth: 92
                                height: 22
                                model: ["Directional", "Point", "Spot", "Area"]
                                currentIndex: data.type
                                font.pixelSize: 9
                                onActivated: if (data.objectId >= 0) Modeler.lightSetType(data.objectId, currentIndex)
                            }

                            Rectangle {
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: 20
                                radius: 3
                                color: data.color
                                border.color: "#555"
                                border.width: 1
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        lightPanel.editingObjectId = data.objectId
                                        lightColorDialog.selectedColor = data.color
                                        lightColorDialog.open()
                                    }
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
                                onClicked: if (data.objectId >= 0) Modeler.lightRemove(data.objectId)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Text { text: "Intensity"; color: "#888"; font.pixelSize: 9; Layout.alignment: Qt.AlignVCenter }
                            Slider {
                                Layout.fillWidth: true
                                from: 0
                                to: 5
                                stepSize: 0.05
                                value: data.intensity
                                onMoved: if (data.objectId >= 0) Modeler.lightSetIntensity(data.objectId, value)
                            }
                            Text {
                                text: data.intensity.toFixed(2)
                                color: "#ccc"
                                font.pixelSize: 9
                                Layout.alignment: Qt.AlignVCenter
                                Layout.preferredWidth: 36
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: data.type === 1 || data.type === 2
                            spacing: 6

                            Text { text: "Range"; color: "#888"; font.pixelSize: 9; Layout.alignment: Qt.AlignVCenter }
                            Slider {
                                Layout.fillWidth: true
                                from: 1
                                to: 200
                                stepSize: 1
                                value: data.range
                                onMoved: if (data.objectId >= 0) Modeler.lightSetRange(data.objectId, value)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: data.type === 2
                            spacing: 6

                            Text { text: "Cone"; color: "#888"; font.pixelSize: 9; Layout.alignment: Qt.AlignVCenter }
                            Slider {
                                Layout.fillWidth: true
                                from: 5
                                to: 160
                                stepSize: 1
                                value: data.spotAngleDeg
                                onMoved: if (data.objectId >= 0) Modeler.lightSetSpotAngle(data.objectId, value)
                            }
                            Text {
                                text: data.spotAngleDeg.toFixed(0) + "°"
                                color: "#ccc"
                                font.pixelSize: 9
                                Layout.alignment: Qt.AlignVCenter
                                Layout.preferredWidth: 32
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Text {
                                text: data.hasIES ? "IES: " + data.iesProfile.split("/").pop().split("\\").pop()
                                                  : "IES: none"
                                color: data.hasIES ? "#8f8" : "#666"
                                font.pixelSize: 9
                                Layout.fillWidth: true
                                elide: Text.ElideMiddle
                            }

                            AppButton {
                                text: data.hasIES ? "Clear" : "Load .ies"
                                height: 22
                                bgcolor: "#3e3e42"
                                color: "#fff"
                                font.pixelSize: 9
                                onClicked: {
                                    if (data.hasIES)
                                        Modeler.lightSetIesProfile(data.objectId, "")
                                    else {
                                        lightPanel.iesObjectId = data.objectId
                                        iesDialog.open()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
