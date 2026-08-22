import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.Telemetry 1.0
import ksEditor.TelemetryFeedback 1.0

Rectangle {
    id: telemetryViewer
    width: 1280
    height: 720
    color: "#121212"

    property string selectedFile: ""
    property var telemetryData: {}

    property int lapCount: Telemetry ? Telemetry.lapCount : 0
    property bool isRecording: Telemetry ? Telemetry.recording : false

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        // Header
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#1e1e1e"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20

                Text {
                    text: "Telemetry Viewer"
                    color: "#E10600"
                    font.pixelSize: 20
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Open File"
                    flat: true
                    onClicked: {
                        openFileDialog.open()
                    }
                }
            }
        }

        // File selector
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#252526"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20

                Text {
                    text: "File: " + (selectedFile ? selectedFile.split("/").pop() : "None")
                    color: "#aaa"
                    font.pixelSize: 12
                    width: 400
                }

                Button {
                    text: "Browse..."
                    flat: true
                    onClicked: {
                        openFileDialog.open()
                    }
                }
            }

            // Telemetry type tabs
            Rectangle {
                Layout.fillWidth: true
                height: 50
                color: "#252526"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20

                    Button {
                        text: "Speed"
                        flat: true
                        checked: true
                        onClicked: { /* Show speed telemetry */ }
                    }

                    Button {
                        text: "Throttle"
                        flat: true
                        onClicked: { /* Show throttle telemetry */ }
                    }

                    Button {
                        text: "Brake"
                        flat: true
                        onClicked: { /* Show brake telemetry */ }
                    }

                    Button {
                        text: "RPM"
                        flat: true
                        onClicked: { /* Show RPM telemetry */ }
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: "Laps: " + (telemetryData.lapCount || 0)
                        color: "#aaa"
                        font.pixelSize: 12
                    }
                }
            }

            // Telemetry chart area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#333333"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20

                    // Speed chart
                    Text {
                        id: speedChart
                        text: "Speed Chart Placeholder"
                        color: "#666"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    // Throttle chart
                    Text {
                        id: throttleChart
                        text: "Throttle Chart Placeholder"
                        color: "#666"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        visible: false
                    }

                    // Brake chart
                    Text {
                        id: brakeChart
                        text: "Brake Chart Placeholder"
                        color: "#666"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        visible: false
                    }
                }
            }

            // Action buttons
            Rectangle {
                Layout.fillWidth: true
                height: 60
                color: "#1e1e1e"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20

                    Button {
                        text: "Analyze"
                        flat: true
                        Layout.fillWidth: true
                        onClicked: {
                            if (telemetryData.lapCount > 0) {
                                // Perform analysis
                            }
                        }
                    }

                    Button {
                        text: "Export"
                        flat: true
                        Layout.fillWidth: true
                        onClicked: {
                            // Export telemetry data
                        }
                    }

                    // Feedback loop controls
                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        color: "#252526"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10

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
                                onClicked: {
                                    var dialog = telemetryFeedback ? telemetryFeedback.refLoadDialog : null
                                    if (dialog) dialog.open()
                                }
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
                }
            }

            // Telemetry feedback panel
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#1e1e1e"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    spacing: 15

                    // Metrics row
                    Rectangle {
                        height: 80
                        color: "#252526"
                        border.color: "#333333"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Text { text: "CORRELATION"; color: "#888"; font.pixelSize: 10; font.bold: true }
                            Text {
                                text: TelemetryFeedback.correlationPercent.toFixed(1) + "%"
                                color: TelemetryFeedback.correlationPercent > 80 ? "#00aa00" :
                                       TelemetryFeedback.correlationPercent > 50 ? "#ffaa00" : "#ef4444"
                                font.pixelSize: 11; font.bold: true
                            }

                            Text { text: "SPEED RMSE:"; color: "#aaa"; font.pixelSize: 10 }
                            Text { text: TelemetryFeedback.speedRMSE.toFixed(2) + " km/h"; color: "#E10600"; font.pixelSize: 11 }

                            Text { text: "LATERAL G RMSE:"; color: "#aaa"; font.pixelSize: 10 }
                            Text { text: TelemetryFeedback.lateralGRMSE.toFixed(3); color: "#E10600"; font.pixelSize: 11 }

                            Text { text: "LONGITUDINAL G RMSE:"; color: "#aaa"; font.pixelSize: 10 }
                            Text { text: TelemetryFeedback.longitudinalGRMSE.toFixed(3); color: "#E10600"; font.pixelSize: 11 }

                            Text { text: "LAPS COMPLETED:"; color: "#aaa"; font.pixelSize: 10 }
                            Text { text: TelemetryFeedback.lapCount; color: "#fff"; font.pixelSize: 11; font.bold: true }

                            Text { text: "CURRENT LAP:"; color: "#aaa"; font.pixelSize: 10 }
                            Text { text: TelemetryFeedback.currentLapTime.toFixed(3) + "s"; color: "#fff"; font.pixelSize: 11; font.bold: true }
                        }
                    }

                    // Lap comparison
                    Rectangle {
                        height: 100
                        color: "#252526"
                        border.color: "#333333"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4

                            Text { text: "LAP COMPARISON"; color: "#888"; font.pixelSize: 10; font.bold: true }

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

                    // Speed trace canvas
                    Rectangle {
                        height: 200
                        color: "#1a1a1a"

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            Text { text: "SPEED TRACE"; color: "#888"; font.pixelSize: 10; anchors.centerIn: parent }

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
                                        ctx.setLineDash([])
                                        ctx.stroke()
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

                    // Reference status
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

            // Car telemetry panel (from car_telemetry_Viewer.qml)
            Rectangle {
                Layout.fillWidth: true
                height: 300
                color: "#1e1e1e"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    // Lap summary
                    Rectangle {
                        height: 80
                        color: "#252526"
                        border.color: "#333333"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4

                            Text { text: "LAP SUMMARY"; color: "#888"; font.pixelSize: 10; font.bold: true }
                            Text { text: "Laps: " + lapCount; color: "#fff"; font.pixelSize: 14; font.bold: true }
                            Text { text: "Is Recording: " + (isRecording ? "Yes" : "No"); color: "#aaa"; font.pixelSize: 10 }
                        }
                    }

                    // Lap analysis canvas
                    Rectangle {
                        height: 200
                        color: "#1a1a1a"

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            Text { text: "LAP ANALYSIS"; color: "#888"; font.pixelSize: 10; anchors.centerIn: parent }

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
                                    ctx.globalAlpha = 1.0
                                    ctx.strokeStyle = "#333"
                                    ctx.lineWidth = 1
                                    for (var j = 0; j <= 4; j++) {
                                        var gy = (j / 4) * height
                                        ctx.beginPath()
                                        ctx.moveTo(0, gy)
                                        ctx.lineTo(width, gy)
                                        ctx.stroke()
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

                    // Session data table
                    Rectangle {
                        height: 200
                        color: "#1e1e1e"

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            Text { text: "SESSION DATA"; color: "#888"; font.pixelSize: 10; anchors.centerIn: parent }

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

                    // Compare Laps button
                    Rectangle {
                        height: 40
                        color: "#252526"
                        border.color: "#333333"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4

                            Button {
                                text: "Compare Laps"
                                flat: true
                                onClicked: {
                                    if (Telemetry && lapCount >= 2) {
                                        Telemetry.compareLaps(lapCount - 1, lapCount)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Action buttons
            Rectangle {
                Layout.fillWidth: true
                height: 60
                color: "#1e1e1e"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20

                    Button {
                        text: "Analyze"
                        flat: true
                        Layout.fillWidth: true
                        onClicked: {
                            if (telemetryData.lapCount > 0) {
                                // Perform analysis
                            }
                        }
                    }

                    Button {
                        text: "Export"
                        flat: true
                        Layout.fillWidth: true
                        onClicked: {
                            // Export telemetry data
                        }
                    }
                }
            }

            // File dialog
            FileDialog {
                id: openFileDialog
                title: "Open Telemetry File"
                nameFilters: ["Telemetry files (*.txt *.json *.lprf)", "All files (*)"]
                onAccepted: {
                    selectedFile = selectedFile.toString()
                    // Load telemetry data
                }
            }
        }
    }
}