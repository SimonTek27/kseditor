import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import ksEditor.AudioEngine 1.0
import "../../widgets"

Rectangle {
    id: audioMixer
    width: 900
    height: 600
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property var busList: AudioEngine ? AudioEngine.getBuses() : []
    property var vcaList: AudioEngine ? AudioEngine.getVCAs() : []

    Connections {
        target: AudioEngine
        function onBanksChanged() { refreshBusesAndVCAs() }
        function onBankLoaded() { refreshBusesAndVCAs() }
    }

    function refreshBusesAndVCAs() {
        if (!AudioEngine) return
        busList = AudioEngine.getBuses()
        vcaList = AudioEngine.getVCAs()
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

                Text { text: "AUDIO MIXER"; color: "#E10600"; font.pixelSize: 12; font.bold: true }
                Item { Layout.fillWidth: true }
                KsButton { text: "Refresh"; height: 24; font.pixelSize: 10; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: refreshBusesAndVCAs() }
                KsButton { text: "Load Bank..."; height: 24; font.pixelSize: 10; bgcolor: "transparent"; color: "#ffffff"
                    onClicked: { if (AudioEngine) AudioEngine.loadBank("") } }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 6

                RowLayout {
                    Text { text: "MASTER"; color: "#ff6600"; font.pixelSize: 12; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Text { text: "L"; color: "#888888"; font.pixelSize: 10 }
                    Rectangle { width: 80; height: 16; color: "#E10600" }
                    Text { text: "R"; color: "#888888"; font.pixelSize: 10; Layout.leftMargin: 10 }
                    Rectangle { width: 80; height: 16; color: "#E10600" }
                }

                RowLayout {
                    Text { text: "Volume:"; color: "#888888"; font.pixelSize: 11; width: 60 }
                    Slider {
                        from: 0; to: 1; value: 1; stepSize: 0.01
                        Layout.fillWidth: true
                        onValueChanged: { if (AudioEngine) AudioEngine.setBusVolume("Master", value) }
                    }
                    Text { text: Math.round(value * 100) + "%"; color: "#E10600"; font.pixelSize: 11 }
                }

                Rectangle { height: 10 }

                RowLayout {
                    Text { text: "BUSES (" + busList.length + ")"; color: "#ff6600"; font.pixelSize: 12; font.bold: true }
                    Item { Layout.fillWidth: true }
                }

                Repeater {
                    model: busList
                    delegate: RowLayout {
                        Layout.fillWidth: true; spacing: 6

                        Text { text: modelData.split('/').pop(); color: "#cccccc"; font.pixelSize: 11
                            Layout.preferredWidth: 120; elide: Text.ElideRight }

                        Slider {
                            from: 0; to: 1
                            value: AudioEngine ? AudioEngine.getBusVolume(modelData) : 1
                            stepSize: 0.01; Layout.fillWidth: true
                            onValueChanged: { if (AudioEngine) AudioEngine.setBusVolume(modelData, value) }
                        }

                        Text { text: Math.round(value * 100) + "%"; color: "#E10600"; font.pixelSize: 10; Layout.preferredWidth: 35 }

                        KsButton { text: "M"; height: 20; width: 20; font.pixelSize: 9
                            bgcolor: "#555555"; color: "#ffffff"; checkable: true
                            onToggled: { if (AudioEngine) AudioEngine.setBusMute(modelData, checked) } }
                    }
                }
            }

            Rectangle {
                width: 180; color: "#252526"; Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 10; spacing: 6

                    Text { text: "VCA FADERS (" + vcaList.length + ")"; color: "#666666"; font.pixelSize: 10; font.bold: true }

                    Repeater {
                        model: vcaList
                        delegate: ColumnLayout { spacing: 2
                            Text { text: modelData.split('/').pop(); color: "#888888"; font.pixelSize: 10 }
                            RowLayout {
                                Slider { from: 0; to: 1
                                    value: AudioEngine ? AudioEngine.getVCAVolume(modelData) : 1
                                    stepSize: 0.01; Layout.fillWidth: true
                                    onValueChanged: { if (AudioEngine) AudioEngine.setVCAVolume(modelData, value) } }
                                Text { text: Math.round(value * 100) + "%"; color: "#E10600"; font.pixelSize: 9; Layout.preferredWidth: 30 }
                            }
                        }
                    }

                    Rectangle { height: 10 }
                    Text { text: "SNAPSHOTS"; color: "#666666"; font.pixelSize: 10; font.bold: true }
                    KsButton { text: "Interior Mix"; height: 28; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { text: "Exterior Mix"; height: 28; bgcolor: "transparent"; color: "#ffffff" }
                    KsButton { text: "Replay Mix"; height: 28; bgcolor: "transparent"; color: "#ffffff" }
                    Item { Layout.fillHeight: true }
                    KsButton { text: "Bounce"; height: 32; bgcolor: "#ff6600"; color: "#121212" }
                }
            }
        }
    }
}
