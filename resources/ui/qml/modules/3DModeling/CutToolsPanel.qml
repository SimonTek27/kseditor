import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: cutPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int objectId: Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property int loopAxis: 2
    property real loopFactor: 0.5
    property real loopSlide: 0.0
    property bool multiCutActive: false

    function setMultiCut(on) {
        multiCutActive = on
        var root = cutPanel.parent
        while (root && root.objectName !== "pageModelerRoot") root = root.parent
        if (root) {
            if (root.cutMode !== undefined) root.cutMode = on
            if (root.cutStart !== undefined) root.cutStart = null
        }
    }

    function applyLoopCut() {
        if (objectId >= 0)
            Modeler.loopCut(objectId, loopAxis, loopFactor, loopSlide)
    }

    function applyKnife() {
        if (objectId >= 0)
            Modeler.knifeCutSelected(objectId)
    }

    Connections {
        target: Modeler
        function onSelectionChanged() {
            objectId = Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "CUT TOOLS"
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
            text: objectId >= 0 ? "Target: " + Modeler.selectedObject.name
                                : "No object selected"
            color: objectId >= 0 ? "#aaa" : "#E10600"
            font.pixelSize: 10
            font.bold: objectId >= 0
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "LOOP CUT (axis plane, splits edge loop)"; color: "#888"; font.pixelSize: 10; font.bold: true }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: 4
            rowSpacing: 4

            AppButton { text: "X"; height: 26; checkable: true; autoExclusive: true; bgcolor: loopAxis === 0 ? "#E10600" : "#3e3e42"; color: loopAxis === 0 ? "#121212" : "#fff"; font.pixelSize: 10; checked: loopAxis === 0; onClicked: loopAxis = 0 }
            AppButton { text: "Y"; height: 26; checkable: true; autoExclusive: true; bgcolor: loopAxis === 1 ? "#E10600" : "#3e3e42"; color: loopAxis === 1 ? "#121212" : "#fff"; font.pixelSize: 10; checked: loopAxis === 1; onClicked: loopAxis = 1 }
            AppButton { text: "Z"; height: 26; checkable: true; autoExclusive: true; bgcolor: loopAxis === 2 ? "#E10600" : "#3e3e42"; color: loopAxis === 2 ? "#121212" : "#fff"; font.pixelSize: 10; checked: loopAxis === 2; onClicked: loopAxis = 2 }
        }

        Text { text: "Factor: " + loopFactor.toFixed(2); color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: factorSlider
            Layout.fillWidth: true
            from: 0.0; to: 1.0; stepSize: 0.01
            value: loopFactor
            onMoved: loopFactor = value
            background: Rectangle { x: factorSlider.leftPadding; y: factorSlider.topPadding + factorSlider.availableHeight / 2 - 2; width: factorSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: factorSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#E10600" }
            }
            handle: Rectangle { x: factorSlider.leftPadding + factorSlider.visualPosition * (factorSlider.availableWidth - width); y: factorSlider.topPadding + factorSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        Text { text: "Slide (loop-cut and slide): " + loopSlide.toFixed(2); color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: slideSlider
            Layout.fillWidth: true
            from: -1.0; to: 1.0; stepSize: 0.01
            value: loopSlide
            onMoved: loopSlide = value
            background: Rectangle { x: slideSlider.leftPadding; y: slideSlider.topPadding + slideSlider.availableHeight / 2 - 2; width: slideSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: slideSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#ff6600" }
            }
            handle: Rectangle { x: slideSlider.leftPadding + slideSlider.visualPosition * (slideSlider.availableWidth - width); y: slideSlider.topPadding + slideSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "Apply Loop Cut"
            bgcolor: "#E10600"
            color: "#121212"
            font.bold: true
            font.pixelSize: 11
            enabled: objectId >= 0
            onClicked: applyLoopCut()
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "KNIFE"; color: "#888"; font.pixelSize: 10; font.bold: true }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "Knife Cut (bounding box)"
            bgcolor: "#3e3e42"
            color: "#fff"
            font.pixelSize: 10
            enabled: objectId >= 0
            onClicked: applyKnife()
        }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "Knife Cut (bounding box)"
            bgcolor: "#3e3e42"
            color: "#fff"
            font.pixelSize: 10
            enabled: objectId >= 0
            onClicked: applyKnife()
        }

        AppButton {
            Layout.fillWidth: true
            height: 30
            checkable: true
            text: multiCutActive ? "Multi-Cut: click start, click end (ON)" : "Multi-Cut (2 clicks, ON/OFF)"
            bgcolor: multiCutActive ? "#E10600" : "#3e3e42"
            color: multiCutActive ? "#121212" : "#fff"
            font.pixelSize: 10
            font.bold: multiCutActive
            checked: multiCutActive
            onClicked: setMultiCut(!multiCutActive)
        }

        Item { Layout.fillHeight: true }

        Text {
            text: multiCutActive
                ? "Click once on the mesh to place the cut start (red dot), then click again for the end."
                : "Note: edge/vertex slide is provided by the loop-cut Slide slider."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
