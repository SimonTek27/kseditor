import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: audioEffects
    width: 600
    height: 400
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property var loadedPlugins: []
    property var activeEffect: null

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 36
            color: "#252525"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10

                Text {
                    text: "Effects"
                    color: "#ffffff"
                    font.pixelSize: 14
                    font.bold: true
                    Layout.alignment: Qt.AlignVCenter
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Load VST"
                    onClicked: loadVstPlugin()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                width: 180
                color: "#181818"
                border.color: "#2a2a2a"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5

                    Text {
                        text: "Available Effects"
                        color: "#aaaaaa"
                        font.pixelSize: 12
                    }

                    ListView {
                        id: effectsList
                        model: ListModel {
                            ListElement { name: "EQ" }
                            ListElement { name: "Compressor" }
                            ListElement { name: "Reverb" }
                            ListElement { name: "Delay" }
                            ListElement { name: "Chorus" }
                            ListElement { name: "Distortion" }
                            ListElement { name: "Filter" }
                            ListElement { name: "Noise Gate" }
                        }
                        delegate: Item {
                            width: 160
                            height: 32

                            Rectangle {
                                width: 156
                                height: 28
                                color: "#2a2a2a"
                                radius: 3

                                Text {
                                    text: model.name
                                    color: "#cccccc"
                                    font.pixelSize: 12
                                    anchors.centerIn: parent
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: loadEffect(model.name)
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                color: "#1a1a1a"
                border.color: "#2a2a2a"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10

                    Text {
                        text: activeEffect ? activeEffect : "Select an effect"
                        color: "#ffffff"
                        font.pixelSize: 14
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        Text {
                            text: "Bypass"
                            color: "#888888"
                        }
                        Switch {
                            checked: false
                        }
                    }
                }
            }
        }
    }

    function loadEffect(name) {
        activeEffect = name;
    }

    function loadVstPlugin() {
        console.log("VST plugin dialog would open here");
    }
}