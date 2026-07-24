import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Shapes 1.15
import ksEditor.Audio 1.0
import "../widgets"
import ksEditor.AudioEffects 1.0

Rectangle {
    id: audioEditor
    width: 1280
    height: 720
    color: "#121212"

    readonly property color cAccent: "#E10600"
    readonly property color cPanel: "#1e1e1e"
    readonly property color cBg: "#0e0e0e"
    readonly property color cBorder: "#333333"
    readonly property color cText: "#ffffff"
    readonly property color cMuted: "#666666"

    property string activePanel: "waveform"
    property real masterVolume: 80
    property string currentFileName: AudioBridge ? AudioBridge.getFileName() : "engine.wav"
    property real leftPeakDb: AudioBridge ? AudioBridge.leftPeak : 0
    property real rightPeakDb: AudioBridge ? AudioBridge.rightPeak : 0
    property string statusMessage: "Ready"

    // Event model (mirrors ACEventDefs.h)
    property string selectedEvent: ""

    function buildEventCategories() {
        if (typeof eventDefs !== "undefined" && eventDefs) {
            var result = [];
            var cats = eventDefs.categories;
            for (var ci = 0; ci < cats.length; ci++) {
                var evs = eventDefs.eventsByCategory(cats[ci]);
                result.push({ name: cats[ci], icon: "\u266A", collapsed: ci >= 8, events: evs });
            }
            return result;
        }
        return [
            { name: "Engine", icon: "\u2699", collapsed: false,
              events: ["engine_int","engine_ext","turbo","turbo_ext","limiter","gear_ext","gear_int","gear_grind","starter_ext","starter_int","ignition_ext","ignition_int","misc_int"] },
            { name: "Body", icon: "\uF0C2", collapsed: false,
              events: ["door","horn","bodywork","chassis_ext","chassis_int"] },
            { name: "Backfire", icon: "\uF0E7", collapsed: false,
              events: ["backfire_ext","backfire_int"] },
            { name: "Tires", icon: "\uF1B9", collapsed: false,
              events: ["skid_ext","skid_int","wheel","tractioncontrol_ext","tractioncontrol_int"] },
            { name: "Transmission", icon: "\uF085", collapsed: false,
              events: ["transmission","transmission_ext"] },
            { name: "Brakes", icon: "\uF0A7", collapsed: false,
              events: ["brakes"] },
            { name: "Hybrid", icon: "\uF0E7", collapsed: false,
              events: ["hybrid_ext","hybrid_int"] },
            { name: "Environment", icon: "\uF74E", collapsed: false,
              events: ["wind"] },
            { name: "CSP Rain", icon: "\uF743", collapsed: true,
              events: ["rain_amb","rain_amb_thunder","rain_car_ext","rain_car_int","rain_grass","rain_gravel","rain_skid_ext","rain_skid_int"] },
            { name: "CSP Vehicle", icon: "\uF1B9", collapsed: true,
              events: ["turn_signal_ext__off","turn_signal_int__off","turn_signal_int","wiper_car_ext","wiper_car_ext_vintage","wiper_car_int","wiper_car_int_vintage","handbrake_int"] },
            { name: "CSP Wind", icon: "\uF74E", collapsed: true,
              events: ["external_wind"] },
            { name: "CSP Surfaces", icon: "\uF7A0", collapsed: true,
              events: ["csp_surfaces_skid","csp_surfaces_force","csp_surfaces_rocks","csp_surfaces_ice"] }
        ];
    }

    property var eventCategories: buildEventCategories()

    function eventInfo(ev) {
        if (typeof eventDefs !== "undefined" && eventDefs) {
            var info = eventDefs.eventInfo(ev);
            if (info && info.name) return info;
        }
        var map = {
            "engine_int":     { cat: "Engine",       desc: "Engine interior",       params: ["RPM","Load","Gear"] },
            "engine_ext":     { cat: "Engine",       desc: "Engine exterior",       params: ["RPM","Load","Gear"] },
            "turbo":          { cat: "Engine",       desc: "Turbo interior",        params: ["RPM","Boost"] },
            "turbo_ext":      { cat: "Engine",       desc: "Turbo exterior",        params: ["RPM","Boost"] },
            "limiter":        { cat: "Engine",       desc: "Rev limiter",           params: ["RPM"] },
            "gear_ext":       { cat: "Engine",       desc: "Gear change ext",       params: ["RPM","Gear"] },
            "gear_int":       { cat: "Engine",       desc: "Gear change int",       params: ["RPM","Gear"] },
            "gear_grind":     { cat: "Engine",       desc: "Gear grinding",         params: ["RPM","Gear","Speed"] },
            "starter_ext":    { cat: "Engine",       desc: "Starter exterior",      params: ["RPM"] },
            "starter_int":    { cat: "Engine",       desc: "Starter interior",      params: ["RPM"] },
            "ignition_ext":   { cat: "Engine",       desc: "Ignition ext",          params: [] },
            "ignition_int":   { cat: "Engine",       desc: "Ignition int",          params: [] },
            "misc_int":       { cat: "Engine",       desc: "Misc interior",         params: [] },
            "door":           { cat: "Body",         desc: "Door close/open",       params: [] },
            "horn":           { cat: "Body",         desc: "Horn",                  params: [] },
            "bodywork":       { cat: "Body",         desc: "Bodywork rattles",      params: ["Speed"] },
            "chassis_ext":    { cat: "Body",         desc: "Chassis exterior",      params: ["Speed"] },
            "chassis_int":    { cat: "Body",         desc: "Chassis interior",      params: ["Speed"] },
            "backfire_ext":   { cat: "Backfire",     desc: "Backfire exterior",     params: ["RPM","Load"] },
            "backfire_int":   { cat: "Backfire",     desc: "Backfire interior",     params: ["RPM","Load"] },
            "skid_ext":       { cat: "Tires",        desc: "Skid exterior",         params: ["Speed","Slip"] },
            "skid_int":       { cat: "Tires",        desc: "Skid interior",         params: ["Speed","Slip"] },
            "wheel":          { cat: "Tires",        desc: "Wheel rolling noise",   params: ["Speed"] },
            "tractioncontrol_ext": { cat: "Tires",   desc: "TC exterior",           params: ["RPM","Slip"] },
            "tractioncontrol_int": { cat: "Tires",   desc: "TC interior",           params: ["RPM","Slip"] },
            "transmission":   { cat: "Transmission", desc: "Transmission int",      params: ["RPM","Gear"] },
            "transmission_ext": { cat: "Transmission", desc: "Transmission ext",    params: ["RPM","Gear"] },
            "brakes":         { cat: "Brakes",       desc: "Brake squeal",          params: ["Speed","Brake"] },
            "hybrid_ext":     { cat: "Hybrid",       desc: "Hybrid motor ext",      params: ["RPM","Load"] },
            "hybrid_int":     { cat: "Hybrid",       desc: "Hybrid motor int",      params: ["RPM","Load"] },
            "wind":           { cat: "Environment",  desc: "Wind noise",            params: ["Speed"] }
        };
        if (ev.indexOf("turn_signal") >= 0) return { cat: "CSP Vehicle", desc: "Turn signal", params: [] };
        if (ev.indexOf("wiper") >= 0) return { cat: "CSP Vehicle", desc: "Wiper", params: ["Speed"] };
        if (ev.indexOf("handbrake") >= 0) return { cat: "CSP Vehicle", desc: "Handbrake", params: [] };
        if (ev.indexOf("rain_amb") >= 0) return { cat: "CSP Rain", desc: "Rain ambient", params: ["Intensity"] };
        if (ev.indexOf("rain_car") >= 0) return { cat: "CSP Rain", desc: "Rain on car", params: ["Intensity"] };
        if (ev.indexOf("rain_grass") >= 0) return { cat: "CSP Rain", desc: "Rain on grass", params: ["Intensity"] };
        if (ev.indexOf("rain_gravel") >= 0) return { cat: "CSP Rain", desc: "Rain on gravel", params: ["Intensity"] };
        if (ev.indexOf("rain_skid") >= 0) return { cat: "CSP Rain", desc: "Rain skid", params: ["Intensity","Slip"] };
        if (ev.indexOf("external_wind") >= 0) return { cat: "CSP Wind", desc: "External wind", params: ["Speed"] };
        if (ev.indexOf("csp_surfaces") >= 0) return { cat: "CSP Surfaces", desc: "Surface contact", params: ["Surface","Speed"] };
        return map[ev] || { category: "Unknown", description: ev, parameters: [] };
    }

    function loadEventFile(ev) {
        if (AudioBridge) {
            var path = "events/" + ev + ".wav";
            AudioBridge.loadAudio(path);
            statusMessage = "Loaded: " + ev;
        }
    }

    // Panel definitions
    property var panels: [
        {key: "waveform", label: "Waveform"},
        {key: "events", label: "Events"},
        {key: "effects", label: "Effects"},
        {key: "eq", label: "Equalizer"},
        {key: "dynamics", label: "Dynamics"},
        {key: "convolution", label: "Convolution"},
        {key: "stereo", label: "Stereo"},
        {key: "tape", label: "Tape"},
        {key: "transient", label: "Transient"},
        {key: "pitch", label: "Pitch"},
        {key: "noisegate", label: "Noise Gate"},
        {key: "multiband", label: "Multi-Band"},
        {key: "delayfx", label: "Delay"},
        {key: "limiter", label: "Limiter"},
        {key: "reverb", label: "Reverb"},
        {key: "modulation", label: "Modulation"},
        {key: "saturation", label: "Saturation"},
        {key: "deesser", label: "De-Esser"},
        {key: "ducker", label: "Ducker"},
        {key: "bitcrusher", label: "Bit Crusher"},
        {key: "correlator", label: "Correlator"},
        {key: "wahwah", label: "Auto-Wah"},
        {key: "ringmod", label: "Ring Mod"},
        {key: "formant", label: "Formant"},
        {key: "spectralgate", label: "Spectral Gate"},
        {key: "analyzer", label: "Analyzer"},
        {key: "timeline", label: "Timeline"},
        {key: "automation", label: "Automation"},
        {key: "mastering", label: "Mastering"}
    ]

    FileDialog {
        id: audioImportDialog
        title: "Import Audio File"
        nameFilters: ["Audio files (*.wav *.mp3 *.ogg *.flac *.aiff)", "All files (*)"]
        onAccepted: {
            if (AudioBridge)
                AudioBridge.loadAudio(selectedFile.toString().replace("file:///", ""))
        }
    }

    FileDialog {
        id: audioExportDialog
        title: "Export Audio File"
        nameFilters: ["WAV files (*.wav)", "OGG files (*.ogg)", "MP3 files (*.mp3)", "All files (*)"]
        onAccepted: {
            if (AudioBridge)
                AudioBridge.exportAudio(selectedFile.toString().replace("file:///", ""))
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Toolbar ---
        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10

                AppButton { text: "Import"; flat: true; height: 32; bgcolor: "transparent"; color: "#ffffff"; onClicked: audioImportDialog.open() }
                AppButton { text: "Export"; flat: true; height: 32; bgcolor: "transparent"; color: "#ffffff"; onClicked: audioExportDialog.open() }

                Rectangle { width: 1; height: 20; color: "#444444" }

                AppButton {
                    text: "Play"; flat: true; height: 32
                    bgcolor: AudioBridge && AudioBridge.isPlaying ? cAccent : "transparent"
                    color: "#ffffff"
                    onClicked: { if (AudioBridge) { if (AudioBridge.isPlaying) AudioBridge.pause(); else AudioBridge.play() } }
                }
                AppButton { text: "Stop"; flat: true; height: 32; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (AudioBridge) AudioBridge.stop() } }
                AppButton {
                    text: "Loop"; flat: true; height: 32
                    bgcolor: AudioBridge && AudioBridge.isLoopEnabled ? cAccent : "transparent"
                    color: "#ffffff"
                    onClicked: { if (AudioBridge) AudioBridge.isLoopEnabled = !AudioBridge.isLoopEnabled }
                }

                Item { Layout.fillWidth: true }

                Text { text: currentFileName; color: "#aaaaaa"; font.pixelSize: 12 }

                Rectangle { width: 1; height: 20; color: "#444444" }

                Text { text: selectedEvent || "No event"; color: selectedEvent ? cAccent : cMuted; font.pixelSize: 11; font.bold: true; rightPadding: 10 }
            }
        }

        // --- Main Content ---
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // --- Left Panel: Panel & Event Browser ---
            Rectangle {
                width: 180
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 10
                    clip: true
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width - 20
                        spacing: 4

                        Text { text: "PANELS"; color: "#666"; font.pixelSize: 10; font.bold: true }

                        Repeater {
                            model: panels
                            delegate: AppButton {
                                height: 26
                                text: modelData.label
                                bgcolor: activePanel === modelData.key ? cAccent : "#3e3e42"
                                color: activePanel === modelData.key ? "#121212" : "#ffffff"
                                onClicked: activePanel = modelData.key
                            }
                        }

                        Rectangle { height: 10; color: "transparent" }

                        Text { text: "EVENT CATEGORIES"; color: "#666"; font.pixelSize: 10; font.bold: true }

                        Repeater {
                            model: eventCategories
                            delegate: ColumnLayout {
                                width: parent.width
                                spacing: 1

                                AppButton {
                                    height: 22; text: modelData.icon + " " + modelData.name
                                    font.pixelSize: 9
                                    bgcolor: "#2e2e32"
                                    color: "#aaaaaa"
                                    onClicked: {
                                        var idx = index
                                        var arr = eventCategories
                                        arr[idx].collapsed = !arr[idx].collapsed
                                        eventCategories = arr
                                    }
                                }

                                Repeater {
                                    model: modelData.collapsed ? [] : modelData.events
                                    delegate: AppButton {
                                        height: 20; text: "  " + modelData
                                        font.pixelSize: 8
                                        leftPadding: 16
                                        bgcolor: selectedEvent === modelData ? cAccent : "transparent"
                                        color: selectedEvent === modelData ? "#121212" : "#888888"
                                        onClicked: {
                                            selectedEvent = modelData
                                            loadEventFile(modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // --- Center: Editor View ---
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#2a2a2a"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    // === WAVEFORM ===
                    Loader {
                        active: activePanel === "waveform"
                        Layout.fillWidth: true; Layout.fillHeight: true
                        sourceComponent: ColumnLayout {
                            spacing: 10
                            Text { text: "WAVEFORM EDITOR"; color: "white"; font.bold: true; font.pixelSize: 14 }
                            Text { text: selectedEvent ? ("Editing: " + selectedEvent) : "No event selected — choose from Events panel"; color: cMuted; font.pixelSize: 10 }

                            Rectangle {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                color: "#0e0e0e"; border.color: "#3e3e42"; border.width: 1

                                Canvas {
                                    id: waveformCanvas
                                    anchors.fill: parent
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        if (!AudioBridge || AudioBridge.getSampleCount() === 0) return
                                        var data = AudioBridge.getWaveformData(Math.floor(width))
                                        if (data.length === 0) return
                                        ctx.strokeStyle = cAccent
                                        ctx.lineWidth = 1
                                        ctx.beginPath()
                                        var midY = height / 2
                                        for (var i = 0; i < data.length; ++i) {
                                            var x = i * width / data.length
                                            var sample = parseFloat(data[i]) * midY * 0.8
                                            ctx.moveTo(x, midY - sample)
                                            ctx.lineTo(x, midY + sample)
                                        }
                                        ctx.stroke()
                                    }
                                    Connections {
                                        target: AudioBridge
                                        function onPositionChanged() { waveformCanvas.requestPaint() }
                                    }
                                }
                            }

                            RowLayout {
                                AppButton { height: 28; text: "Play"; bgcolor: cAccent; color: "#121212"
                                    onClicked: { if (AudioBridge) AudioBridge.play() } }
                                AppButton { height: 28; text: "Stop"; bgcolor: "transparent"; color: "#ffffff"
                                    onClicked: { if (AudioBridge) AudioBridge.stop() } }
                                AppButton { height: 28; text: "Loop"; bgcolor: AudioBridge && AudioBridge.isLoopEnabled ? cAccent : "transparent"; color: "#ffffff"
                                    onClicked: { if (AudioBridge) AudioBridge.isLoopEnabled = !AudioBridge.isLoopEnabled } }
                            }
                        }
                    }

                    // === EVENTS BROWSER ===
                    Loader {
                        active: activePanel === "events"
                        Layout.fillWidth: true; Layout.fillHeight: true
                        sourceComponent: ColumnLayout {
                            spacing: 8

                            Text { text: "CAR AUDIO EVENTS"; color: "white"; font.bold: true; font.pixelSize: 14 }
                            Text { text: "Select an event to view its properties and assign audio files."; color: cMuted; font.pixelSize: 10 }

                            Rectangle {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                color: "#0e0e0e"; border.color: "#3e3e42"; border.width: 1

                                RowLayout {
                                    anchors.fill: parent; anchors.margins: 10
                                    spacing: 20

                                    // Category tree (compact)
                                    ScrollView {
                                        Layout.fillHeight: true; Layout.preferredWidth: parent.width * 0.35
                                        clip: true; ScrollBar.vertical.policy: ScrollBar.AsNeeded

                                        ColumnLayout {
                                            width: parent.width - 6; spacing: 2

                                            Repeater {
                                                model: eventCategories
                                                delegate: ColumnLayout {
                                                    width: parent.width; spacing: 1

                                                    Text {
                                                        text: modelData.icon + " " + modelData.name + " (" + modelData.events.length + ")"
                                                        color: "#bbbbbb"; font.pixelSize: 9; font.bold: true; leftPadding: 4; topPadding: 6
                                                    }

                                                    Repeater {
                                                        model: modelData.events
                                                        delegate: Rectangle {
                                                            height: 20; width: parent.width; radius: 2
                                                            color: selectedEvent === modelData ? cAccent : "transparent"
                                                            AppButton {
                                                                anchors.fill: parent; text: modelData; font.pixelSize: 8
                                                                bgcolor: "transparent"
                                                                color: selectedEvent === modelData ? "#121212" : "#888888"
                                                                onClicked: { selectedEvent = modelData; loadEventFile(modelData) }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // Event property panel
                                    Rectangle {
                                        Layout.fillHeight: true; Layout.fillWidth: true
                                        color: "#1a1a1a"; border.color: "#3e3e42"; border.width: 1

                                        ColumnLayout {
                                            anchors.fill: parent; anchors.margins: 12
                                            visible: selectedEvent !== ""
                                            spacing: 8

                                            Text { text: selectedEvent; color: cAccent; font.pixelSize: 14; font.bold: true }
                                            Text { text: eventInfo(selectedEvent).desc; color: cMuted; font.pixelSize: 10 }

                                            Rectangle { height: 1; color: "#3e3e42"; Layout.fillWidth: true }

                                            RowLayout { Text { text: "Category:"; color: cMuted; font.pixelSize: 9 }
                                                Text { text: eventInfo(selectedEvent).cat; color: cText; font.pixelSize: 9 } }
                                            RowLayout { Text { text: "Volume:"; color: cMuted; font.pixelSize: 9; Layout.preferredWidth: 50 }
                                                Slider { from: 0; to: 100; value: 80; Layout.fillWidth: true; height: 16
                                                    background: Rectangle { x: 0; y: 6; width: parent.width; height: 4; radius: 2; color: "#2a2a2a" }
                                                    handle: Rectangle { x: parent.left + parent.visualPosition * parent.width - 6; y: 4; width: 12; height: 12; radius: 6; color: cAccent } }
                                                Text { text: "80%"; color: cAccent; font.pixelSize: 10; width: 30 } }
                                            RowLayout { Text { text: "Looping:"; color: cMuted; font.pixelSize: 9 }
                                                Rectangle { width: 50; height: 16; radius: 2; color: "#3e3e42"
                                                    Text { anchors.centerIn: parent; text: "Off"; color: "#888"; font.pixelSize: 8 } } }

                                            Rectangle { height: 1; color: "#3e3e42"; Layout.fillWidth: true }

                                            Text { text: "Parameters:"; color: cMuted; font.pixelSize: 9 }

                                            Repeater {
                                                model: eventInfo(selectedEvent).params
                                                delegate: RowLayout {
                                                    Text { text: modelData + ":"; color: "#aaaaaa"; font.pixelSize: 9; Layout.preferredWidth: 60 }
                                                    Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true; height: 16 }
                                                    Text { text: "50"; color: cAccent; font.pixelSize: 9; width: 20 }
                                                }
                                            }
                                        }

                                        Text {
                                            anchors.centerIn: parent
                                            text: "Select an event to view properties"
                                            color: cMuted; font.pixelSize: 10
                                            visible: selectedEvent === ""
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // === EFFECTS ===
                    Loader {
                        active: activePanel === "effects"
                        Layout.fillWidth: true; Layout.fillHeight: true
                        sourceComponent: ColumnLayout {
                            spacing: 8
                            Text { text: "AUDIO EFFECTS"; color: "white"; font.bold: true; font.pixelSize: 14 }

                            GridLayout {
                                columns: 4; columnSpacing: 8; rowSpacing: 8
                                AppButton { height: 36; text: "Compressor"; bgcolor: cAccent; color: "#121212"; onClicked: { if (AudioBridge) AudioBridge.applyCompressor(-20, 4, 5, 100, 0) } }
                                AppButton { height: 36; text: "Reverb"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.applyReverb(0.5, 0.5, 0.3) } }
                                AppButton { height: 36; text: "Delay"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.applyDelay(200, 0.3, 0.5) } }
                                AppButton { height: 36; text: "Chorus"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.applyChorus(0.5, 0.5, 0.4) } }
                                AppButton { height: 36; text: "Flanger"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.applyFlanger(0.5, 0.5, 0.4) } }
                                AppButton { height: 36; text: "Normalize"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.normalize(0.95) } }
                                AppButton { height: 36; text: "Reverse"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.reverse() } }
                                AppButton { height: 36; text: "Pitch Shift"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.pitchShift(2) } }
                            }

                            RowLayout {
                                Text { text: "Effect Chain:"; color: "#bbbbbb" }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: AudioEffects ? AudioEffects.availableEffectTypes() : []
                                    onActivated: { if (AudioEffects) AudioEffects.addEffect(model[index]) }
                                }
                            }
                        }
                    }

                    // === EQ ===
                    Loader {
                        active: activePanel === "eq"
                        Layout.fillWidth: true; Layout.fillHeight: true
                        sourceComponent: ColumnLayout {
                            spacing: 8
                            Text { text: "EQUALIZER"; color: "white"; font.bold: true; font.pixelSize: 14 }

                            Rectangle {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                color: "#252526"; border.color: "#3e3e42"; border.width: 1

                                ColumnLayout {
                                    anchors.fill: parent; anchors.margins: 10; spacing: 6

                                    RowLayout {
                                        Text { text: "Preset:"; color: "#bbbbbb"}
                                        ComboBox {
                                            Layout.fillWidth: true
                                            model: ["Flat", "Voice", "Bass Boost", "Treble", "Rock", "Pop", "Custom"]
                                            onActivated: { if (AudioEffects) AudioEffects.applyEqPreset(model[index]) }
                                        }
                                    }

                                    Repeater {
                                        model: ["31Hz","62Hz","125Hz","250Hz","500Hz","1kHz","2kHz","4kHz","8kHz","16kHz"]
                                        delegate: RowLayout {
                                            Text { text: modelData; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(index) : 0; Layout.fillWidth: true
                                                onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(index, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(index).toFixed(1) : "0") + "dB"; color: cAccent; font.pixelSize: 11 }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // === All effect panels via Loader ===
                    Loader { active: activePanel === "dynamics";     Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioDynamicsProcessor {} }
                    Loader { active: activePanel === "convolution";  Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioConvolutionReverb {} }
                    Loader { active: activePanel === "stereo";       Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioStereoEnhancer {} }
                    Loader { active: activePanel === "tape";         Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioTapeEmulator {} }
                    Loader { active: activePanel === "transient";    Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioTransientDesigner {} }
                    Loader { active: activePanel === "pitch";        Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioPitchShifter {} }
                    Loader { active: activePanel === "noisegate";    Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioNoiseGate {} }
                    Loader { active: activePanel === "multiband";    Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioMultiBandSplitter {} }
                    Loader { active: activePanel === "delayfx";      Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioDelayEffect {} }
                    Loader { active: activePanel === "limiter";      Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioLimiter {} }
                    Loader { active: activePanel === "reverb";       Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioReverbUI {} }
                    Loader { active: activePanel === "modulation";   Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioModulation {} }
                    Loader { active: activePanel === "saturation";   Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioSaturationDistortion {} }
                    Loader { active: activePanel === "deesser";      Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioDeEsser {} }
                    Loader { active: activePanel === "ducker";       Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioDucker {} }
                    Loader { active: activePanel === "bitcrusher";   Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioBitCrusher {} }
                    Loader { active: activePanel === "correlator";   Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioStereoCorrelator {} }
                    Loader { active: activePanel === "wahwah";       Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioWahWah {} }
                    Loader { active: activePanel === "ringmod";      Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioRingMod {} }
                    Loader { active: activePanel === "formant";      Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioFormantFilter {} }
                    Loader { active: activePanel === "spectralgate"; Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioSpectralGate {} }
                    Loader { active: activePanel === "analyzer";     Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioAnalyzer {} }
                    Loader { active: activePanel === "loudness";     Layout.fillWidth: true; Layout.fillHeight: true; sourceComponent: AudioLoudnessMeter {} }

                    // === TIMELINE ===
                    Loader {
                        active: activePanel === "timeline"
                        Layout.fillWidth: true; Layout.fillHeight: true
                        sourceComponent: ColumnLayout {
                            spacing: 8
                            Text { text: "TIMELINE"; color: "white"; font.bold: true; font.pixelSize: 14 }
                            Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: "#0e0e0e"; border.color: "#3e3e42"; border.width: 1 }
                            RowLayout { Text { text: "Zoom:"; color: "#bbbbbb" }; Slider { from: 10; to: 100; value: 50; Layout.fillWidth: true } }
                        }
                    }

                    // === AUTOMATION ===
                    Loader {
                        active: activePanel === "automation"
                        Layout.fillWidth: true; Layout.fillHeight: true
                        sourceComponent: ColumnLayout {
                            spacing: 8
                            Text { text: "AUTOMATION"; color: "white"; font.bold: true; font.pixelSize: 14 }
                            RowLayout { Text { text: "Parameter:"; color: "#bbbbbb" };
                                ComboBox { Layout.fillWidth: true; model: ["Volume", "Pan", "Pitch", "Filter"] } }
                            Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: "#0e0e0e"; border.color: "#3e3e42"; border.width: 1 }
                            RowLayout {
                                AppButton { height: 28; text: "Add Point"; bgcolor: "transparent"; color: "#ffffff" }
                                AppButton { height: 28; text: "Delete Point"; bgcolor: "transparent"; color: "#ffffff" }
                            }
                        }
                    }

                    // === MASTERING ===
                    Loader {
                        active: activePanel === "mastering"
                        Layout.fillWidth: true; Layout.fillHeight: true
                        sourceComponent: AudioLoudnessMeter {}
                    }
                }
            }

            // --- Right Panel: Master & Metering ---
            Rectangle {
                width: 200
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15

                    Text { text: "MASTER"; color: "#666"; font.pixelSize: 10; font.bold: true }

                    RowLayout {
                        Text { text: "Volume:"; color: "#bbbbbb"; Layout.preferredWidth: 50 }
                        Slider { from: 0; to: 100; value: masterVolume; Layout.fillWidth: true; onValueChanged: masterVolume = value }
                        Text { text: Math.round(masterVolume) + "%"; color: cAccent; font.pixelSize: 11 }
                    }

                    // Mini waveform
                    Rectangle { width: 170; height: 60; color: "#0e0e0e"; border.color: "#3e3e42"; border.width: 1
                        Canvas {
                            id: waveformMini; anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d"); ctx.clearRect(0, 0, width, height)
                                if (!AudioBridge || AudioBridge.getSampleCount() === 0) return
                                var data = AudioBridge.getWaveformData(Math.floor(width))
                                if (data.length === 0) return
                                ctx.strokeStyle = "#3a3a3e"; ctx.lineWidth = 1; ctx.beginPath()
                                var midY = height / 2
                                for (var i = 0; i < data.length; ++i) {
                                    var x = i * width / data.length
                                    var sample = parseFloat(data[i]) * midY * 0.8
                                    ctx.moveTo(x, midY - sample); ctx.lineTo(x, midY + sample)
                                }
                                ctx.stroke()
                            }
                        }
                    }

                    Rectangle { height: 10; color: "transparent" }

                    Text { text: "METERING"; color: "#666"; font.pixelSize: 10; font.bold: true }

                    RowLayout {
                        Text { text: "L"; color: "#888"; width: 20 }
                        Rectangle { width: Math.max(4, leftPeakDb * 100); height: 14; color: cAccent }
                        Text { text: Math.round(leftPeakDb * 100) + "%"; color: "#888"; font.pixelSize: 9 }
                    }

                    RowLayout {
                        Text { text: "R"; color: "#888"; width: 20 }
                        Rectangle { width: Math.max(4, rightPeakDb * 100); height: 14; color: cAccent }
                        Text { text: Math.round(rightPeakDb * 100) + "%"; color: "#888"; font.pixelSize: 9 }
                    }

                    // Event info panel (when selected)
                    Rectangle {
                        width: 170; Layout.fillHeight: true
                        color: "#0e0e0e"; border.color: "#3e3e42"; border.width: 1
                        visible: selectedEvent !== ""

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 6; spacing: 4
                            Text { text: "EVENT INFO"; color: "#666"; font.pixelSize: 9; font.bold: true }
                            Rectangle { height: 1; color: "#3e3e42"; Layout.fillWidth: true }

                            Text { text: selectedEvent; color: cAccent; font.pixelSize: 9; font.bold: true }
                            Text { text: eventInfo(selectedEvent).desc; color: "#888888"; font.pixelSize: 8; wrapMode: Text.WordWrap }

                            Text { text: "Category: " + eventInfo(selectedEvent).cat; color: "#666"; font.pixelSize: 8 }
                            Text { text: "File: " + selectedEvent + ".wav"; color: "#666"; font.pixelSize: 8 }

                            Item { Layout.fillHeight: true }

                            AppButton { height: 22; text: "Browse..."; bgcolor: "transparent"; color: "#ffffff"; font.pixelSize: 8
                                onClicked: audioImportDialog.open() }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    AppButton {
                        height: 36; text: "Render"
                        bgcolor: cAccent; color: "#121212"
                        onClicked: {
                            if (AudioBridge) AudioBridge.saveAudio("render.wav")
                            statusMessage = "Rendered to render.wav"
                        }
                    }
                }
            }
        }

        // --- Status Bar ---
        Rectangle {
            height: 24; color: "#252526"; Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent; anchors.margins: 4
                Text { text: statusMessage; color: cAccent; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: selectedEvent ? "Event: " + selectedEvent : ""; color: cMuted; font.pixelSize: 9 }
                Rectangle { width: 1; height: 12; color: "#3e3e42" }
                Text { text: "ksEditor v1.0 - Audio"; color: cMuted; font.pixelSize: 10 }
            }
        }
    }

    Connections {
        target: AudioBridge
        function onStatusMessage(msg) { statusMessage = msg }
    }

    Connections {
        target: AudioEffects
        function onMasterSettingsChanged() { statusMessage = "Master chain updated" }
        function onEffectChainChanged() { statusMessage = "Effect chain updated" }
    }
}
