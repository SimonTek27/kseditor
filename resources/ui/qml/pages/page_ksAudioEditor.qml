import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import ksEditor.Audio 1.0
import ksEditor.AudioEffects 1.0

Rectangle {
    id: root
    color: "#121212"

    property string currentFile: ""
    property string selectedEvent: "engine_int"
    property string selectedTab: "Timeline"
    property bool isPlaying: false
    property real selStart: 0.0
    property real selEnd: 0.0
    property real selLength: 0.0
    property real zoomLevel: 1.0

    function eventInfo(name) {
        if (typeof eventDefs !== "undefined" && eventDefs) {
            return eventDefs.eventInfo(name);
        }
        return { cat: "\u2014", desc: "\u2014", parameters: [], loops: false, defaultVolume: 1.0 };
    }

    function eventParams(name) {
        if (typeof eventDefs !== "undefined" && eventDefs)
            return eventDefs.eventParameters(name);
        return [];
    }

    readonly property color cBg:       "#121212"
    readonly property color cPanel:    "#1a1a1a"
    readonly property color cBar:      "#1e1e1e"
    readonly property color cBorder:   "#262626"
    readonly property color cAccent:   "#E10600"
    readonly property color cText:     "#cccccc"
    readonly property color cMuted:    "#666666"
    readonly property color cWave:     "#00e6b8"
    readonly property color cSelected: "#f0c040"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Toolbar ──
        Rectangle {
            height: 36; color: cBar; Layout.fillWidth: true
            border.color: cBorder; border.width: 1
            RowLayout {
                anchors.fill: parent; anchors.margins: 4; spacing: 4
                Button { text: "Open"; flat: true; font.pixelSize: 11; onClicked: { if (waveformBridge) { waveformBridge.loadFile(""); root.requestPaint() } } }
                Button { text: "Save"; flat: true; font.pixelSize: 11 }
                Rectangle { width: 1; height: 20; color: cBorder }
                Button { text: "Cut"; flat: true; font.pixelSize: 11 }
                Button { text: "Copy"; flat: true; font.pixelSize: 11 }
                Button { text: "Paste"; flat: true; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                Text { text: currentFile || "<no file>"; color: cMuted; font.pixelSize: 11; elide: Text.ElideMiddle }
            }
        }

        // ── Transport bar ──
        Rectangle {
            height: 48; color: "#181818"; Layout.fillWidth: true
            border.color: cBorder; border.width: 1
            RowLayout {
                anchors.fill: parent; anchors.margins: 6; spacing: 6
                Button { text: "\u23EE"; width: 34; height: 34; font.pixelSize: 16 }
                Button { text: "\u25B6"; width: 34; height: 34; font.pixelSize: 16;
                    onClicked: isPlaying = !isPlaying; highlighted: isPlaying }
                Button { text: "\u25A0"; width: 34; height: 34; font.pixelSize: 16;
                    onClicked: isPlaying = false }
                Button { text: "\u23ED"; width: 34; height: 34; font.pixelSize: 16 }

                Rectangle { width: 1; height: 26; color: cBorder }
                Button { id: btnRec; text: "\u25CF"; width: 34; height: 34; font.pixelSize: 16
                    contentItem: Text { text: parent.text; anchors.centerIn: parent; color: "#ff4c4c"; font.pixelSize: 16 } }

                Item { Layout.fillWidth: true }

                Button { text: "\u2212"; width: 26; height: 26; font.pixelSize: 14; onClicked: zoomLevel = Math.max(0.1, zoomLevel * 0.8) }
                Slider { id: zoomSlider; width: 100; from: 0.1; to: 10.0; value: zoomLevel; onValueChanged: zoomLevel = value }
                Button { text: "+"; width: 26; height: 26; font.pixelSize: 14; onClicked: zoomLevel = Math.min(10, zoomLevel * 1.25) }

                Rectangle { width: 1; height: 26; color: cBorder }

                Repeater {
                    model: {
                        var p = ["Timeline"];
                        var ep = root.eventParams(selectedEvent);
                        for (var i = 0; i < ep.length; i++) p.push(ep[i]);
                        return p;
                    }
                    Rectangle {
                        height: 26; width: 60; radius: 2
                        color: modelData === selectedTab ? "#333" : "transparent"
                        border.color: modelData === selectedTab ? cBorder : "transparent"; border.width: 1
                        Text { anchors.centerIn: parent; text: modelData;
                            color: modelData === selectedTab ? cText : cMuted; font.pixelSize: 10 }
                        MouseArea { anchors.fill: parent; onClicked: selectedTab = modelData }
                    }
                }
            }
        }

        // ── Main workspace ──
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            // ═══ LEFT: Event browser ═══
            Rectangle {
                width: 180; Layout.fillHeight: true
                color: cPanel; border.color: cBorder; border.width: 1

                ColumnLayout { anchors.fill: parent; spacing: 0
                    Rectangle {
                        height: 24; Layout.fillWidth: true; color: cBar
                        border.color: cBorder; border.width: 1
                        Text { anchors.centerIn: parent; text: "Events"; color: cAccent; font.pixelSize: 10; font.bold: true }
                    }
                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: {
                            if (typeof eventDefs === "undefined" || !eventDefs) return [];
                            var m = [];
                            var cats = eventDefs.categories;
                            for (var c = 0; c < cats.length; c++) {
                                m.push({ label: cats[c], depth: 0, isCat: true });
                                var evs = eventDefs.eventsByCategory(cats[c]);
                                for (var e = 0; e < evs.length; e++)
                                    m.push({ label: evs[e], depth: 1, isCat: false });
                            }
                            return m;
                        }
                        delegate: Rectangle {
                            required property string label
                            required property int depth
                            required property bool isCat
                            width: ListView.view.width; height: 20
                            color: !isCat && label === selectedEvent ? cSelected : "transparent"
                            RowLayout {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left; anchors.leftMargin: 4 + depth * 10; spacing: 3
                                Text { text: isCat ? "\u25B8" : "\u266A"; color: isCat ? cMuted : (label === selectedEvent ? "#121212" : cMuted); font.pixelSize: 8 }
                                Text { text: label; color: label === selectedEvent ? "#121212" : cText; font.pixelSize: 10; font.bold: label === selectedEvent }
                            }
                            MouseArea { anchors.fill: parent; onClicked: { if (!isCat) { selectedEvent = label; root.currentFile = label } } }
                        }
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }
                }
            }

            // ═══ CENTER: Waveform ═══
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: "#101010"; border.color: cBorder; border.width: 1

                ColumnLayout { anchors.fill: parent; spacing: 0
                    Rectangle {
                        height: 22; Layout.fillWidth: true; color: "#181818"
                        border.color: cBorder; border.width: 1
                        Row {
                            anchors.fill: parent; anchors.leftMargin: 4
                            Repeater {
                                model: 30
                                Item {
                                    width: (parent.width - 4) / 30; height: 22
                                    Rectangle { x: 0; y: 2; width: 1; height: 6; color: "#444" }
                                    Text { x: 3; y: 4; text: (index * 0.5).toFixed(1); color: "#666"; font.pixelSize: 8 }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#151515"; clip: true

                        Canvas {
                            id: waveCanvas
                            anchors.fill: parent
                            property real midY: height / 2
                            property real scaleY: height * 0.4

                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.fillStyle = "#151515";
                                ctx.fillRect(0, 0, width, height);

                                ctx.strokeStyle = "#222"; ctx.lineWidth = 1;
                                var step = 40 * root.zoomLevel;
                                for (var x = 0; x < width; x += step) {
                                    ctx.beginPath(); ctx.moveTo(x + 0.5, 0); ctx.lineTo(x + 0.5, height); ctx.stroke();
                                }

                                if (root.selLength > 0.001) {
                                    var sx = root.selStart / (root.selEnd || 1) * width;
                                    var ex = root.selEnd / (root.selEnd || 1) * width;
                                    ctx.fillStyle = "#1a3a5f";
                                    ctx.fillRect(sx, 0, ex - sx, height);
                                }

                                ctx.strokeStyle = "#00aa88";
                                ctx.beginPath(); ctx.moveTo(0, midY + 0.5); ctx.lineTo(width, midY + 0.5); ctx.stroke();

                                if (waveformBridge && waveformBridge.hasData) {
                                    var points = waveformBridge.getWaveformPoints(width, 0);
                                    if (points.length > 0) {
                                        ctx.strokeStyle = root.cWave; ctx.lineWidth = 1.5;
                                        ctx.beginPath();
                                        for (var i = 0; i < points.length; ++i) {
                                            var px = points[i].x * width;
                                            var py = midY - points[i].y * scaleY;
                                            if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
                                        }
                                        ctx.stroke();
                                        ctx.beginPath();
                                        for (var i = 0; i < points.length; ++i) {
                                            var px = points[i].x * width;
                                            var py = midY + points[i].y * scaleY;
                                            if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
                                        }
                                        ctx.stroke();
                                    }
                                } else {
                                    ctx.strokeStyle = root.cWave; ctx.globalAlpha = 0.3;
                                    ctx.beginPath();
                                    for (var i = 0; i < width; ++i) {
                                        var t = i / width * Math.PI * 8 * root.zoomLevel;
                                        var amp = Math.sin(t) * (height * 0.1);
                                        if (i === 0) ctx.moveTo(i, midY); else ctx.lineTo(i, midY - amp);
                                    }
                                    ctx.stroke();
                                    ctx.globalAlpha = 1.0;
                                }
                            }

                            Connections { target: waveformBridge; onDataChanged: waveCanvas.requestPaint() }
                        }

                        Rectangle {
                            id: playhead
                            x: 0; width: 2; height: parent.height; color: cAccent; opacity: 0.7
                            visible: isPlaying
                            NumberAnimation on x {
                                running: isPlaying; from: 0; to: parent ? parent.width : 800; duration: 30000; loops: Animation.Infinite
                            }
                        }

                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onPressed: {
                                if (waveformBridge && waveformBridge.hasData) {
                                    var time = mouse.x / width * (waveformBridge.duration || 1);
                                    selStart = time; selEnd = time; selLength = 0;
                                }
                            }
                            onPositionChanged: {
                                if ((pressed & Qt.LeftButton) && waveformBridge && waveformBridge.hasData) {
                                    selEnd = mouse.x / width * (waveformBridge.duration || 1);
                                    selLength = Math.abs(selEnd - selStart);
                                }
                            }
                            onClicked: {
                                if (mouse.button === Qt.RightButton) {
                                    waveformCtx.popup()
                                }
                            }
                        }
                    }

                    // Mini overview strip
                    Rectangle {
                        height: 16; Layout.fillWidth: true; color: "#111"
                        border.color: cBorder; border.width: 1
                        Canvas {
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.fillStyle = "#151515";
                                ctx.fillRect(0, 0, width, height);
                                if (waveformBridge && waveformBridge.hasData) {
                                    ctx.strokeStyle = "#004488"; ctx.lineWidth = 1;
                                    ctx.beginPath();
                                    for (var x = 0; x < width; x++) {
                                        var y = height/2 + height*0.35 * Math.sin(x/width*Math.PI*40) * 0.5 * (Math.sin(x/width*Math.PI*5)*0.3+0.7);
                                        if (x===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
                                    }
                                    ctx.stroke();
                                }
                            }
                        }
                    }
                }
            }

            // ═══ RIGHT: Info panels ═══
            Rectangle {
                width: 240; Layout.fillHeight: true
                color: cPanel; border.color: cBorder; border.width: 1

                ColumnLayout { anchors.fill: parent; spacing: 0
                    Rectangle {
                        height: 22; Layout.fillWidth: true; color: cBar
                        border.color: cBorder; border.width: 1
                        Text { anchors.centerIn: parent; text: "Selection"; color: cText; font.pixelSize: 10; font.bold: true }
                    }
                    GridLayout {
                        columns: 2; columnSpacing: 6; rowSpacing: 4
                        Layout.fillWidth: true; Layout.margins: 8
                        Text { text: "Start"; color: cMuted; font.pixelSize: 10 }
                        Text { text: selStart.toFixed(3) + "s"; color: cWave; font.pixelSize: 10 }
                        Text { text: "End"; color: cMuted; font.pixelSize: 10 }
                        Text { text: selEnd.toFixed(3) + "s"; color: cWave; font.pixelSize: 10 }
                        Text { text: "Length"; color: cMuted; font.pixelSize: 10 }
                        Text { text: selLength.toFixed(3) + "s"; color: cWave; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; Layout.fillWidth: true; color: cBorder }

                    Rectangle {
                        height: 22; Layout.fillWidth: true; color: cBar
                        border.color: cBorder; border.width: 1
                        Text { anchors.centerIn: parent; text: "File Info"; color: cText; font.pixelSize: 10; font.bold: true }
                    }
                    GridLayout {
                        columns: 2; columnSpacing: 6; rowSpacing: 4
                        Layout.fillWidth: true; Layout.margins: 8
                        Text { text: "Duration"; color: cMuted; font.pixelSize: 10 }
                        Text { text: waveformBridge && waveformBridge.hasData ? waveformBridge.duration.toFixed(2) + "s" : "\u2014"; color: cText; font.pixelSize: 10 }
                        Text { text: "Sample Rate"; color: cMuted; font.pixelSize: 10 }
                        Text { text: waveformBridge ? waveformBridge.sampleRate + " Hz" : "\u2014"; color: cText; font.pixelSize: 10 }
                        Text { text: "Channels"; color: cMuted; font.pixelSize: 10 }
                        Text { text: waveformBridge ? waveformBridge.channelCount.toString() : "\u2014"; color: cText; font.pixelSize: 10 }
                        Text { text: "Peak"; color: cMuted; font.pixelSize: 10 }
                        Text { text: waveformBridge ? waveformBridge.peakAmplitude().toFixed(2) + " dB" : "\u2014"; color: "#10b981"; font.pixelSize: 10 }
                        Text { text: "RMS"; color: cMuted; font.pixelSize: 10 }
                        Text { text: waveformBridge ? waveformBridge.rmsAmplitude().toFixed(2) + " dB" : "\u2014"; color: "#10b981"; font.pixelSize: 10 }
                    }

                    Rectangle { height: 1; Layout.fillWidth: true; color: cBorder }

                    Rectangle {
                        height: 22; Layout.fillWidth: true; color: cBar
                        border.color: cBorder; border.width: 1
                        Text { anchors.centerIn: parent; text: "Event: " + selectedEvent; color: cAccent; font.pixelSize: 10; font.bold: true }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; Layout.margins: 8; spacing: 4
                        RowLayout { spacing: 6
                            Text { text: root.eventInfo(selectedEvent).category || root.eventInfo(selectedEvent).cat; color: cMuted; font.pixelSize: 10 }
                            Rectangle {
                                height: 16; width: 36; radius: 2
                                color: root.eventInfo(selectedEvent).loops ? "#1a3a1a" : "#3a3a3a"
                                border.color: root.eventInfo(selectedEvent).loops ? "#3a5a3a" : cBorder; border.width: 1
                                Text { anchors.centerIn: parent; text: root.eventInfo(selectedEvent).loops ? "Loop" : "Once"; color: root.eventInfo(selectedEvent).loops ? "#80ff80" : cMuted; font.pixelSize: 8 }
                            }
                        }
                        Text { text: root.eventInfo(selectedEvent).description || root.eventInfo(selectedEvent).desc; color: cText; font.pixelSize: 9; wrapMode: Text.WordWrap }
                        Text { text: "Volume: " + (root.eventInfo(selectedEvent).defaultVolume || root.eventInfo(selectedEvent).vol); color: cMuted; font.pixelSize: 9 }

                        Text { text: "Parameters"; color: cMuted; font.pixelSize: 9; font.bold: true; visible: (root.eventInfo(selectedEvent).parameters || root.eventInfo(selectedEvent).params || []).length > 0 }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 3
                            Repeater {
                                model: root.eventInfo(selectedEvent).parameters || root.eventInfo(selectedEvent).params || []
                                RowLayout { spacing: 6
                                    Text { text: modelData; color: "#80c8ff"; font.pixelSize: 9; width: 60 }
                                    Slider {
                                        Layout.fillWidth: true; height: 16
                                        from: 0; to: 1; value: 0.5
                                        background: Rectangle { x: 0; y: 6; width: parent.width; height: 4; radius: 2; color: "#2a2a2a" }
                                    }
                                    Text { text: "0.50"; color: cMuted; font.pixelSize: 8; width: 30 }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    Rectangle {
                        height: 22; Layout.fillWidth: true; color: "#191919"
                        border.color: cBorder; border.width: 1
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 4
                            Text { text: isPlaying ? "Playing" : "Ready"; color: isPlaying ? cAccent : "#10b981"; font.pixelSize: 10 }
                            Text { text: "| " + selectedEvent; color: cMuted; font.pixelSize: 10 }
                            Item { Layout.fillWidth: true }
                            Text { text: waveformBridge && waveformBridge.sampleRate ? waveformBridge.sampleRate + " Hz" : "\u2014"; color: cMuted; font.pixelSize: 10 }
                        }
                    }
                }
            }
        }
    }

    // ── Waveform context menu ──
    Menu {
        id: waveformCtx
        MenuItem {
            text: "Open Audio in Audio Bin"; onTriggered: {
                if (AudioBridge && AudioBridge.getFileName())
                    Qt.openUrlExternally(Qt.resolvedUrl(AudioBridge.getFileName()))
            }
        }
        MenuItem {
            text: "Open in Explorer"; onTriggered: {
                if (AudioBridge && AudioBridge.getFileName())
                    Qt.openUrlExternally("file:///" + AudioBridge.getFileName())
            }
        }
        MenuItem { text: "Open in AudioStudio"; onTriggered: { console.log("Navigate to AudioStudio page") } }
        MenuSeparator {}
        MenuItem { text: "Cut"; onTriggered: { if (AudioBridge) AudioBridge.cut() } }
        MenuItem { text: "Copy"; onTriggered: { if (AudioBridge) AudioBridge.copy() } }
        MenuItem { text: "Paste"; onTriggered: { if (AudioBridge) AudioBridge.paste() } }
        MenuItem { text: "Split"; onTriggered: { if (AudioBridge && selLength > 0) AudioBridge.cut() } }
        MenuItem { text: "Delete"; onTriggered: { if (AudioBridge) AudioBridge.deleteSelection(); waveCanvas.requestPaint() } }
        MenuSeparator {}
        MenuItem { text: "Select All"; onTriggered: { if (waveformBridge && waveformBridge.hasData) { selStart = 0; selEnd = waveformBridge.duration; selLength = selEnd } } }
        MenuItem { text: "Trim to Selection"; onTriggered: { console.log("Trim to Selection (requires AudioBridge)") } }
    }
}
