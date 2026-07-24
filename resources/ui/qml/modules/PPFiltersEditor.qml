import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../widgets"
import ksEditor.PPFilters 1.0

Rectangle {
    id: root
    width: 1280
    height: 720
    color: "#121212"

    property string selectedFilter: PPFilters ? PPFilters.currentFilter : ""
    property string selectedSection: "Exposure"
    property real previewStrength: 1.0

    FileDialog {
        id: ppLoadDialog
        title: "Load Filter"
        nameFilters: ["Filter files (*.ini *.json)", "All files (*)"]
        onAccepted: { if (PPFilters) PPFilters.loadFilter(selectedFile.toString().replace("file:///", "")) }
    }

    FileDialog {
        id: ppSaveDialog
        title: "Save Filter"
        nameFilters: ["Filter files (*.ini)", "JSON files (*.json)", "All files (*)"]
        onAccepted: { if (PPFilters) PPFilters.saveFilter(selectedFile.toString().replace("file:///", "")) }
    }

    FileDialog {
        id: ppExportDialog
        title: "Export Filter"
        nameFilters: ["Filter files (*.ini)", "JSON files (*.json)", "All files (*)"]
        onAccepted: { if (PPFilters) PPFilters.exportFilter(selectedFile.toString().replace("file:///", "")) }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10

                Text { text: "PP FILTERS"; color: "white"; font.pixelSize: 14; font.bold: true }

                Item { Layout.fillWidth: true }

                AppButton { height: 28; text: "Load"; bgcolor: "transparent"; color: "#ffffff"; onClicked: ppLoadDialog.open() }
                AppButton { height: 28; text: "Save"; bgcolor: "transparent"; color: "#ffffff"; onClicked: ppSaveDialog.open() }
                AppButton { height: 28; text: "Scene"; bgcolor: "transparent"; color: "#ffffff"; onClicked: { if (PPFilters) PPFilters.reloadFilter() } }
                AppButton { height: 28; text: "Side by Side"; bgcolor: "transparent"; color: "#ffffff"; onClicked: { if (PPFilters) PPFilters.startPreview() } }
                AppButton { height: 28; text: "Wipe"; bgcolor: "transparent"; color: "#ffffff"; onClicked: { if (PPFilters) PPFilters.statusMessage("Wipe comparison activated") } }

                Text { text: "Strength:"; color: "#888888" }
                Slider {
                    id: strengthSlider; width: 100; from: 0; to: 1; value: previewStrength
                    onValueChanged: previewStrength = value
                }
                Text { text: Math.round(previewStrength * 100) + "%"; color: "#E10600" }

                AppButton {
                    id: liveToggle
                    height: 28
                    text: PPFilters && PPFilters.isPreviewActive ? "LIVE" : "OFF"
                    bgcolor: PPFilters && PPFilters.isPreviewActive ? "#E10600" : "transparent"
                    color: PPFilters && PPFilters.isPreviewActive ? "#121212" : "#888888"
                    onClicked: {
                        if (PPFilters) {
                            if (PPFilters.isPreviewActive) PPFilters.stopPreview()
                            else PPFilters.startPreview()
                        }
                    }
                }
            }
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // Left: filter list
            Rectangle {
                width: 200
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        height: 44
                        color: "#252526"
                        Text {
                            anchors.centerIn: parent
                            text: "FILTERS"
                            color: "#E10600"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        Repeater {
                            model: PPFilters ? PPFilters.getFilters() : []

                            delegate: Rectangle {
                                height: 56
                                radius: 4
                                color: selectedFilter === modelData.id ? "#E10600" + "22" : "transparent"
                                border.color: selectedFilter === modelData.id ? "#E10600" : "#333333"
                                border.width: 1

                                RowLayout {
                                    anchors { fill: parent; margins: 8 }
                                    spacing: 10

                                    Rectangle {
                                        width: 32; height: 32; radius: 4
                                        color: modelData.preview || "#333"
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Text {
                                            text: modelData.name || "Unnamed"
                                            color: selectedFilter === modelData.id ? "#E10600" : "#ffffff"
                                            font.pixelSize: 11; font.bold: true
                                        }
                                        Text { text: modelData.author || ""; color: "#888888"; font.pixelSize: 10 }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        root.selectedFilter = modelData.id
                                        if (PPFilters) PPFilters.setCurrentFilter(modelData.id)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Centre: preview + histogram
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#0a0a0a"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 2

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#1e1e1e"
                            Rectangle {
                                anchors.fill: parent
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#4a6080" }
                                    GradientStop { position: 0.5; color: "#303848" }
                                    GradientStop { position: 1.0; color: "#181c20" }
                                }
                            }
                            Rectangle {
                                anchors { top: parent.top; left: parent.left; margins: 8 }
                                width: 60; height: 20; radius: 4; color: "#00000080"
                                Text { anchors.centerIn: parent; text: "BEFORE"; color: "white"; font.pixelSize: 9; font.bold: true }
                            }
                        }

                        Rectangle { width: 2; Layout.fillHeight: true; color: "#E10600" }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#1e1e1e"
                            Rectangle {
                                anchors.fill: parent
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#5a7a9a" }
                                    GradientStop { position: 0.5; color: "#3a4858" }
                                    GradientStop { position: 1.0; color: "#202428" }
                                }
                            }
                            Rectangle {
                                anchors.fill: parent
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#00000066" }
                                    GradientStop { position: 0.5; color: "transparent" }
                                    GradientStop { position: 1.0; color: "#00000066" }
                                }
                            }
                            Rectangle {
                                anchors { top: parent.top; left: parent.left; margins: 8 }
                                width: 60; height: 20; radius: 4; color: "#00000080"
                                Text { anchors.centerIn: parent; text: "AFTER"; color: "#E10600"; font.pixelSize: 9; font.bold: true }
                            }
                            Text {
                                anchors { bottom: parent.bottom; right: parent.right; margins: 8 }
                                text: (PPFilters ? PPFilters.currentFilter : "") + " · Strength " + Math.round(previewStrength * 100) + "%"
                                color: "#888888"; font.pixelSize: 10
                            }
                        }
                    }
                }

                Rectangle {
                    height: 72
                    color: "#1e1e1e"
                    border.color: "#333333"
                    border.width: 1

                    RowLayout {
                        anchors { fill: parent; margins: 10 }
                        spacing: 12

                        Rectangle {
                            width: 200; height: 52; color: "#0a0a0a"; border.color: "#333333"; border.width: 1
                        }

                        ColumnLayout {
                            spacing: 4
                            Text { text: "Histogram"; color: "#ffffff"; font.pixelSize: 11 }
                            Text { id: histoValues; text: "R: --  G: --  B: --"; color: "#888888"; font.pixelSize: 10 }
                            Text { id: histoRange; text: "Min: --  Max: --"; color: "#888888"; font.pixelSize: 10 }
                        }

                        Item { Layout.fillWidth: true }

                        AppButton { height: 36; text: "Auto Tone"; bgcolor: "#E10600"; color: "#121212"; onClicked: { if (PPFilters) PPFilters.statusMessage("Auto tone applied") } }
                        AppButton { height: 36; text: "Reset"; bgcolor: "transparent"; color: "#ffffff"; onClicked: { if (PPFilters) PPFilters.resetParameters() } }
                        AppButton { height: 36; text: "Save Filter"; bgcolor: "#E10600"; color: "#121212"; onClicked: ppSaveDialog.open() }
                    }
                }
            }

            // Right: parameters
            Rectangle {
                width: 240
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        height: 44
                        color: "#252526"
                        Text {
                            anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                            text: "PARAMETERS"
                            color: "white"; font.pixelSize: 12; font.bold: true
                        }
                    }

                    RowLayout {
                        height: 32
                        anchors.margins: 8
                        Repeater {
                            model: ["Exposure", "Color", "Tone", "Bloom", "Lens", "DOF", "Glare", "GodRays"]
                            delegate: Rectangle {
                                height: 24; radius: 4
                                color: selectedSection === modelData ? "#E10600" : "#333333"
                                border.color: selectedSection === modelData ? "#E10600" : "transparent"
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: selectedSection === modelData ? "#121212" : "#888888"
                                    font.pixelSize: 10
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.selectedSection = modelData
                                }
                            }
                        }
                    }

                    Flickable {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentHeight: sectionCol.height + 20
                        clip: true

                        ColumnLayout {
                            id: sectionCol
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            Text { text: selectedSection.toUpperCase(); color: "#E10600"; font.pixelSize: 11; font.bold: true }

                            // Exposure section: hardcoded sliders wired to bridge
                            Repeater {
                                model: selectedSection === "Exposure" ? [
                                    {label: "Exposure:", key: "EXPOSURE", min: -3, max: 3, def: 0.2},
                                    {label: "Highlights:", key: "HIGHLIGHTS", min: -1, max: 1, def: -0.15},
                                    {label: "Shadows:", key: "SHADOWS", min: -1, max: 1, def: 0.1},
                                    {label: "Whites:", key: "WHITES", min: -1, max: 1, def: 0.05},
                                    {label: "Blacks:", key: "BLACKS", min: -1, max: 1, def: -0.05}
                                ] : []

                                delegate: RowLayout {
                                    Text { text: modelData.label; color: "#bbbbbb"; Layout.fillWidth: true }
                                    Slider {
                                        from: modelData.min; to: modelData.max
                                        value: PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def
                                        Layout.preferredWidth: 80
                                        onValueChanged: { if (PPFilters) PPFilters.setParameter(modelData.key, value) }
                                    }
                                    Text {
                                        text: (PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def).toFixed(2)
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }

                            // Color section
                            Repeater {
                                model: selectedSection === "Color" ? [
                                    {label: "Saturation:", key: "SATURATION", min: 0, max: 2, def: 1},
                                    {label: "Contrast:", key: "CONTRAST", min: 0, max: 2, def: 1},
                                    {label: "Temperature:", key: "TEMPERATURE", min: -1, max: 1, def: 0},
                                    {label: "Tint:", key: "TINT", min: -1, max: 1, def: 0},
                                    {label: "Vibrance:", key: "VIBRANCE", min: 0, max: 2, def: 1}
                                ] : []

                                delegate: RowLayout {
                                    Text { text: modelData.label; color: "#bbbbbb"; Layout.fillWidth: true }
                                    Slider {
                                        from: modelData.min; to: modelData.max
                                        value: PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def
                                        Layout.preferredWidth: 80
                                        onValueChanged: { if (PPFilters) PPFilters.setParameter(modelData.key, value) }
                                    }
                                    Text {
                                        text: (PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def).toFixed(2)
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }

                            // Tone section
                            Repeater {
                                model: selectedSection === "Tone" ? [
                                    {label: "Gamma:", key: "GAMMA", min: 0.5, max: 3, def: 2.2},
                                    {label: "Toe:", key: "TOE", min: 0, max: 1, def: 0.5},
                                    {label: "Shoulder:", key: "SHOULDER", min: 0, max: 1, def: 0.5},
                                    {label: "Linear Segment:", key: "LINEAR", min: 0, max: 1, def: 0.3}
                                ] : []

                                delegate: RowLayout {
                                    Text { text: modelData.label; color: "#bbbbbb"; Layout.fillWidth: true }
                                    Slider {
                                        from: modelData.min; to: modelData.max
                                        value: PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def
                                        Layout.preferredWidth: 80
                                        onValueChanged: { if (PPFilters) PPFilters.setParameter(modelData.key, value) }
                                    }
                                    Text {
                                        text: (PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def).toFixed(2)
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }

                            // Bloom section
                            Repeater {
                                model: selectedSection === "Bloom" ? [
                                    {label: "Bloom Intensity:", key: "BLOOM_INTENSITY", min: 0, max: 5, def: 1},
                                    {label: "Bloom Threshold:", key: "BLOOM_THRESHOLD", min: 0, max: 2, def: 0.8},
                                    {label: "Bloom Radius:", key: "BLOOM_RADIUS", min: 0, max: 20, def: 5}
                                ] : []

                                delegate: RowLayout {
                                    Text { text: modelData.label; color: "#bbbbbb"; Layout.fillWidth: true }
                                    Slider {
                                        from: modelData.min; to: modelData.max
                                        value: PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def
                                        Layout.preferredWidth: 80
                                        onValueChanged: { if (PPFilters) PPFilters.setParameter(modelData.key, value) }
                                    }
                                    Text {
                                        text: (PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def).toFixed(2)
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }

                            // Lens section
                            Repeater {
                                model: selectedSection === "Lens" ? [
                                    {label: "Vignette:", key: "VIGNETTE", min: 0, max: 2, def: 0.3},
                                    {label: "Chromatic Aberration:", key: "CHROMATIC_ABERRATION", min: 0, max: 5, def: 0},
                                    {label: "Film Grain:", key: "FILM_GRAIN", min: 0, max: 1, def: 0},
                                    {label: "Sharpness:", key: "SHARPNESS", min: 0, max: 2, def: 0}
                                ] : []

                                delegate: RowLayout {
                                    Text { text: modelData.label; color: "#bbbbbb"; Layout.fillWidth: true }
                                    Slider {
                                        from: modelData.min; to: modelData.max
                                        value: PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def
                                        Layout.preferredWidth: 80
                                        onValueChanged: { if (PPFilters) PPFilters.setParameter(modelData.key, value) }
                                    }
                                    Text {
                                        text: (PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def).toFixed(2)
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }

                            // DOF section
                            Repeater {
                                model: selectedSection === "DOF" ? [
                                    {label: "DOF Focus Distance:", key: "DOF_FOCUS_DISTANCE", min: 0, max: 1000, def: 15},
                                    {label: "DOF Aperture:", key: "DOF_APERTURE", min: 0, max: 100, def: 20},
                                    {label: "DOF Blur:", key: "DOF_BLUR", min: 0, max: 10, def: 3}
                                ] : []

                                delegate: RowLayout {
                                    Text { text: modelData.label; color: "#bbbbbb"; Layout.fillWidth: true }
                                    Slider {
                                        from: modelData.min; to: modelData.max
                                        value: PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def
                                        Layout.preferredWidth: 80
                                        onValueChanged: { if (PPFilters) PPFilters.setParameter(modelData.key, value) }
                                    }
                                    Text {
                                        text: (PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def).toFixed(2)
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }

                            // Glare section
                            Repeater {
                                model: selectedSection === "Glare" ? [
                                    {label: "Glare Intensity:", key: "GLARE_INTENSITY", min: 0, max: 5, def: 1},
                                    {label: "Glare Threshold:", key: "GLARE_THRESHOLD", min: 0, max: 2, def: 0.8},
                                    {label: "Glare Radius:", key: "GLARE_RADIUS", min: 0, max: 20, def: 5}
                                ] : []

                                delegate: RowLayout {
                                    Text { text: modelData.label; color: "#bbbbbb"; Layout.fillWidth: true }
                                    Slider {
                                        from: modelData.min; to: modelData.max
                                        value: PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def
                                        Layout.preferredWidth: 80
                                        onValueChanged: { if (PPFilters) PPFilters.setParameter(modelData.key, value) }
                                    }
                                    Text {
                                        text: (PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def).toFixed(2)
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }

                            // GodRays section
                            Repeater {
                                model: selectedSection === "GodRays" ? [
                                    {label: "God Rays Intensity:", key: "GODRAYS_INTENSITY", min: 0, max: 5, def: 1},
                                    {label: "God Rays Decay:", key: "GODRAYS_DECAY", min: 0, max: 2, def: 0.95},
                                    {label: "God Rays Exposure:", key: "GODRAYS_EXPOSURE", min: 0, max: 2, def: 1}
                                ] : []

                                delegate: RowLayout {
                                    Text { text: modelData.label; color: "#bbbbbb"; Layout.fillWidth: true }
                                    Slider {
                                        from: modelData.min; to: modelData.max
                                        value: PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def
                                        Layout.preferredWidth: 80
                                        onValueChanged: { if (PPFilters) PPFilters.setParameter(modelData.key, value) }
                                    }
                                    Text {
                                        text: (PPFilters ? PPFilters.getParameterValue(modelData.key) : modelData.def).toFixed(2)
                                        color: "#E10600"; font.pixelSize: 10
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    ColumnLayout {
                        anchors.margins: 10
                        spacing: 8
                        AppButton { height: 34; text: "Apply to AC"; bgcolor: "#E10600"; color: "#121212"; onClicked: { if (PPFilters) PPFilters.exportToACDialog() } }
                        AppButton { height: 34; text: "Export pp_filter"; bgcolor: "#ff6600"; color: "#ffffff"; onClicked: ppExportDialog.open() }
                        AppButton { height: 34; text: "Export as Preset"; bgcolor: "transparent"; color: "#ffffff"; onClicked: { if (PPFilters) PPFilters.statusMessage("Exported as preset") } }
                    }
                }
            }
        }

        Rectangle {
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                Text { id: statusText; text: "Ready"; color: "#E10600"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "ksEditor v1.0 - PP Filters"; color: "#666"; font.pixelSize: 10 }
            }
        }
    }

    Connections {
        target: PPFilters
        function onStatusMessage(msg) { statusText.text = msg }
        function onFilterLoaded() { statusText.text = "Filter loaded: " + PPFilters.currentFilter }
        function onFilterSaved() { statusText.text = "Filter saved" }
        function onParameterChanged(name, value) { statusText.text = name + " = " + value.toFixed(2) }
    }
}


