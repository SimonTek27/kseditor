import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: trackSystems
    width: 800
    height: 600
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property string activeSystem: "lighting"
    property string timeOfDay: "afternoon"
    property real sunElevation: 45
    property real sunAzimuth: 180
    property real sunIntensity: 1.0
    property bool useDaylight: true
    property bool useNightLighting: true

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
                    text: "TRACK SYSTEMS"
                    color: "#ff6600"
                    font.pixelSize: 12
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                KsButton { text: "Apply All"; height: 24; font.pixelSize: 10; bgcolor: "#E10600"; color: "#121212" }
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

                    Text { text: "SYSTEMS"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                    KsButton { height: 26; text: "Lighting"; font.pixelSize: 10; bgcolor: activeSystem === "lighting" ? "#E10600" : "#3e3e42"; color: activeSystem === "lighting" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Sky"; font.pixelSize: 10; bgcolor: activeSystem === "sky" ? "#E10600" : "#3e3e42"; color: activeSystem === "sky" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Weather"; font.pixelSize: 10; bgcolor: activeSystem === "weather" ? "#E10600" : "#3e3e42"; color: activeSystem === "weather" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Water"; font.pixelSize: 10; bgcolor: activeSystem === "water" ? "#E10600" : "#3e3e42"; color: activeSystem === "water" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Cameras"; font.pixelSize: 10; bgcolor: activeSystem === "cameras" ? "#E10600" : "#3e3e42"; color: activeSystem === "cameras" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Audio"; font.pixelSize: 10; bgcolor: activeSystem === "audio" ? "#E10600" : "#3e3e42"; color: activeSystem === "audio" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Environment"; font.pixelSize: 10; bgcolor: activeSystem === "environment" ? "#E10600" : "#3e3e42"; color: activeSystem === "environment" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Time of Day"; font.pixelSize: 10; bgcolor: activeSystem === "time" ? "#E10600" : "#3e3e42"; color: activeSystem === "time" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Marshal Points"; font.pixelSize: 10; bgcolor: activeSystem === "marshal" ? "#E10600" : "#3e3e42"; color: activeSystem === "marshal" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "DRS Zones"; font.pixelSize: 10; bgcolor: activeSystem === "drs" ? "#E10600" : "#3e3e42"; color: activeSystem === "drs" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Event Rules"; font.pixelSize: 10; bgcolor: activeSystem === "event" ? "#E10600" : "#3e3e42"; color: activeSystem === "event" ? "#121212" : "#ffffff" }

                    Item { Layout.fillHeight: true }

                    Text { text: "PRESETS"; color: "#666666"; font.pixelSize: 10; font.bold: true }
                    KsButton { height: 26; text: "Save Preset"; font.pixelSize: 10; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { height: 26; text: "Load Preset"; font.pixelSize: 10; bgcolor: "transparent"; color: "#ffffff" }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                anchors.margins: 8
                spacing: 8

                Text { text: "TRACK LIGHTING"; color: "#ff6600"; font.pixelSize: 12; font.bold: true }

                RowLayout {
                    CheckBox { checked: useDaylight }
                    Text { text: "Daylight"; color: "#888888" }
                }

                RowLayout {
                    CheckBox { checked: useNightLighting }
                    Text { text: "Night Lighting"; color: "#888888" }
                }

                RowLayout {
                    Text { text: "Intensity:"; color: "#888888"; font.pixelSize: 11; width: 80 }
                    Slider { width: 150; from: 0; to: 100; value: 80; Layout.fillWidth: true }
                    Text { text: "80%"; color: "#E10600"; font.pixelSize: 11 }
                }

                Text { text: "LIGHT TYPES"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                RowLayout {
                    KsButton { height: 26; text: "Floodlights"; bgcolor: "transparent" }
                    KsButton { height: 26; text: "Pole Lights"; bgcolor: "transparent" }
                    KsButton { height: 26; text: "Start Lights"; bgcolor: "transparent" }
                }

                KsButton { text: "Auto-Place Lights"; height: 28; bgcolor: "transparent"; color: "#ffffff" }
            }

            Rectangle {
                width: 160
                color: "#252526"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    Text { text: "PREVIEW"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                    Rectangle {
                        width: 140
                        height: 140
                        color: "#121212"
                        border.color: "#333333"
                        border.width: 1
                    }

                    Text { text: "SYSTEM STATUS"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                    RowLayout {
                        Rectangle { width: 8; height: 8; radius: 4; color: "#E10600" }
                        Text { text: "Lighting: OK"; color: "#888888"; font.pixelSize: 10 }
                    }

                    RowLayout {
                        Rectangle { width: 8; height: 8; radius: 4; color: "#E10600" }
                        Text { text: "Sky: OK"; color: "#888888"; font.pixelSize: 10 }
                    }

                    RowLayout {
                        Rectangle { width: 8; height: 8; radius: 4; color: "#E10600" }
                        Text { text: "Audio: OK"; color: "#888888"; font.pixelSize: 10 }
                    }

                    Item { Layout.fillHeight: true }

                    KsButton { text: "Reset All"; height: 28; bgcolor: "#ef4444"; color: "#ffffff" }
                }
            }
        }
    }
}
