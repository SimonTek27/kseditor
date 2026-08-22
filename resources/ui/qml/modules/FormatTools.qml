import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ksEditor.FormatTools 1.0

Rectangle {
    id: formatTools
    width: 1280
    height: 720
    color: "#121212"

    property string statusText: ""
    property int importedPoints: 0
    property var importedPointsData: []
    property var importedVertices: []
    property var importedCameras: []
    property var componentTree: ({})

    // Keyboard shortcuts
    Shortcut { sequence: "Ctrl+I"; onActivated: aiOpenDialog.open(); description: "Import AI Line" }
    Shortcut { sequence: "Ctrl+E"; onActivated: aiSaveDialog.open(); description: "Export AI Line" }
    Shortcut { sequence: "Ctrl+Shift+I"; onActivated: csvOpenDialog.open(); description: "Import CSV" }
    Shortcut { sequence: "Ctrl+Shift+E"; onActivated: csvSaveDialog.open(); description: "Export CSV" }
    Shortcut { sequence: "Ctrl+L"; onActivated: FormatTools.loadComponentTree ""; description: "Load Component Tree" }
    Shortcut { sequence: "Ctrl+S"; onActivated: FormatTools.saveComponentTree ""; description: "Save Component Tree" }
    Shortcut { sequence: "Ctrl+P"; onActivated: FormatTools.createCarProject("", "new_car", {}); description: "Create Car Project" }
    Shortcut { sequence: "Ctrl+T"; onActivated: FormatTools.createTrackProject("", "new_track", {}); description: "Create Track Project" }
    Shortcut { sequence: "F5"; onActivated: refreshTree(); description: "Refresh Tree" }

    FileDialog {
        id: aiOpenDialog
        title: "Import AI Line"
        nameFilters: ["AI files (*.ai)", "All files (*)"]
        onAccepted: {
            selectedFile = selectedFile.toString()
        }
    }

    FileDialog {
        id: csvOpenDialog
        title: "Select CSV File"
        nameFilters: ["CSV files (*.csv)", "All files (*)"]
        onAccepted: {
            selectedFile = selectedFile.toString()
        }
    }

    // Source file selection
    FileDialog {
        id: fileOpenDialog
        title: "Select Source File"
        nameFilters: ["All files (*)"]
        onAccepted: {
            selectedFile = selectedFile.toString()
        }
    }

    // Conversion result display
    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        // Header
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#1e1e1e"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20

                Text {
                    text: "Format Tools"
                    color: "#E10600"
                    font.pixelSize: 20
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                ComboBox {
                    id: formatCombo
                    model: ["CSP", "AC1", "EVO", "KN5", "FBX", "LUA"]
                    preferredWidth: 200
                    onActivated: {
                        currentFormat = modelData
                    }
                }
            }
        }

        // Conversion options
        Rectangle {
            Layout.fillWidth: true
            height: 200
            color: "#252526"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 15

                // Source file
                RowLayout {
                    spacing: 10

                    Text {
                        text: "Source File:"
                        color: "#aaa"
                        font.pixelSize: 14
                    }

                    Button {
                        text: "Browse..."
                        flat: true
                        onClicked: {
                            fileOpenDialog.open()
                        }
                    }

                    Text {
                        text: selectedFile ? selectedFile.split("/").pop() : "None"
                        color: "#666"
                        font.pixelSize: 12
                        implicitWidth: 150
                    }
                }

                // Target format settings
                Repeater {
                    model: formatConverters[currentFormat] || []

                    delegate: FormatOption {
                        format: currentFormat
                    }
                }
            }
        }

        // Conversion result
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#333333"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 15

                Text {
                    text: "Conversion Result"
                    color: "#E10600"
                    font.pixelSize: 16
                    font.bold: true
                }

                Item { height: 10 }

                Text {
                    text: conversionResult.status || "No conversion performed"
                    color: "#aaa"
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                }

                Item { height: 10 }

                if (conversionResult.success) {
                    Button {
                        text: "Open Output Folder"
                        flat: true
                        onClicked: {
                            // Open output folder
                        }
                    }
                } else {
                    Text {
                        text: "Error: " + (conversionResult.error || "Unknown error")
                        color: "#f44336"
                        font.pixelSize: 12
                    }
                }
            }
        }

        // Action buttons
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#1e1e1e"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20

                Button {
                    text: "Convert"
                    flat: true
                    Layout.fillWidth: true
                    onClicked: {
                        if (selectedFile && currentFormat) {
                            // Perform conversion
                        }
                    }
                }

                Button {
                    text: "Cancel"
                    flat: true
                    Layout.fillWidth: true
                    onClicked: {
                        // switchTo("home")
                    }
                }
            }
        }
    }

    // Format option delegate
    Rectangle {
        id: FormatOption
        property string format

        width: 200
        height: 40
        color: "#444444"
        MouseArea {
            anchors.fill: parent
            onClicked: {
                // Handle format option selection
            }
        }

        Text {
            text: "Option for " + format
            color: "#fff"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}