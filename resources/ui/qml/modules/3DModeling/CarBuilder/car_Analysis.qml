import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: carAnalysis
    width: 800
    height: 600
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property string activeTool: "telemetry"

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

                Text {
                    text: "CAR ANALYSIS TOOLS"
                    color: "#ff6600"
                    font.pixelSize: 12
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                KsButton { text: "Import Data"; height: 24; font.pixelSize: 10; bgcolor: "transparent"; color: "#ffffff" }
                KsButton { text: "Export"; height: 24; font.pixelSize: 10; bgcolor: "transparent"; color: "#ffffff" }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Rectangle {
                width: 140
                color: "#252526"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    Text { text: "TOOLS"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                    KsButton { height: 26; text: "Telemetry"; font.pixelSize: 10; bgcolor: activeTool === "telemetry" ? "#E10600" : "#3e3e42"; color: activeTool === "telemetry" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Lap Analysis"; font.pixelSize: 10; bgcolor: activeTool === "lapAnalysis" ? "#E10600" : "#3e3e42"; color: activeTool === "lapAnalysis" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Tire Monitor"; font.pixelSize: 10; bgcolor: activeTool === "tireMonitor" ? "#E10600" : "#3e3e42"; color: activeTool === "tireMonitor" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Damage Report"; font.pixelSize: 10; bgcolor: activeTool === "damage" ? "#E10600" : "#3e3e42"; color: activeTool === "damage" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Pit Stop Sim"; font.pixelSize: 10; bgcolor: activeTool === "pitStop" ? "#E10600" : "#3e3e42"; color: activeTool === "pitStop" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Power Curve"; font.pixelSize: 10; bgcolor: activeTool === "power" ? "#E10600" : "#3e3e42"; color: activeTool === "power" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Replay"; font.pixelSize: 10; bgcolor: activeTool === "replay" ? "#E10600" : "#3e3e42"; color: activeTool === "replay" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Comparison"; font.pixelSize: 10; bgcolor: activeTool === "comparison" ? "#E10600" : "#3e3e42"; color: activeTool === "comparison" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Championship"; font.pixelSize: 10; bgcolor: activeTool === "championship" ? "#E10600" : "#3e3e42"; color: activeTool === "championship" ? "#121212" : "#ffffff" }

                    Item { Layout.fillHeight: true }

                    KsButton { text: "Run All"; height: 28; bgcolor: "#E10600"; color: "#121212" }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Text { text: "TELEMETRY VIEWER"; color: "#ff6600"; font.pixelSize: 12; font.bold: true }

                RowLayout {
                    Text { text: "Session:"; color: "#888888"; font.pixelSize: 11; width: 70 }
                    ComboBox { width: 150; model: ["Race 1", "Qualifying", "Practice 2"] }
                }

                RowLayout {
                    Text { text: "Lap:"; color: "#888888"; font.pixelSize: 11; width: 70 }
                    ComboBox { width: 100; model: ["Lap 1", "Lap 2", "Lap 3", "Best"] }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#121212"
                    border.color: "#333333"
                    border.width: 1
                }

                RowLayout {
                    Text { text: "Speed: 185 km/h"; color: "#888888"; font.pixelSize: 11 }
                    Text { text: "Throttle: 85%"; color: "#888888"; font.pixelSize: 11; Layout.leftMargin: 20 }
                    Text { text: "Brake: 20%"; color: "#888888"; font.pixelSize: 11; Layout.leftMargin: 20 }
                    Text { text: "Steering: 15deg"; color: "#888888"; font.pixelSize: 11; Layout.leftMargin: 20 }
                }
            }

            Rectangle {
                width: 140
                color: "#252526"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8

                    Text { text: "QUICK STATS"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                    RowLayout { Text { text: "Best Lap:"; color: "#888888"; font.pixelSize: 10 }; Text { text: "1:41.654"; color: "#E10600"; font.pixelSize: 10 } }
                    RowLayout { Text { text: "Top Speed:"; color: "#888888"; font.pixelSize: 10 }; Text { text: "285 km/h"; color: "#ffffff"; font.pixelSize: 10 } }
                    RowLayout { Text { text: "Fuel:"; color: "#888888"; font.pixelSize: 10 }; Text { text: "65L"; color: "#ff6600"; font.pixelSize: 10 } }

                    Rectangle { height: 10 }

                    Text { text: "TIRE STATUS"; color: "#666666"; font.pixelSize: 10; font.bold: true }
                    Text { text: "Soft - 15 laps"; color: "#ffffff"; font.pixelSize: 11 }
                    Text { text: "Medium - 25 laps"; color: "#888888"; font.pixelSize: 11 }
                    Text { text: "Hard - 35 laps"; color: "#666666"; font.pixelSize: 11 }

                    Item { Layout.fillHeight: true }

                    KsButton { text: "Clear Data"; height: 28; bgcolor: "#ef4444"; color: "#ffffff" }
                }
            }
        }
    }
}
