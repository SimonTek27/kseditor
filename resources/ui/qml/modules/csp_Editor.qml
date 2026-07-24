import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../widgets"
import ksEditor.CspConfig 1.0

Rectangle {
    id: root
    width: 1280
    height: 720
    color: "#121212"

    readonly property color cBg: "#121212"
    readonly property color cSurface: "#1e1e1e"
    readonly property color cSurface2: "#252526"
    readonly property color cBorder: "#333333"
    readonly property color cAccent: "#E10600"
    readonly property color cText: "#ffffff"
    readonly property color cMuted: "#888888"

    property var configData: ({})
    property var sectionKeys: []
    property string currentSection: ""
    property string currentFilePath: ""
    property string configType: "car"

    function refreshSectionList() {
        sectionKeys = CspConfig.getSectionNames(configData)
    }

    function loadFile(path) {
        configData = CspConfig.loadFile(path)
        currentFilePath = path
        refreshSectionList()
    }

    FileDialog {
        id: openDialog
        title: "Open CSP Config"
        nameFilters: ["CSP config (*.ini)", "All files (*)"]
        onAccepted: {
            loadFile(selectedFile.toString().replace("file:///", ""))
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save CSP Config"
        nameFilters: ["CSP config (*.ini)", "All files (*)"]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            var ok = CspConfig.saveFile(path, configData)
            if (ok) currentFilePath = path
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Toolbar ──────────────────────────────────────────────────────
        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                spacing: 8

                Text { text: "CSP CONFIG EDITOR"; color: "white"; font.pixelSize: 14; font.bold: true }

                Rectangle { width: 1; height: 20; color: "#444" }

                AppButton {
                    text: "Open"; height: 28
                    bgcolor: "transparent"; color: "#ffffff"
                    onClicked: openDialog.open()
                }
                AppButton {
                    text: "Save"; height: 28
                    bgcolor: currentFilePath ? "#E10600" : "transparent"
                    color: "#ffffff"
                    enabled: currentFilePath !== ""
                    onClicked: {
                        if (currentFilePath)
                            CspConfig.saveFile(currentFilePath, configData)
                    }
                }
                AppButton {
                    text: "Save As"; height: 28
                    bgcolor: "transparent"; color: "#ffffff"
                    enabled: Object.keys(configData).length > 0
                    onClicked: saveDialog.open()
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    height: 24
                    color: "#252526"
                    border.color: "#444"
                    border.width: 1
                    radius: 3
                    visible: currentFilePath !== ""

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4
                        Text {
                            text: {
                                if (currentFilePath.length > 55)
                                    return "..." + currentFilePath.slice(-52)
                                return currentFilePath
                            }
                            color: "#aaa"; font.pixelSize: 9; font.family: "monospace"
                        }
                    }
                }
            }
        }

        // ── Main Content ─────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ── Left: Section List ───────────────────────────────────────
            Rectangle {
                width: 220
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        height: 36
                        color: "#252526"
                        Layout.fillWidth: true
                        Text {
                            anchors.centerIn: parent
                            text: "SECTIONS (" + sectionKeys.length + ")"
                            color: "#E10600"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ListView {
                            id: sectionList
                            model: sectionKeys
                            anchors.margins: 4
                            spacing: 2

                            delegate: Rectangle {
                                width: parent ? parent.width : 200
                                height: 28
                                radius: 4
                                color: currentSection === modelData ? "#E1060033" : "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    spacing: 4

                                    Rectangle {
                                        width: 4; height: 16; radius: 2
                                        color: {
                                            var t = CspConfig.getSectionType(modelData)
                                            if (t === "EMISSIVE") return "#ff6600"
                                            if (t === "LIGHT" || t === "LIGHT_SERIES") return "#ffcc00"
                                            if (t === "MATERIAL_ADJUSTMENT") return "#a855f7"
                                            if (t === "CONDITION") return "#3b82f6"
                                            if (t === "BRAKEDISC") return "#ef4444"
                                            return "#666"
                                        }
                                    }

                                    Text {
                                        text: modelData
                                        color: currentSection === modelData ? "#ffffff" : "#cccccc"
                                        font.pixelSize: 10
                                        font.bold: currentSection === modelData
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: currentSection = modelData
                                }
                            }
                        }
                    }

                    // Add section AppButton
                    Rectangle {
                        height: 36
                        color: "#252526"
                        Layout.fillWidth: true

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 4

                            AppButton {
                                text: "+ Add Section"; height: 28
                                bgcolor: "#E10600"; color: "#121212"
                                Layout.fillWidth: true
                                font.pixelSize: 10
                                onClicked: addMenu.popup()
                            }

                            Menu {
                                id: addMenu
                                Repeater {
                                    model: CspConfig.getSectionTypes()
                                    MenuItem {
                                        text: modelData
                                        onClicked: {
                                            configData = CspConfig.addSection(configData, modelData)
                                            refreshSectionList()
                                        }
                                    }
                                }
                            }

                            AppButton {
                                text: "Remove"; height: 28
                                bgcolor: "transparent"; color: "#ff4444"
                                enabled: currentSection !== ""
                                font.pixelSize: 10
                                onClicked: {
                                    configData = CspConfig.removeSection(configData, currentSection)
                                    refreshSectionList()
                                    currentSection = ""
                                }
                            }
                        }
                    }
                }
            }

            // ── Center: Section Editor ───────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#121212"

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 10
                    clip: true

                    ColumnLayout {
                        width: parent ? parent.width - 20 : 500
                        spacing: 8

                        // Section header
                        Text {
                            text: currentSection ? "EDIT: " + currentSection : "Select a section"
                            color: "#ffffff"
                            font.pixelSize: 14
                            font.bold: true
                            visible: currentSection !== ""
                        }

                        Text {
                            text: currentSection ? "Type: " + CspConfig.getSectionType(currentSection) : ""
                            color: "#E10600"
                            font.pixelSize: 11
                            visible: currentSection !== ""
                        }

                        Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                        // Section properties
                        Repeater {
                            model: currentSection ? CspConfig.getSectionPropertyKeys(currentSection, configData) : []

                            Rectangle {
                                width: parent ? parent.width : 480
                                height: 40
                                radius: 4
                                color: "#1e1e1e"
                                border.color: "#333"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 8

                                    Text {
                                        text: modelData
                                        color: "#aaa"
                                        font.pixelSize: 10
                                        font.bold: true
                                        font.family: "monospace"
                                        Layout.preferredWidth: 160
                                    }

                                    TextField {
                                        id: propField
                                        text: {
                                            var s = configData[currentSection]
                                            return s ? (s[modelData] || "") : ""
                                        }
                                        Layout.fillWidth: true
                                        height: 28
                                        color: "#ffffff"
                                        font.pixelSize: 11
                                        font.family: "monospace"
                                        background: Rectangle {
                                            color: "#252526"
                                            radius: 3
                                            border.color: "#444"
                                            border.width: 1
                                        }
                                        onEditingFinished: {
                                            var s = configData[currentSection]
                                            if (s) {
                                                s[modelData] = text
                                                configData = CspConfig.updateSection(configData, currentSection, s)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Empty state
                        Text {
                            text: currentSection ? "No properties — select a section with content" : "Open a CSP config file to begin editing"
                            color: "#666"
                            font.pixelSize: 11
                            visible: currentSection !== "" && CspConfig.getSectionPropertyKeys(currentSection, configData).length === 0
                        }

                        Item { height: 10 }
                    }
                }
            }

            // ── Right: Quick Actions ──────────────────────────────────────
            Rectangle {
                width: 200
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Text { text: "ACTIONS"; color: "#E10600"; font.pixelSize: 11; font.bold: true }

                    AppButton {
                        text: "New Car Config"; height: 28
                        bgcolor: "transparent"; color: "#ffffff"
                        Layout.fillWidth: true
                        font.pixelSize: 10
                        onClicked: {
                            configData = CspConfig.createDefaultCarConfig()
                            currentFilePath = ""
                            refreshSectionList()
                        }
                    }

                    AppButton {
                        text: "New Track Config"; height: 28
                        bgcolor: "transparent"; color: "#ffffff"
                        Layout.fillWidth: true
                        font.pixelSize: 10
                        onClicked: {
                            configData = CspConfig.createDefaultTrackConfig()
                            currentFilePath = ""
                            refreshSectionList()
                        }
                    }

                    Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

                    Text { text: "HELP"; color: "#888"; font.pixelSize: 10; font.bold: true }
                    Text {
                        text: "CSP configs use sections like [EMISSIVE_0], [LIGHT_HEADLIGHTS], [CONDITION_BRAKE], etc. Each section has KEY=VALUE pairs."
                        color: "#666"
                        font.pixelSize: 9
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Text {
                        text: "Common EMISSIVE keys:\nNAME, MESHES, MATERIALS, COLOR, COLOR_OFF, ACTIVE, LAG, LOCATION"
                        color: "#555"
                        font.pixelSize: 9
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        visible: currentSection ? CspConfig.getSectionType(currentSection) === "EMISSIVE" : false
                    }

                    Text {
                        text: "Common LIGHT keys:\nMESHES, MATERIALS, COLOR, DIRECTION, SPOT, RANGE, ACTIVE"
                        color: "#555"
                        font.pixelSize: 9
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        visible: currentSection ? CspConfig.getSectionType(currentSection) === "LIGHT" || CspConfig.getSectionType(currentSection) === "LIGHT_SERIES" : false
                    }

                    Text {
                        text: "Condition INPUT values:\nONE, NONE, TIME, AMBIENT, FOG, SUN, SPEED, RPM, GEAR, HEADLIGHTS, BRAKING"
                        color: "#555"
                        font.pixelSize: 9
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        visible: currentSection ? CspConfig.getSectionType(currentSection) === "CONDITION" : false
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // ── Status Bar ──────────────────────────────────────────────────
        Rectangle {
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text {
                    text: currentFilePath ? "Loaded: " + currentFilePath : "No file open"
                    color: currentFilePath ? "#10b981" : "#888"
                    font.pixelSize: 9
                    font.family: "monospace"
                    elide: Text.ElideLeft
                    Layout.fillWidth: true
                }

                Text {
                    text: sectionKeys.length + " sections"
                    color: "#888"
                    font.pixelSize: 10
                }
            }
        }
    }

    Connections {
        target: CspConfig
        function onConfigSaved(path, success) {
            if (success) currentFilePath = path
        }
    }
}


