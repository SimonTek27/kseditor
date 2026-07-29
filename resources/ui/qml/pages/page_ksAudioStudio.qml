import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window 2.15
import Qt.labs.platform 1.1
import ksEditor.Audio 1.0
import ksEditor.AudioEngine 1.0
import ksEditor.AudioEffects 1.0
import ksEditor.AudioModule 1.0
import "../widgets"

Rectangle {
    id: root
    color: "#121212"

    readonly property color cAccent: "#E10600"
    readonly property color cPanel: "#1e1e1e"
    readonly property color cBg: "#0e0e0e"
    readonly property color cBorder: "#333333"
    readonly property color cText: "#ffffff"
    readonly property color cMuted: "#666666"
    readonly property color cWave: "#00e6b8"

    property string currentProject: "untitled"
    property string activePanel: "timeline"
    property bool isPlaying: AudioBridge ? AudioBridge.isPlaying : false
    property bool isRecording: AudioBridge ? AudioBridge.isRecording : false
    property real masterVolume: 80
    property real leftPeakDb: AudioBridge ? AudioBridge.leftPeak : 0
    property real rightPeakDb: AudioBridge ? AudioBridge.rightPeak : 0
    property string selectedEvent: ""
    property string statusMessage: "Ready"
    property bool isModified: false

    property var panels: [
        { key: "timeline", label: "Timeline", icon: "\u23F1" },
        { key: "mixer", label: "Mixer", icon: "\uF001" },
        { key: "waveform", label: "Waveform", icon: "\uF012" },
        { key: "events", label: "Events", icon: "\u266B" },
        { key: "effects", label: "Effects", icon: "\uF0E7" },
        { key: "recording", label: "Recording", icon: "\u25CF" },
        { key: "banks", label: "Sound Banks", icon: "\uF1C0" },
        { key: "batch", label: "Batch", icon: "\uF0AE" },
        { key: "export", label: "Export", icon: "\uF019" }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Menu bar ---
        MenuBar {
            Layout.fillWidth: true

            Menu {
                title: "Project"
                Action { text: "New Project"; shortcut: "Ctrl+N"; onTriggered: AudioModule.newProject() }
                Action { text: "Open Project..."; shortcut: "Ctrl+O"; onTriggered: openProjectDialog.open() }
                Action { text: "Save Project"; shortcut: "Ctrl+S"; onTriggered: saveProject() }
                Action { text: "Save Project As..."; shortcut: "Ctrl+Shift+S"; onTriggered: saveProjectDialog.open() }
                MenuSeparator {}
                Action { text: "Import Audio..."; shortcut: "Ctrl+I"; onTriggered: importDialog.open() }
                Action { text: "Export Mix..."; shortcut: "Ctrl+E"; onTriggered: exportDialog.open() }
                MenuSeparator {}
                Action { text: "Build Sound Banks"; onTriggered: AudioModule.onBuildBanks() }
                MenuSeparator {}
                Action { text: "Exit"; shortcut: "Ctrl+Q"; onTriggered: Qt.quit() }
            }

            Menu {
                title: "Edit"
                Action { text: "Undo"; shortcut: "Ctrl+Z"; enabled: AudioBridge && AudioBridge.canUndo()
                    onTriggered: { if (AudioBridge) AudioBridge.undo() } }
                Action { text: "Redo"; shortcut: "Ctrl+Shift+Z"; enabled: AudioBridge && AudioBridge.canRedo()
                    onTriggered: { if (AudioBridge) AudioBridge.redo() } }
                MenuSeparator {}
                Action { text: "Cut"; shortcut: "Ctrl+X"
                    onTriggered: { if (AudioBridge) AudioBridge.cut() } }
                Action { text: "Copy"; shortcut: "Ctrl+C"
                    onTriggered: { if (AudioBridge) AudioBridge.copy() } }
                Action { text: "Paste"; shortcut: "Ctrl+V"
                    onTriggered: { if (AudioBridge) AudioBridge.paste() } }
                MenuSeparator {}
                Action { text: "Select All"; shortcut: "Ctrl+A"
                    onTriggered: { if (AudioBridge) AudioBridge.selectAll() } }
            }

            Menu {
                title: "View"
                Action { text: "Timeline"; onTriggered: activePanel = "timeline"; checkable: true; checked: activePanel === "timeline" }
                Action { text: "Mixer"; onTriggered: activePanel = "mixer"; checkable: true; checked: activePanel === "mixer" }
                Action { text: "Waveform Editor"; onTriggered: activePanel = "waveform"; checkable: true; checked: activePanel === "waveform" }
                Action { text: "Events Browser"; onTriggered: activePanel = "events"; checkable: true; checked: activePanel === "events" }
                Action { text: "Effects Rack"; onTriggered: activePanel = "effects"; checkable: true; checked: activePanel === "effects" }
                Action { text: "Recording Studio"; onTriggered: activePanel = "recording"; checkable: true; checked: activePanel === "recording" }
                Action { text: "Sound Banks"; onTriggered: activePanel = "banks"; checkable: true; checked: activePanel === "banks" }
                Action { text: "Batch Processor"; onTriggered: activePanel = "batch"; checkable: true; checked: activePanel === "batch" }
            }

            Menu {
                title: "Transport"
                Action { text: "Play"; shortcut: "Space"; onTriggered: togglePlay() }
                Action { text: "Stop"; shortcut: "Shift+Space"; onTriggered: stopPlayback() }
                Action { text: "Record"; shortcut: "R"; onTriggered: toggleRecord() }
                MenuSeparator {}
                Action { text: "Loop"; checkable: true; checked: AudioBridge && AudioBridge.isLoopEnabled }
                Action { text: "Jump to Start"; shortcut: "Home"
                    onTriggered: { if (AudioBridge) AudioBridge.setPositionMs(0) } }
                Action { text: "Jump to End"; shortcut: "End"
                    onTriggered: { if (AudioBridge) AudioBridge.setPositionMs(AudioBridge.duration) } }
            }

            Menu {
                title: "Tools"
                Action { text: "Audio Analyzer"; onTriggered: activePanel = "analyzer" }
                Action { text: "Batch Converter"; onTriggered: activePanel = "batch" }
                Action { text: "Format Converter"
                    onTriggered: importDialog.open() }
                MenuSeparator {}
                Action { text: "Preferences..."; onTriggered: statusMessage = "Preferences dialog coming soon" }
            }

            Menu {
                title: "Help"
                Action { text: "About KS Audio Studio"; onTriggered: aboutDialog.open() }
                Action { text: "Keyboard Shortcuts"; onTriggered: shortcutDialog.open() }
            }
        }

        // --- Header toolbar ---
        Rectangle {
            height: 36
            color: cPanel
            border.color: cBorder; border.width: 1

            RowLayout {
                anchors.fill: parent; anchors.margins: 4; spacing: 4

                Text { text: "KS AUDIO"; color: cAccent; font.pixelSize: 12; font.bold: true; font.letterSpacing: 2; rightPadding: 12 }

                Rectangle { width: 1; height: 20; color: "#444" }

                ToolButton {
                    text: "\u{1F4C4}"; font.pixelSize: 14
                    ToolTip.visible: hovered; ToolTip.text: "New Project"; ToolTip.delay: 500
                    onClicked: AudioModule.newProject()
                    background: Rectangle { color: parent.hovered ? "#3a3a3e" : "transparent"; radius: 3 }
                }
                ToolButton {
                    text: "\u{1F4C2}"; font.pixelSize: 14
                    ToolTip.visible: hovered; ToolTip.text: "Open Project"; ToolTip.delay: 500
                    onClicked: openProjectDialog.open()
                    background: Rectangle { color: parent.hovered ? "#3a3a3e" : "transparent"; radius: 3 }
                }
                ToolButton {
                    text: "\u{1F4BE}"; font.pixelSize: 14
                    ToolTip.visible: hovered; ToolTip.text: "Save Project"; ToolTip.delay: 500
                    onClicked: saveProject()
                    background: Rectangle { color: parent.hovered ? "#3a3a3e" : "transparent"; radius: 3 }
                }

                Rectangle { width: 1; height: 20; color: "#444" }

                ToolButton {
                    text: "\u25B6"; font.pixelSize: 14
                    ToolTip.visible: hovered; ToolTip.text: "Play (Space)"; ToolTip.delay: 500
                    onClicked: togglePlay()
                    background: Rectangle { color: isPlaying ? cAccent : (parent.hovered ? "#3a3a3e" : "transparent"); radius: 3 }
                    contentItem: Text { text: parent.text; color: isPlaying ? "#121212" : cText; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                ToolButton {
                    text: "\u25A0"; font.pixelSize: 14
                    ToolTip.visible: hovered; ToolTip.text: "Stop"; ToolTip.delay: 500
                    onClicked: stopPlayback()
                    background: Rectangle { color: parent.hovered ? "#3a3a3e" : "transparent"; radius: 3 }
                }
                ToolButton {
                    text: "\u25CF"; font.pixelSize: 14
                    ToolTip.visible: hovered; ToolTip.text: "Record (R)"; ToolTip.delay: 500
                    onClicked: toggleRecord()
                    background: Rectangle { color: parent.hovered ? "#3a3a3e" : "transparent"; radius: 3 }
                    contentItem: Text { text: parent.text; color: isRecording ? "#ff4c4c" : cMuted; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
                ToolButton {
                    text: "\u21BA"; font.pixelSize: 14
                    ToolTip.visible: hovered; ToolTip.text: "Loop"; ToolTip.delay: 500
                    checkable: true; checked: AudioBridge && AudioBridge.isLoopEnabled
                    onClicked: { if (AudioBridge) AudioBridge.setLoopEnabled(checked) }
                    background: Rectangle { color: parent.checked ? "#334466" : (parent.hovered ? "#3a3a3e" : "transparent"); radius: 3 }
                }

                Rectangle { width: 1; height: 20; color: "#444" }

                Item { Layout.fillWidth: true }

                Text { text: currentProject + (isModified ? " *" : ""); color: "#aaa"; font.pixelSize: 10; font.italic: true; rightPadding: 8 }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

        Rectangle {
            width: 160
            Layout.fillHeight: true
            color: cPanel
            border.color: cBorder; border.width: 1

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 6; spacing: 2

                Text { text: "VIEWS"; color: cMuted; font.pixelSize: 9; font.bold: true; leftPadding: 4 }

                Repeater {
                    model: panels
                    delegate: AppButton {
                        height: 26; text: modelData.icon + "  " + modelData.label; font.pixelSize: 9
                        bgcolor: activePanel === modelData.key ? cAccent : "#3e3e42"
                        color: activePanel === modelData.key ? "#121212" : cText
                        onClicked: activePanel = modelData.key
                    }
                }

                Item { Layout.fillHeight: true }

                Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                AppButton {
                    height: 24; text: "\u2699  Settings"; font.pixelSize: 8
                    bgcolor: "transparent"; color: cMuted
                    onClicked: statusMessage = "Audio settings"
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: cBg

            StackLayout {
                anchors.fill: parent
                currentIndex: {
                    var idx = 0;
                    for (var i = 0; i < panels.length; ++i) {
                        if (panels[i].key === activePanel) { idx = i; break }
                    }
                    return idx;
                }

                // Timeline
                Loader {
                    sourceComponent: ColumnLayout {
                        spacing: 6; anchors.margins: 10

                        Text { text: "TIMELINE"; color: cText; font.bold: true; font.pixelSize: 14 }

                        Rectangle {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            color: cBg; border.color: cBorder; border.width: 1

                            ListView {
                                anchors.fill: parent; anchors.margins: 4; clip: true
                                model: AudioEngine ? AudioEngine.loadedBanks : []
                                delegate: Rectangle {
                                    width: ListView.view.width; height: 32
                                    color: index % 2 === 0 ? "#141414" : "#121212"
                                    border.color: "#252525"; border.width: 1

                                    RowLayout {
                                        anchors.fill: parent; anchors.margins: 4; spacing: 6
                                        Text { text: modelData; color: "#888"; font.pixelSize: 9; width: 80; elide: Text.ElideMiddle }
                                        Rectangle { Layout.fillWidth: true; height: 20; color: "#1a1a1a"; radius: 2
                                            border.color: "#2a2a2a"; border.width: 1
                                            Rectangle { width: parent.width * 0.3; height: parent.height; color: cAccent; opacity: 0.3; radius: 2 }
                                            Text { text: AudioEngine ? AudioEngine.getEvents(modelData).length + " events" : ""; color: cMuted; font.pixelSize: 8; anchors.centerIn: parent }
                                        }
                                        Rectangle { width: 16; height: 16; radius: 2; color: "#3a3a3a"
                                            Text { anchors.centerIn: parent; text: "S"; color: cMuted; font.pixelSize: 8 } }
                                        Rectangle { width: 16; height: 16; radius: 2; color: "#3a3a3a"
                                            Text { anchors.centerIn: parent; text: "M"; color: cMuted; font.pixelSize: 8 } }
                                    }
                                }
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                Text {
                                    anchors.centerIn: parent
                                    text: AudioEngine && AudioEngine.loadedBanks.length > 0 ? "" : "No sound banks loaded\nOpen a project or import audio"
                                    color: cMuted; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; visible: parent.count === 0
                                }
                            }
                        }
                    }
                }

                // Mixer
                Item {
                    AudioMixer {
                        anchors.fill: parent
                    }
                }

                // Waveform
                Item {
                    Loader {
                        anchors.fill: parent
                        source: "../AudioEditor/audio_Editor.qml"
                    }
                }

                // Events
                Item {
                    AudioEventBrowser {
                        anchors.fill: parent
                    }
                }

                // Effects
                Item {
                    AudioEffectsRack {
                        anchors.fill: parent
                    }
                }

                // Recording
                Item {
                    AudioRecordingStudio {
                        anchors.fill: parent
                    }
                }

                // Sound Banks
                Item {
                    AudioSoundBanks {
                        anchors.fill: parent
                    }
                }

                // Batch
                Item {
                    AudioBatchProcessor {
                        anchors.fill: parent
                    }
                }

                // Export
                Item {
                    AudioExportPanel {
                        anchors.fill: parent
                    }
                }
            }
        }

        // Right panel: Master & metering
        Rectangle {
            width: 200
            Layout.fillHeight: true
            color: cPanel
            border.color: cBorder; border.width: 1

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 12; spacing: 10

                Text { text: "MASTER"; color: cMuted; font.pixelSize: 10; font.bold: true }

                RowLayout {
                    Text { text: "Volume:"; color: "#bbb"; Layout.preferredWidth: 50 }
                    Slider { from: 0; to: 100; value: masterVolume; Layout.fillWidth: true; height: 16
                        onValueChanged: {
                            masterVolume = value
                            if (AudioEngine) AudioEngine.setBusVolume("master", value / 100.0)
                        }
                        background: Rectangle { x: 0; y: 6; width: parent.width; height: 4; radius: 2; color: "#2a2a2a" }
                        handle: Rectangle { x: parent.left + parent.visualPosition * parent.width - 6; y: 4; width: 12; height: 12; radius: 6; color: cAccent }
                    }
                    Text { text: Math.round(masterVolume) + "%"; color: cAccent; font.pixelSize: 10 }
                }

                Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                Text { text: "METERING"; color: cMuted; font.pixelSize: 10; font.bold: true }

                ColumnLayout {
                    spacing: 4; Layout.fillWidth: true

                    RowLayout {
                        Text { text: "L"; color: "#888"; width: 16; font.pixelSize: 9 }
                        Rectangle { height: 12; width: Math.max(4, leftPeakDb * 160); color: leftPeakDb > 0.9 ? "#ff4444" : cAccent; radius: 2 }
                        Item { Layout.fillWidth: true }
                        Text { text: Math.round(leftPeakDb * 100) + "%"; color: "#888"; font.pixelSize: 8 }
                    }
                    RowLayout {
                        Text { text: "R"; color: "#888"; width: 16; font.pixelSize: 9 }
                        Rectangle { height: 12; width: Math.max(4, rightPeakDb * 160); color: rightPeakDb > 0.9 ? "#ff4444" : cAccent; radius: 2 }
                        Item { Layout.fillWidth: true }
                        Text { text: Math.round(rightPeakDb * 100) + "%"; color: "#888"; font.pixelSize: 8 }
                    }
                }

                Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                Text { text: "PROJECT INFO"; color: cMuted; font.pixelSize: 10; font.bold: true }

                Text { text: "Sample Rate: 44100 Hz"; color: cMuted; font.pixelSize: 8 }
                Text { text: "Bit Depth: 16-bit"; color: cMuted; font.pixelSize: 8 }
                Text { text: "Channels: Stereo"; color: cMuted; font.pixelSize: 8 }

                Item { Layout.fillHeight: true }

                AppButton {
                    height: 32; text: "Build Banks"
                    bgcolor: cAccent; color: "#121212"; font.bold: true
                    onClicked: AudioModule.onBuildBanks()
                }
            }
        }
        }

        // --- Status bar ---
        Rectangle {
            height: 22
            color: "#181818"

            RowLayout {
                anchors.fill: parent; anchors.margins: 4

                Text { text: statusMessage; color: cAccent; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: selectedEvent ? "Event: " + selectedEvent : ""; color: cMuted; font.pixelSize: 9 }
                Rectangle { width: 1; height: 12; color: "#3e3e42" }
                Text { text: "ksEditor v1.0 - Audio Studio"; color: cMuted; font.pixelSize: 10 }
            }
        }
    }

    function togglePlay() {
        if (AudioBridge) {
            if (AudioBridge.isPlaying) AudioBridge.pause()
            else AudioBridge.play()
            isPlaying = AudioBridge.isPlaying
        }
    }

    function stopPlayback() {
        if (AudioBridge) AudioBridge.stop()
        isPlaying = false
    }

    function toggleRecord() {
        if (AudioBridge) {
            if (AudioBridge.isRecording) {
                AudioBridge.stopRecording()
                statusMessage = "Recording stopped"
            } else {
                AudioBridge.startRecording("recording.wav")
                statusMessage = "Recording..."
            }
            isRecording = AudioBridge.isRecording
        }
    }

    function saveProject() {
        if (currentProject === "untitled") {
            saveProjectDialog.open()
        } else {
            AudioModule.onSaveProject()
            isModified = false
        }
    }

    Dialog {
        id: aboutDialog
        title: "About KS Audio Studio"
        standardButtons: Dialog.Ok
        modal: true; anchors.centerIn: parent
        width: 320; height: 200

        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 8

            Text { text: "KS Audio Studio"; color: cAccent; font.pixelSize: 18; font.bold: true }
            Text { text: "Assetto Corsa Sound Editor Suite"; color: "#888"; font.pixelSize: 11 }
            Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }
            Text { text: "Multi-track audio editing, mixing, and sound bank management"; color: "#aaa"; font.pixelSize: 10; wrapMode: Text.WordWrap }
            Text { text: "Version 1.0"; color: "#666"; font.pixelSize: 9 }
        }
    }

    Dialog {
        id: shortcutDialog
        title: "Keyboard Shortcuts"
        standardButtons: Dialog.Close
        modal: true; anchors.centerIn: parent
        width: 400; height: 380

        ColumnLayout {
            anchors.fill: parent; anchors.margins: 12; spacing: 4

            Text { text: "KEYBOARD SHORTCUTS"; color: cAccent; font.pixelSize: 13; font.bold: true; bottomPadding: 6 }

            Repeater {
                model: [
                    "Space - Play/Pause", "Shift+Space - Stop", "R - Toggle Record",
                    "Home - Jump to Start", "End - Jump to End",
                    "Ctrl+N - New Project", "Ctrl+O - Open Project",
                    "Ctrl+S - Save", "Ctrl+Shift+S - Save As",
                    "Ctrl+I - Import Audio", "Ctrl+E - Export Mix",
                    "Ctrl+Z - Undo", "Ctrl+Shift+Z - Redo",
                    "Ctrl+X - Cut", "Ctrl+C - Copy", "Ctrl+V - Paste",
                    "Ctrl+A - Select All", "Ctrl+Q - Exit"
                ]
                Text {
                    text: modelData; color: "#bbb"; font.pixelSize: 10; font.family: "monospace"; leftPadding: 8
                }
            }
        }
    }

    FileDialog {
        id: openProjectDialog
        title: "Open Audio Project"
        nameFilters: ["Audio Project (*.ksap)", "All Files (*)"]
        onAccepted: {
            var path = file.toString().replace("file:///", "")
            currentProject = path.split("/").pop().replace(".ksap", "")
            AudioModule.onOpenProject()
            statusMessage = "Opened: " + path
        }
    }

    FileDialog {
        id: saveProjectDialog
        title: "Save Audio Project As"
        nameFilters: ["Audio Project (*.ksap)", "All Files (*)"]
        onAccepted: {
            var path = file.toString().replace("file:///", "")
            currentProject = path.split("/").pop().replace(".ksap", "")
            AudioModule.onSaveProject()
            isModified = false
            statusMessage = "Saved: " + path
        }
    }

    FileDialog {
        id: importDialog
        title: "Import Audio File"
        nameFilters: ["Audio files (*.wav *.mp3 *.ogg *.flac *.aiff)", "All files (*)"]
        onAccepted: {
            var path = file.toString().replace("file:///", "")
            if (AudioBridge) AudioBridge.loadAudio(path)
            statusMessage = "Imported: " + path.split("/").pop()
        }
    }

    FileDialog {
        id: exportDialog
        title: "Export Mix"
        nameFilters: ["WAV (*.wav)", "OGG (*.ogg)", "MP3 (*.mp3)", "FLAC (*.flac)"]
        onAccepted: {
            var path = file.toString().replace("file:///", "")
            AudioModule.onExportAsset()
            statusMessage = "Exported to: " + path
        }
    }

    Shortcut { sequence: "Space"; onActivated: togglePlay() }
    Shortcut { sequence: "Shift+Space"; onActivated: stopPlayback() }
    Shortcut { sequence: "R"; onActivated: toggleRecord() }

    Component.onCompleted: {
        if (AudioBridge) {
            statusMessage = "Audio engine ready"
        }
    }

    Connections {
        target: AudioBridge
        function onStatusMessage(msg) { statusMessage = msg }
        function onRecordingStateChanged(recording) {
            isRecording = recording
            statusMessage = recording ? "Recording..." : "Recording stopped"
        }
    }
}
