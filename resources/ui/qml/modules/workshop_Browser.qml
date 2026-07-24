import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#1a1a1a"

    property string currentCategory: Workshop ? Workshop.currentCategory : "cars"
    property int itemCount: Workshop ? Workshop.itemCount : 0
    property bool isLoading: Workshop ? Workshop.isLoading : false

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 32
            color: "#252525"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Text { text: "Workshop"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                Text { text: itemCount + " items"; color: "#888"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }

                TextField {
                    id: searchField
                    placeholderText: "Search workshop..."
                    font.pixelSize: 11
                    implicitWidth: 200
                    background: Rectangle { color: "#2d2d2d"; border.color: "#444"; radius: 2 }
                    onAccepted: {
                        if (Workshop) Workshop.searchItems(text)
                    }
                }
                Button {
                    text: "Refresh"
                    flat: true
                    font.pixelSize: 11
                    color: "#aaa"
                    onClicked: {
                        if (Workshop) Workshop.refreshWorkshop()
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                width: 180
                color: "#1e1e1e"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 2

                    Repeater {
                        model: Workshop ? Workshop.getCategories() : ["Cars", "Tracks", "Skins", "Apps", "Sounds", "Physics"]
                        Button {
                            text: modelData
                            flat: true
                            Layout.fillWidth: true
                            font.pixelSize: 12
                            contentItem: Text { text: parent.text; color: currentCategory === modelData.toLowerCase() ? "#E10600" : "#ccc"; leftPadding: 12 }
                            background: Rectangle { color: currentCategory === modelData.toLowerCase() ? "#2a2a3a" : "#2a2a2a"; radius: 2 }
                            onClicked: {
                                if (Workshop) Workshop.browseCategory(modelData.toLowerCase())
                            }
                        }
                    }
                }
            }

            Rectangle {
                color: "#1a1a1a"
                Layout.fillWidth: true
                Layout.fillHeight: true

                GridView {
                    anchors.fill: parent
                    anchors.margins: 8
                    cellWidth: 200
                    cellHeight: 240
                    clip: true

                    model: Workshop ? Workshop.getItems() : []

                    delegate: Rectangle {
                        width: 180
                        height: 220
                        color: "#252526"
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "#333"
                                radius: 2
                                Image {
                                    anchors.fill: parent
                                    fillMode: Image.PreserveAspectCrop
                                    source: modelData.previewUrl || ""
                                    asynchronous: true
                                }
                                Text { text: modelData.previewUrl ? "" : "Preview"; color: "#666"; anchors.centerIn: parent }
                            }

                            Text { text: modelData.title || "Untitled"; color: "#fff"; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight }
                            Text { text: "by " + (modelData.author || "Unknown"); color: "#888"; font.pixelSize: 10 }
                            Text { text: (modelData.downloads || 0) + " downloads"; color: "#666"; font.pixelSize: 10 }

                            Button {
                                text: "Download"
                                flat: true
                                Layout.fillWidth: true
                                font.pixelSize: 11
                                background: Rectangle { color: "#E10600"; radius: 2 }
                                contentItem: Text { text: parent.text; color: "#fff"; horizontalAlignment: Text.AlignHCenter }
                                onClicked: {
                                    if (Workshop) Workshop.downloadItem(modelData.id || "")
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
