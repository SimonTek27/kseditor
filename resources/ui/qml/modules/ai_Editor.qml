import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs
import ksEditor.AIEditor 1.0

Rectangle {
    color: "#1a1a1a"

    property string currentFile: AIEditor ? AIEditor.currentFile : ""
    property int waypointCount: AIEditor ? AIEditor.waypointCount : 0

    FileDialog {
        id: aiOpenDialog
        title: "Open AI Line"
        nameFilters: ["AI files (*.ai *.ini *.json)", "All files (*)"]
        onAccepted: {
            if (AIEditor) {
                AIEditor.openAILine(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: aiSaveDialog
        title: "Save AI Line"
        nameFilters: ["AI files (*.ai)", "INI files (*.ini)", "JSON files (*.json)", "All files (*)"]
        onAccepted: {
            if (AIEditor) {
                AIEditor.saveAILine(selectedFile.toString().replace("file:///", ""))
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

                Text { text: "AI Editor"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                Text { text: (AIEditor ? AIEditor.waypointCount : 0) + " waypoints"; color: "#888"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }

                Button {
                    text: "Open AI Line"
                    flat: true
                    font.pixelSize: 11
                    onClicked: aiOpenDialog.open()
                }
                Button {
                    text: "Save AI Line"
                    flat: true
                    font.pixelSize: 11
                    onClicked: aiSaveDialog.open()
                }
                Button {
                    text: "Auto Compute"
                    flat: true
                    font.pixelSize: 11
                    onClicked: {
                        if (AIEditor) AIEditor.autoComputeBrakePoints()
                    }
                }
                Button {
                    text: "Profile Editor"
                    flat: true
                    font.pixelSize: 11
                    onClicked: {
                        if (AIEditor) AIEditor.analyzeLine("")
                        statusMessage = "Opening AI profile editor..."
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
                SplitView.preferredWidth: 600

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        height: 28
                        color: "#2a2a2a"
                        Layout.fillWidth: true
                        Text { text: "Waypoint List"; color: "#888"; font.pixelSize: 11; anchors.centerIn: parent }
                    }

                    ListView {
                        id: waypointList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: AIEditor ? AIEditor.waypoints : []
                        currentIndex: AIEditor ? AIEditor.selectedWaypointIndex : -1
                        delegate: Rectangle {
                            width: parent.width
                            height: 24
                            color: ListView.isCurrentItem ? "#3a3a3a" : (index % 2 === 0 ? "#252525" : "#1e1e1e")
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                Text { text: "#" + index; color: "#E10600"; font.pixelSize: 10; width: 24 }
                                Text { text: "X:" + modelData.x.toFixed(1); color: "#aaa"; font.pixelSize: 10; width: 60 }
                                Text { text: "Y:" + modelData.y.toFixed(1); color: "#aaa"; font.pixelSize: 10; width: 60 }
                                Text { text: "Z:" + modelData.z.toFixed(1); color: "#aaa"; font.pixelSize: 10; width: 60 }
                                Text { text: modelData.speed + " km/h"; color: "#4a4"; font.pixelSize: 10 }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    waypointList.currentIndex = index
                                    if (AIEditor) AIEditor.selectedWaypointIndex = index
                                }
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
                        title: "Waypoint Details"
                        Layout.fillWidth: true
                        ColumnLayout {
                            id: wpDetailsGroup
                            anchors.fill: parent

                            RowLayout {
                                Text { text: "X:"; color: "#aaa"; width: 30 }
                                SpinBox {
                                    id: wpX; value: 0; from: -10000; to: 10000
                                    Layout.fillWidth: true
                                    onValueModified: {
                                        if (AIEditor && AIEditor.selectedWaypointIndex >= 0)
                                            AIEditor.updateWaypoint(AIEditor.selectedWaypointIndex, value, wpY.value, wpZ.value, wpSpeed.value)
                                    }
                                }
                            }
                            RowLayout {
                                Text { text: "Y:"; color: "#aaa"; width: 30 }
                                SpinBox {
                                    id: wpY; value: 0; from: -10000; to: 10000
                                    Layout.fillWidth: true
                                    onValueModified: {
                                        if (AIEditor && AIEditor.selectedWaypointIndex >= 0)
                                            AIEditor.updateWaypoint(AIEditor.selectedWaypointIndex, wpX.value, value, wpZ.value, wpSpeed.value)
                                    }
                                }
                            }
                            RowLayout {
                                Text { text: "Z:"; color: "#aaa"; width: 30 }
                                SpinBox {
                                    id: wpZ; value: 0; from: -10000; to: 10000
                                    Layout.fillWidth: true
                                    onValueModified: {
                                        if (AIEditor && AIEditor.selectedWaypointIndex >= 0)
                                            AIEditor.updateWaypoint(AIEditor.selectedWaypointIndex, wpX.value, wpY.value, value, wpSpeed.value)
                                    }
                                }
                            }
                            RowLayout {
                                Text { text: "Speed:"; color: "#aaa"; width: 50 }
                                SpinBox {
                                    id: wpSpeed; value: 100; from: 0; to: 400
                                    Layout.fillWidth: true
                                    onValueModified: {
                                        if (AIEditor && AIEditor.selectedWaypointIndex >= 0)
                                            AIEditor.updateWaypoint(AIEditor.selectedWaypointIndex, wpX.value, wpY.value, wpZ.value, value)
                                    }
                                }
                            }
                        }

                        Connections {
                            target: AIEditor
                            function onSelectedWaypointIndexChanged() {
                                var idx = AIEditor.selectedWaypointIndex
                                if (idx >= 0 && idx < AIEditor.waypointCount) {
                                    var wp = AIEditor.waypoints[idx]
                                    wpX.value = wp.x
                                    wpY.value = wp.y
                                    wpZ.value = wp.z
                                    wpSpeed.value = wp.speed
                                }
                            }
                        }
                    }

                    GroupBox {
                        title: "Driver Profile"
                        Layout.fillWidth: true
                        ColumnLayout {
                            anchors.fill: parent
                            RowLayout {
                                Text { text: "Aggression:"; color: "#aaa"; width: 80 }
                                Slider {
                                    from: 0; to: 100; value: AIEditor ? AIEditor.aggression : 50
                                    onValueChanged: if (AIEditor) AIEditor.setAggression(value)
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Text { text: "Consistency:"; color: "#aaa"; width: 80 }
                                Slider {
                                    from: 0; to: 100; value: AIEditor ? AIEditor.consistency : 70
                                    onValueChanged: if (AIEditor) AIEditor.setConsistency(value)
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Text { text: "Precision:"; color: "#aaa"; width: 80 }
                                Slider {
                                    from: 0; to: 100; value: AIEditor ? AIEditor.precision : 50
                                    onValueChanged: if (AIEditor) AIEditor.setPrecision(value)
                                    Layout.fillWidth: true
                                }
                            }
                            RowLayout {
                                Text { text: "Difficulty:"; color: "#aaa"; width: 80 }
                                Slider {
                                    from: 0; to: 100; value: AIEditor ? AIEditor.difficulty : 50
                                    onValueChanged: if (AIEditor) AIEditor.setDifficulty(value)
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    Label {
                        text: "Status: AI parameters ready"
                        color: "#666"
                        font.pixelSize: 10
                        Layout.margins: 8
                    }
                }
            }
        }
    }
}
