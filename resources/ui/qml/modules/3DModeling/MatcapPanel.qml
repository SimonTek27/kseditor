import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: matcapPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property bool matcapEnabled: Modeler.matcapIsEnabled ? Modeler.matcapIsEnabled() : false
    property var presets: Modeler.matcapPresets ? Modeler.matcapPresets() : []

    Connections {
        target: Modeler
        function onSceneChanged() { refresh() }
    }

    function refresh() {
        matcapEnabled = Modeler.matcapIsEnabled ? Modeler.matcapIsEnabled() : false
        presets = Modeler.matcapPresets ? Modeler.matcapPresets() : []
    }

    Component.onCompleted: refresh()

    FileDialog {
        id: matcapDialog
        title: "Load Matcap Image"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.exr *.hdr)", "All files (*)"]
        onAccepted: {
            if (Modeler.matcapLoad)
                Modeler.matcapLoad(selectedFile.toString())
            refresh()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text { text: "MATCAP"; color: "#E10600"; font.pixelSize: 13; font.bold: true; Layout.fillWidth: true }
            AppButton { text: "X"; height: 24; width: 26; bgcolor: "#3e3e42"; color: "#ffffff"; font.pixelSize: 10; font.bold: true; onClicked: closePanel() }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: matcapEnabled ? "Disable Matcap" : "Enable Matcap"
            bgcolor: matcapEnabled ? "#2a6e2a" : "#E10600"
            color: "#ffffff"
            font.bold: true
            font.pixelSize: 11
            onClicked: {
                if (Modeler.matcapSetEnabled)
                    Modeler.matcapSetEnabled(!matcapEnabled)
                refresh()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "BUILT-IN PRESETS"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        GridView {
            id: presetGrid
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            cellWidth: 64
            cellHeight: 64
            clip: true

            model: presets

            delegate: Rectangle {
                width: 60
                height: 60
                color: "#2a2a2e"
                border.color: matcapEnabled ? "#E10600" : "#444"
                border.width: 1
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 2
                    spacing: 1

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: modelData && modelData.color ? modelData.color : "#888"
                        radius: 28
                        Layout.preferredHeight: 40
                    }

                    Text {
                        text: modelData ? modelData.name : ""
                        color: "#aaa"
                        font.pixelSize: 7
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (Modeler.matcapApplyPreset)
                            Modeler.matcapApplyPreset(modelData.name)
                        refresh()
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 2; color: "#333" }

        Text { text: "CUSTOM MATCAP"; color: "#E10600"; font.pixelSize: 10; font.bold: true }

        AppButton {
            Layout.fillWidth: true
            height: 30
            text: "Load Custom Matcap..."
            bgcolor: "#ff6600"
            color: "#121212"
            font.bold: true
            font.pixelSize: 11
            onClicked: matcapDialog.open()
        }

        Item { Layout.fillHeight: true }

        Text { text: "Tip: Matcap (Material Capture) renders the object using a spherical environment map instead of real lighting. Great for quick sculpting previews."; color: "#666"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    }
}
