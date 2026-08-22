import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: liveryEditor
    width: 1280
    height: 720
    color: "#121212"

    property string currentLivery: ""
    property string currentCar: ""

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
                    text: "Livery Editor"
                    color: "#E10600"
                    font.pixelSize: 20
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Import"
                    flat: true
                    onClicked: {
                        // Import livery logic
                    }
                }

                Button {
                    text: "Export"
                    flat: true
                    onClicked: {
                        // Export livery logic
                    }
                }
            }
        }

        // Car selector
        Rectangle {
            Layout.fillWidth: true
            height: 80
            color: "#252526"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20

                Text {
                    text: "Car: " + (currentCar || "None selected")
                    color: "#aaa"
                    font.pixelSize: 14
                }

                ComboBox {
                    id: carCombo
                    model: ["Car 1", "Car 2", "Car 3"]
                    preferredWidth: 200
                    onActivated: {
                        currentCar = modelData
                        // Load livery for selected car
                    }
                }
            }
        }

        // Livery preview area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#333333"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20

                Text {
                    text: "Livery Preview"
                    color: "#aaa"
                    font.pixelSize: 14
                    Layout.alignment: Qt.AlignHCenter
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    id: liveryPreview
                    width: 300
                    height: 200
                    color: "#555555"
                    anchors.centerIn: parent
                    visible: currentLivery !== ""
                    Text {
                        anchors.centerIn: parent
                        text: currentLivery || "No livery loaded"
                        color: "#666"
                        font.pixelSize: 12
                    }
                }
            }
        }

        // Actions at bottom
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#1e1e1e"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20

                Button {
                    text: "Apply to Game"
                    flat: true
                    Layout.fillWidth: true
                    onClicked: {
                        // Apply livery to Assetto Corsa
                    }
                }

                Button {
                    text: "Cancel"
                    flat: true
                    Layout.fillWidth: true
                    onClicked: {
                        switchTo("car")
                    }
                }
            }
        }
    }
}