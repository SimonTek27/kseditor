import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Window
import ksEditor.FontCreator 1.0

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    minimumWidth: 900
    minimumHeight: 500
    title: "KS Font Editor"
    visible: true
    color: "#121212"

    // Toolbar with font controls
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header row
        RowLayout {
            id: headerRow
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            spacing: 12
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            Text {
                text: "ksFontEditor"
                font.pixelSize: 14
                font.bold: true
                color: "#E10600"
            }

            Item { Layout.fillWidth: true }

            // Font selector
            RowLayout {
                spacing: 0
                Rectangle {
                    Layout.preferredHeight: 32
                    color: "#1e1e1e"
                    radius: 4
                    Layout.leftMargin: 1
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Font"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#ffffff"
                        padding: 8
                    }
                }
                ComboBox {
                    id: fontSelector
                    model: ["Tahoma", "Arial", "Verdana"]
                    currentIndex: 0
                    implicitWidth: 100
                    background: Rectangle {
                        color: "white"
                        radius: 4
                        border.color: "#444444"
                    }
                    onCurrentTextChanged: {
                        if (FontCreator) FontCreator.setFontFamily(currentText)
                    }
                }
            }

            // Font size
            RowLayout {
                spacing: 0
                Rectangle {
                    Layout.preferredHeight: 32
                    color: "#1e1e1e"
                    radius: 4
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Size"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#ffffff"
                        padding: 10
                    }
                }
                Button {
                    text: "<"
                    implicitWidth: 28
                    implicitHeight: 32
                    background: Rectangle { color: "white"; border.color: "#444444" }
                    onClicked: if (fontSize > 8) FontCreator.fontSize--
                }
                TextField {
                    id: sizeInput
                    implicitWidth: 40
                    implicitHeight: 32
                    text: FontCreator ? FontCreator.fontSize : 32
                    horizontalAlignment: TextInput.AlignHCenter
                    background: Rectangle { color: "white"; border.color: "#444444" }
                    onTextChanged: {
                        var val = parseInt(text)
                        if (!isNaN(val) && val > 0) {
                            if (FontCreator) FontCreator.fontSize = val
                        }
                    }
                }
                Button {
                    text: ">"
                    implicitWidth: 28
                    implicitHeight: 32
                    background: Rectangle { color: "white"; border.color: "#444444" }
                    onClicked: if (fontSize < 128) FontCreator.fontSize++
                }
            }

            // Resolution
            RowLayout {
                spacing: 0
                Rectangle {
                    Layout.preferredHeight: 32
                    color: "#1e1e1e"
                    radius: 4
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Res"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#ffffff"
                        padding: 10
                    }
                }
                ComboBox {
                    id: resSelector
                    model: ["1920 x 1080 px", "1280 x 720 px", "1024 x 768 px"]
                    currentIndex: 0
                    implicitWidth: 140
                    background: Rectangle { color: "white"; radius: 4; border.color: "#444444" }
                    onCurrentTextChanged: {
                        if (FontCreator) {
                            FontCreator.resolution = currentText
                        }
                    }
                }
            }
        }

        // Separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#333333"
        }

        // Main content - font generator UI
        ScrollView {
            anchors.top: headerRow.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            clip: true

            ColumnLayout {
                id: contentColumn
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 24
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                Layout.topMargin: 16

                // Font settings section
                Rectangle {
                    id: fontSettings
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180
                    color: "#1e1e1e"
                    border.color: "#333333"
                    border.width: 1
                    radius: 6

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        Text { text: "FONT SETTINGS"; color: "white"; font.pixelSize: 14; font.bold: true }

                        RowLayout {
                            Text { text: "Size:"; color: "#888"; Layout.preferredWidth: 50 }
                            Text { text: (FontCreator ? FontCreator.fontSize : 32) + "px"; color: "#E10600"; font.pixelSize: 11 }
                        }
                        Slider {
                            Layout.fillWidth: true
                            from: 8; to: 128
                            value: FontCreator ? FontCreator.fontSize : 32
                            onValueChanged: { if (FontCreator) FontCreator.fontSize = value }
                        }

                        Text { text: "UNICODE RANGES"; color: "white"; font.pixelSize: 14; font.bold: true }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#1e1e1e"
                            border.color: "#333"
                            border.width: 1
                            clip: true

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 4

                                Repeater {
                                    model: FontCreator ? FontCreator.getAvailableRanges() : []
                                    delegate: RowLayout {
                                        Layout.fillWidth: true
                                        height: 24
                                        spacing: 6

                                        CheckBox {
                                            id: rangeCheck
                                            checked: {
                                                var ranges = FontCreator ? FontCreator.getEnabledRanges() : []
                                                return ranges.indexOf(modelData) >= 0
                                            }
                                            onCheckedChanged: {
                                                if (FontCreator) FontCreator.enableRange(modelData, checked)
                                            }
                                        }
                                        Text {
                                            text: modelData
                                            color: rangeCheck.checked ? "#E10600" : "#bbb"
                                            font.pixelSize: 11
                                            Layout.fillWidth: true
                                        }
                                    }
                                }

                                Item { Layout.fillHeight: true }

                                RowLayout {
                                    spacing: 6
                                    AppButton {
                                        text: "Apply Ranges"
                                        height: 26
                                        flat: true
                                        bgcolor: "#E10600"
                                        color: "#121212"
                                        onClicked: {
                                            if (FontCreator) FontCreator.applyCombinedCharset()
                                        }
                                    }
                                    AppButton {
                                        text: "Clear"
                                        height: 26
                                        flat: true
                                        bgcolor: "#3e3e42"
                                        color: "#ffffff"
                                        onClicked: {
                                            if (FontCreator) FontCreator.clearRanges()
                                        }
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // Character grid
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#1e1e1e"
                    border.color: "#333333"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 4

                        Text { text "GLYPH SET"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.margins: 8
                            spacing: 6

                            Repeater {
                                model: FontCreator ? FontCreator.getCommonCharsets() : ["Basic Latin (32-126)"]
                                delegate: Rectangle {
                                    Layout.fillWidth: true
                                    height: 28; radius: 4
                                    color: FontCreator && FontCreator.getEnabledRanges().indexOf(modelData) >= 0 ? "#E10600" : "#2d2d2d"
                                    border.color: "#444444"; border.width: 1
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        color: FontCreator && FontCreator.getEnabledRanges().indexOf(modelData) >= 0 ? "#121212" : "#ffffff"
                                        font.pixelSize: 11
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: if (FontCreator) FontCreator.setCharset(modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Bottom stats
                Rectangle {
                    Layout.fillWidth: true
                    height: 80
                    color: "#1e1e1e"
                    border.color: "#333333"
                    border.width: 1

                    RowLayout {
                        anchors { fill: parent; margins: 12 }
                        spacing: 20

                        ColumnLayout {
                            spacing: 4
                            Text { text: "KERNING PREVIEW"; color: "white"; font.pixelSize: 12; font.bold: true }
                            Rectangle {
                                width: 500; height: 50; radius: 4
                                color: "#0a0a0a"; border.color: "#333333"; border.width: 1
                                Row {
                                    anchors.centerIn: parent
                                    spacing: -1
                                    Repeater {
                                        model: "AVATAR"
                                        delegate: Text {
                                            text: modelData
                                            color: "#E10600"
                                            font.pixelSize: 28
                                            font.bold: true
                                            font.family: FontCreator ? FontCreator.currentFont : "Arial"
                                            font.letterSpacing: {
                                                if (index > 0 && FontCreator) {
                                                    var l = FontCreator.getEnabledRanges().indexOf("Basic Latin (32-126)")
                                                    var r = FontCreator.getEnabledRanges().indexOf(modelData)
                                                    return -FontCreator.getKerningOffset("AVATAR"[index-1].charCodeAt(0), modelData.charCodeAt(0)) / 10.0
                                                }
                                                return 0
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            spacing: 6
                            Text { text: "STATS"; color: "#666"; font.pixelSize: 10 }
                            Text { text: "Glyphs: " + (FontCreator ? FontCreator.getGlyphs().length : 95); color: "#bbb"; font.pixelSize: 11 }
                            Text { text: "Kerning: " + (FontCreator ? FontCreator.getKerningPairs().length : 0) + " pairs"; color: "#bbb"; font.pixelSize: 11 }
                            Text { text: "Atlas: " + (FontCreator ? FontCreator.atlasWidth : 4096) + "×" + (FontCreator ? FontCreator.atlasHeight : 64); color: "#bbb"; font.pixelSize: 11 }
                        }
                    }
                }
            }
        }
    }
}