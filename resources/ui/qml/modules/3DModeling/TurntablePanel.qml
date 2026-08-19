import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: turntablePanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property var info: Modeler.turntableInfo ? Modeler.turntableInfo() : ({})

    function refresh() {
        info = Modeler.turntableInfo ? Modeler.turntableInfo() : {}
    }

    Connections {
        target: Modeler
        function onSceneChanged() { refresh() }
    }

    Component.onCompleted: refresh()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text { text: "TURNTABLE"; color: "#E10600"; font.pixelSize: 13; font.bold: true; Layout.fillWidth: true }
            AppButton { text: "X"; height: 24; width: 26; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10; font.bold: true; onClicked: closePanel() }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: info.enabled ? "Stop Turntable" : "Start Turntable"
            bgcolor: info.enabled ? "#E10600" : "#2a6e2a"
            color: "#ffffff"
            font.bold: true
            font.pixelSize: 11
            onClicked: {
                if (Modeler.turntableSetEnabled)
                    Modeler.turntableSetEnabled(!info.enabled)
                refresh()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "SPEED: " + (info.speed || 45).toFixed(0) + "°/s"; color: "#aaa"; font.pixelSize: 9 }

        Slider {
            id: speedSlider
            Layout.fillWidth: true
            from: 5.0; to: 180.0; stepSize: 1.0
            value: info.speed || 45
            onMoved: {
                if (Modeler.turntableSetSpeed)
                    Modeler.turntableSetSpeed(value)
            }
            background: Rectangle {
                x: speedSlider.leftPadding
                y: speedSlider.topPadding + speedSlider.availableHeight / 2 - 2
                width: speedSlider.availableWidth
                height: 4
                radius: 2
                color: "#3e3e42"
                Rectangle {
                    width: speedSlider.visualPosition * parent.width
                    height: 4
                    radius: 2
                    color: "#E10600"
                }
            }
            handle: Rectangle {
                x: speedSlider.leftPadding + speedSlider.visualPosition * (speedSlider.availableWidth - width)
                y: speedSlider.topPadding + speedSlider.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7; color: "#ffffff"
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "AXIS"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton {
                text: "Y (Up)"
                height: 24
                bgcolor: info.axisY > 0.5 ? "#E10600" : "#3e3e42"
                color: info.axisY > 0.5 ? "#121212" : "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    if (Modeler.turntableSetAxis)
                        Modeler.turntableSetAxis(0, 1, 0)
                    refresh()
                }
            }
            AppButton {
                text: "X"
                height: 24
                bgcolor: info.axisX > 0.5 ? "#E10600" : "#3e3e42"
                color: info.axisX > 0.5 ? "#121212" : "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    if (Modeler.turntableSetAxis)
                        Modeler.turntableSetAxis(1, 0, 0)
                    refresh()
                }
            }
            AppButton {
                text: "Z"
                height: 24
                bgcolor: info.axisZ > 0.5 ? "#E10600" : "#3e3e42"
                color: info.axisZ > 0.5 ? "#121212" : "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    if (Modeler.turntableSetAxis)
                        Modeler.turntableSetAxis(0, 0, 1)
                    refresh()
                }
            }
        }

        Text {
            text: "Angle: " + (info.angle || 0).toFixed(1) + "°"
            color: "#888"
            font.pixelSize: 9
        }

        Item { Layout.fillHeight: true }

        Text { text: "Tip: Turntable auto-rotates the camera around the model. Use for presentation or 360° preview. Combine with HDRI for environment rendering."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
}
