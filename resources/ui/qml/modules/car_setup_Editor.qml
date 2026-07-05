import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import ksEditor.SetupEditor 1.0

Rectangle {
    color: "#1a1a1a"

    property string currentSetup: ""
    property bool isDirty: SetupEditor ? SetupEditor.modified : false

    FileDialog {
        id: setupLoadDialog
        title: "Load Setup"
        nameFilters: ["Setup files (*.ini *.json)", "All files (*)"]
        onAccepted: {
            if (SetupEditor) {
                SetupEditor.loadSetup(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: setupSaveDialog
        title: "Save Setup"
        nameFilters: ["Setup files (*.ini)", "JSON files (*.json)", "All files (*)"]
        onAccepted: {
            if (SetupEditor) {
                SetupEditor.saveSetup(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 32
            color: "#252525"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Text { text: "Setup Editor"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                Text { text: isDirty ? "Modified" : "Saved"; color: isDirty ? "#ff6600" : "#888"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }

                Button {
                    text: "Load Setup"
                    flat: true
                    font.pixelSize: 11
                    color: "#aaa"
                    onClicked: setupLoadDialog.open()
                }
                Button {
                    text: "Save Setup"
                    flat: true
                    font.pixelSize: 11
                    color: "#aaa"
                    onClicked: setupSaveDialog.open()
                }
                Button {
                    text: "Reset"
                    flat: true
                    font.pixelSize: 11
                    color: "#aaa"
                    onClicked: {
                        if (SetupEditor) SetupEditor.resetToDefault()
                    }
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Rectangle {
                color: "#1e1e1e"
                SplitView.preferredWidth: 350

                ScrollView {
                    anchors.fill: parent

                    ColumnLayout {
                        width: 330
                        spacing: 8

                        GroupBox {
                            title: "Suspension"
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                RowLayout { Text { text: "Front Bump:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.frontBump : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.frontBump = value } } }
                                RowLayout { Text { text: "Rear Bump:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.rearBump : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.rearBump = value } } }
                                RowLayout { Text { text: "Front Rebound:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.frontRebound : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.frontRebound = value } } }
                                RowLayout { Text { text: "Rear Rebound:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.rearRebound : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.rearRebound = value } } }
                                RowLayout { Text { text: "Front Spring:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.frontSpring : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.frontSpring = value } } }
                                RowLayout { Text { text: "Rear Spring:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.rearSpring : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.rearSpring = value } } }
                                RowLayout { Text { text: "Ride Height:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.rideHeight : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.rideHeight = value } } }
                            }
                        }

                        GroupBox {
                            title: "Aerodynamics"
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                RowLayout { Text { text: "Front Wing:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.frontWing : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.frontWing = value } } }
                                RowLayout { Text { text: "Rear Wing:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.rearWing : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.rearWing = value } } }
                            }
                        }

                        GroupBox {
                            title: "Drivetrain"
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                RowLayout { Text { text: "Preload:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.preload : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.preload = value } } }
                                RowLayout { Text { text: "Fast Bump:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true } }
                                RowLayout { Text { text: "Slow Bump:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true } }
                            }
                        }

                        GroupBox {
                            title: "Brakes"
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                RowLayout { Text { text: "Bias:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.brakeBias : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.brakeBias = value } } }
                                RowLayout { Text { text: "Power:"; color: "#aaa"; width: 100 }
                                    Slider { from: 0; to: 100; value: SetupEditor ? SetupEditor.brakePower : 50; Layout.fillWidth: true; onValueChanged: { if (SetupEditor) SetupEditor.brakePower = value } } }
                            }
                        }
                    }
                }
            }

            Rectangle {
                color: "#1e1e1e"
                SplitView.preferredWidth: 300

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4

                    GroupBox {
                        title: "Setup Presets"
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent
                            Repeater {
                                model: ["Qualifying", "Race", "Wet", "Street Circuit", "High Speed"]
                                Button {
                                    text: modelData
                                    flat: true
                                    Layout.fillWidth: true
                                    font.pixelSize: 12
                                    contentItem: Text { text: parent.text; color: "#ccc"; leftPadding: 12 }
                                    background: Rectangle { color: "#2a2a2a"; radius: 2 }
                                    onClicked: {
                                        if (SetupEditor) SetupEditor.applyPreset(modelData)
                                    }
                                }
                            }
                        }
                    }

                    GroupBox {
                        title: "Performance Estimate"
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent
                            RowLayout { Text { text: "Top Speed:"; color: "#aaa" }
                                Text { text: "285 km/h"; color: "#E10600"; font.bold: true } }
                            RowLayout { Text { text: "Cornering:"; color: "#aaa" }
                                Text { text: "1.8G"; color: "#E10600"; font.bold: true } }
                            RowLayout { Text { text: "Braking:"; color: "#aaa" }
                                Text { text: "2.1G"; color: "#E10600"; font.bold: true } }
                            RowLayout { Text { text: "Est. Lap:"; color: "#aaa" }
                                Text { text: "1:31.5"; color: "#E10600"; font.bold: true } }
                        }
                    }
                }
            }
        }
    }
}
