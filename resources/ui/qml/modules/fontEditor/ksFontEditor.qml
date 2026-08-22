import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.0 as QQC2
import "widgets"

ApplicationWindow {
    id: root
    width: 1200
    height: 800
    title: "acFontGenerator"
    visible: true

    // Colori tema
    readonly property color surfaceColor: "#F8F9FA"
    readonly property color borderColor: "#E9ECEF"
    readonly property color textColor: "#212529"
    readonly property color primaryColor: "#0D6EFD"
    readonly property color secondaryColor: "#6C757D"
    readonly property color cardBg: "#1E293B"
    readonly property color cardBorder: "#1F2937"

    // Dati font
    property int fontSize: 35
    property string selectedFont: "Tahoma"
    property string resolution: "1920 x 1080 px"

    // Gruppi di caratteri - computed once at startup
    var allSymbols = ['!', '"', '#', '$', '%', '&', "'", '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~']
    var allNumbers = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
    var allUppercase = []
    var allLowercase = []
    
    for (var i = 65; i <= 90; i++) allUppercase.push(String.fromCharCode(i))
    for (var i = 97; i <= 122; i++) allLowercase.push(String.fromCharCode(i))

    // Modello per i caratteri con i loro valori
    property var charData: (function() {
        var data = []
        var chars = allSymbols.concat(allNumbers).concat(allUppercase).concat(allLowercase)
        chars.forEach(function(ch, idx) {
            data.push({
                char: ch,
                hPad: 0,
                vPad: 0,
                width: ch >= 'A' && ch <= 'Z' ? 28 : 
                       ch >= 'a' && ch <= 'z' ? 24 : 
                       ch >= '0' && ch <= '9' ? 26 : 
                       18 + (allSymbols.indexOf(ch) >= 0 ? allSymbols.indexOf(ch) % 15 : 0)
            });
        });
        return data;
    })()

    // HEADER - Navigation
    ColumnLayout {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0
        z: 10

        // Ribbon Tabs
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            spacing: 0
            Layout.leftMargin: 8
            Layout.rightMargin: 8

            Rectangle {
                color: "transparent"
                Layout.preferredWidth: 70
                Layout.preferredHeight: 28
                border.width: 0
                
                Text {
                    anchors.centerIn: parent
                    text: "File"
                    font.pixelSize: 12
                    font.bold: true
                    color: textColor
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width - 16
                    height: 3
                    color: primaryColor
                }
            }

            Repeater {
                model: ["Edit", "View", "Tools"]
                Rectangle {
                    Layout.preferredWidth: 50
                    Layout.preferredHeight: 28
                    color: "transparent"
                    
                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 12
                        color: secondaryColor
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: parent.color = "#F0F0F0"
                        onExited: parent.color = "transparent"
                    }
                }
            }
            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: borderColor
        }

        // Ribbon Actions
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            spacing: 2
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            Repeater {
                model: [
                    { icon: "note_add", label: "New" },
                    { icon: "folder_open", label: "Open" },
                    { icon: "save", label: "Save" }
                ]
                ColumnLayout {
                    spacing: 2
                    Layout.alignment: Qt.AlignCenter
                    
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.icon
                        font.pixelSize: 22
                        font.family: "Material Symbols Outlined"
                        color: secondaryColor
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.label
                        font.pixelSize: 10
                        color: secondaryColor
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: parent.children[0].color = textColor
                        onExited: parent.children[0].color = secondaryColor
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 32
                color: borderColor
            }

            ColumnLayout {
                spacing: 2
                Layout.alignment: Qt.AlignCenter
                
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "upload"
                    font.pixelSize: 22
                    font.family: "Material Symbols Outlined"
                    color: secondaryColor
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Export"
                    font.pixelSize: 10
                    color: secondaryColor
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.children[0].color = textColor
                    onExited: parent.children[0].color = secondaryColor
                }
            }

            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: borderColor
        }

        // Toolbar
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            spacing: 12
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            Text {
                text: "acFontGenerator"
                font.pixelSize: 14
                font.bold: true
                color: textColor
            }

            Item { Layout.fillWidth: true }

            // Font Selector
            RowLayout {
                spacing: 0
                Rectangle {
                    Layout.preferredHeight: 32
                    color: "#F1F3F5"
                    radius: 4
                    Layout.leftMargin: 1
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Select Font"
                        font.pixelSize: 10
                        font.bold: true
                        color: textColor
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
                        border.color: borderColor
                    }
                    onCurrentTextChanged: selectedFont = currentText
                }
            }

            // Font Size
            RowLayout {
                spacing: 0
                Rectangle {
                    Layout.preferredHeight: 32
                    color: "#F1F3F5"
                    radius: 4
                    
                    Text {
                        anchors.centerIn: parent
                        text: "H"
                        font.pixelSize: 12
                        font.bold: true
                        color: textColor
                        padding: 10
                    }
                }
                Button {
                    text: "<"
                    implicitWidth: 28
                    implicitHeight: 32
                    background: Rectangle {
                        color: "white"
                        border.color: borderColor
                    }
                    onClicked: if(fontSize > 10) fontSize--
                }
                TextField {
                    id: sizeInput
                    implicitWidth: 40
                    implicitHeight: 32
                    text: fontSize.toString()
                    horizontalAlignment: TextInput.AlignHCenter
                    // Use int conversion instead of validator for compatibility
                    text: fontSize.toString()
                    onTextChanged: {
                        var val = parseInt(text)
                        if(!isNaN(val) && val > 0) fontSize = val
                    }
                }
                Button {
                    text: ">"
                    implicitWidth: 28
                    implicitHeight: 32
                    background: Rectangle {
                        color: "white"
                        border.color: borderColor
                    }
                    onClicked: if(fontSize < 200) fontSize++
                }
            }

            // Resolution
            RowLayout {
                spacing: 0
                Rectangle {
                    Layout.preferredHeight: 32
                    color: "#F1F3F5"
                    radius: 4
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Res"
                        font.pixelSize: 10
                        font.bold: true
                        color: textColor
                        padding: 10
                    }
                }
                ComboBox {
                    id: resSelector
                    model: ["1920 x 1080 px", "1280 x 720 px", "1024 x 768 px"]
                    currentIndex: 0
                    implicitWidth: 140
                    background: Rectangle {
                        color: "white"
                        radius: 4
                        border.color: borderColor
                    }
                    onCurrentTextChanged: resolution = currentText
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: borderColor
        }
    }

    // MAIN CONTENT - Scrollable area
    ScrollView {
        id: scrollView
        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        contentWidth: availableWidth
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            id: contentColumn
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 24
            Layout.leftMargin: 24
            Layout.rightMargin: 24
            Layout.topMargin: 16

            // FUNZIONE per creare una sezione
            function createSection(title, charList) {
                return ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: title
                            font.pixelSize: 12
                            font.bold: true
                            color: secondaryColor
                            textFormat: Text.PlainText
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Item { Layout.fillWidth: true }

                        // Sync Controls
                        RowLayout {
                            spacing: 4
                            Rectangle {
                                color: "white"
                                border.color: borderColor
                                radius: 4
                                Layout.preferredHeight: 28
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    spacing: 4
                                    Text {
                                        text: "Sync"
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: secondaryColor
                                        textFormat: Text.PlainText
                                    }
                                    Rectangle {
                                        Layout.preferredWidth: 1
                                        Layout.preferredHeight: 16
                                        color: borderColor
                                    }
                                    Text {
                                        text: "lock"
                                        font.pixelSize: 14
                                        font.family: "Material Symbols Outlined"
                                        color: textColor
                                    }
                                    Text {
                                        text: "H Pad"
                                        font.pixelSize: 9
                                        color: textColor
                                        font.bold: true
                                    }
                                    Text {
                                        text: "lock_open"
                                        font.pixelSize: 14
                                        font.family: "Material Symbols Outlined"
                                        color: secondaryColor
                                    }
                                    Text {
                                        text: "V Pad"
                                        font.pixelSize: 9
                                        color: secondaryColor
                                    }
                                    Text {
                                        text: "lock_open"
                                        font.pixelSize: 14
                                        font.family: "Material Symbols Outlined"
                                        color: secondaryColor
                                    }
                                    Text {
                                        text: "Width"
                                        font.pixelSize: 9
                                        color: secondaryColor
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: borderColor
                    }

                    // Griglia caratteri
                    // Use a fixed column count based on width, computed at layout time
                    GridLayout {
                        id: charGrid
                        Layout.fillWidth: true
                        columns: Math.max(1, Math.floor((scrollView.width - 32) / 180))
                        columnSpacing: 8
                        rowSpacing: 8

                        Repeater {
                            model: charList
                            delegate: CharacterCard {
                                character: modelData
                                fontSize: root.fontSize
                                cardColor: cardBg
                                cardBorder: cardBorder
                            }
                        }
                    }
                }
            }

            // Sezioni
            Loader {
                active: true
                sourceComponent: createSection("1. Symbols & Special Characters", allSymbols)
                Layout.fillWidth: true
            }

            Loader {
                active: true
                sourceComponent: createSection("2. Numbers (0-9)", allNumbers)
                Layout.fillWidth: true
            }

            Loader {
                active: true
                sourceComponent: createSection("3. Uppercase Letters", allUppercase)
                Layout.fillWidth: true
            }

            Loader {
                active: true
                sourceComponent: createSection("4. Lowercase Letters", allLowercase)
                Layout.bottomMargin: 16
            }
        }
    }

    // FOOTER - Bottom Action Bar
    Rectangle {
        id: footer
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 44
        color: "white"
        border.color: borderColor
        z: 20

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Text {
                text: "Project: Untitled_Font_1"
                font.pixelSize: 10
                font.letterSpacing: 0.5
                color: secondaryColor
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 14
                color: borderColor
            }

            Text {
                text: "Glyphs: " + charData.length
                font.pixelSize: 10
                font.letterSpacing: 0.5
                color: secondaryColor
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 14
                color: borderColor
            }

            Text {
                text: "Format: TTF/OTF"
                font.pixelSize: 10
                font.letterSpacing: 0.5
                color: secondaryColor
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                spacing: 4
                Rectangle {
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: 4
                    color: "#22C55E"
                }
                Text {
                    text: "Ready"
                    font.pixelSize: 10
                    font.letterSpacing: 0.5
                    color: secondaryColor
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 14
                color: borderColor
            }

            Text {
                text: "Zoom: 100%"
                font.pixelSize: 10
                font.letterSpacing: 0.5
                color: secondaryColor
            }
        }
    }

    // Componente scheda carattere
    component CharacterCard: Rectangle {
        id: card
        required property string character
        required property int fontSize
        required property color cardColor
        required property color cardBorder

        implicitWidth: 172
        implicitHeight: 72
        radius: 6
        color: cardColor
        border.color: cardBorder
        border.width: 1

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 56
                Layout.fillHeight: true
                color: "black"
                border.color: cardBorder
                border.width: 1
                radius: 6
                Layout.leftMargin: -1

                Text {
                    anchors.centerIn: parent
                    text: character
                    font.pixelSize: 28
                    font.family: "Arial"
                    font.bold: true
                    color: "white"
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 4
                spacing: 2

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 4

                    Text {
                        text: "H Pad"
                        font.pixelSize: 9
                        color: "#9CA3AF"
                        Layout.preferredWidth: 32
                    }

                    RowLayout {
                        spacing: 0
                        Button {
                            text: "<"
                            implicitWidth: 18
                            implicitHeight: 16
                            font.pixelSize: 8
                            background: Rectangle {
                                color: "#2C3035"
                                border.color: "#4B5563"
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font.pixelSize: 8
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        // Use int field instead of TextField with validator for broader compatibility
                        TextField {
                            implicitWidth: 28
                            implicitHeight: 16
                            text: "0"
                            horizontalAlignment: TextInput.AlignHCenter
                            font.pixelSize: 9
                            color: "white"
                            background: Rectangle {
                                color: "black"
                                border.color: "#4B5563"
                            }
                            // Store value in model data when lost focus
                            focusFocusedChanged: {
                                if (!hasFocus && text !== "") {
                                    var val = parseInt(text)
                                    if (!isNaN(val)) model.hPad = val
                                }
                            }
                        }
                        Button {
                            text: ">"
                            implicitWidth: 18
                            implicitHeight: 16
                            font.pixelSize: 8
                            background: Rectangle {
                                color: "#2C3035"
                                border.color: "#4B5563"
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font.pixelSize: 8
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 4

                    Text {
                        text: "V Pad"
                        font.pixelSize: 9
                        color: "#9CA3AF"
                        Layout.preferredWidth: 32
                    }

                    RowLayout {
                        spacing: 0
                        Button {
                            text: "<"
                            implicitWidth: 18
                            implicitHeight: 16
                            font.pixelSize: 8
                            background: Rectangle {
                                color: "#2C3035"
                                border.color: "#4B5563"
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font.pixelSize: 8
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        TextField {
                            implicitWidth: 28
                            implicitHeight: 16
                            text: "0"
                            horizontalAlignment: TextInput.AlignHCenter
                            font.pixelSize: 9
                            color: "white"
                            background: Rectangle {
                                color: "black"
                                border.color: "#4B5563"
                            }
                            focusFocusedChanged: {
                                if (!hasFocus && text !== "") {
                                    var val = parseInt(text)
                                    if (!isNaN(val)) model.vPad = val
                                }
                            }
                        }
                        Button {
                            text: ">"
                            implicitWidth: 18
                            implicitHeight: 16
                            font.pixelSize: 8
                            background: Rectangle {
                                color: "#2C3035"
                                border.color: "#4B5563"
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font.pixelSize: 8
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#374151"
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 4

                    Text {
                        text: "Width"
                        font.pixelSize: 9
                        font.bold: true
                        color: "white"
                        Layout.preferredWidth: 32
                    }

                    RowLayout {
                        spacing: 0
                        Button {
                            text: "<"
                            implicitWidth: 18
                            implicitHeight: 16
                            font.pixelSize: 8
                            background: Rectangle {
                                color: "#2C3035"
                                border.color: "#4B5563"
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font.pixelSize: 8
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        TextField {
                            id: widthField
                            implicitWidth: 28
                            implicitHeight: 16
                            text: function() {
                                var ch = character;
                                if(ch >= 'A' && ch <= 'Z') return "28";
                                if(ch >= 'a' && ch <= 'z') return "24";
                                if(ch >= '0' && ch <= '9') return "26";
                                var idx = allSymbols.indexOf(ch);
                                if(idx >= 0) return String(18 + (idx % 15));
                                return "24";
                            }
                            horizontalAlignment: TextInput.AlignHCenter
                            font.pixelSize: 9
                            font.bold: true
                            color: "white"
                            background: Rectangle {
                                color: "black"
                                border.color: "#4B5563"
                            }
                            // Compute initial value based on character type
                            onTextChanging: {
                                // Keep within reasonable bounds
                                var val = parseInt(text)
                                if (isNaN(val)) text = "24"
                            }
                            // Apply width to model when focus lost
                            focusFocusedChanged: {
                                if (!hasFocus && text !== "") {
                                    var val = parseInt(text)
                                    if (!isNaN(val)) model.width = val
                                }
                            }
                        }
                        Button {
                            text: ">"
                            implicitWidth: 18
                            implicitHeight: 16
                            font.pixelSize: 8
                            background: Rectangle {
                                color: "#2C3035"
                                border.color: "#4B5563"
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font.pixelSize: 8
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
            }
        }
    }
}