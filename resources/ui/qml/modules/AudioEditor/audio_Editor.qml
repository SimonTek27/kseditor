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

    property string activePanel: "waveform"
    property real masterVolume: 80
    property string currentFileName: AudioBridge ? AudioBridge.getFileName() : "engine.wav"
    property real leftPeakDb: AudioBridge ? AudioBridge.leftPeak : 0
    property real rightPeakDb: AudioBridge ? AudioBridge.rightPeak : 0
    property string statusMessage: "Ready"

    FileDialog {
        id: audioImportDialog
        title: "Import Audio File"
        nameFilters: ["Audio files (*.wav *.mp3 *.ogg *.flac *.aiff)", "All files (*)"]
        onAccepted: {
            if (AudioBridge) {
                AudioBridge.loadAudio(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: audioExportDialog
        title: "Export Audio File"
        nameFilters: ["WAV files (*.wav)", "OGG files (*.ogg)", "MP3 files (*.mp3)", "All files (*)"]
        onAccepted: {
            if (AudioBridge) {
                AudioBridge.exportAudio(selectedFile.toString().replace("file:///", ""))
            }
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

                KsButton {
                    text: "Import"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: audioImportDialog.open()
                }
                KsButton {
                    text: "Export"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: audioExportDialog.open()
                }
                Rectangle {
                    width: 1
                    height: 20
                    color: "#444444"
                }
                KsButton {
                    text: "Play"
                    flat: true
                    height: 32
                    bgcolor: AudioBridge && AudioBridge.isPlaying ? "#E10600" : "transparent"
                    color: "#ffffff"
                    onClicked: {
                        if (AudioBridge) {
                            if (AudioBridge.isPlaying) AudioBridge.pause()
                            else AudioBridge.play()
                        }
                    }
                }
                KsButton {
                    text: "Stop"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: {
                        if (AudioBridge) AudioBridge.stop()
                    }
                }
                KsButton {
                    text: "Loop"
                    flat: true
                    height: 32
                    bgcolor: AudioBridge && AudioBridge.isLoopEnabled ? "#E10600" : "transparent"
                    color: "#ffffff"
                    onClicked: {
                        if (AudioBridge) AudioBridge.isLoopEnabled = !AudioBridge.isLoopEnabled
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: currentFileName
                    color: "#aaaaaa"
                    font.pixelSize: 12
                }
            }
        }

        // --- Main Content ---
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // --- Left Panel: Panels List ---
            Rectangle {
                width: 160
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

                        Text {
                            text: "PANELS"
                            color: "#666"
                            font.pixelSize: 10
                            font.bold: true
                        }

                        Repeater {
                            model: [
                                {key: "waveform", label: "Waveform"},
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
                            delegate: KsButton {
                                height: 28
                                text: modelData.label
                                bgcolor: activePanel === modelData.key ? "#E10600" : "#3e3e42"
                                color: activePanel === modelData.key ? "#121212" : "#ffffff"
                                onClicked: activePanel = modelData.key
                            }
                        }

                        Rectangle { height: 10 }

                        Text {
                            text: "DIALOGS"
                            color: "#666"
                            font.pixelSize: 10
                            font.bold: true
                        }

                        KsButton {
                            height: 28
                            text: "Settings"
                            bgcolor: "transparent"
                            color: "#ffffff"
                        }
                        KsButton {
                            height: 28
                            text: "Shortcuts"
                            bgcolor: "transparent"
                            color: "#ffffff"
                        }
                        KsButton {
                            height: 28
                            text: "Metadata"
                            bgcolor: "transparent"
                            color: "#ffffff"
                        }

                        Item { Layout.fillHeight: true }

                        KsButton {
                            height: 32
                            text: "Bounce"
                            bgcolor: "#ff6600"
                            color: "#ffffff"
                            onClicked: {
                                if (AudioBridge) AudioBridge.saveAudio("bounce.wav")
                                statusMessage = "Bounced to bounce.wav"
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

                    if (activePanel === "waveform") {
                        Text {
                            text: "WAVEFORM EDITOR"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#0e0e0e"
                            border.color: "#3e3e42"
                            border.width: 1

                            Canvas {
                                id: waveformCanvas
                                anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d")
                                    ctx.clearRect(0, 0, width, height)
                                    if (!AudioBridge || AudioBridge.getSampleCount() === 0) return
                                    var data = AudioBridge.getWaveformData(Math.floor(width))
                                    if (data.length === 0) return
                                    ctx.strokeStyle = "#E10600"
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
                            KsButton {
                                height: 28; text: "Play"
                                bgcolor: "#E10600"; color: "#121212"
                                onClicked: { if (AudioBridge) AudioBridge.play() }
                            }
                            KsButton {
                                height: 28; text: "Stop"
                                bgcolor: "transparent"; color: "#ffffff"
                                onClicked: { if (AudioBridge) AudioBridge.stop() }
                            }
                            KsButton {
                                height: 28; text: "Loop"
                                bgcolor: AudioBridge && AudioBridge.isLoopEnabled ? "#E10600" : "transparent"
                                color: "#ffffff"
                                onClicked: { if (AudioBridge) AudioBridge.isLoopEnabled = !AudioBridge.isLoopEnabled }
                            }
                            KsButton {
                                height: 28; text: "Zoom In"
                                bgcolor: "transparent"; color: "#ffffff"
                                onClicked: { if (AudioBridge) AudioBridge.setLoopRegion(0, AudioBridge.getDurationMs()) }
                            }
                        }
                    }

                    if (activePanel === "effects") {
                        Text {
                            text: "AUDIO EFFECTS"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        GridLayout {
                            columns: 4
                            columnSpacing: 8
                            rowSpacing: 8

                            KsButton { height: 36; text: "Compressor"; bgcolor: "#E10600"; color: "#121212"; onClicked: { if (AudioBridge) AudioBridge.applyCompressor(-20, 4, 5, 100, 0) } }
                            KsButton { height: 36; text: "Reverb"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.applyReverb(0.5, 0.5, 0.3) } }
                            KsButton { height: 36; text: "Delay"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.applyDelay(200, 0.3, 0.5) } }
                            KsButton { height: 36; text: "Chorus"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.applyChorus(0.5, 0.5, 0.4) } }
                            KsButton { height: 36; text: "Flanger"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.applyFlanger(0.5, 0.5, 0.4) } }
                            KsButton { height: 36; text: "Normalize"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.normalize(0.95) } }
                            KsButton { height: 36; text: "Reverse"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.reverse() } }
                            KsButton { height: 36; text: "Pitch Shift"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: { if (AudioBridge) AudioBridge.pitchShift(2) } }
                        }

                        RowLayout {
                            Text { text: "Effect Chain:"; color: "#bbbbbb" }
                            ComboBox {
                                id: effectChainCombo
                                Layout.fillWidth: true
                                model: AudioEffects ? AudioEffects.availableEffectTypes() : []
                                onActivated: {
                                    if (AudioEffects) AudioEffects.addEffect(model[index])
                                }
                            }
                        }
                    }

                    if (activePanel === "eq") {
                        Text {
                            text: "EQUALIZER"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            color: "#252526"
                            border.color: "#3e3e42"
                            border.width: 1

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 8

                                RowLayout {
                                    Text { text: "Preset:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                    ComboBox {
                                        id: eqPresetCombo
                                        Layout.fillWidth: true
                                        model: ["Flat", "Voice", "Bass Boost", "Treble", "Rock", "Pop", "Custom"]
                                        onActivated: {
                                            if (AudioEffects) AudioEffects.applyEqPreset(model[index])
                                        }
                                    }
                                }

                                    ColumnLayout {
                                        spacing: 4
                                        RowLayout {
                                            Text { text: "31Hz"; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(0) : 0; Layout.fillWidth: true; onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(0, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(0).toFixed(1) : "0") + "dB"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                        RowLayout {
                                            Text { text: "62Hz"; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(1) : 0; Layout.fillWidth: true; onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(1, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(1).toFixed(1) : "0") + "dB"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                        RowLayout {
                                            Text { text: "125Hz"; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(2) : 0; Layout.fillWidth: true; onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(2, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(2).toFixed(1) : "0") + "dB"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                        RowLayout {
                                            Text { text: "250Hz"; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(3) : 0; Layout.fillWidth: true; onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(3, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(3).toFixed(1) : "0") + "dB"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                        RowLayout {
                                            Text { text: "500Hz"; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(4) : 0; Layout.fillWidth: true; onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(4, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(4).toFixed(1) : "0") + "dB"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                        RowLayout {
                                            Text { text: "1kHz"; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(5) : 0; Layout.fillWidth: true; onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(5, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(5).toFixed(1) : "0") + "dB"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                        RowLayout {
                                            Text { text: "2kHz"; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(6) : 0; Layout.fillWidth: true; onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(6, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(6).toFixed(1) : "0") + "dB"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                        RowLayout {
                                            Text { text: "4kHz"; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(7) : 0; Layout.fillWidth: true; onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(7, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(7).toFixed(1) : "0") + "dB"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                        RowLayout {
                                            Text { text: "8kHz"; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(8) : 0; Layout.fillWidth: true; onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(8, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(8).toFixed(1) : "0") + "dB"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                        RowLayout {
                                            Text { text: "16kHz"; color: "#888"; width: 50 }
                                            Slider { from: -12; to: 12; value: AudioEffects ? AudioEffects.getEqBand(9) : 0; Layout.fillWidth: true; onValueChanged: { if (AudioEffects) AudioEffects.setEqBand(9, value) } }
                                            Text { text: (AudioEffects ? AudioEffects.getEqBand(9).toFixed(1) : "0") + "dB"; color: "#E10600"; font.pixelSize: 11 }
                                        }
                                    }
                            }
                        }
                    }

                    if (activePanel === "dynamics") {
                        AudioDynamicsProcessor {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "convolution") {
                        AudioConvolutionReverb {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "stereo") {
                        AudioStereoEnhancer {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "tape") {
                        AudioTapeEmulator {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "transient") {
                        AudioTransientDesigner {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "pitch") {
                        AudioPitchShifter {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "noisegate") {
                        AudioNoiseGate {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "multiband") {
                        AudioMultiBandSplitter {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "delayfx") {
                        AudioDelayEffect {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "limiter") {
                        AudioLimiter {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "reverb") {
                        AudioReverbUI {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "modulation") {
                        AudioModulation {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "saturation") {
                        AudioSaturationDistortion {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "deesser") {
                        AudioDeEsser {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "ducker") {
                        AudioDucker {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "bitcrusher") {
                        AudioBitCrusher {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "correlator") {
                        AudioStereoCorrelator {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "wahwah") {
                        AudioWahWah {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "ringmod") {
                        AudioRingMod {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "formant") {
                        AudioFormantFilter {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "spectralgate") {
                        AudioSpectralGate {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "analyzer") {
                        AudioAnalyzer {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    if (activePanel === "timeline") {
                        Text {
                            text: "TIMELINE"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 200
                            color: "#0e0e0e"
                            border.color: "#3e3e42"
                            border.width: 1
                        }

                        RowLayout {
                            Text { text: "Zoom:"; color: "#bbbbbb" }
                            Slider { from: 10; to: 100; value: 50; Layout.fillWidth: true }
                        }
                    }

                    if (activePanel === "automation") {
                        Text {
                            text: "AUTOMATION"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 14
                        }

                        RowLayout {
                            Text { text: "Parameter:"; color: "#bbbbbb" }
                            ComboBox {
                                Layout.fillWidth: true
                                model: ["Volume", "Pan", "Pitch", "Filter"]
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#0e0e0e"
                            border.color: "#3e3e42"
                            border.width: 1
                        }

                        RowLayout {
                            KsButton { height: 28; text: "Add Point"; bgcolor: "transparent"; color: "#ffffff" }
                            KsButton { height: 28; text: "Delete Point"; bgcolor: "transparent"; color: "#ffffff" }
                        }
                    }

                    if (activePanel === "mastering") {
                        AudioLoudnessMeter {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
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

                    Text {
                        text: "MASTER"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    RowLayout {
                        Text { text: "Volume:"; color: "#bbbbbb"; Layout.preferredWidth: 50 }
                        Slider {
                            from: 0; to: 100; value: masterVolume; Layout.fillWidth: true
                            onValueChanged: masterVolume = value
                        }
                        Text { text: Math.round(masterVolume) + "%"; color: "#E10600"; font.pixelSize: 11 }
                    }

                    Rectangle {
                        width: 170
                        height: 80
                        color: "#0e0e0e"
                        border.color: "#3e3e42"
                        border.width: 1

                        Canvas {
                            id: waveformMini
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                if (!AudioBridge || AudioBridge.getSampleCount() === 0) return
                                var data = AudioBridge.getWaveformData(Math.floor(width))
                                if (data.length === 0) return
                                ctx.strokeStyle = "#3a3a3e"
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
                        }
                    }

                    Rectangle { height: 10 }

                    Text {
                        text: "METERING"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    RowLayout {
                        Text { text: "L"; color: "#888"; width: 20 }
                        Rectangle {
                            width: Math.max(4, leftPeakDb * 100)
                            height: 14; color: "#E10600"
                        }
                        Text { text: Math.round(leftPeakDb * 100) + "%"; color: "#888"; font.pixelSize: 9 }
                    }

                    RowLayout {
                        Text { text: "R"; color: "#888"; width: 20 }
                        Rectangle {
                            width: Math.max(4, rightPeakDb * 100)
                            height: 14; color: "#E10600"
                        }
                        Text { text: Math.round(rightPeakDb * 100) + "%"; color: "#888"; font.pixelSize: 9 }
                    }

                    Item { Layout.fillHeight: true }

                    KsButton {
                        height: 36
                        text: "Render"
                        bgcolor: "#E10600"
                        color: "#121212"
                        onClicked: {
                            if (AudioBridge) AudioBridge.saveAudio("render.wav")
                        }
                    }
                }
            }
        }

        // --- Status Bar ---
        Rectangle {
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text { text: statusMessage; color: "#E10600"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "ksEditor v1.0 - Audio"; color: "#666"; font.pixelSize: 10 }
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
