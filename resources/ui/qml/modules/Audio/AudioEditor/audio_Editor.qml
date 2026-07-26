import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ksEditor.Audio 1.0
import ksEditor.AudioEffects 1.0

Rectangle {
    id: audioEditor
    width: 1280
    height: 720
    color: "#1e1e1e"

    readonly property color cAccent: "#00aaff"
    readonly property color cAccent2: "#ff6633"
    readonly property color cPanel: "#252526"
    readonly property color cBg: "#1a1a1a"
    readonly property color cBorder: "#3c3c3c"
    readonly property color cText: "#cccccc"
    readonly property color cMuted: "#888888"
    readonly property color cWaveL: "#00cc66"
    readonly property color cWaveR: "#ff6633"

    property string currentFileName: AudioBridge ? AudioBridge.getFileName() : "No file"
    property real masterVolume: 80
    property real playbackSpeed: 1.0
    property real leftPeakDb: AudioBridge ? AudioBridge.leftPeak : 0
    property real rightPeakDb: AudioBridge ? AudioBridge.rightPeak : 0
    property string statusMessage: "Ready"
    property real selectionStart: 0
    property real selectionEnd: 1
    property real zoomLevel: 1.0
    property real scrollOffset: 0
    property bool isPlaying: AudioBridge ? AudioBridge.isPlaying : false
    property bool isLooping: AudioBridge ? AudioBridge.isLoopEnabled : false
    property real playbackPosition: AudioBridge ? AudioBridge.position : 0
    property real totalDuration: AudioBridge ? AudioBridge.duration : 1.0
    property real totalSamples: AudioBridge ? AudioBridge.getSampleCount() : 0

    function formatTime(seconds) {
        var h = Math.floor(seconds / 3600)
        var m = Math.floor((seconds % 3600) / 60)
        var s = Math.floor(seconds % 60)
        var ms = Math.floor((seconds % 1) * 1000)
        if (h > 0) return h + ":" + (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s + "." + (ms < 100 ? "0" : "") + (ms < 10 ? "0" : "") + ms
        return m + ":" + (s < 10 ? "0" : "") + s + "." + (ms < 100 ? "0" : "") + (ms < 10 ? "0" : "") + ms
    }

    FileDialog {
        id: openDialog
        title: "Open Audio File"
        nameFilters: ["Audio files (*.wav *.mp3 *.ogg *.flac *.aiff)", "All files (*)"]
        onAccepted: { if (AudioBridge) AudioBridge.loadAudio(selectedFile.toString().replace("file:///", "")) }
    }

    FileDialog {
        id: saveDialog
        title: "Save Audio File"
        nameFilters: ["WAV files (*.wav)", "OGG files (*.ogg)", "MP3 files (*.mp3)"]
        onAccepted: { if (AudioBridge) AudioBridge.saveAudio(selectedFile.toString().replace("file:///", "")) }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ═══════════════════════════════════════════════════════════
        // UPPER TOOLBAR - File / Edit / View / Tool commands
        // ═══════════════════════════════════════════════════════════
        Rectangle {
            height: 32
            color: cPanel
            Layout.fillWidth: true
            border.color: cBorder; border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                spacing: 2

                Repeater {
                    model: [
                        { label: "New", tip: "New (Ctrl+N)" },
                        { label: "Open", tip: "Open (Ctrl+O)" },
                        { label: "Save", tip: "Save (Ctrl+S)" },
                        { label: "SaveAs", tip: "Save As" }
                    ]
                    delegate: Rectangle {
                        width: 52; height: 26; radius: 3
                        color: tb1Ma.containsMouse ? "#3c3c3c" : "transparent"
                        Text { anchors.centerIn: parent; text: modelData.label; color: "#ccc"; font.pixelSize: 10 }
                        MouseArea { id: tb1Ma; anchors.fill: parent; hoverEnabled: true
                            onClicked: {
                                if (index === 0) { if (AudioBridge) AudioBridge.newAudio(2, 44100, 16) }
                                else if (index === 1) openDialog.open()
                                else if (index === 2) { if (AudioBridge) AudioBridge.saveAudio(currentFileName) }
                                else if (index === 3) saveDialog.open()
                            }
                        }
                        ToolTip { visible: tb1Ma.containsMouse; text: modelData.tip }
                    }
                }

                Rectangle { width: 1; height: 20; color: cBorder }

                Repeater {
                    model: [
                        { label: "Undo", tip: "Undo (Ctrl+Z)" },
                        { label: "Redo", tip: "Redo (Ctrl+Y)" },
                        { label: "Cut", tip: "Cut (Ctrl+X)" },
                        { label: "Copy", tip: "Copy (Ctrl+C)" },
                        { label: "Paste", tip: "Paste (Ctrl+V)" },
                        { label: "Del", tip: "Delete (Del)" }
                    ]
                    delegate: Rectangle {
                        width: 46; height: 26; radius: 3
                        color: tb2Ma.containsMouse ? "#3c3c3c" : "transparent"
                        Text { anchors.centerIn: parent; text: modelData.label; color: "#ccc"; font.pixelSize: 10 }
                        MouseArea { id: tb2Ma; anchors.fill: parent; hoverEnabled: true
                            onClicked: {
                                if (index === 0 && AudioBridge) AudioBridge.undo()
                                else if (index === 1 && AudioBridge) AudioBridge.redo()
                                else if (index === 2 && AudioBridge) AudioBridge.cut()
                                else if (index === 3 && AudioBridge) AudioBridge.copy()
                                else if (index === 4 && AudioBridge) AudioBridge.paste()
                                else if (index === 5 && AudioBridge) AudioBridge.deleteSelection()
                            }
                        }
                        ToolTip { visible: tb2Ma.containsMouse; text: modelData.tip }
                    }
                }

                Rectangle { width: 1; height: 20; color: cBorder }

                Repeater {
                    model: [
                        { label: "Trim", tip: "Trim to selection" },
                        { label: "Crop", tip: "Crop selection" },
                        { label: "Sel All", tip: "Select All (Ctrl+A)" },
                        { label: "Sel End", tip: "Select to end" }
                    ]
                    delegate: Rectangle {
                        width: 50; height: 26; radius: 3
                        color: tb3Ma.containsMouse ? "#3c3c3c" : "transparent"
                        Text { anchors.centerIn: parent; text: modelData.label; color: "#ccc"; font.pixelSize: 10 }
                        MouseArea { id: tb3Ma; anchors.fill: parent; hoverEnabled: true
                            onClicked: {
                                if (index === 0 && AudioBridge) AudioBridge.trimToSelection()
                                else if (index === 1 && AudioBridge) AudioBridge.cropSelection()
                                else if (index === 2 && AudioBridge) AudioBridge.selectAll()
                                else if (index === 3 && AudioBridge) AudioBridge.selectToEnd()
                            }
                        }
                        ToolTip { visible: tb3Ma.containsMouse; text: modelData.tip }
                    }
                }

                Rectangle { width: 1; height: 20; color: cBorder }

                Repeater {
                    model: [
                        { label: "Z+", tip: "Zoom In" },
                        { label: "Z-", tip: "Zoom Out" },
                        { label: "ZFit", tip: "Zoom to fit" },
                        { label: "ZSel", tip: "Zoom to selection" }
                    ]
                    delegate: Rectangle {
                        width: 42; height: 26; radius: 3
                        color: tb4Ma.containsMouse ? "#3c3c3c" : "transparent"
                        Text { anchors.centerIn: parent; text: modelData.label; color: "#ccc"; font.pixelSize: 10 }
                        MouseArea { id: tb4Ma; anchors.fill: parent; hoverEnabled: true
                            onClicked: {
                                if (index === 0) zoomLevel = Math.min(100, zoomLevel * 2)
                                else if (index === 1) zoomLevel = Math.max(0.1, zoomLevel / 2)
                                else if (index === 2) zoomLevel = 1.0
                                else if (index === 3) zoomLevel = Math.max(0.1, 1.0 / Math.max(0.01, selectionEnd - selectionStart))
                            }
                        }
                        ToolTip { visible: tb4Ma.containsMouse; text: modelData.tip }
                    }
                }

                Item { Layout.fillWidth: true }

                Text { text: currentFileName; color: cMuted; font.pixelSize: 10 }
            }
        }

        // ═══════════════════════════════════════════════════════════
        // LOWER TOOLBAR - Effect commands
        // ═══════════════════════════════════════════════════════════
        Rectangle {
            height: 30
            color: "#2a2a2a"
            Layout.fillWidth: true
            border.color: cBorder; border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                spacing: 2

                Repeater {
                    model: [
                        { label: "Volume", tip: "Change Volume" },
                        { label: "Fade In", tip: "Fade In" },
                        { label: "Fade Out", tip: "Fade Out" },
                        { label: "Normalize", tip: "Normalize" },
                        { label: "Reverse", tip: "Reverse" },
                        { label: "Invert", tip: "Invert" },
                        { label: "Compressor", tip: "Compressor" },
                        { label: "EQ", tip: "Equalizer" },
                        { label: "Reverb", tip: "Reverb" },
                        { label: "Delay", tip: "Delay" },
                        { label: "Chorus", tip: "Chorus" },
                        { label: "Flanger", tip: "Flanger" },
                        { label: "Pitch", tip: "Pitch Shift" },
                        { label: "Time Str", tip: "Time Stretch" },
                        { label: "Noise Red", tip: "Noise Reduction" },
                        { label: "Gate", tip: "Noise Gate" },
                        { label: "Limiter", tip: "Limiter" },
                        { label: "Stereo", tip: "Stereo Enhancer" }
                    ]
                    delegate: Rectangle {
                        width: efMa.containsMouse ? 64 : 54
                        height: 24
                        radius: 3
                        color: efMa.containsMouse ? cAccent : "#3c3c3c"
                        Behavior on width { NumberAnimation { duration: 100 } }

                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: efMa.containsMouse ? "#111111" : "#aaa"
                            font.pixelSize: 9
                            font.bold: efMa.containsMouse
                        }

                        MouseArea {
                            id: efMa
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if (index === 0) { if (AudioBridge) AudioBridge.normalize(1.0) }
                                else if (index === 1) { if (AudioBridge) AudioBridge.fadeIn() }
                                else if (index === 2) { if (AudioBridge) AudioBridge.fadeOut() }
                                else if (index === 3) { if (AudioBridge) AudioBridge.normalize(0.95) }
                                else if (index === 4) { if (AudioBridge) AudioBridge.reverse() }
                                else if (index === 5) { if (AudioBridge) AudioBridge.invert() }
                                else if (index === 6) { if (AudioBridge) AudioBridge.applyCompressor(-20, 4, 5, 100, 0) }
                                else if (index === 7) { if (AudioEffects) AudioEffects.applyEqPreset("Flat") }
                                else if (index === 8) { if (AudioBridge) AudioBridge.applyReverb(0.5, 0.5, 0.3) }
                                else if (index === 9) { if (AudioBridge) AudioBridge.applyDelay(200, 0.3, 0.5) }
                                else if (index === 10) { if (AudioBridge) AudioBridge.applyChorus(0.5, 0.5, 0.4) }
                                else if (index === 11) { if (AudioBridge) AudioBridge.applyFlanger(0.5, 0.5, 0.4) }
                                else if (index === 12) { if (AudioBridge) AudioBridge.pitchShift(2) }
                                else if (index === 13) { if (AudioBridge) AudioBridge.timeStretch(1.0) }
                                else if (index === 16) { if (AudioBridge) AudioBridge.applyLimiter(-1, 5, 50) }
                            }
                        }
                        ToolTip { visible: efMa.containsMouse; text: modelData.tip }
                    }
                }
            }
        }

        // ═══════════════════════════════════════════════════════════
        // MAIN CONTENT AREA
        // ═══════════════════════════════════════════════════════════
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ─── LEFT PANEL: Event Browser ───────────────────────
            Rectangle {
                width: 180
                Layout.fillHeight: true
                color: cPanel
                border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 2

                    Rectangle { height: 20; Layout.fillWidth: true; color: "#333"
                        Text { anchors.centerIn: parent; text: "AC SOUND EVENTS"; color: "#888"; font.pixelSize: 9; font.bold: true }
                    }

                    ScrollView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        ColumnLayout { width: parent.width - 4; spacing: 1
                            Repeater {
                                model: [
                                    { cat: "Engine", events: ["engine_int","engine_ext","turbo","turbo_ext","limiter","gear_ext","gear_int","starter_ext"] },
                                    { cat: "Body", events: ["door","horn","bodywork","chassis_ext","chassis_int"] },
                                    { cat: "Backfire", events: ["backfire_ext","backfire_int"] },
                                    { cat: "Tires", events: ["skid_ext","skid_int","wheel","tractioncontrol_ext"] },
                                    { cat: "Transmission", events: ["transmission","transmission_ext"] },
                                    { cat: "Brakes", events: ["brakes"] },
                                    { cat: "Hybrid", events: ["hybrid_ext","hybrid_int"] },
                                    { cat: "Environment", events: ["wind"] }
                                ]
                                delegate: Column {
                                    width: parent.width
                                    Rectangle { width: parent.width; height: 18; color: catMouse.containsMouse ? "#333" : "transparent"; radius: 2
                                        Text { anchors.left: parent.left; anchors.leftMargin: 4; anchors.verticalCenter: parent.verticalCenter; text: "\u25BC " + modelData.cat; color: "#aaa"; font.pixelSize: 9; font.bold: true }
                                        MouseArea { id: catMouse; anchors.fill: parent; hoverEnabled: true; onClicked: {} }
                                    }
                                    Repeater {
                                        model: modelData.events
                                        delegate: Rectangle {
                                            width: parent.width; height: 16; color: evMouse.containsMouse ? "#334" : (audioEditor.currentFileName.indexOf(modelData) >= 0 ? "#003355" : "transparent"); radius: 1
                                            Text { anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; text: modelData; color: audioEditor.currentFileName.indexOf(modelData) >= 0 ? cAccent : "#777"; font.pixelSize: 8 }
                                            MouseArea { id: evMouse; anchors.fill: parent; hoverEnabled: true
                                                onClicked: { if (AudioBridge) AudioBridge.loadAudio("events/" + modelData + ".wav") }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ─── CENTER: Waveform + Overview ─────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: cBg

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 2
                    spacing: 2

                    // File tab bar
                    Rectangle {
                        Layout.fillWidth: true; height: 24; color: "#252526"
                        Row {
                            anchors.fill: parent; anchors.leftMargin: 4; spacing: 2
                            Rectangle {
                                width: tabLabel.contentWidth + 20; height: 22; radius: 3
                                color: "#333"
                                Row { anchors.centerIn: parent; spacing: 4
                                    Text { id: tabLabel; text: currentFileName; color: cAccent; font.pixelSize: 9; anchors.verticalCenter: parent.verticalCenter }
                                    Rectangle { width: 12; height: 12; radius: 6; color: closeTabMa.containsMouse ? "#E10600" : "#555"
                                        Text { anchors.centerIn: parent; text: "x"; color: "#fff"; font.pixelSize: 8 }
                                        MouseArea { id: closeTabMa; anchors.fill: parent; hoverEnabled: true }
                                    }
                                }
                            }
                        }
                    }

                    // Waveform display - Left Channel
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#111111"; border.color: "#222"; border.width: 1

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 2; spacing: 0

                            Rectangle {
                                Layout.fillWidth: true; height: 16; color: "transparent"
                                Text { anchors.left: parent.left; anchors.leftMargin: 4; anchors.verticalCenter: parent.verticalCenter; text: "L"; color: cWaveL; font.pixelSize: 9; font.bold: true }
                            }

                            Rectangle {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                color: "#0a0a0a"

                                Canvas {
                                    id: waveCanvasL
                                    anchors.fill: parent
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        if (!AudioBridge || AudioBridge.getSampleCount() === 0) {
                                            ctx.fillStyle = "#333"
                                            ctx.font = "11px sans-serif"
                                            ctx.textAlign = "center"
                                            ctx.fillText("No audio loaded", width/2, height/2)
                                            return
                                        }
                                        var data = AudioBridge.getWaveformData(Math.floor(width))
                                        if (data.length === 0) return
                                        var midY = height / 2
                                        ctx.strokeStyle = cWaveL
                                        ctx.lineWidth = 1
                                        ctx.beginPath()
                                        for (var i = 0; i < data.length; ++i) {
                                            var x = i
                                            var sample = parseFloat(data[i]) * midY * 0.9
                                            ctx.moveTo(x, midY - sample)
                                            ctx.lineTo(x, midY + sample)
                                        }
                                        ctx.stroke()
                                        // Center line
                                        ctx.strokeStyle = "#222"
                                        ctx.beginPath()
                                        ctx.moveTo(0, midY)
                                        ctx.lineTo(width, midY)
                                        ctx.stroke()
                                    }
                                    Connections { target: AudioBridge; function onPositionChanged() { waveCanvasL.requestPaint() } }
                                }

                                // Playback marker
                                Rectangle {
                                    x: playbackPosition / Math.max(0.01, totalDuration) * parent.width
                                    width: 1; height: parent.height; color: "#ffffff"; opacity: 0.7
                                    visible: isPlaying || playbackPosition > 0
                                }

                                // Selection highlight
                                Rectangle {
                                    x: selectionStart * parent.width
                                    width: (selectionEnd - selectionStart) * parent.width
                                    height: parent.height
                                    color: cAccent; opacity: 0.15
                                }
                            }

                            // Right Channel
                            Rectangle {
                                Layout.fillWidth: true; height: 16; color: "transparent"
                                Text { anchors.left: parent.left; anchors.leftMargin: 4; anchors.verticalCenter: parent.verticalCenter; text: "R"; color: cWaveR; font.pixelSize: 9; font.bold: true }
                            }

                            Rectangle {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                color: "#0a0a0a"

                                Canvas {
                                    id: waveCanvasR
                                    anchors.fill: parent
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        if (!AudioBridge || AudioBridge.getSampleCount() === 0) return
                                        var data = AudioBridge.getWaveformData(Math.floor(width))
                                        if (data.length === 0) return
                                        var midY = height / 2
                                        ctx.strokeStyle = cWaveR
                                        ctx.lineWidth = 1
                                        ctx.beginPath()
                                        for (var i = 0; i < data.length; ++i) {
                                            var x = i
                                            var sample = parseFloat(data[i]) * midY * 0.9
                                            ctx.moveTo(x, midY - sample)
                                            ctx.lineTo(x, midY + sample)
                                        }
                                        ctx.stroke()
                                        ctx.strokeStyle = "#222"
                                        ctx.beginPath()
                                        ctx.moveTo(0, midY)
                                        ctx.lineTo(width, midY)
                                        ctx.stroke()
                                    }
                                    Connections { target: AudioBridge; function onPositionChanged() { waveCanvasR.requestPaint() } }
                                }

                                Rectangle {
                                    x: playbackPosition / Math.max(0.01, totalDuration) * parent.width
                                    width: 1; height: parent.height; color: "#ffffff"; opacity: 0.7
                                    visible: isPlaying || playbackPosition > 0
                                }

                                Rectangle {
                                    x: selectionStart * parent.width
                                    width: (selectionEnd - selectionStart) * parent.width
                                    height: parent.height
                                    color: cAccent; opacity: 0.15
                                }
                            }
                        }

                        // Time axis
                        Rectangle {
                            anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
                            height: 18; color: "#1a1a1a"

                            Canvas {
                                id: timeAxisCanvas
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.clearRect(0, 0, width, height)
                                    ctx.fillStyle = "#555"
                                    ctx.font = "8px monospace"
                                    ctx.textAlign = "center"
                                    var dur = totalDuration
                                    if (dur <= 0) dur = 1
                                    var step = dur / 10
                                    for (var i = 0; i <= 10; i++) {
                                        var x = i * width / 10
                                        var t = i * step
                                        var h = Math.floor(t / 3600)
                                        var m = Math.floor((t % 3600) / 60)
                                        var s = Math.floor(t % 60)
                                        var label = m + ":" + (s < 10 ? "0" : "") + s
                                        ctx.fillText(label, x, 12)
                                        ctx.strokeStyle = "#333"
                                        ctx.beginPath()
                                        ctx.moveTo(x, 0)
                                        ctx.lineTo(x, 6)
                                        ctx.stroke()
                                    }
                                }
                                Connections { target: audioEditor; function onTotalDurationChanged() { timeAxisCanvas.requestPaint() } }
                            }
                        }
                    }

                    // Overview strip
                    Rectangle {
                        Layout.fillWidth: true; height: 30
                        color: "#1a1a1a"; border.color: "#222"; border.width: 1

                        Canvas {
                            id: overviewCanvas
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                if (!AudioBridge || AudioBridge.getSampleCount() === 0) return
                                var data = AudioBridge.getWaveformData(Math.floor(width))
                                if (data.length === 0) return
                                var midY = height / 2
                                ctx.strokeStyle = "#444"
                                ctx.lineWidth = 1
                                ctx.beginPath()
                                for (var i = 0; i < data.length; ++i) {
                                    var sample = parseFloat(data[i]) * midY * 0.8
                                    ctx.moveTo(i, midY - sample)
                                    ctx.lineTo(i, midY + sample)
                                }
                                ctx.stroke()
                                // Selection highlight
                                ctx.fillStyle = "rgba(0,170,255,0.25)"
                                ctx.fillRect(selectionStart * width, 0, (selectionEnd - selectionStart) * width, height)
                                // Playback position
                                ctx.strokeStyle = "#fff"
                                ctx.beginPath()
                                ctx.moveTo(playbackPosition / Math.max(0.01, totalDuration) * width, 0)
                                ctx.lineTo(playbackPosition / Math.max(0.01, totalDuration) * width, height)
                                ctx.stroke()
                            }
                            Connections { target: AudioBridge; function onPositionChanged() { overviewCanvas.requestPaint() } }
                        }
                    }
                }
            }

            // ─── RIGHT PANEL: Controls + Metering ────────────────
            Rectangle {
                width: 200
                Layout.fillHeight: true
                color: cPanel
                border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    // Transport controls
                    Rectangle { height: 24; Layout.fillWidth: true; color: "transparent"
                        Text { text: "CONTROLS"; color: "#666"; font.pixelSize: 9; font.bold: true }
                    }

                    GridLayout {
                        columns: 5; Layout.fillWidth: true; columnSpacing: 2; rowSpacing: 2

                        Rectangle { Layout.fillWidth: true; height: 32; radius: 4; color: ctrlRewindMa.containsMouse ? "#333" : "#2a2a2a"
                            Text { anchors.centerIn: parent; text: "\u23EA"; color: "#ccc"; font.pixelSize: 16 }
                            MouseArea { id: ctrlRewindMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: { if (AudioBridge) AudioBridge.seek(Math.max(0, playbackPosition - 5)) }
                            }
                        }
                        Rectangle { Layout.fillWidth: true; height: 32; radius: 4; color: ctrlPlayMa.containsMouse ? "#333" : (isPlaying ? cAccent : "#2a2a2a")
                            Text { anchors.centerIn: parent; text: isPlaying ? "\u23F8" : "\u25B6"; color: isPlaying ? "#111" : "#ccc"; font.pixelSize: 16 }
                            MouseArea { id: ctrlPlayMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: { if (AudioBridge) { if (isPlaying) AudioBridge.pause(); else AudioBridge.play() } }
                            }
                        }
                        Rectangle { Layout.fillWidth: true; height: 32; radius: 4; color: ctrlStopMa.containsMouse ? "#333" : "#2a2a2a"
                            Text { anchors.centerIn: parent; text: "\u23F9"; color: "#ccc"; font.pixelSize: 16 }
                            MouseArea { id: ctrlStopMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: { if (AudioBridge) AudioBridge.stop() }
                            }
                        }
                        Rectangle { Layout.fillWidth: true; height: 32; radius: 4; color: ctrlFFMa.containsMouse ? "#333" : "#2a2a2a"
                            Text { anchors.centerIn: parent; text: "\u23E9"; color: "#ccc"; font.pixelSize: 16 }
                            MouseArea { id: ctrlFFMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: { if (AudioBridge) AudioBridge.seek(Math.min(totalDuration, playbackPosition + 5)) }
                            }
                        }
                        Rectangle { Layout.fillWidth: true; height: 32; radius: 4; color: ctrlLoopMa.containsMouse ? "#333" : (isLooping ? "#004466" : "#2a2a2a")
                            Text { anchors.centerIn: parent; text: "\u21BB"; color: isLooping ? cAccent : "#ccc"; font.pixelSize: 16 }
                            MouseArea { id: ctrlLoopMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: { if (AudioBridge) AudioBridge.isLoopEnabled = !AudioBridge.isLoopEnabled }
                            }
                        }
                    }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    // Volume
                    RowLayout {
                        Text { text: "Vol"; color: "#888"; font.pixelSize: 9; width: 28 }
                        Slider {
                            id: volSlider
                            from: 0; to: 100; value: masterVolume
                            Layout.fillWidth: true; height: 16
                            onValueChanged: masterVolume = value
                            background: Rectangle { x: 0; y: 6; width: parent.width; height: 4; radius: 2; color: "#333"
                                Rectangle { width: parent.width * (volSlider.value / volSlider.to); height: parent.height; color: cAccent; radius: 2 }
                            }
                            handle: Rectangle { x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width); y: 2; width: 12; height: 12; radius: 6; color: "#ddd" }
                        }
                        Text { text: Math.round(masterVolume) + "%"; color: cAccent; font.pixelSize: 9; width: 30 }
                    }

                    // Speed
                    RowLayout {
                        Text { text: "Spd"; color: "#888"; font.pixelSize: 9; width: 28 }
                        Slider {
                            id: speedSlider
                            from: 0.25; to: 4.0; value: playbackSpeed; stepSize: 0.05
                            Layout.fillWidth: true; height: 16
                            onValueChanged: playbackSpeed = value
                            background: Rectangle { x: 0; y: 6; width: parent.width; height: 4; radius: 2; color: "#333"
                                Rectangle { width: parent.width * ((speedSlider.value - 0.25) / (4.0 - 0.25)); height: parent.height; color: "#66aa33"; radius: 2 }
                            }
                            handle: Rectangle { x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width); y: 2; width: 12; height: 12; radius: 6; color: "#ddd" }
                        }
                        Text { text: playbackSpeed.toFixed(2) + "x"; color: "#66aa33"; font.pixelSize: 9; width: 30 }
                    }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    // Position display
                    Rectangle { Layout.fillWidth: true; height: 36; color: "#111"; radius: 4
                        Column {
                            anchors.centerIn: parent
                            spacing: 2
                            Text { text: formatTime(playbackPosition); color: cAccent; font.pixelSize: 16; font.family: "Consolas"; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { text: formatTime(totalDuration); color: "#555"; font.pixelSize: 9; font.family: "Consolas"; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                    }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    // Level meters
                    Rectangle { height: 20; Layout.fillWidth: true; color: "transparent"
                        Text { text: "LEVELS"; color: "#666"; font.pixelSize: 9; font.bold: true }
                    }

                    RowLayout {
                        Text { text: "L"; color: cWaveL; font.pixelSize: 9; width: 12 }
                        Rectangle { Layout.fillWidth: true; height: 12; color: "#111"; radius: 2
                            Rectangle { width: Math.max(2, leftPeakDb * parent.width); height: parent.height; color: leftPeakDb > 0.9 ? "#ff3333" : cWaveL; radius: 2; Behavior on width { NumberAnimation { duration: 50 } } }
                        }
                        Text { text: Math.round(leftPeakDb * 100) + "%"; color: "#666"; font.pixelSize: 8; width: 30 }
                    }

                    RowLayout {
                        Text { text: "R"; color: cWaveR; font.pixelSize: 9; width: 12 }
                        Rectangle { Layout.fillWidth: true; height: 12; color: "#111"; radius: 2
                            Rectangle { width: Math.max(2, rightPeakDb * parent.width); height: parent.height; color: rightPeakDb > 0.9 ? "#ff3333" : cWaveR; radius: 2; Behavior on width { NumberAnimation { duration: 50 } } }
                        }
                        Text { text: Math.round(rightPeakDb * 100) + "%"; color: "#666"; font.pixelSize: 8; width: 30 }
                    }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    // File info
                    Rectangle { height: 20; Layout.fillWidth: true; color: "transparent"
                        Text { text: "FILE INFO"; color: "#666"; font.pixelSize: 9; font.bold: true }
                    }

                    Column { spacing: 2
                        Text { text: "File: " + currentFileName; color: "#777"; font.pixelSize: 8; elide: Text.ElideRight; width: 188 }
                        Text { text: "Duration: " + formatTime(totalDuration); color: "#777"; font.pixelSize: 8 }
                        Text { text: "Samples: " + totalSamples; color: "#777"; font.pixelSize: 8 }
                        Text { text: "Selection: " + formatTime(selectionStart * totalDuration) + " - " + formatTime(selectionEnd * totalDuration); color: "#777"; font.pixelSize: 8 }
                    }

                    Item { Layout.fillHeight: true }

                    // Quick actions
                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    RowLayout {
                        Layout.fillWidth: true
                        Rectangle { Layout.fillWidth: true; height: 28; radius: 4; color: qaUndoMa.containsMouse ? "#333" : "#2a2a2a"
                            Text { anchors.centerIn: parent; text: "Undo"; color: "#aaa"; font.pixelSize: 9 }
                            MouseArea { id: qaUndoMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: { if (AudioBridge) AudioBridge.undo() }
                            }
                        }
                        Rectangle { Layout.fillWidth: true; height: 28; radius: 4; color: qaRedoMa.containsMouse ? "#333" : "#2a2a2a"
                            Text { anchors.centerIn: parent; text: "Redo"; color: "#aaa"; font.pixelSize: 9 }
                            MouseArea { id: qaRedoMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: { if (AudioBridge) AudioBridge.redo() }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 32; radius: 4; color: renderMa.containsMouse ? "#005577" : cAccent
                        Text { anchors.centerIn: parent; text: "RENDER"; color: "#fff"; font.pixelSize: 11; font.bold: true }
                        MouseArea { id: renderMa; anchors.fill: parent; hoverEnabled: true
                            onClicked: { if (AudioBridge) AudioBridge.saveAudio(currentFileName); statusMessage = "Saved" }
                        }
                    }
                }
            }
        }

        // ═══════════════════════════════════════════════════════════
        // STATUS BAR
        // ═══════════════════════════════════════════════════════════
        Rectangle {
            height: 22; color: "#252526"; Layout.fillWidth: true
            border.color: cBorder; border.width: 1

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 12

                Text { text: statusMessage; color: cAccent; font.pixelSize: 9 }
                Rectangle { width: 1; height: 14; color: "#444" }
                Text { text: "Length: " + formatTime(totalDuration); color: cMuted; font.pixelSize: 9 }
                Rectangle { width: 1; height: 14; color: "#444" }
                Text { text: "Sel: " + formatTime(selectionStart * totalDuration) + " - " + formatTime(selectionEnd * totalDuration); color: cMuted; font.pixelSize: 9 }
                Rectangle { width: 1; height: 14; color: "#444" }
                Text { text: "Pos: " + formatTime(playbackPosition); color: cMuted; font.pixelSize: 9 }
                Item { Layout.fillWidth: true }
                Text { text: "Zoom: " + zoomLevel.toFixed(1) + "x"; color: cMuted; font.pixelSize: 9 }
                Rectangle { width: 1; height: 14; color: "#444" }
                Text { text: currentFileName; color: "#555"; font.pixelSize: 9 }
            }
        }
    }

    Connections {
        target: AudioBridge
        function onStatusMessage(msg) { statusMessage = msg }
        function onPositionChanged() {
            waveCanvasL.requestPaint()
            waveCanvasR.requestPaint()
            overviewCanvas.requestPaint()
        }
    }
}
