import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ksEditor.Telemetry 1.0

Rectangle {
    color: "#1a1a1a"

    property int lapCount: Telemetry ? Telemetry.lapCount : 0
    property bool isRecording: Telemetry ? Telemetry.recording : false

    FileDialog {
        id: replayLoadDialog
        title: "Load Telemetry"
        nameFilters: ["Telemetry files (*.csv *.json)", "All files (*)"]
        onAccepted: {
            if (Telemetry) {
                Telemetry.loadFile(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: videoExportDialog
        title: "Export Session"
        nameFilters: ["JSON files (*.json)", "CSV files (*.csv)", "All files (*)"]
        onAccepted: {
            if (Telemetry) {
                Telemetry.exportSession(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 32
            color: "#252525"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Text { text: "Telemetry"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                Text { text: lapCount + " laps"; color: "#888"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }

                Button {
                    text: "Load Replay"
                    flat: true
                    font.pixelSize: 11
                    color: "#aaa"
                    onClicked: replayLoadDialog.open()
                }
                Button {
                    text: "Export"
                    flat: true
                    font.pixelSize: 11
                    color: "#aaa"
                    onClicked: videoExportDialog.open()
                }
                Button {
                    text: "Compare Laps"
                    flat: true
                    font.pixelSize: 11
                    color: "#aaa"
                    onClicked: {
                        if (Telemetry && Telemetry.lapCount >= 2) {
                            Telemetry.compareLaps(Telemetry.lapCount - 1, Telemetry.lapCount)
                        }
                    }
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical

            Rectangle {
                color: "#1e1e1e"
                SplitView.preferredHeight: 300

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        height: 28
                        color: "#2a2a2a"
                        Layout.fillWidth: true
                        Text { text: "Lap Analysis"; color: "#888"; font.pixelSize: 11; anchors.centerIn: parent }
                    }

                    Canvas {
                        id: telemetryCanvas
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        property int selectedLap: 0
                        property real maxSpeed: 250

                        function fetchSpeedTrace() { return Telemetry ? Telemetry.getSpeedTrace(selectedLap) : [] }
                        function fetchThrottleTrace() { return Telemetry ? Telemetry.getThrottleTrace(selectedLap) : [] }

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)

                            var speeds = fetchSpeedTrace()
                            var n = speeds.length
                            if (n < 2) {
                                ctx.fillStyle = "#444"
                                ctx.font = "11px sans-serif"
                                ctx.textAlign = "center"
                                ctx.fillText("No lap data", width / 2, height / 2)
                                return
                            }

                            ctx.strokeStyle = "#E10600"
                            ctx.lineWidth = 2
                            ctx.beginPath()

                            for (var i = 0; i < n; i++) {
                                var x = (i / (n - 1)) * width
                                var y = height - (speeds[i] / maxSpeed) * height
                                if (i === 0) ctx.moveTo(x, y)
                                else ctx.lineTo(x, y)
                            }
                            ctx.stroke()

                            var throttles = fetchThrottleTrace()
                            ctx.strokeStyle = "#4CAF50"
                            ctx.lineWidth = 1
                            ctx.globalAlpha = 0.5
                            ctx.beginPath()
                            for (var k = 0; k < throttles.length; k++) {
                                var tx = (k / (Math.max(throttles.length - 1, 1))) * width
                                var ty = height - (throttles[k] / 1.0) * height * 0.5
                                if (k === 0) ctx.moveTo(tx, ty)
                                else ctx.lineTo(tx, ty)
                            }
                            ctx.stroke()
                            ctx.globalAlpha = 1.0

                            ctx.strokeStyle = "#333"
                            ctx.lineWidth = 1
                            for (var j = 0; j <= 4; j++) {
                                var gy = (j / 4) * height
                                ctx.beginPath()
                                ctx.moveTo(0, gy)
                                ctx.lineTo(width, gy)
                                ctx.stroke()
                                ctx.fillStyle = "#666"
                                ctx.font = "9px sans-serif"
                                ctx.fillText(Math.round(maxSpeed * (1 - j / 4)) + " km/h", 4, gy - 3)
                            }
                        }

                        Connections {
                            target: Telemetry
                            function onLapCompleted(lapNumber, lapTime) { selectedLap = lapNumber; requestPaint() }
                            function onLapCountChanged() { selectedLap = Telemetry ? Telemetry.lapCount - 1 : 0; requestPaint() }
                            function onCurrentLapChanged() { requestPaint() }
                            function onSessionStarted(name) { selectedLap = 0; requestPaint() }
                        }
                    }
                }
            }

            Rectangle {
                color: "#1e1e1e"
                SplitView.preferredHeight: 200

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        height: 28
                        color: "#2a2a2a"
                        Layout.fillWidth: true
                        Text { text: "Session Data"; color: "#888"; font.pixelSize: 11; anchors.centerIn: parent }
                    }

                    TableView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        model: Telemetry ? Telemetry.getAllLaps() : []

                        delegate: Rectangle {
                            color: index % 2 === 0 ? "#1a1a1a" : "#222"
                            implicitWidth: 600
                            implicitHeight: 24

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                Text { text: modelData.lapNumber || (index + 1); color: "#fff"; font.pixelSize: 10; width: 40 }
                                Text { text: modelData.lapTime ? Number(modelData.lapTime).toFixed(3) + "s" : "--"; color: "#E10600"; font.pixelSize: 10; width: 80 }
                                Text { text: (modelData.topSpeed || 0) + " km/h"; color: "#aaa"; font.pixelSize: 10; width: 80 }
                                Text { text: modelData.sector1 ? Number(modelData.sector1).toFixed(3) : "--"; color: "#888"; font.pixelSize: 10; width: 80 }
                                Text { text: modelData.sector2 ? Number(modelData.sector2).toFixed(3) : "--"; color: "#888"; font.pixelSize: 10; width: 80 }
                                Text { text: modelData.sector3 ? Number(modelData.sector3).toFixed(3) : "--"; color: "#888"; font.pixelSize: 10; width: 80 }
                            }
                        }
                    }
                }
            }
        }
    }
}
