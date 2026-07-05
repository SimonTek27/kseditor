import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: audioExport
    width: 450
    height: 350
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property string selectedFormat: "WAV"
    property int selectedQuality: 100
    property string outputPath: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15

        Text {
            text: "Export Audio"
            color: "#ffffff"
            font.pixelSize: 16
            font.bold: true
        }

        Item { height: 10 }

        RowLayout {
            Text {
                text: "Format:"
                color: "#aaaaaa"
                width: 80
            }

            ComboBox {
                id: formatCombo
                width: 120
                model: ["WAV", "MP3", "OGG", "FLAC", "AIFF"]
                currentIndex: 0
                onCurrentTextChanged: selectedFormat = currentText
            }
        }

        RowLayout {
            Text {
                text: "Quality:"
                color: "#aaaaaa"
                width: 80
            }

            Slider {
                id: qualitySlider
                width: 150
                from: 0
                to: 100
                value: 100
                onValueChanged: selectedQuality = value
            }

            Text {
                text: selectedQuality + "%"
                color: "#888888"
            }
        }

        RowLayout {
            Text {
                text: "Sample Rate:"
                color: "#aaaaaa"
                width: 80
            }

            ComboBox {
                width: 120
                model: ["44100 Hz", "48000 Hz", "96000 Hz", "192000 Hz"]
                currentIndex: 1
            }
        }

        RowLayout {
            Text {
                text: "Bit Depth:"
                color: "#aaaaaa"
                width: 80
            }

            ComboBox {
                width: 120
                model: ["16-bit", "24-bit", "32-bit float"]
                currentIndex: 1
            }
        }

        RowLayout {
            Text {
                text: "Channels:"
                color: "#aaaaaa"
                width: 80
            }

            ComboBox {
                width: 120
                model: ["Mono", "Stereo", "5.1", "7.1"]
                currentIndex: 1
            }
        }

        RowLayout {
            Text {
                text: "Output:"
                color: "#aaaaaa"
                width: 80
            }

            TextField {
                id: outputPathField
                width: 200
                text: outputPath
                placeholderText: "Select output folder..."
            }

            Button {
                text: "Browse..."
                width: 70
                onClicked: outputPath = "C:/Output"
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                width: 80
            }

            Button {
                text: "Export"
                width: 80
                onClicked: console.log("Export to: " + outputPath + " as " + selectedFormat)
            }
        }
    }
}