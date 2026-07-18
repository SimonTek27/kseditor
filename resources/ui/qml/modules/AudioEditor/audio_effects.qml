import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import ksEditor.Audio 1.0
import ksEditor.AudioEffects 1.0

Rectangle {
    id: audioEffects
    width: 600
    height: 400
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property var loadedPlugins: []
    property string activeEffect: ""

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 36; color: "#252525"; Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 10
                Text { text: "Effects"; color: "#ffffff"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignVCenter }
                Item { Layout.fillWidth: true }
                Button { text: "Clear All"; onClicked: { if (AudioEffects) AudioEffects.clearEffects() } }
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true

            Rectangle {
                width: 180; color: "#181818"; border.color: "#2a2a2a"; border.width: 1
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 5
                    Text { text: "Available Effects"; color: "#aaaaaa"; font.pixelSize: 12 }

                    ListView {
                        id: effectsList
                        model: AudioEffects ? AudioEffects.availableEffectTypes() : []
                        delegate: Item {
                            width: 160; height: 32
                            Rectangle {
                                width: 156; height: 28; color: "#2a2a2a"; radius: 3
                                Text { text: modelData; color: "#cccccc"; font.pixelSize: 12; anchors.centerIn: parent }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: { activeEffect = modelData; if (AudioEffects) AudioEffects.addEffect(modelData) }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                color: "#1a1a1a"; border.color: "#2a2a2a"; border.width: 1; Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 10
                    Text { text: activeEffect || "Select an effect"; color: "#ffffff"; font.pixelSize: 14 }

                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: AudioEffects ? AudioEffects.getEffectChain() : []
                        delegate: Rectangle {
                            width: parent.width; height: 24; color: index % 2 === 0 ? "#222" : "#1a1a1a"
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 4
                                Text { text: modelData.type + " [" + modelData.index + "]"; color: "#cccccc"; font.pixelSize: 10; Layout.fillWidth: true }
                                Switch { checked: !modelData.bypassed; onToggled: { if (AudioEffects) AudioEffects.bypassEffect(index, !checked) } }
                                Button { text: "X"; width: 20; height: 20; font.pixelSize: 8; onClicked: { if (AudioEffects) AudioEffects.removeEffect(index) } }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Text { text: "Bypass All"; color: "#888888" }
                        Switch { checked: AudioEffects ? AudioEffects.masterBypassed : false; onToggled: { if (AudioEffects) AudioEffects.setMasterBypass(checked) } }
                    }
                }
            }
        }
    }
}
