import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import ksEditor.FfbEditor 1.0

Rectangle {
    color: "#1a1a1a"

    property string statusText: ""
    property var currentPreset: ({})

    FileDialog {
        id: openDialog
        title: "Load FFB Settings"
        nameFilters: ["INI files (*.ini)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            FfbEditor.loadSettings(path)
            statusText = "Loaded settings from " + path
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save FFB Settings"
        nameFilters: ["INI files (*.ini)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            FfbEditor.saveSettings(path)
            statusText = "Saved settings to " + path
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

                Text { text: "FFB Editor"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                Text {
                    text: FfbEditor.currentFile ? FfbEditor.currentFile.split("/").pop() : "No file loaded"
                    color: "#888"
                    font.pixelSize: 11
                }
                Item { Layout.fillWidth: true }

                Button {
                    text: "Load"
                    flat: true
                    font.pixelSize: 11
                    onClicked: openDialog.open()
                }
                Button {
                    text: "Save"
                    flat: true
                    font.pixelSize: 11
                    onClicked: {
                        if (FfbEditor.currentFile) {
                            FfbEditor.saveSettings("")
                            statusText = "Settings saved"
                        } else {
                            saveDialog.open()
                        }
                    }
                }
                Button {
                    text: "Save As..."
                    flat: true
                    font.pixelSize: 11
                    onClicked: saveDialog.open()
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Rectangle {
                color: "#1e1e1e"
                SplitView.preferredWidth: 350

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 8
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        Rectangle {
                            Layout.fillWidth: true
                            height: presetCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: presetCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Wheel Presets"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                ComboBox {
                                    id: presetCombo
                                    Layout.fillWidth: true
                                    font.pixelSize: 11
                                    model: FfbEditor.getPresets()
                                    textRole: "name"
                                    onCurrentIndexChanged: {
                                        if (currentIndex >= 0) {
                                            var presets = FfbEditor.getPresets()
                                            currentPreset = presets[currentIndex]
                                        }
                                    }
                                }

                                Text {
                                    text: currentPreset.description || "Select a preset for your wheel"
                                    color: "#666"
                                    font.pixelSize: 9
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }

                                RowLayout {
                                    spacing: 4
                                    Button {
                                        text: "Apply Preset"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: {
                                            if (presetCombo.currentIndex >= 0) {
                                                var presets = FfbEditor.getPresets()
                                                FfbEditor.applyPreset(presets[presetCombo.currentIndex].name)
                                                statusText = "Applied preset: " + presets[presetCombo.currentIndex].name
                                            }
                                        }
                                    }
                                    Button {
                                        text: "Save as Custom"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: customPresetDialog.open()
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: ffbCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: ffbCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Force Feedback Settings"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                // Gain
                                RowLayout {
                                    Text { text: "Gain:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: gainSlider
                                        from: 0; to: 100; value: FfbEditor.gain; stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: FfbEditor.gain = value
                                    }
                                    Text { text: Math.round(gainSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }

                                // Filter
                                RowLayout {
                                    Text { text: "Filter:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: filterSlider
                                        from: 0; to: 100; value: FfbEditor.filter; stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: FfbEditor.filter = value
                                    }
                                    Text { text: Math.round(filterSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }

                                // Minimum Force
                                RowLayout {
                                    Text { text: "Min Force:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: minForceSlider
                                        from: 0; to: 100; value: FfbEditor.minimumForce; stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: FfbEditor.minimumForce = value
                                    }
                                    Text { text: Math.round(minForceSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                                // Kerb Effect
                                RowLayout {
                                    Text { text: "Kerb:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: kerbSlider
                                        from: 0; to: 100; value: FfbEditor.kerbEffect; stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: FfbEditor.kerbEffect = value
                                    }
                                    Text { text: Math.round(kerbSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }

                                // Road Effect
                                RowLayout {
                                    Text { text: "Road:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: roadSlider
                                        from: 0; to: 100; value: FfbEditor.roadEffect; stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: FfbEditor.roadEffect = value
                                    }
                                    Text { text: Math.round(roadSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }

                                // Slip Effect
                                RowLayout {
                                    Text { text: "Slip:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: slipSlider
                                        from: 0; to: 100; value: FfbEditor.slipEffect; stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: FfbEditor.slipEffect = value
                                    }
                                    Text { text: Math.round(slipSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }

                                // ABS Effect
                                RowLayout {
                                    Text { text: "ABS:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: absSlider
                                        from: 0; to: 100; value: FfbEditor.absEffect; stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: FfbEditor.absEffect = value
                                    }
                                    Text { text: Math.round(absSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                                // Enhance Understeer
                                RowLayout {
                                    Text { text: "Understeer:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: understeerSlider
                                        from: 0; to: 100; value: FfbEditor.enhUndersteer; stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: FfbEditor.enhUndersteer = value
                                    }
                                    Text { text: Math.round(understeerSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }

                                // Centre Boost Gain
                                RowLayout {
                                    Text { text: "Boost Gain:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: boostGainSlider
                                        from: 0; to: 100; value: FfbEditor.centreBoostGain; stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: FfbEditor.centreBoostGain = value
                                    }
                                    Text { text: Math.round(boostGainSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }

                                // Centre Boost Range
                                RowLayout {
                                    Text { text: "Boost Range:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: boostRangeSlider
                                        from: 0; to: 100; value: FfbEditor.centreBoostRange; stepSize: 1
                                        Layout.fillWidth: true
                                        onMoved: FfbEditor.centreBoostRange = value
                                    }
                                    Text { text: Math.round(boostRangeSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                                // Gyro
                                RowLayout {
                                    Text { text: "Gyro:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Switch {
                                        id: gyroSwitch
                                        checked: FfbEditor.enableGyro
                                        onToggled: FfbEditor.enableGyro = checked
                                    }
                                }

                                // Gyro Strength
                                RowLayout {
                                    Text { text: "Gyro Str:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 80 }
                                    Slider {
                                        id: gyroStrSlider
                                        from: 0; to: 100; value: FfbEditor.gyroStrength; stepSize: 1
                                        Layout.fillWidth: true
                                        enabled: gyroSwitch.checked
                                        onMoved: FfbEditor.gyroStrength = value
                                    }
                                    Text { text: Math.round(gyroStrSlider.value) + "%"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: optCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: optCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Wheel Optimization"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                ComboBox {
                                    id: wheelCombo
                                    Layout.fillWidth: true
                                    font.pixelSize: 11
                                    model: FfbEditor.getSupportedWheels()
                                }

                                RowLayout {
                                    spacing: 4
                                    Button {
                                        text: "Optimize for Wheel"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        onClicked: {
                                            FfbEditor.optimizeForWheel(wheelCombo.currentText)
                                            statusText = "Optimized for: " + wheelCombo.currentText
                                        }
                                    }
                                }

                                Text {
                                    text: "Manufacturer: " + FfbEditor.getWheelManufacturer(wheelCombo.currentText)
                                    color: "#666"
                                    font.pixelSize: 9
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: actionsCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: actionsCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Actions"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                Button {
                                    text: "Reset to Defaults"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        FfbEditor.resetToDefaults()
                                        statusText = "Reset to defaults"
                                    }
                                }

                                Button {
                                    text: "Validate Settings"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var result = FfbEditor.validateSettings()
                                        if (result.valid) {
                                            statusText = "Settings are valid"
                                        } else {
                                            statusText = "Validation error: " + result.error
                                        }
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

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
                                text: statusText || "Ready. Load an FFB config or select a preset."
                                color: "#aaa"
                                font.pixelSize: 11
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Text {
                                text: "\nUnsaved changes: " + (FfbEditor.hasUnsavedChanges ? "Yes" : "No")
                                color: FfbEditor.hasUnsavedChanges ? "#E10600" : "#666"
                                font.pixelSize: 10
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: infoCol.height + 16
                        color: "#1a1a1a"
                        radius: 4

                        ColumnLayout {
                            id: infoCol
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4

                            Text { text: "FFB Settings Guide"; color: "#E10600"; font.pixelSize: 11; font.bold: true }
                            Text { text: "Gain: Overall FFB strength"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Filter: Smoothing filter (lower = more detail)"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Min Force: Minimum force threshold"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Kerb: Road rumble/kerb effects"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Road: Surface texture feedback"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Slip: Tire slip angle feedback"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "ABS: Anti-lock braking feedback"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Understeer: Front loss of grip feedback"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Centre Boost: Force near center of wheel"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Gyro: Gyroscope-based force feedback"; color: "#888"; font.pixelSize: 9 }

                            Item { Layout.fillHeight: true }

                            Text { text: "Keyboard Shortcuts"; color: "#E10600"; font.pixelSize: 10; font.bold: true }
                            Text { text: "Ctrl+O  Load Settings"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "Ctrl+S  Save Settings"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "Ctrl+Shift+S  Save As..."; color: "#666"; font.pixelSize: 8 }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: customPresetDialog
        title: "Save Custom Preset"
        modal: true
        anchors.centerIn: parent
        width: 300

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text { text: "Preset Name:"; color: "#ccc"; font.pixelSize: 11 }
            TextField { id: presetNameField; placeholderText: "My Custom Preset"; Layout.fillWidth: true; font.pixelSize: 11 }

            Text { text: "Wheel Model:"; color: "#ccc"; font.pixelSize: 11 }
            TextField { id: wheelModelField; placeholderText: "T300RS"; Layout.fillWidth: true; font.pixelSize: 11 }

            Text { text: "Manufacturer:"; color: "#ccc"; font.pixelSize: 11 }
            TextField { id: manufacturerField; placeholderText: "Thrustmaster"; Layout.fillWidth: true; font.pixelSize: 11 }

            Text { text: "Description:"; color: "#ccc"; font.pixelSize: 11 }
            TextField { id: descriptionField; placeholderText: "My custom settings"; Layout.fillWidth: true; font.pixelSize: 11 }

            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Button {
                    text: "Cancel"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    onClicked: customPresetDialog.close()
                }
                Button {
                    text: "Save"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    onClicked: {
                        FfbEditor.saveCustomPreset(presetNameField.text, wheelModelField.text, manufacturerField.text, descriptionField.text)
                        statusText = "Saved custom preset: " + presetNameField.text
                        customPresetDialog.close()
                    }
                }
            }
        }
    }

    Connections {
        target: FfbEditor
        function onSettingsLoaded(path) { statusText = "Loaded: " + path }
        function onSettingsSaved(path) { statusText = "Saved: " + path }
        function onPresetApplied(name) { statusText = "Applied preset: " + name }
        function onValidationFailed(error) { statusText = "Error: " + error }
    }

    Shortcut { sequence: "Ctrl+O"; onActivated: openDialog.open(); description: "Load Settings" }
    Shortcut { sequence: "Ctrl+S"; onActivated: {
        if (FfbEditor.currentFile) { FfbEditor.saveSettings(""); statusText = "Settings saved" }
        else { saveDialog.open() }
    }; description: "Save Settings" }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: saveDialog.open(); description: "Save As" }
}
