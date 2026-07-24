import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes 1.15
import Qt.labs.platform 1.1
import ksEditor.Audio 1.0
import "../../widgets"

Rectangle {
    id: engineGen
    width: 900
    height: 680
    color: "#121212"

    readonly property color cAccent: "#E10600"
    readonly property color cPanel: "#1e1e1e"
    readonly property color cBg: "#0e0e0e"
    readonly property color cBorder: "#333333"
    readonly property color cText: "#ffffff"
    readonly property color cMuted: "#666666"

    property int wizardStep: 0
    property string engineConfig: "3.5L V8"
    property string sampleType: "vlow_on_int"
    property int targetRpm: 4000
    property int durationSec: 16
    property bool isGenerating: false
    property bool hasGenerated: false
    property real previewProgress: 0
    property var generatedSamples: []

    property var cylinderOptions: ["I4", "I5", "I6", "V6", "V8", "V10", "V12", "Flat-4", "Flat-6", "Rotary"]
    property var inductionOptions: ["Naturally Aspirated", "Turbocharged", "Supercharged"]
    property var fuelOptions: ["Petrol", "Diesel", "E85", "Hybrid"]
    property var sampleTypeOptions: {
        var ranges = [
            { prefix: "idle", label: "Idle", desc: "Idle RPM (~800-1200)" },
            { prefix: "vlow", label: "Very Low", desc: "Very low RPM (~1200-2500)" },
            { prefix: "low", label: "Low", desc: "Low RPM (~2500-4500)" },
            { prefix: "mid", label: "Mid", desc: "Mid RPM (~4500-6500)" },
            { prefix: "high", label: "High", desc: "High RPM (~6500-8000)" },
            { prefix: "max", label: "Max", desc: "Max RPM (~8000+)" }
        ]
        var loads = ["on", "off"]
        var positions = ["int", "ext"]
        var result = []
        for (var r = 0; r < ranges.length; r++) {
            if (ranges[r].prefix === "idle") {
                var idlePositions = ["int", "ext"]
                for (var p = 0; p < idlePositions.length; p++) {
                    var posLabel = idlePositions[p] === "int" ? "Interior" : "Exterior"
                    result.push({
                        key: "idle_" + idlePositions[p],
                        label: "Idle (" + posLabel + ")",
                        desc: "Idle RPM (~800-1200)",
                        rpmPrefix: "idle",
                        load: "",
                        position: idlePositions[p]
                    })
                }
            } else {
                for (var l = 0; l < loads.length; l++) {
                    for (var p = 0; p < positions.length; p++) {
                        var key = ranges[r].prefix + "_" + loads[l] + "_" + positions[p]
                        var loadLabel = loads[l] === "on" ? "On Load" : "Off Load"
                        var posLabel = positions[p] === "int" ? "Interior" : "Exterior"
                        result.push({
                            key: key,
                            label: ranges[r].label + " - " + loadLabel + " (" + posLabel + ")",
                            desc: ranges[r].desc,
                            rpmPrefix: ranges[r].prefix,
                            load: loads[l],
                            position: positions[p]
                        })
                    }
                }
            }
        }
        return result
    }()

    property real displacement: 3.5
    property int cylinderCount: 8
    property int cylinderLayout: 1
    property int inductionType: 0
    property int fuelType: 0
    property string modelName: ""

    function rpmToFundamental(rpm, cylinders, layout) {
        var firingOrder = cylinders / 2
        if (layout === 4) firingOrder = 1
        return rpm * firingOrder / 60
    }

    function buildSampleName() {
        return modelName + "_eng_" + sampleType
    }

    function generateEngineAudio() {
        if (isGenerating) return
        isGenerating = true
        hasGenerated = false
        previewProgress = 0

        var sampleRate = 44100
        var totalSamples = Math.floor(sampleRate * durationSec)
        var channels = 2
        var data = []

        var fundamental = rpmToFundamental(targetRpm, cylinderCount, cylinderLayout)
        var harmonics = []
        var numHarmonics = 24

        if (cylinderLayout === 3) {
            for (var h = 1; h <= numHarmonics; h++) {
                harmonics.push({ freq: fundamental * h * (h % 2 === 0 ? 0.98 : 1.02), amp: 1.0 / (h * 0.8 + 0.2) })
            }
        } else if (cylinderLayout === 0) {
            for (var h = 1; h <= numHarmonics; h++) {
                harmonics.push({ freq: fundamental * h, amp: 1.0 / (h * 0.6 + 0.4) })
            }
        } else {
            for (var h = 1; h <= numHarmonics; h++) {
                harmonics.push({ freq: fundamental * h, amp: 1.0 / (h * 0.7 + 0.3) })
            }
        }

        var noiseAmount = 0.15
        if (inductionType === 1) noiseAmount = 0.35
        else if (inductionType === 2) noiseAmount = 0.25

        var sampleRpmFactor = 0
        if (sampleType.indexOf("idle") === 0) sampleRpmFactor = 0
        else if (sampleType.indexOf("vlow") === 0) sampleRpmFactor = 1
        else if (sampleType.indexOf("low") === 0) sampleRpmFactor = 2
        else if (sampleType.indexOf("mid") === 0) sampleRpmFactor = 3
        else if (sampleType.indexOf("high") === 0) sampleRpmFactor = 4
        else if (sampleType.indexOf("max") === 0) sampleRpmFactor = 5

        var isOnLoad = sampleType.indexOf("on_") >= 0
        var isInterior = sampleType.indexOf("_int") >= 0

        var rpmRangeStart = [800, 1200, 2500, 4500, 6500, 8000]
        var rpmRangeEnd = [1200, 2500, 4500, 6500, 8000, 10000]
        var typeTargetRpm = Math.round((rpmRangeStart[sampleRpmFactor] + rpmRangeEnd[sampleRpmFactor]) / 2)

        var harmonicBoost = [0.6, 0.8, 1.0, 1.3, 1.6, 2.0]
        var noiseFactor = [2.0, 1.5, 1.0, 0.8, 0.6, 0.5]
        var compressionFactor = [1.0, 1.0, 1.0, 0.85, 0.7, 0.55]

        for (var i = 0; i < totalSamples; i++) {
            var t = i / sampleRate

            var fadeIn = Math.min(1, t * 4)
            var fadeOut = Math.min(1, (durationSec - t) * 2)
            var envelope = fadeIn * fadeOut

            var sample = 0.0
            for (var h = 0; h < harmonics.length; h++) {
                var freq = harmonics[h].freq
                var hBoost = harmonicBoost[sampleRpmFactor]
                var hAmp = harmonics[h].amp * hBoost
                if (h > 6) hAmp *= Math.max(0.2, 1.0 - (h - 6) * 0.05 * (1 + sampleRpmFactor * 0.1))
                var phase = 2 * Math.PI * freq * t + h * 0.3
                sample += Math.sin(phase) * hAmp
            }

            var noise = (Math.random() * 2 - 1) * noiseAmount * noiseFactor[sampleRpmFactor]
            sample += noise

            var inductionNoiseFreq = 60 + Math.random() * 40
            var inductionNoise = Math.sin(2 * Math.PI * inductionNoiseFreq * t) * 0.08 * noiseAmount * noiseFactor[sampleRpmFactor]
            if (inductionType > 0) {
                var boostOsc = Math.sin(2 * Math.PI * fundamental * 0.5 * t) * 0.5 + 0.5
                inductionNoise += boostOsc * 0.1 * noiseAmount * noiseFactor[sampleRpmFactor]
            }
            sample += inductionNoise

            var exhaustNote = Math.sin(2 * Math.PI * fundamental * 0.5 * t + 1.2) * 0.2
            if (cylinderLayout === 3 || cylinderLayout === 2) {
                exhaustNote += Math.sin(2 * Math.PI * fundamental * 0.25 * t + 0.8) * 0.15
            }
            sample += exhaustNote

            if (fundamental > 0) {
                var cylinderRumble = 0
                for (var c = 0; c < cylinderCount; c++) {
                    var firingAngle = 2 * Math.PI * fundamental * t + c * 2 * Math.PI / cylinderCount
                    cylinderRumble += Math.sin(firingAngle) * (0.08 / cylinderCount)
                }
                sample += cylinderRumble
            }

            if (sampleType.indexOf("idle") === 0) {
                var idleMiss = Math.sin(t * 3.7 * Math.PI) * 0.5 + 0.5
                sample *= 0.6 + idleMiss * 0.4
            }

            sample *= envelope * 0.4 * compressionFactor[sampleRpmFactor]

            if (isOnLoad) sample *= 1.15
            else sample *= 0.85

            if (isInterior) {
                sample *= 0.7
                sample += sample * 0.15 * Math.sin(2 * Math.PI * 0.5 * t)
            }

            sample = Math.max(-1, Math.min(1, sample))

            data.push(sample)
            data.push(sample)

            if (i % 4410 === 0) previewProgress = i / totalSamples
        }

        generatedSamples = data
        isGenerating = false
        hasGenerated = true
        previewProgress = 1
        statusText = "Generated: " + buildSampleName() + " (" + totalSamples + " samples)"
    }

    function playSample() {
        if (hasGenerated && AudioBridge) {
            AudioBridge.loadRaw(generatedSamples, 44100, 2)
            AudioBridge.play()
        }
    }

    function exportSample() {
        if (hasGenerated) saveDialog.open()
    }

    function parseInput(text) {
        var parts = text.trim().split(/\s+/)
        if (parts.length >= 2) {
            var dispMatch = parts[0].match(/([\d.]+)\s*L/i)
            if (dispMatch) displacement = parseFloat(dispMatch[1])

            var cylMatch = parts[1].match(/[VvFfIiRr]?\d+/)
            if (cylMatch) {
                var cylStr = cylMatch[0].toLowerCase()
                if (cylStr.indexOf("v") === 0) {
                    var num = parseInt(cylStr.substring(1))
                    cylinderCount = num
                    cylinderLayout = num === 6 ? 2 : (num === 8 ? 3 : (num === 10 ? 4 : (num === 12 ? 5 : 1)))
                } else if (cylStr.indexOf("i") === 0) {
                    cylinderCount = parseInt(cylStr.substring(1))
                    cylinderLayout = 0
                } else if (cylStr.indexOf("flat") >= 0 || cylStr.indexOf("f") === 0) {
                    cylinderCount = parseInt(cylStr.replace("flat", "").replace("f", ""))
                    cylinderLayout = 7
                } else if (cylStr.indexOf("rotary") >= 0 || cylStr.indexOf("r") === 0) {
                    cylinderCount = 2
                    cylinderLayout = 8
                } else {
                    cylinderCount = parseInt(cylStr)
                    cylinderLayout = 1
                }
            }

            if (parts.length >= 3) {
                var typeStr = parts.slice(2).join("_")
                var found = false
                for (var s = 0; s < sampleTypeOptions.length; s++) {
                    if (sampleTypeOptions[s].key === typeStr) {
                        sampleType = typeStr
                        found = true
                        break
                    }
                }
                if (!found) sampleType = typeStr.replace(/-/g, "_")
            }
        }
    }

    property string statusText: "Enter engine parameters and click Generate"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 40
            color: cPanel
            Layout.fillWidth: true
            border.color: cBorder; border.width: 1

            RowLayout {
                anchors.fill: parent; anchors.margins: 8; spacing: 8

                Text { text: "ENGINE SAMPLE GENERATOR"; color: cAccent; font.pixelSize: 13; font.bold: true; font.letterSpacing: 1 }

                Rectangle { width: 1; height: 20; color: cBorder }

                Text { text: "Quick entry:"; color: cMuted; font.pixelSize: 9 }
                TextField {
                    id: quickEntryField
                    Layout.preferredWidth: 280; height: 24
                    font.pixelSize: 10; color: cText
                    placeholderText: "e.g. 3.5L V8 vlow_on_int"
                    background: Rectangle { color: "#252526"; radius: 3; border.color: cBorder; border.width: 1 }
                    onAccepted: {
                        parseInput(text)
                        wizardStep = 4
                    }
                }
                AppButton {
                    text: "Parse"; height: 24; font.pixelSize: 9
                    bgcolor: cAccent; color: "#121212"
                    onClicked: {
                        parseInput(quickEntryField.text)
                        wizardStep = 4
                    }
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    text: "Close"; height: 24; font.pixelSize: 9
                    bgcolor: "transparent"; color: cMuted
                    onClicked: engineGen.visible = false
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 2; color: cBorder
        }

        Rectangle {
            height: 36
            color: "#181818"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent; anchors.margins: 4; spacing: 2

                Repeater {
                    model: [
                        { step: 0, label: "Engine" },
                        { step: 1, label: "Sample Type" },
                        { step: 2, label: "RPM & Duration" },
                        { step: 3, label: "Preview" },
                        { step: 4, label: "Generate" }
                    ]

                    Rectangle {
                        height: 28; radius: 3
                        color: wizardStep === modelData.step ? cAccent : (wizardStep > modelData.step ? "#3a5a3a" : "#252526")
                        Layout.preferredWidth: 100

                        RowLayout {
                            anchors.centerIn: parent; spacing: 4
                            Text {
                                text: (wizardStep > modelData.step ? "\u2713" : (modelData.step + 1)) + "."
                                color: wizardStep >= modelData.step ? "#121212" : cMuted
                                font.pixelSize: 10; font.bold: true
                            }
                            Text {
                                text: modelData.label
                                color: wizardStep === modelData.step ? "#121212" : (wizardStep > modelData.step ? "#80ff80" : cMuted)
                                font.pixelSize: 10; font.bold: wizardStep === modelData.step
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (modelData.step <= wizardStep || modelData.step === 0)
                                    wizardStep = modelData.step
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 1; color: cBorder
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: cBg

            StackLayout {
                anchors.fill: parent; anchors.margins: 16
                currentIndex: wizardStep

                // Step 0: Engine Configuration
                ColumnLayout {
                    spacing: 12

                    Text { text: "ENGINE CONFIGURATION"; color: cText; font.pixelSize: 16; font.bold: true }
                    Text { text: "Configure the engine parameters for sample generation"; color: cMuted; font.pixelSize: 10 }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    GridLayout {
                        columns: 2; columnSpacing: 20; rowSpacing: 10

                        Text { text: "Displacement (L):"; color: "#aaa"; font.pixelSize: 10 }
                        SpinBox {
                            id: dispSpin
                            from: 0.5; to: 16; decimals: 1; stepSize: 0.1; value: displacement; editable: true
                            font.pixelSize: 10
                            onValueModified: displacement = value
                        }

                        Text { text: "Cylinder Layout:"; color: "#aaa"; font.pixelSize: 10 }
                        ComboBox {
                            id: cylLayoutCombo
                            model: cylinderOptions
                            currentIndex: cylinderLayout
                            Layout.preferredWidth: 200; height: 24; font.pixelSize: 10
                            onActivated: {
                                cylinderLayout = index
                                var cylMap = [4, 5, 6, 6, 8, 10, 12, 4, 6, 2]
                                cylinderCount = cylMap[index]
                            }
                        }

                        Text { text: "Cylinders:"; color: "#aaa"; font.pixelSize: 10 }
                        SpinBox {
                            id: cylCountSpin
                            from: 1; to: 16; value: cylinderCount; editable: true
                            font.pixelSize: 10
                            onValueModified: cylinderCount = value
                        }

                        Text { text: "Induction:"; color: "#aaa"; font.pixelSize: 10 }
                        ComboBox {
                            model: inductionOptions; currentIndex: inductionType
                            Layout.preferredWidth: 200; height: 24; font.pixelSize: 10
                            onActivated: inductionType = index
                        }

                        Text { text: "Fuel Type:"; color: "#aaa"; font.pixelSize: 10 }
                        ComboBox {
                            model: fuelOptions; currentIndex: fuelType
                            Layout.preferredWidth: 200; height: 24; font.pixelSize: 10
                            onActivated: fuelType = index
                        }

                        Text { text: "Model Name:"; color: "#aaa"; font.pixelSize: 10 }
                        TextField {
                            text: modelName; height: 24; font.pixelSize: 10; color: cText
                            placeholderText: "e.g. f40, m3, gt86"
                            Layout.preferredWidth: 200
                            background: Rectangle { color: "#252526"; radius: 3; border.color: cBorder; border.width: 1 }
                            onTextEdited: modelName = text.toLowerCase().replace(/[^a-z0-9_]/g, "")
                        }
                    }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    Text { text: "Fundamental frequency at target RPM: " + rpmToFundamental(targetRpm, cylinderCount, cylinderLayout).toFixed(1) + " Hz"; color: cAccent; font.pixelSize: 11; font.bold: true }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        spacing: 8
                        Item { Layout.fillWidth: true }
                        AppButton { text: "Next \u2192"; height: 32; bgcolor: cAccent; color: "#121212"; font.bold: true; font.pixelSize: 11
                            onClicked: wizardStep = 1 }
                    }
                }

                // Step 1: Sample Type
                ColumnLayout {
                    spacing: 8

                    Text { text: "SAMPLE TYPE"; color: cText; font.pixelSize: 16; font.bold: true }
                    Text { text: "Select the type of engine sample to generate"; color: cMuted; font.pixelSize: 10 }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    ScrollView {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        clip: true
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        contentWidth: availableWidth

                        ColumnLayout {
                            width: parent.availableWidth
                            spacing: 2

                            Repeater {
                                model: sampleTypeOptions
                        delegate: Rectangle {
                            height: 32; Layout.fillWidth: true; radius: 4
                            color: sampleType === modelData.key ? cAccent : "#1a1a1a"
                            border.color: sampleType === modelData.key ? cAccent : cBorder
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent; anchors.margins: 8; spacing: 8
                                Rectangle {
                                    width: 16; height: 16; radius: 8
                                    color: sampleType === modelData.key ? "#121212" : "transparent"
                                    border.color: sampleType === modelData.key ? "#121212" : cMuted; border.width: 2
                                }
                                Text { text: modelData.key; color: sampleType === modelData.key ? "#121212" : cText; font.pixelSize: 11; font.bold: true; font.family: "monospace" }
                                Text { text: modelData.label; color: sampleType === modelData.key ? "#121212" : "#aaa"; font.pixelSize: 10 }
                                Item { Layout.fillWidth: true }
                                Text { text: modelData.desc; color: sampleType === modelData.key ? "#333" : cMuted; font.pixelSize: 8 }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: sampleType = modelData.key
                            }
                            }
                        }
                    }
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        spacing: 8
                        AppButton { text: "\u2190 Back"; height: 32; bgcolor: "transparent"; color: cText; font.pixelSize: 10
                            onClicked: wizardStep = 0 }
                        Item { Layout.fillWidth: true }
                        AppButton { text: "Next \u2192"; height: 32; bgcolor: cAccent; color: "#121212"; font.bold: true; font.pixelSize: 11
                            onClicked: wizardStep = 2 }
                    }
                }

                // Step 2: RPM & Duration
                ColumnLayout {
                    spacing: 12

                    Text { text: "RPM & DURATION"; color: cText; font.pixelSize: 16; font.bold: true }
                    Text { text: "Set the target RPM and sample duration"; color: cMuted; font.pixelSize: 10 }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    GridLayout {
                        columns: 2; columnSpacing: 20; rowSpacing: 12

                        Text { text: "Target RPM:"; color: "#aaa"; font.pixelSize: 10 }
                        RowLayout {
                            Slider {
                                id: rpmSlider
                                from: 500; to: 9000; stepSize: 100; value: targetRpm
                                Layout.fillWidth: true; height: 20
                                onValueChanged: targetRpm = value
                                background: Rectangle { x: 0; y: 8; width: parent.width; height: 4; radius: 2; color: "#2a2a2a"
                                    Rectangle { width: parent.width * (parent.parent.value - 500) / 8500; height: parent.height; color: cAccent; radius: 2 } }
                                handle: Rectangle { x: parent.left + parent.visualPosition * parent.width - 8; y: 2; width: 16; height: 16; radius: 8; color: cAccent }
                            }
                            Text {
                                text: targetRpm + " rpm"
                                color: cAccent; font.pixelSize: 12; font.bold: true; font.family: "monospace"
                                Layout.preferredWidth: 80
                            }
                        }

                        Text { text: "Duration:"; color: "#aaa"; font.pixelSize: 10 }
                        RowLayout {
                            Slider {
                                id: durSlider
                                from: 2; to: 60; stepSize: 1; value: durationSec
                                Layout.fillWidth: true; height: 20
                                onValueChanged: durationSec = value
                                background: Rectangle { x: 0; y: 8; width: parent.width; height: 4; radius: 2; color: "#2a2a2a"
                                    Rectangle { width: parent.width * (parent.parent.value - 2) / 58; height: parent.height; color: cAccent; radius: 2 } }
                                handle: Rectangle { x: parent.left + parent.visualPosition * parent.width - 8; y: 2; width: 16; height: 16; radius: 8; color: cAccent }
                            }
                            Text {
                                text: durationSec + "s"
                                color: cAccent; font.pixelSize: 12; font.bold: true; font.family: "monospace"
                                Layout.preferredWidth: 40
                            }
                        }
                    }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    ColumnLayout {
                        spacing: 4
                        Text { text: "Sample Info:"; color: cMuted; font.pixelSize: 9; font.bold: true }
                        Text { text: "Name: " + buildSampleName(); color: cText; font.pixelSize: 10; font.family: "monospace" }
                        Text { text: "Length: " + durationSec + "s @ 44100 Hz"; color: cText; font.pixelSize: 10 }
                        Text { text: "Fundamental: " + rpmToFundamental(targetRpm, cylinderCount, cylinderLayout).toFixed(1) + " Hz"; color: cText; font.pixelSize: 10 }
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        spacing: 8
                        AppButton { text: "\u2190 Back"; height: 32; bgcolor: "transparent"; color: cText; font.pixelSize: 10
                            onClicked: wizardStep = 1 }
                        Item { Layout.fillWidth: true }
                        AppButton { text: "Next \u2192 Preview"; height: 32; bgcolor: cAccent; color: "#121212"; font.bold: true; font.pixelSize: 11
                            onClicked: wizardStep = 3 }
                    }
                }

                // Step 3: Preview
                ColumnLayout {
                    spacing: 10

                    Text { text: "PREVIEW"; color: cText; font.pixelSize: 16; font.bold: true }
                    Text { text: "Preview the generated engine sample before finalizing"; color: cMuted; font.pixelSize: 10 }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    Rectangle {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        color: cBg; border.color: cBorder; border.width: 1; radius: 4

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 16; spacing: 12

                            RowLayout {
                                spacing: 8
                                AppButton {
                                    text: isGenerating ? "Generating..." : "Generate & Preview"
                                    height: 32; font.bold: true; font.pixelSize: 11
                                    bgcolor: isGenerating ? cMuted : cAccent; color: "#121212"
                                    enabled: !isGenerating
                                    onClicked: generateEngineAudio()
                                }
                                AppButton {
                                    text: "\u25B6 Play"; height: 32
                                    bgcolor: hasGenerated ? "#3e3e42" : "transparent"; color: hasGenerated ? cText : cMuted
                                    enabled: hasGenerated
                                    onClicked: playSample()
                                }
                                AppButton {
                                    text: "\u25A0 Stop"; height: 32
                                    bgcolor: "transparent"; color: cText
                                    onClicked: { if (AudioBridge) AudioBridge.stop() }
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: buildSampleName()
                                    color: cAccent; font.pixelSize: 11; font.family: "monospace"; font.bold: true
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                color: "#0e0e0e"; border.color: cBorder; border.width: 1; radius: 3

                                Canvas {
                                    id: waveformCanvas
                                    anchors.fill: parent
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        if (!hasGenerated || generatedSamples.length === 0) {
                                            ctx.fillStyle = cMuted
                                            ctx.font = "11px monospace"
                                            ctx.textAlign = "center"
                                            ctx.fillText("Click 'Generate & Preview' to render waveform", width / 2, height / 2)
                                            return
                                        }
                                        var step = Math.max(1, Math.floor(generatedSamples.length / 2 / width))
                                        ctx.strokeStyle = cAccent
                                        ctx.lineWidth = 1
                                        ctx.beginPath()
                                        for (var x = 0; x < width; x++) {
                                            var idx = Math.floor(x * generatedSamples.length / 2 / width) * 2
                                            if (idx < generatedSamples.length) {
                                                var val = generatedSamples[idx] * height * 0.4
                                                ctx.moveTo(x, height / 2 - val)
                                                ctx.lineTo(x, height / 2 + val)
                                            }
                                        }
                                        ctx.stroke()
                                    }
                                    Connections {
                                        target: engineGen
                                        function onPreviewProgressChanged() { waveformCanvas.requestPaint() }
                                    }
                                }
                            }

                            RowLayout {
                                spacing: 16

                                ColumnLayout { spacing: 2
                                    Text { text: "Status"; color: cMuted; font.pixelSize: 8 }
                                    Text { text: statusText; color: cText; font.pixelSize: 10 }
                                }

                                Item { Layout.fillWidth: true }

                                ColumnLayout { spacing: 2
                                    Text { text: "Samples"; color: cMuted; font.pixelSize: 8 }
                                    Text { text: hasGenerated ? generatedSamples.length / 2 + " (x2 ch)" : "-"; color: cText; font.pixelSize: 10 }
                                }
                                ColumnLayout { spacing: 2
                                    Text { text: "Duration"; color: cMuted; font.pixelSize: 8 }
                                    Text { text: hasGenerated ? (generatedSamples.length / 2 / 44100).toFixed(1) + "s" : "-"; color: cText; font.pixelSize: 10 }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true; height: 16; radius: 3
                                color: "#1a1a1a"; border.color: cBorder; border.width: 1

                                Rectangle {
                                    height: parent.height
                                    width: parent.width * previewProgress
                                    color: isGenerating ? "#ff8800" : cAccent; radius: 3
                                    Behavior on width { NumberAnimation { duration: 50 } }
                                }

                                Text {
                                    anchors.centerIn: parent
                                    text: isGenerating ? "Generating..." + Math.floor(previewProgress * 100) + "%" : (hasGenerated ? "Complete" : "Ready")
                                    color: cMuted; font.pixelSize: 9
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 8
                        AppButton { text: "\u2190 Back"; height: 32; bgcolor: "transparent"; color: cText; font.pixelSize: 10
                            onClicked: wizardStep = 2 }
                        Item { Layout.fillWidth: true }
                        AppButton { text: "Generate & Export \u2192"; height: 32; bgcolor: cAccent; color: "#121212"; font.bold: true; font.pixelSize: 11
                            enabled: hasGenerated
                            onClicked: wizardStep = 4 }
                    }
                }

                // Step 4: Generate & Export
                ColumnLayout {
                    spacing: 12

                    Text { text: "GENERATE & EXPORT"; color: cText; font.pixelSize: 16; font.bold: true }
                    Text { text: "Finalize and export the generated engine sample"; color: cMuted; font.pixelSize: 10 }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    GridLayout {
                        columns: 2; columnSpacing: 16; rowSpacing: 8

                        Text { text: "Engine:"; color: "#aaa"; font.pixelSize: 10 }
                        Text { text: displacement.toFixed(1) + "L " + cylinderOptions[cylinderLayout]; color: cText; font.pixelSize: 11; font.bold: true }

                        Text { text: "Sample Type:"; color: "#aaa"; font.pixelSize: 10 }
                        Text { text: sampleType; color: cText; font.pixelSize: 11; font.family: "monospace" }

                        Text { text: "Target RPM:"; color: "#aaa"; font.pixelSize: 10 }
                        Text { text: targetRpm + " rpm"; color: cText; font.pixelSize: 11 }

                        Text { text: "Duration:"; color: "#aaa"; font.pixelSize: 10 }
                        Text { text: durationSec + " seconds"; color: cText; font.pixelSize: 11 }

                        Text { text: "Induction:"; color: "#aaa"; font.pixelSize: 10 }
                        Text { text: inductionOptions[inductionType]; color: cText; font.pixelSize: 11 }

                        Text { text: "Output:"; color: "#aaa"; font.pixelSize: 10 }
                        Text { text: "44100 Hz / 16-bit / Stereo"; color: cText; font.pixelSize: 11 }
                    }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 80
                        color: "#1a1a1a"; border.color: cBorder; border.width: 1; radius: 4

                        RowLayout {
                            anchors.fill: parent; anchors.margins: 12; spacing: 12

                            ColumnLayout { spacing: 2
                                Text { text: "File Name"; color: cMuted; font.pixelSize: 8 }
                                TextField {
                                    id: fileNameField
                                    text: buildSampleName() + ".wav"
                                    Layout.preferredWidth: 300; height: 24
                                    font.pixelSize: 10; color: cText; font.family: "monospace"
                                    background: Rectangle { color: "#252526"; radius: 3; border.color: cBorder; border.width: 1 }
                                }
                            }

                            Item { Layout.fillWidth: true }

                            ColumnLayout { spacing: 2
                                Text { text: "Export Status"; color: cMuted; font.pixelSize: 8 }
                                Text { text: statusText; color: cAccent; font.pixelSize: 10 }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        spacing: 8; Layout.fillWidth: true

                        AppButton { text: "\u2190 Back"; height: 32; bgcolor: "transparent"; color: cText; font.pixelSize: 10
                            onClicked: wizardStep = 3 }

                        Item { Layout.fillWidth: true }

                        AppButton {
                            text: "\u25B6 Play"; height: 32
                            bgcolor: hasGenerated ? "#3e3e42" : "transparent"; color: hasGenerated ? cText : cMuted
                            enabled: hasGenerated
                            onClicked: playSample()
                        }

                        AppButton {
                            text: "\u25CF Regenerate"; height: 32
                            bgcolor: "#3e3e42"; color: cText
                            onClicked: generateEngineAudio()
                        }

                        AppButton {
                            text: "\u{1F4BE} Save WAV"; height: 36; font.bold: true; font.pixelSize: 11
                            bgcolor: hasGenerated ? cAccent : cMuted; color: "#121212"
                            enabled: hasGenerated
                            onClicked: exportSample()
                        }
                    }
                }
            }
        }

        Rectangle {
            height: 24
            color: "#181818"
            Layout.fillWidth: true
            border.color: cBorder; border.width: 1

            RowLayout {
                anchors.fill: parent; anchors.margins: 4
                Text { text: statusText; color: cAccent; font.pixelSize: 9 }
                Item { Layout.fillWidth: true }
                Text { text: "Step " + (wizardStep + 1) + " of 5"; color: cMuted; font.pixelSize: 9 }
            }
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save Engine Sample"
        nameFilters: ["WAV files (*.wav)", "All files (*)"]
        defaultSuffix: "wav"
        onAccepted: {
            var path = file.toString().replace("file:///", "")
            if (AudioBridge && hasGenerated) {
                AudioBridge.saveRaw(generatedSamples, 44100, 2, 16, path)
                statusText = "Exported: " + path.split("/").pop()
            }
        }
    }
}
