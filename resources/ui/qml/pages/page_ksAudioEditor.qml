import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

// Audio editor page inspired by GoldWave layout
// Connected to AudioWaveformBridge C++ backend for real waveform data

Rectangle {
    id: pageKsAudioEditor
    width: 1280
    height: 720
    color: "#121212"

    // API di base per collegare il backend
    property alias playButton: btnPlay
    property alias stopButton: btnStop
    property alias rewindButton: btnRew
    property alias forwardButton: btnFwd
    property alias recordButton: btnRec
    property alias zoomInButton: btnZoomIn
    property alias zoomOutButton: btnZoomOut
    property alias waveformView: waveArea
    property alias selectionStartField: tfSelStart
    property alias selectionEndField: tfSelEnd
    property alias selectionLengthField: tfSelLen

    property string currentFile: ""         // nome file audio corrente
    property string selectionStart: "0.000"  // in secondi (testo)
    property string selectionEnd: "0.000"
    property string selectionLength: "0.000"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Menu / Toolbar superiore ---
        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 8

                // File
                Button { text: "New"; flat: true; font.pixelSize: 12 }
                Button { text: "Open"; flat: true; font.pixelSize: 12 }
                Button { text: "Save"; flat: true; font.pixelSize: 12 }
                Button { text: "Save As"; flat: true; font.pixelSize: 12 }

                Rectangle { width: 1; height: 22; color: "#444" }

                // Edit
                Button { text: "Cut"; flat: true; font.pixelSize: 12 }
                Button { text: "Copy"; flat: true; font.pixelSize: 12 }
                Button { text: "Paste"; flat: true; font.pixelSize: 12 }
                Button { text: "Trim"; flat: true; font.pixelSize: 12 }
                Button { text: "Delete"; flat: true; font.pixelSize: 12 }

                Rectangle { width: 1; height: 22; color: "#444" }

                // Effects principali stile GoldWave
                Button { text: "Volume"; flat: true; font.pixelSize: 12 }
                Button { text: "Fade"; flat: true; font.pixelSize: 12 }
                Button { text: "Filter"; flat: true; font.pixelSize: 12 }
                Button { text: "EQ"; flat: true; font.pixelSize: 12 }
                Button { text: "Time"; flat: true; font.pixelSize: 12 }

                Item { Layout.fillWidth: true }

                Text {
                    text: currentFile === "" ? "<no file>" : currentFile
                    color: "#aaaaaa"
                    font.pixelSize: 12
                    elide: Text.ElideMiddle
                }
            }
        }

        // --- Barra trasporto (Play/Rec) ---
        Rectangle {
            height: 56
            color: "#181818"
            border.color: "#262626"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                // Pulsanti stile GoldWave (semplici)
                Button {
                    id: btnRew
                    text: "⏮"
                    width: 40
                    height: 40
                    font.pixelSize: 18
                }
                Button {
                    id: btnPlay
                    text: "▶"
                    width: 40
                    height: 40
                    font.pixelSize: 18
                }
                Button {
                    id: btnStop
                    text: "■"
                    width: 40
                    height: 40
                    font.pixelSize: 18
                }
                Button {
                    id: btnFwd
                    text: "⏭"
                    width: 40
                    height: 40
                    font.pixelSize: 18
                }

                Rectangle { width: 1; height: 30; color: "#333" }

                Button {
                    id: btnRec
                    text: "●"
                    width: 40
                    height: 40
                    font.pixelSize: 18
                    contentItem: Text {
                        text: parent.text
                        anchors.centerIn: parent
                        color: "#ff4c4c"
                        font.pixelSize: 18
                    }
                }

                Item { Layout.fillWidth: true }

                // Zoom orizzontale
                Button { id: btnZoomOut; text: "-"; width: 32; height: 32 }
                Slider {
                    id: zoomSlider
                    width: 150
                    from: 0.1
                    to: 10.0
                    value: 1.0
                }
                Button { id: btnZoomIn; text: "+"; width: 32; height: 32 }
            }
        }

        // --- Corpo principale ---
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // --- Area onde (sinistra/centro) ---
            Rectangle {
                id: waveArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#101010"
                border.color: "#202020"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Ruler temporale superiore
                    Rectangle {
                        height: 22
                        color: "#181818"
                        border.color: "#262626"
                        Layout.fillWidth: true

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Time (s)"
                            color: "#888"
                            font.pixelSize: 10
                        }
                    }

                    // Vista waveform - connected to C++ waveformBridge
                    Rectangle {
                        id: waveViewport
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#151515"

                        // griglia leggera
                        Canvas {
                            id: waveCanvas
                            anchors.fill: parent

                            property real midY: height / 2
                            property real scaleY: height * 0.4

                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.fillStyle = "#151515";
                                ctx.fillRect(0, 0, width, height);

                                ctx.strokeStyle = "#222";
                                ctx.lineWidth = 1;

                                // linee verticali di griglia
                                var step = 50;
                                for (var x = 0; x < width; x += step) {
                                    ctx.beginPath();
                                    ctx.moveTo(x + 0.5, 0);
                                    ctx.lineTo(x + 0.5, height);
                                    ctx.stroke();
                                }

                                // linea zero centrale
                                ctx.strokeStyle = "#00aa88";
                                ctx.beginPath();
                                ctx.moveTo(0, midY + 0.5);
                                ctx.lineTo(width, midY + 0.5);
                                ctx.stroke();

                                // Draw real waveform from bridge
                                if (waveformBridge && waveformBridge.hasData) {
                                    var points = waveformBridge.getWaveformPoints(width, 0);
                                    if (points.length > 0) {
                                        ctx.strokeStyle = "#00e6b8";
                                        ctx.lineWidth = 1.5;
                                        ctx.beginPath();
                                        for (var i = 0; i < points.length; ++i) {
                                            var px = points[i].x * width;
                                            var py = midY - points[i].y * scaleY;
                                            if (i === 0)
                                                ctx.moveTo(px, py);
                                            else
                                                ctx.lineTo(px, py);
                                        }
                                        ctx.stroke();

                                        // Draw negative mirror
                                        ctx.strokeStyle = "#00e6b8";
                                        ctx.beginPath();
                                        for (var i = 0; i < points.length; ++i) {
                                            var px = points[i].x * width;
                                            var py = midY + points[i].y * scaleY;
                                            if (i === 0)
                                                ctx.moveTo(px, py);
                                            else
                                                ctx.lineTo(px, py);
                                        }
                                        ctx.stroke();
                                    }
                                } else {
                                    // waveform placeholder when no file loaded
                                    ctx.strokeStyle = "#00e6b8";
                                    ctx.beginPath();
                                    var mid = height / 2;
                                    for (var i = 0; i < width; ++i) {
                                        var t = i / width * Math.PI * 8;
                                        var amp = Math.sin(t) * (height * 0.1);
                                        if (i === 0)
                                            ctx.moveTo(i, mid - amp * height);
                                        else
                                            ctx.lineTo(i, mid - amp * height);
                                    }
                                    ctx.stroke();
                                }
                            }

                            Connections {
                                target: waveformBridge
                                onDataChanged: waveCanvas.requestPaint()
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onPressed: {
                                if (waveformBridge && waveformBridge.hasData) {
                                    var time = waveformBridge.sampleToTime(waveformBridge.timeToSample(mouse.x / width * waveformBridge.duration));
                                    // selectionStart = time.toFixed(3)
                                    console.log("Selection start:", time.toFixed(3))
                                }
                            }
                            onPositionChanged: {
                                if (waveformBridge && waveformBridge.hasData && mouse.buttons & Qt.LeftButton) {
                                    var time = waveformBridge.sampleToTime(waveformBridge.timeToSample(mouse.x / width * waveformBridge.duration));
                                    // selectionEnd = time.toFixed(3)
                                }
                            }
                        }
                    }
                }
            }

            // --- Pannello destro info selezione/analisi ---
            Rectangle {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                color: "#1a1a1a"
                border.color: "#262626"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 14

                    Text {
                        text: "SELECTION"
                        color: "#ffffff"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 6
                        rowSpacing: 4
                        Layout.fillWidth: true

                        Text { text: "Start"; color: "#bbbbbb"; font.pixelSize: 11 }
                        TextField {
                            id: tfSelStart
                            text: selectionStart
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }

                        Text { text: "End"; color: "#bbbbbb"; font.pixelSize: 11 }
                        TextField {
                            id: tfSelEnd
                            text: selectionEnd
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }

                        Text { text: "Length"; color: "#bbbbbb"; font.pixelSize: 11 }
                        TextField {
                            id: tfSelLen
                            text: selectionLength
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        height: 1
                        Layout.fillWidth: true
                        color: "#333"
                    }

                    Text {
                        text: "ANALYSIS"
                        color: "#ffffff"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 6
                        rowSpacing: 4
                        Layout.fillWidth: true

                        Text { text: "Peak"; color: "#bbbbbb"; font.pixelSize: 11 }
                        Text { text: "-0.0 dB"; color: "#10b981"; font.pixelSize: 11 }

                        Text { text: "RMS"; color: "#bbbbbb"; font.pixelSize: 11 }
                        Text { text: "-12.3 dB"; color: "#10b981"; font.pixelSize: 11 }

                        Text { text: "DC Offset"; color: "#bbbbbb"; font.pixelSize: 11 }
                        Text { text: "0.0 %"; color: "#10b981"; font.pixelSize: 11 }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // --- Status bar in basso ---
        Rectangle {
            height: 22
            color: "#191919"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text {
                    text: "Ready"
                    color: "#10b981"
                    font.pixelSize: 10
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "44100 Hz, 16-bit, Stereo"
                    color: "#777"
                    font.pixelSize: 10
                }
            }
        }
    }
}
