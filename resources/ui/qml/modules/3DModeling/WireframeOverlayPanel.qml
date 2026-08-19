import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: wireOverlayPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property var info: Modeler.wireframeOverlayInfo ? Modeler.wireframeOverlayInfo() : ({})

    function refresh() {
        info = Modeler.wireframeOverlayInfo ? Modeler.wireframeOverlayInfo() : {}
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
            Text { text: "WIREFRAME OVERLAY"; color: "#E10600"; font.pixelSize: 13; font.bold: true; Layout.fillWidth: true }
            AppButton { text: "X"; height: 24; width: 26; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10; font.bold: true; onClicked: closePanel() }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: info.enabled ? "Disable Overlay" : "Enable Overlay"
            bgcolor: info.enabled ? "#2a6e2a" : "#E10600"
            color: "#ffffff"
            font.bold: true
            font.pixelSize: 11
            onClicked: {
                if (Modeler.wireframeOverlaySetEnabled)
                    Modeler.wireframeOverlaySetEnabled(!info.enabled)
                refresh()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "COLOR"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            AppButton {
                text: "Black"
                height: 24
                bgcolor: "#2a2a2e"
                color: "#fff"
                border.color: "#666"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    if (Modeler.wireframeOverlaySetColor)
                        Modeler.wireframeOverlaySetColor(0, 0, 0, 0.5)
                }
            }
            AppButton {
                text: "White"
                height: 24
                bgcolor: "#ffffff"
                color: "#000"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    if (Modeler.wireframeOverlaySetColor)
                        Modeler.wireframeOverlaySetColor(1, 1, 1, 0.5)
                }
            }
            AppButton {
                text: "Red"
                height: 24
                bgcolor: "#E10600"
                color: "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    if (Modeler.wireframeOverlaySetColor)
                        Modeler.wireframeOverlaySetColor(0.88, 0.02, 0, 0.5)
                }
            }
            AppButton {
                text: "Green"
                height: 24
                bgcolor: "#2a6e2a"
                color: "#fff"
                font.pixelSize: 9
                Layout.fillWidth: true
                onClicked: {
                    if (Modeler.wireframeOverlaySetColor)
                        Modeler.wireframeOverlaySetColor(0.16, 0.43, 0.16, 0.5)
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "LINE THICKNESS: " + (info.thickness || 1.0).toFixed(1) + "px"; color: "#aaa"; font.pixelSize: 9 }

        Slider {
            id: thicknessSlider
            Layout.fillWidth: true
            from: 0.5; to: 4.0; stepSize: 0.1
            value: info.thickness || 1.0
            onMoved: {
                if (Modeler.wireframeOverlaySetThickness)
                    Modeler.wireframeOverlaySetThickness(value)
            }
            background: Rectangle {
                x: thicknessSlider.leftPadding
                y: thicknessSlider.topPadding + thicknessSlider.availableHeight / 2 - 2
                width: thicknessSlider.availableWidth
                height: 4
                radius: 2
                color: "#3e3e42"
                Rectangle {
                    width: thicknessSlider.visualPosition * parent.width
                    height: 4
                    radius: 2
                    color: "#E10600"
                }
            }
            handle: Rectangle {
                x: thicknessSlider.leftPadding + thicknessSlider.visualPosition * (thicknessSlider.availableWidth - width)
                y: thicknessSlider.topPadding + thicknessSlider.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7; color: "#ffffff"
            }
        }

        Item { Layout.fillHeight: true }

        Text { text: "Tip: Wireframe overlay draws edges on top of the shaded model. Useful for topology inspection while sculpting or modeling."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
}
