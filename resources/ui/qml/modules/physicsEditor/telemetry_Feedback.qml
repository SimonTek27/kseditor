import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.Telemetry 1.0
import ksEditor.TelemetryFeedback 1.0

Rectangle {
    id: telemetryFeedback
    color: "#1e1e1e"

    FileDialog {
        id: refLoadDialog
        title: "Load Reference Telemetry"
        nameFilters: ["Telemetry files (*.json)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            TelemetryFeedback.loadReferenceTelemetry(path)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 36
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Text { text: "TELEMETRY FEEDBACK LOOP"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                Item { Layout.fillWidth: true }

                AppButton {
                    text: TelemetryFeedback.isActive ? "Stop" : "Start"
                    height: 26
                    font.pixelSize: 10
                    bgcolor: TelemetryFeedback.isActive ? "#ef4444" : "#E10600"
                    color: "#ffffff"
                    onClicked: {
                        if (TelemetryFeedback.isActive) TelemetryFeedback.stopFeedback()
                        else TelemetryFeedback.startFeedback()
                    }
                }

                AppButton {
                    text: "Load Reference"
                    height: 26
                    font.pixelSize: 10
                    bgcolor: "transparent"
                    color: TelemetryFeedback.referenceLoaded ? "#00aa00" : "#aaaaaa"
                    onClicked: refLoadDialog.open()
                }

                AppButton {
                    text: "Reset"
                    height: 26
                    font.pixelSize: 10
                    bgcolor: "transparent"
                    color: "#aaaaaa"
                    onClicked: TelemetryFeedback.reset()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                color: "#1a1a1a"
                Layout.preferredWidth: 260
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    Text { text: "METRICS"; color: "#888"; font.pixelSize: 10; font.bold: true }

                    Rectangle {
                        color: "#252526"
                        radius: 4
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.margins: 8
                            anchors.fill: parent
                            spacing: 4

                            RowLayout {
                                Text { text: "Correlation:"; color: "#aaa"; font.pixelSize: 10 }
                                Text {
                                    text: TelemetryFeedback.correlationPercent.toFixed(1) + "%"
                                    color: TelemetryFeedback.correlationPercent > 80 ? "#00aa00" :
                                           TelemetryFeedback.correlationPercent > 50 ? "#ffaa00" : "#ef4444"
                                    font.pixelSize: 11; font.bold: true
                                }
                            }

                            RowLayout {
                                Text { text: "Speed RMSE:"; color: "#aaa"; font.pixelSize: 10 }
                                Text { text: TelemetryFeedback.speedRMSE.toFixed(2) + " km/h"; color: "#E10600"; font.pixelSize: 11 }
                            }

                            RowLayout {
                                Text { text: "Lateral G RMSE:"; color: "#aaa"; font.pixelSize: 10 }
                                Text { text: TelemetryFeedback.lateralGRMSE.toFixed(3); color: "#E10600"; font.pixelSize: 11 }
                            }

                            RowLayout {
                                Text { text: "Longitudinal G RMSE:"; color: "#aaa"; font.pixelSize: 10 }
                                Text { text: TelemetryFeedback.longitudinalGRMSE.toFixed(3); color: "#E10600"; font.pixelSize: 11 }
                            }

                            Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                            RowLayout {
                                Text { text: "Laps Completed:"; color: "#aaa"; font.pixelSize: 10 }
                                Text { text: TelemetryFeedback.lapCount; color: "#fff"; font.pixelSize: 11; font.bold: true }
                            }

                            RowLayout {
                                Text { text: "Current Lap:"; color: "#aaa"; font.pixelSize: 10 }
                                Text { text: TelemetryFeedback.currentLapTime.toFixed(3) + "s"; color: "#fff"; font.pixelSize: 11 }
                            }
                        }
                    }

                    Text { text: "LAP COMPARISON"; color: "#888"; font.pixelSize: 10; font.bold: true; topPadding: 6 }

                    Rectangle {
                        color: "#252526"
                        radius: 4
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.margins: 8
                            anchors.fill: parent
                            spacing: 4

                            RowLayout {
                                Text { text: "Sim Lap:"; color: "#aaa"; font.pixelSize: 10 }
                                Text { text: TelemetryFeedback.lapTimeSim.toFixed(3) + "s"; color: "#E10600"; font.pixelSize: 11; font.bold: true }
                            }

                            RowLayout {
                                Text { text: "Ref Lap:"; color: "#aaa"; font.pixelSize: 10 }
                                Text { text: TelemetryFeedback.lapTimeRef.toFixed(3) + "s"; color: "#3498DB"; font.pixelSize: 11; font.bold: true }
                            }

                            RowLayout {
                                Text { text: "Delta:"; color: "#aaa"; font.pixelSize: 10 }
                                Text {
                                    text: (TelemetryFeedback.lapTimeDelta > 0 ? "+" : "") + TelemetryFeedback.lapTimeDelta.toFixed(3) + "s"
                                    color: TelemetryFeedback.lapTimeDelta < 0 ? "#00aa00" : "#ef4444"
                                    font.pixelSize: 12; font.bold: true
                                }
                            }
                        }
                    }

                    Text { text: "REFERENCE"; color: "#888"; font.pixelSize: 10; font.bold: true; topPadding: 6 }

                    Rectangle {
                        color: "#252526"
                        radius: 4
                        Layout.fillWidth: true

                        ColumnLayout {
                            anchors.margins: 8
                            anchors.fill: parent
                            spacing: 4

                            Rectangle {
                                width: 12; height: 12; radius: 6
                                color: TelemetryFeedback.referenceLoaded ? "#00aa00" : "#666"
                            }
                            Text {
                                text: TelemetryFeedback.referenceLoaded ? "Reference loaded" : "No reference loaded"
                                color: TelemetryFeedback.referenceLoaded ? "#00aa00" : "#666"
                                font.pixelSize: 10
                            }
                            Text {
                                text: "Load a telemetry JSON file to compare simulated vs real data."
                                color: "#555"
                                font.pixelSize: 9
                                visible: !TelemetryFeedback.referenceLoaded
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Rectangle {
                color: "#1a1a1a"
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        height: 28
                        color: "#252526"
                        Layout.fillWidth: true
                        Text { text: "SPEED TRACE"; color: "#888"; font.pixelSize: 10; anchors.centerIn: parent }
                    }

                    Canvas {
                        id: speedCanvas
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = "#333"
                            ctx.lineWidth = 1
                            for (var g = 0; g <= 4; g++) {
                                var gy = (g / 4) * height
                                ctx.beginPath()
                                ctx.moveTo(0, gy)
                                ctx.lineTo(width, gy)
                                ctx.stroke()
                            }

                            var simTrace = TelemetryFeedback.getSimSpeedTrace()
                            if (simTrace.length > 0) {
                                ctx.strokeStyle = "#E10600"
                                ctx.lineWidth = 2
                                ctx.beginPath()
                                for (var i = 0; i < simTrace.length; i++) {
                                    var x = (i / (simTrace.length - 1)) * width
                                    var y = height - (simTrace[i].v / 300) * height
                                    if (i === 0) ctx.moveTo(x, y)
                                    else ctx.lineTo(x, y)
                                }
                                ctx.stroke()
                            }

                            var refTrace = TelemetryFeedback.getRefSpeedTrace()
                            if (refTrace.length > 0) {
                                ctx.strokeStyle = "#3498DB"
                                ctx.lineWidth = 1.5
                                ctx.setLineDash([4, 4])
                                ctx.beginPath()
                                for (var j = 0; j < refTrace.length; j++) {
                                    var rx = (j / (refTrace.length - 1)) * width
                                    var ry = height - (refTrace[j].v / 300) * height
                                    if (j === 0) ctx.moveTo(rx, ry)
                                    else ctx.lineTo(rx, ry)
                                }
                                ctx.stroke()
                                ctx.setLineDash([])
                            }
                        }

                        Connections {
                            target: TelemetryFeedback
                            function onSampleRecorded() { speedCanvas.requestPaint() }
                            function onLapCompared() { speedCanvas.requestPaint() }
                            function onReferenceChanged() { speedCanvas.requestPaint() }
                            function onReset() { speedCanvas.requestPaint() }
                        }
                    }

                    Rectangle {
                        height: 20
                        color: "#1a1a1a"
                        Layout.fillWidth: true
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 4
                            Rectangle { width: 10; height: 10; color: "#E10600"; radius: 2 }
                            Text { text: "Sim"; color: "#aaa"; font.pixelSize: 9 }
                            Rectangle { width: 10; height: 10; color: "#3498DB"; radius: 2 }
                            Text { text: "Ref"; color: "#aaa"; font.pixelSize: 9 }
                            Item { Layout.fillWidth: true }
                            Text { text: "km/h"; color: "#555"; font.pixelSize: 9 }
                        }
                    }
                }
            }

            Rectangle {
                color: "#1a1a1a"
                Layout.preferredWidth: 220
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        height: 28
                        color: "#252526"
                        Layout.fillWidth: true
                        Text { text: "SECTORS"; color: "#888"; font.pixelSize: 10; anchors.centerIn: parent }
                    }

                    ColumnLayout {
                        anchors.margins: 8
                        anchors.fill: parent
                        spacing: 4

                        Repeater {
                            model: TelemetryFeedback.getSectorComparison()

                            Rectangle {
                                color: "#252526"
                                radius: 4
                                height: 48
                                Layout.fillWidth: true

                                ColumnLayout {
                                    anchors.margins: 6
                                    anchors.fill: parent
                                    spacing: 2

                                    Text { text: "Sector " + modelData.sector; color: "#888"; font.pixelSize: 9; font.bold: true }
                                    RowLayout {
                                        Text { text: "Sim: " + modelData.simTime.toFixed(3) + "s"; color: "#E10600"; font.pixelSize: 10 }
                                        Text { text: "Ref: " + modelData.refTime.toFixed(3) + "s"; color: "#3498DB"; font.pixelSize: 10 }
                                        Text {
                                            text: (modelData.delta > 0 ? "+" : "") + modelData.delta.toFixed(3) + "s"
                                            color: modelData.delta < 0 ? "#00aa00" : "#ef4444"
                                            font.pixelSize: 10; font.bold: true
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent; anchors.margins: 4
                Text {
                    text: TelemetryFeedback.isActive ? "Feedback loop active - recording telemetry"
                                                     : "Feedback loop stopped"
                    color: TelemetryFeedback.isActive ? "#00aa00" : "#666"
                    font.pixelSize: 10
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: TelemetryFeedback.lapCount + " laps | "
                          + (TelemetryFeedback.referenceLoaded ? "reference loaded" : "no reference")
                    color: "#555"
                    font.pixelSize: 9
                }
            }
        }
    }
}

