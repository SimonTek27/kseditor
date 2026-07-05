import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import KsComponents

Rectangle {
    id: root
    color: Theme.backgroundColor

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            Label {
                text: "Engine Sound Recording Studio"
                font.pixelSize: 18
                font.bold: true
                color: Theme.textColor
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Connect to EngineSim"
                enabled: !recorder.connected
                onClicked: recorder.connect()
            }

            Button {
                text: "Disconnect"
                enabled: recorder.connected
                onClicked: recorder.disconnect()
            }
        }

        // Connection Status
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: recorder.connected ? "#1a3d1a" : "#3d1a1a"
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8

                CircleIndicator {
                    color: recorder.connected ? "#4CAF50" : "#F44336"
                    size: 12
                }

                Label {
                    text: recorder.connected ? "Connected to Engine Simulator" : "Not Connected"
                    color: "white"
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: "RPM: " + recorder.currentRPM.toFixed(0)
                    color: "white"
                    font.bold: true
                }
            }
        }

        // Main Content
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Left Panel - Configuration
            ColumnLayout {
                Layout.preferredWidth: 280
                Layout.fillHeight: true

                GroupBox {
                    title: "Recording Settings"
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            Label { text: "Sample Rate:" }
                            ComboBox {
                                id: sampleRateCombo
                                model: ["44100 Hz", "48000 Hz"]
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Label { text: "Channels:" }
                            ComboBox {
                                id: channelsCombo
                                model: ["Mono", "Stereo"]
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Label { text: "Load Type:" }
                            ComboBox {
                                id: loadTypeCombo
                                model: ["On-Load", "Off-Load"]
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Label { text: "Hold Duration (ms):" }
                            SpinBox {
                                id: holdDurationSpin
                                from: 1000
                                to: 10000
                                value: 3000
                                stepSize: 500
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Label { text: "Output Directory:" }
                            TextField {
                                id: outputDirField
                                placeholderText: "Select output folder"
                                text: recorder.outputDirectory
                                Layout.fillWidth: true
                            }
                            Button {
                                text: "..."
                                implicitWidth: 40
                                onClicked: {
                                    // Open folder picker dialog
                                    folderDialog.open()
                                }
                            }
                        }

                        RowLayout {
                            Label { text: "Sample Prefix:" }
                            TextField {
                                id: samplePrefixField
                                placeholderText: "engine"
                                text: recorder.samplePrefix
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                GroupBox {
                    title: "RPM Profile"
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        RowLayout {
                            ComboBox {
                                id: profileCombo
                                model: profileModel
                                Layout.fillWidth: true
                                onCurrentIndexChanged: recorder.loadProfile(profileModel.get(currentIndex))
                            }

                            Button {
                                text: "New"
                                onClicked: createProfileDialog.open()
                            }

                            Button {
                                text: "Save"
                                onClicked: recorder.saveCurrentProfile()
                            }
                        }

                        ListView {
                            id: rpmListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model: rpmPointsModel
                            clip: true

                            delegate: RowLayout {
                                width: parent ? parent.width : 0
                                height: 30

                                CheckBox {
                                    checked: model.skip
                                    onToggled: model.skip = checked
                                }

                                Label {
                                    text: model.rpm + " RPM"
                                    Layout.fillWidth: true
                                }

                                SpinBox {
                                    value: model.duration
                                    from: 500
                                    to: 10000
                                    stepSize: 500
                                    Layout.preferredWidth: 80
                                    onValueChanged: model.duration = value
                                }

                                Button {
                                    text: "×"
                                    implicitWidth: 30
                                    onClicked: rpmPointsModel.remove(index)
                                }
                            }
                        }

                        Button {
                            text: "Add RPM Point"
                            onClicked: addRPMDialog.open()
                        }
                    }
                }
            }

            // Center Panel - Recording Control
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // RPM Gauge
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    color: Theme.panelColor
                    radius: 8

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 16

                        Gauge {
                            id: rpmGauge
                            value: recorder.currentRPM
                            minValue: 0
                            maxValue: 12000
                            label: "RPM"
                            Layout.preferredSize: 120
                        }

                        ColumnLayout {
                            Layout.fillWidth: true

                            ProgressBar {
                                id: recordingProgress
                                Layout.fillWidth: true
                                value: recorder.recordingProgress
                            }

                            Label {
                                text: "Recording: " + recorder.currentRPMIndex + " / " + recorder.totalRPMPoints
                                color: Theme.textColor
                            }

                            Item { Layout.fillHeight: true }

                            RowLayout {
                                spacing: 8

                                Button {
                                    id: recordButton
                                    text: recorder.isRecording ? "Stop Recording" : "Start Recording"
                                    enabled: recorder.connected && !recorder.isRecording
                                    palette.button: recorder.isRecording ? "#f44336" : "#4CAF50"
                                    onClicked: {
                                        if (recorder.isRecording) {
                                            recorder.stopRecording()
                                        } else {
                                            recorder.startRecording()
                                        }
                                    }
                                }

                                Button {
                                    text: "Emergency Stop"
                                    enabled: recorder.isRecording
                                    palette.button: "#F44336"
                                    onClicked: recorder.emergencyStop()
                                }
                            }
                        }
                    }
                }

                // Recording Log
                GroupBox {
                    title: "Recording Log"
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: logListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: recordingLogModel
                        clip: true

                        delegate: Rectangle {
                            width: parent ? parent.width : 0
                            height: 24
                            color: index % 2 === 0 ? Theme.backgroundColor : Theme.panelColor

                            Label {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                text: model.time + " - " + model.message
                                color: model.type === "error" ? "#F44336" :
                                      model.type === "success" ? "#4CAF50" : Theme.textColor
                                font.pixelSize: 12
                            }
                        }
                    }
                }
            }

            // Right Panel - Processing
            ColumnLayout {
                Layout.preferredWidth: 280
                Layout.fillHeight: true

                GroupBox {
                    title: "Car Acoustics"
                    Layout.fillWidth: true

                    ColumnLayout {
                        spacing: 8

                        RowLayout {
                            Label { text: "Mode:" }
                            ComboBox {
                                id: modeCombo
                                model: ["Exterior", "Interior"]
                                Layout.fillWidth: true
                                onCurrentIndexChanged: acoustics.mode = currentIndex
                            }
                        }

                        RowLayout {
                            Label { text: "Car Type:" }
                            ComboBox {
                                id: carTypeCombo
                                model: ["Sedan", "Coupe", "SUV", "Convertible", "Race Car", "Truck"]
                                Layout.fillWidth: true
                                onCurrentIndexChanged: acoustics.carType = currentIndex
                            }
                        }

                        StackLayout {
                            id: presetStack
                            Layout.fillWidth: true

                            RowLayout {
                                Label { text: "Exterior Preset:" }
                                ComboBox {
                                    id: exteriorPresetCombo
                                    model: ["Raw", "Sport", "Race", "Classic", "Modern"]
                                    Layout.fillWidth: true
                                    onCurrentIndexChanged: acoustics.exteriorPreset = currentIndex
                                }
                            }

                            RowLayout {
                                Label { text: "Interior Preset:" }
                                ComboBox {
                                    id: interiorPresetCombo
                                    model: ["Stock", "Sport", "Racing", "Open", "Luxury"]
                                    Layout.fillWidth: true
                                    onCurrentIndexChanged: acoustics.interiorPreset = currentIndex
                                }
                            }
                        }

                        Button {
                            text: "Process Samples"
                            onClicked: acoustics.processSelectedSamples()
                        }
                    }
                }

                GroupBox {
                    title: "Bank Generator"
                    Layout.fillWidth: true

                    ColumnLayout {
                        spacing: 8

                        RowLayout {
                            Label { text: "Car Name:" }
                            TextField {
                                id: carNameField
                                placeholderText: "my_car"
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Label { text: "Project Path:" }
                            TextField {
                                id: bankPathField
                                placeholderText: "Select project folder"
                                Layout.fillWidth: true
                            }
                            Button {
                                text: "..."
                                implicitWidth: 40
                                onClicked: bankFolderDialog.open()
                            }
                        }

                        RowLayout {
                            Label { text: "Mode:" }
                            ComboBox {
                                id: generationModeCombo
                                model: ["Use Existing Template", "From Scratch"]
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            CheckBox {
                                id: includeInteriorCheck
                                text: "Interior"
                                checked: true
                            }
                            CheckBox {
                                id: includeExteriorCheck
                                text: "Exterior"
                                checked: true
                            }
                            CheckBox {
                                id: includeTurboCheck
                                text: "Turbo"
                            }
                        }

                        Button {
                            text: "Generate Script"
                            onClicked: fmodGenerator.generate()
                        }

                        Button {
                            text: "Open in Audio Editor"
                            enabled: fmodGenerator.scriptGenerated
                            onClicked: fmodGenerator.openInFMOD()
                        }
                    }
                }
            }
        }

        // Status Bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: Theme.panelColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8

                Label {
                    text: "Status: " + statusText
                    color: Theme.textColor
                    font.pixelSize: 12
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: "Samples Recorded: " + recorder.samplesRecorded
                    color: Theme.textColor
                    font.pixelSize: 12
                }
            }
        }
    }

    // Dialogs
    FolderDialog {
        id: folderDialog
        onAccepted: {
            outputDirField.text = folderDialog.folder
            recorder.outputDirectory = folderDialog.folder
        }
    }

    FolderDialog {
        id: bankFolderDialog
        onAccepted: {
            bankPathField.text = bankFolderDialog.folder
            fmodGenerator.fmodProjectPath = bankFolderDialog.folder
        }
    }

    Dialog {
        id: addRPMDialog
        title: "Add RPM Point"
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            RowLayout {
                Label { text: "RPM:" }
                SpinBox {
                    id: newRPMSpin
                    from: 500
                    to: 15000
                    value: 3000
                }
            }
            RowLayout {
                Label { text: "Duration (ms):" }
                SpinBox {
                    id: newDurationSpin
                    from: 500
                    to: 10000
                    value: 3000
                }
            }
        }

        onAccepted: {
            rpmPointsModel.append({ rpm: newRPMSpin.value, duration: newDurationSpin.value, skip: false })
        }
    }

    Dialog {
        id: createProfileDialog
        title: "Create New Profile"
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            RowLayout {
                Label { text: "Profile Name:" }
                TextField {
                    id: profileNameField
                    placeholderText: "My Engine Profile"
                }
            }
            RowLayout {
                Label { text: "Engine Type:" }
                ComboBox {
                    id: engineTypeCombo
                    model: ["Economy 4-cyl", "Sport 6-cyl", "V8", "V10", "V12", "Flat-6", "Rotary", "Race Car"]
                }
            }
        }

        onAccepted: {
            var types = ["economy4", "sport6", "v8", "v10", "v12", "flat6", "rotary", "race"]
            recorder.createProfile(profileNameField.text, types[engineTypeCombo.currentIndex])
        }
    }

    // Models
    ListModel {
        id: profileModel
        ListElement { name: "Economy 4-cyl"; type: "economy4" }
        ListElement { name: "Sport 6-cyl"; type: "sport6" }
        ListElement { name: "V8"; type: "v8" }
        ListElement { name: "V10"; type: "v10" }
        ListElement { name: "V12"; type: "v12" }
        ListElement { name: "Flat-6"; type: "flat6" }
        ListElement { name: "Rotary"; type: "rotary" }
        ListElement { name: "Race Car"; type: "race" }
    }

    ListModel {
        id: rpmPointsModel
    }

    ListModel {
        id: recordingLogModel
    }

    // Signal bindings
    Connections {
        target: recorder
        onConnectedChanged: {
            statusText = recorder.connected ? "Connected" : "Disconnected"
        }
        onRpmReached: {
            recordingLogModel.append({
                time: new Date().toLocaleTimeString(),
                message: "Reached " + rpm + " RPM",
                type: "info"
            })
        }
        onSampleRecorded: {
            recordingLogModel.append({
                time: new Date().toLocaleTimeString(),
                message: "Recorded: " + filePath,
                type: "success"
            })
        }
        onRecordingCompleted: {
            statusText = "Recording completed!"
            recordingLogModel.append({
                time: new Date().toLocaleTimeString(),
                message: "Recording session completed!",
                type: "success"
            })
        }
        onError: {
            recordingLogModel.append({
                time: new Date().toLocaleTimeString(),
                message: "Error: " + error,
                type: "error"
            })
        }
    }

    Connections {
        target: fmodGenerator
        onGenerationCompleted: {
            statusText = "Script generated!"
        }
        onGenerationFailed: {
            statusText = "Generation failed"
        }
    }

    property string statusText: "Ready"
}
