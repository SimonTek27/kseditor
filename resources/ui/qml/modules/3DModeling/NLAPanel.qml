import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: nlaPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property var clipList: Modeler ? Modeler.nlaClipList() : []
    property double nlaDuration: Modeler ? Modeler.nlaDuration() : 1.0

    Connections {
        target: Modeler
        function onNlaChanged() {
            clipList = Modeler ? Modeler.nlaClipList() : []
            nlaDuration = Modeler ? Modeler.nlaDuration() : 1.0
            sourceCombo.model = Modeler ? Modeler.animationNames() : []
        }
        function onNlaTimeChanged() {
            nlaDuration = Modeler ? Modeler.nlaDuration() : 1.0
            timeSlider.to = Math.max(0.1, nlaDuration)
            if (!timeSlider.pressed) timeSlider.value = Modeler.nlaTime()
            timeLabel.text = formatTime(Modeler.nlaTime()) + " / " + formatTime(nlaDuration)
        }
        function onAnimationNamesChanged() {
            sourceCombo.model = Modeler ? Modeler.animationNames() : []
        }
    }

    function formatTime(t) {
        return Math.floor(t) + ":" + Math.floor((t - Math.floor(t)) * 60).toString().padStart(2, "0")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "NLA TRACK (non-linear animation)"
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

        // Add clip bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "Source:"
                color: "#999"
                font.pixelSize: 10
            }

            ComboBox {
                id: sourceCombo
                Layout.fillWidth: true
                height: 24
                model: Modeler ? Modeler.animationNames() : []
                font.pixelSize: 10
                background: Rectangle { color: parent.hovered ? "#333" : "#252526"; radius: 3 }
                contentItem: Text {
                    text: parent.currentText || "(no animations)"
                    color: "#aaa"; font.pixelSize: 10
                    verticalAlignment: Text.AlignVCenter; leftPadding: 6
                }
                indicator: Text {
                    text: "\u25BC"; color: "#666"; font.pixelSize: 8
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 6
                }
            }

            AppButton {
                text: "+ Add Clip"
                height: 24
                bgcolor: "#E10600"; color: "#fff"
                font.pixelSize: 10
                enabled: sourceCombo.currentText.length > 0
                onClicked: {
                    var idx = Modeler.nlaAddClip(sourceCombo.currentText)
                    if (idx >= 0) {
                        clipList = Modeler.nlaClipList()
                        stripList.currentIndex = idx
                    }
                }
            }
        }

        // Clip strips (master timeline)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#151515"
            border.color: "#2a2a2a"
            border.width: 1
            radius: 3

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Time ruler
                Rectangle {
                    height: 18
                    Layout.fillWidth: true
                    color: "#111"
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 2
                        Repeater {
                            model: Math.max(1, Math.floor(nlaDuration / 0.5))
                            Rectangle {
                                width: (parent.width - 20) / Math.max(1, Math.floor(nlaDuration / 0.5))
                                height: parent.height
                                color: "transparent"
                                Rectangle { width: 1; height: 5; color: "#444"; anchors.left: parent.left; anchors.top: parent.top }
                                Text {
                                    text: (index * 0.5).toFixed(1) + "s"
                                    color: "#555"; font.pixelSize: 8
                                    anchors.left: parent.left; anchors.leftMargin: 2; anchors.top: parent.top; anchors.topMargin: 4
                                }
                            }
                        }
                    }
                }

                // Strips
                ListView {
                    id: stripList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: clipList
                    delegate: Rectangle {
                        id: strip
                        width: Math.max(30, 20 + (modelData.duration / Math.max(0.1, nlaDuration)) * (stripList.width - 20))
                        height: 34
                        x: 20 + (modelData.start / Math.max(0.1, nlaDuration)) * (stripList.width - 20)
                        radius: 3
                        color: modelData.enabled ? (stripList.currentIndex === index ? "#3a4a5a" : "#2a3542") : "#222"
                        border.color: stripList.currentIndex === index ? "#E10600" : (modelData.enabled ? "#445566" : "#333")
                        border.width: 1

                        Text {
                            anchors { left: parent.left; leftMargin: 6; verticalCenter: parent.verticalCenter }
                            text: modelData.name + " \u2190 " + modelData.source
                            color: modelData.enabled ? "#ddd" : "#666"
                            font.pixelSize: 10
                            font.bold: true
                            elide: Text.ElideRight
                            width: parent.width - 12
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: stripList.currentIndex = index
                            onDoubleClicked: {
                                stripList.currentIndex = index
                                startField.text = modelData.start.toFixed(2)
                                durField.text = modelData.duration.toFixed(2)
                                tsSlider.value = modelData.timescale
                                loopCheck.checked = modelData.loop
                                onCheck.checked = modelData.enabled
                                weightSlider.value = modelData.weight
                            }
                        }
                    }
                }

                // Time indicator
                Rectangle {
                    id: timeMarker
                    z: 5
                    width: 2
                    color: "#E10600"
                    visible: nlaDuration > 0
                    height: parent.height - 18
                    anchors.top: parent.top
                    anchors.topMargin: 18
                    x: 20 + (timeSlider.value / Math.max(0.1, nlaDuration)) * (parent.width - 20)
                }
            }
        }

        // Transport
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            AppButton {
                text: "\u23EE"
                width: 28; height: 24; font.pixelSize: 12
                bgcolor: "transparent"; color: "#aaa"
                onClicked: { if (Modeler) Modeler.nlaSetTime(0) }
                ToolTip.visible: hovered; ToolTip.text: "Start"
            }
            AppButton {
                text: "\u25B6"
                width: 28; height: 24; font.pixelSize: 12
                bgcolor: Modeler && Modeler.nlaPlaying() ? "#E10600" : "#3e3e42"
                color: Modeler && Modeler.nlaPlaying() ? "#121212" : "#fff"
                onClicked: {
                    if (!Modeler) return
                    if (Modeler.nlaPlaying()) Modeler.nlaPause()
                    else Modeler.nlaPlay()
                }
                ToolTip.visible: hovered; ToolTip.text: "Play / Pause"
            }
            AppButton {
                text: "\u23F9"
                width: 28; height: 24; font.pixelSize: 12
                bgcolor: "transparent"; color: "#aaa"
                onClicked: { if (Modeler) Modeler.nlaStop() }
                ToolTip.visible: hovered; ToolTip.text: "Stop"
            }

            Text {
                id: timeLabel
                text: "0:00 / 0:01"
                color: "#999"; font.pixelSize: 10; font.family: "monospace"
                width: 84
            }

            Slider {
                id: timeSlider
                Layout.fillWidth: true
                height: 20
                from: 0; to: Math.max(0.1, nlaDuration)
                value: Modeler ? Modeler.nlaTime() : 0
                onMoved: { if (Modeler) Modeler.nlaSetTime(value) }
                background: Rectangle {
                    implicitHeight: 4
                    color: "#333"; radius: 2
                    Rectangle {
                        width: parent.width * (parent.parent.value / parent.parent.to)
                        height: parent.height
                        color: "#E10600"; radius: 2
                    }
                }
                handle: Rectangle {
                    implicitWidth: 8; implicitHeight: 14; radius: 2
                    color: "#ff6666"
                    x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                    y: (parent.height - height) / 2
                }
            }
        }

        // Selected clip editor
        Rectangle {
            id: clipEditor
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: "#191919"
            border.color: "#333"
            border.width: 1
            radius: 3
            visible: stripList.currentIndex >= 0

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Text {
                    text: stripList.currentIndex >= 0 ? "Edit: " + clipList[stripList.currentIndex].name : ""
                    color: "#E10600"; font.pixelSize: 11; font.bold: true
                }

                RowLayout {
                    spacing: 6
                    Text { text: "Start:"; color: "#999"; font.pixelSize: 10 }
                    TextField {
                        id: startField
                        Layout.preferredWidth: 60
                        height: 22
                        color: "#ddd"
                        font.pixelSize: 10
                        background: Rectangle { color: "#1a1a1a"; radius: 3; border.color: "#444"; border.width: 1 }
                        validator: DoubleValidator { bottom: 0; top: 99999; decimals: 2 }
                    }
                    Text { text: "Dur:"; color: "#999"; font.pixelSize: 10 }
                    TextField {
                        id: durField
                        Layout.preferredWidth: 60
                        height: 22
                        color: "#ddd"
                        font.pixelSize: 10
                        background: Rectangle { color: "#1a1a1a"; radius: 3; border.color: "#444"; border.width: 1 }
                        validator: DoubleValidator { bottom: 0.1; top: 99999; decimals: 2 }
                    }
                    AppButton {
                        text: "Apply Range"
                        height: 22
                        bgcolor: "#3e3e42"; color: "#fff"; font.pixelSize: 10
                        onClicked: {
                            if (Modeler && stripList.currentIndex >= 0)
                                Modeler.nlaSetClipRange(stripList.currentIndex, parseFloat(startField.text), parseFloat(durField.text))
                        }
                    }
                }

                RowLayout {
                    spacing: 8
                    Text { text: "Timescale:"; color: "#999"; font.pixelSize: 10 }
                    Slider {
                        id: tsSlider
                        Layout.fillWidth: true
                        from: 0.05; to: 8; stepSize: 0.05
                        value: 1
                        onMoved: {
                            if (Modeler && stripList.currentIndex >= 0)
                                Modeler.nlaSetClipTimescale(stripList.currentIndex, value)
                        }
                        background: Rectangle {
                            implicitHeight: 3
                            color: "#333"; radius: 1
                            Rectangle { width: parent.width * (parent.parent.value / parent.parent.to); height: parent.height; color: "#E10600"; radius: 1 }
                        }
                        handle: Rectangle {
                            implicitWidth: 6; implicitHeight: 10; radius: 1
                            color: "#ff6666"
                            x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                            y: (parent.height - height) / 2
                        }
                    }
                    Text { text: tsSlider.value.toFixed(2) + "x"; color: "#888"; font.pixelSize: 10 }

                    CheckBox {
                        id: loopCheck
                        text: "Loop"
                        checked: false
                        font.pixelSize: 10
                        onToggled: {
                            if (Modeler && stripList.currentIndex >= 0)
                                Modeler.nlaSetClipLoop(stripList.currentIndex, checked)
                        }
                        indicator: Rectangle {
                            implicitWidth: 13; implicitHeight: 13; radius: 2
                            border.color: loopCheck.checked ? "#E10600" : "#555"
                            color: loopCheck.checked ? "#cc2200" : "#1a1a1a"
                            Rectangle {
                                anchors.centerIn: parent
                                width: 7; height: 7; radius: 1
                                color: loopCheck.checked ? "#fff" : "transparent"
                            }
                        }
                    }

                    CheckBox {
                        id: onCheck
                        text: "Enabled"
                        checked: true
                        font.pixelSize: 10
                        onToggled: {
                            if (Modeler && stripList.currentIndex >= 0)
                                Modeler.nlaSetClipEnabled(stripList.currentIndex, checked)
                        }
                        indicator: Rectangle {
                            implicitWidth: 13; implicitHeight: 13; radius: 2
                            border.color: onCheck.checked ? "#E10600" : "#555"
                            color: onCheck.checked ? "#cc2200" : "#1a1a1a"
                            Rectangle {
                                anchors.centerIn: parent
                                width: 7; height: 7; radius: 1
                                color: onCheck.checked ? "#fff" : "transparent"
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: 8
                    Text { text: "Weight:"; color: "#999"; font.pixelSize: 10 }
                    Slider {
                        id: weightSlider
                        Layout.fillWidth: true
                        from: 0; to: 1; stepSize: 0.05
                        value: 1
                        onMoved: {
                            if (Modeler && stripList.currentIndex >= 0)
                                Modeler.nlaSetClipWeight(stripList.currentIndex, value)
                        }
                        background: Rectangle {
                            implicitHeight: 3
                            color: "#333"; radius: 1
                            Rectangle { width: parent.width * (parent.parent.value / parent.parent.to); height: parent.height; color: "#E10600"; radius: 1 }
                        }
                        handle: Rectangle {
                            implicitWidth: 6; implicitHeight: 10; radius: 1
                            color: "#ff6666"
                            x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                            y: (parent.height - height) / 2
                        }
                    }
                    Text { text: Math.round(weightSlider.value * 100) + "%"; color: "#888"; font.pixelSize: 10 }

                    Item { width: 4 }

                    AppButton {
                        text: "Remove Clip"
                        height: 22
                        bgcolor: "transparent"; color: "#ff6666"; font.pixelSize: 10
                        onClicked: {
                            if (Modeler && stripList.currentIndex >= 0)
                                Modeler.nlaRemoveClip(stripList.currentIndex)
                        }
                    }
                }
            }
        }

        Text {
            text: "Hint: create animations, then place them as clips on this track. Layer the timeline with NLA while keeping your source clips untouched."
            color: "#666"
            font.pixelSize: 9
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
