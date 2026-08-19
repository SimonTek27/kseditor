import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: bakePanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property int bakeType: 4
    property int bakeResolution: 512
    property string bakePath: ""
    property bool hasResult: false
    property string statusText: ""
    property int packChannels: 0

    function targetObjectId() {
        return Modeler && Modeler.selectedObject ? Modeler.selectedObject.id : -1
    }
    function targetName() {
        return Modeler && Modeler.selectedObject ? Modeler.selectedObject.name : "no selection"
    }

    Connections {
        target: Modeler
        function onBakeResultChanged() {
            bakePanel.hasResult = true
        }
        function onSceneChanged() {
            bakePanel.hasResult = false
        }
    }

    FileDialog {
        id: bakeFileDialog
        title: "Save baked map"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "png"
        nameFilters: ["PNG Image (*.png)", "Targa (*.tga)", "JPEG (*.jpg)", "All Files (*)"]
        onAccepted: {
            bakePanel.bakePath = fileUrl.toString().replace("file:///", "").replace("file://", "")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "BAKE TO TEXTURE"
                color: "#E10600"
                font.pixelSize: 13
                font.bold: true
                Layout.fillWidth: true
            }

            AppButton {
                text: "X"
                height: 24
                width: 26
                bgcolor: "#3e3e42"
                color: "#ffffff"
                font.pixelSize: 10
                font.bold: true
                onClicked: closePanel()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text {
            text: "TARGET: " + bakePanel.targetName()
            color: "#ccc"
            font.pixelSize: 10
            font.bold: true
        }

        Text { text: "MAP TYPE"; color: "#888"; font.pixelSize: 10; font.bold: true }

        GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: 4
            rowSpacing: 4

            Repeater {
                model: ["Diffuse", "Normal", "Roughness", "Metallic", "AO", "Height", "Emission"]
                delegate: AppButton {
                    text: modelData
                    height: 26
                    Layout.preferredWidth: bakePanel.width / 4 - 6
                    bgcolor: bakePanel.bakeType === index ? "#E10600" : "#3e3e42"
                    color: bakePanel.bakeType === index ? "#121212" : "#fff"
                    font.pixelSize: 9
                    onClicked: bakePanel.bakeType = index
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text { text: "Resolution"; color: "#888"; font.pixelSize: 9 }
            ComboBox {
                id: resCombo
                Layout.fillWidth: true
                height: 24
                model: [128, 256, 512, 1024, 2048]
                currentIndex: 2
                font.pixelSize: 10
                onActivated: bakePanel.bakeResolution = parseInt(currentText)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text { text: "Pack Channels"; color: "#888"; font.pixelSize: 9; font.bold: true }
            AppCheckBox {
                id: roughnessCheck
                text: "Roughness"
                checked: (bakePanel.packChannels & 1) !== 0
                onCheckedChanged: {
                    var ch = bakePanel.packChannels
                    if (checked) ch |= 1
                    else ch &= ~1
                    bakePanel.packChannels = ch
                }
            }
            AppCheckBox {
                id: metallicCheck
                text: "Metallic"
                checked: (bakePanel.packChannels & 2) !== 0
                onCheckedChanged: {
                    var ch = bakePanel.packChannels
                    if (checked) ch |= 2
                    else ch &= ~2
                    bakePanel.packChannels = ch
                }
            }
            AppCheckBox {
                id: aoCheck
                text: "AO"
                checked: (bakePanel.packChannels & 4) !== 0
                onCheckedChanged: {
                    var ch = bakePanel.packChannels
                    if (checked) ch |= 4
                    else ch &= ~4
                    bakePanel.packChannels = ch
                }
            }
            AppCheckBox {
                id: heightCheck
                text: "Height"
                checked: (bakePanel.packChannels & 8) !== 0
                onCheckedChanged: {
                    var ch = bakePanel.packChannels
                    if (checked) ch |= 8
                    else ch &= ~8
                    bakePanel.packChannels = ch
                }
            }
            AppCheckBox {
                id: emissionCheck
                text: "Emission"
                checked: (bakePanel.packChannels & 16) !== 0
                onCheckedChanged: {
                    var ch = bakePanel.packChannels
                    if (checked) ch |= 16
                    else ch &= ~16
                    bakePanel.packChannels = ch
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text { text: "Output"; color: "#888"; font.pixelSize: 9; font.bold: true; Layout.alignment: Qt.AlignVCenter }
            TextField {
                id: pathField
                Layout.fillWidth: true
                height: 24
                text: bakePanel.bakePath
                placeholderText: "bake map output path..."
                font.pixelSize: 9
                onEditingFinished: bakePanel.bakePath = text
            }
            AppButton {
                text: "..."
                height: 24
                width: 30
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                onClicked: bakeFileDialog.open()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: "Bake"
                height: 30
                Layout.fillWidth: true
                bgcolor: "#E10600"
                color: "#121212"
                font.pixelSize: 10
                font.bold: true
                onClicked: {
                    bakePanel.bakePath = pathField.text
                    var ok = Modeler.bakeObject(bakePanel.targetObjectId(), bakePanel.bakeType,
                                                bakePanel.bakePath, bakePanel.bakeResolution, bakePanel.bakeResolution)
                    bakePanel.statusText = ok ? "Bake complete" : "Bake failed"
                }
            }
            AppButton {
                text: "Pack Channels"
                height: 30
                Layout.fillWidth: true
                bgcolor: "#E10600"
                color: "#121212"
                font.pixelSize: 10
                font.bold: true
                onClicked: {
                    bakePanel.statusText = "Packing channels..."
                    var packed = Modeler.packBakeChannels(bakePanel.targetObjectId(), bakePanel.packChannels,
                                                          bakePanel.bakeResolution, bakePanel.bakeResolution)
                    if (packed) {
                        var df = bakePanel.bakePath
                        if (!df) df = "baked_packed.png"
                        var ok = Modeler.saveBakedTexture(df)
                        bakePanel.statusText = ok ? "Pack complete" : "Pack failed"
                    } else {
                        bakePanel.statusText = "Pack failed"
                    }
                }
            }
            AppButton {
                text: "Preview"
                height: 30
                Layout.fillWidth: true
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 10
                onClicked: {
                    bakePanel.statusText = "Baking preview..."
                    var ok = Modeler.bakePreview(bakePanel.targetObjectId(), bakePanel.bakeType,
                                                 bakePanel.bakeResolution, bakePanel.bakeResolution)
                    bakePanel.statusText = ok ? "Preview ready" : "Preview failed"
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text { text: bakePanel.statusText
                color: "#8f8"
                font.pixelSize: 9
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            AppButton {
                text: "Clear"
                height: 22
                visible: bakePanel.hasResult
                bgcolor: "#3e3e42"
                color: "#fff"
                font.pixelSize: 9
                onClicked: Modeler.bakeClearResult()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Image {
                id: previewImage
                anchors.fill: parent
                anchors.margins: 2
                fillMode: Image.PreserveAspectFit
                smooth: true
                source: bakePanel.hasResult ? "image://bake/result?rev=" + Modeler.bakeRevision : ""
            }

            Text {
                anchors.centerIn: parent
                text: "No bake result yet"
                color: "#555"
                font.pixelSize: 10
                visible: !bakePanel.hasResult
            }
        }
    }
}