import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0
import ksEditor.Symmetry 1.0

Rectangle {
    id: symPanel
    visible: false
    width: 260
    height: 420
    color: "#1e1e1e"
    border.color: "#E10600"
    border.width: 1
    radius: 6

    signal closePanel()

    property string activeAxis: "X"

    onVisibleChanged: {
        if (visible) Symmetry.updateSelection()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Text {
            text: "SYMMETRY EDITOR"
            color: "#E10600"
            font.pixelSize: 13
            font.bold: true
        }

        Item { height: 2 }

        Text {
            text: "Axis:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        RowLayout {
            spacing: 4
            Layout.fillWidth: true

            KsButton {
                text: "X"
                checkable: true
                autoExclusive: true
                Layout.fillWidth: true
                height: 28
                font.pixelSize: 12
                font.bold: true
                bgcolor: checked ? "#E10600" : "#3e3e42"
                color: checked ? "#121212" : "#ffffff"
                checked: Symmetry.axis === 0
                onClicked: { Symmetry.axis = 0; activeAxis = "X" }
            }
            KsButton {
                text: "Y"
                checkable: true
                autoExclusive: true
                Layout.fillWidth: true
                height: 28
                font.pixelSize: 12
                font.bold: true
                bgcolor: checked ? "#E10600" : "#3e3e42"
                color: checked ? "#121212" : "#ffffff"
                checked: Symmetry.axis === 1
                onClicked: { Symmetry.axis = 1; activeAxis = "Y" }
            }
            KsButton {
                text: "Z"
                checkable: true
                autoExclusive: true
                Layout.fillWidth: true
                height: 28
                font.pixelSize: 12
                font.bold: true
                bgcolor: checked ? "#E10600" : "#3e3e42"
                color: checked ? "#121212" : "#ffffff"
                checked: Symmetry.axis === 2
                onClicked: { Symmetry.axis = 2; activeAxis = "Z" }
            }
        }

        Item { height: 2 }

        Text {
            text: "Plane Offset:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        RowLayout {
            spacing: 6
            Layout.fillWidth: true

            Slider {
                id: offsetSlider
                from: -5
                to: 5
                value: Symmetry.offset
                stepSize: 0.01
                Layout.fillWidth: true
                onMoved: Symmetry.offset = value
            }
            Text {
                text: offsetSlider.value.toFixed(2)
                color: "#E10600"
                font.pixelSize: 11
                font.bold: true
                implicitWidth: 40
                horizontalAlignment: Text.AlignRight
            }
        }

        Item { height: 2 }

        Text {
            text: "Weld Threshold:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        RowLayout {
            spacing: 6
            Layout.fillWidth: true

            Slider {
                id: weldSlider
                from: 0
                to: 0.1
                value: Symmetry.weldThreshold
                stepSize: 0.0001
                Layout.fillWidth: true
                onMoved: Symmetry.weldThreshold = value
            }
            Text {
                text: weldSlider.value.toFixed(4)
                color: "#E10600"
                font.pixelSize: 11
                font.bold: true
                implicitWidth: 50
                horizontalAlignment: Text.AlignRight
            }
        }

        Item { height: 2 }

        Text {
            text: "Clip Mode:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        RowLayout {
            spacing: 4
            Layout.fillWidth: true

            KsButton {
                text: "None"
                checkable: true
                autoExclusive: true
                Layout.fillWidth: true
                height: 26
                font.pixelSize: 10
                bgcolor: checked ? "#E10600" : "#3e3e42"
                color: checked ? "#121212" : "#ffffff"
                checked: Symmetry.clipMode === 0
                onClicked: Symmetry.clipMode = 0
            }
            KsButton {
                text: "+ Side"
                checkable: true
                autoExclusive: true
                Layout.fillWidth: true
                height: 26
                font.pixelSize: 10
                bgcolor: checked ? "#E10600" : "#3e3e42"
                color: checked ? "#121212" : "#ffffff"
                checked: Symmetry.clipMode === 1
                onClicked: Symmetry.clipMode = 1
            }
            KsButton {
                text: "- Side"
                checkable: true
                autoExclusive: true
                Layout.fillWidth: true
                height: 26
                font.pixelSize: 10
                bgcolor: checked ? "#E10600" : "#3e3e42"
                color: checked ? "#121212" : "#ffffff"
                checked: Symmetry.clipMode === 2
                onClicked: Symmetry.clipMode = 2
            }
        }

        Item { height: 2 }

        Text {
            text: "Merge Mode:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        RowLayout {
            spacing: 4
            Layout.fillWidth: true

            KsButton {
                text: "Append"
                checkable: true
                autoExclusive: true
                Layout.fillWidth: true
                height: 26
                font.pixelSize: 10
                bgcolor: checked ? "#E10600" : "#3e3e42"
                color: checked ? "#121212" : "#ffffff"
                checked: Symmetry.mergeMode === 0
                onClicked: Symmetry.mergeMode = 0
            }
            KsButton {
                text: "Replace"
                checkable: true
                autoExclusive: true
                Layout.fillWidth: true
                height: 26
                font.pixelSize: 10
                bgcolor: checked ? "#E10600" : "#3e3e42"
                color: checked ? "#121212" : "#ffffff"
                checked: Symmetry.mergeMode === 1
                onClicked: Symmetry.mergeMode = 1
            }
            KsButton {
                text: "New Obj"
                checkable: true
                autoExclusive: true
                Layout.fillWidth: true
                height: 26
                font.pixelSize: 10
                bgcolor: checked ? "#E10600" : "#3e3e42"
                color: checked ? "#121212" : "#ffffff"
                checked: Symmetry.mergeMode === 2
                onClicked: Symmetry.mergeMode = 2
            }
        }

        Item { height: 6 }

        RowLayout {
            spacing: 8
            Layout.fillWidth: true

            KsButton {
                height: 32
                text: "Preview"
                bgcolor: Symmetry.previewVisible ? "#ff6600" : "#3e3e42"
                color: "#ffffff"
                Layout.fillWidth: true
                font.bold: true
                enabled: Symmetry.hasSelection
                onClicked: {
                    if (Symmetry.previewVisible)
                        Symmetry.clearPreview()
                    else
                        Symmetry.previewSymmetry()
                }
            }

            KsButton {
                height: 32
                text: "Apply"
                bgcolor: "#E10600"
                color: "#121212"
                Layout.fillWidth: true
                font.bold: true
                enabled: Symmetry.hasSelection
                onClicked: {
                    Symmetry.clearPreview()
                    Symmetry.applySymmetry()
                    closePanel()
                }
            }
        }

        Item { height: 2 }

        Text {
            text: Symmetry.statusText
            color: "#888"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    Connections {
        target: Symmetry
        function onSymmetryApplied(success, info) {
            if (!success) console.warn("Symmetry error:", info)
        }
    }
}


