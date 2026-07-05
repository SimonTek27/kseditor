import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"

Rectangle {
    id: audioStudio
    width: 800
    height: 600
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property string activeSystem: "engine"

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
                    text: "CAR AUDIO SYSTEMS"
                    color: "#ff6600"
                    font.pixelSize: 12
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                KsButton { text: "Preview"; height: 24; font.pixelSize: 10; bgcolor: "transparent"; color: "#ffffff" }
                KsButton { text: "Export Bank"; height: 24; font.pixelSize: 10; bgcolor: "#E10600"; color: "#121212" }
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

                    KsButton { height: 26; text: "Engine"; font.pixelSize: 10; bgcolor: activeSystem === "engine" ? "#E10600" : "#3e3e42"; color: activeSystem === "engine" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Exhaust"; font.pixelSize: 10; bgcolor: activeSystem === "exhaust" ? "#E10600" : "#3e3e42"; color: activeSystem === "exhaust" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Turbo"; font.pixelSize: 10; bgcolor: activeSystem === "turbo" ? "#E10600" : "#3e3e42"; color: activeSystem === "turbo" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Tires"; font.pixelSize: 10; bgcolor: activeSystem === "tires" ? "#E10600" : "#3e3e42"; color: activeSystem === "tires" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Brakes"; font.pixelSize: 10; bgcolor: activeSystem === "brakes" ? "#E10600" : "#3e3e42"; color: activeSystem === "brakes" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Wind"; font.pixelSize: 10; bgcolor: activeSystem === "wind" ? "#E10600" : "#3e3e42"; color: activeSystem === "wind" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Suspension"; font.pixelSize: 10; bgcolor: activeSystem === "suspension" ? "#E10600" : "#3e3e42"; color: activeSystem === "suspension" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Transmission"; font.pixelSize: 10; bgcolor: activeSystem === "transmission" ? "#E10600" : "#3e3e42"; color: activeSystem === "transmission" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Cockpit"; font.pixelSize: 10; bgcolor: activeSystem === "cockpit" ? "#E10600" : "#3e3e42"; color: activeSystem === "cockpit" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Crowd"; font.pixelSize: 10; bgcolor: activeSystem === "crowd" ? "#E10600" : "#3e3e42"; color: activeSystem === "crowd" ? "#121212" : "#ffffff" }
                    KsButton { height: 26; text: "Recording Studio"; font.pixelSize: 10; bgcolor: activeSystem === "recording" ? "#E10600" : "#3e3e42"; color: activeSystem === "recording" ? "#121212" : "#ffffff" }

                    Item { Layout.fillHeight: true }

                    KsButton { text: "Mixer"; height: 28; bgcolor: "transparent"; color: "#ffffff" }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Text { text: "ENGINE SOUNDS"; color: "#ff6600"; font.pixelSize: 12; font.bold: true }

                RowLayout {
                    Text { text: "Type:"; color: "#888888"; width: 70 }
                    ComboBox { width: 150; model: ["Inline-4", "V6", "V8", "V10", "V12", "Flat-6", "Rotary"] }
                }

                RowLayout {
                    Text { text: "Cylinders:"; color: "#888888"; width: 70 }
                    Slider { from: 4; to: 12; value: 8; Layout.fillWidth: true }
                    Text { text: "8"; color: "#E10600" }
                }

                Text { text: "RPM CURVE"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                RowLayout {
                    Text { text: "Idle RPM:"; color: "#888888"; width: 70 }
                    Slider { from: 500; to: 2000; value: 800; Layout.fillWidth: true }
                    Text { text: "800"; color: "#E10600" }
                }

                RowLayout {
                    Text { text: "Max RPM:"; color: "#888888"; width: 70 }
                    Slider { from: 5000; to: 15000; value: 8000; Layout.fillWidth: true }
                    Text { text: "8000"; color: "#E10600" }
                }

                RowLayout {
                    Text { text: "Power:"; color: "#888888"; width: 70 }
                    Slider { from: 200; to: 1000; value: 500; Layout.fillWidth: true }
                    Text { text: "500 HP"; color: "#E10600" }
                }

                RowLayout {
                    Text { text: "Volume:"; color: "#888888"; width: 70 }
                    Slider { from: 0; to: 100; value: 80; Layout.fillWidth: true }
                    Text { text: "80%"; color: "#E10600" }
                }
            }

            Rectangle {
                width: 140
                color: "#252526"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8

                    Text { text: "PREVIEW"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                    RowLayout {
                        KsButton { text: "Play"; height: 28; bgcolor: "#E10600"; color: "#121212" }
                        KsButton { text: "Stop"; height: 28; bgcolor: "#ef4444"; color: "#ffffff" }
                    }

                    Rectangle { height: 10 }

                    Text { text: "BANK STATUS"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                    RowLayout { Text { text: "Engine:"; color: "#888888"; font.pixelSize: 10 }; Text { text: "OK"; color: "#E10600"; font.pixelSize: 10 } }
                    RowLayout { Text { text: "Exhaust:"; color: "#888888"; font.pixelSize: 10 }; Text { text: "OK"; color: "#E10600"; font.pixelSize: 10 } }
                    RowLayout { Text { text: "Tires:"; color: "#888888"; font.pixelSize: 10 }; Text { text: "OK"; color: "#E10600"; font.pixelSize: 10 } }

                    Item { Layout.fillHeight: true }

                    KsButton { text: "Export"; height: 28; bgcolor: "#ff6600"; color: "#121212" }
                }
            }
        }

        Loader {
            id: studioLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.fill: parent
            anchors.margins: 10
            visible: activeSystem === "recording"
            source: activeSystem === "recording" ? "audio_recording_studio.qml" : ""
        }
    }
}
