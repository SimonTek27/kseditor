pragma ComponentBehavior: Bound
import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ksEditor.Audio 1.0
import ksEditor.AudioEffects 1.0

Rectangle {
    id: root
    color: "#111111"
    focus: true

    property real baseUiScale: 1.18
    property real uiZoom: 1.0
    property real uiScale: baseUiScale * uiZoom

    readonly property color cBg: "#111111"
    readonly property color cPanel: "#1f1f22"
    readonly property color cPanel2: "#2d2d30"
    readonly property color cBorder: "#3f3f46"
    readonly property color cAccent: "#569cd6"
    readonly property color cAccentDim: "#264f78"
    readonly property color cDanger: "#E81131"
    readonly property color cOk: "#10b981"
    readonly property color cText: "#cccccc"
    readonly property color cMuted: "#888888"
    readonly property color cDim: "#5a5a60"
    readonly property color cWaveL: "#569cd6"
    readonly property color cWaveR: "#10b981"

    property string currentFileName: AudioBridge ? AudioBridge.getFileName() : "No file"
    property real masterVolume: 80
    property real leftPeakDb: AudioBridge ? AudioBridge.leftPeak : 0
    property real rightPeakDb: AudioBridge ? AudioBridge.rightPeak : 0
    property string statusMessage: "Ready"
    property real selectionStart: 0
    property real selectionEnd: 1
    property real playbackPosition: AudioBridge ? AudioBridge.position : 0
    property real totalDuration: AudioBridge ? AudioBridge.duration : 1.0
    property real totalSamples: AudioBridge ? AudioBridge.getSampleCount() : 0
    property bool isPlaying: AudioBridge ? AudioBridge.isPlaying : false
    property bool isLooping: AudioBridge ? AudioBridge.isLoopEnabled : false
    property bool isRecording: AudioBridge ? AudioBridge.isRecording : false
    property bool trackMute: false
    property bool trackSolo: false
    property int activeMenu: -1
    property int activeView: 0
    property int activeTool: 0
    property string fileRate: AudioBridge ? String(AudioBridge.getSampleRate()) + " Hz" : "--"
    property int activeTrackVar: AudioBridge ? AudioBridge.activeTrack() : 0
    property var trackModel: ListModel {}

    property bool selectionDragging: false

    property real viewStart: 0
    property real viewEnd: 1
    function fracToMs(f) { return f * totalDuration * 1000; }
    function msToFrac(ms) { if (totalDuration <= 0) return 0; return Math.max(0, Math.min(1, ms / (totalDuration * 1000))); }
    function clampInRange(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }
    function viewX(frac, w) { var span = viewEnd - viewStart; return (frac - viewStart) / Math.max(0.000001, span) * w }
    function xToFrac(x, w) { return viewStart + (x / Math.max(1, w)) * (viewEnd - viewStart) }

    property var menus: [
        { title: "File", items: [
            { label: "New", cmd: "new" },
            { label: "Open...", cmd: "open" },
            { label: "Save", cmd: "save" },
            { label: "Save As...", cmd: "saveas" },
            { label: "-", cmd: "" },
            { label: "Import", cmd: "import" },
            { label: "Export", cmd: "export" },
            { label: "-", cmd: "" },
            { label: "Save Project", cmd: "saveproject" },
            { label: "Load Project", cmd: "loadproject" }
        ]},
        { title: "Edit", items: [
            { label: "Undo", cmd: "undo" },
            { label: "Redo", cmd: "redo" },
            { label: "-", cmd: "" },
            { label: "Cut", cmd: "cut" },
            { label: "Copy", cmd: "copy" },
            { label: "Paste", cmd: "paste" },
            { label: "Delete", cmd: "delete" },
            { label: "-", cmd: "" },
            { label: "Select All", cmd: "selectall" },
            { label: "Select to End", cmd: "selectend" },
            { label: "Trim", cmd: "trim" },
            { label: "Crop", cmd: "crop" }
        ]},
        { title: "Select", items: [
            { label: "All", cmd: "selectall" },
            { label: "None", cmd: "selectnone" },
            { label: "To End", cmd: "selectend" }
        ]},
        { title: "View", items: [
            { label: "Zoom In", cmd: "zoomin" },
            { label: "Zoom Out", cmd: "zoomout" },
            { label: "Fit", cmd: "zoomfit" },
            { label: "Zoom to Selection", cmd: "zoomsel" },
            { label: "-", cmd: "" },
            { label: "Waveform", cmd: "view0" },
            { label: "Spectrum", cmd: "view1" },
            { label: "Levels", cmd: "view2" }
        ]},
        { title: "Transport", items: [
            { label: "Play", cmd: "play" },
            { label: "Pause", cmd: "pause" },
            { label: "Stop", cmd: "stop" },
            { label: "Record", cmd: "record" },
            { label: "Loop", cmd: "loop" }
        ]},
        { title: "Effects", items: [
            { label: "Volume", cmd: "volume" },
            { label: "Normalize", cmd: "normalize" },
            { label: "Fade In", cmd: "fadein" },
            { label: "Fade Out", cmd: "fadeout" },
            { label: "Reverse", cmd: "reverse" },
            { label: "Invert", cmd: "invert" },
            { label: "-", cmd: "" },
            { label: "Compressor", cmd: "compressor" },
            { label: "Limiter", cmd: "limiter" },
            { label: "Noise Reduction", cmd: "noisered" },
            { label: "Noise Gate", cmd: "gate" },
            { label: "De-Esser", cmd: "deesser" },
            { label: "-", cmd: "" },
            { label: "Delay", cmd: "delay" },
            { label: "Echo", cmd: "echo" },
            { label: "Reverb", cmd: "reverb" },
            { label: "Chorus", cmd: "chorus" },
            { label: "Flanger", cmd: "flanger" },
            { label: "Phaser", cmd: "phaser" },
            { label: "Tremolo", cmd: "tremolo" },
            { label: "Wah-Wah", cmd: "wahwah" },
            { label: "Vocal Reduction", cmd: "vocalred" },
            { label: "-", cmd: "" },
            { label: "Pitch Shift", cmd: "pitch" },
            { label: "Time Stretch", cmd: "timestr" },
            { label: "-", cmd: "" },
            { label: "Equalizer", cmd: "eq" },
            { label: "Low-Pass", cmd: "lowpass" },
            { label: "High-Pass", cmd: "highpass" },
            { label: "Band-Pass", cmd: "bandpass" },
            { label: "Notch", cmd: "notch" },
            { label: "-", cmd: "" },
            { label: "Bitcrusher", cmd: "bitcrusher" },
            { label: "Ring Modulator", cmd: "ringmod" },
            { label: "Saturation", cmd: "saturation" },
            { label: "Tape Emulation", cmd: "tape" },
            { label: "Guitar Amp", cmd: "guitaramp" },
            { label: "Transient Designer", cmd: "transient" },
            { label: "Stereo Enhancer", cmd: "stereo" },
            { label: "Multiband Compressor", cmd: "multiband" }
        ]},
        { title: "Analyze", items: [
            { label: "Spectrum", cmd: "view1" },
            { label: "Levels", cmd: "view2" },
            { label: "-", cmd: "" },
            { label: "Oscilloscope", cmd: "oscilloscope" },
            { label: "Sonogram", cmd: "sonogram" },
            { label: "Pitch Analysis", cmd: "pitchanalysis" },
            { label: "Contrast", cmd: "contrast" },
            { label: "-", cmd: "" },
            { label: "Plot Spectrum", cmd: "plotspectrum" },
            { label: "Statistics", cmd: "statistics" }
        ]},
        { title: "Help", items: [
            { label: "About ksAudioEditor", cmd: "about" }
        ]}
    ]

    function formatTime(seconds) {
        var h = Math.floor(seconds / 3600)
        var m = Math.floor((seconds % 3600) / 60)
        var s = Math.floor(seconds % 60)
        var ms = Math.floor((seconds % 1) * 1000)
        if (h > 0) return h + ":" + (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s + "." + (ms < 100 ? "0" : "") + (ms < 10 ? "0" : "") + ms
        return m + ":" + (s < 10 ? "0" : "") + s + "." + (ms < 100 ? "0" : "") + (ms < 10 ? "0" : "") + ms
    }

    function setSelection(a, b) {
        selectionStart = Math.min(a, b)
        selectionEnd = Math.max(a, b)
        if (AudioBridge) AudioBridge.selectRegion(Math.round(selectionStart * totalDuration * 1000), Math.round(selectionEnd * totalDuration * 1000))
    }

    function trimSelection() {
        if (!AudioBridge || AudioBridge.getSampleCount() === 0) return
        var durMs = totalDuration * 1000
        var startMs = Math.round(selectionStart * durMs)
        var endMs = Math.round(selectionEnd * durMs)
        AudioBridge.deleteRegion(endMs, Math.round(durMs))
        AudioBridge.deleteRegion(0, startMs)
    }

    function runCommand(cmd) {
        var c = String(cmd).trim().toLowerCase()
        if (c === "new") { if (AudioBridge) AudioBridge.newAudio(2, 44100, 10000) }
        else if (c === "open") { openDialog.open() }
        else if (c === "save") { if (AudioBridge) AudioBridge.saveAudio(currentFileName) }
        else if (c === "saveas") { saveDialog.open() }
        else if (c === "import") { openDialog.open() }
        else if (c === "export") { saveDialog.open() }
        else if (c === "saveproject") { projectSaveDialog.open() }
        else if (c === "loadproject") { projectOpenDialog.open() }
        else if (c === "undo") { if (AudioBridge) AudioBridge.undo() }
        else if (c === "redo") { if (AudioBridge) AudioBridge.redo() }
        else if (c === "cut") { if (AudioBridge) AudioBridge.cut() }
        else if (c === "copy") { if (AudioBridge) AudioBridge.copy() }
        else if (c === "paste") { if (AudioBridge) AudioBridge.paste() }
        else if (c === "delete") { if (AudioBridge) AudioBridge.deleteSelection() }
        else if (c === "trim") { trimSelection() }
        else if (c === "crop") { trimSelection() }
        else if (c === "selectall") { if (AudioBridge) AudioBridge.selectAll() }
        else if (c === "selectnone") { if (AudioBridge) AudioBridge.selectNone() }
        else if (c === "selectend") { if (AudioBridge) AudioBridge.selectRegion(Math.round(selectionStart * totalDuration * 1000), Math.round(totalDuration * 1000)) }
        else if (c === "volume") { if (AudioBridge) AudioBridge.amplify(masterVolume / 80.0) }
        else if (c === "normalize") { if (AudioBridge) AudioBridge.normalize(0.95) }
        else if (c === "fadein") { if (AudioBridge) AudioBridge.fadeIn(0, 1000) }
        else if (c === "fadeout") { if (AudioBridge) AudioBridge.fadeOut(0, 1000) }
        else if (c === "reverse") { if (AudioBridge) AudioBridge.reverse() }
        else if (c === "invert") { if (AudioBridge) AudioBridge.invert() }
        else if (c === "compressor") { if (AudioBridge) AudioBridge.applyCompressor(-20, 4, 10, 100, 1.0) }
        else if (c === "limiter") { if (AudioBridge) AudioBridge.applyLimiter(-1, 50) }
        else if (c === "gate") { if (AudioBridge) AudioBridge.applyNoiseGate(-40, -80, 5, 100) }
        else if (c === "deesser") { if (AudioBridge) AudioBridge.applyDeEsser(5000, -20) }
        else if (c === "noisered") { if (AudioBridge) AudioBridge.applyNoiseReduction(-15) }
        else if (c === "delay") { if (AudioBridge) AudioBridge.applyDelay(200, 0.3, 0.5) }
        else if (c === "echo") { if (AudioBridge) AudioBridge.applyEcho(200, 0.5, 0.5) }
        else if (c === "reverb") { if (AudioBridge) AudioBridge.applyReverb(0.5, 0.5, 0.3) }
        else if (c === "chorus") { if (AudioBridge) AudioBridge.applyChorus(0.5, 0.5, 0.4) }
        else if (c === "flanger") { if (AudioBridge) AudioBridge.applyFlanger(0.5, 0.5, 0.5) }
        else if (c === "phaser") { if (AudioBridge) AudioBridge.applyPhaser(0.5, 1.0, 0.0, 0.5) }
        else if (c === "tremolo") { if (AudioBridge) AudioBridge.applyTremolo(5.0, 1.0) }
        else if (c === "wahwah") { if (AudioBridge) AudioBridge.applyWahWah(1000, 1200, 1.0) }
        else if (c === "vocalred") { if (AudioBridge) AudioBridge.applyVocalReduction(0.1, 0.9) }
        else if (c === "pitch") { if (AudioBridge) AudioBridge.pitchShift(2) }
        else if (c === "timestr") { if (AudioBridge) AudioBridge.timeStretch(1.0) }
        else if (c === "eq") { if (AudioEffects) AudioEffects.applyEqPreset("Flat") }
        else if (c === "lowpass") { if (AudioBridge) AudioBridge.applyLowPassFilter(1000, 0.7) }
        else if (c === "highpass") { if (AudioBridge) AudioBridge.applyHighPassFilter(100, 0.7) }
        else if (c === "bandpass") { if (AudioBridge) AudioBridge.applyBandPassFilter(100, 4000) }
        else if (c === "notch") { if (AudioBridge) AudioBridge.applyNotchFilter(1000, 100) }
        else if (c === "bitcrusher") { if (AudioBridge) AudioBridge.applyBitCrusher(8, 4.0) }
        else if (c === "ringmod") { if (AudioBridge) AudioBridge.applyRingMod(1000, 0.5) }
        else if (c === "saturation") { if (AudioBridge) AudioBridge.applySaturation(2.0, 0.5) }
        else if (c === "tape") { if (AudioBridge) AudioBridge.applyTapeEmulation(1.0, 0.1, 0.1) }
        else if (c === "guitaramp") { if (AudioBridge) AudioBridge.applyGuitarAmp(10.0, 0.5, 1.0) }
        else if (c === "transient") { if (AudioBridge) AudioBridge.applyTransientDesigner(0.0, 0.0) }
        else if (c === "stereo") { if (AudioBridge) AudioBridge.applyStereoEnhancer(1.5) }
        else if (c === "multiband") { if (AudioBridge) AudioBridge.applyMultibandCompressor(-20, -20, -20, 4, 4, 4, 10, 100) }
        else if (c === "play") { if (AudioBridge) { if (isPlaying) AudioBridge.pause(); else AudioBridge.play() } }
        else if (c === "pause") { if (AudioBridge) AudioBridge.pause() }
        else if (c === "stop") { if (AudioBridge) AudioBridge.stop() }
        else if (c === "record") { if (AudioBridge) { if (isRecording) AudioBridge.stopRecording(); else AudioBridge.startRecording("/dev", 44100, 2) } }
        else if (c === "loop") { if (AudioBridge) AudioBridge.setLoopEnabled(!AudioBridge.isLoopEnabled) }
        else if (c === "zoomin") { zoomViewport(0.5) }
        else if (c === "zoomout") { zoomViewport(2.0) }
        else if (c === "zoomfit") { viewStart = 0; viewEnd = 1 }
        else if (c === "zoomsel") { viewStart = selectionStart; viewEnd = selectionEnd }
        else if (c === "view0") { activeView = 0 }
        else if (c === "view1") { activeView = 1 }
        else if (c === "view2") { activeView = 2 }
        else if (c === "oscilloscope") { activeView = 3 }
        else if (c === "sonogram") { activeView = 4 }
        else if (c === "pitchanalysis") { activeView = 5 }
        else if (c === "contrast") { activeView = 6 }
        else if (c === "plotspectrum") { activeView = 7 }
        else if (c === "statistics") { activeView = 8 }
        else if (c === "about") { statusMessage = "ksAudioEditor \u2014 audio wave editor" }
        statusMessage = "Command: " + cmd
    }

    function zoomViewport(factor) {
        var span = viewEnd - viewStart
        if (factor < 1) {
            var newSpan = span * factor
            var center = (viewStart + viewEnd) / 2
            viewStart = clampInRange(center - newSpan / 2, 0, 1 - newSpan)
            viewEnd = viewStart + newSpan
        } else {
            var newSpan2 = span * factor
            if (newSpan2 > 1) { viewStart = 0; viewEnd = 1; return }
            var center2 = (viewStart + viewEnd) / 2
            viewStart = clampInRange(center2 - newSpan2 / 2, 0, 1 - newSpan2)
            viewEnd = viewStart + newSpan2
        }
    }

function refreshTrackModel() {
        if (!AudioBridge) return
        var n = AudioBridge.trackCount()
        while (trackModel.count > n) trackModel.remove(trackModel.count - 1)
        for (var i = 0; i < n; ++i) {
            var row = {
                name: AudioBridge.trackName(i),
                gain: AudioBridge.trackGain(i),
                pan: AudioBridge.trackPan(i),
                mute: AudioBridge.trackMute(i),
                solo: AudioBridge.trackSolo(i),
                rate: AudioBridge.trackRate(i),
                canUndo: AudioBridge.trackCanUndo(i),
                canRedo: AudioBridge.trackCanRedo(i),
                clipCount: AudioBridge.trackClipCount(i),
                envPointCount: AudioBridge.trackEnvelopePointCount(i, 0)
            }
            if (i < trackModel.count) trackModel.set(i, row)
            else trackModel.append(row)
        }
    }

    Component.onCompleted: { refreshTrackModel() }

    FileDialog {
        id: openDialog; title: "Open Audio File"
        nameFilters: ["Audio files (*.wav *.mp3 *.ogg *.flac *.aiff)", "All files (*)"]
        onAccepted: { if (AudioBridge) AudioBridge.loadAudio(selectedFile.toString().replace("file:///", "")) }
    }
    FileDialog {
        id: importDialog; title: "Import Clip to Active Track"
        nameFilters: ["Audio files (*.wav *.mp3 *.ogg *.flac *.aiff)", "All files (*)"]
        onAccepted: { if (AudioBridge) AudioBridge.trackAddClip(activeTrackVar, selectedFile.toString().replace("file:///", ""), Math.round(selectionStart * totalDuration * 1000)) }
    }
    FileDialog {
        id: saveDialog; title: "Save Audio File"
        nameFilters: ["WAV files (*.wav)", "OGG files (*.ogg)", "MP3 files (*.mp3)"]
        onAccepted: { if (AudioBridge) AudioBridge.saveAudio(selectedFile.toString().replace("file:///", "")) }
    }
    FileDialog {
        id: projectOpenDialog; title: "Load Project"
        nameFilters: ["ksEditor Audio Project (*.ksaudio)", "All files (*)"]
        onAccepted: { if (AudioBridge) AudioBridge.loadProject(selectedFile.toString().replace("file:///", "")) }
    }
    FileDialog {
        id: projectSaveDialog; title: "Save Project"
        nameFilters: ["ksEditor Audio Project (*.ksaudio)", "All files (*)"]
        defaultSuffix: "ksaudio"
        onAccepted: { if (AudioBridge) AudioBridge.saveProject(selectedFile.toString().replace("file:///", "")) }
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Space) {
            if (AudioBridge) { if (isPlaying) AudioBridge.pause(); else AudioBridge.play() }
            event.accepted = true
        } else if (event.key === Qt.Key_O && event.modifiers & Qt.ControlModifier) {
            openDialog.open(); event.accepted = true
        } else if (event.key === Qt.Key_S && event.modifiers & Qt.ControlModifier) {
            if (AudioBridge) AudioBridge.saveAudio(currentFileName); event.accepted = true
        } else if (event.key === Qt.Key_A && event.modifiers & Qt.ControlModifier) {
            if (AudioBridge) AudioBridge.selectAll(); event.accepted = true
        } else if (event.key === Qt.Key_Z && event.modifiers & Qt.ControlModifier) {
            if (event.modifiers & Qt.ShiftModifier) { if (AudioBridge) AudioBridge.redo() }
            else { if (AudioBridge) AudioBridge.undo() }
            event.accepted = true
        } else if (event.key === Qt.Key_Y && event.modifiers & Qt.ControlModifier) {
            if (AudioBridge) AudioBridge.redo(); event.accepted = true
        }
    }

    Item {
        width: parent.width / uiScale
        height: parent.height / uiScale
        scale: uiScale
        transformOrigin: Item.TopLeft

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: menuBar
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: cPanel
            border.color: cBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                spacing: 2

                Rectangle {
                    width: 64; Layout.fillHeight: true
                    color: "#1e1e1e"
                    Text {
                        anchors.centerIn: parent
                        text: "KS Audio"
                        color: cDanger; font.pixelSize: 11; font.bold: true
                    }
                }

                Repeater {
                    model: menus
                    delegate: Rectangle {
                        Layout.preferredWidth: mmHover.containsMouse || activeMenu === index ? mmTxt.implicitWidth + 22 : mmTxt.implicitWidth + 14
                        Layout.preferredHeight: 22
                        radius: 3
                        color: activeMenu === index ? cAccentDim : (mmHover.containsMouse ? cPanel2 : "transparent")
                        Text {
                            id: mmTxt
                            anchors.centerIn: parent
                            text: modelData.title
                            color: activeMenu === index ? "#ffffff" : (mmHover.containsMouse ? "#ffffff" : "#bbb")
                            font.pixelSize: 10
                        }
                        MouseArea { id: mmHover; anchors.fill: parent; hoverEnabled: true
                            onClicked: { if (activeMenu === index) activeMenu = -1; else activeMenu = index }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    Layout.rightMargin: 6
                    text: fileRate
                    color: cDim; font.pixelSize: 9
                }
            }

            Popup {
                id: menuPopup
                x: activeMenuBarX()
                y: menuBar.height + 2
                width: 190
                padding: 4
                visible: activeMenu >= 0
                onVisibleChanged: if (!visible) activeMenu = -1

                background: Rectangle { color: cPanel; border.color: cBorder; border.width: 1; radius: 4 }

                contentItem: Column {
                    spacing: 1
                    Repeater {
                        model: activeMenu >= 0 ? menus[activeMenu].items : []
                        delegate: Rectangle {
                            width: 182
                            height: modelData.cmd === "" ? 9 : 24
                            radius: 2
                            color: miHover.containsMouse ? cAccentDim : "transparent"
                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width; height: 1
                                color: cBorder
                                visible: modelData.cmd === ""
                            }
                            Text {
                                anchors.left: parent.left; anchors.leftMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                color: "#ddd"; font.pixelSize: 10
                            }
                            MouseArea { id: miHover; anchors.fill: parent; hoverEnabled: true
                                onClicked: { if (modelData.cmd !== "") runCommand(modelData.cmd); activeMenu = -1 }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: "#1c1c1f"
            border.color: cBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 3

                Repeater {
                    model: [
                        { icon: "\u23EE", act: "start", tip: "Skip to Start" },
                        { icon: "\u25B6", act: "play", tip: "Play", green: true },
                        { icon: "\u23F8", act: "pause", tip: "Pause" },
                        { icon: "\u23F9", act: "stop", tip: "Stop" },
                        { icon: "\u23ED", act: "end", tip: "Skip to End" },
                        { icon: "\u25CF", act: "record", tip: "Record", red: true },
                        { icon: "\u21BB", act: "loop", tip: "Loop" }
                    ]
                    delegate: Rectangle {
                        width: 34; height: 30; radius: 3
                        color: (modelData.green === true && isPlaying) ? "#1d5c3a" : (modelData.red === true && isRecording) ? "#5c1d1d" : (tpHover.containsMouse ? cPanel2 : "#252529")
                        border.color: modelData.act === "loop" && isLooping ? cOk : "transparent"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: modelData.icon
                            color: modelData.green === true ? (isPlaying ? cOk : "#6fbf8f") : modelData.red === true ? (isRecording ? "#ff6666" : "#d96459") : "#ddd"
                            font.pixelSize: 15
                        }
                        MouseArea { id: tpHover; anchors.fill: parent; hoverEnabled: true
                            onClicked: {
                                if (modelData.act === "start") { if (AudioBridge) AudioBridge.setPositionMs(0) }
                                else if (modelData.act === "play") runCommand("play")
                                else if (modelData.act === "pause") runCommand("pause")
                                else if (modelData.act === "stop") runCommand("stop")
                                else if (modelData.act === "end") { if (AudioBridge) AudioBridge.setPositionMs(Math.max(0, totalDuration - 500)) }
                                else if (modelData.act === "record") runCommand("record")
                                else if (modelData.act === "loop") runCommand("loop")
                            }
                        }
                        ToolTip { visible: tpHover.containsMouse; text: modelData.tip }
                    }
                }

                Rectangle { width: 1; height: 24; color: cBorder }

                Repeater {
                    model: [
                        { icon: "\u25F0", act: 0, tip: "Selection" },
                        { icon: "\u223C", act: 1, tip: "Envelope" },
                        { icon: "\u270E", act: 2, tip: "Draw" },
                        { icon: "\u2315", act: 3, tip: "Zoom" },
                        { icon: "\u2922", act: 4, tip: "Move" },
                        { icon: "\u2637", act: 5, tip: "Analyze" }
                    ]
                    delegate: Rectangle {
                        width: 30; height: 28; radius: 3
                        color: activeTool === modelData.act ? cAccentDim : (toolHover.containsMouse ? cPanel2 : "transparent")
                        border.color: activeTool === modelData.act ? cAccent : "transparent"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: modelData.icon; color: activeTool === modelData.act ? cAccent : "#aaa"
                            font.pixelSize: 13
                        }
                        MouseArea { id: toolHover; anchors.fill: parent; hoverEnabled: true
                            onClicked: activeTool = modelData.act
                        }
                        ToolTip { visible: toolHover.containsMouse; text: modelData.tip }
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    Layout.rightMargin: 4
                    text: currentFileName
                    color: cMuted; font.pixelSize: 9
                    elide: Text.ElideMiddle
                    Layout.maximumWidth: 260
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: "#1c1c1f"
            border.color: cBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                spacing: 1

                Repeater {
                    model: [
                        { label: "Vol", cmd: "volume", tip: "Volume" },
                        { label: "Norm", cmd: "normalize", tip: "Normalize" },
                        { label: "FdIn", cmd: "fadein", tip: "Fade In" },
                        { label: "FdOut", cmd: "fadeout", tip: "Fade Out" },
                        { label: "Rev", cmd: "reverse", tip: "Reverse" },
                        { label: "Inv", cmd: "invert", tip: "Invert" },
                        { label: "Cmp", cmd: "compressor", tip: "Compressor" },
                        { label: "Lim", cmd: "limiter", tip: "Limiter" },
                        { label: "Noise", cmd: "noisered", tip: "Noise Reduction" },
                        { label: "Dly", cmd: "delay", tip: "Delay" },
                        { label: "Echo", cmd: "echo", tip: "Echo" },
                        { label: "Revb", cmd: "reverb", tip: "Reverb" },
                        { label: "Cho", cmd: "chorus", tip: "Chorus" },
                        { label: "Fla", cmd: "flanger", tip: "Flanger" },
                        { label: "Pit", cmd: "pitch", tip: "Pitch Shift" },
                        { label: "Time", cmd: "timestr", tip: "Time Stretch" },
                        { label: "EQ", cmd: "eq", tip: "Equalizer" },
                        { label: "LP", cmd: "lowpass", tip: "Low-Pass" },
                        { label: "HP", cmd: "highpass", tip: "High-Pass" },
                        { label: "BP", cmd: "bandpass", tip: "Band-Pass" },
                        { label: "Notch", cmd: "notch", tip: "Notch" },
                        { label: "Gate", cmd: "gate", tip: "Noise Gate" }
                    ]
                    delegate: Rectangle {
                        width: 46; height: 24; radius: 3
                        color: effHover.containsMouse ? cAccentDim : "transparent"
                        border.color: "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: effHover.containsMouse ? "#ffffff" : "#999"
                            font.pixelSize: 9; font.bold: effHover.containsMouse
                        }
                        MouseArea { id: effHover; anchors.fill: parent; hoverEnabled: true
                            onClicked: runCommand(modelData.cmd)
                        }
                        ToolTip { visible: effHover.containsMouse; text: modelData.tip }
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: cBg

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 20
                        color: cPanel
                        border.color: "#2d2d30"
                        border.width: 1
                        Canvas { id: rulerCanvas; anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d"); ctx.clearRect(0, 0, width, height); ctx.fillStyle = "#999"; ctx.font = "8px monospace"; ctx.textAlign = "center"
                                var dur = totalDuration; if (dur <= 0) dur = 1
                                var start = viewStart * dur, end = viewEnd * dur, span = end - start
                                var steps = 10
                                ctx.strokeStyle = "#444"
                                for (var i = 0; i <= steps; i++) {
                                    var x = i * width / steps
                                    var t = start + i * span / steps
                                    ctx.fillText(formatTime(t), x, 12)
                                    ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, 6); ctx.stroke()
                                }
                            }
                            Connections { target: root; function onTotalDurationChanged() { rulerCanvas.requestPaint() }
                                function onViewStartChanged() { rulerCanvas.requestPaint() }
                                function onViewEndChanged() { rulerCanvas.requestPaint() }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#111112"
                        border.color: "#22222a"
                        border.width: 1

                        ColumnLayout { anchors.fill: parent; spacing: 0
                            Rectangle { Layout.fillWidth: true; height: 22; color: "#1a1a1e"
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 6
                                    Text { text: "Tracks"; color: cDim; font.pixelSize: 9; font.bold: true }
                                    Item { Layout.fillWidth: true }
                                    Rectangle { width: 64; height: 18; radius: 3; color: cAccentDim
                                        Text { anchors.centerIn: parent; text: "+ Track"; color: cAccent; font.pixelSize: 9 }
                                        MouseArea { anchors.fill: parent; onClicked: { if (AudioBridge) AudioBridge.addTrack() } }
                                    }
                                }
                            }
                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                                Column {
                                    width: parent.width
                                    spacing: 0
                                    Repeater {
                                        model: trackModel
                                        delegate: RowLayout {
                                            height: 132
                                            width: parent.width
                                            spacing: 0
                                            Rectangle {
                                                width: 156
                                                Layout.fillHeight: true
                                                color: index === activeTrackVar ? "#1d2226" : "#1a1a1e"
                                                border.color: "#2d2d30"
                                                border.width: 1
                                                ColumnLayout {
                                                    anchors.fill: parent
                                                    anchors.margins: 4
                                                    spacing: 3
                                                    RowLayout {
                                                        Layout.fillWidth: true
                                                        Layout.preferredHeight: 18
                                                        Rectangle { width: 16; height: 16; radius: 2; color: "#2d2d30"
                                                            Text { anchors.centerIn: parent; text: "x"; color: "#aaa"; font.pixelSize: 9 }
                                                            MouseArea { anchors.fill: parent; onClicked: { if (AudioBridge) AudioBridge.removeTrack(index) } }
                                                        }
                                                        Text {
                                                            Layout.fillWidth: true
                                                            text: model.name
                                                            color: "#bbb"; font.pixelSize: 9
                                                            elide: Text.ElideMiddle
                                                        }
                                                        Rectangle { width: 16; height: 16; radius: 2; color: index === activeTrackVar ? cAccent : "#2d2d30"
                                                            Text { anchors.centerIn: parent; text: "\u2297"; color: index === activeTrackVar ? "#0a0a0a" : cWaveL; font.pixelSize: 9 }
                                                            MouseArea { anchors.fill: parent; onClicked: { if (AudioBridge) AudioBridge.setActiveTrack(index) } }
                                                        }
                                                    }
                                                    RowLayout {
                                                        Layout.fillWidth: true; spacing: 4
                                                        Rectangle { Layout.fillWidth: true; height: 22; radius: 3; color: model.solo ? cAccentDim : "#252529"
                                                            Text { anchors.centerIn: parent; text: "S"; color: model.solo ? cAccent : "#aaa"; font.pixelSize: 10; font.bold: true }
                                                            MouseArea { anchors.fill: parent; onClicked: { if (AudioBridge) { AudioBridge.setTrackSolo(index, !AudioBridge.trackSolo(index)); AudioBridge.setActiveTrack(index) } } }
                                                        }
                                                        Rectangle { Layout.fillWidth: true; height: 22; radius: 3; color: model.mute ? cAccentDim : "#252529"
                                                            Text { anchors.centerIn: parent; text: "M"; color: model.mute ? cAccent : "#aaa"; font.pixelSize: 10; font.bold: true }
                                                            MouseArea { anchors.fill: parent; onClicked: { if (AudioBridge) { AudioBridge.setTrackMute(index, !AudioBridge.trackMute(index)); AudioBridge.setActiveTrack(index) } } }
                                                        }
                                                    }
                                                    Text { text: "GAIN"; color: cDim; font.pixelSize: 8; font.bold: true }
                                                    Slider {
                                                        id: gainSl
                                                        from: 0; to: 200; value: model.gain * 100
                                                        Layout.fillWidth: true; Layout.preferredHeight: 14
                                                        onValueChanged: { if (AudioBridge) AudioBridge.setTrackGain(index, value / 100.0) }
                                                        background: Rectangle { x: 0; y: 6; width: parent.width; height: 4; radius: 2; color: "#333"
                                                            Rectangle { width: parent.width * gainSl.visualPosition; height: parent.height; color: cOk; radius: 2 }
                                                        }
                                                        handle: Rectangle { x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width); y: 2; width: 12; height: 12; radius: 6; color: "#ddd" }
                                                    }
                                                    Text { text: "PAN"; color: cDim; font.pixelSize: 8; font.bold: true }
                                                    Slider {
                                                        id: panSl
                                                        from: -1; to: 1; value: model.pan
                                                        Layout.fillWidth: true; Layout.preferredHeight: 14
                                                        onValueChanged: { if (AudioBridge) AudioBridge.setTrackPan(index, value) }
                                                        background: Rectangle { x: 0; y: 6; width: parent.width; height: 4; radius: 2; color: "#333"
                                                            Rectangle { width: parent.width * ((panSl.value + 1) / 2); height: parent.height; color: cAccent; radius: 2 }
                                                        }
                                                        handle: Rectangle { x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width); y: 2; width: 12; height: 12; radius: 6; color: "#ddd" }
                                                    }
                                                    Text { text: "RATE " + model.rate + " Hz"; color: cDim; font.pixelSize: 8 }
                                                    Text { text: "CLIPS: " + model.clipCount; color: cDim; font.pixelSize: 8 }
                                                    Text { text: "ENV: " + model.envPointCount; color: cDim; font.pixelSize: 8 }
                                                    RowLayout {
                                                        Layout.fillWidth: true; spacing: 4
                                                        Rectangle { Layout.fillWidth: true; height: 20; radius: 3; color: "#2d2d30"
                                                            Text { anchors.centerIn: parent; text: "Split"; color: "#aaa"; font.pixelSize: 9 }
                                                            MouseArea { anchors.fill: parent; onClicked: { if (AudioBridge) AudioBridge.trackSplitClip(index, 0, Math.round(selectionStart * totalDuration * 1000)) } }
                                                        }
                                                        Rectangle { Layout.fillWidth: true; height: 20; radius: 3; color: "#2d2d30"
                                                            Text { anchors.centerIn: parent; text: "Join"; color: "#aaa"; font.pixelSize: 9 }
                                                            MouseArea { anchors.fill: parent; onClicked: { if (AudioBridge && model.clipCount > 1) AudioBridge.trackJoinClips(index, 0, 1) } }
                                                        }
                                                    }
                                                    RowLayout {
                                                        Layout.fillWidth: true; spacing: 4
                                                        Rectangle { Layout.fillWidth: true; height: 20; radius: 3; color: "#2d2d30"
                                                            Text { anchors.centerIn: parent; text: "Add Clip"; color: "#aaa"; font.pixelSize: 9 }
                                                            MouseArea { anchors.fill: parent; onClicked: { if (AudioBridge) importDialog.open() } }
                                                        }
                                                    }
                                                    RowLayout {
                                                        Layout.fillWidth: true; spacing: 4
                                                        Rectangle { Layout.fillWidth: true; height: 20; radius: 3; color: model.canUndo ? "#2d2d30" : "#1a1a1e"
                                                            Text { anchors.centerIn: parent; text: "\u2190"; color: model.canUndo ? "#ccc" : "#666"; font.pixelSize: 10 }
                                                            MouseArea { anchors.fill: parent; enabled: model.canUndo; onClicked: { if (AudioBridge) AudioBridge.trackUndo(index) } }
                                                        }
                                                        Rectangle { Layout.fillWidth: true; height: 20; radius: 3; color: model.canRedo ? "#2d2d30" : "#1a1a1e"
                                                            Text { anchors.centerIn: parent; text: "\u2192"; color: model.canRedo ? "#ccc" : "#666"; font.pixelSize: 10 }
                                                            MouseArea { anchors.fill: parent; enabled: model.canRedo; onClicked: { if (AudioBridge) AudioBridge.trackRedo(index) } }
                                                        }
                                                    }
                                                    Text { text: "ENVELOPE"; color: cDim; font.pixelSize: 8; font.bold: true }
                                                    RowLayout {
                                                        Layout.fillWidth: true; spacing: 4
                                                        Rectangle { Layout.fillWidth: true; height: 20; radius: 3; color: "#2d2d30"
                                                            Text { anchors.centerIn: parent; text: "Add Point"; color: "#aaa"; font.pixelSize: 9 }
                                                            MouseArea { anchors.fill: parent; onClicked: { if (AudioBridge) AudioBridge.trackAddEnvelopePoint(index, 0, Math.round(selectionStart * totalDuration * 1000), 0.5) } }
                                                        }
                                                        Rectangle { Layout.fillWidth: true; height: 20; radius: 3; color: "#2d2d30"
                                                            Text { anchors.centerIn: parent; text: "Apply"; color: "#aaa"; font.pixelSize: 9 }
                                                            MouseArea { anchors.fill: parent; onClicked: { if (AudioBridge) AudioBridge.trackApplyEnvelope(index, 0) } }
                                                        }
                                                    }
                                                }
                                            }
                                            Rectangle {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                color: "#0a0a0a"
                                                ColumnLayout { anchors.fill: parent; anchors.margins: 2; spacing: 0
                                                    Rectangle { Layout.fillWidth: true; height: 18; color: "transparent"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 4; anchors.verticalCenter: parent.verticalCenter; text: "L"; color: cWaveL; font.pixelSize: 9; font.bold: true }
                                                    }
                                                    Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: "#0a0a0a"
                                                        Canvas { anchors.fill: parent
                                                            onPaint: drawTrackWave(index, cWaveL, width, height, ctx)
                                                            Connections { target: root
                                                                function onViewStartChanged() { requestPaint() }
                                                                function onViewEndChanged() { requestPaint() }
                                                            }
                                                            Connections { target: AudioBridge
                                                                function onPositionChanged() { requestPaint() }
                                                                function onAudioChanged() { requestPaint() }
                                                                function onLoadComplete() { requestPaint() }
                                                            }
                                                        }
                                                        Rectangle { x: viewX(selectionStart, parent.width); width: Math.max(0, viewX(selectionEnd, parent.width) - viewX(selectionStart, parent.width)); height: parent.height; color: cAccent; opacity: 0.14 }
                                                        Rectangle { x: viewX(playbackPosition / Math.max(0.01, totalDuration), parent.width); width: 1; height: parent.height; color: "#ffffff"; opacity: 0.75; visible: isPlaying || playbackPosition > 0 }
                                                        MouseArea { anchors.fill: parent; acceptedButtons: Qt.LeftButton; property real base: 0
                                                            onPressed: (m) => { if (AudioBridge) AudioBridge.setActiveTrack(index); selectionDragging = true; base = xToFrac(m.x, parent.width); selectionStart = base; selectionEnd = base }
                                                            onPositionChanged: (m) => { if (selectionDragging) setSelection(base, xToFrac(m.x, parent.width)) }
                                                            onReleased: { selectionDragging = false }
                                                        }
                                                    }
                                                    Rectangle { Layout.fillWidth: true; height: 18; color: "transparent"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 4; anchors.verticalCenter: parent.verticalCenter; text: "R"; color: cWaveR; font.pixelSize: 9; font.bold: true }
                                                    }
                                                    Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: "#0a0a0a"
                                                        Canvas { anchors.fill: parent
                                                            onPaint: drawTrackWave(index, cWaveR, width, height, ctx)
                                                            Connections { target: root
                                                                function onViewStartChanged() { requestPaint() }
                                                                function onViewEndChanged() { requestPaint() }
                                                            }
                                                            Connections { target: AudioBridge
                                                                function onPositionChanged() { requestPaint() }
                                                                function onAudioChanged() { requestPaint() }
                                                                function onLoadComplete() { requestPaint() }
                                                            }
                                                        }
                                                        Rectangle { x: viewX(selectionStart, parent.width); width: Math.max(0, viewX(selectionEnd, parent.width) - viewX(selectionStart, parent.width)); height: parent.height; color: cAccent; opacity: 0.14 }
                                                        Rectangle { x: viewX(playbackPosition / Math.max(0.01, totalDuration), parent.width); width: 1; height: parent.height; color: "#ffffff"; opacity: 0.75; visible: isPlaying || playbackPosition > 0 }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; height: 24; color: cPanel
                        Canvas { id: overviewCanvas; anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d"); ctx.clearRect(0, 0, width, height)
                                if (!AudioBridge || AudioBridge.getSampleCount() === 0) return
                                var data = AudioBridge.getWaveformData(Math.floor(width))
                                if (data.length === 0) return
                                var midY = height / 2
                                ctx.strokeStyle = "#888"; ctx.lineWidth = 1; ctx.beginPath()
                                for (var i = 0; i < data.length; ++i) { var sample = parseFloat(data[i]) * midY * 0.8; ctx.moveTo(i, midY - sample); ctx.lineTo(i, midY + sample) }
                                ctx.stroke()
                                ctx.fillStyle = "rgba(86,156,214,0.25)"; ctx.fillRect(selectionStart * width, 0, (selectionEnd - selectionStart) * width, height)
                                ctx.strokeStyle = "#fff"; ctx.beginPath(); ctx.moveTo(playbackPosition / Math.max(0.01, totalDuration) * width, 0); ctx.lineTo(playbackPosition / Math.max(0.01, totalDuration) * width, height); ctx.stroke()
                            }
                            Connections { target: AudioBridge; function onPositionChanged() { overviewCanvas.requestPaint() } }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 22
            color: cPanel
            border.color: cBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 10

                Text { text: "Project Rate: " + fileRate; color: cMuted; font.pixelSize: 9 }
                Rectangle { width: 1; height: 14; color: cBorder }
                Text { text: "Selection Start: " + formatTime(selectionStart * totalDuration); color: cMuted; font.pixelSize: 9 }
                Text { text: "End: " + formatTime(selectionEnd * totalDuration); color: cMuted; font.pixelSize: 9 }
                Text { text: "Length: " + formatTime((selectionEnd - selectionStart) * totalDuration); color: cMuted; font.pixelSize: 9 }
                Item { Layout.fillWidth: true }
                Text { text: statusMessage; color: cAccent; font.pixelSize: 9 }
                Rectangle { width: 1; height: 14; color: cBorder }
                Text { text: "Pos: " + formatTime(playbackPosition); color: cMuted; font.pixelSize: 9 }
                Text { text: currentFileName; color: "#555"; font.pixelSize: 9 }
            }
        }
    }
    }

    function drawTrackWave(trackIndex, color, w, h, ctx) {
        ctx.clearRect(0, 0, w, h)
        if (!AudioBridge || AudioBridge.trackSampleCount(trackIndex) === 0) {
            ctx.fillStyle = "#888"; ctx.font = "11px sans-serif"; ctx.textAlign = "center"
            ctx.fillText("No audio loaded", w/2, h/2); return
        }
        var data = AudioBridge.getTrackWaveformData(
            trackIndex, Math.round(fracToMs(viewStart)), Math.round(fracToMs(viewEnd)), Math.floor(w))
        if (data.length === 0) return
        var midY = h / 2
        ctx.fillStyle = color
        for (var i = 0; i < data.length; i += 2) {
            var m = parseFloat(data[i]) * midY * 1.5
            var M = parseFloat(data[i+1]) * midY * 1.5
            ctx.fillRect(i/2, midY - Math.max(M, m), 1, Math.max(1, M - m))
        }
        // Draw envelope
        var envData = AudioBridge.trackEnvelopePoints(trackIndex, 0)
        if (envData.length > 1) {
            ctx.strokeStyle = "#ffff00"
            ctx.lineWidth = 2
            ctx.beginPath()
            var durMs = (viewEnd - viewStart) * totalDuration * 1000
            for (var i = 0; i < envData.length; ++i) {
                var p = envData[i]
                var x = ((p.timeMs - fracToMs(viewStart) * 1000) / durMs) * w
                var y = midY - (p.gain - 0.5) * midY * 2
                if (i === 0) ctx.moveTo(x, y)
                else ctx.lineTo(x, y)
            }
            ctx.stroke()
        }
        ctx.strokeStyle = "#333"; ctx.beginPath()
        ctx.moveTo(0, midY); ctx.lineTo(w, midY); ctx.stroke()
    }

    function drawWave(color, w, h, ctx) {
        drawTrackWave(AudioBridge ? AudioBridge.activeTrack() : 0, color, w, h, ctx)
    }

    function activeMenuBarX() {
        if (activeMenu < 0) return 0
        var total = 6 + 64
        for (var i = 0; i < activeMenu; i++) total += menus[i].title.length * 7 + 16
        return total
    }

    Connections {
        target: AudioBridge
        function onStatusMessage(msg) { statusMessage = msg }

        function onPositionChanged() { overviewCanvas.requestPaint() }
        function onAudioChanged() { activeTrackVar = AudioBridge.activeTrack(); refreshTrackModel(); overviewCanvas.requestPaint() }
        function onLoadComplete() { viewStart = 0; viewEnd = 1; selectionStart = 0; selectionEnd = 1; activeTrackVar = AudioBridge.activeTrack(); refreshTrackModel(); overviewCanvas.requestPaint() }
    }
    Connections {
        target: root
        function onViewStartChanged() { rulerCanvas.requestPaint() }
        function onViewEndChanged() { rulerCanvas.requestPaint() }
    }
}