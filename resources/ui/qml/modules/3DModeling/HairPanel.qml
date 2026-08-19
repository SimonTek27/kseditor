import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: hairPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property var hairListData: Modeler ? Modeler.hairList() : []
    property var objectList: Modeler ? Modeler.getMeshObjects() : []
    property bool isRunning: Modeler ? Modeler.hairRunning() : false
    property int selectedId: -1

    Connections {
        target: Modeler
        function onHairChanged() {
            hairListData = Modeler ? Modeler.hairList() : []
            isRunning = Modeler ? Modeler.hairRunning() : false
            refreshObjects()
        }
        function onSceneChanged() { refreshObjects() }
    }

    function refreshObjects() {
        var all = Modeler ? Modeler.getMeshObjects() : []
        var have = {}
        for (var i = 0; i < hairListData.length; ++i) have[hairListData[i].objectId] = true
        var filtered = []
        for (var j = 0; j < all.length; ++j)
            if (!have[all[j].id]) filtered.push(all[j])
        objectList = filtered
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "HAIR / FUR"
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

        // Add hair
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            ComboBox {
                id: objectCombo
                Layout.fillWidth: true
                height: 24
                model: objectList
                textRole: "name"
                font.pixelSize: 10
                background: Rectangle { color: parent.hovered ? "#333" : "#252526"; radius: 3 }
                contentItem: Text {
                    text: parent.currentText || "(choose surface)"
                    color: "#aaa"; font.pixelSize: 10
                    verticalAlignment: Text.AlignVCenter; leftPadding: 6
                }
                indicator: Text {
                    text: "\u25BC"; color: "#666"; font.pixelSize: 8
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 6
                }
            }

            AppButton {
                text: "Grow"
                height: 24
                bgcolor: "#E10600"; color: "#fff"
                font.pixelSize: 10
                enabled: objectCombo.currentIndex >= 0
                onClicked: {
                    if (!Modeler) return
                    var id = objectList[objectCombo.currentIndex].id
                    Modeler.hairAdd(id, strandSlider.value, 5, lengthSlider.value)
                    selectedId = id
                    refreshObjects()
                }
            }
        }

        // Hair list
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 110
            color: "#151515"
            border.color: "#2a2a2a"
            border.width: 1
            radius: 3

            ListView {
                anchors.fill: parent
                clip: true
                spacing: 2
                model: hairListData

                delegate: Rectangle {
                    width: parent.width
                    height: 24
                    color: modelData.objectId === selectedId ? "#2a2222" : "#1c1c1c"
                    border.color: "#2a2a2a"
                    border.width: 1
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 6

                        Text {
                            text: modelData.objectName
                            color: "#ddd"; font.pixelSize: 10; font.bold: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Text {
                            text: modelData.strands + " strands"
                            color: "#888"; font.pixelSize: 9
                        }

                        Text {
                            text: (modelData.length).toFixed(2) + "m"
                            color: "#666"; font.pixelSize: 9
                        }

                        AppButton {
                            text: "\u2716"
                            width: 20; height: 20
                            font.pixelSize: 10
                            bgcolor: "transparent"; color: "#ff6666"
                            onClicked: { if (Modeler) Modeler.hairRemove(modelData.objectId) }
                            ToolTip.visible: hovered; ToolTip.text: "Remove hair"
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            selectedId = modelData.objectId
                            if (Modeler) {
                                lengthSlider.value = Modeler.hairList()
                                    .filter(function(x){ return x.objectId === selectedId })[0].length
                            }
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                text: hairListData.length === 0 ? "No hair. Pick a mesh surface and press Grow." : ""
                color: "#444"
                font.pixelSize: 10
            }
        }

        // Params
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 6
            rowSpacing: 4
            enabled: selectedId >= 0

            Text { text: "Strands"; color: "#777"; font.pixelSize: 9 }
            Slider {
                id: strandSlider
                Layout.fillWidth: true
                from: 20; to: 2000; stepSize: 20; value: 300
                background: Rectangle { implicitHeight: 3; color: "#333"; radius: 1 }
                handle: Rectangle { implicitWidth: 6; implicitHeight: 10; radius: 1; color: "#E10600"; x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width); y: (parent.height - height) / 2 }
            }

            Text { text: "Length"; color: "#777"; font.pixelSize: 9 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Slider {
                    id: lengthSlider
                    Layout.fillWidth: true
                    from: 0.05; to: 2.0; stepSize: 0.05; value: 0.4
                    background: Rectangle { implicitHeight: 3; color: "#333"; radius: 1 }
                    handle: Rectangle { implicitWidth: 6; implicitHeight: 10; radius: 1; color: "#E10600"; x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width); y: (parent.height - height) / 2 }
                    onMoved: { if (Modeler && selectedId >= 0) Modeler.hairSetLength(selectedId, value) }
                }
                Text { text: lengthSlider.value.toFixed(2); color: "#888"; font.pixelSize: 9; width: 30 }
            }

            Text { text: "Stiffness"; color: "#777"; font.pixelSize: 9 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Slider {
                    id: stiffSlider
                    Layout.fillWidth: true
                    from: 0.0; to: 1.0; stepSize: 0.05; value: 0.7
                    background: Rectangle { implicitHeight: 3; color: "#333"; radius: 1 }
                    handle: Rectangle { implicitWidth: 6; implicitHeight: 10; radius: 1; color: "#E10600"; x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width); y: (parent.height - height) / 2 }
                    onMoved: { if (Modeler && selectedId >= 0) Modeler.hairSetStiffness(selectedId, value) }
                }
                Text { text: stiffSlider.value.toFixed(2); color: "#888"; font.pixelSize: 9; width: 30 }
            }

            Text { text: "Wind"; color: "#777"; font.pixelSize: 9 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Slider {
                    id: windSlider
                    Layout.fillWidth: true
                    from: 0.0; to: 1.0; stepSize: 0.05; value: 0.0
                    background: Rectangle { implicitHeight: 3; color: "#333"; radius: 1 }
                    handle: Rectangle { implicitWidth: 6; implicitHeight: 10; radius: 1; color: "#E10600"; x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width); y: (parent.height - height) / 2 }
                    onMoved: { if (Modeler && selectedId >= 0) Modeler.hairSetWind(selectedId, value) }
                }
                Text { text: windSlider.value.toFixed(2); color: "#888"; font.pixelSize: 9; width: 30 }
            }
        }

        // Transport
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: isRunning ? "\u23F8 Pause" : "\u25B6 Simulate"
                height: 24
                bgcolor: isRunning ? "#E10600" : "#3e3e42"
                color: isRunning ? "#121212" : "#fff"
                font.pixelSize: 10
                enabled: hairListData.length > 0
                onClicked: {
                    if (!Modeler) return
                    if (Modeler.hairRunning()) Modeler.hairPause()
                    else Modeler.hairPlay()
                }
            }

            AppButton {
                text: "Remove All"
                height: 24
                bgcolor: "transparent"; color: "#aaa"
                font.pixelSize: 10
                enabled: hairListData.length > 0
                onClicked: { if (Modeler) Modeler.hairRemoveAll() }
            }
        }

        Text {
            text: "Hair objects: " + hairListData.length + (isRunning ? "  \u25B6 simulating" : "") + "\nStrands are grown from the mesh surface by area, simulated as\npinned Verlet chains and rendered as thin ribbon cards."
            color: "#777"
            font.pixelSize: 9
        }
    }
}