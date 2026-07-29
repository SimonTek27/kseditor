import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: root
    color: "#1a1a1a"

    property string statusText: ""
    property int selectedCameraIndex: -1
    property int selectedLightIndex: -1

    FileDialog {
        id: openDialog
        title: "Load Showroom Config"
        nameFilters: ["INI files (*.ini)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            ShowroomEditor.loadShowroom(path)
            statusText = "Loaded showroom config"
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save Showroom Config"
        nameFilters: ["INI files (*.ini)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            ShowroomEditor.saveShowroom(path)
            statusText = "Saved showroom config"
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

                Text { text: "Showroom Editor"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                Text { text: ShowroomEditor.showroomName || "Default"; color: "#888"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }

                Button { text: "Load"; flat: true; font.pixelSize: 11; onClicked: openDialog.open() }
                Button { text: "Save"; flat: true; font.pixelSize: 11; onClicked: {
                    if (ShowroomEditor.showroomName) ShowroomEditor.saveShowroom("")
                    else saveDialog.open()
                }}
                Button { text: "Save As..."; flat: true; font.pixelSize: 11; onClicked: saveDialog.open() }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Rectangle {
                color: "#1e1e1e"
                SplitView.preferredWidth: 340

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 8
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        Rectangle {
                            Layout.fillWidth: true
                            height: camCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: camCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Camera Settings"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                RowLayout {
                                    Text { text: "Distance:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Slider { id: distSlider; from: 1; to: 20; value: ShowroomEditor.cameraDistance; stepSize: 0.1; Layout.fillWidth: true; onMoved: ShowroomEditor.cameraDistance = value }
                                    Text { text: distSlider.value.toFixed(1); color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 35 }
                                }

                                RowLayout {
                                    Text { text: "Height:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Slider { id: heightSlider; from: 0; to: 10; value: ShowroomEditor.cameraHeight; stepSize: 0.1; Layout.fillWidth: true; onMoved: ShowroomEditor.cameraHeight = value }
                                    Text { text: heightSlider.value.toFixed(1); color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 35 }
                                }

                                RowLayout {
                                    Text { text: "Angle:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Slider { id: angleSlider; from: 0; to: 90; value: ShowroomEditor.cameraAngle; stepSize: 1; Layout.fillWidth: true; onMoved: ShowroomEditor.cameraAngle = value }
                                    Text { text: Math.round(angleSlider.value) + "\u00b0"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 35 }
                                }

                                RowLayout {
                                    Text { text: "FOV:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Slider { id: fovSlider; from: 20; to: 120; value: ShowroomEditor.cameraFov; stepSize: 1; Layout.fillWidth: true; onMoved: ShowroomEditor.cameraFov = value }
                                    Text { text: Math.round(fovSlider.value) + "\u00b0"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 35 }
                                }

                                RowLayout {
                                    Text { text: "Speed:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Slider { id: speedSlider; from: 0; to: 5; value: ShowroomEditor.rotateSpeed; stepSize: 0.1; Layout.fillWidth: true; onMoved: ShowroomEditor.rotateSpeed = value }
                                    Text { text: speedSlider.value.toFixed(1); color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 35 }
                                }

                                RowLayout {
                                    Text { text: "Auto Rotate:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Switch { checked: ShowroomEditor.autoRotate; onToggled: ShowroomEditor.autoRotate = checked }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: lightCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: lightCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Lighting"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                RowLayout {
                                    Text { text: "Sun Color:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Rectangle {
                                        width: 24; height: 24; radius: 4
                                        color: ShowroomEditor.sunColor
                                        border.color: "#555"; border.width: 1
                                        MouseArea { anchors.fill: parent; onClicked: sunColorDialog.open() }
                                    }
                                    ColorDialog { id: sunColorDialog; color: ShowroomEditor.sunColor; onAccepted: ShowroomEditor.sunColor = color.toString() }
                                }

                                RowLayout {
                                    Text { text: "Sun Int:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Slider { id: sunIntSlider; from: 0; to: 3; value: ShowroomEditor.sunIntensity; stepSize: 0.1; Layout.fillWidth: true; onMoved: ShowroomEditor.sunIntensity = value }
                                    Text { text: sunIntSlider.value.toFixed(1); color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 35 }
                                }

                                RowLayout {
                                    Text { text: "Amb Color:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Rectangle {
                                        width: 24; height: 24; radius: 4
                                        color: ShowroomEditor.ambientColor
                                        border.color: "#555"; border.width: 1
                                        MouseArea { anchors.fill: parent; onClicked: ambColorDialog.open() }
                                    }
                                    ColorDialog { id: ambColorDialog; color: ShowroomEditor.ambientColor; onAccepted: ShowroomEditor.ambientColor = color.toString() }
                                }

                                RowLayout {
                                    Text { text: "Amb Int:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    Slider { id: ambIntSlider; from: 0; to: 2; value: ShowroomEditor.ambientIntensity; stepSize: 0.1; Layout.fillWidth: true; onMoved: ShowroomEditor.ambientIntensity = value }
                                    Text { text: ambIntSlider.value.toFixed(1); color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 35 }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: bgCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: bgCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Background"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                RowLayout {
                                    Text { text: "Path:"; color: "#ccc"; font.pixelSize: 11; Layout.preferredWidth: 70 }
                                    TextField { id: bgPathField; text: ShowroomEditor.backgroundPath; Layout.fillWidth: true; font.pixelSize: 11; onEditingFinished: ShowroomEditor.backgroundPath = text }
                                    Button { text: "..."; font.pixelSize: 11; onClicked: bgFileDialog.open() }
                                }

                                FileDialog { id: bgFileDialog; title: "Select Background"; nameFilters: ["Images (*.png *.jpg *.hdr)", "All files (*)"]; onAccepted: { var p = selectedFile.toString().replace("file:///", ""); bgPathField.text = p; ShowroomEditor.backgroundPath = p } }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: camListCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: camListCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Cameras"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                Repeater {
                                    model: ShowroomEditor.getCameras()

                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 28
                                        color: selectedCameraIndex === index ? "#3a3a3a" : "#1e1e1e"
                                        radius: 3

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            spacing: 4

                                            Rectangle { width: 8; height: 8; radius: 4; color: modelData.isActive ? "#4CAF50" : "#666" }
                                            Text { text: modelData.name; color: "#ccc"; font.pixelSize: 10; Layout.fillWidth: true }
                                            Text { text: "FOV:" + Math.round(modelData.fov); color: "#666"; font.pixelSize: 9 }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: selectedCameraIndex = index
                                            onDoubleClicked: selectedCameraIndex = index
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Button { text: "Add"; font.pixelSize: 10; Layout.fillWidth: true; onClicked: { ShowroomEditor.addCamera({"name": "Camera " + ShowroomEditor.getCameras().length}); statusText = "Added camera" } }
                                    Button { text: "Remove"; font.pixelSize: 10; Layout.fillWidth: true; enabled: selectedCameraIndex >= 0; onClicked: { ShowroomEditor.removeCamera(selectedCameraIndex); selectedCameraIndex = -1; statusText = "Removed camera" } }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: lightListCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: lightListCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Lights"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                Repeater {
                                    model: ShowroomEditor.getLights()

                                    Rectangle {
                                        Layout.fillWidth: true
                                        height: 28
                                        color: selectedLightIndex === index ? "#3a3a3a" : "#1e1e1e"
                                        radius: 3

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            spacing: 4

                                            Rectangle { width: 8; height: 8; radius: 4; color: modelData.isActive ? "#4CAF50" : "#666" }
                                            Text { text: modelData.name; color: "#ccc"; font.pixelSize: 10; Layout.fillWidth: true }
                                            Text { text: modelData.type; color: "#666"; font.pixelSize: 9 }
                                            Rectangle { width: 12; height: 12; radius: 2; color: modelData.color }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: selectedLightIndex = index
                                            onDoubleClicked: selectedLightIndex = index
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 4
                                    Button { text: "Add"; font.pixelSize: 10; Layout.fillWidth: true; onClicked: { ShowroomEditor.addLight({"name": "Light " + ShowroomEditor.getLights().length, "type": "point"}); statusText = "Added light" } }
                                    Button { text: "Remove"; font.pixelSize: 10; Layout.fillWidth: true; enabled: selectedLightIndex >= 0; onClicked: { ShowroomEditor.removeLight(selectedLightIndex); selectedLightIndex = -1; statusText = "Removed light" } }
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: actionsCol.height + 16
                            color: "#252525"
                            radius: 4

                            ColumnLayout {
                                id: actionsCol
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Text { text: "Actions"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                                Button {
                                    text: "Reset to Defaults"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: { ShowroomEditor.resetToDefaults(); statusText = "Reset to defaults" }
                                }

                                Button {
                                    text: "Validate Config"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                    onClicked: {
                                        var result = ShowroomEditor.validateConfig()
                                        statusText = result.valid ? "Config is valid" : "Error: " + result.error
                                    }
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            Rectangle {
                color: "#252525"
                SplitView.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    Text { text: "Status"; color: "#E10600"; font.pixelSize: 12; font.bold: true }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#1a1a1a"
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8

                            Text {
                                text: statusText || "Ready. Configure your showroom settings."
                                color: "#aaa"
                                font.pixelSize: 11
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Text {
                                text: "\nUnsaved: " + (ShowroomEditor.hasUnsavedChanges ? "Yes" : "No")
                                color: ShowroomEditor.hasUnsavedChanges ? "#E10600" : "#666"
                                font.pixelSize: 10
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: infoCol.height + 16
                        color: "#1a1a1a"
                        radius: 4

                        ColumnLayout {
                            id: infoCol
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4

                            Text { text: "Showroom Settings Guide"; color: "#E10600"; font.pixelSize: 11; font.bold: true }
                            Text { text: "Distance: Camera distance from car center"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Height: Camera height above ground"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Angle: Camera viewing angle"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "FOV: Field of view"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Speed: Auto-rotation speed"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Sun: Main directional light"; color: "#888"; font.pixelSize: 9 }
                            Text { text: "Ambient: Global fill light"; color: "#888"; font.pixelSize: 9 }

                            Item { Layout.fillHeight: true }

                            Text { text: "Keyboard Shortcuts"; color: "#E10600"; font.pixelSize: 10; font.bold: true }
                            Text { text: "Ctrl+O  Load Config"; color: "#666"; font.pixelSize: 8 }
                            Text { text: "Ctrl+S  Save Config"; color: "#666"; font.pixelSize: 8 }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: ShowroomEditor
        function onShowroomLoaded(name) { statusText = "Loaded: " + name }
        function onShowroomSaved(name) { statusText = "Saved: " + name }
        function onPreviewGenerated(path) { statusText = "Preview saved: " + path }
    }

    Shortcut { sequence: "Ctrl+O"; onActivated: openDialog.open() }
    Shortcut { sequence: "Ctrl+S"; onActivated: { if (ShowroomEditor.showroomName) ShowroomEditor.saveShowroom(""); else saveDialog.open() } }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: saveDialog.open() }
}
