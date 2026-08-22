import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.ShowroomEditor 1.0

Rectangle {
    id: showroomEditor
    width: 1280
    height: 720
    color: "#121212"

    property string selectedCar: ""
    property var showroomCars: []

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
                    text: "Showroom Editor"
                    color: "#E10600"
                    font.pixelSize: 20
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Load Car"
                    flat: true
                    onClicked: {
                        // Load car for showroom
                    }
                }

                Button {
                    text: "Save Scene"
                    flat: true
                    onClicked: {
                        // Save showroom scene
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

                ComboBox {
                    id: carCombo
                    model: showroomCars.map(function(c) { return c.name })
                    preferredWidth: 300
                    onActivated: {
                        selectedCar = showroomCars[index]
                        // Load car into showroom
                    }
                }

                Button {
                    text: "Import"
                    flat: true
                    onClicked: {
                        // Import car to showroom
                    }
                }
            }
        }

        // Showroom preview
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#333333"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 15

                // 3D Viewport placeholder
                Text {
                    id: viewportPlaceholder
                    text: "3D Showroom Preview\n(Car: " + (selectedCar ? selectedCar : "None") + ")"
                    color: "#666"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                // Camera controls
                RowLayout {
                    spacing: 10

                    Button {
                        text: "Reset View"
                        flat: true
                        onClicked: {
                            // Reset camera view
                        }
                    }

                    Button {
                        text: "Capture"
                        flat: true
                        onClicked: {
                            // Capture showroom image
                        }
                    }

                    Button {
                        text: "Export"
                        flat: true
                        onClicked: {
                            // Export showroom scene
                        }
                    }
                }

                // Car list
                Text {
                    text: "Cars in Showroom (" + showroomCars.length + ")"
                    color: "#aaa"
                    font.pixelSize: 12
                }

                Repeater {
                    model: showroomCars

                    delegate: CarListItem {
                        car: modelData
                        onSelect: {
                            selectedCar = modelData
                        }
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
                    text: "Add Car"
                    flat: true
                    Layout.fillWidth: true
                    onClicked: {
                        // Add car to showroom
                    }
                }

                Button {
                    text: "Remove"
                    flat: true
                    Layout.fillWidth: true
                    onClicked: {
                        if (selectedCar) {
                            // Remove car from showroom
                        }
                    }
                }
            }
        }

        // Status bar
        Rectangle {
            Layout.fillWidth: true
            height: 30
            color: "#202020"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text {
                    text: "Ready"
                    color: "#E10600"
                    font.pixelSize: 10
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "ksEditor v1.0"
                    color: "#666"
                    font.pixelSize: 10
                }
            }
        }
    }
}

// Car list item delegate
Rectangle {
    id: CarListItem
    property variant car

    width: 150
    height: 60
    color: "#444444"
    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (onSelect) onSelect()
        }
    }

    Image {
        source: car.thumbnail
        fillMode: Image.PreserveAspectFit
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: 40
        implicitHeight: 40
    }

    Text {
        text: car.name
        color: "#fff"
        font.pixelSize: 12
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: right.left
        anchors.leftMargin: 10
    }

    Text {
        text: car.year + " " + car.model
        color: "#666"
        font.pixelSize: 10
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: right.left
        anchors.leftMargin: 10
    }
}