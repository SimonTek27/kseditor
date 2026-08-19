import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: silhouettePanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property var info: Modeler.silhouetteInfo ? Modeler.silhouetteInfo() : ({})

    function refresh() {
        info = Modeler.silhouetteInfo ? Modeler.silhouetteInfo() : {}
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
            Text { text: "SILHOUETTE"; color: "#E10600"; font.pixelSize: 13; font.bold: true; Layout.fillWidth: true }
            AppButton { text: "X"; height: 24; width: 26; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10; font.bold: true; onClicked: closePanel() }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: info.enabled ? "Disable Silhouette" : "Enable Silhouette"
            bgcolor: info.enabled ? "#2a6e2a" : "#E10600"
            color: "#ffffff"
            font.bold: true
            font.pixelSize: 11
            onClicked: {
                if (Modeler.silhouetteSetEnabled)
                    Modeler.silhouetteSetEnabled(!info.enabled)
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
                    if (Modeler.silhouetteSetColor)
                        Modeler.silhouetteSetColor(0, 0, 0, 0.8)
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
                    if (Modeler.silhouetteSetColor)
                        Modeler.silhouetteSetColor(1, 1, 1, 0.8)
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
                    if (Modeler.silhouetteSetColor)
                        Modeler.silhouetteSetColor(0.88, 0.02, 0, 0.8)
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "EDGE ANGLE THRESHOLD: " + (info.threshold || 60).toFixed(0) + "°"; color: "#aaa"; font.pixelSize: 9 }

        Slider {
            id: thresholdSlider
            Layout.fillWidth: true
            from: 10.0; to: 120.0; stepSize: 1.0
            value: info.threshold || 60
            onMoved: {
                if (Modeler.silhouetteSetThreshold)
                    Modeler.silhouetteSetThreshold(value)
            }
            background: Rectangle {
                x: thresholdSlider.leftPadding
                y: thresholdSlider.topPadding + thresholdSlider.availableHeight / 2 - 2
                width: thresholdSlider.availableWidth
                height: 4
                radius: 2
                color: "#3e3e42"
                Rectangle {
                    width: thresholdSlider.visualPosition * parent.width
                    height: 4
                    radius: 2
                    color: "#E10600"
                }
            }
            handle: Rectangle {
                x: thresholdSlider.leftPadding + thresholdSlider.visualPosition * (thresholdSlider.availableWidth - width)
                y: thresholdSlider.topPadding + thresholdSlider.availableHeight / 2 - height / 2
                width: 14; height: 14; radius: 7; color: "#ffffff"
            }
        }

        Text {
            text: "Edges with dihedral angle above the threshold are highlighted.\nLower = more edges shown, Higher = only hard edges."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }

        Text { text: "Tip: Silhouette mode highlights edges where adjacent faces meet at a sharp angle. Great for evaluating the shape and form of your model."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
}
