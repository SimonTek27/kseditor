import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: curvePanel
    width: 360
    height: 560
    color: "#1e1e1e"
    border.color: "#E10600"
    border.width: 1
    radius: 6

    signal closePanel()

    property int curveId: Modeler.selectedObject ? Modeler.selectedObject.id : -1
    property var curveData: ({})
    property int selectedCV: -1
    property int lastCurveId: -1

    function isCurveSelected() {
        return curveId >= 0 && Modeler.getCurve(curveId).points !== undefined
    }

    function refresh() {
        var current = isCurveSelected()
        curveData = current ? Modeler.getCurve(curveId) : {}
        if (!current) {
            selectedCV = -1
            if (Modeler.curveSelectedCV !== undefined) Modeler.curveSelectedCV = -1
            lastCurveId = -1
        } else if (curveId !== lastCurveId) {
            selectedCV = -1
            if (Modeler.curveSelectedCV !== undefined) Modeler.curveSelectedCV = -1
            lastCurveId = curveId
        }
    }

    onVisibleChanged: {
        if (visible) refresh()
    }

    Connections {
        target: Modeler
        function onCurveChanged() { refresh() }
        function onSelectionChanged() { refresh() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "CURVE EDITOR"
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
            text: "Primitives:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: 4
            rowSpacing: 4

            AppButton { text: "Line"; height: 26; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: Modeler.addCurve("Line", [[-1,0,0],[1,0,0]]) }
            AppButton { text: "Bezier"; height: 26; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: Modeler.addCurve("Bezier", [[-1,0,0],[-0.3,1,0],[0.3,-1,0],[1,0,0]]) }
            AppButton { text: "BSpline"; height: 26; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: Modeler.addCurve("BSpline", [[-1,0,0],[-0.6,0.6,0],[-0.2,-0.4,0],[0.4,0.7,0],[1,0,0]]) }
            AppButton { text: "Arc"; height: 26; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: Modeler.addCurve("Arc", [[-1,0,0],[0,1,0],[1,0,0]]) }
            AppButton { text: "Circle"; height: 26; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: Modeler.addCurve("Circle", [[0,0,0],[1,0,0]]) }
        }

        Item { height: 2 }

        Text {
            text: isCurveSelected() ? "Curve: " + (curveData.type || "") + "  (" + (curveData.points ? curveData.points.length : 0) + " CVs)"
                                   : "No curve selected (create one above)"
            color: "#aaa"
            font.pixelSize: 11
            font.bold: isCurveSelected()
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(200, curvePanel.height * 0.4)
            color: "#252526"
            border.color: "#333"
            border.width: 1
            radius: 3

            ListView {
                id: cvList
                anchors.fill: parent
                anchors.margins: 2
                clip: true
                spacing: 2
                model: isCurveSelected() ? curveData.points : []

                delegate: Rectangle {
                    id: cvRow
                    width: cvList.width
                    height: 26
                    radius: 3
                    color: selectedCV === index ? "#3a3a3e" : "#2c2c2e"
                    border.color: selectedCV === index ? "#E10600" : "#333"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 4
                        spacing: 6

                        Text {
                            text: "CV" + index
                            color: "#E10600"
                            font.pixelSize: 10
                            font.bold: true
                            width: 30
                        }

                        Text {
                            text: "(" + (modelData[0] !== undefined ? modelData[0].toFixed(2) : 0) + ", "
                                       + (modelData[1] !== undefined ? modelData[1].toFixed(2) : 0) + ", "
                                       + (modelData[2] !== undefined ? modelData[2].toFixed(2) : 0) + ")"
                            color: "#ccc"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        AppButton {
                            text: "\u2715"
                            height: 20
                            width: 22
                            bgcolor: "#E10600"
                            color: "#fff"
                            font.pixelSize: 9
                            onClicked: { Modeler.curveRemoveCV(curveId, index); selectedCV = -1 }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            selectedCV = index
                            if (Modeler.curveSelectedCV !== undefined) Modeler.curveSelectedCV = index
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            CheckBox {
                id: cvEditChk
                text: "Edit CVs in viewport (W = Move gizmo)"
                font.pixelSize: 10
                Layout.fillWidth: true
                checked: Modeler.curveCvEdit !== undefined ? Modeler.curveCvEdit : false
                onToggled: { if (Modeler.setCurveCvEdit) Modeler.setCurveCvEdit(checked) }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: "Add CV"
                height: 26
                bgcolor: "#3e3e42"
                color: "#fff"
                Layout.fillWidth: true
                font.pixelSize: 10
                enabled: isCurveSelected()
                onClicked: {
                    var pts = curveData.points
                    var last = pts.length > 0 ? pts[pts.length - 1] : [0,0,0]
                    Modeler.curveAddCV(curveId, [last[0] + 0.5, last[1], last[2]])
                }
            }
        }

        Text {
            text: "Edit Selected CV:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
            visible: selectedCV >= 0
        }

        GridLayout {
            visible: selectedCV >= 0
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 6
            rowSpacing: 4

            Text { text: "X"; color: "#E10600"; font.pixelSize: 10; font.bold: true }
            TextField {
                id: cvX
                Layout.fillWidth: true
                height: 24
                color: "#fff"
                font.pixelSize: 10
                text: selectedCV >= 0 ? (curveData.points[selectedCV][0] || 0) : 0
                onEditingFinished: applyCV()
            }
            Text { text: "Y"; color: "#E10600"; font.pixelSize: 10; font.bold: true }
            TextField {
                id: cvY
                Layout.fillWidth: true
                height: 24
                color: "#fff"
                font.pixelSize: 10
                text: selectedCV >= 0 ? (curveData.points[selectedCV][1] || 0) : 0
                onEditingFinished: applyCV()
            }
            Text { text: "Z"; color: "#E10600"; font.pixelSize: 10; font.bold: true }
            TextField {
                id: cvZ
                Layout.fillWidth: true
                height: 24
                color: "#fff"
                font.pixelSize: 10
                text: selectedCV >= 0 ? (curveData.points[selectedCV][2] || 0) : 0
                onEditingFinished: applyCV()
            }
        }

        function applyCV() {
            if (selectedCV >= 0 && isCurveSelected()) {
                Modeler.curveUpdateCV(curveId, selectedCV, [parseFloat(cvX.text) || 0, parseFloat(cvY.text) || 0, parseFloat(cvZ.text) || 0])
            }
        }

        Item { height: 4 }

        Text {
            text: "Continuity:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: 4
            rowSpacing: 4

            AppButton {
                text: "C0"
                height: 26
                bgcolor: Modeler.curveContinuityOf(curveId) === 0 ? "#E10600" : "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                Layout.fillWidth: true
                enabled: isCurveSelected()
                onClicked: Modeler.curveSetContinuity(curveId, 0)
            }
            AppButton {
                text: "C1"
                height: 26
                bgcolor: Modeler.curveContinuityOf(curveId) === 1 ? "#E10600" : "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                Layout.fillWidth: true
                enabled: isCurveSelected()
                onClicked: Modeler.curveSetContinuity(curveId, 1)
            }
            AppButton {
                text: "C2"
                height: 26
                bgcolor: Modeler.curveContinuityOf(curveId) === 2 ? "#E10600" : "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                Layout.fillWidth: true
                enabled: isCurveSelected()
                onClicked: Modeler.curveSetContinuity(curveId, 2)
            }
        }

        Text {
            text: "C1 = shared tangents, C2 = curvature continuity."
            color: "#777"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { height: 4 }

        Text {
            text: "Surfaces:"
            color: "#888"
            font.pixelSize: 10
            font.bold: true
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 4
            rowSpacing: 4

            AppButton {
                text: "To Mesh"
                height: 28
                bgcolor: "#E10600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 10
                Layout.fillWidth: true
                enabled: isCurveSelected()
                onClicked: Modeler.curveToMesh(curveId, 0.02, 32)
            }
            AppButton {
                text: "Revolve Y"
                height: 28
                bgcolor: "#ff6600"
                color: "#121212"
                font.bold: true
                font.pixelSize: 10
                Layout.fillWidth: true
                enabled: isCurveSelected()
                onClicked: Modeler.curveRevolve(curveId, 360, 32)
            }
            AppButton {
                text: "Loft"
                height: 28
                bgcolor: "#3e3e42"
                color: "#fff"
                font.bold: true
                font.pixelSize: 10
                Layout.fillWidth: true
                enabled: isCurveSelected()
                onClicked: Modeler.curveLoft([curveId], 32)
            }
            AppButton {
                text: "Delete Curve"
                height: 28
                bgcolor: "#3e3e42"
                color: "#fff"
                font.bold: true
                font.pixelSize: 10
                Layout.fillWidth: true
                enabled: isCurveSelected()
                onClicked: { Modeler.curveDelete(curveId); closePanel() }
            }
        }

        Item { height: 2 }

        Text {
            text: "Loft needs 2+ curves; Sweep/Rail select profile+path."
            color: "#777"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
