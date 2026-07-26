import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import ksEditor.AudioEffects 1.0
import "../../../widgets"

Item {
    id: effectsRack
    anchors.fill: parent

    readonly property color cAccent: "#E10600"
    readonly property color cMuted: "#666666"
    readonly property color cText: "#cccccc"
    readonly property color cBorder: "#333333"

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        Rectangle {
            height: 32; color: "#252526"; Layout.fillWidth: true
            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "EFFECTS RACK"; color: cAccent; font.pixelSize: 11; font.bold: true
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            Rectangle {
                Layout.preferredWidth: 220; Layout.fillHeight: true
                color: "#1e1e1e"; border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 8; spacing: 6

                    Text { text: "EFFECT CHAIN"; color: cMuted; font.pixelSize: 9; font.bold: true }

                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: AudioEffects ? AudioEffects.availableEffectTypes() : []
                        delegate: Rectangle {
                            width: ListView.view.width; height: 26; color: index % 2 === 0 ? "#1a1a1a" : "#161616"; radius: 2
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 4
                                Text { text: modelData; color: cText; font.pixelSize: 10; Layout.fillWidth: true }
                                Rectangle { width: 14; height: 14; radius: 2; color: "#3a3a3a"
                                    Text { anchors.centerIn: parent; text: "+"; color: cMuted; font.pixelSize: 8 }
                                    MouseArea { anchors.fill: parent; onClicked: { if (AudioEffects) AudioEffects.addEffect(modelData) } } }
                            }
                        }
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }

                    AppButton { height: 26; text: "Clear All"; bgcolor: "transparent"; color: cText; font.pixelSize: 9 }
                }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: "#1a1a1a"; border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16; spacing: 10

                    Text { text: "EFFECT PARAMETERS"; color: cMuted; font.pixelSize: 10; font.bold: true }
                    Text { text: "Select an effect from the chain to edit its parameters"; color: "#666"; font.pixelSize: 10 }

                    Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: "#0e0e0e"; border.color: cBorder; border.width: 1

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 10; spacing: 6
                            visible: false

                            RowLayout { Text { text: "Parameter 1:"; color: "#888"; font.pixelSize: 9 }; Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true } }
                            RowLayout { Text { text: "Parameter 2:"; color: "#888"; font.pixelSize: 9 }; Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true } }
                            RowLayout { Text { text: "Parameter 3:"; color: "#888"; font.pixelSize: 9 }; Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true } }
                            CheckBox { text: "Bypass"; font.pixelSize: 9 }
                        }

                        Text { anchors.centerIn: parent; text: "No effect selected"; color: cMuted; font.pixelSize: 10 }
                    }

                    RowLayout {
                        spacing: 8
                        AppButton { height: 28; text: "Bypass All"; bgcolor: "transparent"; color: cText }
                        AppButton { height: 28; text: "Remove Selected"; bgcolor: "#3e3e42"; color: cText }
                    }
                }
            }
        }
    }
}
