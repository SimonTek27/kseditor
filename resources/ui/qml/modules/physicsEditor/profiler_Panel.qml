import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.PhysicsProfiler 1.0

Rectangle {
    id: profilerPanel
    color: "#1e1e1e"

    property int refreshTimer: 0

    Timer {
        interval: 250
        running: PhysicsProfiler.enabled
        repeat: true
        onTriggered: {
            frameTimeText.text = PhysicsProfiler.frameTimeMs.toFixed(2) + " ms"
            fpsText.text = PhysicsProfiler.fps + " FPS"
            peakText.text = PhysicsProfiler.peakFrameTimeMs.toFixed(2) + " ms"
            avgText.text = PhysicsProfiler.avgFrameTimeMs.toFixed(2) + " ms"
            frameCountText.text = PhysicsProfiler.frameCount
            barList.model = PhysicsProfiler.allSubsystemPercentages()
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

                Text { text: "PHYSICS PROFILER"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                Item { Layout.fillWidth: true }

                KsButton {
                    text: PhysicsProfiler.enabled ? "Pause" : "Resume"
                    height: 26
                    font.pixelSize: 10
                    bgcolor: PhysicsProfiler.enabled ? "#ef4444" : "#22c55e"
                    color: "#ffffff"
                    onClicked: PhysicsProfiler.enabled = !PhysicsProfiler.enabled
                }

                KsButton {
                    text: "Reset"
                    height: 26
                    font.pixelSize: 10
                    bgcolor: "transparent"
                    color: "#aaaaaa"
                    onClicked: PhysicsProfiler.reset()
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 0

                Rectangle {
                    height: 80
                    color: "#2d2d2d"
                    Layout.fillWidth: true

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 16

                        ColumnLayout {
                            Text { text: "Frame Time"; color: "#888"; font.pixelSize: 9 }
                            Text { id: frameTimeText; text: "0.00 ms"; color: "#fff"; font.pixelSize: 16; font.bold: true }
                        }
                        ColumnLayout {
                            Text { text: "FPS"; color: "#888"; font.pixelSize: 9 }
                            Text { id: fpsText; text: "0 FPS"; color: "#fff"; font.pixelSize: 16; font.bold: true }
                        }
                        ColumnLayout {
                            Text { text: "Peak"; color: "#888"; font.pixelSize: 9 }
                            Text { id: peakText; text: "0.00 ms"; color: "#f59e0b"; font.pixelSize: 14 }
                        }
                        ColumnLayout {
                            Text { text: "Average"; color: "#888"; font.pixelSize: 9 }
                            Text { id: avgText; text: "0.00 ms"; color: "#888"; font.pixelSize: 14 }
                        }
                        ColumnLayout {
                            Text { text: "Frames"; color: "#888"; font.pixelSize: 9 }
                            Text { id: frameCountText; text: "0"; color: "#888"; font.pixelSize: 14 }
                        }
                    }
                }

                Text {
                    text: "SUBSYSTEM BREAKDOWN"
                    color: "#666"
                    font.pixelSize: 9
                    font.bold: true
                    leftPadding: 12
                    topPadding: 8
                    bottomPadding: 4
                }

                ListView {
                    id: barList
                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    interactive: false
                    spacing: 2
                    delegate: subsystemBar
                }
            }
        }
    }

    Component {
        id: subsystemBar

        Rectangle {
            height: 28
            color: "#2d2d2d"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Text {
                    text: modelData ? modelData.key : ""
                    color: "#ccc"
                    font.pixelSize: 10
                    Layout.preferredWidth: 120
                }

                Rectangle {
                    height: 12
                    radius: 2
                    color: {
                        var pct = modelData ? modelData.value : 0
                        if (pct > 50) return "#ef4444"
                        if (pct > 30) return "#f59e0b"
                        return "#22c55e"
                    }
                    Layout.preferredWidth: {
                        var pct = modelData ? modelData.value : 0
                        return Math.max(4, parent.width * pct / 100 * 0.5)
                    }
                }

                Text {
                    text: modelData ? modelData.value.toFixed(1) + "%" : ""
                    color: "#888"
                    font.pixelSize: 9
                }
            }
        }
    }
}

