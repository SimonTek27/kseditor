import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#1a1a1a"

    // ── Palette ────────────────────────────────────────────────────────────
    readonly property color cBg:        "#1a1a1a"
    readonly property color cPanel:     "#252526"
    readonly property color cBar:       "#2d2d2d"
    readonly property color cBorder:    "#3a3a3a"
    readonly property color cAccent:    "#E10600"
    readonly property color cTrackBg:   "#1e3a5f"
    readonly property color cTrackFill: "#2a5298"
    readonly property color cCurve:     "#e0004d"
    readonly property color cText:      "#cccccc"
    readonly property color cMuted:     "#666666"
    readonly property color cSelected:  "#f0c040"

    // ── State ──────────────────────────────────────────────────────────────
    property string selectedEvent: "engine_int"
    property string selectedTab:   "rpms"
    property bool   isPlaying:     false

    property var trackList: [
        { name: "load",      solo: false, mute: false, vol: 0 },
        { name: "load exh",  solo: false, mute: false, vol: 0 },
        { name: "coast",     solo: false, mute: false, vol: 0 },
        { name: "coast exh", solo: false, mute: false, vol: 0 }
    ]

    property var eventTree: {
        "cars": {
            "tatusfa1": [
                "backfire_ext","backfire_int","bodywork","door",
                "engine_ext","engine_int","gear_ext","gear_grind",
                "gear_int","horn_int","horn_ext","limiter",
                "skid_ext","skid_int","starter","tractioncontrol_ext",
                "tractioncontrol_int","transmission_int","transmission_ext",
                "turbo_int","turbo_ext","wheel","wind"
            ]
        },
        "collisions": [],
        "common":     [],
        "showrooms":  [],
        "surfaces":   []
    }

    // ── Root layout: left sidebar | center | right panel ──────────────────
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ════════════════════════════════════════════════════════
        // LEFT: Events / Banks / Assets tree
        // ════════════════════════════════════════════════════════
        Rectangle {
            width: 170
            Layout.fillHeight: true
            color: cPanel
            border.color: cBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Tab bar
                Rectangle {
                    height: 30
                    color: cBar
                    Layout.fillWidth: true
                    RowLayout {
                        anchors.fill: parent
                        spacing: 0
                        Repeater {
                            model: ["Events", "Banks", "Assets"]
                            Rectangle {
                                Layout.fillWidth: true
                                height: 30
                                color: modelData === "Events" ? cPanel : "transparent"
                                border.color: modelData === "Events" ? cAccent : "transparent"
                                border.width: modelData === "Events" ? 0 : 0
                                // bottom border indicator
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width; height: 2
                                    color: modelData === "Events" ? cAccent : "transparent"
                                }
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: modelData === "Events" ? cAccent : cMuted
                                    font.pixelSize: 11
                                }
                            }
                        }
                    }
                }

                // Search
                Rectangle {
                    height: 24; Layout.fillWidth: true
                    color: "#1e1e1e"; border.color: cBorder; border.width: 1
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 6; spacing: 4
                        Text { text: "Q:"; color: cMuted; font.pixelSize: 10 }
                        Rectangle { Layout.fillWidth: true; height: 1; color: "transparent" }
                    }
                }

                // Tree
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: ListModel {
                        ListElement { label: "cars";       depth: 0; expandable: true;  isExpanded: true  }
                        ListElement { label: "tatusfa1";   depth: 1; expandable: true;  isExpanded: true  }
                        ListElement { label: "backfire_ext";   depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "backfire_int";   depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "bodywork";       depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "door";           depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "engine_ext";     depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "engine_int";     depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "gear_ext";       depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "gear_grind";     depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "gear_int";       depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "horn_int";       depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "horn_ext";       depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "limiter";        depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "skid_ext";       depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "skid_int";       depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "starter";        depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "tractioncontrol_ext"; depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "tractioncontrol_int"; depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "transmission_int";    depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "transmission_ext";    depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "turbo_int";      depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "turbo_ext";      depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "wheel";          depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "wind";           depth: 2; expandable: false; isExpanded: false }
                        ListElement { label: "collisions"; depth: 0; expandable: true;  isExpanded: false }
                        ListElement { label: "common";     depth: 0; expandable: true;  isExpanded: false }
                        ListElement { label: "showrooms";  depth: 0; expandable: true;  isExpanded: false }
                        ListElement { label: "surfaces";   depth: 0; expandable: true;  isExpanded: false }
                    }
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 20
                        color: label === selectedEvent ? cSelected : "transparent"

                        MouseArea {
                            anchors.fill: parent
                            onClicked: { if (!expandable) selectedEvent = label }
                        }

                        RowLayout {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 4 + depth * 12
                            spacing: 3

                            Text {
                                text: expandable ? "▼" : "♪"
                                color: expandable ? cMuted : (label === selectedEvent ? "#121212" : cMuted)
                                font.pixelSize: 8
                            }
                            Text {
                                text: label
                                color: label === selectedEvent ? "#121212" : cText
                                font.pixelSize: 10
                                font.bold: label === selectedEvent
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                }
            }
        }

        // ════════════════════════════════════════════════════════
        // CENTER: Timeline + tracks + bottom strip
        // ════════════════════════════════════════════════════════
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ── Top bar: tab strip for open events ───────────────
            Rectangle {
                height: 28; Layout.fillWidth: true
                color: cBar; border.color: cBorder; border.width: 1

                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 4; spacing: 0

                    // Tab: engine_int (active)
                    Rectangle {
                        width: 100; height: 28
                        color: cPanel
                        border.color: cBorder; border.width: 1
                        Rectangle { anchors.top: parent.top; width: parent.width; height: 2; color: cAccent }
                        Text {
                            anchors.centerIn: parent
                            text: "engine_int"; color: cAccent; font.pixelSize: 11
                        }
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            // ── Transport bar ─────────────────────────────────────
            Rectangle {
                height: 38; Layout.fillWidth: true
                color: cBar

                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; spacing: 8

                    // Stop / Play
                    Rectangle {
                        width: 26; height: 26; color: "#3a3a3a"; radius: 3
                        border.color: cBorder; border.width: 1
                        Text { anchors.centerIn: parent; text: "■"; color: cText; font.pixelSize: 12 }
                        MouseArea { anchors.fill: parent; onClicked: isPlaying = false }
                    }
                    Rectangle {
                        width: 26; height: 26
                        color: isPlaying ? cAccent : "#3a3a3a"; radius: 3
                        border.color: cBorder; border.width: 1
                        Text { anchors.centerIn: parent; text: "▶"; color: isPlaying ? "#121212" : cText; font.pixelSize: 12 }
                        MouseArea { anchors.fill: parent; onClicked: isPlaying = !isPlaying }
                    }

                    // Timecode display
                    Rectangle {
                        width: 110; height: 26; color: "#111111"; radius: 3
                        border.color: cBorder; border.width: 1
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 3; spacing: 0
                            Text { text: "TIME"; color: cMuted; font.pixelSize: 7 }
                            Text { text: "00:00.000"; color: cAccent; font.pixelSize: 12; font.family: "Courier New" }
                        }
                    }

                    // Grid / snap buttons
                    Rectangle { width: 26; height: 26; color: "#3a3a3a"; radius: 3; border.color: cBorder; border.width: 1
                        Text { anchors.centerIn: parent; text: "⊞"; color: cText; font.pixelSize: 13 } }
                    Rectangle { width: 26; height: 26; color: "#3a3a3a"; radius: 3; border.color: cBorder; border.width: 1
                        Text { anchors.centerIn: parent; text: "⋮⋮"; color: cText; font.pixelSize: 13 } }

                    // Tabs: Timeline | rpms | throttle
                    Repeater {
                        model: ["Timeline", "rpms", "throttle"]
                        Rectangle {
                            height: 26; width: 64
                            color: modelData === selectedTab ? "#444" : "transparent"
                            border.color: modelData === selectedTab ? cBorder : "transparent"
                            border.width: 1; radius: 2
                            Text { anchors.centerIn: parent; text: modelData; color: modelData === selectedTab ? cText : cMuted; font.pixelSize: 11 }
                            MouseArea { anchors.fill: parent; onClicked: selectedTab = modelData }
                        }
                    }

                    // rpms knob indicator
                    Rectangle {
                        width: 42; height: 26; color: "#111111"; radius: 3; border.color: cBorder; border.width: 1
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 3; spacing: 0
                            Text { text: "rpms"; color: cMuted; font.pixelSize: 7 }
                            Text { text: "4.00"; color: cText; font.pixelSize: 11; font.family: "Courier New" }
                        }
                    }
                    // throttle knob indicator
                    Rectangle {
                        width: 50; height: 26; color: "#111111"; radius: 3; border.color: cBorder; border.width: 1
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 3; spacing: 0
                            Text { text: "throttle"; color: cMuted; font.pixelSize: 7 }
                            Text { text: "0.00"; color: cText; font.pixelSize: 11; font.family: "Courier New" }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            // ── Track area ────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true; height: 580
                color: cBg; clip: true

                RowLayout {
                    anchors.fill: parent; spacing: 0

                    // Left gutter: track labels + controls
                    Rectangle {
                        width: 100; Layout.fillHeight: true; color: cPanel
                        border.color: cBorder; border.width: 1

                        // "Logic Tracks" header
                        Rectangle {
                            id: logicHeader
                            height: 22; width: parent.width
                            color: cBar; border.color: cBorder; border.width: 1
                            Text { anchors.centerIn: parent; text: "Logic Tracks"; color: cMuted; font.pixelSize: 10 }
                        }

                        Column {
                            anchors.top: logicHeader.bottom
                            width: parent.width
                            spacing: 0

                            Repeater {
                                model: root.trackList

                                Rectangle {
                                    width: 100
                                    height: modelData.name === "load" || modelData.name === "coast" ? 130 : 200
                                    color: cPanel; border.color: cBorder; border.width: 1

                                    ColumnLayout {
                                        anchors.fill: parent; anchors.margins: 4; spacing: 3

                                        Text { text: modelData.name; color: cText; font.pixelSize: 11; font.bold: true }

                                        RowLayout {
                                            spacing: 3
                                            // SOLO
                                            Rectangle {
                                                width: 36; height: 16; radius: 2
                                                color: modelData.solo ? "#f0c040" : "#3a3a3a"
                                                border.color: cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: "SOLO"; color: modelData.solo ? "#121212" : cMuted; font.pixelSize: 8; font.bold: true }
                                                MouseArea { anchors.fill: parent; onClicked: modelData.solo = !modelData.solo }
                                            }
                                            // MUTE
                                            Rectangle {
                                                width: 36; height: 16; radius: 2
                                                color: modelData.mute ? "#e05050" : "#3a3a3a"
                                                border.color: cBorder; border.width: 1
                                                Text { anchors.centerIn: parent; text: "MUTE"; color: modelData.mute ? "#ffffff" : cMuted; font.pixelSize: 8; font.bold: true }
                                                MouseArea { anchors.fill: parent; onClicked: modelData.mute = !modelData.mute }
                                            }
                                        }

                                        // Volume knob (visual circle)
                                        Rectangle {
                                            width: 28; height: 28; radius: 14
                                            color: "#111"; border.color: cAccent; border.width: 2
                                            // Knob tick
                                            Rectangle {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                anchors.top: parent.top; anchors.topMargin: 3
                                                width: 2; height: 7; color: cAccent; radius: 1
                                            }
                                        }

                                        // dB label
                                        Text { text: "-∞ dB"; color: cMuted; font.pixelSize: 8 }
                                        Text { text: "Volume   10 dB"; color: cMuted; font.pixelSize: 8 }
                                    }
                                }
                            }
                        }
                    }

                    // ── Timeline ruler + track lanes ──────────────
                    Item {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true

                        // Ruler
                        Rectangle {
                            id: ruler
                            height: 22; width: parent.width; color: "#2a2a2a"; border.color: cBorder; border.width: 1

                            Row {
                                anchors.fill: parent; anchors.leftMargin: 4
                                spacing: 0
                                Repeater {
                                    model: 20
                                    Item {
                                        width: (ruler.width - 4) / 20; height: 22
                                        Rectangle { x: 0; y: 0; width: 1; height: 8; color: cMuted }
                                        Text {
                                            x: 4; y: 4
                                            text: (index * 1000).toString()
                                            color: cMuted; font.pixelSize: 9
                                        }
                                    }
                                }
                            }
                        }

                        // Track lanes
                        Column {
                            anchors.top: ruler.bottom
                            width: parent.width
                            spacing: 0

                            Repeater {
                                model: root.trackList

                                Column {
                                    width: parent.width
                                    spacing: 0

                                    // Waveform lane
                                    Rectangle {
                                        width: parent.width
                                        height: modelData.name === "load" || modelData.name === "coast" ? 90 : 90
                                        color: "#1c1c1c"; border.color: cBorder; border.width: 1
                                        clip: true

                                        // Tinted track background blocks
                                        Row {
                                            anchors.fill: parent; anchors.margins: 2; spacing: 3

                                            Repeater {
                                                model: modelData.name === "load" ? 7 :
                                                       modelData.name === "load exh" ? 4 :
                                                       modelData.name === "coast" ? 5 : 3

                                                Rectangle {
                                                    height: parent.height
                                                    width: modelData.name === "load" ? 140 :
                                                           modelData.name === "load exh" ? 80 : 120
                                                    color: cTrackFill; radius: 2; opacity: 0.85

                                                    // Waveform visualization (sine-like)
                                                    Canvas {
                                                        anchors.fill: parent
                                                        onPaint: {
                                                            var ctx = getContext("2d");
                                                            ctx.clearRect(0,0,width,height);
                                                            ctx.strokeStyle = "#80c8ff";
                                                            ctx.lineWidth = 1.5;
                                                            ctx.beginPath();
                                                            var amp = height * 0.35;
                                                            var mid = height * 0.5;
                                                            for (var x = 0; x < width; x++) {
                                                                var y = mid + amp * Math.sin(x / width * Math.PI * (6 + index * 2)) * Math.sin(x / width * Math.PI);
                                                                if (x === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
                                                            }
                                                            ctx.stroke();
                                                        }
                                                    }
                                                    Text {
                                                        anchors.left: parent.left; anchors.top: parent.top
                                                        anchors.margins: 3
                                                        text: modelData.name + "_" + (index * 1500)
                                                        color: "#aaccff"; font.pixelSize: 8
                                                        elide: Text.ElideRight; width: parent.width - 6
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // Volume curve lane
                                    Rectangle {
                                        width: parent.width
                                        height: modelData.name === "load" || modelData.name === "coast" ? 40 : 110
                                        color: "#161616"; border.color: cBorder; border.width: 1
                                        clip: true

                                        // "Volume  10 dB" labels
                                        Text { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 3
                                            text: "Volume   10 dB"; color: cMuted; font.pixelSize: 8 }
                                        Text { anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 3
                                            text: "-∞ dB"; color: cMuted; font.pixelSize: 8 }

                                        // Volume automation curve
                                        Canvas {
                                            anchors.fill: parent
                                            onPaint: {
                                                var ctx = getContext("2d");
                                                ctx.clearRect(0,0,width,height);
                                                ctx.strokeStyle = root.cCurve;
                                                ctx.lineWidth = 1.5;
                                                ctx.beginPath();
                                                // Draws a simple S-shaped rising then flat curve
                                                var startX = width * 0.08;
                                                var riseEnd = width * 0.25;
                                                ctx.moveTo(startX, height * 0.95);
                                                ctx.bezierCurveTo(
                                                    startX + (riseEnd - startX)*0.4, height*0.95,
                                                    startX + (riseEnd - startX)*0.6, height*0.25,
                                                    riseEnd, height*0.25
                                                );
                                                ctx.lineTo(width, height*0.25);
                                                ctx.stroke();

                                                // dB value labels
                                                ctx.fillStyle = root.cCurve;
                                                ctx.font = "8px Courier New";
                                                ctx.fillText("-80.00", startX, height * 0.92);
                                                ctx.fillText("-0.17", riseEnd + 10, height * 0.22);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Playhead
                        Rectangle {
                            x: 10; y: 0
                            width: 1; height: parent.height
                            color: cAccent; opacity: 0.8
                            visible: isPlaying
                            NumberAnimation on x {
                                running: isPlaying
                                from: 10; to: parent ? parent.width : 800
                                duration: 30000
                                loops: Animation.Infinite
                            }
                        }
                    }
                }
            }

            // ── Master bar ────────────────────────────────────────
            Rectangle {
                height: 24; Layout.fillWidth: true
                color: "#2a2a2a"; border.color: cBorder; border.width: 1

                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 100; spacing: 8

                    Text { text: "Master"; color: cText; font.pixelSize: 11; font.bold: true }
                    Rectangle { width: 26; height: 16; color: "#3a3a3a"; radius: 2; border.color: cBorder; border.width: 1
                        Text { anchors.centerIn: parent; text: "▶"; color: cText; font.pixelSize: 10 }
                        MouseArea { anchors.fill: parent; onClicked: isPlaying = !isPlaying }
                    }
                    Rectangle { width: 36; height: 16; color: "#3a5a3a"; radius: 2
                        Text { anchors.centerIn: parent; text: "100%"; color: "#80ff80"; font.pixelSize: 9 } }
                    Rectangle { width: 36; height: 16; color: "#3a3a5a"; radius: 2
                        Text { anchors.centerIn: parent; text: "100%"; color: "#8080ff"; font.pixelSize: 9 } }
                    Rectangle { width: 36; height: 16; color: "#5a3a3a"; radius: 2
                        Text { anchors.centerIn: parent; text: "REC"; color: "#ff8080"; font.pixelSize: 9 } }
                    Item { Layout.fillWidth: true }
                }
            }

            // ── Mini waveform overview strip ──────────────────────
            Rectangle {
                height: 20; Layout.fillWidth: true; color: "#111"; border.color: cBorder; border.width: 1
                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.clearRect(0,0,width,height);
                        // Overview waveform
                        ctx.strokeStyle = "#004488";
                        ctx.lineWidth = 1;
                        for (var i = 0; i < 4; i++) {
                            ctx.fillStyle = ["#1e3a5f","#1e4a3f","#1e3a5f","#1e3a4f"][i];
                            ctx.fillRect(i * width/4, 0, width/4 - 2, height);
                        }
                        ctx.strokeStyle = "#3377cc";
                        ctx.lineWidth = 1;
                        ctx.beginPath();
                        for (var x = 0; x < width; x++) {
                            var y = height/2 + height*0.35 * Math.sin(x/width*Math.PI*40) * 0.6;
                            if (x===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
                        }
                        ctx.stroke();
                    }
                }
            }

            // ── Bottom panel: Distance atten + Env + LFE + Fader + Panner ──
            Rectangle {
                height: 110; Layout.fillWidth: true
                color: "#1e1e1e"; border.color: cBorder; border.width: 1

                RowLayout {
                    anchors.fill: parent; anchors.margins: 4; spacing: 8

                    // IN label
                    Text { text: "In"; color: cMuted; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter }

                    // Distance Attenuation
                    Rectangle {
                        width: 130; height: 100; color: "#252526"; radius: 4; border.color: cBorder; border.width: 1
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 4; spacing: 2
                            Text { text: "Distance Attenuation"; color: cMuted; font.pixelSize: 9; font.bold: true }
                            // Curve graph
                            Rectangle {
                                width: parent.width - 8; height: 50; color: "#1a1a1a"; radius: 2
                                Canvas {
                                    anchors.fill: parent
                                    onPaint: {
                                        var ctx = getContext("2d");
                                        ctx.clearRect(0,0,width,height);
                                        ctx.strokeStyle = "#f0c040";
                                        ctx.lineWidth = 2;
                                        ctx.beginPath();
                                        ctx.moveTo(0, 4);
                                        ctx.bezierCurveTo(width*0.3, 4, width*0.5, height*0.5, width*0.7, height-4);
                                        ctx.lineTo(width, height-4);
                                        ctx.stroke();
                                    }
                                }
                            }
                            RowLayout {
                                spacing: 3
                                Rectangle { width: 22; height: 16; color: cSelected; radius: 2
                                    Text { anchors.centerIn: parent; text: "▣"; color: "#121212"; font.pixelSize: 10 } }
                                Rectangle { width: 22; height: 16; color: "#3a3a3a"; radius: 2
                                    Text { anchors.centerIn: parent; text: "Off"; color: cMuted; font.pixelSize: 9 } }
                            }
                            Text { text: "Min & Max Distance"; color: cMuted; font.pixelSize: 8 }
                            Rectangle { width: 80; height: 12; color: "#111"; radius: 2
                                Rectangle { width: 30; height: 12; color: "#334"; radius: 2 }
                            }
                            RowLayout { spacing: 10
                                Text { text: "7.00"; color: cText; font.pixelSize: 9 }
                                Text { text: "363"; color: cText; font.pixelSize: 9 }
                            }
                        }
                    }

                    // Envelopment
                    Rectangle {
                        width: 100; height: 100; color: "#252526"; radius: 4; border.color: cBorder; border.width: 1
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 4; spacing: 2
                            Text { text: "Envelopment"; color: cMuted; font.pixelSize: 9; font.bold: true }
                            RowLayout { spacing: 3
                                Repeater { model: ["Auto","User","Off"]
                                    Rectangle { width: 28; height: 16; color: modelData==="User" ? "#3a5a3a" : "#3a3a3a"; radius: 2
                                        Text { anchors.centerIn: parent; text: modelData; color: modelData==="User" ? "#80ff80" : cMuted; font.pixelSize: 8 } }
                                }
                            }
                            Text { text: "Sound Size   Mix Extent"; color: cMuted; font.pixelSize: 8 }
                            RowLayout { spacing: 8
                                // Sound Size knob
                                Rectangle { width: 28; height: 28; radius: 14; color: "#111"; border.color: "#666"; border.width: 2
                                    Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top; anchors.topMargin: 3
                                        width: 2; height: 6; color: "#aaa"; radius: 1 } }
                                // Mix Extent knob
                                Rectangle { width: 28; height: 28; radius: 14; color: "#111"; border.color: "#666"; border.width: 2
                                    Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top; anchors.topMargin: 3
                                        width: 2; height: 6; color: "#aaa"; radius: 1 } }
                            }
                            RowLayout { spacing: 8
                                Text { text: "14.0"; color: cText; font.pixelSize: 9 }
                                Text { text: "0 Deg"; color: cText; font.pixelSize: 9 }
                            }
                        }
                    }

                    // ── LFE + stereo VU meters ─────────────────────────────────────
                    Rectangle {
                        width: 90; height: 100; color: "#1a1a1a"; radius: 4; border.color: cBorder; border.width: 1
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 5; spacing: 3

                            RowLayout { spacing: 4
                                Text { text: "LFE"; color: cMuted; font.pixelSize: 9; font.bold: true; Layout.fillWidth: true }
                                // dB scale labels
                                ColumnLayout { spacing: 0; width: 22
                                    Repeater { model: ["+10","0","-10","-20","-40","−∞"]
                                        Text { text: modelData; color: "#444"; font.pixelSize: 7; font.family: "Courier New";
                                               Layout.alignment: Qt.AlignRight }
                                    }
                                }
                            }

                            RowLayout { spacing: 3; Layout.fillWidth: true; Layout.fillHeight: true
                                // L channel
                                Rectangle {
                                    width: 12; height: 60; color: "#111"; radius: 2; border.color: "#222"; border.width: 1
                                    property real level: 0.72
                                    NumberAnimation on level {
                                        running: root.isPlaying; from: 0.3; to: 0.9; duration: 800
                                        loops: Animation.Infinite; easing.type: Easing.InOutSine
                                    }
                                    Canvas {
                                        anchors.fill: parent
                                        property real lv: parent.level
                                        onLvChanged: requestPaint()
                                        onPaint: {
                                            var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                            var segs = 14, segH = (height-segs) / segs;
                                            for (var i = 0; i < segs; i++) {
                                                var lit = (segs - i) / segs <= lv;
                                                var col = i < 2 ? "#cc2222" : i < 4 ? "#aaaa00" : "#226633";
                                                ctx.fillStyle = lit ? col : "#1a1a1a";
                                                ctx.fillRect(1, height - (i+1)*(segH+1), width-2, segH);
                                            }
                                        }
                                    }
                                }
                                // R channel
                                Rectangle {
                                    width: 12; height: 60; color: "#111"; radius: 2; border.color: "#222"; border.width: 1
                                    property real level: 0.65
                                    NumberAnimation on level {
                                        running: root.isPlaying; from: 0.2; to: 0.85; duration: 650
                                        loops: Animation.Infinite; easing.type: Easing.InOutSine
                                    }
                                    Canvas {
                                        anchors.fill: parent
                                        property real lv: parent.level
                                        onLvChanged: requestPaint()
                                        onPaint: {
                                            var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                            var segs = 14, segH = (height-segs) / segs;
                                            for (var i = 0; i < segs; i++) {
                                                var lit = (segs - i) / segs <= lv;
                                                var col = i < 2 ? "#cc2222" : i < 4 ? "#aaaa00" : "#226633";
                                                ctx.fillStyle = lit ? col : "#1a1a1a";
                                                ctx.fillRect(1, height - (i+1)*(segH+1), width-2, segH);
                                            }
                                        }
                                    }
                                }
                                // LFE channel
                                Rectangle {
                                    width: 12; height: 60; color: "#111"; radius: 2; border.color: "#222"; border.width: 1
                                    property real level: 0.3
                                    NumberAnimation on level {
                                        running: root.isPlaying; from: 0.1; to: 0.55; duration: 1200
                                        loops: Animation.Infinite; easing.type: Easing.InOutSine
                                    }
                                    Canvas {
                                        anchors.fill: parent
                                        property real lv: parent.level
                                        onLvChanged: requestPaint()
                                        onPaint: {
                                            var ctx = getContext("2d"); ctx.clearRect(0,0,width,height);
                                            var segs = 14, segH = (height-segs) / segs;
                                            for (var i = 0; i < segs; i++) {
                                                var lit = (segs - i) / segs <= lv;
                                                ctx.fillStyle = lit ? "#1a4488" : "#111";
                                                ctx.fillRect(1, height - (i+1)*(segH+1), width-2, segH);
                                            }
                                        }
                                    }
                                }
                            }

                            RowLayout { spacing: 4
                                Text { text: "L"; color: "#444"; font.pixelSize: 8; font.family: "Courier New" }
                                Text { text: "R"; color: "#444"; font.pixelSize: 8; font.family: "Courier New" }
                                Text { text: "LFE"; color: "#444"; font.pixelSize: 8; font.family: "Courier New" }
                            }
                        }
                    }

                    // ── Fader (vertical) ────────────────────────────────────────────
                    Rectangle {
                        width: 56; height: 100; color: "#1a1a1a"; radius: 4; border.color: cBorder; border.width: 1
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 5; spacing: 2

                            Text { text: "Fader"; color: cMuted; font.pixelSize: 9; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                            // Vertical fader track
                            Item {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                property real faderPos: 0.75   // 0=bottom 1=top

                                // Track groove
                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: 4; width: 4; height: parent.height - 8
                                    color: "#111"; radius: 2; border.color: "#333"; border.width: 1
                                }
                                // Fill below handle
                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: 4; radius: 2
                                    y: 4 + (parent.height - 8) * (1 - parent.faderPos)
                                    height: (parent.height - 8) * parent.faderPos
                                    color: cAccent; opacity: 0.7
                                }
                                // dB notches
                                Repeater { model: 5
                                    Rectangle {
                                        x: parent.width / 2 - 6
                                        y: 4 + index * (parent.height - 8) / 4
                                        width: 12; height: 1; color: "#333"
                                    }
                                }
                                // Handle
                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: 4 + (parent.height - 8) * (1 - parent.faderPos) - 6
                                    width: 22; height: 12; radius: 3
                                    color: "#555"; border.color: "#888"; border.width: 1
                                    Rectangle { anchors.centerIn: parent; width: 16; height: 1; color: "#222" }
                                    MouseArea {
                                        anchors.fill: parent
                                        drag.target: parent
                                        drag.axis: Drag.YAxis
                                        onPositionChanged: function(mouse) {
                                            var trackH = parent.parent.height - 8;
                                            var newY = parent.y + mouse.y;
                                            var clamped = Math.max(4, Math.min(4 + trackH - 6, newY));
                                            parent.parent.faderPos = 1 - (clamped - 4) / trackH;
                                        }
                                    }
                                }
                            }

                            // dB readout
                            Rectangle {
                                Layout.fillWidth: true; height: 18; color: "#111"; radius: 2
                                border.color: "#333"; border.width: 1
                                Text { anchors.centerIn: parent; text: "0.00 dB"; color: cText; font.pixelSize: 8; font.family: "Courier New" }
                            }

                            // Pre/Post toggle
                            RowLayout { spacing: 2; Layout.alignment: Qt.AlignHCenter
                                property bool post: true
                                Repeater { model: ["Pre", "Post"]
                                    Rectangle {
                                        width: 22; height: 14; radius: 2
                                        color: parent.post === (modelData === "Post") ? "#2a4a3a" : "#2a2a2a"
                                        border.color: parent.post === (modelData === "Post") ? cAccent : "#444"; border.width: 1
                                        Text { anchors.centerIn: parent; text: modelData
                                            color: parent.parent.post === (modelData === "Post") ? cAccent : cMuted; font.pixelSize: 8 }
                                        MouseArea { anchors.fill: parent; onClicked: parent.parent.post = (modelData === "Post") }
                                    }
                                }
                            }
                        }
                    }

                    // ── 3D Panner (full polar display) ───────────────────────────
                    ColumnLayout {
                        spacing: 2; Layout.alignment: Qt.AlignVCenter

                        Text { text: "3D Panner"; color: cMuted; font.pixelSize: 9; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                        Rectangle {
                            width: 90; height: 90; color: "#0f0f0f"; radius: 4
                            border.color: cBorder; border.width: 1

                            // Polar rings
                            Repeater { model: 3
                                Rectangle {
                                    property int sz: 78 - index * 24
                                    anchors.centerIn: parent
                                    width: sz; height: sz; radius: sz/2
                                    color: "transparent"; border.color: "#1e2e2e"; border.width: 1
                                }
                            }
                            // Crosshairs
                            Rectangle { anchors.centerIn: parent; width: 1; height: 80; color: "#222" }
                            Rectangle { anchors.centerIn: parent; width: 80; height: 1; color: "#222" }

                            // Draggable panner dot
                            property real dotX: 45
                            property real dotY: 45
                            Rectangle {
                                x: parent.dotX - 6; y: parent.dotY - 6
                                width: 12; height: 12; radius: 6
                                color: cAccent; opacity: 0.9
                                Rectangle { anchors.centerIn: parent; width: 4; height: 4; radius: 2; color: "#fff" }
                            }
                            // Azimuth line from center
                            Canvas {
                                anchors.fill: parent
                                property real dx: parent.dotX
                                property real dy: parent.dotY
                                onDxChanged: requestPaint()
                                onDyChanged: requestPaint()
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.clearRect(0,0,width,height);
                                    ctx.strokeStyle = root.cAccent;
                                    ctx.lineWidth = 1; ctx.globalAlpha = 0.4;
                                    ctx.beginPath();
                                    ctx.moveTo(width/2, height/2);
                                    ctx.lineTo(dx, dy);
                                    ctx.stroke();
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onPositionChanged: function(mouse) {
                                    parent.dotX = Math.max(5, Math.min(parent.width-5, mouse.x));
                                    parent.dotY = Math.max(5, Math.min(parent.height-5, mouse.y));
                                }
                            }
                        }

                        // Azimuth + Elevation readouts
                        RowLayout { spacing: 8; Layout.alignment: Qt.AlignHCenter
                            ColumnLayout { spacing: 1
                                Text { text: "Az"; color: cMuted; font.pixelSize: 8; Layout.alignment: Qt.AlignHCenter }
                                Text { text: "0°"; color: cText; font.pixelSize: 9; font.family: "Courier New"; Layout.alignment: Qt.AlignHCenter }
                            }
                            ColumnLayout { spacing: 1
                                Text { text: "El"; color: cMuted; font.pixelSize: 8; Layout.alignment: Qt.AlignHCenter }
                                Text { text: "0°"; color: cText; font.pixelSize: 9; font.family: "Courier New"; Layout.alignment: Qt.AlignHCenter }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // ── Panner output (right side circle) ────────────────────────
                    ColumnLayout {
                        spacing: 2; Layout.alignment: Qt.AlignVCenter

                        Text { text: "Panner"; color: cMuted; font.pixelSize: 9; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                        Rectangle {
                            width: 70; height: 70; color: "#0f0f0f"; radius: 35
                            border.color: "#333"; border.width: 1

                            Repeater { model: 2
                                Rectangle {
                                    property int sz: 48 - index * 20
                                    anchors.centerIn: parent; width: sz; height: sz; radius: sz/2
                                    color: "transparent"; border.color: "#1e2e2e"; border.width: 1
                                }
                            }
                            Rectangle { anchors.centerIn: parent; width: 1; height: 60; color: "#1e2e2e" }
                            Rectangle { anchors.centerIn: parent; width: 60; height: 1; color: "#1e2e2e" }
                            Rectangle { anchors.centerIn: parent; width: 8; height: 8; radius: 4; color: cAccent }
                        }

                        // L/R pan readout
                        Rectangle {
                            width: 70; height: 18; color: "#1e1e1e"; radius: 2; border.color: cBorder; border.width: 1
                            Text { anchors.centerIn: parent; text: "C  0.0"; color: cText; font.pixelSize: 9; font.family: "Courier New" }
                        }
                    }

                    // OUT label
                    Text { text: "Out"; color: cMuted; font.pixelSize: 10; Layout.alignment: Qt.AlignVCenter }
                }
            }

            // ── Log bar ───────────────────────────────────────────
            Rectangle {
                height: 48; Layout.fillWidth: true
                color: "#111111"; border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 4; spacing: 1
                    Repeater {
                        model: [
                            "[INFO] Car module loaded",
                            "[INFO] Track module loaded",
                            "[INFO] Physics module loaded",
                            "[INFO] All modules ready"
                        ]
                        Text { text: modelData; color: cMuted; font.pixelSize: 9; font.family: "Courier New" }
                    }
                }
            }
        }

        // ════════════════════════════════════════════════════════
        // RIGHT: Overview + Properties + Master panel
        // ════════════════════════════════════════════════════════
        Rectangle {
            width: 210; Layout.fillHeight: true
            color: cPanel; border.color: cBorder; border.width: 1

            ColumnLayout {
                anchors.fill: parent; spacing: 0

                // ── Overview header ───────────────────────────────
                Rectangle {
                    height: 26; Layout.fillWidth: true; color: cBar
                    border.color: cBorder; border.width: 1
                    Text { anchors.centerIn: parent; text: "Overview"; color: cText; font.pixelSize: 11; font.bold: true }
                }

                // ── 3D Preview section ────────────────────────────
                Rectangle {
                    height: 22; Layout.fillWidth: true; color: "#2a2a2a"
                    border.color: cBorder; border.width: 1
                    property bool expanded: true
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 6
                        Text { text: expanded ? "▼" : "▶"; color: cMuted; font.pixelSize: 9 }
                        Text { text: "3D Preview"; color: cAccent; font.pixelSize: 10; font.bold: true }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true; height: 150; color: "#0f0f0f"
                    // Polar panner display
                    Item {
                        anchors.centerIn: parent; width: 130; height: 130

                        // Outer ring
                        Rectangle {
                            anchors.centerIn: parent
                            width: 128; height: 128; radius: 64
                            color: "transparent"; border.color: "#333"; border.width: 1
                        }
                        // Mid rings
                        Repeater { model: 3
                            Rectangle {
                                property int sz: 88 - index * 28
                                anchors.centerIn: parent
                                width: sz; height: sz; radius: sz/2
                                color: "transparent"; border.color: "#252525"; border.width: 1
                            }
                        }
                        // Crosshairs
                        Rectangle { anchors.centerIn: parent; width: 1; height: 120; color: "#2c2c2c" }
                        Rectangle { anchors.centerIn: parent; width: 120; height: 1; color: "#2c2c2c" }
                        // Diagonal guides
                        Canvas {
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.clearRect(0,0,width,height);
                                ctx.strokeStyle = "#222222";
                                ctx.lineWidth = 1;
                                ctx.beginPath(); ctx.moveTo(15,15); ctx.lineTo(115,115); ctx.stroke();
                                ctx.beginPath(); ctx.moveTo(115,15); ctx.lineTo(15,115); ctx.stroke();
                            }
                        }
                        // Listener (center dot with ring)
                        Rectangle {
                            anchors.centerIn: parent; width: 16; height: 16; radius: 8
                            color: "transparent"; border.color: cAccent; border.width: 1
                        }
                        Rectangle { anchors.centerIn: parent; width: 6; height: 6; radius: 3; color: cAccent }
                        // Compass labels
                        Text { anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.top; anchors.topMargin: 2
                            text: "F"; color: "#444"; font.pixelSize: 8 }
                        Text { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 2
                            text: "B"; color: "#444"; font.pixelSize: 8 }
                        Text { anchors.left: parent.left; anchors.leftMargin: 2; anchors.verticalCenter: parent.verticalCenter
                            text: "L"; color: "#444"; font.pixelSize: 8 }
                        Text { anchors.right: parent.right; anchors.rightMargin: 2; anchors.verticalCenter: parent.verticalCenter
                            text: "R"; color: "#444"; font.pixelSize: 8 }
                    }
                }

                // ── Properties section ────────────────────────────
                Rectangle {
                    height: 22; Layout.fillWidth: true; color: "#2a2a2a"
                    border.color: cBorder; border.width: 1
                    property bool expanded: true
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 6
                        Text { text: "▼"; color: cMuted; font.pixelSize: 9 }
                        Text { text: "Properties"; color: cText; font.pixelSize: 10; font.bold: true }
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.leftMargin: 8; Layout.rightMargin: 8
                    Layout.topMargin: 6; spacing: 4

                    Text { text: "Tags"; color: cMuted; font.pixelSize: 9; font.bold: true }
                    Rectangle {
                        Layout.fillWidth: true; height: 22; color: "#1e1e1e"
                        radius: 3; border.color: cBorder; border.width: 1
                        TextInput {
                            anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6
                            verticalAlignment: Text.AlignVCenter
                            color: cText; font.pixelSize: 10; clip: true
                            placeholderText: "add tag..."
                            placeholderTextColor: "#444"
                        }
                    }

                    Text { text: "User Properties"; color: cMuted; font.pixelSize: 9; font.bold: true }
                    Rectangle {
                        Layout.fillWidth: true; height: 22; color: "#1e1e1e"
                        radius: 3; border.color: cBorder; border.width: 1
                        Text { anchors.left: parent.left; anchors.leftMargin: 6; anchors.verticalCenter: parent.verticalCenter
                            text: "—"; color: "#444"; font.pixelSize: 10 }
                    }

                    Text { text: "Locale"; color: cMuted; font.pixelSize: 9; font.bold: true }
                    Rectangle {
                        Layout.fillWidth: true; height: 22; color: "#1e1e1e"
                        radius: 3; border.color: cBorder; border.width: 1
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 4
                            Text { text: "—"; color: "#444"; font.pixelSize: 10; Layout.fillWidth: true }
                            Text { text: "▼"; color: "#555"; font.pixelSize: 9 }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // ── Master section ────────────────────────────────
                Rectangle {
                    height: 22; Layout.fillWidth: true; color: "#2a2a2a"
                    border.color: cBorder; border.width: 1
                    Text { anchors.centerIn: parent; text: "Master"; color: cText; font.pixelSize: 11; font.bold: true }
                }

                ColumnLayout {
                    Layout.fillWidth: true; Layout.leftMargin: 8; Layout.rightMargin: 8
                    Layout.topMargin: 6; spacing: 6

                    // Row 1: Pitch knob + Max Instances
                    RowLayout {
                        Layout.fillWidth: true; spacing: 8

                        ColumnLayout { spacing: 2
                            Text { text: "Pitch"; color: cMuted; font.pixelSize: 9 }
                            // Rotary knob
                            Rectangle {
                                width: 38; height: 38; radius: 19
                                color: "#0f0f0f"; border.color: "#555"; border.width: 2
                                Canvas {
                                    anchors.fill: parent
                                    property real angle: -0.1  // near center
                                    onPaint: {
                                        var ctx = getContext("2d");
                                        ctx.clearRect(0,0,width,height);
                                        var cx = width/2, cy = height/2, r = width/2 - 4;
                                        // Arc track
                                        ctx.beginPath();
                                        ctx.arc(cx, cy, r, Math.PI * 0.75, Math.PI * 2.25);
                                        ctx.strokeStyle = "#333"; ctx.lineWidth = 3; ctx.stroke();
                                        // Value arc
                                        ctx.beginPath();
                                        ctx.arc(cx, cy, r, Math.PI * 1.5, Math.PI * 1.5 + angle);
                                        ctx.strokeStyle = root.cAccent; ctx.lineWidth = 3; ctx.stroke();
                                        // Tick
                                        var tx = cx + (r-2) * Math.cos(Math.PI * 1.5 + angle);
                                        var ty = cy + (r-2) * Math.sin(Math.PI * 1.5 + angle);
                                        ctx.beginPath();
                                        ctx.moveTo(cx, cy); ctx.lineTo(tx, ty);
                                        ctx.strokeStyle = "#aaa"; ctx.lineWidth = 1.5; ctx.stroke();
                                    }
                                }
                            }
                            Text { text: "0.00 st"; color: cText; font.pixelSize: 9; font.family: "Courier New" }
                        }

                        ColumnLayout { spacing: 2; Layout.fillWidth: true
                            Text { text: "Max Instances"; color: cMuted; font.pixelSize: 9 }
                            Rectangle {
                                width: 60; height: 22; color: "#1e1e1e"; radius: 3
                                border.color: cBorder; border.width: 1
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 4
                                    Text { text: "on"; color: cAccent; font.pixelSize: 10; Layout.fillWidth: true }
                                    Text { text: "▼"; color: "#555"; font.pixelSize: 9 }
                                }
                            }
                            // Stealing mode
                            Rectangle {
                                width: 70; height: 22; color: "#1e1e1e"; radius: 3
                                border.color: cBorder; border.width: 1
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 4
                                    Text { text: "Stealing"; color: cText; font.pixelSize: 9; Layout.fillWidth: true }
                                    Text { text: "▼"; color: "#555"; font.pixelSize: 9 }
                                }
                            }
                            // Virtualize toggle
                            Rectangle {
                                width: 80; height: 22; color: "#1e1e1e"; radius: 3
                                border.color: cBorder; border.width: 1
                                property bool on: true
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 4
                                    Text { text: "Virtualize"; color: cMuted; font.pixelSize: 9; Layout.fillWidth: true }
                                    Rectangle {
                                        width: 30; height: 16; radius: 2
                                        color: parent.parent.on ? "#1a4a3a" : "#3a3a3a"
                                        border.color: parent.parent.on ? cAccent : "#555"; border.width: 1
                                        Text { anchors.centerIn: parent; text: parent.parent.on ? "On" : "Off"
                                            color: parent.parent.on ? cAccent : cMuted; font.pixelSize: 9 }
                                        MouseArea { anchors.fill: parent; onClicked: parent.parent.parent.parent.on = !parent.parent.parent.parent.on }
                                    }
                                }
                            }
                        }
                    }

                    // Row 2: Doppler + Cooldown
                    RowLayout { Layout.fillWidth: true; spacing: 8
                        ColumnLayout { spacing: 2
                            Text { text: "Doppler"; color: cMuted; font.pixelSize: 9 }
                            Rectangle {
                                width: 44; height: 22; color: "#1e1e1e"; radius: 3
                                border.color: cBorder; border.width: 1
                                property bool on: false
                                Text { anchors.centerIn: parent; text: on ? "ON" : "OFF"
                                    color: on ? cAccent : cMuted; font.pixelSize: 9; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: parent.on = !parent.on }
                            }
                        }
                        ColumnLayout { spacing: 2; Layout.fillWidth: true
                            Text { text: "Cooldown"; color: cMuted; font.pixelSize: 9 }
                            Rectangle {
                                Layout.fillWidth: true; height: 22; color: "#1e1e1e"; radius: 3
                                border.color: cBorder; border.width: 1
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 4
                                    Text { text: "0.00 ms"; color: cText; font.pixelSize: 9; font.family: "Courier New"; Layout.fillWidth: true }
                                }
                            }
                        }
                    }

                    // Row 3: Scale + Priority
                    RowLayout { Layout.fillWidth: true; spacing: 8
                        ColumnLayout { spacing: 2
                            Text { text: "Scale"; color: cMuted; font.pixelSize: 9 }
                            Rectangle {
                                width: 44; height: 22; color: "#1e1e1e"; radius: 3
                                border.color: cBorder; border.width: 1
                                Text { anchors.centerIn: parent; text: "100%"; color: cText; font.pixelSize: 9 }
                            }
                        }
                        ColumnLayout { spacing: 2; Layout.fillWidth: true
                            Text { text: "Priority"; color: cMuted; font.pixelSize: 9 }
                            Rectangle {
                                Layout.fillWidth: true; height: 22; color: "#1a3a1a"; radius: 3
                                border.color: "#3a5a3a"; border.width: 1
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 4
                                    Text { text: "Medium"; color: "#80ff80"; font.pixelSize: 9; Layout.fillWidth: true }
                                    Text { text: "▼"; color: "#555"; font.pixelSize: 9 }
                                }
                            }
                        }
                    }
                }

                // ── Event Macros section ──────────────────────────
                Rectangle {
                    height: 22; Layout.fillWidth: true; color: "#2a2a2a"
                    Layout.topMargin: 6
                    border.color: cBorder; border.width: 1
                    Text { anchors.centerIn: parent; text: "Event Macros"; color: cText; font.pixelSize: 11; font.bold: true }
                }
                // Macro slots
                ColumnLayout {
                    Layout.fillWidth: true; Layout.leftMargin: 8; Layout.rightMargin: 8
                    Layout.topMargin: 4; Layout.bottomMargin: 4; spacing: 3

                    Repeater {
                        model: ["Macro 1", "Macro 2", "Macro 3"]
                        Rectangle {
                            Layout.fillWidth: true; height: 22; color: "#1e1e1e"
                            radius: 3; border.color: cBorder; border.width: 1
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 4
                                Text { text: modelData; color: cMuted; font.pixelSize: 9; Layout.fillWidth: true }
                                Rectangle { width: 14; height: 14; radius: 7; color: "#333"; border.color: "#555"; border.width: 1
                                    Text { anchors.centerIn: parent; text: "+"; color: cMuted; font.pixelSize: 10 } }
                            }
                        }
                    }

                    // Add macro button
                    Rectangle {
                        Layout.fillWidth: true; height: 22; color: "transparent"
                        border.color: "#444"; border.width: 1; radius: 3
                        Text { anchors.centerIn: parent; text: "+ Add Macro"; color: "#555"; font.pixelSize: 9 }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                    }
                }

                // ── Platform / Live Update strip ──────────────────
                Rectangle {
                    height: 28; Layout.fillWidth: true; color: "#1a1a1a"
                    border.color: cBorder; border.width: 1

                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 6

                        // Live Update toggle
                        Rectangle {
                            width: 80; height: 20; color: "#1e1e1e"; radius: 3
                            border.color: cBorder; border.width: 1
                            property bool liveUpdate: false
                            RowLayout { anchors.fill: parent; anchors.leftMargin: 4; spacing: 3
                                Rectangle {
                                    width: 10; height: 10; radius: 5
                                    color: parent.parent.liveUpdate ? "#00cc88" : "#555"
                                }
                                Text { text: "Live Update"; color: cMuted; font.pixelSize: 8 }
                            }
                            MouseArea { anchors.fill: parent; onClicked: parent.liveUpdate = !parent.liveUpdate }
                        }

                        Item { Layout.fillWidth: true }

                        // Platform selector
                        Text { text: "Platform"; color: cMuted; font.pixelSize: 9 }
                        Rectangle {
                            width: 70; height: 20; color: "#1e1e1e"; radius: 3
                            border.color: cBorder; border.width: 1
                            RowLayout { anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 4
                                Text { text: "Desktop"; color: cText; font.pixelSize: 9; Layout.fillWidth: true }
                                Text { text: "▼"; color: "#555"; font.pixelSize: 9 }
                            }
                        }
                    }
                }
            }
        }
    }
}
