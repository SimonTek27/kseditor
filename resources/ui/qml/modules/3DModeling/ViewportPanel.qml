import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: vpPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    Connections {
        target: Modeler
        function onCameraChanged() { fovLabel.text = "Vertical FOV: " + (Modeler.cameraFov ? Modeler.cameraFov.toFixed(1) : "60.0") + "\u00B0" }
    }

    Component.onCompleted: {
        cullCheck.checked = Modeler.cullingEnabled !== undefined ? Modeler.cullingEnabled : false
        distSlider.value = Modeler.cullDistance !== undefined ? Modeler.cullDistance : 200
        focalSlider.value = Modeler.cameraFocalLength !== undefined ? Modeler.cameraFocalLength : 35
        sensorSlider.value = Modeler.cameraSensorWidth !== undefined ? Modeler.cameraSensorWidth : 36
        fovLabel.text = "Vertical FOV: " + (Modeler.cameraFov ? Modeler.cameraFov.toFixed(1) : "60.0") + "\u00B0"
        toneCombo.currentIndex = Modeler.tonemappingMode !== undefined ? Modeler.tonemappingMode : 0
        expSlider.value = Modeler.tonemapExposure !== undefined ? Modeler.tonemapExposure : 1.0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "VIEWPORT"
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

        Text { text: "CULLING"; color: "#888"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            CheckBox {
                id: cullCheck
                text: "Distance culling"
                font.pixelSize: 10
                onCheckedChanged: {
                    if (Modeler && Modeler.setCullingEnabled) Modeler.setCullingEnabled(checked)
                }
            }
            Text {
                id: cullStatus
                color: "#aaa"
                font.pixelSize: 10
                text: "OFF"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { text: "Distance"; color: "#888"; font.pixelSize: 10; Layout.preferredWidth: 60 }
            Slider {
                id: distSlider
                Layout.fillWidth: true
                from: 1
                to: 1000
                stepSize: 1
                onValueChanged: {
                    if (Modeler && Modeler.setCullDistance) Modeler.setCullDistance(value)
                }
            }
            Text {
                text: Math.round(distSlider.value) + " u"
                color: "#aaa"
                font.pixelSize: 10
                Layout.preferredWidth: 50
                horizontalAlignment: Text.AlignRight
            }
        }

        Text { text: "Hides objects farther than the distance from each viewport camera. "
                     + "Quick3D frustum culling already removes off-screen objects."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "CAMERA MATCHING"; color: "#888"; font.pixelSize: 10; font.bold: true }
        Text { text: "Simulates a real lens on the perspective viewport (35mm = full frame)."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { text: "Focal"; color: "#888"; font.pixelSize: 10; Layout.preferredWidth: 60 }
            Slider {
                id: focalSlider
                Layout.fillWidth: true
                from: 5
                to: 200
                stepSize: 1
                onValueChanged: {
                    if (Modeler && Modeler.setCameraFocalLength) Modeler.setCameraFocalLength(value)
                }
            }
            Text {
                text: Math.round(focalSlider.value) + " mm"
                color: "#aaa"
                font.pixelSize: 10
                Layout.preferredWidth: 50
                horizontalAlignment: Text.AlignRight
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { text: "Sensor"; color: "#888"; font.pixelSize: 10; Layout.preferredWidth: 60 }
            Slider {
                id: sensorSlider
                Layout.fillWidth: true
                from: 4
                to: 72
                stepSize: 1
                onValueChanged: {
                    if (Modeler && Modeler.setCameraSensorWidth) Modeler.setCameraSensorWidth(value)
                }
            }
            Text {
                text: Math.round(sensorSlider.value) + " mm"
                color: "#aaa"
                font.pixelSize: 10
                Layout.preferredWidth: 50
                horizontalAlignment: Text.AlignRight
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                id: fovLabel
                color: "#aaa"
                font.pixelSize: 10
                font.bold: true
                Layout.fillWidth: true
            }
            AppButton {
                text: "Reset Lens"
                height: 28
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                onClicked: {
                    focalSlider.value = 35
                    sensorSlider.value = 36
                }
            }
        }

        AppButton {
            Layout.fillWidth: true
            height: 32
            text: "Match Viewport to Selected Camera"
            bgcolor: "#E10600"
            color: "#121212"
            font.bold: true
            font.pixelSize: 10
            onClicked: { if (Modeler) Modeler.matchCameraToSelection() }
        }

        Text {
            text: "Select a Camera object in the outliner, then press Match to aim the "
                 + "orbit viewport at it."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "RENDERING"; color: "#888"; font.pixelSize: 10; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { text: "Tonemap"; color: "#888"; font.pixelSize: 10; Layout.preferredWidth: 60 }
            ComboBox {
                id: toneCombo
                Layout.fillWidth: true
                model: ["None", "Linear", "ACES", "HejlDawson", "Filmic"]
                font.pixelSize: 10
                onActivated: {
                    if (Modeler && Modeler.setTonemappingMode) Modeler.setTonemappingMode(index)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { text: "Exposure"; color: "#888"; font.pixelSize: 10; Layout.preferredWidth: 60 }
            Slider {
                id: expSlider
                Layout.fillWidth: true
                from: 0.1
                to: 4
                stepSize: 0.05
                onValueChanged: {
                    if (Modeler && Modeler.setTonemapExposure) Modeler.setTonemapExposure(value)
                }
            }
            Text {
                text: expSlider.value.toFixed(2)
                color: "#aaa"
                font.pixelSize: 10
                Layout.preferredWidth: 50
                horizontalAlignment: Text.AlignRight
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { text: "SUBDIV CAGE"; color: "#E10600"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }

            CheckBox {
                id: cageCheck
                text: "Smooth on"
                checked: Modeler && Modeler.subdivCageEnabled !== undefined ? Modeler.subdivCageEnabled : false
                font.pixelSize: 10
                onToggled: {
                    if (Modeler && Modeler.setSubdivCageEnabled) Modeler.setSubdivCageEnabled(checked)
                }
                indicator: Rectangle {
                    implicitWidth: 13; implicitHeight: 13; radius: 2
                    border.color: cageCheck.checked ? "#E10600" : "#555"
                    color: cageCheck.checked ? "#cc2200" : "#1a1a1a"
                    Rectangle {
                        anchors.centerIn: parent
                        width: 7; height: 7; radius: 1
                        color: cageCheck.checked ? "#fff" : "transparent"
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { text: "Level"; color: "#888"; font.pixelSize: 10; Layout.preferredWidth: 60 }
            Slider {
                id: cageLevelSlider
                Layout.fillWidth: true
                from: 1; to: 4; stepSize: 1
                value: Modeler && Modeler.subdivCageLevel !== undefined ? Modeler.subdivCageLevel : 2
                onValueChanged: {
                    if (Modeler && Modeler.setSubdivCageLevel) Modeler.setSubdivCageLevel(value)
                }
                background: Rectangle {
                    implicitHeight: 4
                    color: "#333"; radius: 2
                    Rectangle {
                        width: parent.width * (parent.parent.value - parent.parent.from) / (parent.parent.to - parent.parent.from)
                        height: parent.height
                        color: "#E10600"; radius: 2
                    }
                }
                handle: Rectangle {
                    implicitWidth: 8; implicitHeight: 14; radius: 2
                    color: "#ff6666"
                    x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                    y: (parent.height - height) / 2
                }
            }
            Text {
                text: cageLevelSlider.value.toFixed(0)
                color: "#aaa"
                font.pixelSize: 10
                Layout.preferredWidth: 30
                horizontalAlignment: Text.AlignRight
            }
        }

        Text {
            text: "Shows the smoothed (Catmull-Clark) result while the control cage is "
                 + "kept; edit the cage and the smooth mesh updates live. Toggling off "
                 + "restores the original cage geometry."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { text: "RAYTRACE VIEWPORT"; color: "#E10600"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }

            CheckBox {
                id: rtCheck
                text: "CPU preview"
                checked: Modeler && Modeler.rayTraceEnabled !== undefined ? Modeler.rayTraceEnabled : false
                font.pixelSize: 10
                onToggled: {
                    if (Modeler && Modeler.setRayTraceEnabled) Modeler.setRayTraceEnabled(checked)
                }
                indicator: Rectangle {
                    implicitWidth: 13; implicitHeight: 13; radius: 2
                    border.color: rtCheck.checked ? "#E10600" : "#555"
                    color: rtCheck.checked ? "#cc2200" : "#1a1a1a"
                    Rectangle {
                        anchors.centerIn: parent
                        width: 7; height: 7; radius: 1
                        color: rtCheck.checked ? "#fff" : "transparent"
                    }
                }
            }
        }

        Text {
            text: "Replaces the shaded viewport with a software ray-traced preview "
                 + "(sun + ambient, shadows, specular). Updates at low resolution "
                 + "while orbiting the camera."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
