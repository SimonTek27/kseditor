import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#0d0d0d"

    // ── Palette ────────────────────────────────────────────────────────────
    readonly property color cBg:       "#0d0d0d"
    readonly property color cPanel:    "#1a1a1a"
    readonly property color cBar:      "#222222"
    readonly property color cBorder:   "#333333"
    readonly property color cAccent:   "#E10600"
    readonly property color cAccent2:  "#ff6b00"
    readonly property color cGreen:    "#00cc66"
    readonly property color cRed:      "#e53935"
    readonly property color cYellow:   "#ffc107"
    readonly property color cText:     "#cccccc"
    readonly property color cMuted:    "#555555"
    readonly property color cWave:     "#1565c0"
    readonly property color cWaveFill: "#0d47a1"
    readonly property color cSel:      "rgba(225,6,0,0.18)"

    // ── State ──────────────────────────────────────────────────────────────
    property string activePanel:    "waveform"   // waveform | effects | eq | spectrum | batch
    property string activeEffect:   ""
    property bool   isPlaying:      false
    property bool   isRecording:    false
    property bool   isLooping:      false
    property real   playheadPos:    0.0          // 0..1
    property real   selStart:       0.22
    property real   selEnd:         0.68
    property real   zoomLevel:      1.0
    property real   masterVol:      0.8
    property real   playbackRate:   1.0
    property int    sampleRate:     44100
    property int    bitDepth:       16
    property int    channels:       2
    property real   duration:       12.437       // seconds
    property string filename:       "engine_int_load.wav"
    property real   pitchSemitones: 0.0
    property real   eqLow:          0.0
    property real   eqMid:          0.0
    property real   eqHigh:         0.0
    property real   compThresh:     -18.0
    property real   compRatio:      4.0
    property real   compAttack:     10.0
    property real   compRelease:    100.0
    property real   reverbMix:      0.25
    property real   reverbDecay:    1.8
    property real   delayTime:      250.0
    property real   delayFeedback:  0.35
    property real   noiseGateThresh: -40.0
    property real   normalizeTarget: -1.0

    // ── History for undo/redo ──────────────────────────────────────────────
    property var history: []
    property int historyIdx: -1
    function pushHistory(action) {
        history = history.slice(0, historyIdx + 1);
        history.push(action);
        historyIdx = history.length - 1;
    }

    // ── Helpers ───────────────────────────────────────────────────────────
    function formatTime(s) {
        var m = Math.floor(s / 60);
        var sec = (s % 60).toFixed(3);
        return (m < 10 ? "0" : "") + m + ":" + (sec < 10 ? "0" : "") + sec;
    }
    function dbToLinear(db) { return Math.pow(10, db / 20); }
    function linearToDb(lin) { return lin > 0 ? 20 * Math.log(lin) / Math.log(10) : -999; }

    // ════════════════════════════════════════════════════════════════════════
    // ROOT LAYOUT
    // ════════════════════════════════════════════════════════════════════════
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Menu bar ───────────────────────────────────────────────────────
        Rectangle {
            height: 26; Layout.fillWidth: true
            color: "#111111"; border.color: cBorder; border.width: 1

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; spacing: 2

                Repeater {
                    model: ["File", "Edit", "View", "Effect", "Tool", "Options", "Help"]
                    Rectangle {
                        height: 22; width: 46; radius: 2
                        color: mArea.containsMouse ? "#2a2a2a" : "transparent"
                        Text { anchors.centerIn: parent; text: modelData; color: cText; font.pixelSize: 11 }
                        MouseArea { id: mArea; anchors.fill: parent; hoverEnabled: true }
                    }
                }

                Item { Layout.fillWidth: true }
                Text { text: root.filename; color: cMuted; font.pixelSize: 10; font.family: "Courier New" }
                Item { width: 8 }
            }
        }

        // ── Toolbar ────────────────────────────────────────────────────────
        Rectangle {
            height: 38; Layout.fillWidth: true
            color: cBar; border.color: cBorder; border.width: 1

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 4

                // File ops
                Repeater {
                    model: [
                        { icon: "📂", tip: "Open" },
                        { icon: "💾", tip: "Save" },
                        { icon: "📋", tip: "Save As" }
                    ]
                    Rectangle {
                        width: 30; height: 28; radius: 3
                        color: ma.containsMouse ? "#333" : "transparent"
                        border.color: ma.containsMouse ? cBorder : "transparent"; border.width: 1
                        Text { anchors.centerIn: parent; text: modelData.icon; font.pixelSize: 14 }
                        MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true }
                        ToolTip.visible: ma.containsMouse; ToolTip.text: modelData.tip
                    }
                }

                Rectangle { width: 1; height: 24; color: cBorder }

                // Edit ops
                Repeater {
                    model: [
                        { icon: "↩", tip: "Undo",   act: function() { if (historyIdx > 0) historyIdx-- } },
                        { icon: "↪", tip: "Redo",   act: function() { if (historyIdx < history.length-1) historyIdx++ } },
                        { icon: "✂", tip: "Cut",    act: function() { pushHistory("cut") } },
                        { icon: "⎘", tip: "Copy",   act: function() { pushHistory("copy") } },
                        { icon: "📌", tip: "Paste",  act: function() { pushHistory("paste") } },
                        { icon: "🗑", tip: "Delete", act: function() { pushHistory("delete") } },
                        { icon: "⬚", tip: "Trim to selection", act: function() { pushHistory("trim") } }
                    ]
                    Rectangle {
                        width: 30; height: 28; radius: 3
                        color: ma2.containsMouse ? "#333" : "transparent"
                        border.color: ma2.containsMouse ? cBorder : "transparent"; border.width: 1
                        Text { anchors.centerIn: parent; text: modelData.icon; font.pixelSize: 14 }
                        MouseArea { id: ma2; anchors.fill: parent; hoverEnabled: true; onClicked: modelData.act() }
                        ToolTip.visible: ma2.containsMouse; ToolTip.text: modelData.tip
                    }
                }

                Rectangle { width: 1; height: 24; color: cBorder }

                // Selection tools
                Repeater {
                    model: [
                        { icon: "◻", tip: "Select All" },
                        { icon: "⊡", tip: "Select None" },
                        { icon: "⊞", tip: "Select to Start" },
                        { icon: "⊟", tip: "Select to End" }
                    ]
                    Rectangle {
                        width: 30; height: 28; radius: 3
                        color: ma3.containsMouse ? "#333" : "transparent"
                        border.color: ma3.containsMouse ? cBorder : "transparent"; border.width: 1
                        Text { anchors.centerIn: parent; text: modelData.icon; font.pixelSize: 14 }
                        MouseArea { id: ma3; anchors.fill: parent; hoverEnabled: true }
                        ToolTip.visible: ma3.containsMouse; ToolTip.text: modelData.tip
                    }
                }

                Rectangle { width: 1; height: 24; color: cBorder }

                // Zoom
                Repeater {
                    model: [
                        { icon: "🔍+", tip: "Zoom In",   act: function() { zoomLevel = Math.min(zoomLevel * 1.5, 32) } },
                        { icon: "🔍-", tip: "Zoom Out",  act: function() { zoomLevel = Math.max(zoomLevel / 1.5, 0.5) } },
                        { icon: "⊕",  tip: "Zoom All",  act: function() { zoomLevel = 1.0 } },
                        { icon: "⊗",  tip: "Zoom Sel",  act: function() { zoomLevel = 1.0 / (selEnd - selStart) } }
                    ]
                    Rectangle {
                        width: 34; height: 28; radius: 3
                        color: ma4.containsMouse ? "#333" : "transparent"
                        border.color: ma4.containsMouse ? cBorder : "transparent"; border.width: 1
                        Text { anchors.centerIn: parent; text: modelData.icon; color: cText; font.pixelSize: 11 }
                        MouseArea { id: ma4; anchors.fill: parent; hoverEnabled: true; onClicked: modelData.act() }
                        ToolTip.visible: ma4.containsMouse; ToolTip.text: modelData.tip
                    }
                }

                Item { Layout.fillWidth: true }

                // Info
                Text {
                    text: sampleRate + " Hz  " + bitDepth + "-bit  " + (channels === 2 ? "Stereo" : "Mono")
                    color: cMuted; font.pixelSize: 10; font.family: "Courier New"
                }
            }
        }

        // ── Panel tabs ─────────────────────────────────────────────────────
        Rectangle {
            height: 30; Layout.fillWidth: true
            color: "#111"; border.color: cBorder; border.width: 1

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 4; spacing: 0

                Repeater {
                    model: [
                        { id: "waveform",  label: "Waveform"  },
                        { id: "spectrum",  label: "Spectrum"  },
                        { id: "eq",        label: "Equalizer" },
                        { id: "effects",   label: "Effects"   },
                        { id: "compressor",label: "Compressor"},
                        { id: "batch",     label: "Batch"     },
                        { id: "cue",       label: "Cue Points"}
                    ]
                    Rectangle {
                        height: 30; width: 86
                        color: activePanel === modelData.id ? cPanel : "transparent"
                        border.color: "transparent"
                        Rectangle { // active indicator
                            anchors.top: parent.top; width: parent.width; height: 2
                            color: activePanel === modelData.id ? cAccent : "transparent"
                        }
                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: activePanel === modelData.id ? cAccent : cMuted
                            font.pixelSize: 11
                            font.bold: activePanel === modelData.id
                        }
                        MouseArea { anchors.fill: parent; onClicked: activePanel = modelData.id }
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        // ── Main content area ──────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ── WAVEFORM PANEL ─────────────────────────────────────────────
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                visible: activePanel === "waveform"

                // Time ruler
                Rectangle {
                    height: 20; Layout.fillWidth: true
                    color: "#111"; border.color: cBorder; border.width: 1
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0,0,width,height);
                            ctx.fillStyle = "#111";
                            ctx.fillRect(0,0,width,height);
                            var ticks = 20;
                            for (var i = 0; i <= ticks; i++) {
                                var x = i * width / ticks;
                                ctx.strokeStyle = "#444";
                                ctx.lineWidth = 1;
                                ctx.beginPath(); ctx.moveTo(x,12); ctx.lineTo(x,height); ctx.stroke();
                                var t = i / ticks * root.duration;
                                ctx.fillStyle = "#666";
                                ctx.font = "9px Courier New";
                                ctx.fillText(root.formatTime(t), x+2, 10);
                            }
                        }
                    }
                }

                // Waveform canvas - Left channel
                Rectangle {
                    Layout.fillWidth: true; height: 120
                    color: "#050e1a"; border.color: cBorder; border.width: 1

                    // dB axis
                    Column {
                        anchors.right: parent.right; anchors.rightMargin: 2
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 14
                        Repeater { model: ["+0","-6","-12","-18","-∞"]
                            Text { text: modelData; color: "#444"; font.pixelSize: 7; font.family: "Courier New"
                                   anchors.right: parent ? parent.right : undefined }
                        }
                    }

                    // Selection highlight
                    Rectangle {
                        x: parent.width * selStart; width: parent.width * (selEnd - selStart)
                        y: 0; height: parent.height
                        color: cSel
                    }

                    // Waveform
                    Canvas {
                        id: waveCanvasL
                        anchors.fill: parent
                        property real ph: playheadPos
                        onPhChanged: requestPaint()
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0,0,width,height);
                            var mid = height / 2;
                            // waveform fill
                            ctx.fillStyle = root.cWaveFill;
                            ctx.beginPath(); ctx.moveTo(0, mid);
                            for (var x = 0; x < width; x++) {
                                var t = x / width;
                                var env = Math.sin(t * Math.PI);
                                var amp = mid * 0.85 * env * (0.5 + 0.5 * Math.abs(Math.sin(x * 0.3)));
                                var noise = (Math.random() - 0.5) * amp * 0.15;
                                ctx.lineTo(x, mid - amp - noise);
                            }
                            for (var x2 = width - 1; x2 >= 0; x2--) {
                                var t2 = x2 / width;
                                var env2 = Math.sin(t2 * Math.PI);
                                var amp2 = mid * 0.85 * env2 * (0.5 + 0.5 * Math.abs(Math.sin(x2 * 0.3)));
                                var noise2 = (Math.random() - 0.5) * amp2 * 0.15;
                                ctx.lineTo(x2, mid + amp2 + noise2);
                            }
                            ctx.closePath(); ctx.fill();
                            // top stroke
                            ctx.strokeStyle = "#3d8bcd"; ctx.lineWidth = 1.2;
                            ctx.beginPath();
                            for (var x3 = 0; x3 < width; x3++) {
                                var t3 = x3 / width;
                                var env3 = Math.sin(t3 * Math.PI);
                                var amp3 = mid * 0.85 * env3 * (0.5 + 0.5 * Math.abs(Math.sin(x3 * 0.3)));
                                if (x3 === 0) ctx.moveTo(x3, mid - amp3); else ctx.lineTo(x3, mid - amp3);
                            }
                            ctx.stroke();
                            // zero line
                            ctx.strokeStyle = "#1a3a5a"; ctx.lineWidth = 1;
                            ctx.beginPath(); ctx.moveTo(0, mid); ctx.lineTo(width, mid); ctx.stroke();
                            // playhead
                            ctx.strokeStyle = root.cAccent; ctx.lineWidth = 1.5;
                            ctx.beginPath();
                            ctx.moveTo(ph * width, 0); ctx.lineTo(ph * width, height); ctx.stroke();
                        }
                    }

                    Text { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 4
                        text: "L"; color: cAccent; font.pixelSize: 10; font.bold: true }

                    // Draggable playhead
                    MouseArea {
                        anchors.fill: parent
                        onPositionChanged: function(m) {
                            playheadPos = Math.max(0, Math.min(1, m.x / width));
                            waveCanvasL.requestPaint();
                        }
                        onPressed: function(m) { playheadPos = Math.max(0, Math.min(1, m.x / width)) }
                    }
                }

                // Waveform canvas - Right channel
                Rectangle {
                    Layout.fillWidth: true; height: 120
                    color: "#060e14"; border.color: cBorder; border.width: 1

                    Rectangle {
                        x: parent.width * selStart; width: parent.width * (selEnd - selStart)
                        y: 0; height: parent.height; color: cSel
                    }

                    Canvas {
                        id: waveCanvasR
                        anchors.fill: parent
                        property real ph: playheadPos
                        onPhChanged: requestPaint()
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0,0,width,height);
                            var mid = height / 2;
                            ctx.fillStyle = "#0a2a1a";
                            ctx.beginPath(); ctx.moveTo(0, mid);
                            for (var x = 0; x < width; x++) {
                                var t = x / width;
                                var env = Math.sin(t * Math.PI * 0.95 + 0.1);
                                var amp = mid * 0.82 * env * (0.5 + 0.5 * Math.abs(Math.sin(x * 0.28 + 1)));
                                ctx.lineTo(x, mid - amp);
                            }
                            for (var x2 = width - 1; x2 >= 0; x2--) {
                                var t2 = x2 / width;
                                var env2 = Math.sin(t2 * Math.PI * 0.95 + 0.1);
                                var amp2 = mid * 0.82 * env2 * (0.5 + 0.5 * Math.abs(Math.sin(x2 * 0.28 + 1)));
                                ctx.lineTo(x2, mid + amp2);
                            }
                            ctx.closePath(); ctx.fill();
                            ctx.strokeStyle = "#2daa6a"; ctx.lineWidth = 1.2;
                            ctx.beginPath();
                            for (var x3 = 0; x3 < width; x3++) {
                                var t3 = x3 / width;
                                var env3 = Math.sin(t3 * Math.PI * 0.95 + 0.1);
                                var amp3 = mid * 0.82 * env3 * (0.5 + 0.5 * Math.abs(Math.sin(x3 * 0.28 + 1)));
                                if (x3 === 0) ctx.moveTo(x3, mid - amp3); else ctx.lineTo(x3, mid - amp3);
                            }
                            ctx.stroke();
                            ctx.strokeStyle = "#0a2a1a"; ctx.lineWidth = 1;
                            ctx.beginPath(); ctx.moveTo(0, mid); ctx.lineTo(width, mid); ctx.stroke();
                            ctx.strokeStyle = root.cAccent; ctx.lineWidth = 1.5;
                            ctx.beginPath();
                            ctx.moveTo(ph * width, 0); ctx.lineTo(ph * width, height); ctx.stroke();
                        }
                    }

                    Text { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 4
                        text: "R"; color: cGreen; font.pixelSize: 10; font.bold: true }
                }

                // Scrollbar / overview
                Rectangle {
                    Layout.fillWidth: true; height: 30; color: "#0a0a0a"; border.color: cBorder; border.width: 1
                    Rectangle {
                        x: parent.width * selStart
                        width: parent.width * (selEnd - selStart)
                        height: parent.height; color: "#1a2a3a"; radius: 2
                        border.color: cAccent; border.width: 1
                        // Drag handles
                        Rectangle { anchors.left: parent.left; width: 6; height: parent.height; color: cAccent; radius: 2 }
                        Rectangle { anchors.right: parent.right; width: 6; height: parent.height; color: cAccent; radius: 2 }
                    }
                    Canvas {
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                            ctx.fillStyle = "#1a2a1a";
                            ctx.beginPath(); ctx.moveTo(0, height/2);
                            for (var x = 0; x < width; x++) {
                                var amp = height * 0.35 * Math.sin(x * 0.2) * (0.3 + 0.7 * Math.sin(x * 0.07));
                                ctx.lineTo(x, height/2 - amp);
                            }
                            ctx.lineTo(width, height/2);
                            for (var x2 = width-1; x2 >= 0; x2--) {
                                var amp2 = height * 0.35 * Math.sin(x2 * 0.2) * (0.3 + 0.7 * Math.sin(x2 * 0.07));
                                ctx.lineTo(x2, height/2 + amp2);
                            }
                            ctx.closePath(); ctx.fill();
                        }
                    }
                }

                // ── Transport + selection info ─────────────────────────────
                Rectangle {
                    height: 48; Layout.fillWidth: true
                    color: "#111"; border.color: cBorder; border.width: 1

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 8

                        // Transport buttons
                        Repeater {
                            model: [
                                { icon: "⏮",  col: cText,    tip: "Go to Start",  act: function() { playheadPos=0; waveCanvasL.requestPaint(); waveCanvasR.requestPaint() } },
                                { icon: "⏪",  col: cText,    tip: "Rewind",       act: function() { playheadPos=Math.max(0,playheadPos-0.05); waveCanvasL.requestPaint(); waveCanvasR.requestPaint() } },
                                { icon: "■",   col: cRed,     tip: "Stop",         act: function() { isPlaying=false; isRecording=false } },
                                { icon: "▶",   col: isPlaying ? cAccent : cText, tip: "Play", act: function() { isPlaying=!isPlaying } },
                                { icon: "⏩",  col: cText,    tip: "Fast Forward", act: function() { playheadPos=Math.min(1,playheadPos+0.05); waveCanvasL.requestPaint(); waveCanvasR.requestPaint() } },
                                { icon: "⏭",  col: cText,    tip: "Go to End",    act: function() { playheadPos=1; waveCanvasL.requestPaint(); waveCanvasR.requestPaint() } },
                                { icon: "⏺",  col: isRecording ? cRed : cText, tip: "Record", act: function() { isRecording=!isRecording } },
                                { icon: "🔁",  col: isLooping ? cAccent : cMuted, tip: "Loop",  act: function() { isLooping=!isLooping } }
                            ]
                            Rectangle {
                                width: 32; height: 32; radius: 4
                                color: ma5.containsMouse ? "#2a2a2a" : (
                                    (modelData.icon==="▶" && isPlaying) ||
                                    (modelData.icon==="⏺" && isRecording) ||
                                    (modelData.icon==="🔁" && isLooping) ? "#1a2a3a" : "transparent")
                                border.color: (
                                    (modelData.icon==="▶" && isPlaying) ||
                                    (modelData.icon==="⏺" && isRecording) ||
                                    (modelData.icon==="🔁" && isLooping) ? cAccent : "transparent"); border.width: 1
                                Text { anchors.centerIn: parent; text: modelData.icon; color: modelData.col; font.pixelSize: 15 }
                                MouseArea { id: ma5; anchors.fill: parent; hoverEnabled: true; onClicked: modelData.act() }
                                ToolTip.visible: ma5.containsMouse; ToolTip.text: modelData.tip
                            }
                        }

                        // Playback rate
                        ColumnLayout { spacing: 1
                            Text { text: "Rate"; color: cMuted; font.pixelSize: 8; Layout.alignment: Qt.AlignHCenter }
                            Rectangle { width: 50; height: 22; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                Text { anchors.centerIn: parent; text: playbackRate.toFixed(2) + "x"; color: cText; font.pixelSize: 10; font.family: "Courier New" }
                                MouseArea {
                                    anchors.fill: parent; property real startY: 0; property real startVal: 0
                                    onPressed: function(m) { startY = m.y; startVal = playbackRate }
                                    onPositionChanged: function(m) { playbackRate = Math.max(0.25, Math.min(4.0, startVal - (m.y - startY) * 0.01)) }
                                }
                            }
                        }

                        Rectangle { width: 1; height: 36; color: cBorder }

                        // Timecodes
                        Repeater {
                            model: [
                                { label: "Start",    val: formatTime(selStart * duration) },
                                { label: "End",      val: formatTime(selEnd * duration) },
                                { label: "Length",   val: formatTime((selEnd - selStart) * duration) },
                                { label: "Position", val: formatTime(playheadPos * duration) }
                            ]
                            ColumnLayout { spacing: 1
                                Text { text: modelData.label; color: cMuted; font.pixelSize: 8 }
                                Rectangle { width: 80; height: 22; color: "#0d1a0d"; radius: 3; border.color: "#1a3a1a"; border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData.val; color: cGreen; font.pixelSize: 11; font.family: "Courier New" }
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Volume
                        ColumnLayout { spacing: 1
                            Text { text: "Volume"; color: cMuted; font.pixelSize: 8 }
                            Slider {
                                width: 80; height: 22; from: 0; to: 1; value: masterVol
                                onValueChanged: masterVol = value
                                background: Rectangle { x: 0; y: parent.height/2-2; width: parent.width; height: 4; radius: 2; color: "#333"
                                    Rectangle { width: parent.width * parent.parent.value; height: 4; radius: 2; color: cAccent } }
                                handle: Rectangle { x: parent.visualPosition * (parent.width-14); y: 4; width: 14; height: 14; radius: 7; color: "#888"; border.color: "#aaa"; border.width: 1 }
                            }
                        }
                    }
                }

                // ── Quick effects strip ────────────────────────────────────
                Rectangle {
                    height: 36; Layout.fillWidth: true
                    color: "#0f0f0f"; border.color: cBorder; border.width: 1

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 8; spacing: 6

                        Text { text: "Quick:"; color: cMuted; font.pixelSize: 10 }

                        Repeater {
                            model: [
                                "Normalize","Amplify","Fade In","Fade Out",
                                "Reverse","Silence","Invert","Pan"
                            ]
                            Rectangle {
                                height: 24; width: 72; radius: 3
                                color: ma6.containsMouse ? "#2a2a2a" : "#1a1a1a"
                                border.color: ma6.containsMouse ? cAccent : cBorder; border.width: 1
                                Text { anchors.centerIn: parent; text: modelData; color: cText; font.pixelSize: 10 }
                                MouseArea { id: ma6; anchors.fill: parent; hoverEnabled: true
                                    onClicked: { activeEffect = modelData; pushHistory(modelData) }
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Status
                        Text {
                            text: activeEffect !== "" ? "Applied: " + activeEffect : "Ready"
                            color: activeEffect !== "" ? cAccent : cMuted
                            font.pixelSize: 10; font.family: "Courier New"
                        }
                    }
                }
            }

            // ── SPECTRUM PANEL ─────────────────────────────────────────────
            Rectangle {
                anchors.fill: parent; color: cBg
                visible: activePanel === "spectrum"

                ColumnLayout {
                    anchors.fill: parent; spacing: 0

                    Rectangle {
                        height: 28; Layout.fillWidth: true; color: cBar; border.color: cBorder; border.width: 1
                        RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 12
                            Text { text: "FFT Spectrum Analyzer"; color: cAccent; font.pixelSize: 12; font.bold: true }
                            Rectangle { width: 1; height: 18; color: cBorder }
                            Repeater { model: ["512","1024","2048","4096"]
                                Rectangle { height: 20; width: 44; radius: 2
                                    color: modelData === "2048" ? "#1a2a3a" : "transparent"
                                    border.color: modelData === "2048" ? cAccent : cBorder; border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: modelData === "2048" ? cAccent : cMuted; font.pixelSize: 10 }
                                }
                            }
                            Text { text: "Window:"; color: cMuted; font.pixelSize: 10 }
                            ComboBox {
                                width: 90; height: 22; model: ["Hanning","Hamming","Blackman","Flat Top","Rectangular"]
                                background: Rectangle { color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1 }
                                contentItem: Text { text: parent.displayText; color: cText; font.pixelSize: 10; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                            }
                        }
                    }

                    // Spectrum canvas
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true; color: "#050505"
                        Canvas {
                            anchors.fill: parent
                            NumberAnimation on rotation { running: false }
                            property real tick: 0
                            Timer { interval: 80; running: isPlaying; repeat: true; onTriggered: { parent.tick++; parent.requestPaint() } }
                            onPaint: {
                                var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                // Grid
                                ctx.strokeStyle = "#1a1a1a"; ctx.lineWidth = 1;
                                for (var i = 0; i <= 10; i++) {
                                    var y = i * height / 10;
                                    ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(width,y); ctx.stroke();
                                    ctx.fillStyle = "#333"; ctx.font = "9px Courier New";
                                    var db = (10 - i) * 9 - 90;
                                    ctx.fillText(db + " dB", 4, y + 10);
                                }
                                // Frequency labels
                                var freqs = [20,50,100,200,500,1000,2000,5000,10000,20000];
                                freqs.forEach(function(f) {
                                    var x = Math.log10(f/20) / Math.log10(20000/20) * width;
                                    ctx.strokeStyle = "#1a1a1a";
                                    ctx.beginPath(); ctx.moveTo(x,0); ctx.lineTo(x,height); ctx.stroke();
                                    ctx.fillStyle = "#333"; ctx.font = "9px Courier New";
                                    var label = f >= 1000 ? (f/1000) + "k" : f.toString();
                                    ctx.fillText(label, x+2, height-4);
                                });
                                // Bars
                                var bands = 128;
                                for (var b = 0; b < bands; b++) {
                                    var t = b / bands;
                                    var bx = Math.pow(t, 0.5) * width;
                                    var nextBx = Math.pow((b+1)/bands, 0.5) * width;
                                    var barW = Math.max(1, nextBx - bx - 1);
                                    var baseAmp = 0.15 + 0.6 * Math.exp(-Math.pow(t - 0.12, 2) / 0.02)
                                               + 0.35 * Math.exp(-Math.pow(t - 0.35, 2) / 0.05);
                                    var noise = Math.sin(b * 7.3 + tick * 0.4) * 0.08;
                                    var amp = Math.max(0, Math.min(1, baseAmp + noise)) * (isPlaying ? 1 : 0.3);
                                    var barH = amp * (height - 20);
                                    // Gradient fill
                                    var grad = ctx.createLinearGradient(0, height-barH, 0, height);
                                    grad.addColorStop(0, "#E10600");
                                    grad.addColorStop(0.6, "#0066aa");
                                    grad.addColorStop(1, "#003355");
                                    ctx.fillStyle = grad;
                                    ctx.fillRect(bx, height - barH, barW, barH);
                                    // Peak hold
                                    ctx.fillStyle = "#E10600";
                                    ctx.fillRect(bx, height - barH - 3, barW, 2);
                                }
                            }
                        }
                    }
                }
            }

            // ── EQUALIZER PANEL ────────────────────────────────────────────
            Rectangle {
                anchors.fill: parent; color: cBg
                visible: activePanel === "eq"

                ColumnLayout {
                    anchors.fill: parent; spacing: 0

                    Rectangle {
                        height: 28; Layout.fillWidth: true; color: cBar; border.color: cBorder; border.width: 1
                        RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 12
                            Text { text: "10-Band Equalizer"; color: cAccent; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle { width: 60; height: 20; color: "#1a2a1a"; radius: 3; border.color: cGreen; border.width: 1
                                Text { anchors.centerIn: parent; text: "Flat"; color: cGreen; font.pixelSize: 10 }
                                MouseArea { anchors.fill: parent; onClicked: { eqLow=0; eqMid=0; eqHigh=0 } }
                            }
                            Rectangle { width: 60; height: 20; color: "#1a1a2a"; radius: 3; border.color: cAccent; border.width: 1
                                Text { anchors.centerIn: parent; text: "Bypass"; color: cAccent; font.pixelSize: 10 }
                            }
                        }
                    }

                    // EQ curve display
                    Rectangle {
                        Layout.fillWidth: true; height: 160; color: "#050a05"; border.color: cBorder; border.width: 1
                        Canvas {
                            anchors.fill: parent
                            property real l: eqLow; property real m: eqMid; property real h: eqHigh
                            onLChanged: requestPaint(); onMChanged: requestPaint(); onHChanged: requestPaint()
                            onPaint: {
                                var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                // Grid
                                ctx.strokeStyle = "#0d1a0d"; ctx.lineWidth = 1;
                                for (var i = 0; i <= 8; i++) {
                                    var y = i * height / 8;
                                    ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(width,y); ctx.stroke();
                                    var db = 24 - i * 6;
                                    ctx.fillStyle = "#333"; ctx.font = "9px Courier New";
                                    ctx.fillText((db > 0 ? "+" : "") + db + " dB", 4, y+10);
                                }
                                // Frequency bands
                                var bands = [31,62,125,250,500,1000,2000,4000,8000,16000];
                                bands.forEach(function(f, i) {
                                    var x = (i + 0.5) * width / bands.length;
                                    ctx.strokeStyle = "#1a2a1a"; ctx.lineWidth = 1;
                                    ctx.beginPath(); ctx.moveTo(x,0); ctx.lineTo(x,height); ctx.stroke();
                                    ctx.fillStyle = "#444"; ctx.font = "9px Courier New";
                                    var label = f >= 1000 ? (f/1000) + "k" : f;
                                    ctx.fillText(label, x - 8, height - 4);
                                });
                                // EQ curve
                                ctx.strokeStyle = "#E10600"; ctx.lineWidth = 2;
                                ctx.beginPath();
                                for (var x = 0; x < width; x++) {
                                    var t = x / width;
                                    var gain = 0;
                                    if (t < 0.33) gain = l * (1 - t/0.33);
                                    else if (t < 0.66) gain = m * Math.sin((t - 0.33)/0.33 * Math.PI);
                                    else gain = h * (t - 0.66)/0.34;
                                    var y = height/2 - (gain/24) * (height/2 - 10);
                                    if (x === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
                                }
                                ctx.stroke();
                                // Fill under curve
                                ctx.fillStyle = "rgba(0,229,255,0.08)";
                                ctx.lineTo(width, height/2); ctx.lineTo(0, height/2); ctx.closePath(); ctx.fill();
                                // Zero line
                                ctx.strokeStyle = "#1a3a1a"; ctx.lineWidth = 1;
                                ctx.beginPath(); ctx.moveTo(0,height/2); ctx.lineTo(width,height/2); ctx.stroke();
                            }
                        }
                    }

                    // Sliders
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true; color: cPanel
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 16; spacing: 8

                            Repeater {
                                model: [
                                    { freq: "31",  val: -3.0 },
                                    { freq: "62",  val:  1.5 },
                                    { freq: "125", val:  4.0 },
                                    { freq: "250", val:  2.5 },
                                    { freq: "500", val:  0.0 },
                                    { freq: "1k",  val: -1.0 },
                                    { freq: "2k",  val:  3.0 },
                                    { freq: "4k",  val:  5.0 },
                                    { freq: "8k",  val:  2.0 },
                                    { freq: "16k", val: -2.0 }
                                ]
                                ColumnLayout {
                                    Layout.fillHeight: true; spacing: 4

                                    Text { Layout.alignment: Qt.AlignHCenter; text: modelData.val.toFixed(1); color: cAccent; font.pixelSize: 9; font.family: "Courier New" }

                                    Slider {
                                        orientation: Qt.Vertical
                                        Layout.fillHeight: true; width: 24
                                        from: -24; to: 24; value: modelData.val
                                        background: Rectangle {
                                            x: parent.width/2 - 2; y: 0; width: 4; height: parent.height
                                            color: "#1a1a1a"; radius: 2
                                            Rectangle { anchors.bottom: parent.bottom; width: 4; radius: 2
                                                height: parent.height * parent.parent.visualPosition
                                                color: parent.parent.value >= 0 ? root.cAccent : root.cRed }
                                        }
                                        handle: Rectangle {
                                            x: parent.width/2 - 10; y: parent.visualPosition * (parent.height - 20)
                                            width: 20; height: 10; radius: 5
                                            color: "#666"; border.color: "#999"; border.width: 1
                                        }
                                        onValueChanged: {
                                            if (index < 4) eqLow = value;
                                            else if (index < 7) eqMid = value;
                                            else eqHigh = value;
                                        }
                                    }

                                    Text { Layout.alignment: Qt.AlignHCenter; text: modelData.freq; color: cMuted; font.pixelSize: 9 }
                                }
                            }
                        }
                    }
                }
            }

            // ── EFFECTS PANEL ──────────────────────────────────────────────
            Rectangle {
                anchors.fill: parent; color: cBg
                visible: activePanel === "effects"

                RowLayout {
                    anchors.fill: parent; spacing: 0

                    // Effects list
                    Rectangle {
                        width: 180; Layout.fillHeight: true; color: cPanel; border.color: cBorder; border.width: 1
                        ColumnLayout {
                            anchors.fill: parent; spacing: 0
                            Rectangle { height: 28; Layout.fillWidth: true; color: cBar; border.color: cBorder; border.width: 1
                                Text { anchors.centerIn: parent; text: "Effects Chain"; color: cText; font.pixelSize: 11; font.bold: true } }
                            ListView {
                                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                                model: ListModel {
                                    ListElement { name: "Amplify/Fade";   icon: "🔊" }
                                    ListElement { name: "Chorus";         icon: "🎶" }
                                    ListElement { name: "Compressor";     icon: "⊟"  }
                                    ListElement { name: "Delay/Echo";     icon: "↩↩" }
                                    ListElement { name: "Distortion";     icon: "⚡" }
                                    ListElement { name: "Doppler";        icon: "🚗" }
                                    ListElement { name: "Dynamics";       icon: "📊" }
                                    ListElement { name: "Equalizer";      icon: "🎚" }
                                    ListElement { name: "Flanger";        icon: "~"  }
                                    ListElement { name: "Interpolate";    icon: "⋯"  }
                                    ListElement { name: "Invert";         icon: "⊖"  }
                                    ListElement { name: "Mechanize";      icon: "⚙"  }
                                    ListElement { name: "Noise Reduction";icon: "🔇" }
                                    ListElement { name: "Normalize";      icon: "⊕"  }
                                    ListElement { name: "Pan";            icon: "◁▷" }
                                    ListElement { name: "Pitch";          icon: "♪"  }
                                    ListElement { name: "Repair";         icon: "🔧" }
                                    ListElement { name: "Resample";       icon: "⇅"  }
                                    ListElement { name: "Reverb";         icon: "🔘" }
                                    ListElement { name: "Reverse";        icon: "⏪" }
                                    ListElement { name: "Silence";        icon: "⬜" }
                                    ListElement { name: "Stereo";         icon: "◁ ▷"}
                                    ListElement { name: "Time Warp";      icon: "⏳" }
                                    ListElement { name: "Volume Mixer";   icon: "⊞"  }
                                }
                                delegate: Rectangle {
                                    width: ListView.view.width; height: 28
                                    color: name === activeEffect ? "#1a2a3a" : (ma7.containsMouse ? "#1e1e1e" : "transparent")
                                    border.color: name === activeEffect ? cAccent : "transparent"; border.width: 1
                                    RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 6
                                        Text { text: icon; font.pixelSize: 12 }
                                        Text { text: name; color: name === activeEffect ? cAccent : cText; font.pixelSize: 11 }
                                    }
                                    MouseArea { id: ma7; anchors.fill: parent; hoverEnabled: true; onClicked: activeEffect = name }
                                }
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                            }
                        }
                    }

                    // Effect parameters
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true; color: "#0f0f0f"

                        ColumnLayout {
                            anchors.fill: parent; spacing: 0

                            Rectangle { height: 28; Layout.fillWidth: true; color: cBar; border.color: cBorder; border.width: 1
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 8
                                    Text { text: activeEffect !== "" ? activeEffect : "— Select an effect —"; color: activeEffect !== "" ? cAccent : cMuted; font.pixelSize: 12; font.bold: true }
                                    Item { Layout.fillWidth: true }
                                    Rectangle { width: 70; height: 22; color: "#1a2a1a"; radius: 3; border.color: cGreen; border.width: 1
                                        Text { anchors.centerIn: parent; text: "▶  Apply"; color: cGreen; font.pixelSize: 11 }
                                        MouseArea { anchors.fill: parent; onClicked: { pushHistory(activeEffect); activeEffect = "" } }
                                    }
                                    Rectangle { width: 70; height: 22; color: "#1a1a2a"; radius: 3; border.color: "#444"; border.width: 1
                                        Text { anchors.centerIn: parent; text: "Preview"; color: cMuted; font.pixelSize: 11 }
                                    }
                                }
                            }

                            // Pitch effect controls
                            ColumnLayout {
                                visible: activeEffect === "Pitch"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14

                                Text { text: "Pitch Shift"; color: cText; font.pixelSize: 13; font.bold: true }

                                RowLayout { spacing: 12
                                    ColumnLayout { spacing: 4
                                        Text { text: "Semitones"; color: cMuted; font.pixelSize: 10 }
                                        RowLayout { spacing: 6
                                            Slider { width: 200; from: -24; to: 24; value: pitchSemitones; onValueChanged: pitchSemitones = value
                                                background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                    Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: root.cAccent } }
                                                handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                            }
                                            Rectangle { width: 60; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: (pitchSemitones >= 0 ? "+" : "") + pitchSemitones.toFixed(1) + " st"; color: cAccent; font.pixelSize: 11; font.family: "Courier New" }
                                            }
                                        }
                                    }
                                }

                                RowLayout { spacing: 20
                                    ColumnLayout { spacing: 4
                                        Text { text: "Algorithm"; color: cMuted; font.pixelSize: 10 }
                                        ComboBox { width: 160; model: ["High Quality","Fast","Formant Preserve","WSOLA"]
                                            background: Rectangle { color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1 }
                                            contentItem: Text { text: parent.displayText; color: cText; font.pixelSize: 10; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                                        }
                                    }
                                    ColumnLayout { spacing: 4
                                        Text { text: "Formant Correction"; color: cMuted; font.pixelSize: 10 }
                                        Switch { checked: true }
                                    }
                                }
                            }

                            // Reverb controls
                            ColumnLayout {
                                visible: activeEffect === "Reverb"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14

                                Text { text: "Reverb"; color: cText; font.pixelSize: 13; font.bold: true }

                                Repeater {
                                    model: [
                                        { label: "Mix",         val: reverbMix,   min: 0,    max: 1,    fmt: function(v) { return Math.round(v*100) + "%" }, set: function(v) { reverbMix = v } },
                                        { label: "Decay (s)",   val: reverbDecay, min: 0.1,  max: 10,   fmt: function(v) { return v.toFixed(2) + " s" },    set: function(v) { reverbDecay = v } }
                                    ]
                                    RowLayout { spacing: 12
                                        Text { text: modelData.label; color: cMuted; font.pixelSize: 10; width: 80 }
                                        Slider { width: 200; from: modelData.min; to: modelData.max; value: modelData.val; onValueChanged: modelData.set(value)
                                            background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: root.cAccent } }
                                            handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                        }
                                        Rectangle { width: 70; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData.fmt(modelData.val); color: cAccent; font.pixelSize: 11; font.family: "Courier New" }
                                        }
                                    }
                                }

                                RowLayout { spacing: 16
                                    Repeater { model: ["Hall","Room","Plate","Chamber","Spring"]
                                        Rectangle { height: 26; width: 68; radius: 3
                                            color: index === 0 ? "#1a2a3a" : "#1a1a1a"
                                            border.color: index === 0 ? cAccent : cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData; color: index === 0 ? cAccent : cMuted; font.pixelSize: 10 }
                                        }
                                    }
                                }
                            }

                            // Delay controls
                            ColumnLayout {
                                visible: activeEffect === "Delay/Echo"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14

                                Text { text: "Delay / Echo"; color: cText; font.pixelSize: 13; font.bold: true }

                                Repeater {
                                    model: [
                                        { label: "Delay (ms)",  val: delayTime,     min: 1,   max: 2000, fmt: function(v) { return v.toFixed(0) + " ms" }, set: function(v) { delayTime = v } },
                                        { label: "Feedback",    val: delayFeedback, min: 0,   max: 0.99, fmt: function(v) { return Math.round(v*100) + "%" }, set: function(v) { delayFeedback = v } }
                                    ]
                                    RowLayout { spacing: 12
                                        Text { text: modelData.label; color: cMuted; font.pixelSize: 10; width: 80 }
                                        Slider { width: 200; from: modelData.min; to: modelData.max; value: modelData.val; onValueChanged: modelData.set(value)
                                            background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: root.cAccent2 } }
                                            handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                        }
                                        Rectangle { width: 70; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData.fmt(modelData.val); color: cAccent2; font.pixelSize: 11; font.family: "Courier New" }
                                        }
                                    }
                                }

                                RowLayout { spacing: 12
                                    Text { text: "Taps"; color: cMuted; font.pixelSize: 10 }
                                    Repeater { model: 4
                                        Rectangle { width: 32; height: 26; radius: 3
                                            color: index < 2 ? "#2a1a0a" : "#1a1a1a"
                                            border.color: index < 2 ? cAccent2 : cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: (index+1).toString(); color: index < 2 ? cAccent2 : cMuted; font.pixelSize: 11 }
                                        }
                                    }
                                }
                            }

                            // Noise Reduction controls
                            ColumnLayout {
                                visible: activeEffect === "Noise Reduction"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14

                                Text { text: "Noise Reduction"; color: cText; font.pixelSize: 13; font.bold: true }

                                Rectangle { Layout.fillWidth: true; Layout.rightMargin: 20; height: 60; color: "#050a05"; radius: 4; border.color: cBorder; border.width: 1
                                    ColumnLayout { anchors.centerIn: parent; spacing: 4
                                        Text { text: "No noise profile loaded"; color: cMuted; font.pixelSize: 11; Layout.alignment: Qt.AlignHCenter }
                                        Rectangle { width: 160; height: 26; color: "#1a2a1a"; radius: 3; border.color: cGreen; border.width: 1
                                            Text { anchors.centerIn: parent; text: "Learn Noise Profile"; color: cGreen; font.pixelSize: 11 }
                                            MouseArea { anchors.fill: parent }
                                        }
                                    }
                                }

                                Repeater {
                                    model: [
                                        { label: "Reduction (dB)", val: 18.0, min: 0, max: 60 },
                                        { label: "Smoothing",      val: 2.0,  min: 0, max: 10 },
                                        { label: "Transition",     val: 4.0,  min: 0, max: 10 }
                                    ]
                                    RowLayout { spacing: 12
                                        Text { text: modelData.label; color: cMuted; font.pixelSize: 10; width: 100 }
                                        Slider { width: 180; from: modelData.min; to: modelData.max; value: modelData.val
                                            background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: root.cGreen } }
                                            handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                        }
                                        Rectangle { width: 60; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData.val.toFixed(1); color: cGreen; font.pixelSize: 11; font.family: "Courier New" }
                                        }
                                    }
                                }
                            }

                            // ── Normalize ─────────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Normalize"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Normalize"; color: cText; font.pixelSize: 13; font.bold: true }
                                RowLayout { spacing: 12
                                    Text { text: "Target level"; color: cMuted; font.pixelSize: 11; width: 90 }
                                    Slider { width: 180; from: -24; to: 0; value: normalizeTarget; onValueChanged: normalizeTarget = value
                                        background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                            Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: root.cAccent } }
                                        handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                    }
                                    Rectangle { width: 70; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                        Text { anchors.centerIn: parent; text: (normalizeTarget >= 0 ? "+" : "") + normalizeTarget.toFixed(1) + " dB"; color: cAccent; font.pixelSize: 11; font.family: "Courier New" }
                                    }
                                }
                                RowLayout { spacing: 20
                                    ColumnLayout { spacing: 4
                                        Text { text: "Mode"; color: cMuted; font.pixelSize: 10 }
                                        Repeater { model: ["Peak","RMS","Loudness (LUFS)"]
                                            Rectangle { height: 26; width: 110; radius: 3
                                                color: index === 0 ? "#1a2a3a" : "#1a1a1a"
                                                border.color: index === 0 ? cAccent : cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: modelData; color: index === 0 ? cAccent : cMuted; font.pixelSize: 10 }
                                            }
                                        }
                                    }
                                    ColumnLayout { spacing: 4
                                        Text { text: "Options"; color: cMuted; font.pixelSize: 10 }
                                        RowLayout { spacing: 8
                                            CheckBox { checked: true; text: ""; indicator: Rectangle { width:14; height:14; radius:2; color: parent.checked?"#1a2a3a":"#1a1a1a"; border.color: parent.checked?cAccent:cBorder; border.width:1 } }
                                            Text { text: "Independent channels"; color: cText; font.pixelSize: 10 }
                                        }
                                        RowLayout { spacing: 8
                                            CheckBox { checked: false; text: ""; indicator: Rectangle { width:14; height:14; radius:2; color: parent.checked?"#1a2a3a":"#1a1a1a"; border.color: parent.checked?cAccent:cBorder; border.width:1 } }
                                            Text { text: "DC offset correction"; color: cText; font.pixelSize: 10 }
                                        }
                                    }
                                }
                                // Level meter preview
                                Rectangle { width: 300; height: 40; color: "#0a0a0a"; radius: 4; border.color: cBorder; border.width: 1
                                    RowLayout { anchors.fill: parent; anchors.margins: 6; spacing: 4
                                        Text { text: "Peak:"; color: cMuted; font.pixelSize: 10; width: 36 }
                                        Rectangle { Layout.fillWidth: true; height: 20; color: "#111"; radius: 2
                                            Rectangle { width: parent.width * 0.88; height: 20; radius: 2
                                                gradient: Gradient { orientation: Gradient.Horizontal
                                                    GradientStop { position: 0.0; color: "#226633" }
                                                    GradientStop { position: 0.75; color: "#887700" }
                                                    GradientStop { position: 1.0; color: "#cc2222" }
                                                }
                                            }
                                        }
                                        Text { text: "-1.0 dB"; color: cAccent; font.pixelSize: 10; font.family: "Courier New" }
                                    }
                                }
                            }

                            // ── Amplify / Fade ─────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Amplify/Fade"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Amplify / Fade"; color: cText; font.pixelSize: 13; font.bold: true }
                                RowLayout { spacing: 16
                                    ColumnLayout { spacing: 8
                                        Text { text: "Start Volume"; color: cMuted; font.pixelSize: 10 }
                                        RowLayout { spacing: 8
                                            Slider { width: 180; from: -60; to: 12; value: 0
                                                background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                    Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: root.cAccent } }
                                                handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                            }
                                            Rectangle { width: 60; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: "0.0 dB"; color: cAccent; font.pixelSize: 11; font.family: "Courier New" }
                                            }
                                        }
                                        Text { text: "End Volume"; color: cMuted; font.pixelSize: 10 }
                                        RowLayout { spacing: 8
                                            Slider { width: 180; from: -60; to: 12; value: -60
                                                background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                    Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: root.cAccent2 } }
                                                handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                            }
                                            Rectangle { width: 60; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: "-∞ dB"; color: cAccent2; font.pixelSize: 11; font.family: "Courier New" }
                                            }
                                        }
                                    }
                                    // Fade curve preview
                                    Rectangle { width: 140; height: 100; color: "#0a0a0a"; radius: 4; border.color: cBorder; border.width: 1
                                        Canvas { anchors.fill: parent; anchors.margins: 8
                                            onPaint: {
                                                var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                                ctx.strokeStyle = root.cAccent; ctx.lineWidth = 2;
                                                ctx.beginPath(); ctx.moveTo(0,0);
                                                ctx.bezierCurveTo(width*0.3, 0, width*0.7, height, width, height);
                                                ctx.stroke();
                                                ctx.fillStyle = "rgba(0,229,255,0.1)";
                                                ctx.lineTo(0, height); ctx.closePath(); ctx.fill();
                                            }
                                        }
                                        Text { anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottomMargin: 4
                                            text: "Fade Out (Log)"; color: cMuted; font.pixelSize: 9 }
                                    }
                                }
                                RowLayout { spacing: 12
                                    Text { text: "Curve"; color: cMuted; font.pixelSize: 10 }
                                    Repeater { model: ["Linear","Logarithmic","Exponential","S-Curve"]
                                        Rectangle { height: 24; width: 88; radius: 3
                                            color: index === 1 ? "#1a2a3a" : "#1a1a1a"
                                            border.color: index === 1 ? cAccent : cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData; color: index === 1 ? cAccent : cMuted; font.pixelSize: 10 }
                                        }
                                    }
                                }
                            }

                            // ── Chorus ────────────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Chorus"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Chorus"; color: cText; font.pixelSize: 13; font.bold: true }
                                property var chorusParams: [
                                    { label: "Voices",      min: 1,   max: 8,   val: 3,    fmt: function(v){ return Math.round(v).toString() } },
                                    { label: "Depth (ms)",  min: 0,   max: 40,  val: 12.0, fmt: function(v){ return v.toFixed(1)+" ms" } },
                                    { label: "Rate (Hz)",   min: 0.1, max: 10,  val: 1.2,  fmt: function(v){ return v.toFixed(2)+" Hz" } },
                                    { label: "Delay (ms)",  min: 1,   max: 50,  val: 18.0, fmt: function(v){ return v.toFixed(1)+" ms" } },
                                    { label: "Feedback",    min: 0,   max: 0.99,val: 0.25, fmt: function(v){ return Math.round(v*100)+"%" } },
                                    { label: "Mix",         min: 0,   max: 1,   val: 0.5,  fmt: function(v){ return Math.round(v*100)+"%" } }
                                ]
                                Repeater {
                                    model: parent.chorusParams
                                    RowLayout { spacing: 10
                                        Text { text: modelData.label; color: cMuted; font.pixelSize: 11; width: 90 }
                                        Slider { width: 180; from: modelData.min; to: modelData.max; value: modelData.val
                                            background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: root.cAccent } }
                                            handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                        }
                                        Rectangle { width: 80; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData.fmt(modelData.val); color: cAccent; font.pixelSize: 11; font.family: "Courier New" }
                                        }
                                    }
                                }
                                RowLayout { spacing: 12
                                    Text { text: "Waveform"; color: cMuted; font.pixelSize: 10 }
                                    Repeater { model: ["Sine","Triangle","Square","Random"]
                                        Rectangle { height: 24; width: 70; radius: 3
                                            color: index === 0 ? "#1a2a3a" : "#1a1a1a"
                                            border.color: index === 0 ? cAccent : cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData; color: index === 0 ? cAccent : cMuted; font.pixelSize: 10 }
                                        }
                                    }
                                }
                            }

                            // ── Flanger ───────────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Flanger"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Flanger"; color: cText; font.pixelSize: 13; font.bold: true }
                                property var flangerParams: [
                                    { label: "Delay (ms)",  min: 0,   max: 20,  val: 4.5,  fmt: function(v){ return v.toFixed(2)+" ms" } },
                                    { label: "Depth",       min: 0,   max: 1,   val: 0.7,  fmt: function(v){ return Math.round(v*100)+"%" } },
                                    { label: "Rate (Hz)",   min: 0.01,max: 10,  val: 0.35, fmt: function(v){ return v.toFixed(3)+" Hz" } },
                                    { label: "Feedback",    min: -1,  max: 1,   val: 0.6,  fmt: function(v){ return (v>=0?"+":"")+Math.round(v*100)+"%" } },
                                    { label: "Mix",         min: 0,   max: 1,   val: 0.5,  fmt: function(v){ return Math.round(v*100)+"%" } }
                                ]
                                Repeater {
                                    model: parent.flangerParams
                                    RowLayout { spacing: 10
                                        Text { text: modelData.label; color: cMuted; font.pixelSize: 11; width: 90 }
                                        Slider { width: 180; from: modelData.min; to: modelData.max; value: modelData.val
                                            background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: "#aa44ff" } }
                                            handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                        }
                                        Rectangle { width: 80; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData.fmt(modelData.val); color: "#aa44ff"; font.pixelSize: 11; font.family: "Courier New" }
                                        }
                                    }
                                }
                                // LFO visualizer
                                Rectangle { width: 280; height: 60; color: "#0a0a0a"; radius: 4; border.color: cBorder; border.width: 1
                                    Canvas { anchors.fill: parent; anchors.margins: 6
                                        property real tick: 0
                                        Timer { interval: 60; running: root.isPlaying; repeat: true; onTriggered: { parent.tick += 0.08; parent.requestPaint() } }
                                        onPaint: {
                                            var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                            ctx.strokeStyle = "#aa44ff"; ctx.lineWidth = 1.5;
                                            ctx.beginPath();
                                            for (var x = 0; x < width; x++) {
                                                var y = height/2 - (height/2 - 4) * Math.sin(x/width * Math.PI*4 + tick);
                                                if (x===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
                                            }
                                            ctx.stroke();
                                            ctx.strokeStyle = "#333"; ctx.lineWidth = 0.5;
                                            ctx.beginPath(); ctx.moveTo(0,height/2); ctx.lineTo(width,height/2); ctx.stroke();
                                        }
                                    }
                                    Text { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 4
                                        text: "LFO"; color: "#553388"; font.pixelSize: 9 }
                                }
                            }

                            // ── Distortion ────────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Distortion"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Distortion"; color: cText; font.pixelSize: 13; font.bold: true }
                                RowLayout { spacing: 20
                                    ColumnLayout { spacing: 10
                                        Repeater {
                                            model: [
                                                { label: "Drive",     min: 0, max: 100, val: 45, fmt: function(v){ return v.toFixed(0)+"%" }, col: cRed },
                                                { label: "Tone",      min: 0, max: 100, val: 60, fmt: function(v){ return v.toFixed(0)+"%" }, col: cYellow },
                                                { label: "Output",    min: 0, max: 100, val: 75, fmt: function(v){ return v.toFixed(0)+"%" }, col: cAccent },
                                                { label: "Mix",       min: 0, max: 100, val: 80, fmt: function(v){ return v.toFixed(0)+"%" }, col: cAccent }
                                            ]
                                            RowLayout { spacing: 10
                                                Text { text: modelData.label; color: cMuted; font.pixelSize: 11; width: 60 }
                                                Slider { width: 160; from: modelData.min; to: modelData.max; value: modelData.val
                                                    background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                        Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: modelData.col } }
                                                    handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                                }
                                                Rectangle { width: 60; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                    Text { anchors.centerIn: parent; text: modelData.fmt(modelData.val); color: modelData.col; font.pixelSize: 11; font.family: "Courier New" }
                                                }
                                            }
                                        }
                                        RowLayout { spacing: 12
                                            Text { text: "Type"; color: cMuted; font.pixelSize: 10 }
                                            Repeater { model: ["Hard Clip","Soft Clip","Fuzz","Bitcrush","Tube","Wave Shape"]
                                                Rectangle { height: 24; width: 72; radius: 3
                                                    color: index === 1 ? "#2a0a0a" : "#1a1a1a"
                                                    border.color: index === 1 ? cRed : cBorder; border.width: 1
                                                    Text { anchors.centerIn: parent; text: modelData; color: index === 1 ? cRed : cMuted; font.pixelSize: 9 }
                                                }
                                            }
                                        }
                                    }
                                    // Transfer curve
                                    Rectangle { width: 130; height: 130; color: "#050505"; radius: 4; border.color: cBorder; border.width: 1
                                        Canvas { anchors.fill: parent; anchors.margins: 8
                                            onPaint: {
                                                var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                                ctx.strokeStyle = "#222"; ctx.lineWidth = 0.5;
                                                ctx.beginPath(); ctx.moveTo(0,height/2); ctx.lineTo(width,height/2); ctx.stroke();
                                                ctx.beginPath(); ctx.moveTo(width/2,0); ctx.lineTo(width/2,height); ctx.stroke();
                                                // Soft clip curve
                                                ctx.strokeStyle = root.cRed; ctx.lineWidth = 2;
                                                ctx.beginPath();
                                                for (var x = 0; x < width; x++) {
                                                    var inp = (x / width - 0.5) * 4;
                                                    var out = inp / (1 + Math.abs(inp * 1.5));
                                                    var y = height/2 - out * (height/2 - 8);
                                                    if (x===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
                                                }
                                                ctx.stroke();
                                            }
                                        }
                                        Text { anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottomMargin: 3
                                            text: "Transfer"; color: "#552222"; font.pixelSize: 9 }
                                    }
                                }
                            }

                            // ── Doppler ───────────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Doppler"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Doppler Effect"; color: cText; font.pixelSize: 13; font.bold: true }
                                RowLayout { spacing: 20
                                    ColumnLayout { spacing: 10
                                        Repeater {
                                            model: [
                                                { label: "Speed (km/h)",  min: 0,   max: 400, val: 120,  fmt: function(v){ return v.toFixed(0)+" km/h" }, col: cAccent },
                                                { label: "Start dist (m)",min: 1,   max: 500, val: 100,  fmt: function(v){ return v.toFixed(0)+" m" },    col: cAccent },
                                                { label: "End dist (m)",  min: 1,   max: 500, val: 100,  fmt: function(v){ return v.toFixed(0)+" m" },    col: cAccent },
                                                { label: "Direction",     min: -90, max: 90,  val: 0,    fmt: function(v){ return v.toFixed(0)+"°" },     col: cYellow },
                                                { label: "Air temp (°C)", min: -20, max: 50,  val: 20,   fmt: function(v){ return v.toFixed(0)+"°C" },    col: cMuted }
                                            ]
                                            RowLayout { spacing: 10
                                                Text { text: modelData.label; color: cMuted; font.pixelSize: 11; width: 110 }
                                                Slider { width: 160; from: modelData.min; to: modelData.max; value: modelData.val
                                                    background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                        Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: modelData.col } }
                                                    handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                                }
                                                Rectangle { width: 80; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                    Text { anchors.centerIn: parent; text: modelData.fmt(modelData.val); color: modelData.col; font.pixelSize: 11; font.family: "Courier New" }
                                                }
                                            }
                                        }
                                        // Speed of sound info
                                        Rectangle { height: 26; width: 280; color: "#0a1a0a"; radius: 3; border.color: "#1a3a1a"; border.width: 1
                                            Text { anchors.fill: parent; anchors.leftMargin: 8; text: "Speed of sound @ 20°C: 343 m/s  |  Δf = ±41.7 Hz"
                                                color: cGreen; font.pixelSize: 9; font.family: "Courier New"; verticalAlignment: Text.AlignVCenter }
                                        }
                                    }
                                    // Path diagram
                                    Rectangle { width: 140; height: 140; color: "#050505"; radius: 4; border.color: cBorder; border.width: 1
                                        Canvas { anchors.fill: parent; anchors.margins: 10
                                            onPaint: {
                                                var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                                // Road
                                                ctx.strokeStyle = "#333"; ctx.lineWidth = 2;
                                                ctx.beginPath(); ctx.moveTo(0,height*0.6); ctx.lineTo(width,height*0.6); ctx.stroke();
                                                // Listener
                                                ctx.fillStyle = root.cAccent;
                                                ctx.beginPath(); ctx.arc(width*0.5, height*0.2, 6, 0, Math.PI*2); ctx.fill();
                                                ctx.fillStyle = root.cMuted; ctx.font = "8px Courier"; ctx.fillText("Listener", width*0.5-20, height*0.12);
                                                // Source moving
                                                ctx.fillStyle = root.cRed;
                                                ctx.beginPath(); ctx.moveTo(width*0.2, height*0.55); ctx.lineTo(width*0.32, height*0.65); ctx.lineTo(width*0.08, height*0.65); ctx.closePath(); ctx.fill();
                                                // Arrow
                                                ctx.strokeStyle = root.cYellow; ctx.lineWidth = 1.5;
                                                ctx.beginPath(); ctx.moveTo(width*0.3, height*0.6); ctx.lineTo(width*0.8, height*0.6); ctx.stroke();
                                                ctx.beginPath(); ctx.moveTo(width*0.78, height*0.56); ctx.lineTo(width*0.84, height*0.6); ctx.lineTo(width*0.78, height*0.64); ctx.stroke();
                                                // Wave fronts
                                                ctx.strokeStyle = "rgba(255,193,7,0.3)"; ctx.lineWidth = 1;
                                                for (var r = 10; r < 50; r += 12) {
                                                    ctx.beginPath(); ctx.arc(width*0.25, height*0.6, r, -Math.PI, 0); ctx.stroke();
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // ── Pan ───────────────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Pan"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Pan / Balance"; color: cText; font.pixelSize: 13; font.bold: true }
                                property real panStart: 0.0
                                property real panEnd:   0.0
                                RowLayout { spacing: 24
                                    ColumnLayout { spacing: 10
                                        Text { text: "Start Pan"; color: cMuted; font.pixelSize: 10 }
                                        RowLayout { spacing: 8
                                            Text { text: "L"; color: cMuted; font.pixelSize: 10 }
                                            Slider { width: 200; from: -100; to: 100; value: panStart; onValueChanged: panStart = value
                                                background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                    Rectangle {
                                                        x: panStart <= 0 ? parent.width/2 + panStart/100*(parent.width/2) : parent.width/2
                                                        width: Math.abs(panStart)/100*(parent.width/2)
                                                        height:4; color: root.cAccent
                                                    }
                                                }
                                                handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                            }
                                            Text { text: "R"; color: cMuted; font.pixelSize: 10 }
                                            Rectangle { width: 60; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: panStart === 0 ? "C" : (panStart < 0 ? "L " : "R ") + Math.abs(panStart).toFixed(0); color: cAccent; font.pixelSize: 11; font.family: "Courier New" }
                                            }
                                        }
                                        Text { text: "End Pan"; color: cMuted; font.pixelSize: 10 }
                                        RowLayout { spacing: 8
                                            Text { text: "L"; color: cMuted; font.pixelSize: 10 }
                                            Slider { width: 200; from: -100; to: 100; value: panEnd; onValueChanged: panEnd = value
                                                background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                    Rectangle {
                                                        x: panEnd <= 0 ? parent.width/2 + panEnd/100*(parent.width/2) : parent.width/2
                                                        width: Math.abs(panEnd)/100*(parent.width/2)
                                                        height:4; color: root.cAccent2
                                                    }
                                                }
                                                handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                            }
                                            Text { text: "R"; color: cMuted; font.pixelSize: 10 }
                                            Rectangle { width: 60; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: panEnd === 0 ? "C" : (panEnd < 0 ? "L " : "R ") + Math.abs(panEnd).toFixed(0); color: cAccent2; font.pixelSize: 11; font.family: "Courier New" }
                                            }
                                        }
                                    }
                                    ColumnLayout { spacing: 8
                                        Text { text: "Pan Law"; color: cMuted; font.pixelSize: 10 }
                                        Repeater { model: ["-3 dB (Constant Power)","-6 dB (Linear)","0 dB (No compensation)"]
                                            Rectangle { height: 24; width: 200; radius: 3
                                                color: index === 0 ? "#1a2a3a" : "#1a1a1a"
                                                border.color: index === 0 ? cAccent : cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: modelData; color: index === 0 ? cAccent : cMuted; font.pixelSize: 10 }
                                            }
                                        }
                                    }
                                }
                            }

                            // ── Stereo ────────────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Stereo"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Stereo Processor"; color: cText; font.pixelSize: 13; font.bold: true }
                                RowLayout { spacing: 24
                                    ColumnLayout { spacing: 10
                                        Repeater {
                                            model: [
                                                { label: "Width",       min: 0,    max: 200,  val: 100,  fmt: function(v){ return v.toFixed(0)+"%" }, col: cAccent },
                                                { label: "Mid/Side Bal",min: -100, max: 100,  val: 0,    fmt: function(v){ return (v>=0?"+":"")+v.toFixed(0) }, col: cYellow },
                                                { label: "Rotation",    min: -45,  max: 45,   val: 0,    fmt: function(v){ return v.toFixed(1)+"°" }, col: cGreen },
                                                { label: "L Vol",       min: -12,  max: 12,   val: 0,    fmt: function(v){ return (v>=0?"+":"")+v.toFixed(1)+" dB" }, col: cAccent },
                                                { label: "R Vol",       min: -12,  max: 12,   val: 0,    fmt: function(v){ return (v>=0?"+":"")+v.toFixed(1)+" dB" }, col: cAccent }
                                            ]
                                            RowLayout { spacing: 10
                                                Text { text: modelData.label; color: cMuted; font.pixelSize: 11; width: 90 }
                                                Slider { width: 160; from: modelData.min; to: modelData.max; value: modelData.val
                                                    background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                        Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: modelData.col } }
                                                    handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                                }
                                                Rectangle { width: 80; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                    Text { anchors.centerIn: parent; text: modelData.fmt(modelData.val); color: modelData.col; font.pixelSize: 11; font.family: "Courier New" }
                                                }
                                            }
                                        }
                                    }
                                    ColumnLayout { spacing: 8
                                        Text { text: "Conversion"; color: cMuted; font.pixelSize: 10 }
                                        Repeater { model: ["Stereo → Stereo","Mono → Stereo (L=R)","Stereo → Mono (Mix)","Swap L/R","Invert R","Mid/Side Encode","Mid/Side Decode"]
                                            Rectangle { height: 24; width: 180; radius: 3
                                                color: index === 0 ? "#1a2a3a" : "#1a1a1a"
                                                border.color: index === 0 ? cAccent : cBorder; border.width: 1
                                                Text { anchors.fill: parent; anchors.leftMargin: 8; text: modelData; color: index === 0 ? cAccent : cMuted; font.pixelSize: 10; verticalAlignment: Text.AlignVCenter }
                                            }
                                        }
                                    }
                                }
                            }

                            // ── Resample ──────────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Resample"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Resample / Convert"; color: cText; font.pixelSize: 13; font.bold: true }
                                GridLayout { columns: 2; columnSpacing: 24; rowSpacing: 10
                                    Text { text: "Source rate"; color: cMuted; font.pixelSize: 11 }
                                    Rectangle { height: 28; width: 140; color: "#0a1a0a"; radius: 3; border.color: "#1a3a1a"; border.width: 1
                                        Text { anchors.centerIn: parent; text: root.sampleRate + " Hz"; color: cGreen; font.pixelSize: 12; font.family: "Courier New" }
                                    }
                                    Text { text: "Target rate"; color: cMuted; font.pixelSize: 11 }
                                    ComboBox { width: 140; model: ["8000 Hz","11025 Hz","16000 Hz","22050 Hz","32000 Hz","44100 Hz","48000 Hz","88200 Hz","96000 Hz","192000 Hz"]
                                        currentIndex: 6
                                        background: Rectangle { color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1 }
                                        contentItem: Text { text: parent.displayText; color: cText; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                                    }
                                    Text { text: "Bit depth"; color: cMuted; font.pixelSize: 11 }
                                    ComboBox { width: 140; model: ["8-bit","16-bit","24-bit","32-bit Float","64-bit Double"]
                                        currentIndex: 1
                                        background: Rectangle { color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1 }
                                        contentItem: Text { text: parent.displayText; color: cText; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                                    }
                                    Text { text: "Quality"; color: cMuted; font.pixelSize: 11 }
                                    ComboBox { width: 140; model: ["Fast (Linear)","Good (Cubic)","High (Sinc)","Ultra (Sinc 256)"]
                                        currentIndex: 2
                                        background: Rectangle { color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1 }
                                        contentItem: Text { text: parent.displayText; color: cText; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                                    }
                                    Text { text: "Anti-alias filter"; color: cMuted; font.pixelSize: 11 }
                                    Switch { checked: true }
                                    Text { text: "Dithering"; color: cMuted; font.pixelSize: 11 }
                                    ComboBox { width: 140; model: ["None","Rectangular","Triangular","Noise Shaped"]
                                        currentIndex: 3
                                        background: Rectangle { color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1 }
                                        contentItem: Text { text: parent.displayText; color: cText; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                                    }
                                }
                                Rectangle { height: 36; width: 320; color: "#0a0a1a"; radius: 4; border.color: "#1a1a3a"; border.width: 1
                                    Text { anchors.fill: parent; anchors.leftMargin: 12
                                        text: "Output: 48000 Hz  ·  16-bit  ·  " + (root.channels===2?"Stereo":"Mono") + "  ·  ~" + (root.duration * 48000 * 2 * 2 / 1048576).toFixed(1) + " MB"
                                        color: cAccent; font.pixelSize: 10; font.family: "Courier New"; verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }

                            // ── Time Warp ─────────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Time Warp"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Time Warp (Time Stretch)"; color: cText; font.pixelSize: 13; font.bold: true }
                                property real stretchFactor: 1.0
                                RowLayout { spacing: 20
                                    ColumnLayout { spacing: 10
                                        Text { text: "Stretch Factor"; color: cMuted; font.pixelSize: 10 }
                                        RowLayout { spacing: 8
                                            Slider { width: 200; from: 0.25; to: 4.0; value: parent.parent.stretchFactor
                                                onValueChanged: parent.parent.stretchFactor = value
                                                background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                    Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: root.cAccent } }
                                                handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                            }
                                            Rectangle { width: 70; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: parent.parent.stretchFactor.toFixed(3) + "x"; color: cAccent; font.pixelSize: 11; font.family: "Courier New" }
                                            }
                                        }
                                        // Quick presets
                                        Text { text: "Presets"; color: cMuted; font.pixelSize: 10 }
                                        RowLayout { spacing: 6
                                            Repeater { model: [
                                                { label: "½x",    val: 0.5  },
                                                { label: "¾x",    val: 0.75 },
                                                { label: "1x",    val: 1.0  },
                                                { label: "1.5x",  val: 1.5  },
                                                { label: "2x",    val: 2.0  }
                                            ]
                                                Rectangle { height: 26; width: 44; radius: 3
                                                    color: Math.abs(parent.parent.parent.stretchFactor - modelData.val) < 0.01 ? "#1a2a3a" : "#1a1a1a"
                                                    border.color: Math.abs(parent.parent.parent.stretchFactor - modelData.val) < 0.01 ? root.cAccent : root.cBorder; border.width: 1
                                                    Text { anchors.centerIn: parent; text: modelData.label; color: Math.abs(parent.parent.parent.stretchFactor - modelData.val) < 0.01 ? root.cAccent : root.cMuted; font.pixelSize: 11 }
                                                    MouseArea { anchors.fill: parent; onClicked: parent.parent.parent.parent.stretchFactor = modelData.val }
                                                }
                                            }
                                        }
                                        // New duration
                                        Rectangle { height: 32; width: 280; color: "#0a1a0a"; radius: 4; border.color: "#1a3a1a"; border.width: 1
                                            Text { anchors.fill: parent; anchors.leftMargin: 10
                                                text: "New duration: " + root.formatTime(root.duration * parent.parent.stretchFactor) + "  (was " + root.formatTime(root.duration) + ")"
                                                color: cGreen; font.pixelSize: 10; font.family: "Courier New"; verticalAlignment: Text.AlignVCenter
                                            }
                                        }
                                        RowLayout { spacing: 8
                                            CheckBox { checked: true; indicator: Rectangle { width:14; height:14; radius:2; color: parent.checked?"#1a2a3a":"#1a1a1a"; border.color: parent.checked?cAccent:cBorder; border.width:1 } }
                                            Text { text: "Preserve pitch"; color: cText; font.pixelSize: 11 }
                                        }
                                        RowLayout { spacing: 8
                                            Text { text: "Algorithm"; color: cMuted; font.pixelSize: 10 }
                                            ComboBox { width: 180; model: ["WSOLA (Best quality)","Phase Vocoder","Resampling (Fast)","Granular Synthesis"]
                                                background: Rectangle { color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1 }
                                                contentItem: Text { text: parent.displayText; color: cText; font.pixelSize: 10; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                                            }
                                        }
                                    }
                                    // Visual stretch diagram
                                    Rectangle { width: 160; height: 120; color: "#050505"; radius: 4; border.color: cBorder; border.width: 1
                                        Canvas { anchors.fill: parent; anchors.margins: 8
                                            property real sf: parent.parent.stretchFactor
                                            onSfChanged: requestPaint()
                                            onPaint: {
                                                var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                                var srcW = Math.min(width*0.8, width/sf*0.8);
                                                var dstW = Math.min(width*0.8, srcW*sf);
                                                // Source waveform
                                                ctx.fillStyle = "#1a2a3a";
                                                ctx.fillRect(4, 8, srcW, 30);
                                                ctx.strokeStyle = root.cAccent; ctx.lineWidth = 1.5;
                                                ctx.beginPath();
                                                for (var x = 0; x < srcW; x++) {
                                                    var y = 22 + 10*Math.sin(x/srcW*Math.PI*6);
                                                    if (x===0) ctx.moveTo(x+4,y); else ctx.lineTo(x+4,y);
                                                }
                                                ctx.stroke();
                                                ctx.fillStyle = root.cMuted; ctx.font = "8px Courier"; ctx.fillText("Source", 4, 50);
                                                // Arrow
                                                ctx.strokeStyle = root.cYellow; ctx.lineWidth = 1.5;
                                                ctx.beginPath(); ctx.moveTo(width*0.3, 65); ctx.lineTo(width*0.3, 75); ctx.stroke();
                                                ctx.fillStyle = root.cYellow; ctx.font = "9px Courier"; ctx.fillText(sf.toFixed(2)+"x", width*0.35, 73);
                                                // Stretched waveform
                                                ctx.fillStyle = "#0a1a2a";
                                                ctx.fillRect(4, 82, dstW, 28);
                                                ctx.strokeStyle = root.cGreen; ctx.lineWidth = 1.5;
                                                ctx.beginPath();
                                                for (var x2 = 0; x2 < dstW; x2++) {
                                                    var y2 = 96 + 8*Math.sin(x2/dstW*Math.PI*6);
                                                    if (x2===0) ctx.moveTo(x2+4,y2); else ctx.lineTo(x2+4,y2);
                                                }
                                                ctx.stroke();
                                                ctx.fillStyle = root.cMuted; ctx.font = "8px Courier"; ctx.fillText("Output", 4, 118);
                                            }
                                        }
                                    }
                                }
                            }

                            // ── Repair (Click/Hiss/Pop removal) ──────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Repair"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Repair / Restoration"; color: cText; font.pixelSize: 13; font.bold: true }
                                RowLayout { spacing: 24
                                    ColumnLayout { spacing: 12
                                        // Click removal
                                        Rectangle { height: 26; width: 260; color: "#1a1a0a"; radius: 3; border.color: "#3a3a1a"; border.width: 1
                                            RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 8
                                                Text { text: "⚡"; font.pixelSize: 14 }
                                                Text { text: "Click & Pop Removal"; color: cYellow; font.pixelSize: 11; font.bold: true }
                                            }
                                        }
                                        Repeater {
                                            model: [
                                                { label: "Sensitivity",  min: 0, max: 100, val: 65, col: cYellow },
                                                { label: "Smoothing",    min: 0, max: 100, val: 30, col: cYellow }
                                            ]
                                            RowLayout { spacing: 10
                                                Text { text: modelData.label; color: cMuted; font.pixelSize: 11; width: 80 }
                                                Slider { width: 150; from: modelData.min; to: modelData.max; value: modelData.val
                                                    background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                        Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: modelData.col } }
                                                    handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                                }
                                                Rectangle { width: 50; height: 24; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                    Text { anchors.centerIn: parent; text: modelData.val.toFixed(0); color: modelData.col; font.pixelSize: 11; font.family: "Courier New" }
                                                }
                                            }
                                        }
                                        // Hiss removal
                                        Rectangle { height: 26; width: 260; color: "#0a1a1a"; radius: 3; border.color: "#1a3a3a"; border.width: 1
                                            RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 8
                                                Text { text: "〰"; font.pixelSize: 14 }
                                                Text { text: "Hiss Removal"; color: cAccent; font.pixelSize: 11; font.bold: true }
                                            }
                                        }
                                        Repeater {
                                            model: [
                                                { label: "Threshold",   min: -80, max: 0,   val: -45, col: cAccent, fmt: function(v){ return v.toFixed(0)+" dB" } },
                                                { label: "Reduction",   min: 0,   max: 60,  val: 24,  col: cAccent, fmt: function(v){ return v.toFixed(0)+" dB" } },
                                                { label: "FFT Size",    min: 0,   max: 3,   val: 2,   col: cMuted,  fmt: function(v){ return ["512","1024","2048","4096"][Math.round(v)] } }
                                            ]
                                            RowLayout { spacing: 10
                                                Text { text: modelData.label; color: cMuted; font.pixelSize: 11; width: 80 }
                                                Slider { width: 150; from: modelData.min; to: modelData.max; value: modelData.val
                                                    background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                        Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: modelData.col } }
                                                    handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                                }
                                                Rectangle { width: 60; height: 24; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                    Text { anchors.centerIn: parent; text: modelData.fmt(modelData.val); color: modelData.col; font.pixelSize: 11; font.family: "Courier New" }
                                                }
                                            }
                                        }
                                        // Clipping repair
                                        Rectangle { height: 26; width: 260; color: "#1a0a0a"; radius: 3; border.color: "#3a1a1a"; border.width: 1
                                            RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 8
                                                Text { text: "⊠"; font.pixelSize: 14 }
                                                Text { text: "Clipping Repair"; color: cRed; font.pixelSize: 11; font.bold: true }
                                            }
                                        }
                                        RowLayout { spacing: 10
                                            Text { text: "Clipped samples"; color: cMuted; font.pixelSize: 11; width: 100 }
                                            Rectangle { height: 24; width: 80; color: "#2a0a0a"; radius: 3; border.color: "#5a1a1a"; border.width: 1
                                                Text { anchors.centerIn: parent; text: "247"; color: cRed; font.pixelSize: 11; font.family: "Courier New" }
                                            }
                                            Rectangle { height: 24; width: 110; color: "#1a2a1a"; radius: 3; border.color: cGreen; border.width: 1
                                                Text { anchors.centerIn: parent; text: "Repair All"; color: cGreen; font.pixelSize: 11 }
                                            }
                                        }
                                    }
                                    // Spectrogram repair preview
                                    Rectangle { width: 180; height: 240; color: "#050505"; radius: 4; border.color: cBorder; border.width: 1
                                        Canvas { anchors.fill: parent; anchors.margins: 6
                                            onPaint: {
                                                var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                                for (var y = 0; y < height; y++) {
                                                    for (var x = 0; x < width; x++) {
                                                        var freq = (height - y) / height;
                                                        var t = x / width;
                                                        var intensity = 0.1 + 0.7 * Math.exp(-Math.pow(freq - 0.3, 2)/0.05) * Math.abs(Math.sin(t*20));
                                                        intensity += 0.05 * Math.random();
                                                        // Clicks shown in red
                                                        var isClick = (Math.abs(t - 0.3) < 0.01 || Math.abs(t - 0.65) < 0.008) && freq > 0.2;
                                                        if (isClick) ctx.fillStyle = "rgba(220,50,50," + (0.5 + 0.5*Math.random()) + ")";
                                                        else {
                                                            var b = Math.floor(intensity * 255);
                                                            ctx.fillStyle = "rgb(0," + Math.floor(b*0.5) + "," + b + ")";
                                                        }
                                                        ctx.fillRect(x,y,1,1);
                                                    }
                                                }
                                                ctx.fillStyle = "rgba(220,50,50,0.6)";
                                                ctx.strokeStyle = root.cRed; ctx.lineWidth = 1; ctx.setLineDash([3,3]);
                                                ctx.strokeRect(width*0.28, 0, width*0.06, height);
                                                ctx.strokeRect(width*0.625, 0, width*0.03, height);
                                                ctx.setLineDash([]);
                                                ctx.fillStyle = root.cMuted; ctx.font = "8px Courier";
                                                ctx.fillText("Spectrogram", 4, 12);
                                                ctx.fillStyle = root.cRed;
                                                ctx.fillText("■ Click", 4, height-4);
                                            }
                                        }
                                    }
                                }
                            }

                            // ── Mechanize ─────────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Mechanize"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Mechanize (Robot Voice)"; color: cText; font.pixelSize: 13; font.bold: true }
                                Repeater {
                                    model: [
                                        { label: "Frequency (Hz)", min: 50,  max: 500, val: 120, fmt: function(v){ return v.toFixed(0)+" Hz" }, col: cAccent },
                                        { label: "Depth",          min: 0,   max: 1,   val: 0.8, fmt: function(v){ return Math.round(v*100)+"%" }, col: cAccent },
                                        { label: "Mix",            min: 0,   max: 1,   val: 1.0, fmt: function(v){ return Math.round(v*100)+"%" }, col: cGreen }
                                    ]
                                    RowLayout { spacing: 10
                                        Text { text: modelData.label; color: cMuted; font.pixelSize: 11; width: 110 }
                                        Slider { width: 180; from: modelData.min; to: modelData.max; value: modelData.val
                                            background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: modelData.col } }
                                            handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                        }
                                        Rectangle { width: 80; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData.fmt(modelData.val); color: modelData.col; font.pixelSize: 11; font.family: "Courier New" }
                                        }
                                    }
                                }
                                RowLayout { spacing: 12
                                    Text { text: "Mode"; color: cMuted; font.pixelSize: 10 }
                                    Repeater { model: ["Ring Mod","AM","Vocoder","Bitcrush"]
                                        Rectangle { height: 24; width: 80; radius: 3
                                            color: index === 0 ? "#1a2a3a" : "#1a1a1a"
                                            border.color: index === 0 ? cAccent : cBorder; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData; color: index === 0 ? cAccent : cMuted; font.pixelSize: 10 }
                                        }
                                    }
                                }
                            }

                            // ── Interpolate ───────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Interpolate"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Interpolate (Fill Silence / Repair)"; color: cText; font.pixelSize: 13; font.bold: true }
                                Text { text: "Fills the selected region by interpolating from surrounding audio."; color: cMuted; font.pixelSize: 11 }
                                RowLayout { spacing: 20
                                    ColumnLayout { spacing: 8
                                        Text { text: "Method"; color: cMuted; font.pixelSize: 10 }
                                        Repeater { model: ["Linear","Cubic Spline","Fourier","Repeat Pattern","Silence"]
                                            Rectangle { height: 26; width: 140; radius: 3
                                                color: index === 2 ? "#1a2a3a" : "#1a1a1a"
                                                border.color: index === 2 ? cAccent : cBorder; border.width: 1
                                                Text { anchors.fill: parent; anchors.leftMargin: 8; text: modelData; color: index === 2 ? cAccent : cMuted; font.pixelSize: 10; verticalAlignment: Text.AlignVCenter }
                                            }
                                        }
                                        Item { height: 8 }
                                        RowLayout { spacing: 8
                                            Text { text: "Context (ms)"; color: cMuted; font.pixelSize: 11; width: 90 }
                                            Slider { width: 120; from: 1; to: 200; value: 50
                                                background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                    Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: root.cAccent } }
                                                handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                            }
                                            Rectangle { width: 60; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: "50 ms"; color: cAccent; font.pixelSize: 11; font.family: "Courier New" }
                                            }
                                        }
                                    }
                                    Rectangle { width: 200; height: 80; color: "#050505"; radius: 4; border.color: cBorder; border.width: 1
                                        Canvas { anchors.fill: parent; anchors.margins: 8
                                            onPaint: {
                                                var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                                var mid = height/2;
                                                ctx.strokeStyle = root.cAccent; ctx.lineWidth = 1.5;
                                                ctx.beginPath();
                                                for (var x = 0; x < width*0.3; x++) { var y = mid + mid*0.6*Math.sin(x*0.4); if(x===0)ctx.moveTo(x,y);else ctx.lineTo(x,y); }
                                                ctx.stroke();
                                                // Gap with interpolation
                                                ctx.setLineDash([4,3]); ctx.strokeStyle = root.cYellow;
                                                ctx.beginPath();
                                                for (var x2 = width*0.3; x2 < width*0.65; x2++) { var y2 = mid + mid*0.6*Math.sin(x2*0.4); if(x2===width*0.3)ctx.moveTo(x2,y2);else ctx.lineTo(x2,y2); }
                                                ctx.stroke(); ctx.setLineDash([]);
                                                ctx.strokeStyle = root.cAccent; ctx.lineWidth = 1.5;
                                                ctx.beginPath();
                                                for (var x3 = width*0.65; x3 < width; x3++) { var y3 = mid + mid*0.6*Math.sin(x3*0.4); if(x3===width*0.65)ctx.moveTo(x3,y3);else ctx.lineTo(x3,y3); }
                                                ctx.stroke();
                                                ctx.fillStyle = root.cYellow; ctx.font = "8px Courier"; ctx.fillText("interpolated", width*0.3+2, mid-8);
                                            }
                                        }
                                    }
                                }
                            }

                            // ── Volume Mixer ──────────────────────────────────────────
                            ColumnLayout {
                                visible: activeEffect === "Volume Mixer"
                                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14
                                Text { text: "Volume Mixer"; color: cText; font.pixelSize: 13; font.bold: true }
                                RowLayout { spacing: 24
                                    // Per-channel faders
                                    ColumnLayout { spacing: 12
                                        Text { text: "Channel Volumes"; color: cMuted; font.pixelSize: 11; font.bold: true }
                                        Repeater {
                                            model: [
                                                { ch: "Left",   col: "#3d8bcd" },
                                                { ch: "Right",  col: "#2daa6a" },
                                                { ch: "Master", col: cAccent   }
                                            ]
                                            RowLayout { spacing: 10
                                                Text { text: modelData.ch; color: modelData.col; font.pixelSize: 11; width: 50 }
                                                Slider { width: 180; from: -60; to: 12; value: 0
                                                    background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                                        Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: modelData.col } }
                                                    handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                                }
                                                Rectangle { width: 70; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                                    Text { anchors.centerIn: parent; text: "0.0 dB"; color: modelData.col; font.pixelSize: 11; font.family: "Courier New" }
                                                }
                                            }
                                        }
                                        RowLayout { spacing: 12
                                            CheckBox { checked: true; indicator: Rectangle { width:14; height:14; radius:2; color: parent.checked?"#1a2a3a":"#1a1a1a"; border.color: parent.checked?cAccent:cBorder; border.width:1 } }
                                            Text { text: "Link L/R channels"; color: cText; font.pixelSize: 11 }
                                        }
                                    }
                                    // Envelope editor (simple)
                                    ColumnLayout { spacing: 8
                                        Text { text: "Envelope"; color: cMuted; font.pixelSize: 11; font.bold: true }
                                        Rectangle { width: 220; height: 120; color: "#050505"; radius: 4; border.color: cBorder; border.width: 1
                                            Canvas { anchors.fill: parent; anchors.margins: 8
                                                property var pts: [{x:0,y:0.9},{x:0.2,y:0.9},{x:0.5,y:0.5},{x:0.8,y:0.85},{x:1,y:0}]
                                                onPaint: {
                                                    var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                                    ctx.strokeStyle = "#1a3a1a"; ctx.lineWidth = 0.5;
                                                    for(var i=0;i<=4;i++){ctx.beginPath();ctx.moveTo(0,i*height/4);ctx.lineTo(width,i*height/4);ctx.stroke();}
                                                    ctx.strokeStyle = root.cGreen; ctx.lineWidth = 1.5;
                                                    ctx.beginPath();
                                                    pts.forEach(function(p,i){ var px=p.x*width, py=(1-p.y)*height; if(i===0)ctx.moveTo(px,py);else ctx.lineTo(px,py); });
                                                    ctx.stroke();
                                                    ctx.fillStyle = "rgba(0,204,102,0.1)";
                                                    ctx.lineTo(width,height); ctx.lineTo(0,height); ctx.closePath(); ctx.fill();
                                                    pts.forEach(function(p){ ctx.fillStyle=root.cGreen; ctx.beginPath(); ctx.arc(p.x*width,(1-p.y)*height,4,0,Math.PI*2); ctx.fill(); });
                                                }
                                            }
                                        }
                                        Text { text: "Click to add nodes, drag to edit"; color: cMuted; font.pixelSize: 9 }
                                    }
                                }
                            }

                            // Default: no selection
                            Item {
                                visible: activeEffect === ""
                                Layout.fillWidth: true; Layout.fillHeight: true
                                ColumnLayout { anchors.centerIn: parent; spacing: 12
                                    Text { text: "Select an effect from the list"; color: cMuted; font.pixelSize: 14; Layout.alignment: Qt.AlignHCenter }
                                    Text { text: "24 effects available"; color: "#333"; font.pixelSize: 11; Layout.alignment: Qt.AlignHCenter }
                                }
                            }
                        }
                    }
                }
            }

            // ── COMPRESSOR PANEL ───────────────────────────────────────────
            Rectangle {
                anchors.fill: parent; color: cBg
                visible: activePanel === "compressor"

                ColumnLayout {
                    anchors.fill: parent; spacing: 0

                    Rectangle {
                        height: 28; Layout.fillWidth: true; color: cBar; border.color: cBorder; border.width: 1
                        RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 12
                            Text { text: "Dynamics / Compressor"; color: cAccent; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Repeater { model: ["Compressor","Limiter","Expander","Gate"]
                                Rectangle { height: 22; width: 76; radius: 2
                                    color: index === 0 ? "#1a2a3a" : "#1a1a1a"
                                    border.color: index === 0 ? cAccent : cBorder; border.width: 1
                                    Text { anchors.centerIn: parent; text: modelData; color: index === 0 ? cAccent : cMuted; font.pixelSize: 10 }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

                        // Controls
                        ColumnLayout {
                            width: 320; Layout.fillHeight: true; Layout.leftMargin: 20; Layout.topMargin: 20; spacing: 14

                            Repeater {
                                model: [
                                    { label: "Threshold", unit: "dB",  val: compThresh,   min: -60, max: 0,    col: cYellow,  set: function(v) { compThresh = v },   fmt: function(v) { return v.toFixed(1) + " dB" } },
                                    { label: "Ratio",     unit: ":1",  val: compRatio,    min: 1,   max: 20,   col: cAccent,  set: function(v) { compRatio = v },    fmt: function(v) { return v.toFixed(1) + ":1" } },
                                    { label: "Attack",    unit: "ms",  val: compAttack,   min: 0.1, max: 500,  col: cGreen,   set: function(v) { compAttack = v },   fmt: function(v) { return v.toFixed(1) + " ms" } },
                                    { label: "Release",   unit: "ms",  val: compRelease,  min: 1,   max: 5000, col: cGreen,   set: function(v) { compRelease = v },  fmt: function(v) { return v.toFixed(0) + " ms" } },
                                    { label: "Knee",      unit: "dB",  val: 6.0,          min: 0,   max: 24,   col: cAccent2, set: function(v) {},                    fmt: function(v) { return v.toFixed(1) + " dB" } },
                                    { label: "Makeup",    unit: "dB",  val: 3.0,          min: -12, max: 24,   col: cAccent2, set: function(v) {},                    fmt: function(v) { return (v>=0?"+":"") + v.toFixed(1) + " dB" } },
                                    { label: "Mix",       unit: "%",   val: 1.0,          min: 0,   max: 1,    col: cAccent,  set: function(v) {},                    fmt: function(v) { return Math.round(v*100) + "%" } }
                                ]
                                RowLayout { spacing: 10
                                    Text { text: modelData.label; color: cMuted; font.pixelSize: 11; width: 80 }
                                    Slider { width: 160; from: modelData.min; to: modelData.max; value: modelData.val; onValueChanged: modelData.set(value)
                                        background: Rectangle { x:0; y:parent.height/2-2; width:parent.width; height:4; radius:2; color:"#222"
                                            Rectangle { width: parent.width * parent.parent.visualPosition; height:4; radius:2; color: modelData.col } }
                                        handle: Rectangle { x: parent.visualPosition*(parent.width-14); y:4; width:14; height:14; radius:7; color:"#777"; border.color:"#aaa"; border.width:1 }
                                    }
                                    Rectangle { width: 80; height: 26; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                        Text { anchors.centerIn: parent; text: modelData.fmt(modelData.val); color: modelData.col; font.pixelSize: 11; font.family: "Courier New" }
                                    }
                                }
                            }
                        }

                        // Transfer curve
                        Rectangle {
                            Layout.fillWidth: true; Layout.fillHeight: true; color: "#050505"; border.color: cBorder; border.width: 1
                            Canvas {
                                anchors.fill: parent; anchors.margins: 20
                                property real thr: compThresh; property real rat: compRatio
                                onThrChanged: requestPaint(); onRatChanged: requestPaint()
                                onPaint: {
                                    var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                    // Axes
                                    ctx.strokeStyle = "#333"; ctx.lineWidth = 1;
                                    ctx.beginPath(); ctx.moveTo(0,0); ctx.lineTo(0,height); ctx.lineTo(width,height); ctx.stroke();
                                    ctx.fillStyle = "#333"; ctx.font = "9px Courier New";
                                    ctx.fillText("In (dB)", width-50, height-4);
                                    ctx.fillText("Out", 4, 14);
                                    // Grid
                                    for (var i = 0; i <= 6; i++) {
                                        var v = i / 6;
                                        ctx.strokeStyle = "#1a1a1a";
                                        ctx.beginPath(); ctx.moveTo(v*width,0); ctx.lineTo(v*width,height); ctx.stroke();
                                        ctx.beginPath(); ctx.moveTo(0,v*height); ctx.lineTo(width,v*height); ctx.stroke();
                                        var db = -60 + i * 10;
                                        ctx.fillText(db, v*width+2, height-4);
                                    }
                                    // 1:1 line
                                    ctx.strokeStyle = "#333"; ctx.lineWidth = 1; ctx.setLineDash([4,4]);
                                    ctx.beginPath(); ctx.moveTo(0,height); ctx.lineTo(width,0); ctx.stroke();
                                    ctx.setLineDash([]);
                                    // Transfer curve
                                    var thrX = (thr + 60) / 60 * width;
                                    var thrY = height - thrX;
                                    ctx.strokeStyle = root.cYellow; ctx.lineWidth = 2;
                                    ctx.beginPath();
                                    ctx.moveTo(0, height);
                                    ctx.lineTo(thrX, thrY);
                                    // Compressed segment
                                    ctx.lineTo(width, thrY + (width - thrX) / rat);
                                    ctx.stroke();
                                    // Threshold marker
                                    ctx.strokeStyle = "rgba(255,193,7,0.4)"; ctx.lineWidth = 1; ctx.setLineDash([3,3]);
                                    ctx.beginPath(); ctx.moveTo(thrX,0); ctx.lineTo(thrX,height); ctx.stroke();
                                    ctx.setLineDash([]);
                                    ctx.fillStyle = root.cYellow; ctx.font = "10px Courier New";
                                    ctx.fillText("Thr: " + thr.toFixed(1) + " dB", thrX+4, 20);
                                    // GR meter dot (gain reduction indicator)
                                    ctx.fillStyle = root.cRed;
                                    ctx.beginPath(); ctx.arc(thrX + 20, thrY - 10, 5, 0, Math.PI*2); ctx.fill();
                                }
                            }
                        }
                    }
                }
            }

            // ── BATCH PANEL ────────────────────────────────────────────────
            Rectangle {
                anchors.fill: parent; color: cBg
                visible: activePanel === "batch"

                ColumnLayout {
                    anchors.fill: parent; spacing: 0

                    Rectangle {
                        height: 28; Layout.fillWidth: true; color: cBar; border.color: cBorder; border.width: 1
                        RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 12
                            Text { text: "Batch Processing"; color: cAccent; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle { width: 100; height: 22; color: "#1a2a1a"; radius: 3; border.color: cGreen; border.width: 1
                                Text { anchors.centerIn: parent; text: "▶  Run Batch"; color: cGreen; font.pixelSize: 11 }
                            }
                        }
                    }

                    RowLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

                        // File list
                        Rectangle { width: 260; Layout.fillHeight: true; color: cPanel; border.color: cBorder; border.width: 1
                            ColumnLayout { anchors.fill: parent; spacing: 0
                                Rectangle { height: 26; Layout.fillWidth: true; color: cBar; border.color: cBorder; border.width: 1
                                    RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 8
                                        Text { text: "Input Files"; color: cText; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                                        Rectangle { width: 24; height: 20; color: "#1a2a1a"; radius: 3; border.color: cGreen; border.width: 1
                                            Text { anchors.centerIn: parent; text: "+"; color: cGreen; font.pixelSize: 14 }
                                        }
                                        Rectangle { width: 24; height: 20; color: "#2a1a1a"; radius: 3; border.color: cRed; border.width: 1
                                            Text { anchors.centerIn: parent; text: "−"; color: cRed; font.pixelSize: 14 }
                                        }
                                    }
                                }
                                ListView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                                    model: ListModel {
                                        ListElement { name: "engine_int_load.wav";     done: true  }
                                        ListElement { name: "engine_int_coast.wav";    done: false }
                                        ListElement { name: "engine_ext_load.wav";     done: false }
                                        ListElement { name: "exhaust_crackle.wav";     done: false }
                                        ListElement { name: "turbo_int_load.wav";      done: false }
                                    }
                                    delegate: Rectangle { width: ListView.view.width; height: 26
                                        color: index % 2 === 0 ? "#141414" : "#111"
                                        RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 6
                                            Rectangle { width: 8; height: 8; radius: 4; color: done ? root.cGreen : "#444" }
                                            Text { text: name; color: done ? root.cText : root.cMuted; font.pixelSize: 10; elide: Text.ElideMiddle; Layout.fillWidth: true }
                                        }
                                    }
                                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                                }
                            }
                        }

                        // Batch settings
                        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: 16; Layout.topMargin: 16; spacing: 12

                            Text { text: "Effect Chain"; color: cText; font.pixelSize: 12; font.bold: true }
                            RowLayout { spacing: 8
                                Repeater { model: ["Normalize","EQ","Compressor","Fade Out"]
                                    Rectangle { height: 28; width: 90; radius: 3
                                        color: "#1a1a2a"; border.color: cAccent; border.width: 1
                                        RowLayout { anchors.fill: parent; anchors.leftMargin: 6; spacing: 4
                                            Text { text: "⚙"; color: cAccent; font.pixelSize: 11 }
                                            Text { text: modelData; color: cText; font.pixelSize: 10 }
                                        }
                                    }
                                }
                                Rectangle { height: 28; width: 28; radius: 3; color: "#1a2a1a"; border.color: cGreen; border.width: 1
                                    Text { anchors.centerIn: parent; text: "+"; color: cGreen; font.pixelSize: 16 }
                                }
                            }

                            Rectangle { height: 1; Layout.fillWidth: true; color: cBorder }

                            Text { text: "Output Settings"; color: cText; font.pixelSize: 12; font.bold: true }
                            GridLayout { columns: 2; columnSpacing: 16; rowSpacing: 8
                                Text { text: "Format"; color: cMuted; font.pixelSize: 11 }
                                ComboBox { width: 140; model: ["WAV 16-bit","WAV 24-bit","WAV 32-bit Float","FLAC","MP3 320kbps","OGG Vorbis"]
                                    background: Rectangle { color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1 }
                                    contentItem: Text { text: parent.displayText; color: cText; font.pixelSize: 10; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                                }
                                Text { text: "Sample Rate"; color: cMuted; font.pixelSize: 11 }
                                ComboBox { width: 140; model: ["44100 Hz","48000 Hz","96000 Hz","192000 Hz","Original"]
                                    background: Rectangle { color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1 }
                                    contentItem: Text { text: parent.displayText; color: cText; font.pixelSize: 10; verticalAlignment: Text.AlignVCenter; leftPadding: 6 }
                                }
                                Text { text: "Output Folder"; color: cMuted; font.pixelSize: 11 }
                                RowLayout { spacing: 4
                                    Rectangle { width: 160; height: 24; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                        Text { anchors.fill: parent; anchors.leftMargin: 6; text: ".\\ same folder"; color: cMuted; font.pixelSize: 10; verticalAlignment: Text.AlignVCenter } }
                                    Rectangle { width: 26; height: 24; color: "#252526"; radius: 3; border.color: cBorder; border.width: 1
                                        Text { anchors.centerIn: parent; text: "…"; color: cText; font.pixelSize: 12 }
                                    }
                                }
                                Text { text: "Overwrite"; color: cMuted; font.pixelSize: 11 }
                                Switch { checked: false }
                            }

                            Rectangle { height: 1; Layout.fillWidth: true; color: cBorder }
                            Text { text: "Rename Pattern"; color: cText; font.pixelSize: 11; font.bold: true }
                            Rectangle { height: 26; width: 300; color: "#1e1e1e"; radius: 3; border.color: cBorder; border.width: 1
                                TextInput { anchors.fill: parent; anchors.leftMargin: 8; text: "{name}_processed"; color: cText; font.pixelSize: 11; font.family: "Courier New"; verticalAlignment: Text.AlignVCenter }
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }

            // ── CUE POINTS PANEL ──────────────────────────────────────────
            Rectangle {
                anchors.fill: parent; color: cBg
                visible: activePanel === "cue"

                ColumnLayout {
                    anchors.fill: parent; spacing: 0

                    Rectangle {
                        height: 28; Layout.fillWidth: true; color: cBar; border.color: cBorder; border.width: 1
                        RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 12
                            Text { text: "Cue Points & Markers"; color: cAccent; font.pixelSize: 12; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Rectangle { width: 80; height: 22; color: "#1a2a1a"; radius: 3; border.color: cGreen; border.width: 1
                                Text { anchors.centerIn: parent; text: "+ Add Cue"; color: cGreen; font.pixelSize: 11 }
                            }
                            Rectangle { width: 90; height: 22; color: "#1a1a2a"; radius: 3; border.color: cAccent; border.width: 1
                                Text { anchors.centerIn: parent; text: "Export Cues"; color: cAccent; font.pixelSize: 11 }
                            }
                        }
                    }

                    // Mini waveform with cue markers
                    Rectangle { Layout.fillWidth: true; height: 80; color: "#050a05"; border.color: cBorder; border.width: 1
                        Canvas { anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                ctx.fillStyle = "#0a2a0a";
                                ctx.beginPath(); ctx.moveTo(0,height/2);
                                for (var x = 0; x < width; x++) {
                                    var amp = height*0.38 * Math.sin(x*0.18) * (0.4+0.6*Math.abs(Math.sin(x*0.07)));
                                    ctx.lineTo(x, height/2 - amp);
                                }
                                for (var x2 = width-1; x2 >= 0; x2--) {
                                    var amp2 = height*0.38 * Math.sin(x2*0.18) * (0.4+0.6*Math.abs(Math.sin(x2*0.07)));
                                    ctx.lineTo(x2, height/2 + amp2);
                                }
                                ctx.closePath(); ctx.fill();
                                // Cue markers
                                var cues = [0.12, 0.28, 0.45, 0.67, 0.83];
                                var labels = ["A","B","C","D","E"];
                                cues.forEach(function(c, i) {
                                    var cx = c * width;
                                    ctx.strokeStyle = root.cYellow; ctx.lineWidth = 1.5;
                                    ctx.beginPath(); ctx.moveTo(cx, 0); ctx.lineTo(cx, height); ctx.stroke();
                                    ctx.fillStyle = root.cYellow;
                                    ctx.fillRect(cx-8, 0, 16, 14);
                                    ctx.fillStyle = "#000"; ctx.font = "bold 9px Courier New";
                                    ctx.fillText(labels[i], cx-4, 10);
                                });
                            }
                        }
                    }

                    // Cue list
                    Rectangle { Layout.fillWidth: true; height: 26; color: "#111"; border.color: cBorder; border.width: 1
                        RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 0
                            Repeater { model: ["#","Label","Position","Length","Type","Note"]
                                Text { text: modelData; color: cMuted; font.pixelSize: 10; font.bold: true
                                       width: [28,100,100,100,80,200][index]; leftPadding: 4 }
                            }
                        }
                    }
                    ListView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: ListModel {
                            ListElement { num: 1; label: "Intro";      pos: "00:00.000"; len: "00:01.480"; type: "Loop Start"; note: "Engine idle entry" }
                            ListElement { num: 2; label: "Low RPM";    pos: "00:01.480"; len: "00:02.320"; type: "Region";     note: "800-2500 RPM blend" }
                            ListElement { num: 3; label: "Mid RPM";    pos: "00:03.800"; len: "00:02.100"; type: "Region";     note: "2500-5500 RPM" }
                            ListElement { num: 4; label: "High RPM";   pos: "00:05.900"; len: "00:01.750"; type: "Region";     note: "5500-8000 RPM" }
                            ListElement { num: 5; label: "Limiter";    pos: "00:07.650"; len: "00:00.300"; type: "Marker";     note: "Rev limiter hit" }
                            ListElement { num: 6; label: "Outro";      pos: "00:09.400"; len: "00:03.037"; type: "Loop End";   note: "Coast down tail" }
                        }
                        delegate: Rectangle { width: ListView.view.width; height: 28
                            color: index % 2 === 0 ? "#111" : "#0e0e0e"
                            RowLayout { anchors.fill: parent; anchors.leftMargin: 8; spacing: 0
                                Text { text: num;   color: root.cMuted;  font.pixelSize: 10; width: 28 }
                                Text { text: label; color: root.cAccent; font.pixelSize: 10; width: 100 }
                                Text { text: pos;   color: root.cGreen;  font.pixelSize: 10; font.family: "Courier New"; width: 100 }
                                Text { text: len;   color: root.cGreen;  font.pixelSize: 10; font.family: "Courier New"; width: 100 }
                                Rectangle { width: 76; height: 20; radius: 3
                                    color: type.indexOf("Loop") >= 0 ? "#1a1a2a" : type === "Marker" ? "#2a1a0a" : "#0a1a0a"
                                    Text { anchors.centerIn: parent; text: type; color: type.indexOf("Loop") >= 0 ? root.cAccent : type === "Marker" ? root.cYellow : root.cGreen; font.pixelSize: 9 }
                                }
                                Text { text: note;  color: root.cMuted;  font.pixelSize: 10; leftPadding: 8 }
                            }
                        }
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }
                }
            }
        }

        // ── Status bar ─────────────────────────────────────────────────────
        Rectangle {
            height: 22; Layout.fillWidth: true
            color: "#0a0a0a"; border.color: cBorder; border.width: 1

            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8; spacing: 16
                Text { text: "▶ " + filename;              color: cMuted;  font.pixelSize: 9; font.family: "Courier New" }
                Text { text: formatTime(duration);         color: cGreen;  font.pixelSize: 9; font.family: "Courier New" }
                Text { text: sampleRate + " Hz";           color: cMuted;  font.pixelSize: 9; font.family: "Courier New" }
                Text { text: bitDepth + "-bit";            color: cMuted;  font.pixelSize: 9; font.family: "Courier New" }
                Text { text: channels === 2 ? "Stereo" : "Mono"; color: cMuted; font.pixelSize: 9 }
                Text { text: "Sel: " + formatTime(selStart*duration) + " – " + formatTime(selEnd*duration); color: cAccent; font.pixelSize: 9; font.family: "Courier New" }
                Item { Layout.fillWidth: true }
                Text { text: isRecording ? "● REC" : isPlaying ? "▶ PLAY" : "■ STOP"; color: isRecording ? cRed : isPlaying ? cAccent : cMuted; font.pixelSize: 9; font.bold: true }
                Text { text: "Zoom: " + zoomLevel.toFixed(2) + "x"; color: cMuted; font.pixelSize: 9; font.family: "Courier New" }
                Text { text: "Undo: " + (historyIdx+1) + "/" + history.length; color: cMuted; font.pixelSize: 9; font.family: "Courier New" }
            }
        }
    }

    // ── Playhead animation ─────────────────────────────────────────────────
    NumberAnimation {
        target: root; property: "playheadPos"
        running: isPlaying; from: playheadPos; to: 1.0
        duration: (1.0 - playheadPos) * duration * 1000 / playbackRate
        onFinished: { if (isLooping) { playheadPos = 0 } else { isPlaying = false } }
    }
}
