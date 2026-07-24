import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    color: "#1a1a1a"

    property string statusText: ""
    property int importedPoints: 0
    property var importedPointsData: []
    property var importedVertices: []
    property var importedCameras: []
    property var componentTree: ({})

    // Keyboard shortcuts
    Shortcut { sequence: "Ctrl+I"; onActivated: aiOpenDialog.open(); description: "Import AI Line" }
    Shortcut { sequence: "Ctrl+E"; onActivated: aiSaveDialog.open(); description: "Export AI Line" }
    Shortcut { sequence: "Ctrl+Shift+I"; onActivated: csvOpenDialog.open(); description: "Import CSV" }
    Shortcut { sequence: "Ctrl+Shift+E"; onActivated: csvSaveDialog.open(); description: "Export CSV" }
    Shortcut { sequence: "Ctrl+L"; onActivated: FormatTools.loadComponentTree ""; description: "Load Component Tree" }
    Shortcut { sequence: "Ctrl+S"; onActivated: FormatTools.saveComponentTree ""; description: "Save Component Tree" }
    Shortcut { sequence: "Ctrl+P"; onActivated: FormatTools.createCarProject("", "new_car", {}); description: "Create Car Project" }
    Shortcut { sequence: "Ctrl+T"; onActivated: FormatTools.createTrackProject("", "new_track", {}); description: "Create Track Project" }
    Shortcut { sequence: "F5"; onActivated: refreshTree(); description: "Refresh Tree" }

    FileDialog {
        id: aiOpenDialog
        title: "Import AI Line"
        nameFilters: ["AI files (*.ai)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            var result = FormatTools.importAiLine(path, scalingSlider.value, extraDataCheck.checked)
            if (result && result.pointCount > 0) {
                importedPoints = result.pointCount
                importedPointsData = result.idealLine || []
                statusText = "Imported " + importedPoints + " points from " + path
            }
        }
    }

    FileDialog {
        id: aiSaveDialog
        title: "Export AI Line"
        nameFilters: ["AI files (*.ai)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            FormatTools.exportAiLine(path, importedPointsData, scalingSlider.value,
                shiftSpinner.value, reverseCheck.checked,
                fixedBordersCheck.checked, leftBorderSpinner.value, rightBorderSpinner.value)
            statusText = "Exported AI line (" + importedPointsData.length + " points) to " + path
        }
    }

    FileDialog {
        id: csvOpenDialog
        title: "Import CSV Border"
        nameFilters: ["CSV files (*.csv)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            var vertices = FormatTools.importCsv(path, scalingSlider.value)
            importedVertices = vertices || []
            statusText = "Imported " + importedVertices.length + " border points"
        }
    }

    FileDialog {
        id: csvSaveDialog
        title: "Export CSV Border"
        nameFilters: ["CSV files (*.csv)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            FormatTools.exportCsv(path, importedVertices, scalingSlider.value,
                shiftSpinner.value, reverseCheck.checked, skipPoTCheck.checked)
            statusText = "Exported CSV (" + importedVertices.length + " vertices) to " + path
        }
    }

    FileDialog {
        id: cameraOpenDialog
        title: "Import Camera.ini"
        nameFilters: ["INI files (*.ini)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            var cameras = FormatTools.importCameraIni(path)
            importedCameras = cameras || []
            statusText = "Imported " + importedCameras.length + " cameras"
        }
    }

    FileDialog {
        id: cameraSaveDialog
        title: "Export Camera.ini"
        nameFilters: ["INI files (*.ini)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            FormatTools.exportCameraIni(path, importedCameras)
            statusText = "Exported " + importedCameras.length + " cameras to " + path
        }
    }

    FileDialog {
        id: batchAiDialog
        title: "Batch Import AI Lines"
        selectMultiple: true
        nameFilters: ["AI files (*.ai)", "All files (*)"]
        onAccepted: {
            var paths = []
            for (var i = 0; i < selectedFiles.length; i++) {
                paths.push(selectedFiles[i].toString().replace("file:///", ""))
            }
            var result = FormatTools.batchImportAiLines(paths, scalingSlider.value)
            if (result) {
                statusText = "Batch: " + result.successCount + "/" + result.totalFiles + " files imported"
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 32
            color: "#252525"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Text { text: "Format Tools"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                Text { text: importedPoints > 0 ? importedPoints + " points loaded" : "No data"; color: "#888"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }

                Button {
                    text: "Batch Import"
                    flat: true
                    font.pixelSize: 11
                    onClicked: batchAiDialog.open()
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Rectangle {
                color: "#1e1e1e"
                SplitView.preferredWidth: 320

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 8
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        // ── Scaling ─────────────────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: scalingCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: scalingCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 4

                                Text { text: "Global Settings"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                RowLayout {
                                    Text { text: "Scaling:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Slider {
                                        id: scalingSlider
                                        from: 0.001
                                        to: 1.0
                                        value: 1.0
                                        stepSize: 0.001
                                        Layout.fillWidth: true
                                    }
                                    Text { text: scalingSlider.value.toFixed(3); color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 50 }
                                }

                                RowLayout {
                                    Text { text: "Shift:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    SpinBox {
                                        id: shiftSpinner
                                        from: -1000
                                        to: 1000
                                        value: 0
                                        Layout.fillWidth: true
                                    }
                                }

                                RowLayout {
                                    Text { text: "Options:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    CheckBox { id: reverseCheck; text: "Reverse"; font.pixelSize: 11 }
                                    CheckBox { id: extraDataCheck; text: "Extra Data"; font.pixelSize: 11; checked: true }
                                    CheckBox { id: skipPoTCheck; text: "Skip PoT"; font.pixelSize: 11 }
                                }
                            }
                        }

                        // ── AI Line Operations ──────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: aiLineCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: aiLineCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "AI Line Operations"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                                Text { text: "Binary AI line format support"; color: "#666"; font.pixelSize: 9 }

                                RowLayout {
                                    spacing: 4
                                    Button {
                                        text: "Import AI Line"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: aiOpenDialog.open()
                                    }
                                    Button {
                                        text: "Export AI Line"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: aiSaveDialog.open()
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Button {
                                        text: "Import Borders"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: {
                                            var result = FormatTools.importAiLineBorders(
                                                aiOpenDialog.selectedFiles.length > 0 ?
                                                aiOpenDialog.selectedFiles[0].toString().replace("file:///", "") : "",
                                                scalingSlider.value)
                                            statusText = "Imported borders from AI line"
                                        }
                                    }
                                }

                                CheckBox {
                                    id: fixedBordersCheck
                                    text: "Fixed Borders"
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    Text { text: "Left:"; color: "#ccc"; font.pixelSize: 11 }
                                    SpinBox {
                                        id: leftBorderSpinner
                                        from: 0
                                        to: 1000
                                        value: 200
                                        decimals: 1
                                    }
                                    Text { text: "Right:"; color: "#ccc"; font.pixelSize: 11 }
                                    SpinBox {
                                        id: rightBorderSpinner
                                        from: 0
                                        to: 1000
                                        value: 200
                                        decimals: 1
                                    }
                                }
                            }
                        }

                        // ── CSV Operations ──────────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: csvCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: csvCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "CSV Border Operations"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                                Text { text: "CSV border import/export"; color: "#666"; font.pixelSize: 9 }

                                RowLayout {
                                    spacing: 4
                                    Button {
                                        text: "Import CSV"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: csvOpenDialog.open()
                                    }
                                    Button {
                                        text: "Export CSV"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: csvSaveDialog.open()
                                    }
                                }
                            }
                        }

                        // ── Camera Operations ───────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: cameraCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: cameraCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Camera.ini Operations"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                RowLayout {
                                    spacing: 4
                                    Button {
                                        text: "Import Cameras"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: cameraOpenDialog.open()
                                    }
                                    Button {
                                        text: "Export Cameras"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: cameraSaveDialog.open()
                                    }
                                }
                            }
                        }

                        // ── Material Tools ──────────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: matCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: matCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Material Fix Tools"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                                Text { text: "Fix material properties for sim rendering"; color: "#666"; font.pixelSize: 9 }

                                Button {
                                    text: "Fix Alpha Blend to Opaque"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var result = FormatTools.fixAlphaBlendToOpaque([])
                                        statusText = "Fixed " + result.fixedCount + " materials"
                                    }
                                }

                                Button {
                                    text: "Reset Specular & Metallic to 0"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var result = FormatTools.resetSpecularMetallic([])
                                        statusText = "Reset " + result.resetCount + " materials"
                                    }
                                }
                            }
                        }

                        // ── Mesh Tools ──────────────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: meshCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: meshCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Mesh Cleanup Tools"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                                Text { text: "Vertex merging and cleanup"; color: "#666"; font.pixelSize: 9 }

                                RowLayout {
                                    Text { text: "Threshold:"; color: "#ccc"; font.pixelSize: 11 }
                                    SpinBox {
                                        id: mergeThreshold
                                        from: 0
                                        to: 100
                                        value: 1
                                        decimals: 3
                                        stepSize: 0.001
                                    }
                                }

                                Button {
                                    text: "Merge Vertices by Distance"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var result = FormatTools.mergeByDistance([], mergeThreshold.value / 1000.0)
                                        statusText = "Merged: " + result.originalCount + " -> " + result.mergedCount + " vertices"
                                    }
                                }
                            }
                        }

                        // ── Name Cleanup ────────────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: nameCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: nameCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Name Cleanup (FBX Export)"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                                Text { text: "Strip auto-generated suffixes"; color: "#666"; font.pixelSize: 9 }

                                Button {
                                    text: "Clean .001 Suffixes"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var cleaned = FormatTools.cleanNames(["test.001", "mesh.002", "normal"])
                                        statusText = "Cleaned: " + cleaned.join(", ")
                                    }
                                }
                            }
                        }

                        // ── AC Object Helpers ───────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: acObjCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: acObjCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "AC Object Helpers"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                                Text { text: "Track object creation tools"; color: "#666"; font.pixelSize: 9 }

                                RowLayout {
                                    spacing: 4
                                    Button {
                                        text: "Start Position"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: {
                                            var obj = FormatTools.createStartPosition(0, 0, 0, false)
                                            statusText = "Created start position"
                                        }
                                    }
                                    Button {
                                        text: "Timing (L/R)"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: {
                                            var left = FormatTools.createTimingPosition(-5, 0, false)
                                            var right = FormatTools.createTimingPosition(5, 0, true)
                                            statusText = "Created timing positions"
                                        }
                                    }
                                }

                                Button {
                                    text: "Grid Lineup (2x2)"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        for (var r = 0; r < 2; r++) {
                                            for (var c = 0; c < 2; c++) {
                                                FormatTools.createAcObject("grid_position", {
                                                    "row": r, "col": c,
                                                    "spacing": 5.0,
                                                    "startX": 0, "startZ": 0,
                                                    "baseRotation": 0
                                                })
                                            }
                                        }
                                        statusText = "Created 2x2 grid lineup"
                                    }
                                }
                            }
                        }

                        // ── Replay Visualization ────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: replayCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: replayCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "3D Replay Visualization"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                                Text { text: "Import replay data as 3D path"; color: "#666"; font.pixelSize: 9 }

                                Button {
                                    text: "Import Replay Path"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var path = FormatTools.importReplayPath("", 0)
                                        statusText = "Imported " + path.length + " replay points"
                                    }
                                }
                            }
                        }

                        // ── Naming Convention ────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: namingCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: namingCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Naming Convention"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                                Text { text: "Format: modder_year_manufacturer_carname"; color: "#666"; font.pixelSize: 9 }

                                RowLayout {
                                    Text { text: "Modder:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 60 }
                                    TextField { id: modderInput; placeholderText: "asr"; font.pixelSize: 11; Layout.fillWidth: true }
                                }
                                RowLayout {
                                    Text { text: "Year:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 60 }
                                    TextField { id: yearInput; placeholderText: "1991"; font.pixelSize: 11; Layout.fillWidth: true }
                                }
                                RowLayout {
                                    Text { text: "Mfr:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 60 }
                                    TextField { id: mfrInput; placeholderText: "benetton"; font.pixelSize: 11; Layout.fillWidth: true }
                                }
                                RowLayout {
                                    Text { text: "Car:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 60 }
                                    TextField { id: carInput; placeholderText: "b191"; font.pixelSize: 11; Layout.fillWidth: true }
                                }

                                Button {
                                    text: "Generate Car Name"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var name = FormatTools.generateCarName(modderInput.text, parseInt(yearInput.text) || 2024, mfrInput.text, carInput.text)
                                        statusText = "Generated: " + name
                                    }
                                }

                                Button {
                                    text: "Generate Component Names"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var comps = FormatTools.generateComponentNames(carInput.text, mfrInput.text)
                                        var names = []
                                        for (var i = 0; i < comps.length; i++) names.push(comps[i].name)
                                        statusText = comps.length + " components: " + names.join(", ")
                                    }
                                }

                                Button {
                                    text: "Show Node Hierarchy"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var tree = FormatTools.getComponentTree()
                                        statusText = "Tree has " + Object.keys(tree).length + " root nodes"
                                    }
                                }

                                Button {
                                    text: "Validate Naming"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var name = FormatTools.generateCarName(modderInput.text, parseInt(yearInput.text) || 2024, mfrInput.text, carInput.text)
                                        var tree = FormatTools.getComponentTree()
                                        var result = FormatTools.validateCarNaming(name, tree)
                                        statusText = result.valid ? "Valid (" + result.nodeCount + " nodes)" : "Errors: " + result.errors.join("; ")
                                    }
                                }

                                Button {
                                    text: "Parse Node List"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var sample = "ben_body=0\nben_dashboard=0\nben_seat=0"
                                        var tree = FormatTools.parseNodeHierarchy(sample)
                                        statusText = "Parsed " + Object.keys(tree).length + " nodes"
                                    }
                                }

                                Button {
                                    text: "Create Car Project"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var name = FormatTools.generateCarName(modderInput.text, parseInt(yearInput.text) || 2024, mfrInput.text, carInput.text)
                                        FormatTools.createCarProject("", name, {
                                            "manufacturer": mfrInput.text,
                                            "year": yearInput.text,
                                            "brand": mfrInput.text,
                                            "model": carInput.text,
                                            "author": modderInput.text
                                        })
                                        statusText = "Created project: " + name
                                    }
                                }
                            }
                        }

                        // ── Track Project ────────────────────────
                        Rectangle {
                            Layout.fillWidth: true
                            height: trackProjCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: trackProjCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Track Project"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                RowLayout {
                                    Text { text: "Track:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 60 }
                                    TextField { id: trackInput; placeholderText: "my_track"; font.pixelSize: 11; Layout.fillWidth: true }
                                }

                                Button {
                                    text: "Create Track Project"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        FormatTools.createTrackProject("", trackInput.text, {
                                            "name": trackInput.text,
                                            "author": modderInput.text
                                        })
                                        statusText = "Created track project: " + trackInput.text
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // ── Status Panel ────────────────────────────────────
            Rectangle {
                color: "#252525"
                SplitView.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Text { text: "Status"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8

                            Text {
                                text: statusText || "Ready. Select a tool from the left panel."
                                color: "#aaa"
                                font.pixelSize: 11
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Text {
                                text: "\nImported Points: " + importedPoints
                                color: "#888"
                                font.pixelSize: 10
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: creditsCol.height + 16
                        color: "#1a1a1a"
                        radius: 4

                        ColumnLayout {
                            id: creditsCol
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4

                            Text { text: "Credits"; color: "#E10600"; font.pixelSize: 11; font.bold: true }
                            Text { text: "AI line format based on racing sim community tools"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "CSV border format from sim track tools"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Camera/overlay config from sim SDKs"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Material fixes for sim rendering pipelines"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Mesh cleanup for sim vertex limits"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Object helpers for track construction"; color: "#888"; font.pixelSize: 9 }

                            Item { Layout.fillHeight: true }

                            Text { text: "Keyboard Shortcuts"; color: "#E10600"; font.pixelSize: 10; font.bold: true }
                            Text { text: "Ctrl+I  Import AI Line"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "Ctrl+E  Export AI Line"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "Ctrl+Shift+I  Import CSV"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "Ctrl+Shift+E  Export CSV"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "Ctrl+L  Load Tree"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "Ctrl+S  Save Tree"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "Ctrl+P  New Car Project"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "Ctrl+T  New Track Project"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "F5  Refresh Tree"; color: "#666"; font.pixelSize: 8 }
                        }
                    }
                }
            }
        }
    }

    // ── Tree View Panel ──────────────────────────────────────
    property var flatNodes: []

    function refreshTree() {
        componentTree = FormatTools.getComponentTree()
        flatNodes = FormatTools.flattenTree(componentTree)
        statusText = "Tree refreshed: " + flatNodes.length + " nodes"
    }

    function findNode(path) {
        var parts = path.split("/")
        var current = componentTree
        for (var i = 0; i < parts.length; i++) {
            if (parts[i] === "") continue
            if (!current[parts[i]]) return null
            if (i < parts.length - 1) {
                current = current[parts[i]]
            }
        }
        return current
    }

    Connections {
        target: FormatTools
        function onStatusMessage(msg) { statusText = msg }
        function onErrorMessage(msg) { statusText = "ERROR: " + msg }
        function onImportComplete(path, count) { importedPoints = count }
    }

    Component.onCompleted: {
        refreshTree()
    }
}
