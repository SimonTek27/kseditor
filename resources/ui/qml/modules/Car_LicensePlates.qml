import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../widgets"
import ksEditor.LicensePlates 1.0

Rectangle {
    id: root
    width: 1280
    height: 720
    color: "#121212"

    readonly property color cBg: "#121212"
    readonly property color cSurface: "#1e1e1e"
    readonly property color cBorder: "#333333"
    readonly property color cAccent: "#E10600"
    readonly property color cText: "#ffffff"
    readonly property color cMuted: "#888888"

    property string plateText: LicensePlates ? LicensePlates.plateText : "AB 123 CD"
    property string selectedCountry: LicensePlates ? LicensePlates.country : "IT"
    property string selectedStyle: LicensePlates ? LicensePlates.style : "Standard"
    property color bgColor: "#ffffff"
    property color textColor: "#000000"
    property color borderColor: "#003399"
    property int plateWidth: 520
    property int plateHeight: 110
    property real fontSize: 52
    property string fontStyle: "FE-Schrift"
    property bool showFlag: true
    property bool showEuBand: true
    property string exportFormat: "DDS"
    property string currentFile: "plate.ini"

    FileDialog {
        id: plateOpenDialog
        title: "Open Plate Preset"
        nameFilters: ["Preset files (*.ini *.json)", "All files (*)"]
        onAccepted: {
            if (LicensePlates) {
                LicensePlates.loadPreset(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: plateSaveDialog
        title: "Save Plate Preset"
        nameFilters: ["Preset files (*.ini)", "JSON files (*.json)", "All files (*)"]
        onAccepted: {
            if (LicensePlates) {
                LicensePlates.savePreset(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    readonly property var countries: LicensePlates ? LicensePlates.getCountries() : [
        { code:"IT", name:"Italy", bg:"#ffffff", tc:"#000000", bc:"#0000cc", flag:"IT", fmt:"XX 000 XX", band:"I" },
        { code:"DE", name:"Germany", bg:"#ffffff", tc:"#000000", bc:"#000066", flag:"DE", fmt:"XX-XX 0000", band:"D" },
        { code:"UK", name:"UK", bg:"#ffff00", tc:"#000000", bc:"#000000", flag:"UK", fmt:"XX00 XXX", band:"GB" },
        { code:"FR", name:"France", bg:"#ffffff", tc:"#000000", bc:"#003399", flag:"FR", fmt:"XX-000-XX", band:"F" },
        { code:"ES", name:"Spain", bg:"#ffffff", tc:"#000000", bc:"#003399", flag:"ES", fmt:"0000 XXX", band:"E" },
        { code:"JP", name:"Japan", bg:"#ffffff", tc:"#005aff", bc:"#005aff", flag:"JP", fmt:"00X 00-00", band:"J" },
        { code:"US", name:"USA", bg:"#ffffff", tc:"#bf0a30", bc:"#bf0a30", flag:"USA", fmt:"XXX-0000", band:"USA"}
    ]

    property var currentCountryData: {
        for (var i = 0; i < countries.length; i++)
            if (countries[i].code === selectedCountry) return countries[i];
        return countries[0];
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Toolbar ---
        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10

                AppButton {
                    text: "New"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: {
                        if (LicensePlates) LicensePlates.generatePlate()
                    }
                }
                AppButton {
                    text: "Open"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: plateOpenDialog.open()
                }
                AppButton {
                    text: "Save"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: plateSaveDialog.open()
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: currentFile
                    color: "#aaaaaa"
                    font.pixelSize: 12
                }
            }
        }

        // --- Main Content ---
        RowLayout {
            anchors.fill: parent
            spacing: 0

            // --- Left: controls ---
            Rectangle {
                width: 260
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ScrollView {
                    anchors.fill: parent
                    clip: true

                    ColumnLayout {
                        width: 260
                        spacing: 0

                        // Header
                        Rectangle {
                            height: 44
                            color: "#252526"
                            border.color: "#333333"
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "LICENSE PLATE EDITOR"
                                color: "#E10600"
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }

                        // Country picker
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            Text {
                                text: "COUNTRY"
                                color: "#E10600"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            Flow {
                                Layout.fillWidth: true
                                spacing: 6

                                Repeater {
                                    model: root.countries
                                    delegate: Rectangle {
                                        width: 50
                                        height: 36
                                        radius: 4
                                        color: root.selectedCountry === modelData.code
                                               ? "#E10600" + "33" : "#2d2d2d"
                                        border.color: root.selectedCountry === modelData.code
                                                      ? "#E10600" : "#444444"
                                        border.width: 1

                                        ColumnLayout {
                                            anchors.centerIn: parent
                                            spacing: 2

                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                text: modelData.flag
                                                font.pixelSize: 16
                                                font.bold: true
                                                color: "#ffffff"
                                            }
                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                text: modelData.code
                                                color: "#ffffff"
                                                font.pixelSize: 9
                                                font.bold: true
                                            }
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                root.selectedCountry = modelData.code
                                                if (LicensePlates) {
                                                    LicensePlates.setCountry(modelData.code)
                                                    root.bgColor = modelData.bg
                                                    root.textColor = modelData.tc
                                                    root.borderColor = modelData.bc
                                                    LicensePlates.setBgColor(modelData.bg)
                                                    LicensePlates.setTextColor(modelData.tc)
                                                    LicensePlates.setBorderColor(modelData.bc)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle { height: 1; color: "#333333" }

                        // Plate text
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            Text {
                                text: "PLATE TEXT"
                                color: "#E10600"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            Rectangle {
                                height: 40
                                radius: 4
                                color: "#252526"
                                border.color: "#E10600"
                                border.width: 1

                                TextInput {
                                    anchors { fill: parent; margins: 10 }
                                    text: root.plateText
                                    color: "#ffffff"
                                    font.pixelSize: 18
                                    font.bold: true
                                    font.family: "Courier"
                                    onTextChanged: {
                                        root.plateText = text
                                        if (LicensePlates) LicensePlates.setPlateText(text)
                                    }
                                }
                            }

                            Text {
                                text: "Format: " + root.currentCountryData.fmt
                                color: "#888888"
                                font.pixelSize: 11
                            }
                        }

                        Rectangle { height: 1; color: "#333333" }

                        // Style options
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 6

                            Text {
                                text: "STYLE"
                                color: "#E10600"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            AppButton {
                                height: 28
                                text: "Standard"
                                bgcolor: selectedStyle === "Standard" ? "#E10600" : "#3e3e42"
                                color: selectedStyle === "Standard" ? "#121212" : "#ffffff"
                                onClicked: {
                                    root.selectedStyle = "Standard"
                                    if (LicensePlates) LicensePlates.setStyle("Standard")
                                }
                            }
                            AppButton {
                                height: 28
                                text: "Vintage"
                                bgcolor: selectedStyle === "Vintage" ? "#E10600" : "#3e3e42"
                                color: selectedStyle === "Vintage" ? "#121212" : "#ffffff"
                                onClicked: {
                                    root.selectedStyle = "Vintage"
                                    if (LicensePlates) LicensePlates.setStyle("Vintage")
                                }
                            }
                            AppButton {
                                height: 28
                                text: "Embossed"
                                bgcolor: selectedStyle === "Embossed" ? "#E10600" : "#3e3e42"
                                color: selectedStyle === "Embossed" ? "#121212" : "#ffffff"
                                onClicked: {
                                    root.selectedStyle = "Embossed"
                                    if (LicensePlates) LicensePlates.setStyle("Embossed")
                                }
                            }
                        }

                        Rectangle { height: 1; color: "#333333" }

                        // Colours
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            Text {
                                text: "COLOURS"
                                color: "#E10600"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            RowLayout {
                                Text { text: "Background:"; color: "#bbbbbb"; Layout.fillWidth: true }
                                Rectangle {
                                    width: 24; height: 24; radius: 4; color: root.bgColor; border.color: "#444444"; border.width: 1
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            var dialog = Qt.createComponent(Qt.resolvedUrl("QtQuick/Dialogs/ColorDialog.qml"))
                                            if (dialog.status === Component.Ready) {
                                                // Use a simple approach - cycle through common colors
                                                var colors = ["#ffffff", "#ffff00", "#000000", "#cccccc", "#ff0000"]
                                                var idx = colors.indexOf(root.bgColor)
                                                root.bgColor = colors[(idx + 1) % colors.length]
                                                if (LicensePlates) LicensePlates.setBgColor(root.bgColor)
                                            }
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Text { text: "Text:"; color: "#bbbbbb"; Layout.fillWidth: true }
                                Rectangle {
                                    width: 24; height: 24; radius: 4; color: root.textColor; border.color: "#444444"; border.width: 1
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            var colors = ["#000000", "#ffffff", "#bf0a30", "#005aff", "#003399"]
                                            var idx = colors.indexOf(root.textColor)
                                            root.textColor = colors[(idx + 1) % colors.length]
                                            if (LicensePlates) LicensePlates.setTextColor(root.textColor)
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                Text { text: "Border:"; color: "#bbbbbb"; Layout.fillWidth: true }
                                Rectangle {
                                    width: 24; height: 24; radius: 4; color: root.borderColor; border.color: "#444444"; border.width: 1
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            var colors = ["#000000", "#003399", "#0000cc", "#005aff", "#bf0a30"]
                                            var idx = colors.indexOf(root.borderColor)
                                            root.borderColor = colors[(idx + 1) % colors.length]
                                            if (LicensePlates) LicensePlates.setBorderColor(root.borderColor)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Centre: preview + batch
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Toolbar
                Rectangle {
                    height: 44
                    color: "#1e1e1e"
                    border.color: "#333333"
                    border.width: 1

                    RowLayout {
                        anchors { fill: parent; margins: 12 }

                        Text {
                            text: "Preview — " + root.selectedCountry + " / " + root.selectedStyle
                            color: "white"
                            font.bold: true
                            font.pixelSize: 13
                        }

                        Item { Layout.fillWidth: true }

                        AppButton {
                            height: 28
                            text: "DDS"
                            bgcolor: exportFormat === "DDS" ? "#E10600" : "transparent"
                            color: exportFormat === "DDS" ? "#121212" : "#ffffff"
                            onClicked: root.exportFormat = "DDS"
                        }
                        AppButton {
                            height: 28
                            text: "PNG"
                            bgcolor: exportFormat === "PNG" ? "#E10600" : "transparent"
                            color: exportFormat === "PNG" ? "#121212" : "#ffffff"
                            onClicked: root.exportFormat = "PNG"
                        }
                        AppButton {
                            height: 28
                            text: "TGA"
                            bgcolor: exportFormat === "TGA" ? "#E10600" : "transparent"
                            color: exportFormat === "TGA" ? "#121212" : "#ffffff"
                            onClicked: root.exportFormat = "TGA"
                        }

                        AppButton {
                            height: 32
                            text: "Export"
                            bgcolor: "#E10600"
                            color: "#121212"
                            onClicked: {
                                if (LicensePlates) {
                                    LicensePlates.exportPlate(Qt.resolvedUrl("plate_" + root.selectedCountry + "." + root.exportFormat.toLowerCase()))
                                }
                            }
                        }
                    }
                }

                // Live plate preview
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#2a2a2a"

                    Rectangle {
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 40, root.plateWidth)
                        height: width * (root.plateHeight / root.plateWidth)
                        radius: 8
                        color: root.bgColor

                        // EU band
                        Rectangle {
                            visible: root.showEuBand
                            x: 0; y: 0
                            width: parent.width * 0.1
                            height: parent.height
                            radius: 7
                            color: root.borderColor

                            Text {
                                anchors.centerIn: parent
                                text: root.currentCountryData.flag
                                font.pixelSize: Math.min(parent.height * 0.6, 28)
                                font.bold: true
                                color: "#ffffff"
                            }
                        }

                        // Border
                        Rectangle {
                            anchors.fill: parent
                            radius: 8
                            color: "transparent"
                            border.color: root.borderColor
                            border.width: Math.max(2, parent.height * 0.05)
                        }

                        // Plate text
                        Text {
                            anchors.centerIn: parent
                            anchors.horizontalCenterOffset: root.showEuBand ? parent.width * 0.05 : 0
                            text: root.plateText
                            color: root.textColor
                            font.pixelSize: Math.min(parent.height * 0.55, root.fontSize * (parent.width / root.plateWidth))
                            font.bold: true
                            font.family: "Courier"
                        }
                    }
                }

                Rectangle { height: 1; color: "#333333" }

                // Batch generator
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    anchors.margins: 15
                    spacing: 12

                    RowLayout {
                        Text { text: "Batch Generator"; color: "white"; font.bold: true; font.pixelSize: 14 }
                        Item { Layout.fillWidth: true }
                        AppButton {
                            height: 28
                            text: "Generate All Countries"
                            bgcolor: "transparent"
                            color: "#ffffff"
                            onClicked: {
                                if (LicensePlates) {
                                    var texts = []
                                    for (var i = 0; i < root.countries.length; i++) {
                                        texts.push(root.plateText)
                                    }
                                    LicensePlates.exportBatch(Qt.resolvedUrl("plates/"), texts)
                                }
                            }
                        }
                    }

                    // Batch plate previews strip
                    RowLayout {
                        spacing: 12

                        Repeater {
                            model: root.countries
                            delegate: Rectangle {
                                width: 120
                                height: 40
                                radius: 4
                                color: modelData.bg
                                border.color: modelData.bc
                                border.width: 2

                                Text {
                                    anchors.centerIn: parent
                                    text: root.plateText
                                    color: modelData.tc
                                    font.pixelSize: 12
                                    font.bold: true
                                    font.family: "Courier"
                                }
                            }
                        }
                    }
                }
            }

            // Right: presets
            Rectangle {
                width: 200
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors { fill: parent; margins: 15 }
                    spacing: 12

                    Text {
                        text: "PRESETS"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 14
                    }

                    AppButton {
                        height: 32
                        text: "Save Preset"
                        bgcolor: "#E10600"
                        color: "#121212"
                        onClicked: {
                            if (LicensePlates) {
                                LicensePlates.savePreset("preset_" + root.selectedCountry + "_" + root.selectedStyle)
                            }
                        }
                    }

                    Rectangle { height: 5; color: "transparent" }

                    AppButton {
                        height: 40
                        text: "EU Standard"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        onClicked: {
                            if (LicensePlates) LicensePlates.loadPreset("EU Standard")
                            root.selectedCountry = "IT"
                            root.selectedStyle = "Standard"
                        }
                    }
                    AppButton {
                        height: 40
                        text: "German Black"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        onClicked: {
                            if (LicensePlates) LicensePlates.loadPreset("German Black")
                            root.selectedCountry = "DE"
                            root.selectedStyle = "Standard"
                        }
                    }
                    AppButton {
                        height: 40
                        text: "UK Yellow"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        onClicked: {
                            if (LicensePlates) LicensePlates.loadPreset("UK Yellow")
                            root.selectedCountry = "UK"
                            root.selectedStyle = "Standard"
                        }
                    }
                    AppButton {
                        height: 40
                        text: "JDM White"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        onClicked: {
                            if (LicensePlates) LicensePlates.loadPreset("JDM White")
                            root.selectedCountry = "JP"
                            root.selectedStyle = "Standard"
                        }
                    }
                    AppButton {
                        height: 40
                        text: "US Retro"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        onClicked: {
                            if (LicensePlates) LicensePlates.loadPreset("US Retro")
                            root.selectedCountry = "US"
                            root.selectedStyle = "Vintage"
                        }
                    }

                    Item { Layout.fillHeight: true }

                    Text {
                        text: "EXPORT SETTINGS"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 12
                    }

                    Text { text: "Resolution: 512×128"; color: "#bbbbbb"; font.pixelSize: 11 }
                    Text { text: "Format: " + exportFormat; color: "#bbbbbb"; font.pixelSize: 11 }
                    Text { text: "Compression: BC1"; color: "#bbbbbb"; font.pixelSize: 11 }

                    AppButton {
                        height: 36
                        text: "Batch Export All"
                        bgcolor: "#E10600"
                        color: "#121212"
                        onClicked: {
                            if (LicensePlates) {
                                var texts = []
                                for (var i = 0; i < root.countries.length; i++) {
                                    texts.push(root.plateText)
                                }
                                LicensePlates.exportBatch(Qt.resolvedUrl("batch_plates/"), texts)
                            }
                        }
                    }
                }
            }
        }

        // --- Status Bar ---
        Rectangle {
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text { text: "Ready"; color: "#E10600"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "ksEditor v1.0 - License Plates"; color: "#666"; font.pixelSize: 10 }
            }
        }
    }
}
