import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window 2.15
import Qt.labs.platform 1.1
import ksEditor.Audio 1.0
import ksEditor.AudioEngine 1.0
import ksEditor.AudioEffects 1.0
import ksEditor.AudioModule 1.0
import "../modules/Audio/AudioStudio"
import "../widgets"

// ============================================================================
// KS Audio Studio
// Sound-engine event authoring workspace modeled on a modern studio DAW:
// asset browser (left), event editor with timeline lanes + automation and a
// mixer strip (center), and the full mixer / tools as workspace tabs.
// Chrome (ribbon + command bar + status bar) follows the KS Modeler visual
// language: dark panels, blue selection accent, red brand accent, custom
// MouseArea widgets, uiScale-scaled layout.
// ============================================================================

Rectangle {
    id: root
    width: 1280
    height: 720
    color: "#111111"
    focus: true

    property real baseUiScale: 1.18
    property real uiZoom: 1.0
    property real uiScale: baseUiScale * uiZoom

    // ---- KS Modeler design tokens ----
    readonly property color cAccent:   "#569cd6"
    readonly property color cBrand:    "#E10600"
    readonly property color cBg:       "#111111"
    readonly property color cPanel:    "#1e1e1e"
    readonly property color cHead:     "#1f1f22"
    readonly property color cSub:      "#2d2d30"
    readonly property color cBorder:   "#2d2d30"
    readonly property color cBorderL:  "#3f3f46"
    readonly property color cHover:    "#3e3e42"
    readonly property color cSelected: "#264f78"
    readonly property color cText:     "#cccccc"
    readonly property color cMuted:    "#888888"
    readonly property color cDim:      "#5a5a60"
    readonly property color cWave:     "#00e6b8"

    // ---- State ----
    property string currentProject: "untitled"
    property string activePanel: "timeline"
    property bool browserVisible: true
    property bool isPlaying: AudioBridge ? AudioBridge.isPlaying : false
    property bool isRecording: AudioBridge ? AudioBridge.isRecording : false
    property real masterVolume: 80
    property real leftPeakDb: AudioBridge ? AudioBridge.leftPeak : 0
    property real rightPeakDb: AudioBridge ? AudioBridge.rightPeak : 0
    property string selectedEvent: ""
    property string statusMessage: "Ready"
    property bool isModified: false
    property string browserCat: "events"
    property string searchText: ""
    property int posMs: AudioBridge ? AudioBridge.position : 0
    property int durMs: (AudioBridge && AudioBridge.duration > 0) ? AudioBridge.duration : 30000
    property int activeRibbonTab: 0
    property string commandEcho: "Ready"
    property bool eventActive: true
    property bool eventSpatial: true

    property var expandedNodes: ({ Engine: true, Body: true, Backfire: true })

    // ---- Workspace tabs (Studio views) ----
    property var panels: [
        { key: "timeline",  label: "Event Editor", icon: "\u23F1" },
        { key: "mixer",     label: "Mixer",        icon: "\uF001" },
        { key: "events",    label: "Events",       icon: "\u266B" },
        { key: "effects",   label: "Effects",      icon: "\uF0E7" },
        { key: "recording", label: "Recording",    icon: "\u25CF" },
        { key: "banks",     label: "Sound Banks",  icon: "\uF1C0" },
        { key: "batch",     label: "Batch",        icon: "\uF0AE" },
        { key: "export",    label: "Export",       icon: "\uF019" }
    ]

    // ---- Browser categories (asset groups) ----
    property var browserGroups: [
        { key: "events",    label: "Events",    icon: "\u266B" },
        { key: "buses",     label: "Buses",     icon: "\uF001" },
        { key: "vcas",      label: "VCAs",      icon: "\uF124" },
        { key: "presets",   label: "Mixer Presets", icon: "\uF0E7" },
        { key: "snapshots", label: "Snapshots", icon: "\uF030" },
        { key: "banks",     label: "Banks",     icon: "\uF1C0" },
        { key: "sounds",    label: "Sounds",    icon: "\uF2B7" }
    ]

    property var eventCats: [
        { name: "Engine", events: ["engine_int","engine_ext","turbo","turbo_ext","limiter","gear_ext","gear_int","gear_grind","starter_ext","starter_int","ignition_ext","ignition_int","misc_int"] },
        { name: "Body", events: ["door","horn","bodywork","chassis_ext","chassis_int"] },
        { name: "Backfire", events: ["backfire_ext","backfire_int"] },
        { name: "Tires", events: ["skid_ext","skid_int","wheel","tractioncontrol_ext","tractioncontrol_int"] },
        { name: "Transmission", events: ["transmission","transmission_ext"] },
        { name: "Brakes", events: ["brakes"] },
        { name: "Hybrid", events: ["hybrid_ext","hybrid_int"] },
        { name: "Environment", events: ["wind"] },
        { name: "CSP Rain", events: ["rain_amb","rain_car_ext","rain_car_int","rain_skid_ext"] },
        { name: "CSP Vehicle", events: ["wiper_car_int","handbrake_int"] }
    ]

    // ---- Event editor mixer strips (tracks + buses) ----
    property var mixStrips: [
        { name: "Track 1",    out: "engine", seed: 1 },
        { name: "Track 2",    out: "engine", seed: 2 },
        { name: "Scatterer",  out: "fx",     seed: 3 },
        { name: "engine",     out: "master", seed: 4 },
        { name: "fx",         out: "master", seed: 5 },
        { name: "ui",         out: "master", seed: 6 }
    ]

    // ---- Ribbon definitions ----
    property var ribbonDefs: [
        { title: "File", groups: [
            { name: "Project", buttons: [
                { label: "New", icon: "\u2795", cmd: "new" },
                { label: "Open", icon: "\u2601", cmd: "open" },
                { label: "Save", icon: "\u2913", cmd: "save" },
                { label: "Save As", icon: "\u29C0", cmd: "saveas" }
            ]},
            { name: "Assets", buttons: [
                { label: "Import", icon: "\u2B07", cmd: "import" },
                { label: "Export", icon: "\u2B06", cmd: "export" },
                { label: "Banks", icon: "\uF1C0", cmd: "banks" }
            ]}
        ]},
        { title: "Edit", groups: [
            { name: "History", buttons: [
                { label: "Undo", icon: "\u21B6", cmd: "undo" },
                { label: "Redo", icon: "\u21B7", cmd: "redo" }
            ]},
            { name: "Clipboard", buttons: [
                { label: "Cut", icon: "\u2702", cmd: "cut" },
                { label: "Copy", icon: "\u2398", cmd: "copy" },
                { label: "Paste", icon: "\u2396", cmd: "paste" },
                { label: "Delete", icon: "\u2715", cmd: "delete" }
            ]}
        ]},
        { title: "View", groups: [
            { name: "Workspace", buttons: [
                { label: "Event", icon: "\u23F1", cmd: "panel_timeline", check: function(){ return activePanel === "timeline" } },
                { label: "Mixer", icon: "\uF001", cmd: "panel_mixer", check: function(){ return activePanel === "mixer" } },
                { label: "Events", icon: "\u266B", cmd: "panel_events", check: function(){ return activePanel === "events" } },
                { label: "Effects", icon: "\uF0E7", cmd: "panel_effects", check: function(){ return activePanel === "effects" } },
                { label: "Record", icon: "\u25CF", cmd: "panel_recording", check: function(){ return activePanel === "recording" } },
                { label: "Banks", icon: "\uF1C0", cmd: "panel_banks", check: function(){ return activePanel === "banks" } },
                { label: "Batch", icon: "\uF0AE", cmd: "panel_batch", check: function(){ return activePanel === "batch" } },
                { label: "Export", icon: "\uF019", cmd: "panel_export", check: function(){ return activePanel === "export" } }
            ]},
            { name: "Panels", buttons: [
                { label: "Browser", icon: "\u2630", cmd: "browser", check: function(){ return browserVisible } },
                { label: "Reset", icon: "\u21BB", cmd: "resetlayout" }
            ]}
        ]},
        { title: "Transport", groups: [
            { name: "Transport", buttons: [
                { label: "Play", icon: "\u25B6", cmd: "play", check: function(){ return isPlaying } },
                { label: "Pause", icon: "\u23F8", cmd: "pause" },
                { label: "Stop", icon: "\u25A0", cmd: "stop" },
                { label: "Record", icon: "\u25CF", cmd: "record", check: function(){ return isRecording } },
                { label: "Loop", icon: "\u21BA", cmd: "loop", check: function(){ return AudioBridge && AudioBridge.isLoopEnabled } }
            ]}
        ]},
        { title: "Project", groups: [
            { name: "Output", buttons: [
                { label: "Build Banks", icon: "\uF1C0", cmd: "buildbanks" },
                { label: "Settings", icon: "\u2699", cmd: "settings" }
            ]},
            { name: "Info", buttons: [
                { label: "Sample Rate", icon: "\uF03D", cmd: "samplerate" },
                { label: "Bit Depth", icon: "\uF03D", cmd: "bitdepth" }
            ]}
        ]},
        { title: "Help", groups: [
            { name: "Help", buttons: [
                { label: "About", icon: "\u2139", cmd: "about" },
                { label: "Shortcuts", icon: "\u2328", cmd: "shortcuts" }
            ]}
        ]}
    ]

    // ---- Helpers ----
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

    function selectEvent(name) {
        selectedEvent = name
        if (AudioBridge) AudioBridge.loadAudio("events/" + name + ".wav")
        statusMessage = "Event: " + name
    }

    function fmtTime(ms) {
        var m = Math.floor(ms / 60000)
        var s = Math.floor((ms % 60000) / 1000)
        var c = Math.floor((ms % 1000) / 10)
        return m + ":" + (s < 10 ? "0" : "") + s + "." + (c < 10 ? "0" : "") + c
    }

    function rulerTicks() {
        var dur = Math.max(1000, durMs)
        var step = dur <= 30000 ? 1000 : dur <= 120000 ? 5000 : 10000
        var ticks = []
        var n = Math.ceil(dur / step)
        for (var i = 0; i <= n; ++i) {
            var t = i * step
            ticks.push({ ms: t, label: Math.floor(t / 60000) + ":" + (Math.floor((t % 60000) / 1000) < 10 ? "0" : "") + Math.floor((t % 60000) / 1000) })
        }
        return ticks
    }

    function runCommand(cmd) {
        commandEcho = "Command: " + cmd
        var c = String(cmd).trim().toLowerCase()
        if (c === "new") { if (AudioModule) AudioModule.newProject(); currentProject = "untitled"; isModified = false }
        else if (c === "open") { openProjectDialog.open() }
        else if (c === "import") { importDialog.open() }
        else if (c === "save") { saveProject() }
        else if (c === "saveas") { saveProjectDialog.open() }
        else if (c === "export") { exportDialog.open() }
        else if (c === "banks") { activePanel = "banks" }
        else if (c === "buildbanks") { if (AudioModule) AudioModule.onBuildBanks() }
        else if (c === "undo") { if (AudioBridge) AudioBridge.undo() }
        else if (c === "redo") { if (AudioBridge) AudioBridge.redo() }
        else if (c === "cut") { if (AudioBridge) AudioBridge.cut() }
        else if (c === "copy") { if (AudioBridge) AudioBridge.copy() }
        else if (c === "paste") { if (AudioBridge) AudioBridge.paste() }
        else if (c === "delete") { if (AudioBridge) AudioBridge.deleteSelection() }
        else if (c === "selectall") { if (AudioBridge) AudioBridge.selectAll() }
        else if (c === "play") { togglePlay() }
        else if (c === "pause") { if (AudioBridge && AudioBridge.isPlaying) AudioBridge.pause() }
        else if (c === "stop") { stopPlayback() }
        else if (c === "record") { toggleRecord() }
        else if (c === "loop") { if (AudioBridge) AudioBridge.setLoopEnabled(!AudioBridge.isLoopEnabled) }
        else if (c.indexOf("panel_") === 0) { activePanel = c.substring(6) }
        else if (c === "browser") { browserVisible = !browserVisible }
        else if (c === "resetlayout") { browserVisible = true; activePanel = "timeline" }
        else if (c === "settings") { statusMessage = "Audio settings" }
        else if (c === "samplerate") { statusMessage = "Project sample rate: 44100 Hz" }
        else if (c === "bitdepth") { statusMessage = "Project bit depth: 16-bit" }
        else if (c === "about") { aboutDialog.open() }
        else if (c === "shortcuts") { shortcutDialog.open() }
        else statusMessage = "Unknown command: " + c
    }

    function drawCurve(canvas, type) {
        var ctx = canvas.getContext("2d")
        var w = canvas.width, h = canvas.height
        ctx.clearRect(0, 0, w, h)
        if (w < 2) return
        var seed = (selectedEvent + type).length || 7
        var col = type === "rpm" ? cAccent : type === "throttle" ? cWave : "#d29445"
        var pts = []
        for (var x = 0; x <= w; x += 4) {
            var t = x / w
            var v
            if (type === "rpm") {
                v = 0.15 + 0.6 * Math.max(0, Math.min(1, Math.sin(t * Math.PI + 0.35)))
                v += 0.1 * Math.sin(t * 14 + seed)
            } else if (type === "throttle") {
                v = t < 0.1 ? 0.02 : (t < 0.5 ? (t - 0.1) / 0.4 : 0.82 + 0.1 * Math.sin(t * 22))
            } else {
                v = Math.max(0, Math.sin((t - 0.42) * Math.PI * 2.6)) * 0.85
            }
            v = Math.max(0.02, Math.min(0.98, v))
            pts.push({ x: x, y: h - 2 - v * (h - 6) })
        }
        // glow underlay
        ctx.globalAlpha = 0.1
        ctx.strokeStyle = col
        ctx.lineWidth = 7
        ctx.beginPath()
        for (var i = 0; i < pts.length; ++i) {
            if (i === 0) ctx.moveTo(pts[i].x, pts[i].y); else ctx.lineTo(pts[i].x, pts[i].y)
        }
        ctx.stroke()
        // crisp curve
        ctx.globalAlpha = 1
        ctx.lineWidth = 1.6
        ctx.beginPath()
        for (i = 0; i < pts.length; ++i) {
            if (i === 0) ctx.moveTo(pts[i].x, pts[i].y); else ctx.lineTo(pts[i].x, pts[i].y)
        }
        ctx.stroke()
        ctx.globalAlpha = 1
    }

    onSelectedEventChanged: {
        if (waveCanvas) waveCanvas.requestPaint()
        if (rpmCanvas) rpmCanvas.requestPaint()
        if (thrCanvas) thrCanvas.requestPaint()
        if (turboCanvas) turboCanvas.requestPaint()
    }

    // ==========================================================================
    // SCALED CANVAS (KS Modeler layout system: 1280x720 design space, uiScale)
    // ==========================================================================
    Item {
        width: parent.width / uiScale
        height: parent.height / uiScale
        scale: uiScale
        transformOrigin: Item.TopLeft

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ==================================================================
            // RIBBON (acts as the window title bar)
            // ==================================================================
            Rectangle {
                Layout.fillWidth: true
                height: 96
                color: "#2d2d30"
                border.color: "#3f3f46"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        spacing: 0

                        // Brand / drag handle
                        Rectangle {
                            width: 40; Layout.fillHeight: true
                            color: "#1f1f22"
                            MouseArea {
                                anchors.fill: parent
                                onPressed: { if (winBridge) winBridge.beginMove() }
                            }
                            Text {
                                anchors.centerIn: parent
                                text: "KS"
                                color: "#E10600"; font.pixelSize: 13; font.bold: true
                            }
                        }

                        // Ribbon tabs
                        Repeater {
                            model: ribbonDefs
                            delegate: Rectangle {
                                Layout.fillHeight: true
                                width: Math.max(ribbonTabTxt.implicitWidth + 20, 64)
                                color: activeRibbonTab === index ? "#3e3e42" : (ribbonTabHover.containsMouse ? "#333336" : "transparent")
                                border.color: activeRibbonTab === index ? "#569cd6" : "transparent"
                                border.width: 1
                                Text {
                                    id: ribbonTabTxt
                                    anchors.centerIn: parent
                                    text: modelData.title
                                    color: activeRibbonTab === index ? "#569cd6" : "#ccc"
                                    font.pixelSize: 11; font.bold: activeRibbonTab === index
                                }
                                MouseArea { id: ribbonTabHover; anchors.fill: parent; hoverEnabled: true
                                    onClicked: activeRibbonTab = index
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Current project
                        Rectangle {
                            width: 120; height: 22; radius: 3; color: "#18181b"; border.color: "#3f3f46"; border.width: 1
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideMiddle
                                text: currentProject + (isModified ? " *" : "")
                                color: "#aaa"; font.pixelSize: 9
                            }
                        }

                        Rectangle { width: 1; height: 22; color: "#3f3f46"; anchors.verticalCenter: parent.verticalCenter }

                        // Window controls
                        Repeater {
                            model: [
                                { icon: "\u2013", tip: "Minimize", act: function(){ if (winBridge) winBridge.minimize() } },
                                { icon: "\u2750", tip: "Maximize", act: function(){ if (winBridge) winBridge.toggleMaximize() } },
                                { icon: "\u2715", tip: "Close", act: function(){ if (winBridge) winBridge.closeWindow() } }
                            ]
                            delegate: Rectangle {
                                width: 42; height: 32
                                Layout.fillHeight: true
                                color: (modelData.icon === "\u2715" && wbBtn.containsMouse) ? "#E81123"
                                     : wbBtn.containsMouse ? "#3e3e42" : "transparent"
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.icon; color: "#ccc"; font.pixelSize: 12
                                }
                                MouseArea { id: wbBtn; anchors.fill: parent; hoverEnabled: true
                                    onClicked: modelData.act()
                                }
                                ToolTip { visible: wbBtn.containsMouse; text: modelData.tip }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 64
                        spacing: 0

                        Repeater {
                            model: ribbonDefs[activeRibbonTab] ? ribbonDefs[activeRibbonTab].groups : []
                            delegate: Column {
                                Layout.fillHeight: true
                                width: Math.max(rgName.implicitWidth + 12, ribbonGroupRow.width)
                                spacing: 0

                                RowLayout {
                                    id: ribbonGroupRow
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    height: 46
                                    spacing: 1

                                    Repeater {
                                        model: modelData.buttons
                                        delegate: Rectangle {
                                            width: 46; height: 44
                                            radius: 2
                                            color: rbBtnHover.containsMouse ? "#3e3e42" : "transparent"
                                            border.color: modelData.check !== undefined && modelData.check() ? "#569cd6" : "transparent"
                                            border.width: 1
                                            Column {
                                                anchors.centerIn: parent; spacing: 2
                                                Text {
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    text: modelData.icon
                                                    color: modelData.check !== undefined && modelData.check() ? "#569cd6" : "#bbb"
                                                    font.pixelSize: 16
                                                }
                                                Text {
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    width: 44; elide: Text.ElideMiddle
                                                    text: modelData.label
                                                    color: "#999"; font.pixelSize: 8
                                                }
                                            }
                                            MouseArea { id: rbBtnHover; anchors.fill: parent; hoverEnabled: true
                                                onClicked: runCommand(modelData.cmd)
                                            }
                                        }
                                    }
                                }

                                Text {
                                    id: rgName
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: modelData.name
                                    color: "#666"; font.pixelSize: 8
                                }
                            }
                        }
                    }
                }
            }

            // ==================================================================
            // COMMAND BAR
            // ==================================================================
            Rectangle {
                height: 26
                color: "#1f1f22"
                Layout.fillWidth: true
                border.color: "#3f3f46"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    Text {
                        text: "Command:"
                        color: "#569cd6"; font.pixelSize: 11; font.bold: true
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 18
                        radius: 2
                        color: "#18181b"
                        border.color: "#3f3f46"
                        border.width: 1
                        TextInput {
                            id: commandInput
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            verticalAlignment: Text.AlignVCenter
                            color: "#e0e0e0"; font.pixelSize: 11
                            clip: true
                            Keys.onReturnPressed: { runCommand(commandInput.text); commandInput.text = "" }
                            Keys.onEnterPressed: { runCommand(commandInput.text); commandInput.text = "" }
                        }
                    }
                    Text {
                        text: commandEcho
                        color: "#10b981"; font.pixelSize: 10
                        elide: Text.ElideRight
                        Layout.maximumWidth: 320
                    }
                }
            }

            // ==================================================================
            // MAIN WORKSPACE
            // ==================================================================
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // ----------------------------------------------------------------
                // LEFT: ASSET BROWSER
                // ----------------------------------------------------------------
                Rectangle {
                    id: browserPanel
                    Layout.preferredWidth: 300
                    Layout.fillHeight: true
                    visible: browserVisible
                    color: "#1e1e1e"
                    border.color: "#2d2d30"; border.width: 1
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Browser header with search
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            color: "#1f1f22"
                            border.color: "#2d2d30"; border.width: 1

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10; anchors.rightMargin: 8
                                anchors.topMargin: 5; anchors.bottomMargin: 5
                                spacing: 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Text { text: "BROWSER"; color: cAccent; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; Layout.fillWidth: true }
                                    Rectangle {
                                        width: 20; height: 18; radius: 2; color: brAddHover.containsMouse ? cHover : "transparent"
                                        Text { anchors.centerIn: parent; text: "\u2795"; color: cText; font.pixelSize: 11 }
                                        MouseArea { id: brAddHover; anchors.fill: parent; hoverEnabled: true; onClicked: importDialog.open() }
                                        ToolTip { visible: brAddHover.containsMouse; text: "Add Audio Files" }
                                    }
                                }
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 18
                                    radius: 2; color: "#18181b"; border.color: "#3f3f46"; border.width: 1
                                    RowLayout {
                                        anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 4
                                        Text { text: "\u2315"; color: cDim; font.pixelSize: 10 }
                                        TextInput {
                                            Layout.fillWidth: true
                                            clip: true; verticalAlignment: Text.AlignVCenter
                                            color: cText; font.pixelSize: 10
                                            onTextChanged: searchText = text
                                            Keys.onEscapePressed: { text = ""; focus = false }
                                        }
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 0

                            // Icon rail
                            Rectangle {
                                Layout.preferredWidth: 40
                                Layout.fillHeight: true
                                color: "#1f1f22"
                                border.color: "#2d2d30"; border.width: 1

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.topMargin: 4; spacing: 2
                                    Repeater {
                                        model: browserGroups
                                        delegate: Rectangle {
                                            Layout.preferredWidth: 32; Layout.preferredHeight: 30
                                            Layout.alignment: Qt.AlignHCenter
                                            radius: 3
                                            color: browserCat === modelData.key ? cSelected : (railHover.containsMouse ? cHover : "transparent")
                                            border.color: browserCat === modelData.key ? cAccent : "transparent"
                                            border.width: 1
                                            Text {
                                                anchors.centerIn: parent
                                                text: modelData.icon; color: browserCat === modelData.key ? "#ffffff" : cMuted
                                                font.pixelSize: 13
                                            }
                                            MouseArea { id: railHover; anchors.fill: parent; hoverEnabled: true
                                                onClicked: browserCat = modelData.key
                                            }
                                            ToolTip { visible: railHover.containsMouse; text: modelData.label }
                                        }
                                    }
                                    Item { Layout.fillHeight: true }
                                }
                            }

                            // Tree / list
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "#1a1a1e"

                                ListView {
                                    id: browserList
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    clip: true
                                    spacing: 1
                                    model: browserCat === "events" ? eventCats
                                        : browserCat === "banks" ? (AudioEngine ? AudioEngine.loadedBanks : [])
                                        : browserCat === "buses" ? (AudioEngine ? AudioEngine.getBuses() : ["master","engine","fx","ui"])
                                        : browserCat === "vcas" ? ["Vehicle","Environment","SFX","Music"]
                                        : browserCat === "presets" ? ["Default","Cinematic","Impact","Ambience"]
                                        : browserCat === "snapshots" ? ["Default Snapshot","Replay"]
                                        : ["skid_loop.wav","wind_ext.wav","door_close.wav","horn.wav","rain_car_int.wav"]
                                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                    delegate: Item {
                                        width: ListView.view.width
                                        height: browserCat === "events"
                                            ? ((expandedNodes[modelData.name] === true) ? 24 + modelData.events.length * 21 : 24)
                                            : 22

                                        Column {
                                            visible: browserCat === "events"
                                            anchors.fill: parent
                                            spacing: 1

                                            Rectangle {
                                                width: parent.width; height: 23; radius: 2
                                                color: catHover.containsMouse ? cHover : "transparent"
                                                RowLayout {
                                                    anchors.fill: parent; anchors.leftMargin: 4; spacing: 3
                                                    Text {
                                                        text: (expandedNodes[modelData.name] === true) ? "\u25BC" : "\u25B6"
                                                        color: cMuted; font.pixelSize: 8
                                                    }
                                                    Text {
                                                        text: modelData.name.toUpperCase()
                                                        color: cText; font.pixelSize: 9; font.bold: true
                                                    }
                                                    Item { Layout.fillWidth: true }
                                                    Text {
                                                        text: modelData.events.length + ""
                                                        color: cDim; font.pixelSize: 8
                                                    }
                                                }
                                                MouseArea { id: catHover; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: {
                                                        var e = expandedNodes
                                                        e[modelData.name] = !(e[modelData.name] === true)
                                                        expandedNodes = e
                                                    }
                                                }
                                            }

                                            Repeater {
                                                model: (expandedNodes[modelData.name] === true) ? modelData.events : []
                                                delegate: Rectangle {
                                                    width: parent.width
                                                    height: 20
                                                    radius: 2
                                                    color: selectedEvent === modelData ? cSelected : (evHover.containsMouse ? cHover : "transparent")
                                                    border.color: selectedEvent === modelData ? cAccent : "transparent"
                                                    border.width: 1
                                                    RowLayout {
                                                        anchors.left: parent.left; anchors.leftMargin: 16
                                                        anchors.right: parent.right; anchors.rightMargin: 4
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        spacing: 4
                                                        Text { text: "\u266B"; color: selectedEvent === modelData ? "#ffffff" : "#7fb069"; font.pixelSize: 8 }
                                                        Text {
                                                            text: modelData
                                                            color: selectedEvent === modelData ? "#ffffff" : cMuted
                                                            font.pixelSize: 9; elide: Text.ElideMiddle
                                                            Layout.fillWidth: true
                                                        }
                                                    }
                                                    MouseArea { id: evHover; anchors.fill: parent; hoverEnabled: true
                                                        onClicked: selectEvent(modelData)
                                                    }
                                                }
                                            }
                                        }

                                        Rectangle {
                                            visible: browserCat !== "events"
                                            anchors.fill: parent
                                            radius: 2
                                            color: brFlatHover.containsMouse ? cHover : "transparent"
                                            Text {
                                                anchors.left: parent.left; anchors.leftMargin: 6
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: modelData
                                                color: cMuted; font.pixelSize: 9
                                                elide: Text.ElideMiddle
                                            }
                                            MouseArea { id: brFlatHover; anchors.fill: parent; hoverEnabled: true
                                                onClicked: {
                                                    selectedEvent = String(modelData)
                                                    statusMessage = "Selected: " + modelData
                                                }
                                            }
                                        }
                                    }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: "No assets\nOpen a project or import audio"
                                    color: cDim; font.pixelSize: 10
                                    horizontalAlignment: Text.AlignHCenter
                                    visible: browserList.count === 0
                                }
                            }
                        }
                    }
                }

                // ----------------------------------------------------------------
                // CENTER: WORKSPACE
                // ----------------------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#111111"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Tab strip
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 26
                            color: "#1f1f22"
                            border.color: "#2d2d30"; border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 4; spacing: 2
                                Repeater {
                                    model: panels
                                    delegate: Rectangle {
                                        Layout.preferredWidth: Math.max(84, ptLbl.implicitWidth + 16)
                                        Layout.preferredHeight: 20
                                        radius: 2
                                        color: activePanel === modelData.key ? cSub : (ptHover.containsMouse ? "#2a2a2e" : "transparent")
                                        border.color: activePanel === modelData.key ? cAccent : "transparent"
                                        border.width: 1
                                        Text {
                                            id: ptLbl
                                            anchors.centerIn: parent
                                            text: modelData.label
                                            color: activePanel === modelData.key ? "#ffffff" : cMuted
                                            font.pixelSize: 9; font.bold: activePanel === modelData.key
                                        }
                                        MouseArea { id: ptHover; anchors.fill: parent; hoverEnabled: true
                                            onClicked: activePanel = modelData.key
                                        }
                                    }
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    Layout.rightMargin: 8
                                    text: "Banks: " + (AudioEngine ? AudioEngine.loadedBanks.length : 0)
                                    color: cDim; font.pixelSize: 9
                                }
                            }
                        }

                        StackLayout {
                            id: workspace
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            currentIndex: {
                                var idx = 0
                                for (var i = 0; i < panels.length; ++i) {
                                    if (panels[i].key === activePanel) { idx = i; break }
                                }
                                return idx
                            }

                            // ================= EVENT EDITOR =================
                            Item {
                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 0

                                    // ---- Event editor toolbar ----
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 38
                                        color: "#1f1f22"
                                        border.color: "#2d2d30"; border.width: 1

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8; anchors.rightMargin: 8
                                            spacing: 8

                                            // Breadcrumb
                                            Column {
                                                Layout.fillWidth: true
                                                spacing: 0
                                                Text {
                                                    text: "Soundbank / Events"
                                                    color: cDim; font.pixelSize: 8
                                                }
                                                Text {
                                                    text: selectedEvent !== "" ? selectedEvent : "No event selected"
                                                    color: selectedEvent !== "" ? "#ffffff" : cDim
                                                    font.pixelSize: 12; font.bold: true; elide: Text.ElideMiddle
                                                    width: Math.min(280, parent.width)
                                                }
                                            }

                                            // Active toggle
                                            Rectangle {
                                                Layout.preferredWidth: 54; Layout.preferredHeight: 20; radius: 3
                                                color: eventActive ? cSelected : (actHover.containsMouse ? cHover : "transparent")
                                                border.color: eventActive ? cAccent : "#3f3f46"
                                                border.width: 1
                                                Text { anchors.centerIn: parent; text: "\u25C9 Active"; color: eventActive ? cAccent : cMuted; font.pixelSize: 8 }
                                                MouseArea { id: actHover; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: eventActive = !eventActive
                                                }
                                            }
                                            // Spatialize toggle
                                            Rectangle {
                                                Layout.preferredWidth: 36; Layout.preferredHeight: 20; radius: 3
                                                color: eventSpatial ? cSelected : (spatHover.containsMouse ? cHover : "transparent")
                                                border.color: eventSpatial ? cAccent : "#3f3f46"
                                                border.width: 1
                                                Text { anchors.centerIn: parent; text: "3D"; color: eventSpatial ? cAccent : cMuted; font.pixelSize: 8 }
                                                MouseArea { id: spatHover; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: eventSpatial = !eventSpatial
                                                }
                                            }

                                            Rectangle { width: 1; height: 20; color: "#3f3f46" }

                                            // Transport
                                            Rectangle {
                                                width: 26; height: 24; radius: 3
                                                color: isPlaying ? cBrand : (trPlayHover.containsMouse ? cHover : "transparent")
                                                Text { anchors.centerIn: parent; text: "\u25B6"; color: isPlaying ? "#ffffff" : cText; font.pixelSize: 11 }
                                                MouseArea { id: trPlayHover; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: togglePlay()
                                                }
                                                ToolTip { visible: trPlayHover.containsMouse; text: "Play (Space)" }
                                            }
                                            Rectangle {
                                                width: 26; height: 24; radius: 3
                                                color: trPauseHover.containsMouse ? cHover : "transparent"
                                                Text { anchors.centerIn: parent; text: "\u23F8"; color: cText; font.pixelSize: 11 }
                                                MouseArea { id: trPauseHover; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: { if (AudioBridge && AudioBridge.isPlaying) AudioBridge.pause() }
                                                }
                                                ToolTip { visible: trPauseHover.containsMouse; text: "Pause" }
                                            }
                                            Rectangle {
                                                width: 26; height: 24; radius: 3
                                                color: trStopHover.containsMouse ? cHover : "transparent"
                                                Text { anchors.centerIn: parent; text: "\u25A0"; color: cText; font.pixelSize: 11 }
                                                MouseArea { id: trStopHover; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: stopPlayback()
                                                }
                                                ToolTip { visible: trStopHover.containsMouse; text: "Stop (Shift+Space)" }
                                            }
                                            Rectangle {
                                                width: 26; height: 24; radius: 3
                                                color: (AudioBridge && AudioBridge.isLoopEnabled) ? cSelected : (trLoopHover.containsMouse ? cHover : "transparent")
                                                border.color: (AudioBridge && AudioBridge.isLoopEnabled) ? cAccent : "transparent"
                                                border.width: 1
                                                Text { anchors.centerIn: parent; text: "\u21BA"; color: (AudioBridge && AudioBridge.isLoopEnabled) ? cAccent : cMuted; font.pixelSize: 11 }
                                                MouseArea { id: trLoopHover; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: { if (AudioBridge) AudioBridge.setLoopEnabled(!AudioBridge.isLoopEnabled) }
                                                }
                                                ToolTip { visible: trLoopHover.containsMouse; text: "Loop" }
                                            }

                                            Text {
                                                text: fmtTime(posMs) + " / " + fmtTime(durMs)
                                                color: cAccent; font.pixelSize: 11; font.family: "monospace"
                                                Layout.leftMargin: 6
                                            }

                                            Item { Layout.fillWidth: true }

                                            // Parameter readout
                                            Text {
                                                text: "RPM " + (AudioEngine ? Math.round(AudioEngine.rpm) : 0)
                                                      + "   THR " + (AudioEngine ? Math.round(AudioEngine.throttle * 100) : 0) + "%"
                                                      + "   TBO " + (AudioEngine ? Math.round(AudioEngine.turboBoost * 100) : 0) + "%"
                                                color: cMuted; font.pixelSize: 9; font.family: "monospace"
                                            }
                                        }
                                    }

                                    // ---- Timeline ----
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        color: "#121212"
                                        border.color: "#2d2d30"; border.width: 1

                                        Item {
                                            id: timeline
                                            anchors.fill: parent
                                            anchors.margins: 1
                                            clip: true

                                            // Track header column
                                            Rectangle {
                                                id: trackHeader
                                                anchors.top: parent.top; anchors.bottom: parent.bottom
                                                anchors.left: parent.left
                                                width: 120
                                                color: "#222226"
                                                border.color: "#2d2d30"; border.width: 1

                                                ColumnLayout {
                                                    anchors.fill: parent
                                                    spacing: 0

                                                    Rectangle { Layout.preferredWidth: 120; Layout.preferredHeight: 24; color: "#242428"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: "TRACK"; color: cDim; font.pixelSize: 8; font.bold: true } }
                                                    Rectangle { Layout.preferredWidth: 120; Layout.preferredHeight: 28; color: "#2a2a2e"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: "\u266A Sound"; color: cText; font.pixelSize: 9 } }
                                                    Rectangle { Layout.preferredWidth: 120; Layout.preferredHeight: 28; color: "#2a2a2e"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: "\u21BA Loop"; color: cText; font.pixelSize: 9 } }
                                                    Rectangle { Layout.preferredWidth: 120; Layout.preferredHeight: 28; color: "#2a2a2e"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: "\u2699 Scatterer"; color: cText; font.pixelSize: 9 } }
                                                    Rectangle { Layout.preferredWidth: 120; Layout.preferredHeight: 24; color: "#2a2a2e"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: "\u25C6 Cue"; color: cText; font.pixelSize: 9 } }
                                                    Rectangle { Layout.preferredWidth: 120; Layout.preferredHeight: 16; color: "#1c1c20"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: "PARAMETERS"; color: cAccent; font.pixelSize: 7; font.bold: true; font.letterSpacing: 1 } }
                                                    Rectangle { Layout.preferredWidth: 120; Layout.preferredHeight: 28; color: "#2a2a2e"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: "RPM"; color: cText; font.pixelSize: 9 } }
                                                    Rectangle { Layout.preferredWidth: 120; Layout.preferredHeight: 28; color: "#2a2a2e"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: "Throttle"; color: cText; font.pixelSize: 9 } }
                                                    Rectangle { Layout.preferredWidth: 120; Layout.preferredHeight: 28; color: "#2a2a2e"
                                                        Text { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: "Turbo Boost"; color: cText; font.pixelSize: 9 } }
                                                    Item { Layout.fillHeight: true; width: 120 }
                                                }
                                            }

                                            // Ruler
                                            Rectangle {
                                                id: ruler
                                                anchors.top: parent.top; anchors.left: trackHeader.right; anchors.right: parent.right
                                                height: 24
                                                color: "#1a1a1e"
                                                border.color: "#2d2d30"; border.width: 1

                                                Repeater {
                                                    model: rulerTicks()
                                                    delegate: Item {
                                                        x: modelData.ms / Math.max(1, durMs) * (ruler.width - 10) + 5
                                                        width: 70; height: parent.height
                                                        Rectangle { x: 0; width: 1; height: 6; color: cBorderL }
                                                        Text { x: 4; y: 10; text: modelData.label; color: cMuted; font.pixelSize: 8 }
                                                    }
                                                }
                                            }

                                            // Lane helpers
                                            property int laneLeft: 124
                                            property int laneRight: 6

                                            // ---- Sound lane ----
                                            Rectangle {
                                                anchors.top: ruler.bottom; anchors.left: parent.left; anchors.right: parent.right
                                                anchors.topMargin: 0
                                                height: 28
                                                color: "#161616"
                                                border.color: "#252525"; border.width: 1
                                                Rectangle {
                                                    anchors.left: parent.left; anchors.leftMargin: timeline.laneLeft
                                                    anchors.top: parent.top; anchors.topMargin: 3; anchors.bottom: parent.bottom; anchors.bottomMargin: 3
                                                    width: Math.max(40, (parent.width - timeline.laneLeft - timeline.laneRight) * 0.42)
                                                    radius: 3; color: "#0f2b24"
                                                    border.color: cWave; border.width: 1
                                                    Canvas {
                                                        id: waveCanvas
                                                        anchors.fill: parent
                                                        onPaint: {
                                                            var ctx = getContext("2d")
                                                            var w = width, h = height
                                                            ctx.clearRect(0, 0, w, h)
                                                            var seed = (selectedEvent + "sound").length || 7
                                                            ctx.fillStyle = cWave
                                                            ctx.globalAlpha = 0.75
                                                            for (var x = 0; x < w; x += 2) {
                                                                var t = x / Math.max(1, w)
                                                                var v = Math.sin(t * (22 + seed) + seed) * Math.sin(t * 7 + 2)
                                                                var amp = (v * 0.7 + 0.3)
                                                                var bh = Math.max(2, amp * h * 0.9)
                                                                ctx.fillRect(x, (h - bh) / 2, 1.4, bh)
                                                            }
                                                            ctx.globalAlpha = 1
                                                        }
                                                        onWidthChanged: requestPaint()
                                                        onHeightChanged: requestPaint()
                                                    }
                                                    Text {
                                                        anchors.centerIn: parent
                                                        anchors.leftMargin: 6; anchors.rightMargin: 6
                                                        text: selectedEvent !== "" ? selectedEvent + ".wav" : "clip.wav"
                                                        color: "#9fe8d2"; font.pixelSize: 8; elide: Text.ElideMiddle
                                                        clip: true
                                                    }
                                                }
                                                Rectangle {
                                                    anchors.left: parent.left; anchors.leftMargin: timeline.laneLeft + (parent.width - timeline.laneLeft - timeline.laneRight) * 0.48
                                                    anchors.top: parent.top; anchors.topMargin: 3; anchors.bottom: parent.bottom; anchors.bottomMargin: 3
                                                    width: Math.max(40, (parent.width - timeline.laneLeft - timeline.laneRight) * 0.46)
                                                    radius: 3; color: "#12243a"
                                                    border.color: cAccent; border.width: 1
                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: selectedEvent !== "" ? selectedEvent + "_loop.wav" : "loop.wav"
                                                        color: "#9cc8ee"; font.pixelSize: 8; elide: Text.ElideMiddle
                                                        anchors.leftMargin: 6; anchors.rightMargin: 6
                                                    }
                                                }
                                            }

                                            // ---- Loop lane ----
                                            Rectangle {
                                                anchors.top: ruler.bottom; anchors.topMargin: 28; anchors.left: parent.left; anchors.right: parent.right
                                                height: 28
                                                color: "#161616"
                                                border.color: "#252525"; border.width: 1
                                                Rectangle {
                                                    anchors.left: parent.left; anchors.leftMargin: timeline.laneLeft
                                                    anchors.top: parent.top; anchors.bottom: parent.bottom
                                                    width: (parent.width - timeline.laneLeft - timeline.laneRight) * 0.5
                                                    anchors.topMargin: 7; anchors.bottomMargin: 7
                                                    radius: 2; color: "#1d3a52"
                                                    border.color: cAccent; border.width: 1
                                                    RowLayout {
                                                        anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6
                                                        Text { text: "\u21BA Loop Region"; color: "#9cc8ee"; font.pixelSize: 8 }
                                                        Item { Layout.fillWidth: true }
                                                        Text { text: fmtTime(Math.round(durMs * 0.5)); color: "#7faed8"; font.pixelSize: 8 }
                                                    }
                                                }
                                            }

                                            // ---- Scatterer lane ----
                                            Rectangle {
                                                anchors.top: ruler.bottom; anchors.topMargin: 56; anchors.left: parent.left; anchors.right: parent.right
                                                height: 28
                                                color: "#161616"
                                                border.color: "#252525"; border.width: 1
                                                Repeater {
                                                    model: [0.18, 0.4, 0.62, 0.82]
                                                    delegate: Rectangle {
                                                        x: timeline.laneLeft + modelData * (parent.width - timeline.laneLeft - timeline.laneRight) - 5
                                                        y: 6
                                                        width: 10; height: 10; rotation: 45
                                                        color: "#3aa88f"; opacity: 0.7
                                                    }
                                                }
                                            }

                                            // ---- Cue lane ----
                                            Rectangle {
                                                anchors.top: ruler.bottom; anchors.topMargin: 84; anchors.left: parent.left; anchors.right: parent.right
                                                height: 24
                                                color: "#161616"
                                                border.color: "#252525"; border.width: 1
                                                Repeater {
                                                    model: [0.3, 0.55, 0.8]
                                                    delegate: Rectangle {
                                                        x: timeline.laneLeft + modelData * (parent.width - timeline.laneLeft - timeline.laneRight) - 4
                                                        y: 8
                                                        width: 8; height: 8; rotation: 45
                                                        color: "#d29445"
                                                    }
                                                }
                                            }

                                            // ---- Parameters divider line ----
                                            Rectangle {
                                                anchors.top: ruler.bottom; anchors.topMargin: 108; anchors.left: trackHeader.right; anchors.right: parent.right
                                                height: 1; color: cBorderL
                                            }

                                            // ---- RPM automation lane ----
                                            Rectangle {
                                                anchors.top: ruler.bottom; anchors.topMargin: 124; anchors.left: parent.left; anchors.right: parent.right
                                                height: 28
                                                color: "#151518"
                                                border.color: "#252525"; border.width: 1
                                                Canvas {
                                                    id: rpmCanvas
                                                    anchors.fill: parent
                                                    onPaint: drawCurve(rpmCanvas, "rpm")
                                                    onWidthChanged: requestPaint()
                                                    onHeightChanged: requestPaint()
                                                }
                                                Text {
                                                    anchors.right: parent.right; anchors.rightMargin: 6
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    text: "RPM"
                                                    color: cAccent; font.pixelSize: 8; font.bold: true
                                                }
                                            }

                                            // ---- Throttle automation lane ----
                                            Rectangle {
                                                anchors.top: ruler.bottom; anchors.topMargin: 152; anchors.left: parent.left; anchors.right: parent.right
                                                height: 28
                                                color: "#151518"
                                                border.color: "#252525"; border.width: 1
                                                Canvas {
                                                    id: thrCanvas
                                                    anchors.fill: parent
                                                    onPaint: drawCurve(thrCanvas, "throttle")
                                                    onWidthChanged: requestPaint()
                                                    onHeightChanged: requestPaint()
                                                }
                                                Text {
                                                    anchors.right: parent.right; anchors.rightMargin: 6
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    text: "THR"
                                                    color: cWave; font.pixelSize: 8; font.bold: true
                                                }
                                            }

                                            // ---- Turbo Boost automation lane ----
                                            Rectangle {
                                                anchors.top: ruler.bottom; anchors.topMargin: 180; anchors.left: parent.left; anchors.right: parent.right
                                                height: 28
                                                color: "#151518"
                                                border.color: "#252525"; border.width: 1
                                                Canvas {
                                                    id: turboCanvas
                                                    anchors.fill: parent
                                                    onPaint: drawCurve(turboCanvas, "turbo")
                                                    onWidthChanged: requestPaint()
                                                    onHeightChanged: requestPaint()
                                                }
                                                Text {
                                                    anchors.right: parent.right; anchors.rightMargin: 6
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    text: "TBO"
                                                    color: "#d29445"; font.pixelSize: 8; font.bold: true
                                                }
                                            }

                                            // ---- Playhead ----
                                            Rectangle {
                                                id: playhead
                                                x: trackHeader.width + 5 + (durMs > 0 ? (posMs / durMs) * (ruler.width - 10) : 0)
                                                y: 24; width: 1; height: parent.height - 24
                                                color: cBrand
                                            }
                                            Rectangle {
                                                x: playhead.x - 4; y: 22
                                                width: 9; height: 6; color: cBrand
                                            }

                                            // Empty state overlay
                                            Text {
                                                anchors.centerIn: parent
                                                anchors.verticalCenterOffset: 40
                                                text: "Select an event in the Browser to load it into the timeline"
                                                color: cDim; font.pixelSize: 10
                                                visible: selectedEvent === ""
                                            }
                                        }
                                    }

                                    // ---- Mixer strip ----
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 128
                                        color: "#1a1a1e"
                                        border.color: "#2d2d30"; border.width: 1

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 6
                                            spacing: 4

                                            Text { text: "MIXER"; color: cAccent; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                spacing: 6

                                                // Track / bus strips
                                                Repeater {
                                                    model: mixStrips
                                                    delegate: Rectangle {
                                                        Layout.preferredWidth: 84
                                                        Layout.fillHeight: true
                                                        radius: 3; color: "#202024"
                                                        border.color: "#3f3f46"; border.width: 1

                                                        property real stripLevel: 0.3 + modelData.seed * 0.12
                                                        property real fval: 60 + modelData.seed * 8
                                                        property bool muted: false
                                                        property bool soloed: false

                                                        ColumnLayout {
                                                            anchors.fill: parent; anchors.margins: 5; spacing: 2

                                                            Text {
                                                                text: modelData.name
                                                                color: muted ? cDim : "#ffffff"
                                                                font.pixelSize: 8; font.bold: true; elide: Text.ElideMiddle
                                                                Layout.fillWidth: true
                                                            }

                                                            RowLayout {
                                                                Layout.fillWidth: true
                                                                spacing: 2
                                                                Rectangle {
                                                                    width: 18; height: 14; radius: 2
                                                                    color: muted ? cBrand : (mHover.containsMouse ? cHover : "#18181b")
                                                                    border.color: muted ? cBrand : "#3f3f46"; border.width: 1
                                                                    Text { anchors.centerIn: parent; text: "M"; color: muted ? "#ffffff" : cMuted; font.pixelSize: 8 }
                                                                    MouseArea { id: mHover; anchors.fill: parent; hoverEnabled: true
                                                                        onClicked: muted = !muted
                                                                    }
                                                                }
                                                                Rectangle {
                                                                    width: 18; height: 14; radius: 2
                                                                    color: soloed ? cAccent : (sHover.containsMouse ? cHover : "#18181b")
                                                                    border.color: soloed ? cAccent : "#3f3f46"; border.width: 1
                                                                    Text { anchors.centerIn: parent; text: "S"; color: soloed ? "#121212" : cMuted; font.pixelSize: 8 }
                                                                    MouseArea { id: sHover; anchors.fill: parent; hoverEnabled: true
                                                                        onClicked: soloed = !soloed
                                                                    }
                                                                }
                                                                Item { Layout.fillWidth: true }
                                                            }

                                                            // Meter + fader
                                                            Item {
                                                                Layout.fillWidth: true
                                                                Layout.fillHeight: true
                                                                Rectangle {
                                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                                    anchors.top: parent.top; anchors.bottom: parent.bottom
                                                                    width: 6; radius: 2; color: "#18181b"
                                                                }
                                                                Rectangle {
                                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                                    anchors.bottom: parent.bottom
                                                                    width: 6; height: Math.max(2, parent.height * stripLevel * 0.75); radius: 2
                                                                    color: stripLevel > 0.85 ? "#ff4444" : cWave
                                                                }
                                                                Rectangle {
                                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                                    y: (1 - fval / 100.0) * (parent.height - 10) - 5
                                                                    width: 18; height: 10; radius: 2; color: "#3e3e42"
                                                                    border.color: stripFader.drag.active ? cAccent : "#4a4a50"; border.width: 1
                                                                }
                                                                MouseArea {
                                                                    id: stripFader
                                                                    anchors.fill: parent
                                                                    onPressed: (m) => { fval = Math.max(0, Math.min(100, (1 - m.y / Math.max(1, parent.height)) * 100)) }
                                                                    onPositionChanged: (m) => { fval = Math.max(0, Math.min(100, (1 - m.y / Math.max(1, parent.height)) * 100)) }
                                                                }
                                                            }

                                                            Text {
                                                                text: "\u2192 " + modelData.out
                                                                color: cDim; font.pixelSize: 7
                                                            }
                                                        }
                                                    }
                                                }

                                                // Master channel
                                                Rectangle {
                                                    Layout.preferredWidth: 92
                                                    Layout.fillHeight: true
                                                    radius: 3; color: "#202024"
                                                    border.color: cBrand; border.width: 1

                                                    ColumnLayout {
                                                        anchors.fill: parent; anchors.margins: 5; spacing: 2
                                                        Text { text: "Master"; color: "#ffffff"; font.pixelSize: 9; font.bold: true }

                                                        RowLayout {
                                                            Layout.fillWidth: true
                                                            Rectangle { Layout.preferredWidth: 5; height: 34; radius: 1; color: "#18181b"
                                                                Rectangle { width: 3; anchors.horizontalCenter: parent.horizontalCenter
                                                                    anchors.bottom: parent.bottom; height: Math.max(2, leftPeakDb * 32)
                                                                    color: leftPeakDb > 0.9 ? "#ff4444" : cWave; radius: 1 } }
                                                            Rectangle { Layout.preferredWidth: 5; height: 34; radius: 1; color: "#18181b"
                                                                Rectangle { width: 3; anchors.horizontalCenter: parent.horizontalCenter
                                                                    anchors.bottom: parent.bottom; height: Math.max(2, rightPeakDb * 32)
                                                                    color: rightPeakDb > 0.9 ? "#ff4444" : cWave; radius: 1 } }
                                                            Item { Layout.fillWidth: true }
                                                        }

                                                        Slider {
                                                            Layout.fillWidth: true; from: 0; to: 100; value: masterVolume; height: 14
                                                            onValueChanged: {
                                                                masterVolume = value
                                                                if (AudioEngine) AudioEngine.setBusVolume("master", value / 100.0)
                                                            }
                                                            background: Rectangle { x: 0; y: 5; width: parent.width; height: 3; radius: 2; color: "#2a2a2a" }
                                                            handle: Rectangle { x: parent.left + parent.visualPosition * parent.width - 6; y: 2; width: 11; height: 11; radius: 6; color: cBrand }
                                                        }
                                                        Text { text: Math.round(masterVolume) + "%"; color: cBrand; font.pixelSize: 8 }
                                                    }
                                                }

                                                Item { Layout.fillWidth: true }
                                            }
                                        }
                                    }
                                }
                            }

                            // ---------------- MIXER ----------------
                            Item { AudioMixer { anchors.fill: parent } }

                            // ---------------- EVENTS BROWSER ----------------
                            Item { AudioEventBrowser { anchors.fill: parent } }

                            // ---------------- EFFECTS RACK ----------------
                            Item { AudioEffectsRack { anchors.fill: parent } }

                            // ---------------- RECORDING ----------------
                            Item { AudioRecordingStudio { anchors.fill: parent } }

                            // ---------------- SOUND BANKS ----------------
                            Item { AudioSoundBanks { anchors.fill: parent } }

                            // ---------------- BATCH ----------------
                            Item { AudioBatchProcessor { anchors.fill: parent } }

                            // ---------------- EXPORT ----------------
                            Item { AudioExportPanel { anchors.fill: parent } }
                        }
                    }
                }
            }

            // ==================================================================
            // STATUS BAR
            // ==================================================================
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                color: "#1e1e1e"
                border.color: "#2d2d30"; border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8; anchors.rightMargin: 8
                    spacing: 6

                    Text { text: statusMessage; color: cAccent; font.pixelSize: 10; elide: Text.ElideRight; Layout.maximumWidth: 300 }

                    Item { Layout.fillWidth: true }

                    // Toggle pills
                    Rectangle {
                        Layout.preferredWidth: 48; Layout.preferredHeight: 18; radius: 3
                        color: (AudioBridge && AudioBridge.isLoopEnabled) ? cSelected : (pillLoop.containsMouse ? cHover : "transparent")
                        border.color: (AudioBridge && AudioBridge.isLoopEnabled) ? cAccent : "#3f3f46"
                        border.width: 1
                        Text { anchors.centerIn: parent; text: "Loop"; color: (AudioBridge && AudioBridge.isLoopEnabled) ? cAccent : cMuted; font.pixelSize: 9 }
                        MouseArea { id: pillLoop; anchors.fill: parent; hoverEnabled: true
                            onClicked: { if (AudioBridge) AudioBridge.setLoopEnabled(!AudioBridge.isLoopEnabled) }
                        }
                    }
                    Rectangle {
                        Layout.preferredWidth: 54; Layout.preferredHeight: 18; radius: 3
                        color: isRecording ? cSelected : (pillRec.containsMouse ? cHover : "transparent")
                        border.color: isRecording ? cAccent : "#3f3f46"
                        border.width: 1
                        Text { anchors.centerIn: parent; text: "Record"; color: isRecording ? cAccent : cMuted; font.pixelSize: 9 }
                        MouseArea { id: pillRec; anchors.fill: parent; hoverEnabled: true
                            onClicked: toggleRecord()
                        }
                    }
                    Rectangle {
                        Layout.preferredWidth: 58; Layout.preferredHeight: 18; radius: 3
                        color: browserVisible ? cSelected : (pillBrw.containsMouse ? cHover : "transparent")
                        border.color: browserVisible ? cAccent : "#3f3f46"
                        border.width: 1
                        Text { anchors.centerIn: parent; text: "Browser"; color: browserVisible ? cAccent : cMuted; font.pixelSize: 9 }
                        MouseArea { id: pillBrw; anchors.fill: parent; hoverEnabled: true
                            onClicked: browserVisible = !browserVisible
                        }
                    }

                    Rectangle { width: 1; height: 16; color: "#2d2d30" }
                    Text { text: fmtTime(posMs) + " / " + fmtTime(durMs); color: cMuted; font.pixelSize: 10; font.family: "monospace" }
                    Rectangle { width: 1; height: 16; color: "#2d2d30" }
                    Text { text: "Event: " + (selectedEvent !== "" ? selectedEvent : "\u2014"); color: cMuted; font.pixelSize: 10 }
                    Rectangle { width: 1; height: 16; color: "#2d2d30" }
                    Text { text: (AudioBridge ? AudioBridge.getSampleRate() : 44100) + " Hz"; color: cMuted; font.pixelSize: 10 }
                    Rectangle { width: 1; height: 16; color: "#2d2d30" }
                    Text { text: (AudioBridge ? AudioBridge.getBitDepth() : 16) + "-bit"; color: cMuted; font.pixelSize: 10 }
                    Rectangle { width: 1; height: 16; color: "#2d2d30" }
                    Text { text: (AudioBridge ? (AudioBridge.getChannelCount() === 1 ? "Mono" : "Stereo") : "Stereo"); color: cMuted; font.pixelSize: 10 }
                    Rectangle { width: 1; height: 16; color: "#2d2d30" }
                    Text { text: currentProject; color: cDim; font.pixelSize: 10 }
                }
            }
        }
    }

    // ==========================================================================
    // Dialogs
    // ==========================================================================
    Dialog {
        id: aboutDialog
        title: "About KS Audio Studio"
        standardButtons: Dialog.Ok
        modal: true; anchors.centerIn: parent
        width: 340; height: 220
        background: Rectangle { color: "#1e1e1e"; border.color: "#3f3f46"; border.width: 1; radius: 3 }

        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 8

            Text { text: "KS Audio Studio"; color: cBrand; font.pixelSize: 18; font.bold: true }
            Text { text: "Assetto Corsa Sound Editor Suite"; color: cMuted; font.pixelSize: 11 }
            Rectangle { height: 1; color: "#3f3f46"; Layout.fillWidth: true }
            Text { text: "Multi-track audio editing, event authoring, mixing and sound bank management for car audio systems."; color: "#aaaaaa"; font.pixelSize: 10; wrapMode: Text.WordWrap }
            Text { text: "Version 1.0"; color: cDim; font.pixelSize: 9 }
        }
    }

    Dialog {
        id: shortcutDialog
        title: "Keyboard Shortcuts"
        standardButtons: Dialog.Close
        modal: true; anchors.centerIn: parent
        width: 420; height: 420
        background: Rectangle { color: "#1e1e1e"; border.color: "#3f3f46"; border.width: 1; radius: 3 }

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
                    text: modelData; color: "#bbbbbb"; font.pixelSize: 10; font.family: "monospace"; leftPadding: 8
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

    // ==========================================================================
    // Shortcuts
    // ==========================================================================
    Shortcut { sequence: "Space"; onActivated: togglePlay() }
    Shortcut { sequence: "Shift+Space"; onActivated: stopPlayback() }
    Shortcut { sequence: "R"; onActivated: toggleRecord() }

    // ==========================================================================
    // Connections
    // ==========================================================================
    Connections {
        target: AudioBridge
        function onStatusMessage(msg) { statusMessage = msg }
        function onRecordingStateChanged(recording) {
            isRecording = recording
            statusMessage = recording ? "Recording..." : "Recording stopped"
        }
    }

    Component.onCompleted: {
        if (AudioBridge) statusMessage = "Audio engine ready"
    }
}
