import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: dynPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property var bodyList: Modeler ? Modeler.dynBodies() : []
    property var objectList: Modeler ? Modeler.getMeshObjects() : []
    property bool isRunning: Modeler ? Modeler.dynRunning() : false

    Connections {
        target: Modeler
        function onDynChanged() {
            bodyList = Modeler ? Modeler.dynBodies() : []
            isRunning = Modeler ? Modeler.dynRunning() : false
            refreshObjects()
        }
        function onSceneChanged() { refreshObjects() }
    }

    function refreshObjects() {
        var all = Modeler ? Modeler.getMeshObjects() : []
        // exclude objects that already have a body
        var have = {}
        for (var i = 0; i < bodyList.length; ++i) have[bodyList[i].objectId] = true
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
                text: "DYNAMICS (rigid bodies)"
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

        // Add body
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
                    text: parent.currentText || "(choose object)"
                    color: "#aaa"; font.pixelSize: 10
                    verticalAlignment: Text.AlignVCenter; leftPadding: 6
                }
                indicator: Text {
                    text: "\u25BC"; color: "#666"; font.pixelSize: 8
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 6
                }
            }

            ComboBox {
                id: shapeCombo
                width: 100
                height: 24
                model: ["Box", "Sphere", "Convex Hull"]
                currentIndex: 0
                font.pixelSize: 10
                background: Rectangle { color: parent.hovered ? "#333" : "#252526"; radius: 3 }
                contentItem: Text {
                    text: parent.currentText; color: "#aaa"; font.pixelSize: 10
                    verticalAlignment: Text.AlignVCenter; leftPadding: 6
                }
                indicator: Text {
                    text: "\u25BC"; color: "#666"; font.pixelSize: 8
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 6
                }
            }

            Slider {
                id: massSlider
                width: 70
                height: 24
                from: 0.1; to: 10; stepSize: 0.1; value: 1
                background: Rectangle {
                    implicitHeight: 3
                    color: "#333"; radius: 1
                    Rectangle { width: parent.width * (parent.parent.value / parent.parent.to); height: parent.height; color: "#E10600"; radius: 1 }
                }
                handle: Rectangle {
                    implicitWidth: 6; implicitHeight: 10; radius: 1
                    color: "#ff6666"
                    x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                    y: (parent.height - height) / 2
                }
            }
            Text {
                text: massSlider.value.toFixed(1)
                color: "#888"; font.pixelSize: 9; width: 24
            }

            AppButton {
                text: "Add Body"
                height: 24
                bgcolor: "#E10600"; color: "#fff"
                font.pixelSize: 10
                enabled: objectCombo.currentIndex >= 0
                onClicked: {
                    if (!Modeler) return
                    var id = objectList[objectCombo.currentIndex].id
                    Modeler.dynAddBody(id, shapeCombo.currentIndex, massSlider.value, false)
                    refreshObjects()
                }
            }

            AppButton {
                text: "Static"
                height: 24
                bgcolor: "#3e3e42"; color: "#ccc"
                font.pixelSize: 10
                enabled: objectCombo.currentIndex >= 0
                onClicked: {
                    if (!Modeler) return
                    var id = objectList[objectCombo.currentIndex].id
                    Modeler.dynAddBody(id, shapeCombo.currentIndex, 0.0, false)
                    refreshObjects()
                }
            }
        }

        // Body list
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#151515"
            border.color: "#2a2a2a"
            border.width: 1
            radius: 3

            ListView {
                anchors.fill: parent
                clip: true
                spacing: 2
                model: bodyList

                delegate: Rectangle {
                    width: parent.width
                    height: 26
                    color: "#1c1c1c"
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
                            text: ["Box", "Sphere", "Convex Hull"][modelData.shapeType] || "?"
                            color: "#888"; font.pixelSize: 9
                        }

                        AppButton {
                            text: "Kinematic"
                            height: 20
                            bgcolor: "#3e3e42"; color: "#aaa"; font.pixelSize: 9
                            checkable: true
                            background: Rectangle {
                                color: parent.checked ? "#334466" : (parent.hovered ? "#333" : "#3e3e42")
                                radius: 2
                            }
                            contentItem: Text {
                                text: parent.text; color: parent.checked ? "#88aaff" : "#aaa"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: { if (Modeler) Modeler.dynSetBodyKinematic(modelData.objectId, checked) }
                        }

                        AppButton {
                            text: "\u2716"
                            width: 20; height: 20
                            font.pixelSize: 10
                            bgcolor: "transparent"; color: "#ff6666"
                            onClicked: {
                                if (Modeler) Modeler.dynRemoveBody(modelData.objectId)
                            }
                            ToolTip.visible: hovered; ToolTip.text: "Remove body"
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                text: bodyList.length === 0 ? "No rigid bodies. Pick an object and press Add Body." : ""
                color: "#444"
                font.pixelSize: 10
            }
        }

        // Simulation controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: isRunning ? "\u23F8 Pause" : "\u25B6 Play"
                height: 24
                bgcolor: isRunning ? "#E10600" : "#3e3e42"
                color: isRunning ? "#121212" : "#fff"
                font.pixelSize: 10
                enabled: bodyList.length > 0
                onClicked: {
                    if (!Modeler) return
                    if (Modeler.dynRunning()) Modeler.dynPause()
                    else Modeler.dynPlay()
                }
            }

            AppButton {
                text: "Step"
                height: 24
                bgcolor: "#3e3e42"; color: "#fff"
                font.pixelSize: 10
                enabled: bodyList.length > 0
                onClicked: { if (Modeler) Modeler.dynStepOnce(1.0 / 60.0) }
                ToolTip.visible: hovered; ToolTip.text: "Single step"
            }

            AppButton {
                text: "Reset"
                height: 24
                bgcolor: "transparent"; color: "#aaa"
                font.pixelSize: 10
                onClicked: { if (Modeler) Modeler.dynReset() }
                ToolTip.visible: hovered; ToolTip.text: "Reset bodies to object poses"
            }

            Item { width: 4 }

            Text {
                text: "Gravity Y:"
                color: "#777"; font.pixelSize: 9
            }
            Slider {
                id: gravSlider
                width: 100
                height: 22
                from: -20; to: 0; stepSize: 0.1
                value: -9.8
                Component.onCompleted: {
                    if (Modeler && Modeler.dynGravity) value = Modeler.dynGravity().y
                }
                background: Rectangle {
                    implicitHeight: 3
                    color: "#333"; radius: 1
                    Rectangle { width: parent.width * (parent.parent.value - parent.parent.from) / (parent.parent.to - parent.parent.from); height: parent.height; color: "#7777cc"; radius: 1 }
                }
                handle: Rectangle {
                    implicitWidth: 6; implicitHeight: 10; radius: 1
                    color: "#9999ee"
                    x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                    y: (parent.height - height) / 2
                }
                onMoved: { if (Modeler) Modeler.dynSetGravity(0, value, 0) }
            }
            Text {
                text: gravSlider.value.toFixed(1)
                color: "#888"; font.pixelSize: 9; width: 26
            }
        }

        Text {
            text: "Bodies: " + bodyList.length + (isRunning ? "  \u25B6 running" : "")
            color: "#888"; font.pixelSize: 9
        }
    }
}