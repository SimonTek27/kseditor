import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    property bool vrActive: false
    property bool vrAvailable: true

    signal startVRClicked()
    signal stopVRClicked()
    signal resetCameraClicked()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: "VR Viewport Settings"
            font.pixelSize: 18
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#444"
        }

        GroupBox {
            title: "Status"
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: vrActive ? "#4CAF50" : (vrAvailable ? "#FFC107" : "#F44336")
                    }

                    Label {
                        text: vrActive ? "VR Session Active" :
                              vrAvailable ? "Headset Detected - Ready" : "No VR Headset Detected"
                        font.pixelSize: 14
                    }
                }
            }
        }

        GroupBox {
            title: "Controls"
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: 8

                Button {
                    text: vrActive ? "Stop VR" : "Start VR"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    enabled: vrAvailable
                    onClicked: vrActive ? stopVRClicked() : startVRClicked()
                }

                Button {
                    text: "Reset Camera Position"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    enabled: vrActive
                    onClicked: resetCameraClicked()
                }
            }
        }

        GroupBox {
            title: "Controller Input"
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: 6

                Label { text: "Left Thumbstick: Move (forward/back/strafe)" }
                Label { text: "Right Thumbstick: Look around" }
                Label { text: "Trigger: Select / Interact" }
                Label { text: "Grip: Grab" }
                Label { text: "Menu Button: Open menu" }
            }
        }

        GroupBox {
            title: "Performance"
            Layout.fillWidth: true

            ColumnLayout {
                width: parent.width
                spacing: 6

                Label { text: "FPS: --" }
                Label { text: "Resolution: Per-eye native" }
                Label { text: "Tracking: 6-DOF" }
            }
        }
    }
}
