import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: aceBrowser
    color: "#121212"

    property string currentView: "cars"
    property string selectedCar: ""
    property string selectedTrack: ""
    property string selectedFile: ""
    property var decodedMessage: ({})

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: "#1e1e1e"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16

                Text {
                    text: "Assetto Corsa EVO"
                    color: "#E10600"
                    font.pixelSize: 16
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: ACEContent && ACEContent.isAceInstalled ? "#4CAF50" : "#f44336"
                }

                Text {
                    text: ACEContent && ACEContent.isAceInstalled ? "Connected" : "Not found"
                    color: "#888"
                    font.pixelSize: 11
                }

                Button {
                    flat: true
                    text: "Refresh"
                    onClicked: { if (ACEContent) ACEContent.detectInstallation() }
                }
            }
        }

        // View tabs
        Rectangle {
            Layout.fillWidth: true
            height: 36
            color: "#252526"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8

                Repeater {
                    model: [
                        { label: "Cars",     key: "cars"   },
                        { label: "Tracks",   key: "tracks" },
                        { label: "Mods",     key: "mods"   },
                        { label: "Inspect",  key: "inspect" }
                    ]

                    Button {
                        required property var modelData
                        flat: true
                        height: 28
                        text: modelData.label
                        onClicked: aceBrowser.currentView = modelData.key
                        background: Rectangle {
                            color: currentView === modelData.key ? "#E10600" : "transparent"
                            radius: 3
                        }
                        contentItem: Text {
                            text: modelData.label
                            color: currentView === modelData.key ? "#fff" : "#888"
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                TextField {
                    id: searchField
                    Layout.preferredWidth: 200
                    Layout.maximumWidth: 200
                    placeholderText: "Search..."
                    color: "#fff"
                    placeholderTextColor: "#666"
                    background: Rectangle { color: "#333"; radius: 4; border.color: "#555"; border.width: 1 }
                }
            }
        }

        // Content area
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Main list (left panel)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#1e1e1e"

                // Cars/Tracks/Mods view
                ListView {
                    id: contentList
                    anchors.fill: parent
                    anchors.margins: 8
                    clip: true
                    spacing: 2
                    visible: currentView !== "inspect"

                    model: {
                        if (!ACEContent || !ACEContent.isAceInstalled) return []
                        if (currentView === "cars") return ACEContent.listCarsDetailed()
                        if (currentView === "tracks") return ACEContent.listTracksDetailed()
                        if (currentView === "mods") return ACEContent.listModsDetailed()
                        return []
                    }

                    delegate: Rectangle {
                        width: contentList.width
                        height: 48
                        color: modelData.name === aceBrowser.selectedCar || modelData.name === aceBrowser.selectedTrack
                               ? "#333" : (index % 2 === 0 ? "#252526" : "#1e1e1e")
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12

                            Rectangle {
                                width: 32
                                height: 32
                                radius: 4
                                color: "#333"

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.type === "car" ? "\uD83D\uDE97" : modelData.type === "track" ? "\uD83C\uDFCE" : "\uD83D\uDCE6"
                                    font.pixelSize: 16
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: modelData.name || ""
                                    color: "#eee"
                                    font.pixelSize: 13
                                    font.bold: true
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: {
                                        var parts = []
                                        if (modelData.type) parts.push(modelData.type)
                                        if (modelData.skinCount !== undefined) parts.push(modelData.skinCount + " skins")
                                        if (modelData.size !== undefined) parts.push(formatSize(modelData.size))
                                        return parts.join("  |  ")
                                    }
                                    color: "#666"
                                    font.pixelSize: 10
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onDoubleClicked: {
                                if (currentView === "cars") aceBrowser.selectedCar = modelData.name
                                else if (currentView === "tracks") aceBrowser.selectedTrack = modelData.name
                            }
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: contentList.count === 0
                        text: ACEContent && ACEContent.isAceInstalled
                              ? "No content found"
                              : "Assetto Corsa EVO not detected"
                        color: "#555"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // Protobuf inspector view
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    visible: currentView === "inspect"

                    // Hex input
                    Rectangle {
                        Layout.fillWidth: true
                        height: 100
                        color: "#252526"
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8

                            Text { text: "Hex Input (paste protobuf bytes):" ; color: "#888"; font.pixelSize: 11 }

                            TextArea {
                                id: hexInput
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "#0f0"
                                font.family: "Consolas"
                                font.pixelSize: 11
                                background: Rectangle { color: "#1a1a1a" }
                                placeholderText: "Paste hex-encoded protobuf data here..."
                                placeholderTextColor: "#555"
                                wrapMode: TextArea.Wrap
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        height: 32

                        Button {
                            text: "Decode"
                            onClicked: {
                                if (ACEProtobuf) {
                                    aceBrowser.decodedMessage = ACEProtobuf.decodeHexString(hexInput.text)
                                }
                            }
                        }

                        Button {
                            text: "Clear"
                            onClicked: {
                                hexInput.text = ""
                                aceBrowser.decodedMessage = ({})
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: "Known proto files: " + (ACEProtobuf ? ACEProtobuf.getKnownProtoFiles().length : 0)
                            color: "#666"
                            font.pixelSize: 10
                        }
                    }

                    // Decoded output
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#252526"
                        radius: 4

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: 8

                            TextArea {
                                id: decodedOutput
                                readOnly: true
                                color: "#eee"
                                font.family: "Consolas"
                                font.pixelSize: 11
                                background: Rectangle { color: "#1a1a1a" }
                                text: {
                                    if (!aceBrowser.decodedMessage || Object.keys(aceBrowser.decodedMessage).length === 0)
                                        return "No decoded data"
                                    if (ACEProtobuf)
                                        return ACEProtobuf.printMessage(aceBrowser.decodedMessage)
                                    return JSON.stringify(aceBrowser.decodedMessage, null, 2)
                                }
                            }
                        }
                    }

                    // Known field names reference
                    Rectangle {
                        Layout.fillWidth: true
                        height: 80
                        color: "#252526"
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8

                            Text { text: "Known Field Names:"; color: "#E10600"; font.pixelSize: 11; font.bold: true }

                            Flow {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 4

                                Repeater {
                                    model: {
                                        if (!ACEProtobuf) return []
                                        var fields = ACEProtobuf.getFieldNames()
                                        var result = []
                                        for (var key in fields) {
                                            result.push(key + ": " + fields[key])
                                        }
                                        return result
                                    }

                                    Rectangle {
                                        width: textItem.implicitWidth + 12
                                        height: 20
                                        color: "#333"
                                        radius: 3

                                        Text {
                                            id: textItem
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: "#aaa"
                                            font.pixelSize: 9
                                            font.family: "Consolas"
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Detail panel (right)
            Rectangle {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                color: "#252526"
                visible: aceBrowser.selectedCar !== "" || aceBrowser.selectedTrack !== ""

                property string detailName: aceBrowser.selectedCar || aceBrowser.selectedTrack
                property var detailInfo: {
                    if (aceBrowser.selectedCar && ACEContent)
                        return ACEContent.getCarInfo(aceBrowser.selectedCar)
                    return ({})
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: detailName
                            color: "#E10600"
                            font.pixelSize: 14
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        Button {
                            flat: true
                            width: 24
                            height: 24
                            onClicked: { aceBrowser.selectedCar = ""; aceBrowser.selectedTrack = "" }
                            contentItem: Text { text: "X"; color: "#888"; font.pixelSize: 12 }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#444" }

                    ColumnLayout { spacing: 4
                        Text { text: "Path:"; color: "#888"; font.pixelSize: 10 }
                        Text { text: detailInfo.path || ""; color: "#ccc"; font.pixelSize: 10; wrapMode: Text.WrapAnywhere }
                    }

                    ColumnLayout { spacing: 4; visible: detailInfo.skinCount !== undefined
                        Text { text: "Skins:"; color: "#888"; font.pixelSize: 10 }
                        Text { text: (detailInfo.skinCount || 0) + " available"; color: "#ccc"; font.pixelSize: 10 }
                    }

                    ColumnLayout { spacing: 4; visible: detailInfo.totalSize !== undefined
                        Text { text: "Size:"; color: "#888"; font.pixelSize: 10 }
                        Text { text: formatSize(detailInfo.totalSize || 0); color: "#ccc"; font.pixelSize: 10 }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
    }

    function formatSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB"
        if (bytes < 1073741824) return (bytes / 1048576).toFixed(1) + " MB"
        return (bytes / 1073741824).toFixed(2) + " GB"
    }
}
