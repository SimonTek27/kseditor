import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import ksEditor.Audio 1.0

Rectangle {
    id: root
    color: "#121212"

    readonly property color cBg:       "#121212"
    readonly property color cPanel:    "#1e1e1e"
    readonly property color cBorder:   "#333333"
    readonly property color cAccent:   "#E10600"
    readonly property color cText:     "#cccccc"
    readonly property color cMuted:    "#666666"
    readonly property color cGreen:    "#4CAF50"
    readonly property color cRed:      "#F44336"

    // Local recorder proxy wrapping AudioBridge
    property QtObject recorder: QtObject {
        readonly property bool connected: true
        readonly property bool isRecording: AudioBridge ? AudioBridge.isRecording : false
        readonly property real currentRPM: 0
        readonly property int currentRPMIndex: 0
        readonly property int totalRPMPoints: rpmPointsModel.count
        readonly property real recordingProgress: 0
        readonly property int samplesRecorded: 0
        property string outputDirectory: ""
        property string samplePrefix: "engine"
        property bool recordStarted: false

        function connect() { console.log("EngineSim connect (simulated)") }
        function disconnect() { console.log("EngineSim disconnect") }
        function startRecording() {
            if (AudioBridge) AudioBridge.startRecording(outputDirectory + "/" + samplePrefix + "_sample.wav")
            recordStarted = true
        }
        function stopRecording() {
            if (AudioBridge) AudioBridge.stopRecording()
            recordStarted = false
        }
        function emergencyStop() { stopRecording() }
        function loadProfile(profile) { console.log("Load profile:", profile) }
        function saveCurrentProfile() { console.log("Save profile") }
        function createProfile(name, type) { console.log("Create profile:", name, type) }
    }

    property string statusText: "Ready"

    // Theme substitute
    readonly property QtObject Theme: QtObject {
        readonly property color backgroundColor: cBg
        readonly property color panelColor: cPanel
        readonly property color textColor: cText
        readonly property color accentColor: cAccent
    }

    // Inline CircleIndicator
    component CircleIndicator : Rectangle {
        property color indicatorColor: cGreen
        property int indicatorSize: 12
        width: indicatorSize; height: indicatorSize; radius: indicatorSize / 2
        color: indicatorColor
    }

    // Inline Gauge
    component Gauge : Rectangle {
        property real value: 0
        property real minValue: 0
        property real maxValue: 100
        property string label: ""
        property int preferredSize: 120
        width: preferredSize; height: preferredSize; radius: 8; color: cPanel; border.color: cBorder; border.width: 1
        Canvas {
            anchors.fill: parent; anchors.margins: 4
            onPaint: {
                var ctx = getContext("2d")
                var cx = width / 2, cy = height / 2, r = Math.min(cx, cy) - 4
                ctx.clearRect(0, 0, width, height)
                var frac = (value - minValue) / (maxValue - minValue || 1)
                ctx.strokeStyle = "#333"; ctx.lineWidth = 6
                ctx.beginPath(); ctx.arc(cx, cy, r, 0.75 * Math.PI, 2.25 * Math.PI); ctx.stroke()
                ctx.strokeStyle = cAccent; ctx.lineWidth = 6
                ctx.beginPath(); ctx.arc(cx, cy, r, 0.75 * Math.PI, 0.75 * Math.PI + frac * 1.5 * Math.PI); ctx.stroke()
                ctx.fillStyle = cText; ctx.font = "bold 18px monospace"
                ctx.textAlign = "center"; ctx.fillText(Math.round(value).toString(), cx, cy + 6)
                ctx.fillStyle = cMuted; ctx.font = "10px monospace"
                ctx.fillText(label, cx, cy - r - 8)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent; spacing: 8; anchors.margins: 8

        // Header
        RowLayout {
            Layout.fillWidth: true; Layout.preferredHeight: 40
            Label { text: "Engine Sound Recording Studio"; font.pixelSize: 16; font.bold: true; color: cText }
            Item { Layout.fillWidth: true }
            Button { text: recorder.connected ? "Disconnect" : "Connect to EngineSim"
                onClicked: recorder.connected ? recorder.disconnect() : recorder.connect() }
        }

        // Connection Status
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 36
            color: recorder.connected ? "#1a3d1a" : "#3d1a1a"; radius: 4
            RowLayout { anchors.fill: parent; anchors.margins: 8
                CircleIndicator { indicatorColor: recorder.connected ? cGreen : cRed }
                Label { text: recorder.connected ? "Connected to Engine Simulator" : "Not Connected"; color: "white" }
                Item { Layout.fillWidth: true }
                Label { text: "RPM: " + recorder.currentRPM.toFixed(0); color: "white"; font.bold: true }
            }
        }

        // Main Content
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8

            // Left Panel - Configuration
            ColumnLayout {
                Layout.preferredWidth: 260; Layout.fillHeight: true; spacing: 8

                GroupBox { title: "Recording Settings"; Layout.fillWidth: true
                    ColumnLayout { anchors.fill: parent; spacing: 6
                        RowLayout { Label { text: "Sample Rate:"; color: cMuted }; ComboBox { id: sampleRateCombo; model: ["44100 Hz", "48000 Hz"]; Layout.fillWidth: true } }
                        RowLayout { Label { text: "Channels:"; color: cMuted }; ComboBox { id: channelsCombo; model: ["Mono", "Stereo"]; Layout.fillWidth: true } }
                        RowLayout { Label { text: "Load Type:"; color: cMuted }; ComboBox { id: loadTypeCombo; model: ["On-Load", "Off-Load"]; Layout.fillWidth: true } }
                        RowLayout { Label { text: "Hold (ms):"; color: cMuted }; SpinBox { id: holdDurationSpin; from: 1000; to: 10000; value: 3000; stepSize: 500; Layout.fillWidth: true } }
                        RowLayout { Label { text: "Output Dir:"; color: cMuted }; TextField { id: outputDirField; placeholderText: "Select output folder"; text: recorder.outputDirectory; Layout.fillWidth: true }
                            Button { text: "..."; implicitWidth: 36; onClicked: folderDialog.open() } }
                        RowLayout { Label { text: "Prefix:"; color: cMuted }; TextField { id: samplePrefixField; placeholderText: "engine"; text: recorder.samplePrefix; Layout.fillWidth: true } }
                    }
                }

                GroupBox { title: "RPM Profile"; Layout.fillWidth: true; Layout.fillHeight: true
                    ColumnLayout { anchors.fill: parent; spacing: 4
                        RowLayout { ComboBox { id: profileCombo; model: profileModel; Layout.fillWidth: true; onCurrentIndexChanged: recorder.loadProfile(profileModel.get(currentIndex)) }
                            Button { text: "New"; onClicked: createProfileDialog.open() }
                            Button { text: "Save"; onClicked: recorder.saveCurrentProfile() } }
                        ListView { id: rpmListView; Layout.fillWidth: true; Layout.fillHeight: true; model: rpmPointsModel; clip: true
                            delegate: RowLayout { width: parent ? parent.width : 0; height: 28
                                CheckBox { checked: model.skip; onToggled: model.skip = checked }
                                Label { text: model.rpm + " RPM"; Layout.fillWidth: true; color: cText }
                                SpinBox { value: model.duration; from: 500; to: 10000; stepSize: 500; Layout.preferredWidth: 70
                                    onValueChanged: model.duration = value }
                                Button { text: "\u2715"; implicitWidth: 28; onClicked: rpmPointsModel.remove(index) } } }
                        Button { text: "Add RPM Point"; onClicked: addRPMDialog.open() }
                    }
                }
            }

            // Center Panel - Recording Control
            ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8

                // RPM Gauge
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 140; color: cPanel; radius: 8; border.color: cBorder; border.width: 1
                    RowLayout { anchors.fill: parent; anchors.margins: 12
                        Gauge { id: rpmGauge; value: recorder.currentRPM; minValue: 0; maxValue: 12000; label: "RPM" }
                        ColumnLayout { Layout.fillWidth: true
                            ProgressBar { id: recordingProgress; Layout.fillWidth: true
                                value: recorder.isRecording ? 0.5 : 0 }
                            Label { text: "Recording: " + recorder.currentRPMIndex + " / " + recorder.totalRPMPoints; color: cText }
                            Item { Layout.fillHeight: true }
                            RowLayout { spacing: 8
                                Button { id: recordButton
                                    text: recorder.isRecording ? "Stop Recording" : "Start Recording"
                                    enabled: !recorder.isRecording
                                    contentItem: Text { text: parent.text; color: "white"; font.bold: true }
                                    background: Rectangle { color: recorder.isRecording ? cRed : cGreen; radius: 4 }
                                    onClicked: { recorder.isRecording ? recorder.stopRecording() : recorder.startRecording() } }
                                Button { text: "Emergency Stop"; enabled: recorder.isRecording
                                    contentItem: Text { text: parent.text; color: "white" }
                                    background: Rectangle { color: cRed; radius: 4 }
                                    onClicked: recorder.emergencyStop() } } } }
                }

                // Recording Log
                GroupBox { title: "Recording Log"; Layout.fillWidth: true; Layout.fillHeight: true
                    ListView { id: logListView; Layout.fillWidth: true; Layout.fillHeight: true; model: recordingLogModel; clip: true
                        delegate: Rectangle { width: parent ? parent.width : 0; height: 22
                            color: index % 2 === 0 ? cBg : cPanel
                            Label { anchors.fill: parent; anchors.leftMargin: 8
                                text: model.time + " - " + model.message
                                color: model.type === "error" ? cRed : model.type === "success" ? cGreen : cText; font.pixelSize: 11 } } }
                }
            }

            // Right Panel - Processing
            ColumnLayout { Layout.preferredWidth: 260; Layout.fillHeight: true; spacing: 8

                GroupBox { title: "Car Acoustics"; Layout.fillWidth: true
                    ColumnLayout { spacing: 6
                        RowLayout { Label { text: "Mode:"; color: cMuted }
                            ComboBox { id: modeCombo; model: ["Exterior", "Interior"]; Layout.fillWidth: true } }
                        RowLayout { Label { text: "Car Type:"; color: cMuted }
                            ComboBox { id: carTypeCombo; model: ["Sedan", "Coupe", "SUV", "Convertible", "Race Car", "Truck"]; Layout.fillWidth: true } }
                        RowLayout { Label { text: modeCombo.currentIndex === 0 ? "Ext Preset:" : "Int Preset:"; color: cMuted }
                            ComboBox { id: presetCombo; model: modeCombo.currentIndex === 0 ? ["Raw", "Sport", "Race", "Classic", "Modern"] : ["Stock", "Sport", "Racing", "Open", "Luxury"]; Layout.fillWidth: true } }
                        Button { text: "Process Samples"
                            onClicked: { statusText = "Processing... (requires acoustics bridge)"; recordingLogModel.append({time: new Date().toLocaleTimeString(), message: "Acoustics processing requested", type: "info"}) } }
                    }
                }

                GroupBox { title: "Bank Generator"; Layout.fillWidth: true
                    ColumnLayout { spacing: 6
                        RowLayout { Label { text: "Car Name:"; color: cMuted }; TextField { id: carNameField; placeholderText: "my_car"; Layout.fillWidth: true } }
                        RowLayout { Label { text: "Project:"; color: cMuted }; TextField { id: bankPathField; placeholderText: "Select project folder"; Layout.fillWidth: true }
                            Button { text: "..."; implicitWidth: 36; onClicked: bankFolderDialog.open() } }
                        RowLayout { Label { text: "Mode:"; color: cMuted }; ComboBox { id: generationModeCombo; model: ["Use Existing Template", "From Scratch"]; Layout.fillWidth: true } }
                        RowLayout { CheckBox { id: includeInteriorCheck; text: "Interior"; checked: true }
                            CheckBox { id: includeExteriorCheck; text: "Exterior"; checked: true }
                            CheckBox { id: includeTurboCheck; text: "Turbo" } }
                        Button { text: "Generate Script"
                            onClicked: { statusText = "FMOD generation requested (requires generator bridge)"; recordingLogModel.append({time: new Date().toLocaleTimeString(), message: "Bank generation requested: " + carNameField.text, type: "info"}) } }
                        Button { text: "Open in Audio Editor"
                            onClicked: { statusText = "Opening in Audio Editor... (requires generator bridge)" } }
                    }
                }
            }
        }

        // Status Bar
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 24; color: cPanel
            RowLayout { anchors.fill: parent; anchors.leftMargin: 8
                Label { text: "Status: " + statusText; color: cText; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                Label { text: "Samples Recorded: " + recorder.samplesRecorded; color: cText; font.pixelSize: 11 } }
        }
    }

    FolderDialog { id: folderDialog; onAccepted: { outputDirField.text = selectedFolder.toString().replace("file:///", ""); recorder.outputDirectory = outputDirField.text } }
    FolderDialog { id: bankFolderDialog; onAccepted: { bankPathField.text = selectedFolder.toString().replace("file:///", "") } }

    Dialog { id: addRPMDialog; title: "Add RPM Point"; standardButtons: Dialog.Ok | Dialog.Cancel
        ColumnLayout {
            RowLayout { Label { text: "RPM:"; color: cMuted }; SpinBox { id: newRPMSpin; from: 500; to: 15000; value: 3000 } }
            RowLayout { Label { text: "Duration (ms):"; color: cMuted }; SpinBox { id: newDurationSpin; from: 500; to: 10000; value: 3000 } }
        }
        onAccepted: rpmPointsModel.append({ rpm: newRPMSpin.value, duration: newDurationSpin.value, skip: false })
    }

    Dialog { id: createProfileDialog; title: "Create New Profile"; standardButtons: Dialog.Ok | Dialog.Cancel
        ColumnLayout {
            RowLayout { Label { text: "Profile Name:"; color: cMuted }; TextField { id: profileNameField; placeholderText: "My Engine Profile" } }
            RowLayout { Label { text: "Engine Type:"; color: cMuted }; ComboBox { id: engineTypeCombo; model: ["Economy 4-cyl", "Sport 6-cyl", "V8", "V10", "V12", "Flat-6", "Rotary", "Race Car"] } }
        }
        onAccepted: recorder.createProfile(profileNameField.text, ["economy4","sport6","v8","v10","v12","flat6","rotary","race"][engineTypeCombo.currentIndex])
    }

    ListModel { id: profileModel
        ListElement { name: "Economy 4-cyl"; type: "economy4" }
        ListElement { name: "Sport 6-cyl"; type: "sport6" }
        ListElement { name: "V8"; type: "v8" }
        ListElement { name: "V10"; type: "v10" }
        ListElement { name: "V12"; type: "v12" }
        ListElement { name: "Flat-6"; type: "flat6" }
        ListElement { name: "Rotary"; type: "rotary" }
        ListElement { name: "Race Car"; type: "race" }
    }

    ListModel { id: rpmPointsModel }
    ListModel { id: recordingLogModel }

    Connections {
        target: recorder
        function onConnectedChanged() { statusText = recorder.connected ? "Connected" : "Disconnected" }
        function onRpmReached(rpm) { recordingLogModel.append({time: new Date().toLocaleTimeString(), message: "Reached " + rpm + " RPM", type: "info"}) }
        function onSampleRecorded(filePath) { recordingLogModel.append({time: new Date().toLocaleTimeString(), message: "Recorded: " + filePath, type: "success"}) }
        function onRecordingCompleted() { statusText = "Recording completed!"; recordingLogModel.append({time: new Date().toLocaleTimeString(), message: "Recording session completed!", type: "success"}) }
        function onError(error) { recordingLogModel.append({time: new Date().toLocaleTimeString(), message: "Error: " + error, type: "error"}) }
    }
}
