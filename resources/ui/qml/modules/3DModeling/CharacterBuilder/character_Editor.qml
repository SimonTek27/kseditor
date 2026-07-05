import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"
import ksEditor.Character 1.0

Rectangle {
    id: characterEditor
    width: 1280
    height: 720
    color: "#121212"

    property string activePanel: "skeleton"
    property string currentFile: ""
    property bool skeletonDirty: false
    property int selectedBoneIndex: -1
    property vector3d selectedBonePos: Qt.vector3d(0, 0, 0)
    property var bonePos: ({x: 0, y: 0, z: 0})

    FileDialog {
        id: importDialog
        title: "Import Character"
        nameFilters: ["FBX files (*.fbx)", "KN5 files (*.kn5)", "All files (*)"]
        onAccepted: {
            currentFile = selectedFile.toString().replace("file:///", "").split("/").pop().split("\\").pop()
        }
    }

    FileDialog {
        id: exportDialog
        title: "Export Character"
        nameFilters: ["FBX files (*.fbx)", "All files (*)"]
    }

    Component.onCompleted: {
        if (!CharacterEditor.hasSkeleton)
            CharacterEditor.createHumanoid(1.8)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                KsButton {
                    text: "Humanoid"
                    height: 28
                    flat: true
                    bgcolor: CharacterEditor.hasSkeleton ? "#3e3e42" : "#E10600"
                    color: "#ffffff"
                    onClicked: CharacterEditor.createHumanoid(1.8)
                }
                KsButton {
                    text: "Quadruped"
                    height: 28
                    flat: true
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: CharacterEditor.createQuadruped(1.5, 2.0)
                }

                Rectangle { width: 1; height: 24; color: "#333" }

                KsButton {
                    text: "Bind Mesh"
                    height: 28
                    flat: true
                    bgcolor: "#E10600"
                    color: "#121212"
                    onClicked: CharacterEditor.bindToMesh("selected")
                }
                KsButton {
                    text: "Normalize"
                    height: 28
                    flat: true
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: CharacterEditor.normalizeWeights()
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: currentFile || "No file"
                    color: "#aaaaaa"
                    font.pixelSize: 12
                }
                KsButton {
                    height: 28
                    text: "Import"
                    flat: true
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: importDialog.open()
                }
                KsButton {
                    height: 28
                    text: "Export"
                    flat: true
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: exportDialog.open()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Left panel
            Rectangle {
                width: 140
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 4

                    Text {
                        text: "PANELS"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton {
                        height: 28
                        text: "Skeleton"
                        bgcolor: activePanel === "skeleton" ? "#E10600" : "#3e3e42"
                        color: activePanel === "skeleton" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "skeleton"
                    }
                    KsButton {
                        height: 28
                        text: "Weights"
                        bgcolor: activePanel === "weights" ? "#E10600" : "#3e3e42"
                        color: activePanel === "weights" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "weights"
                    }
                    KsButton {
                        height: 28
                        text: "Poses"
                        bgcolor: activePanel === "poses" ? "#E10600" : "#3e3e42"
                        color: activePanel === "poses" ? "#121212" : "#ffffff"
                        onClicked: activePanel = "poses"
                    }

                    Rectangle { height: 10 }

                    Text {
                        text: "SKINNING"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton { height: 28; text: "Driver"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: activePanel = "driver" }
                    KsButton { height: 28; text: "Helmet"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: activePanel = "helmet" }
                    KsButton { height: 28; text: "Suit"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: activePanel = "suit" }
                    KsButton { height: 28; text: "Gloves"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: activePanel = "gloves" }
                    KsButton { height: 28; text: "Face"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: activePanel = "face" }
                    KsButton { height: 28; text: "Hair"; bgcolor: "#3e3e42"; color: "#ffffff"; onClicked: activePanel = "hair" }

                    Item { Layout.fillHeight: true }
                }
            }

            // Center panel
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#2a2a2a"

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 15
                    clip: true
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: parent.width - 10
                        spacing: 15

                        // === SKELETON PANEL ===
                        ColumnLayout {
                            visible: activePanel === "skeleton"
                            spacing: 12
                            width: parent.width

                            Text {
                                text: "SKELETON EDITOR"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                color: "#252526"
                                border.color: "#3e3e42"
                                border.width: 1
                                implicitHeight: 300

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    // Bone list
                                    Rectangle {
                                        Layout.preferredWidth: 200
                                        Layout.fillHeight: true
                                        color: "#1e1e1e"
                                        border.color: "#333"
                                        border.width: 1

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            spacing: 4

                                            Text {
                                                text: "BONES (%1)".arg(CharacterEditor.boneNames.length)
                                                color: "#E10600"
                                                font.pixelSize: 10
                                                font.bold: true
                                            }

                                            ListView {
                                                id: boneListView
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true
                                                model: CharacterEditor.boneNames
                                                clip: true
                                                spacing: 1

                                                delegate: Rectangle {
                                                    width: ListView.view.width
                                                    height: 24
                                                    color: ListView.isCurrentIndex ? "#E10600" : "transparent"
                                                    radius: 2

                                                    Text {
                                                        anchors.left: parent.left
                                                        anchors.leftMargin: 8
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        text: {
                                                            var indent = CharacterEditor.boneParent(index) >= 0 ? "  " : ""
                                                            return indent + modelData
                                                        }
                                                        color: "white"
                                                        font.pixelSize: 11
                                                    }

                                                    MouseArea {
                                                        anchors.fill: parent
                                                        onClicked: {
                                                            boneListView.currentIndex = index
                                                            CharacterEditor.selectBone(index)
                                                            selectedBoneIndex = index
                                                            selectedBonePos = Qt.vector3d(0, 0, 0)
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // Bone properties
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        color: "transparent"

                                        ColumnLayout {
                                            anchors.fill: parent
                                            spacing: 8

                                            Text {
                                                text: "PROPERTIES"
                                                color: "#E10600"
                                                font.pixelSize: 10
                                                font.bold: true
                                            }

                                            GridLayout {
                                                columns: 2
                                                columnSpacing: 8
                                                rowSpacing: 6

                                                Text { text: "Name:"; color: "#bbb"; font.pixelSize: 11 }
                                                TextField {
                                                    id: boneNameField
                                                    Layout.fillWidth: true
                                                    height: 24
                                                    text: selectedBoneIndex >= 0 && selectedBoneIndex < CharacterEditor.boneCount ? CharacterEditor.boneNames[selectedBoneIndex] : ""
                                                }

                                                Text { text: "Parent:"; color: "#bbb"; font.pixelSize: 11 }
                                                ComboBox {
                                                    id: parentCombo
                                                    Layout.fillWidth: true
                                                    height: 24
                                                    model: CharacterEditor.boneNames
                                                    currentIndex: selectedBoneIndex >= 0 ? CharacterEditor.boneParent(selectedBoneIndex) + 1 : 0
                                                }

                                                Text { text: "Pos X:"; color: "#bbb"; font.pixelSize: 11 }
                                                SpinBox {
                                                    id: posXSpin
                                                    Layout.fillWidth: true
                                                    height: 24
                                                    from: -1000; to: 1000; stepSize: 0.01; decimals: 2
                                                    value: selectedBonePos.x
                                                }

                                                Text { text: "Pos Y:"; color: "#bbb"; font.pixelSize: 11 }
                                                SpinBox {
                                                    id: posYSpin
                                                    Layout.fillWidth: true
                                                    height: 24
                                                    from: -1000; to: 1000; stepSize: 0.01; decimals: 2
                                                    value: selectedBonePos.y
                                                }

                                                Text { text: "Pos Z:"; color: "#bbb"; font.pixelSize: 11 }
                                                SpinBox {
                                                    id: posZSpin
                                                    Layout.fillWidth: true
                                                    height: 24
                                                    from: -1000; to: 1000; stepSize: 0.01; decimals: 2
                                                    value: selectedBonePos.z
                                                }

                                                Text { text: "Rot X:"; color: "#bbb"; font.pixelSize: 11 }
                                                SpinBox {
                                                    id: rotXSpin
                                                    Layout.fillWidth: true
                                                    height: 24
                                                    from: -360; to: 360; stepSize: 1; decimals: 0
                                                }

                                                Text { text: "Rot Y:"; color: "#bbb"; font.pixelSize: 11 }
                                                SpinBox {
                                                    id: rotYSpin
                                                    Layout.fillWidth: true
                                                    height: 24
                                                    from: -360; to: 360; stepSize: 1; decimals: 0
                                                }

                                                Text { text: "Rot Z:"; color: "#bbb"; font.pixelSize: 11 }
                                                SpinBox {
                                                    id: rotZSpin
                                                    Layout.fillWidth: true
                                                    height: 24
                                                    from: -360; to: 360; stepSize: 1; decimals: 0
                                                }
                                            }

                                            RowLayout {
                                                spacing: 6
                                                KsButton {
                                                    text: "Apply"
                                                    height: 26
                                                    bgcolor: "#E10600"
                                                    color: "#121212"
                                                    onClicked: {
                                                        var idx = boneListView.currentIndex
                                                        if (idx < 0) return
                                                        CharacterEditor.moveBone(idx, posXSpin.value - selectedBonePos.x, posYSpin.value - selectedBonePos.y, posZSpin.value - selectedBonePos.z)
                                                        CharacterEditor.rotateBone(idx, rotXSpin.value, rotYSpin.value, rotZSpin.value)
                                                        selectedBonePos = Qt.vector3d(posXSpin.value, posYSpin.value, posZSpin.value)
                                                    }
                                                }
                                                KsButton {
                                                    text: "Add Bone"
                                                    height: 26
                                                    bgcolor: "#3e3e42"
                                                    color: "#ffffff"
                                                    onClicked: {
                                                        var parentIdx = boneListView.currentIndex
                                                        var name = "Bone_" + CharacterEditor.boneNames.length
                                                        CharacterEditor.addBone(name, parentIdx >= 0 ? parentIdx : -1, 0, -0.1, 0)
                                                    }
                                                }
                                                KsButton {
                                                    text: "Remove"
                                                    height: 26
                                                    bgcolor: "#3e3e42"
                                                    color: "#ffffff"
                                                    onClicked: {
                                                        var idx = boneListView.currentIndex
                                                        if (idx >= 0) CharacterEditor.removeBone(idx)
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Manual IK/FK controls
                            Rectangle {
                                Layout.fillWidth: true
                                color: "#252526"
                                border.color: "#3e3e42"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    Text { text: "IK/FK:"; color: "#bbb"; font.pixelSize: 11 }
                                    Text { text: "Root: " + (boneListView.currentIndex >= 0 ? CharacterEditor.boneNames[boneListView.currentIndex] : "-"); color: "white"; font.pixelSize: 11 }
                                    Text { text: "FK +X"; color: "#E10600"; font.pixelSize: 11 }
                                    Text { text: "FK -X"; color: "#E10600"; font.pixelSize: 11 }
                                }
                            }
                        }

                        // === WEIGHTS PANEL ===
                        ColumnLayout {
                            visible: activePanel === "weights"
                            spacing: 12
                            width: parent.width

                            Text {
                                text: "WEIGHT PAINTING"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                color: "#252526"
                                border.color: "#3e3e42"
                                border.width: 1
                                implicitHeight: 200

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 8

                                    Text {
                                        text: "TOOLS"
                                        color: "#E10600"
                                        font.pixelSize: 10
                                        font.bold: true
                                    }

                                    RowLayout {
                                        spacing: 10
                                        Text { text: "Target Bone:"; color: "#bbb"; font.pixelSize: 11 }
                                        ComboBox {
                                            id: weightBoneCombo
                                            Layout.fillWidth: true
                                            height: 24
                                            model: CharacterEditor.boneNames
                                        }
                                    }

                                    RowLayout {
                                        spacing: 10
                                        Text { text: "Radius:"; color: "#bbb"; font.pixelSize: 11 }
                                        Slider {
                                            id: radiusSlider
                                            Layout.fillWidth: true
                                            from: 0.01; to: 10; value: 2.0
                                        }
                                        Text { text: radiusSlider.value.toFixed(2); color: "#E10600"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                    }

                                    RowLayout {
                                        spacing: 10
                                        Text { text: "Strength:"; color: "#bbb"; font.pixelSize: 11 }
                                        Slider {
                                            id: strengthSlider
                                            Layout.fillWidth: true
                                            from: 0; to: 1; value: 0.5
                                        }
                                        Text { text: strengthSlider.value.toFixed(2); color: "#E10600"; font.pixelSize: 11; Layout.preferredWidth: 40 }
                                    }

                                    RowLayout {
                                        spacing: 8
                                        KsButton {
                                            text: "Paint"
                                            height: 28
                                            bgcolor: "#E10600"
                                            color: "#121212"
                                            onClicked: CharacterEditor.paintWeights(weightBoneCombo.currentIndex, 0, 0.9, 0, radiusSlider.value, strengthSlider.value)
                                        }
                                        KsButton {
                                            text: "Smooth"
                                            height: 28
                                            bgcolor: "#3e3e42"
                                            color: "#ffffff"
                                            onClicked: CharacterEditor.smoothWeights(weightBoneCombo.currentIndex, radiusSlider.value)
                                        }
                                        KsButton {
                                            text: "Normalize All"
                                            height: 28
                                            bgcolor: "#3e3e42"
                                            color: "#ffffff"
                                            onClicked: CharacterEditor.normalizeWeights()
                                        }
                                        KsButton {
                                            text: "Auto-Bind"
                                            height: 28
                                            bgcolor: "#E10600"
                                            color: "#121212"
                                            onClicked: CharacterEditor.bindToMesh("selected")
                                        }
                                    }
                                }
                            }
                        }

                        // === POSES PANEL ===
                        ColumnLayout {
                            visible: activePanel === "poses"
                            spacing: 12
                            width: parent.width

                            Text {
                                text: "POSE SYSTEM"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                color: "#252526"
                                border.color: "#3e3e42"
                                border.width: 1
                                implicitHeight: 200

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 8

                                    RowLayout {
                                        spacing: 8
                                        TextField {
                                            id: poseNameField
                                            Layout.preferredWidth: 150
                                            height: 24
                                            placeholderText: "Pose name..."
                                        }
                                        KsButton {
                                            text: "Save Current"
                                            height: 28
                                            bgcolor: "#E10600"
                                            color: "#121212"
                                            onClicked: {
                                                if (poseNameField.text.length > 0) {
                                                    CharacterEditor.savePose(poseNameField.text)
                                                    poseNameField.text = ""
                                                }
                                            }
                                        }
                                    }

                                    ListView {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        model: CharacterEditor.poseList
                                        clip: true
                                        spacing: 2

                                        delegate: Rectangle {
                                            width: parent.width
                                            height: 28
                                            color: "#1e1e1e"
                                            radius: 3

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.margins: 6
                                                spacing: 6

                                                Text {
                                                    text: modelData
                                                    color: "white"
                                                    font.pixelSize: 12
                                                    Layout.fillWidth: true
                                                }

                                                KsButton {
                                                    text: "Apply"
                                                    height: 24
                                                    bgcolor: "#3e3e42"
                                                    color: "#ffffff"
                                                    onClicked: CharacterEditor.applyPose(modelData)
                                                }
                                                KsButton {
                                                    text: "Del"
                                                    height: 24
                                                    bgcolor: "#5a1a1a"
                                                    color: "#ffffff"
                                                    onClicked: CharacterEditor.removePose(modelData)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // === DRIVER / HELMET / SUIT / GLOVES / FACE / HAIR (simplified) ===
                        ColumnLayout {
                            visible: activePanel === "driver"
                            spacing: 12
                            width: parent.width
                            Text { text: "DRIVER MODEL"; color: "white"; font.bold: true; font.pixelSize: 14 }
                            Rectangle {
                                Layout.fillWidth: true; color: "#252526"; border.color: "#3e3e42"; border.width: 1
                                ColumnLayout {
                                    anchors.fill: parent; anchors.margins: 10; spacing: 10
                                    RowLayout { Text { text: "Name:"; color: "#bbb"; Layout.preferredWidth: 70 } TextField { text: "Driver 1"; Layout.fillWidth: true } }
                                    RowLayout { Text { text: "Nationality:"; color: "#bbb"; Layout.preferredWidth: 70 } ComboBox { Layout.fillWidth: true; model: ["USA","UK","Germany","Italy","France","Spain","Japan","Brazil"] } }
                                    RowLayout { Text { text: "Body Type:"; color: "#bbb"; Layout.preferredWidth: 70 } ComboBox { Layout.fillWidth: true; model: ["Slim","Athletic","Average","Muscular"] } }
                                }
                            }
                        }

                        ColumnLayout {
                            visible: activePanel === "helmet"
                            spacing: 12
                            width: parent.width
                            Text { text: "HELMET EDITOR"; color: "white"; font.bold: true; font.pixelSize: 14 }
                            Rectangle {
                                Layout.fillWidth: true; color: "#252526"; border.color: "#3e3e42"; border.width: 1
                                ColumnLayout {
                                    anchors.fill: parent; anchors.margins: 10; spacing: 10
                                    RowLayout { Text { text: "Brand:"; color: "#bbb"; Layout.preferredWidth: 70 } ComboBox { Layout.fillWidth: true; model: ["Bell","Sparco","Schuberth","Alpinestars","Puma"] } }
                                    RowLayout { Text { text: "Color:"; color: "#bbb"; Layout.preferredWidth: 70 } KsButton { height: 28; text: "Select"; bgcolor: "transparent"; color: "#ffffff" } }
                                }
                            }
                        }

                        ColumnLayout {
                            visible: activePanel === "suit"
                            spacing: 12
                            width: parent.width
                            Text { text: "SUIT EDITOR"; color: "white"; font.bold: true; font.pixelSize: 14 }
                            Rectangle {
                                Layout.fillWidth: true; color: "#252526"; border.color: "#3e3e42"; border.width: 1
                                ColumnLayout {
                                    anchors.fill: parent; anchors.margins: 10; spacing: 10
                                    RowLayout { Text { text: "Team:"; color: "#bbb"; Layout.preferredWidth: 70 } TextField { text: "Team Name"; Layout.fillWidth: true } }
                                    RowLayout { Text { text: "Primary:"; color: "#bbb"; Layout.preferredWidth: 70 } KsButton { height: 28; text: "Select"; bgcolor: "transparent"; color: "#ffffff" } }
                                    RowLayout { CheckBox { checked: true } Text { text: "Number Display"; color: "#bbb" } }
                                    RowLayout { CheckBox { checked: true } Text { text: "Sponsor Logos"; color: "#bbb" } }
                                }
                            }
                        }

                        ColumnLayout {
                            visible: activePanel === "gloves"
                            spacing: 12
                            width: parent.width
                            Text { text: "GLOVES"; color: "white"; font.bold: true; font.pixelSize: 14 }
                            Rectangle {
                                Layout.fillWidth: true; color: "#252526"; border.color: "#3e3e42"; border.width: 1
                                ColumnLayout {
                                    anchors.fill: parent; anchors.margins: 10; spacing: 10
                                    RowLayout { Text { text: "Brand:"; color: "#bbb"; Layout.preferredWidth: 70 } ComboBox { Layout.fillWidth: true; model: ["Alpinestars","Sparco","OMP","Racequip"] } }
                                    RowLayout { Text { text: "Color:"; color: "#bbb"; Layout.preferredWidth: 70 } KsButton { height: 28; text: "Select"; bgcolor: "transparent"; color: "#ffffff" } }
                                }
                            }
                        }

                        ColumnLayout {
                            visible: activePanel === "face"
                            spacing: 12
                            width: parent.width
                            Text { text: "FACE MORPH"; color: "white"; font.bold: true; font.pixelSize: 14 }
                            Rectangle {
                                Layout.fillWidth: true; color: "#252526"; border.color: "#3e3e42"; border.width: 1
                                ColumnLayout {
                                    anchors.fill: parent; anchors.margins: 10; spacing: 10
                                    RowLayout { Text { text: "Shape:"; color: "#bbb" } Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true } Text { text: "50%"; color: "#E10600"; font.pixelSize: 11 } }
                                    RowLayout { Text { text: "Skin Tone:"; color: "#bbb" } Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true } Text { text: "50%"; color: "#E10600"; font.pixelSize: 11 } }
                                    RowLayout { Text { text: "Nose:"; color: "#bbb" } Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true } Text { text: "50%"; color: "#E10600"; font.pixelSize: 11 } }
                                    RowLayout { Text { text: "Chin:"; color: "#bbb" } Slider { from: 0; to: 100; value: 50; Layout.fillWidth: true } Text { text: "50%"; color: "#E10600"; font.pixelSize: 11 } }
                                }
                            }
                        }

                        ColumnLayout {
                            visible: activePanel === "hair"
                            spacing: 12
                            width: parent.width
                            Text { text: "HAIR STYLER"; color: "white"; font.bold: true; font.pixelSize: 14 }
                            Rectangle {
                                Layout.fillWidth: true; color: "#252526"; border.color: "#3e3e42"; border.width: 1
                                ColumnLayout {
                                    anchors.fill: parent; anchors.margins: 10; spacing: 10
                                    RowLayout { Text { text: "Style:"; color: "#bbb"; Layout.preferredWidth: 70 } ComboBox { Layout.fillWidth: true; model: ["Short","Medium","Long","Ponytail","Bald"] } }
                                    RowLayout { Text { text: "Color:"; color: "#bbb"; Layout.preferredWidth: 70 } KsButton { height: 28; text: "Select"; bgcolor: "transparent"; color: "#ffffff" } }
                                }
                            }
                        }
                    }
                }
            }

            // Right panel: Actions
            Rectangle {
                width: 160
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    Text {
                        text: "ACTIONS"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    KsButton {
                        height: 28
                        text: "Reset Pose"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        onClicked: {
                            CharacterEditor.createHumanoid(CharacterEditor.boneCount > 0 ? 1.8 : 1.8)
                        }
                    }
                    KsButton {
                        height: 28
                        text: "Reset Skeleton"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        onClicked: CharacterEditor.createHumanoid(1.8)
                    }
                    KsButton {
                        height: 28
                        text: "Save Preset"
                        bgcolor: "#E10600"
                        color: "#121212"
                        onClicked: CharacterEditor.savePose("preset_" + new Date().toISOString().slice(0, 10))
                    }

                    Item { Layout.fillHeight: true }

                    KsButton {
                        height: 36
                        text: "Apply"
                        bgcolor: "#E10600"
                        color: "#121212"
                        onClicked: CharacterEditor.bindToMesh("selected")
                    }
                }
            }
        }

        // Status bar
        Rectangle {
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                Text {
                    text: "Bones: " + CharacterEditor.boneNames.length
                    color: "#E10600"
                    font.pixelSize: 10
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "ksEditor v1.0 - Character Builder"
                    color: "#666"
                    font.pixelSize: 10
                }
            }
        }
    }
}

