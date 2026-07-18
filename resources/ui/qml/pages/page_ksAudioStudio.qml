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
    property string selectedTab:   "Timeline"
    property bool   isPlaying:     false

    property var trackList: []
    property int ctxTrackIndex: -1

    function buildTrackList(event) {
        var colors = ["#2a5298", "#5a2a98", "#2a985a", "#985a2a"];
        var isExt = event.indexOf("_ext") > 0;
        if (isExt || event.indexOf("engine") >= 0) {
            var prefix = event.replace(/_int|_ext/, "");
            return [
                { name: event + " (L)", solo: false, mute: false, vol: 0, clipColor: colors[0],
                  regions: [
                      { startPct: 0.00, endPct: 0.14, label: prefix + "_idle" },
                      { startPct: 0.10, endPct: 0.26, label: prefix + "_on_3200" },
                      { startPct: 0.22, endPct: 0.38, label: prefix + "_off_4000" },
                      { startPct: 0.34, endPct: 0.50, label: prefix + "_80_limiter" }
                  ],
                  volumeDb: -48.0, volumeDbLabel: "-48.00" },
                { name: event + " (R)", solo: false, mute: false, vol: 0, clipColor: colors[1],
                  regions: [
                      { startPct: 0.05, endPct: 0.18, label: prefix + "_on_3200" },
                      { startPct: 0.14, endPct: 0.30, label: prefix + "_off_4000" },
                      { startPct: 0.26, endPct: 0.55, label: prefix + "_80_limiter" }
                  ],
                  volumeDb: -0.17, volumeDbLabel: "-0.17" },
                { name: event + " (mix)", solo: false, mute: false, vol: 0, clipColor: colors[2],
                  regions: [
                      { startPct: 0.00, endPct: 0.12, label: prefix + "_idle" },
                      { startPct: 0.08, endPct: 0.22, label: prefix + "_on_3200" },
                      { startPct: 0.18, endPct: 0.34, label: prefix + "_off_4000" },
                      { startPct: 0.30, endPct: 0.50, label: prefix + "_80_limiter" }
                  ],
                  volumeDb: -17.7, volumeDbLabel: "-17.70" }
            ];
        }
        return [
            { name: event, solo: false, mute: false, vol: 0, clipColor: colors[0],
              regions: [
                  { startPct: 0.00, endPct: 0.15, label: event + "_idle" },
                  { startPct: 0.10, endPct: 0.28, label: event + "_on_3200" },
                  { startPct: 0.22, endPct: 0.42, label: event + "_off_4000" }
              ],
              volumeDb: 0.0, volumeDbLabel: "0.0" },
            { name: event + " (alt)", solo: false, mute: false, vol: 0, clipColor: colors[1],
              regions: [
                  { startPct: 0.05, endPct: 0.20, label: event + "_alt_1" },
                  { startPct: 0.16, endPct: 0.38, label: event + "_alt_2" }
              ],
              volumeDb: 0.8, volumeDbLabel: "0.8" }
        ];
    }
    onSelectedEventChanged: trackList = buildTrackList(selectedEvent)
    Component.onCompleted: trackList = buildTrackList(selectedEvent)

    property string leftTab: "Events"

    // Event metadata lookup
    function eventInfo(name) {
        if (typeof eventDefs !== "undefined" && eventDefs) {
            var info = eventDefs.eventInfo(name);
            if (info && info.name) return info;
        }
        var db = {
            "engine_int":     { cat: "Engine", desc: "Interior engine sound", params: ["rpms","throttle"], loops: true, vol: "1.0" },
            "engine_ext":     { cat: "Engine", desc: "Exterior engine sound", params: ["rpms","throttle"], loops: true, vol: "1.0" },
            "turbo":          { cat: "Engine", desc: "Turbo whistle", params: ["boost"], loops: true, vol: "0.6" },
            "turbo_ext":      { cat: "Engine", desc: "Turbo whistle (exterior)", params: ["boost","event cone angle"], loops: true, vol: "0.6" },
            "limiter":        { cat: "Engine", desc: "Rev limiter", params: ["decay"], loops: false, vol: "0.8" },
            "gear_ext":       { cat: "Engine", desc: "Gear change (exterior)", params: ["state","event cone angle"], loops: false, vol: "0.8" },
            "gear_int":       { cat: "Engine", desc: "Gear change (interior)", params: ["state"], loops: false, vol: "0.8" },
            "gear_grind":     { cat: "Engine", desc: "Gear grind", params: ["timeline"], loops: false, vol: "0.8" },
            "starter_ext":    { cat: "Engine", desc: "Starter motor (exterior)", params: ["crank","start","killed"], loops: false, vol: "0.7" },
            "starter_int":    { cat: "Engine", desc: "Starter motor (interior)", params: ["crank","start","killed"], loops: false, vol: "0.7" },
            "ignition_ext":   { cat: "Engine", desc: "Ignition sound (exterior)", params: ["state"], loops: false, vol: "0.8" },
            "ignition_int":   { cat: "Engine", desc: "Ignition sound (interior)", params: ["state"], loops: false, vol: "0.8" },
            "misc_int":       { cat: "Engine", desc: "Misc interior sounds", params: ["rpms","throttle","pit","antistall","gearClonk","gearReverse"], loops: false, vol: "0.6" },
            "door":           { cat: "Body", desc: "Door open/close", params: ["state"], loops: false, vol: "0.7" },
            "horn":           { cat: "Body", desc: "Horn", params: [], loops: false, vol: "0.9" },
            "bodywork":       { cat: "Body", desc: "Bodywork rattles and creaks", params: ["timeline","speed"], loops: false, vol: "0.6" },
            "chassis_ext":    { cat: "Body", desc: "Chassis creaks & collisions (ext)", params: ["speed","kerbL","kerbR","strike"], loops: false, vol: "0.6" },
            "chassis_int":    { cat: "Body", desc: "Chassis creaks & collisions (int)", params: ["speed","kerbL","kerbR","strike"], loops: false, vol: "0.6" },
            "backfire_ext":   { cat: "Backfire", desc: "Backfire (exterior)", params: ["throttle","event cone angle"], loops: false, vol: "0.8" },
            "backfire_int":   { cat: "Backfire", desc: "Backfire (interior)", params: ["throttle","event cone angle"], loops: false, vol: "0.8" },
            "skid_ext":       { cat: "Tires", desc: "Skid sound (exterior)", params: ["timeline","event cone angle"], loops: false, vol: "0.8" },
            "skid_int":       { cat: "Tires", desc: "Skid sound (interior)", params: ["timeline"], loops: false, vol: "0.8" },
            "wheel":          { cat: "Tires", desc: "Wheel sound", params: ["timeline"], loops: false, vol: "0.8" },
            "tractioncontrol_ext": { cat: "Tires", desc: "Traction control (exterior)", params: ["timeline","event cone angle"], loops: false, vol: "0.7" },
            "tractioncontrol_int": { cat: "Tires", desc: "Traction control (interior)", params: ["timeline"], loops: false, vol: "0.7" },
            "transmission":   { cat: "Transmission", desc: "Transmission sound", params: ["timeline","throttle","drivetrain_speed"], loops: true, vol: "0.7" },
            "transmission_ext": { cat: "Transmission", desc: "Transmission sound (exterior)", params: ["timeline","throttle","drivetrain_speed","event cone angle"], loops: true, vol: "0.7" },
            "brakes":         { cat: "Brakes", desc: "Brake squeal and noise", params: ["speed","brake","brake_temp"], loops: true, vol: "0.7" },
            "hybrid_ext":     { cat: "Hybrid", desc: "Hybrid MGU sound (exterior)", params: ["drivetrain_speed","throttle","brake","deploy","harvest"], loops: true, vol: "0.7" },
            "hybrid_int":     { cat: "Hybrid", desc: "Hybrid MGU sound (interior)", params: ["drivetrain_speed","throttle","brake","deploy","harvest","slip"], loops: true, vol: "0.7" },
            "wind":           { cat: "Environment", desc: "Wind noise", params: ["timeline","air_pressure","speed"], loops: true, vol: "0.5" }
        };
        return db[name] || { category: "—", description: "—", parameters: [], loops: false, defaultVolume: 1.0 };
    }

    function eventParams(name) {
        if (typeof eventDefs !== "undefined" && eventDefs)
            return eventDefs.eventParameters(name);
        return eventInfo(name).params || eventInfo(name).parameters || [];
    }

    // Build flat tree model from eventTree structure
    function buildTreeModel(tree, expandedMap) {
        var model = [];
        function walk(obj, depth) {
            for (var key in obj) {
                var val = obj[key];
                var isArr = Array.isArray(val);
                if (!isArr && typeof val === "object") {
                    model.push({ label: key, depth: depth, expandable: true, isExpanded: expandedMap[key] !== false });
                    walk(val, depth + 1);
                } else {
                    model.push({ label: key, depth: depth, expandable: true, isExpanded: expandedMap[key] !== false });
                    for (var i = 0; i < val.length; i++) {
                        model.push({ label: val[i], depth: depth + 1, expandable: false, isExpanded: false });
                    }
                }
            }
        }
        walk(tree, 0);
        return model;
    }
    property var expandedState: ({})

    function buildEventTree() {
        if (typeof eventDefs !== "undefined" && eventDefs) {
            var tree = {};
            var cats = eventDefs.categories;
            for (var ci = 0; ci < cats.length; ci++) {
                var evs = eventDefs.eventsByCategory(cats[ci]);
                if (evs.length > 0) tree[cats[ci]] = evs;
            }
            return tree;
        }
        return {
            "Engine": [
                "engine_int","engine_ext","turbo","turbo_ext","limiter",
                "gear_ext","gear_int","gear_grind","starter_ext","starter_int",
                "ignition_ext","ignition_int","misc_int"
            ],
            "Body": [
                "door","horn","bodywork","chassis_ext","chassis_int"
            ],
            "Backfire": [
                "backfire_ext","backfire_int"
            ],
            "Tires": [
                "skid_ext","skid_int","wheel","tractioncontrol_ext","tractioncontrol_int"
            ],
            "Transmission": [
                "transmission","transmission_ext"
            ],
            "Brakes": ["brakes"],
            "Hybrid": ["hybrid_ext","hybrid_int"],
            "Environment": ["wind"],
            "CSP Rain": [
                "rain_amb","rain_amb_thunder","rain_car_ext","rain_car_int",
                "rain_grass","rain_gravel","rain_skid_ext","rain_skid_int"
            ],
            "CSP Vehicle": [
                "turn_signal_ext__off","turn_signal_int__off","turn_signal_int",
                "wiper_car_ext","wiper_car_ext_vintage","wiper_car_int","wiper_car_int_vintage",
                "handbrake_int"
            ],
            "CSP Wind": ["external_wind"],
            "CSP Surfaces": [
                "csp_surfaces_skid","csp_surfaces_force","csp_surfaces_rocks","csp_surfaces_ice"
            ]
        };
    }

    property var eventTree: buildEventTree()

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
                                color: modelData === leftTab ? cPanel : "transparent"
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width; height: 2
                                    color: modelData === leftTab ? cAccent : "transparent"
                                }
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: modelData === leftTab ? cAccent : cMuted
                                    font.pixelSize: 11
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: leftTab = modelData
                                }
                            }
                        }
                    }
                }

                // Search
                Rectangle {
                    height: 24; Layout.fillWidth: true
                    color: "#1e1e1e"; border.color: cBorder; border.width: 1
                    visible: leftTab === "Events"
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
                    visible: leftTab === "Events"
                    model: {
                        var raw = buildTreeModel(eventTree, expandedState);
                        var lm = [];
                        for (var i = 0; i < raw.length; i++) lm.push(raw[i]);
                        return lm;
                    }
                    delegate: Rectangle {
                        id: delegateRoot
                        required property string label
                        required property int depth
                        required property bool expandable
                        required property bool isExpanded
                        required property int index

                        width: ListView.view.width
                        height: 20
                        visible: {
                            if (depth === 0) return true;
                            var idx = index - 1;
                            var d = depth;
                            while (idx >= 0 && d > 0) {
                                var item = ListView.view.model[idx];
                                if (!item) break;
                                if (item.depth < d && item.depth < depth) {
                                    d = item.depth;
                                }
                                if (item.depth === d && item.expandable && !item.isExpanded) {
                                    return false;
                                }
                                idx--;
                            }
                            return true;
                        }
                        color: !expandable && label === selectedEvent ? cSelected : "transparent"

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (expandable) {
                                    expandedState[label] = !isExpanded;
                                    ListView.view.model = ListView.view.model.slice(0);
                                } else {
                                    selectedEvent = label;
                                }
                            }
                        }

                        RowLayout {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 4 + depth * 12
                            spacing: 3

                            Text {
                                text: expandable ? (isExpanded ? "▼" : "▶") : "♪"
                                color: expandable ? cMuted : (!expandable && label === selectedEvent ? "#121212" : cMuted)
                                font.pixelSize: 8
                            }
                            Text {
                                text: label
                                color: !expandable && label === selectedEvent ? "#121212" : cText
                                font.pixelSize: 10
                                font.bold: !expandable && label === selectedEvent
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                }

                // Banks tab content
                ColumnLayout {
                    visible: leftTab === "Banks"
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.margins: 8; spacing: 6

                    Text { text: "Sound Banks"; color: cText; font.pixelSize: 11; font.bold: true }
                    Repeater {
                        model: ["csp_base", "csp_extras", "csp_surface_gravel", "csp_surface_ice"]
                        Rectangle {
                            Layout.fillWidth: true; height: 22; color: "#1e1e1e"; radius: 3
                            border.color: cBorder; border.width: 1
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6
                                Text { text: modelData; color: cText; font.pixelSize: 9; Layout.fillWidth: true }
                                Text { text: "▸"; color: cMuted; font.pixelSize: 10 }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // Assets tab content
                ColumnLayout {
                    visible: leftTab === "Assets"
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.margins: 8; spacing: 6

                    Text { text: "Audio Assets"; color: cText; font.pixelSize: 11; font.bold: true }
                    Text { text: "Drop audio files here to import"; color: cMuted; font.pixelSize: 9; wrapMode: Text.WordWrap }
                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: "#1a1a1a"; radius: 4; border.color: cBorder; border.width: 1; border.style: Qt.Dashed
                        Text {
                            anchors.centerIn: parent
                            text: "Drop .wav / .ogg / .bank\nor click to browse"
                            color: "#444"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter
                        }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                    }
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

                    // Tab: selected event (active)
                    Rectangle {
                        width: Math.max(80, selectedEvent.length * 7 + 20); height: 28
                        color: cPanel
                        border.color: cBorder; border.width: 1
                        Rectangle { anchors.top: parent.top; width: parent.width; height: 2; color: cAccent }
                        Text {
                            anchors.centerIn: parent
                            text: selectedEvent; color: cAccent; font.pixelSize: 11
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

                    // Tabs: Timeline | event parameters
                    Repeater {
                        model: {
                            var params = ["Timeline"];
                            var p = root.eventParams(selectedEvent);
                            for (var i = 0; i < p.length; i++) params.push(p[i]);
                            return params;
                        }
                        Rectangle {
                            height: 26; width: 64
                            color: modelData === selectedTab ? "#444" : "transparent"
                            border.color: modelData === selectedTab ? cBorder : "transparent"
                            border.width: 1; radius: 2
                            Text { anchors.centerIn: parent; text: modelData; color: modelData === selectedTab ? cText : cMuted; font.pixelSize: 11 }
                            MouseArea { anchors.fill: parent; onClicked: selectedTab = modelData }
                        }
                    }

                    // Dynamic parameter readouts (first 3 params)
                    Repeater {
                        model: root.eventParams(selectedEvent).slice(0, 3)
                        Rectangle {
                            width: 60; height: 26; color: "#111111"; radius: 3; border.color: cBorder; border.width: 1
                            ColumnLayout {
                                anchors.fill: parent; anchors.margins: 3; spacing: 0
                                Text { text: modelData; color: cMuted; font.pixelSize: 7 }
                                Text { text: "0.00"; color: cText; font.pixelSize: 11; font.family: "Courier New" }
                            }
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
                                    height: 130
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
                                        Text { text: (modelData.volumeDb === -48.0) ? "-48.00" : (modelData.volumeDb === 0.0 ? "0.0" : (modelData.volumeDbLabel || "0.0")) + " dB"; color: cMuted; font.pixelSize: 8 }
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
                                        height: 200
                                        color: "#1a2a3a"; border.color: cBorder; border.width: 1
                                        clip: true

                                        Canvas {
                                            anchors.fill: parent
                                            property var trackData: modelData
                                            onTrackDataChanged: requestPaint()
                                            onWidthChanged: requestPaint()
                                            onHeightChanged: requestPaint()

                                            // Deterministic pseudo-random based on seed
                                            function seededRand(seed) {
                                                var x = Math.sin(seed * 127.1 + 311.7) * 43758.5453;
                                                return x - Math.floor(x);
                                            }

                                            onPaint: {
                                                var ctx = getContext("2d");
                                                ctx.clearRect(0, 0, width, height);
                                                var regions = trackData.regions || [];
                                                if (regions.length === 0) return;

                                                var mid = height / 2;

                                                for (var r = 0; r < regions.length; r++) {
                                                    var reg = regions[r];
                                                    var x0 = reg.startPct * width;
                                                    var x1 = reg.endPct * width;
                                                    var regW = x1 - x0;
                                                    var seed = r * 1000 + (trackData.name || "").length * 7;

                                                    // Clip background
                                                    ctx.fillStyle = trackData.clipColor || "#2a5298";
                                                    ctx.globalAlpha = 0.35;
                                                    ctx.fillRect(x0, 0, regW, height);
                                                    ctx.globalAlpha = 1.0;

                                                    // Draw peak waveform envelope (filled)
                                                    ctx.fillStyle = trackData.clipColor || "#4488cc";
                                                    ctx.globalAlpha = 0.7;
                                                    ctx.beginPath();
                                                    ctx.moveTo(x0, mid);
                                                    var step = 2;
                                                    for (var x = x0; x < x1; x += step) {
                                                        var t = (x - x0) / regW;
                                                        // Envelope shape: attack-sustain-release
                                                        var env = 1.0;
                                                        if (t < 0.05) env = t / 0.05;
                                                        else if (t > 0.92) env = (1.0 - t) / 0.08;
                                                        // High-frequency detail
                                                        var detail = seededRand(seed + x * 0.7) * 0.6 + 0.2;
                                                        var amp = mid * 0.75 * env * detail;
                                                        ctx.lineTo(x, mid - amp);
                                                    }
                                                    for (var x2 = x1; x2 > x0; x2 -= step) {
                                                        var t2 = (x2 - x0) / regW;
                                                        var env2 = 1.0;
                                                        if (t2 < 0.05) env2 = t2 / 0.05;
                                                        else if (t2 > 0.92) env2 = (1.0 - t2) / 0.08;
                                                        var detail2 = seededRand(seed + x2 * 0.7 + 50) * 0.6 + 0.2;
                                                        var amp2 = mid * 0.75 * env2 * detail2;
                                                        ctx.lineTo(x2, mid + amp2);
                                                    }
                                                    ctx.closePath();
                                                    ctx.fill();

                                                    // Draw peak lines for sharper detail
                                                    ctx.strokeStyle = trackData.clipColor || "#66aaee";
                                                    ctx.globalAlpha = 0.9;
                                                    ctx.lineWidth = 0.8;
                                                    ctx.beginPath();
                                                    for (var x3 = x0; x3 < x1; x3 += 1) {
                                                        var t3 = (x3 - x0) / regW;
                                                        var env3 = 1.0;
                                                        if (t3 < 0.05) env3 = t3 / 0.05;
                                                        else if (t3 > 0.92) env3 = (1.0 - t3) / 0.08;
                                                        var peak = seededRand(seed + x3 * 1.3) * 0.5 + 0.3;
                                                        var amp3 = mid * 0.8 * env3 * peak;
                                                        ctx.moveTo(x3, mid - amp3);
                                                        ctx.lineTo(x3, mid + amp3);
                                                    }
                                                    ctx.stroke();
                                                    ctx.globalAlpha = 1.0;

                                                    // Region label
                                                    ctx.fillStyle = "#aaccff";
                                                    ctx.font = "9px sans-serif";
                                                    ctx.globalAlpha = 0.85;
                                                    ctx.fillText(reg.label, x0 + 6, 14);
                                                    ctx.globalAlpha = 1.0;
                                                }

                                                // Crossfade markers between adjacent regions
                                                ctx.strokeStyle = "#ffffff";
                                                ctx.globalAlpha = 0.4;
                                                ctx.lineWidth = 1;
                                                for (var ci = 0; ci < regions.length - 1; ci++) {
                                                    var rA = regions[ci];
                                                    var rB = regions[ci + 1];
                                                    // Crossfade overlap region
                                                    var cfLeft = rA.endPct * width;
                                                    var cfRight = rB.startPct * width;
                                                    // If regions overlap or are adjacent
                                                    var cfCenter = (cfLeft + cfRight) / 2;
                                                    var cfHalf = Math.abs(cfRight - cfLeft) / 2;
                                                    if (cfHalf < 2) {
                                                        // Regions touch — draw at boundary
                                                        ctx.beginPath();
                                                        ctx.moveTo(cfLeft, 0);
                                                        ctx.lineTo(cfLeft, height);
                                                        ctx.stroke();
                                                    } else {
                                                        // Fade-out line (top-left to bottom-right)
                                                        ctx.beginPath();
                                                        ctx.moveTo(cfCenter - cfHalf, 10);
                                                        ctx.lineTo(cfCenter + cfHalf, height - 10);
                                                        ctx.stroke();
                                                        // Fade-in line (bottom-left to top-right)
                                                        ctx.beginPath();
                                                        ctx.moveTo(cfCenter - cfHalf, height - 10);
                                                        ctx.lineTo(cfCenter + cfHalf, 10);
                                                        ctx.stroke();
                                                    }
                                                }
                                                ctx.globalAlpha = 1.0;
                                            }
                                        }

                                        // Right-click context menu
                                        MouseArea {
                                            anchors.fill: parent
                                            acceptedButtons: Qt.RightButton
                                            onClicked: {
                                                root.ctxTrackIndex = index
                                                timelineCtx.trackName = modelData.name
                                                timelineCtx.popup()
                                            }
                                        }
                                    }

                                    // Volume curve lane
                                    Rectangle {
                                        width: parent.width
                                        height: 60
                                        color: "#161616"; border.color: cBorder; border.width: 1
                                        clip: true

                                        Text { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 3
                                            text: "Volume   10 dB"; color: cMuted; font.pixelSize: 8 }
                                        Text { anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 3
                                            text: "-∞ dB"; color: cMuted; font.pixelSize: 8 }

                                        Canvas {
                                            anchors.fill: parent
                                            property var trackData: modelData
                                            onTrackDataChanged: requestPaint()
                                            onWidthChanged: requestPaint()

                                            onPaint: {
                                                var ctx = getContext("2d");
                                                ctx.clearRect(0, 0, width, height);

                                                var regions = trackData.regions || [];
                                                var dB = trackData.volumeDb || 0;
                                                // Map dB to Y position (0 dB = top-ish, -80 dB = bottom)
                                                var dbNorm = Math.max(0, Math.min(1, (dB + 80) / 80));
                                                var curveY = height * (1 - dbNorm * 0.85);

                                                // Volume line from left edge
                                                ctx.strokeStyle = root.cCurve;
                                                ctx.lineWidth = 1.5;
                                                ctx.beginPath();

                                                if (regions.length > 0) {
                                                    var startX = regions[0].startPct * width;
                                                    ctx.moveTo(0, height * 0.95);
                                                    ctx.bezierCurveTo(
                                                        startX * 0.3, height * 0.95,
                                                        startX * 0.7, curveY,
                                                        startX, curveY
                                                    );
                                                    ctx.lineTo(width, curveY);
                                                } else {
                                                    ctx.moveTo(0, height * 0.5);
                                                    ctx.lineTo(width, curveY);
                                                }
                                                ctx.stroke();

                                                // dB value labels at region boundaries
                                                ctx.fillStyle = root.cCurve;
                                                ctx.font = "8px Courier New";
                                                ctx.fillText(dB.toFixed(2), 8, curveY - 4);

                                                if (regions.length > 1) {
                                                    var midX = regions[0].endPct * width;
                                                    ctx.fillText(trackData.volumeDbLabel || dB.toFixed(2), midX + 4, curveY - 4);
                                                }
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

            // ── Status bar ───────────────────────────────────────────
            Rectangle {
                height: 24; Layout.fillWidth: true
                color: "#1a1a1a"; border.color: cBorder; border.width: 1

                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 8; spacing: 8

                    Text { text: "Current Module: sound"; color: cMuted; font.pixelSize: 10 }
                    Item { Layout.fillWidth: true }
                    Text { text: "Ready"; color: cMuted; font.pixelSize: 10 }
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
                    Layout.topMargin: 6; spacing: 6

                    // Tags
                    ColumnLayout { spacing: 2; Layout.fillWidth: true
                        Text { text: "Tags"; color: cMuted; font.pixelSize: 9; font.bold: true }
                        Rectangle {
                            Layout.fillWidth: true; height: 22; color: "#1e1e1e"
                            radius: 3; border.color: cBorder; border.width: 1
                            TextInput {
                                anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6
                                color: cText; font.pixelSize: 9; verticalAlignment: TextInput.AlignVCenter
                                clip: true
                            }
                        }
                    }

                    // User Properties
                    ColumnLayout { spacing: 2; Layout.fillWidth: true
                        Text { text: "User Properties"; color: cMuted; font.pixelSize: 9; font.bold: true }
                        Rectangle {
                            Layout.fillWidth: true; height: 22; color: "#1e1e1e"
                            radius: 3; border.color: cBorder; border.width: 1
                            TextInput {
                                anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6
                                color: cText; font.pixelSize: 9; verticalAlignment: TextInput.AlignVCenter
                                clip: true
                            }
                        }
                    }

                    // Notes
                    ColumnLayout { spacing: 2; Layout.fillWidth: true
                        Text { text: "Notes"; color: cMuted; font.pixelSize: 9; font.bold: true }
                        Rectangle {
                            Layout.fillWidth: true; height: 22; color: "#1e1e1e"
                            radius: 3; border.color: cBorder; border.width: 1
                            TextInput {
                                anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6
                                color: cText; font.pixelSize: 9; verticalAlignment: TextInput.AlignVCenter
                                clip: true
                            }
                        }
                    }

                    // User Notes
                    ColumnLayout { spacing: 2; Layout.fillWidth: true
                        Text { text: "User Notes"; color: cMuted; font.pixelSize: 9; font.bold: true }
                        Rectangle {
                            Layout.fillWidth: true; height: 22; color: "#1e1e1e"
                            radius: 3; border.color: cBorder; border.width: 1
                            TextInput {
                                anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6
                                color: cText; font.pixelSize: 9; verticalAlignment: TextInput.AlignVCenter
                                clip: true
                            }
                        }
                    }

                    // Event EndProperties
                    ColumnLayout { spacing: 2; Layout.fillWidth: true
                        Text { text: "Event EndProperties"; color: cMuted; font.pixelSize: 9; font.bold: true }
                        Rectangle {
                            Layout.fillWidth: true; height: 22; color: "#1e1e1e"
                            radius: 3; border.color: cBorder; border.width: 1
                            TextInput {
                                anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 6
                                color: cText; font.pixelSize: 9; verticalAlignment: TextInput.AlignVCenter
                                clip: true
                            }
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
                            Rectangle {
                                width: 38; height: 38; radius: 19
                                color: "#0f0f0f"; border.color: "#555"; border.width: 2
                                Canvas {
                                    anchors.fill: parent
                                    property real angle: -0.1
                                    onPaint: {
                                        var ctx = getContext("2d");
                                        ctx.clearRect(0,0,width,height);
                                        var cx = width/2, cy = height/2, r = width/2 - 4;
                                        ctx.beginPath();
                                        ctx.arc(cx, cy, r, Math.PI * 0.75, Math.PI * 2.25);
                                        ctx.strokeStyle = "#333"; ctx.lineWidth = 3; ctx.stroke();
                                        ctx.beginPath();
                                        ctx.arc(cx, cy, r, Math.PI * 1.5, Math.PI * 1.5 + angle);
                                        ctx.strokeStyle = root.cAccent; ctx.lineWidth = 3; ctx.stroke();
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
                                Layout.fillWidth: true; height: 22; color: "#1e1e1e"; radius: 3
                                border.color: cBorder; border.width: 1
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 4
                                    Text { text: "on"; color: cAccent; font.pixelSize: 10; Layout.fillWidth: true }
                                    Text { text: "▼"; color: "#555"; font.pixelSize: 9 }
                                }
                            }
                        }
                    }

                    // Row 2: Recording toggle
                    RowLayout { Layout.fillWidth: true; spacing: 8
                        ColumnLayout { spacing: 2; Layout.fillWidth: true
                            Text { text: "Recording"; color: cMuted; font.pixelSize: 9 }
                            Rectangle {
                                Layout.fillWidth: true; height: 22; color: "#1e1e1e"; radius: 3
                                border.color: cBorder; border.width: 1
                                property bool on: false
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 4
                                    Rectangle { width: 8; height: 8; radius: 4
                                        color: parent.parent.parent.on ? "#ff4444" : "#555" }
                                    Text { text: "OFF"; color: cMuted; font.pixelSize: 9; Layout.fillWidth: true }
                                }
                                MouseArea { anchors.fill: parent; onClicked: parent.on = !parent.on }
                            }
                        }
                    }

                    // Row 3: Virtualize toggle + Cooldown
                    RowLayout { Layout.fillWidth: true; spacing: 8
                        ColumnLayout { spacing: 2
                            Text { text: "Virtualize"; color: cMuted; font.pixelSize: 9 }
                            Rectangle {
                                width: 60; height: 22; color: "#1e1e1e"; radius: 3
                                border.color: cBorder; border.width: 1
                                property bool on: true
                                RowLayout { anchors.fill: parent; anchors.leftMargin: 6; anchors.rightMargin: 4
                                    Text { text: "On"; color: cAccent; font.pixelSize: 9; Layout.fillWidth: true }
                                    Text { text: "▼"; color: "#555"; font.pixelSize: 9 }
                                }
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

                    // Row 4: Priority
                    RowLayout { Layout.fillWidth: true; spacing: 8
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

    // ── Timeline context menu ─────────────────────────────────────
    Menu {
        id: timelineCtx
        property string trackName: ""

        MenuItem {
            text: "Open Audio in Audio Bin"
            onTriggered: { console.log("Open in Audio Bin:", timelineCtx.trackName) }
        }
        MenuItem {
            text: "Open in Explorer"
            onTriggered: { console.log("Open in Explorer:", timelineCtx.trackName) }
        }
        MenuItem {
            text: "Open in AudioEditor"
            onTriggered: { console.log("Open in AudioEditor:", timelineCtx.trackName) }
        }

        MenuSeparator {}

        MenuItem {
            text: "Cut"; onTriggered: { console.log("Cut:", timelineCtx.trackName) }
        }
        MenuItem {
            text: "Copy"; onTriggered: { console.log("Copy:", timelineCtx.trackName) }
        }
        MenuItem {
            text: "Paste"; onTriggered: { console.log("Paste:", timelineCtx.trackName) }
        }
        MenuItem {
            text: "Split"; onTriggered: { console.log("Split:", timelineCtx.trackName) }
        }
        MenuItem {
            text: "Delete"
            onTriggered: {
                console.log("Delete:", timelineCtx.trackName)
                // Remove track from trackList
                if (root.ctxTrackIndex >= 0 && root.ctxTrackIndex < root.trackList.length) {
                    var arr = root.trackList.slice(0);
                    arr.splice(root.ctxTrackIndex, 1);
                    root.trackList = arr;
                }
            }
        }

        MenuSeparator {}

        MenuItem {
            text: "Move to.."; onTriggered: { console.log("Move to..:", timelineCtx.trackName) }
        }
        MenuItem {
            text: "Move to Cursor"; onTriggered: { console.log("Move to Cursor:", timelineCtx.trackName) }
        }

        MenuSeparator {}

        MenuItem {
            text: "Bring to Front"; onTriggered: { console.log("Bring to Front:", timelineCtx.trackName) }
        }
        MenuItem {
            text: "Send to Back"; onTriggered: { console.log("Send to Back:", timelineCtx.trackName) }
        }

        MenuSeparator {}

        MenuItem {
            text: "Change Color"
            onTriggered: {
                var colors = ["#2a5298", "#5a2a98", "#2a985a", "#985a2a", "#982a2a", "#2a9898"];
                var idx = root.ctxTrackIndex;
                if (idx >= 0 && idx < root.trackList.length) {
                    var arr = root.trackList.slice(0);
                    var t = arr[idx];
                    var ci = colors.indexOf(t.clipColor);
                    t.clipColor = colors[(ci + 1) % colors.length];
                    root.trackList = arr;
                }
            }
        }
    }
}
