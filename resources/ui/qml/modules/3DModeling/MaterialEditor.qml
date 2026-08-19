import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: materialEditor
    anchors.fill: parent
    color: "transparent"

    property int currentMaterialIndex: -1
    property string currentMaterialName: "Default"
    property color albedoColor: "#c8c8d0"
    property real metallicValue: 0.0
    property real roughnessValue: 0.5
    property real normalStrengthValue: 1.0
    property color emissiveColor: "#000000"
    property real opacityValue: 1.0
    property bool hasSelection: Modeler ? Modeler.hasSelection : false

    signal closeRequested()

    // Material drag chip: drag this swatch onto an object in the viewport to
    // apply the current editor material directly to it (no selection needed).
    property color dragChipColor: albedoColor
    property real dragChipMetallic: metallicValue
    property real dragChipRoughness: roughnessValue
    property real dragChipOpacity: opacityValue
    signal dragPreset(string preset)

    property string dragPresetName: ""

    function selectMaterial(index) {
        currentMaterialIndex = index
        if (Modeler) {
            Modeler.createMaterial("Material_" + index)
            albedoColor = "#c8c8d0"
            metallicValue = 0.0
            roughnessValue = 0.5
            normalStrengthValue = 1.0
            emissiveColor = "#000000"
            opacityValue = 1.0
        }
    }

    function applyMaterialPreset(preset) {
        if (!hasSelection) return
        switch (preset) {
            case "ksPBR":
                Modeler.setMaterialMetallic(0.5)
                Modeler.setMaterialRoughness(0.5)
                Modeler.setMaterialAlbedo(0.78, 0.78, 0.78, 1.0)
                metallicValue = 0.5; roughnessValue = 0.5
                albedoColor = "#c8c8c8"
                break
            case "ksDrude":
                Modeler.setMaterialMetallic(0.0)
                Modeler.setMaterialRoughness(0.8)
                Modeler.setMaterialAlbedo(0.15, 0.15, 0.15, 1.0)
                metallicValue = 0.0; roughnessValue = 0.8
                albedoColor = "#262626"
                break
            case "ksGlass":
                Modeler.setMaterialMetallic(0.0)
                Modeler.setMaterialRoughness(0.05)
                Modeler.setMaterialAlbedo(0.9, 0.95, 1.0, 0.3)
                Modeler.setMaterialOpacity(0.3)
                metallicValue = 0.0; roughnessValue = 0.05
                albedoColor = "#e6f2ff"; opacityValue = 0.3
                break
            case "ksEmissive":
                Modeler.setMaterialMetallic(0.0)
                Modeler.setMaterialRoughness(0.6)
                Modeler.setMaterialAlbedo(1.0, 1.0, 1.0, 1.0)
                Modeler.setMaterialEmissive(2.0, 0.3, 0.0)
                metallicValue = 0.0; roughnessValue = 0.6
                albedoColor = "#ffffff"; emissiveColor = "#ff4d00"
                break
            case "ksSkin":
                Modeler.setMaterialMetallic(0.0)
                Modeler.setMaterialRoughness(0.7)
                Modeler.setMaterialAlbedo(0.92, 0.75, 0.62, 1.0)
                metallicValue = 0.0; roughnessValue = 0.7
                albedoColor = "#ebbf9e"
                break
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 32
            color: "#252526"
            Layout.fillWidth: true

            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "MATERIAL EDITOR"
                color: "#E10600"
                font.pixelSize: 11
                font.bold: true
            }

            Rectangle {
                anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                width: 18; height: 18; radius: 2; color: "#E10600"
                Text { anchors.centerIn: parent; text: "X"; color: "#fff"; font.pixelSize: 10; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: closeRequested() }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 4

                Rectangle {
                    id: dragChip
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.topMargin: 4
                    height: 32
                    radius: 4
                    color: dragChipColor
                    border.color: "#666"
                    border.width: 1

                    Drag.active: dragChipArea.drag.active
                    Drag.supportedActions: Qt.CopyAction
                    Drag.mimeData: {
                        "application/x-ksmodeler-material":
                            dragChipColor.r + "|" + dragChipColor.g + "|" + dragChipColor.b + "|" +
                            dragChipMetallic + "|" + dragChipRoughness + "|" + dragChipOpacity
                    }
                    Drag.hotSpot: Qt.point(dragChip.width / 2, dragChip.height / 2)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 6

                        Text {
                            text: "DRAG TO VIEWPORT"
                            color: "white"
                            font.pixelSize: 8
                            font.bold: true
                            style: Text.Outline
                            styleColor: "#000000"
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: "\u2192"
                            color: "white"
                            font.pixelSize: 12
                            font.bold: true
                            style: Text.Outline
                            styleColor: "#000000"
                        }
                    }

                    MouseArea {
                        id: dragChipArea
                        anchors.fill: parent
                        drag.target: parent
                        onReleased: dragChip.Drag.drop()
                    }
                }

                Text {
                    text: "MATERIAL LIST"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                    topPadding: 6
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 22
                    color: "#252526"
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 4

                        TextField {
                            id: materialNameField
                            text: currentMaterialName
                            Layout.fillWidth: true
                            height: 20
                            font.pixelSize: 10
                            color: "#ffffff"
                            placeholderText: "Material name"
                            onEditingFinished: {
                                currentMaterialName = text
                                if (Modeler && currentMaterialIndex >= 0)
                                    Modeler.createMaterial(text)
                            }
                        }

                        AppButton {
                            text: "+"
                            width: 22
                            height: 20
                            bgcolor: "#E10600"
                            color: "#121212"
                            font.bold: true
                            font.pixelSize: 12
                            onClicked: {
                                currentMaterialIndex = (currentMaterialIndex < 0 ? 0 : currentMaterialIndex + 1)
                                currentMaterialName = "Material_" + currentMaterialIndex
                                materialNameField.text = currentMaterialName
                                if (Modeler) Modeler.createMaterial(currentMaterialName)
                            }
                        }
                    }
                }

                Rectangle { height: 6; color: "transparent" }

                Text {
                    text: "AC MATERIAL PRESETS"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                Flow {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 4

                AppButton { height: 24; text: "ksPBR"; bgcolor: "#E10600"; color: "#121212"; font.pixelSize: 10
                    onClicked: applyMaterialPreset("ksPBR") }
                    AppButton { height: 24; text: "ksDrude"; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10
                        onClicked: applyMaterialPreset("ksDrude") }
                    AppButton { height: 24; text: "ksGlass"; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10
                        onClicked: applyMaterialPreset("ksGlass") }
                    AppButton { height: 24; text: "ksEmissive"; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10
                        onClicked: applyMaterialPreset("ksEmissive") }
                    AppButton { height: 24; text: "ksSkin"; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10
                        onClicked: applyMaterialPreset("ksSkin") }
                }

                Rectangle { height: 8; color: "transparent" }

                Text {
                    text: "ALBEDO"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    height: 28
                    color: "#252526"
                    radius: 3

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 8

                        Rectangle {
                            width: 22
                            height: 22
                            radius: 3
                            color: albedoColor
                            border.color: "#555"
                            border.width: 1

                            MouseArea {
                                anchors.fill: parent
                                onClicked: albedoPicker.open()
                            }
                        }

                        ColorDialog {
                            id: albedoPicker
                            title: "Select Albedo Color"
                            selectedColor: albedoColor
                            onAccepted: {
                                albedoColor = selectedColor
                                if (Modeler && hasSelection)
                                    Modeler.setMaterialAlbedo(
                                        selectedColor.r, selectedColor.g,
                                        selectedColor.b, 1.0)
                            }
                        }

                        Text {
                            text: albedoColor.toString().substring(0, 7)
                            color: "#888"
                            font.pixelSize: 9
                            font.family: "monospace"
                        }

                        Item { Layout.fillWidth: true }

                        AppButton {
                            text: "Pick"
                            height: 20
                            width: 40
                            bgcolor: "#3e3e42"
                            color: "#ffffff"
                            font.pixelSize: 9
                            onClicked: albedoPicker.open()
                        }
                    }
                }

                Rectangle { height: 4; color: "transparent" }

                Text {
                    text: "METALLIC"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 8

                    Slider {
                        id: metallicSlider
                        Layout.fillWidth: true
                        height: 18
                        from: 0.0
                        to: 1.0
                        value: metallicValue
                        stepSize: 0.01

                        onMoved: {
                            metallicValue = value
                            if (Modeler && hasSelection)
                                Modeler.setMaterialMetallic(value)
                        }
                    }

                    Text {
                        text: metallicValue.toFixed(2)
                        color: "#E10600"
                        font.pixelSize: 10
                        font.bold: true
                        width: 35
                        horizontalAlignment: Text.AlignRight
                    }
                }

                Rectangle { height: 4; color: "transparent" }

                Text {
                    text: "ROUGHNESS"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 8

                    Slider {
                        id: roughnessSlider
                        Layout.fillWidth: true
                        height: 18
                        from: 0.0
                        to: 1.0
                        value: roughnessValue
                        stepSize: 0.01

                        onMoved: {
                            roughnessValue = value
                            if (Modeler && hasSelection)
                                Modeler.setMaterialRoughness(value)
                        }
                    }

                    Text {
                        text: roughnessValue.toFixed(2)
                        color: "#E10600"
                        font.pixelSize: 10
                        font.bold: true
                        width: 35
                        horizontalAlignment: Text.AlignRight
                    }
                }

                Rectangle { height: 4; color: "transparent" }

                Text {
                    text: "NORMAL STRENGTH"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 8

                    Slider {
                        id: normalSlider
                        Layout.fillWidth: true
                        height: 18
                        from: 0.0
                        to: 5.0
                        value: normalStrengthValue
                        stepSize: 0.01

                        onMoved: {
                            normalStrengthValue = value
                            if (Modeler && hasSelection)
                                Modeler.setMaterialNormalStrength(value)
                        }
                    }

                    Text {
                        text: normalStrengthValue.toFixed(2)
                        color: "#E10600"
                        font.pixelSize: 10
                        font.bold: true
                        width: 35
                        horizontalAlignment: Text.AlignRight
                    }
                }

                Rectangle { height: 4; color: "transparent" }

                Text {
                    text: "EMISSIVE"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    height: 28
                    color: "#252526"
                    radius: 3

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 8

                        Rectangle {
                            width: 22
                            height: 22
                            radius: 3
                            color: emissiveColor
                            border.color: "#555"
                            border.width: 1

                            MouseArea {
                                anchors.fill: parent
                                onClicked: emissivePicker.open()
                            }
                        }

                        ColorDialog {
                            id: emissivePicker
                            title: "Select Emissive Color"
                            selectedColor: emissiveColor
                            onAccepted: {
                                emissiveColor = selectedColor
                                if (Modeler && hasSelection)
                                    Modeler.setMaterialEmissive(
                                        selectedColor.r * 2.0,
                                        selectedColor.g * 2.0,
                                        selectedColor.b * 2.0)
                            }
                        }

                        Text {
                            text: emissiveColor.toString().substring(0, 7)
                            color: "#888"
                            font.pixelSize: 9
                            font.family: "monospace"
                        }

                        Item { Layout.fillWidth: true }

                        AppButton {
                            text: "None"
                            height: 20
                            width: 40
                            bgcolor: "#3e3e42"
                            color: "#ffffff"
                            font.pixelSize: 9
                            onClicked: {
                                emissiveColor = "#000000"
                                if (Modeler && hasSelection)
                                    Modeler.setMaterialEmissive(0, 0, 0)
                            }
                        }
                    }
                }

                Rectangle { height: 4; color: "transparent" }

                Text {
                    text: "OPACITY"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    spacing: 8

                    Slider {
                        id: opacitySlider
                        Layout.fillWidth: true
                        height: 18
                        from: 0.0
                        to: 1.0
                        value: opacityValue
                        stepSize: 0.01

                        onMoved: {
                            opacityValue = value
                            if (Modeler && hasSelection)
                                Modeler.setMaterialOpacity(value)
                        }
                    }

                    Text {
                        text: (opacityValue * 100).toFixed(0) + "%"
                        color: "#E10600"
                        font.pixelSize: 10
                        font.bold: true
                        width: 35
                        horizontalAlignment: Text.AlignRight
                    }
                }

                Rectangle { height: 10; color: "transparent" }

                Text {
                    text: "SHADER"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.bottomMargin: 4
                    height: 60
                    color: "#252526"
                    radius: 3

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 3

                        Text {
                            text: "Vertex Shader:"
                            color: "#888"
                            font.pixelSize: 8
                        }

                        TextField {
                            id: vertexShaderField
                            Layout.fillWidth: true
                            height: 18
                            font.pixelSize: 9
                            color: "#ccc"
                            placeholderText: "ksMain_vs.hlsl"
                            onTextChanged: {
                                if (text.length > 0) {
                                    KSModelerQml.createShader(text, fragmentShaderField.text || "ksPerPixel_ps.hlsl")
                                }
                            }
                        }

                        Text {
                            text: "Fragment Shader:"
                            color: "#888"
                            font.pixelSize: 8
                        }

                        TextField {
                            id: fragmentShaderField
                            Layout.fillWidth: true
                            height: 18
                            font.pixelSize: 9
                            color: "#ccc"
                            placeholderText: "ksPerPixel_ps.hlsl"
                            onTextChanged: {
                                if (text.length > 0) {
                                    KSModelerQml.createShader(vertexShaderField.text || "ksMain_vs.hlsl", text)
                                }
                            }
                        }
                    }
                }

                Rectangle { height: 4; color: "transparent" }

                Text {
                    text: "TEXTURE SLOTS"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 10
                }

                Repeater {
                    model: [
                        { label: "Albedo",   placeholder: "AlbedoMap.dds" },
                        { label: "Normal",   placeholder: "NormalMap.dds" },
                        { label: "Roughness", placeholder: "RoughnessMap.dds" },
                        { label: "Metalness", placeholder: "MetalnessMap.dds" }
                    ]

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 10; Layout.rightMargin: 10
                        height: 22
                        color: "#252526"
                        radius: 3

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 3
                            spacing: 4

                            Text {
                                text: modelData.label
                                color: "#888"
                                font.pixelSize: 8
                                width: 55
                            }

                            TextField {
                                id: textureField
                                Layout.fillWidth: true
                                height: 18
                                font.pixelSize: 9
                                color: "#ccc"
                                placeholderText: modelData.placeholder
                                property string slotName: modelData.label
                                onTextChanged: {
                                    if (text.length > 0) {
                                        KSModelerQml.setTexture(slotName.toLowerCase(), text)
                                    }
                                }
                            }

                            AppButton {
                                text: "..."
                                width: 20; height: 18
                                font.pixelSize: 8
                                bgcolor: "#3e3e42"
                                color: "#ffffff"
                                onClicked: {
                                    textureFileDialog.selectedSlot = modelData.label
                                    textureFileDialog.selectedTextField = textureField
                                    textureFileDialog.open()
                                }
                            }
                        }
                    }
                }

                FileDialog {
                    id: textureFileDialog
                    property string selectedSlot: ""
                    title: "Select " + selectedSlot + " Texture"
                    nameFilters: ["Image files (*.dds *.png *.jpg *.tga *.bmp)", "All files (*)"]
                    onAccepted: {
                        console.log(selectedSlot + ":", file)
                    }
                }

                Rectangle { height: 6; color: "transparent" }
            }
        }
    }
}


