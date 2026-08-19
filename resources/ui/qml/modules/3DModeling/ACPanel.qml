import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: acPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property real trackWidth: 12.0
    property real trackCamber: 0.0
    property int smoothIterations: 3
    property real terrainSize: 200
    property real terrainHeight: 20

    function pageRoot() {
        var r = acPanel.parent
        while (r && r.objectName !== "pageModelerRoot") r = r.parent
        return r
    }

    function applyTrackSettings() {
        if (Modeler.setTrackWidth) Modeler.setTrackWidth(trackWidth)
        if (Modeler.setTrackCamber) Modeler.setTrackCamber(trackCamber)
    }

    function openGpxImport() { var r = pageRoot(); if (r && r.gpxImportDialog) r.gpxImportDialog.open() }
    function openKmlImport() { var r = pageRoot(); if (r && r.kmlImportDialog) r.kmlImportDialog.open() }
    function openGpxExport() { var r = pageRoot(); if (r && r.gpxExportDialog) r.gpxExportDialog.open() }
    function openKmlExport() { var r = pageRoot(); if (r && r.kmlExportDialog) r.kmlExportDialog.open() }
    function openAiExport() { var r = pageRoot(); if (r && r.aiExportDialog) r.aiExportDialog.open() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "AC TRACK TOOLS"
                color: "#ff6600"
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
            text: "Points: " + (Modeler.trackPointCount ? Modeler.trackPointCount() : 0) +
                  " | Length: " + (Modeler.trackLength ? Modeler.trackLength().toFixed(1) + " m" : "-")
            color: "#aaa"
            font.pixelSize: 10
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "TRACK SETTINGS"; color: "#888"; font.pixelSize: 10; font.bold: true }

        Text { text: "Width: " + trackWidth.toFixed(1); color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: widthSlider
            Layout.fillWidth: true
            from: 2.0; to: 30.0; stepSize: 0.5
            value: trackWidth
            onMoved: { trackWidth = value; applyTrackSettings() }
            background: Rectangle { x: widthSlider.leftPadding; y: widthSlider.topPadding + widthSlider.availableHeight / 2 - 2; width: widthSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: widthSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#ff6600" }
            }
            handle: Rectangle { x: widthSlider.leftPadding + widthSlider.visualPosition * (widthSlider.availableWidth - width); y: widthSlider.topPadding + widthSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        Text { text: "Camber: " + trackCamber.toFixed(3); color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: camberSlider
            Layout.fillWidth: true
            from: -0.06; to: 0.06; stepSize: 0.001
            value: trackCamber
            onMoved: { trackCamber = value; applyTrackSettings() }
            background: Rectangle { x: camberSlider.leftPadding; y: camberSlider.topPadding + camberSlider.availableHeight / 2 - 2; width: camberSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: camberSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#ff6600" }
            }
            handle: Rectangle { x: camberSlider.leftPadding + camberSlider.visualPosition * (camberSlider.availableWidth - width); y: camberSlider.topPadding + camberSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        Text { text: "Smooth iterations: " + smoothIterations; color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: smoothSlider
            Layout.fillWidth: true
            from: 1; to: 10; stepSize: 1
            value: smoothIterations
            onMoved: smoothIterations = value
            background: Rectangle { x: smoothSlider.leftPadding; y: smoothSlider.topPadding + smoothSlider.availableHeight / 2 - 2; width: smoothSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: smoothSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#ff6600" }
            }
            handle: Rectangle { x: smoothSlider.leftPadding + smoothSlider.visualPosition * (smoothSlider.availableWidth - width); y: smoothSlider.topPadding + smoothSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        Text { text: "Terrain size: " + terrainSize.toFixed(0); color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: terrainSizeSlider
            Layout.fillWidth: true
            from: 50; to: 500; stepSize: 10
            value: terrainSize
            onMoved: terrainSize = value
            background: Rectangle { x: terrainSizeSlider.leftPadding; y: terrainSizeSlider.topPadding + terrainSizeSlider.availableHeight / 2 - 2; width: terrainSizeSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: terrainSizeSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#ff6600" }
            }
            handle: Rectangle { x: terrainSizeSlider.leftPadding + terrainSizeSlider.visualPosition * (terrainSizeSlider.availableWidth - width); y: terrainSizeSlider.topPadding + terrainSizeSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        Text { text: "Terrain height: " + terrainHeight.toFixed(0); color: "#aaa"; font.pixelSize: 9 }
        Slider {
            id: terrainHeightSlider
            Layout.fillWidth: true
            from: 1; to: 100; stepSize: 1
            value: terrainHeight
            onMoved: terrainHeight = value
            background: Rectangle { x: terrainHeightSlider.leftPadding; y: terrainHeightSlider.topPadding + terrainHeightSlider.availableHeight / 2 - 2; width: terrainHeightSlider.availableWidth; height: 4; radius: 2; color: "#3e3e42"
                Rectangle { width: terrainHeightSlider.visualPosition * parent.width; height: 4; radius: 2; color: "#ff6600" }
            }
            handle: Rectangle { x: terrainHeightSlider.leftPadding + terrainHeightSlider.visualPosition * (terrainHeightSlider.availableWidth - width); y: terrainHeightSlider.topPadding + terrainHeightSlider.availableHeight / 2 - height / 2; width: 14; height: 14; radius: 7; color: "#ffffff" }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "GENERATE"; color: "#888"; font.pixelSize: 10; font.bold: true }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "Generate Track Mesh"
            bgcolor: "#ff6600"
            color: "#121212"
            font.bold: true
            font.pixelSize: 11
            onClicked: { applyTrackSettings(); if (Modeler.generateTrackMesh) Modeler.generateTrackMesh() }
        }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "AI Line (centerline)"
            bgcolor: "#3e3e42"
            color: "#fff"
            font.pixelSize: 10
            onClicked: { if (Modeler.generateAILine) Modeler.generateAILine() }
        }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "Smooth Track (" + smoothIterations + " it)"
            bgcolor: "#3e3e42"
            color: "#fff"
            font.pixelSize: 10
            onClicked: { if (Modeler.smoothTrackPoints) Modeler.smoothTrackPoints(smoothIterations) }
        }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "Generate Terrain"
            bgcolor: "#3e3e42"
            color: "#fff"
            font.pixelSize: 10
            onClicked: { if (Modeler.generateTerrain) Modeler.generateTerrain(terrainSize, terrainHeight) }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text { text: "GEO / EXPORT"; color: "#888"; font.pixelSize: 10; font.bold: true }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 4
            rowSpacing: 4

            AppButton { text: "Import GPX"; height: 28; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: openGpxImport() }
            AppButton { text: "Import KML"; height: 28; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: openKmlImport() }
            AppButton { text: "Export GPX"; height: 28; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: openGpxExport() }
            AppButton { text: "Export KML"; height: 28; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: openKmlExport() }
            AppButton { text: "Export AI Line"; height: 28; bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10; onClicked: openAiExport() }
        }

        Item { Layout.fillHeight: true }

        Text {
            text: "Import GPX/KML builds m_trackPoints from real GPS data (flat-earth approx). "
                 + "Generate Track Mesh creates a ribbon with UVs and camber. Export AI writes "
                 + "the AC ai format (x,y,z,width,speed)."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
