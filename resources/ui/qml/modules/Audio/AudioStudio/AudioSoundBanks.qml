import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import ksEditor.AudioEngine 1.0
import "../../../widgets"

Item {
    id: soundBanks
    anchors.fill: parent

    readonly property color cAccent: "#E10600"
    readonly property color cMuted: "#666666"
    readonly property color cText: "#cccccc"
    readonly property color cBorder: "#333333"

    property var bankList: AudioEngine ? AudioEngine.getBuses() : []

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        Rectangle {
            height: 32; color: "#252526"; Layout.fillWidth: true
            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "SOUND BANKS"; color: cAccent; font.pixelSize: 11; font.bold: true
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            Rectangle {
                Layout.preferredWidth: 220; Layout.fillHeight: true
                color: "#1e1e1e"; border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 8; spacing: 6

                    Text { text: "BANKS"; color: cMuted; font.pixelSize: 9; font.bold: true }

                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: bankList.length > 0 ? bankList : ["Engine", "Body", "Tires", "Transmission", "Environment"]
                        delegate: Rectangle {
                            width: ListView.view.width; height: 26
                            color: index % 2 === 0 ? "#1a1a1a" : "#161616"; radius: 2
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 4
                                Text { text: "\uF001"; color: cAccent; font.pixelSize: 10; width: 16 }
                                Text { text: modelData; color: cText; font.pixelSize: 10 }
                                Item { Layout.fillWidth: true }
                                Rectangle { width: 14; height: 14; radius: 2; color: "#2a3a2a"
                                    Text { anchors.centerIn: parent; text: "\u2713"; color: "#80ff80"; font.pixelSize: 8 } }
                            }
                        }
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }

                    RowLayout {
                        spacing: 4; Layout.fillWidth: true
                        AppButton { text: "+ Add Bank"; height: 24; bgcolor: "#3e3e42"; color: cText; Layout.fillWidth: true; font.pixelSize: 9 }
                        AppButton { text: "- Remove"; height: 24; bgcolor: "transparent"; color: cText; font.pixelSize: 9 }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: "#1a1a1a"; border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16; spacing: 10

                    Text { text: "BANK DETAILS"; color: cMuted; font.pixelSize: 10; font.bold: true }

                    RowLayout { Text { text: "Name:"; color: "#888"; font.pixelSize: 9 }; TextField { Layout.fillWidth: true; height: 22; font.pixelSize: 9; color: cText; placeholderText: "Bank name"; background: Rectangle { color: "#252526"; radius: 3; border.color: cBorder; border.width: 1 } } }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    Text { text: "SAMPLES"; color: cMuted; font.pixelSize: 9; font.bold: true }

                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: 0
                        delegate: Rectangle {
                            width: ListView.view.width; height: 26; color: "#161616"; radius: 2
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 4
                                Text { text: "sample.wav"; color: cText; font.pixelSize: 10 }
                                Item { Layout.fillWidth: true }
                                Text { text: "44100 Hz"; color: cMuted; font.pixelSize: 8 }
                                AppButton { text: "\u25B6"; width: 22; height: 20; bgcolor: "transparent"; color: cText; font.pixelSize: 8 }
                            }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: "No samples in this bank.\nClick 'Build Banks' to generate."; color: cMuted; font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            visible: parent.count === 0
                        }
                    }

                    RowLayout {
                        spacing: 8; Layout.fillWidth: true
                        AppButton { text: "Add Sample"; height: 26; bgcolor: "#3e3e42"; color: cText; font.pixelSize: 9 }
                        AppButton { text: "Remove"; height: 26; bgcolor: "transparent"; color: cText; font.pixelSize: 9 }
                    }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    RowLayout {
                        spacing: 8; Layout.fillWidth: true
                        AppButton { text: "Build Banks"; height: 30; bgcolor: cAccent; color: "#121212"; font.bold: true; Layout.fillWidth: true; font.pixelSize: 10 }
                        AppButton { text: "Export Banks"; height: 30; bgcolor: "#3e3e42"; color: cText; Layout.fillWidth: true; font.pixelSize: 10 }
                    }
                }
            }
        }
    }
}
