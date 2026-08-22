import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.SetupEditor 1.0

Rectangle {
    id: setupEditor
    width: 1280
    height: 720
    color: "#121212"

    property string selectedCar: ""
    property float suspensionHeight: 0
    property float camberAngle: 0
    property float toeAngle: 0
    property float rideHeight: 0

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
                    text: "Car Setup Editor"
                    color: "#E10600"
                    font.pixelSize: 20
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                ComboBox {
                    id: carCombo
                    model: ["Car 1", "Car 2", "Car 3", "Car 4"]
                    preferredWidth: 200
                    onActivated: {
                        selectedCar = modelData
                        // Load setup for selected car
                    }
                }
            }
        }

        // Setup parameters
        Rectangle {
            Layout.fillWidth: true
            height: 200
            color: "#252526"

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 15

                // Suspension height
                RowLayout {
                    spacing: 10

                    Text {
                        text: "Suspension Height"
                        color: "#aaa"
                        font.pixelSize: 14
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Slider {
                        id: suspSlider
                        from: 0
                        to: 100
                        stepSize: 1
                        value: suspensionHeight
                        Layout.fillWidth: true
                        onValueChanged: {
                            // Update suspension
                        }
                    }

                    Text {
                        text: Math.round(suspensionHeight) + "mm"
                        color: "#E10600"
                        font.pixelSize: 12
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: 80
                    }
                }

                // Camber angle
                RowLayout {
                    spacing: 10

                    Text {
                        text: "Camber Angle"
                        color: "#aaa"
                        font.pixelSize: 14
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Slider {
                        id: camberSlider
                        from: -5
                        to: 5
                        stepSize: 0.1
                        value: camberAngle
                        Layout.fillWidth: true
                        onValueChanged: {
                            // Update camber
                        }
                    }

                    Text {
                        text: camberAngle.toFixed(1) + "°"
                        color: "#aaa"
                        font.pixelSize: 12
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: 60
                    }
                }

                // Toe angle
                RowLayout {
                    spacing: 10

                    Text {
                        text: "Toe Angle"
                        color: "#aaa"
                        font.pixelSize: 14
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Slider {
                        id: toeSlider
                        from: -0.5
                        to: 0.5
                        stepSize: 0.01
                        value: toeAngle
                        Layout.fillWidth: true
                        onValueChanged: {
                            // Update toe
                        }
                    }

                    Text {
                        text: toeAngle.toFixed(2) + "°"
                        color: "#aaa"
                        font.pixelSize: 12
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: 60
                    }
                }

                // Ride height
                RowLayout {
                    spacing: 10

                    Text {
                        text: "Ride Height"
                        color: "#aaa"
                        font.pixelSize: 14
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Slider {
                        id: rideSlider
                        from: 0
                        to: 10
                        stepSize: 0.1
                        value: rideHeight
                        Layout.fillWidth: true
                        onValueChanged: {
                            // Update ride height
                        }
                    }

                    Text {
                        text: rideHeight.toFixed(1) + "mm"
                        color: "#aaa"
                        font.pixelSize: 12
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: 60
                    }
                }
            }
        }

        // Setup buttons
        Rectangle {
            Layout.fillWidth: true
            height: 80
            color: "#1e1e1e"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20

                Button {
                    text: "Save Setup"
                    flat: true
                    Layout.fillWidth: true
                    onClicked: {
                        if (selectedCar) {
                            // Save setup logic
                        }
                    }
                }

                Button {
                    text: "Load Setup"
                    flat: true
                    Layout.fillWidth: true
                    onClicked: {
                        if (selectedCar) {
                            // Load setup logic
                        }
                    }
                }

                Button {
                    text: "Reset to Default"
                    flat: true
                    Layout.fillWidth: true
                    onClicked: {
                        suspensionHeight = 0
                        camberAngle = 0
                        toeAngle = 0
                        rideHeight = 0
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