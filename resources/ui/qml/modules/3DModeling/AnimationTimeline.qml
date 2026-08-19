import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: timelinePanel
    anchors.fill: parent
    color: "transparent"

    signal closeRequested()

    property real currentTime: Modeler ? Modeler.animationTime : 0
    property real duration: Modeler ? Modeler.animationDuration : 5
    property bool isPlaying: Modeler ? Modeler.isAnimating : false
    property string animName: Modeler ? Modeler.animationName : ""

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 32
            color: "#252526"
            Layout.fillWidth: true

            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "ANIMATION TIMELINE"
                color: "#E10600"
                font.pixelSize: 11
                font.bold: true
            }

            Rectangle {
                anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                width: 18; height: 18; radius: 2; color: "#E10600"
                Text { anchors.centerIn: parent; text: "X"; color: "#fff"; font.pixelSize: 10; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: closeRequested() }
            }
        }

        // Animation selector bar
        Rectangle {
            height: 28
            color: "#181818"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4

                AppButton {
                    text: "+"
                    width: 22; height: 22
                    font.pixelSize: 12; font.bold: true
                    bgcolor: "#E10600"; color: "#121212"
                    onClicked: {
                        var count = Modeler.animationNames().length
                        Modeler.addAnimation("Animation " + (count + 1), 5.0)
                        Modeler.setCurrentAnimationByName("Animation " + (count + 1))
                    }
                    ToolTip.visible: hovered; ToolTip.text: "New animation"
                }

                ComboBox {
                    id: animCombo
                    Layout.fillWidth: true
                    height: 22
                    model: Modeler ? Modeler.animationNames() : []
                    currentIndex: {
                        var names = Modeler ? Modeler.animationNames() : []
                        for (var i = 0; i < names.length; ++i)
                            if (names[i] === Modeler.animationName) return i
                        return -1
                    }
                    font.pixelSize: 10
                    onActivated: {
                        if (Modeler) Modeler.setCurrentAnimationByName(currentText)
                    }
                    background: Rectangle {
                        color: parent.hovered ? "#333" : "#252526"
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.currentText || "(no animation)"
                        color: "#aaa"; font.bold: true; font.pixelSize: 10
                        verticalAlignment: Text.AlignVCenter; leftPadding: 6
                    }
                    indicator: Text {
                        text: "\u25BC"; color: "#666"; font.pixelSize: 8
                        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 6
                    }
                }

                AppButton {
                    text: "\u2716"
                    width: 22; height: 22
                    font.pixelSize: 10
                    bgcolor: "transparent"; color: "#ff6666"
                    onClicked: {
                        if (Modeler && animName.length > 0)
                            Modeler.deleteAnimation(animName)
                    }
                    ToolTip.visible: hovered; ToolTip.text: "Delete animation"
                }
            }
        }

        // Walk cycle generator (procedural, needs Humanoid skeleton)
        Rectangle {
            height: 30
            color: "#141414"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4

                Text {
                    text: "WALK CYCLE"
                    color: "#777"
                    font.pixelSize: 9
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                }

                TextField {
                    id: wcNameField
                    Layout.preferredWidth: 80
                    height: 20
                    text: animName.length > 0 ? animName : "Walk"
                    font.pixelSize: 9
                    color: "#fff"
                    background: Rectangle { color: "#1c1c1c"; radius: 2; border.color: "#333"; border.width: 1 }
                }

                Text { text: "Dur"; color: "#777"; font.pixelSize: 9; verticalAlignment: Text.AlignVCenter }
                Slider {
                    id: wcDurSlider
                    Layout.preferredWidth: 70
                    height: 20
                    from: 0.5; to: 4.0; stepSize: 0.1; value: 1.2
                    background: Rectangle {
                        implicitHeight: 3; color: "#333"; radius: 1
                        Rectangle { width: parent.width * (parent.parent.value - parent.parent.from) / (parent.parent.to - parent.parent.from); height: parent.height; color: "#E10600"; radius: 1 }
                    }
                    handle: Rectangle {
                        implicitWidth: 6; implicitHeight: 10; radius: 1; color: "#ff6666"
                        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                        y: (parent.height - height) / 2
                    }
                }
                Text {
                    text: wcDurSlider.value.toFixed(1) + "s"
                    color: "#888"; font.pixelSize: 9; Layout.preferredWidth: 30
                }

                Text { text: "Amp"; color: "#777"; font.pixelSize: 9; verticalAlignment: Text.AlignVCenter }
                Slider {
                    id: wcAmpSlider
                    Layout.preferredWidth: 60
                    height: 20
                    from: 0.2; to: 1.5; stepSize: 0.05; value: 1.0
                    background: Rectangle {
                        implicitHeight: 3; color: "#333"; radius: 1
                        Rectangle { width: parent.width * (parent.parent.value - parent.parent.from) / (parent.parent.to - parent.parent.from); height: parent.height; color: "#E10600"; radius: 1 }
                    }
                    handle: Rectangle {
                        implicitWidth: 6; implicitHeight: 10; radius: 1; color: "#ff6666"
                        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                        y: (parent.height - height) / 2
                    }
                }
                Text {
                    text: wcAmpSlider.value.toFixed(2)
                    color: "#888"; font.pixelSize: 9; Layout.preferredWidth: 26
                }

                AppButton {
                    text: "Generate"
                    height: 20
                    font.pixelSize: 9
                    bgcolor: "#E10600"; color: "#fff"
                    onClicked: {
                        if (Modeler) Modeler.generateWalkCycle(wcNameField.text.trim() || "Walk", wcDurSlider.value, wcAmpSlider.value)
                    }
                    ToolTip.visible: hovered; ToolTip.text: "Generate a loopable walk cycle into the animation"
                }
            }
        }

        // Animation layers bar (non-destructive blend layers)
        Rectangle {
            id: layersBar
            height: 30
            color: "#141414"
            Layout.fillWidth: true
            visible: animName.length > 0

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4

                Text {
                    text: "LAYERS"
                    color: "#777"
                    font.pixelSize: 9
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                }

                AppButton {
                    text: "+"
                    width: 22; height: 20
                    font.pixelSize: 12; font.bold: true
                    bgcolor: "#3e3e42"; color: "#fff"
                    enabled: animName.length > 0
                    onClicked: { if (Modeler) Modeler.animationAddLayer(animName, "") }
                    ToolTip.visible: hovered; ToolTip.text: "Add layer"
                }

                ListView {
                    id: layerList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: ListView.Horizontal
                    spacing: 4
                    clip: true
                    model: {
                        if (!Modeler || animName.length === 0) return []
                        return Modeler.animationLayerList(animName)
                    }

                    delegate: Rectangle {
                        height: 20
                        width: layerRow.width + 8
                        radius: 3
                        color: modelData.enabled ? "#2a2f3a" : "#222"
                        border.color: modelData.enabled ? "#445566" : "#333"

                        RowLayout {
                            id: layerRow
                            anchors.fill: parent
                            anchors.margins: 3
                            spacing: 4

                            Text {
                                text: modelData.name
                                color: modelData.enabled ? "#ddd" : "#666"
                                font.pixelSize: 10
                                font.bold: true
                            }

                            Text {
                                text: "(" + modelData.keyframeCount + "k)"
                                color: "#7777cc"
                                font.pixelSize: 9
                            }

                            CheckBox {
                                id: layerOn
                                implicitWidth: 14; implicitHeight: 14
                                padding: 0
                                checked: modelData.enabled
                                onToggled: {
                                    if (Modeler)
                                        Modeler.animationSetLayerEnabled(animName, modelData.index, checked)
                                }
                                indicator: Rectangle {
                                    implicitWidth: 12; implicitHeight: 12
                                    radius: 2
                                    border.color: layerOn.checked ? "#7777cc" : "#555"
                                    color: layerOn.checked ? "#5555cc" : "#1a1a1a"
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 6; height: 6
                                        radius: 1
                                        color: layerOn.checked ? "#fff" : "transparent"
                                    }
                                }
                            }

                            Slider {
                                id: weightSlider
                                implicitWidth: 50; implicitHeight: 14
                                from: 0; to: 1; stepSize: 0.05
                                value: modelData.weight
                                onMoved: {
                                    if (Modeler)
                                        Modeler.animationSetLayerWeight(animName, modelData.index, value)
                                }
                                background: Rectangle {
                                    implicitHeight: 3
                                    color: "#333"; radius: 1
                                    Rectangle {
                                        width: parent.width * (parent.parent.value / parent.parent.to)
                                        height: parent.height
                                        color: "#7777cc"; radius: 1
                                    }
                                }
                                handle: Rectangle {
                                    implicitWidth: 6; implicitHeight: 10; radius: 1
                                    color: "#9999ee"
                                    x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                                    y: (parent.height - height) / 2
                                }
                            }

                            Text {
                                text: Math.round(modelData.weight * 100) + "%"
                                color: "#888"
                                font.pixelSize: 9
                            }

                            AppButton {
                                text: "\u25CF"
                                width: 18; height: 18
                                font.pixelSize: 11
                                bgcolor: "#3e3e42"; color: "#cc5555"
                                enabled: Modeler && Modeler.hasSelection
                                onClicked: {
                                    if (Modeler)
                                        Modeler.addKeyframeForSelectedObjectToLayer(animName, modelData.index)
                                }
                                ToolTip.visible: hovered; ToolTip.text: "Record pose to layer"
                            }

                            AppButton {
                                text: "\u2716"
                                width: 18; height: 18
                                font.pixelSize: 10
                                bgcolor: "transparent"; color: "#ff6666"
                                onClicked: {
                                    if (Modeler) Modeler.animationRemoveLayer(animName, modelData.index)
                                }
                                ToolTip.visible: hovered; ToolTip.text: "Remove layer"
                            }
                        }
                    }
                }
            }
        }

        // Timeline ruler area
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1a1a1a"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Time ruler
                Rectangle {
                    height: 20
                    color: "#111"
                    Layout.fillWidth: true

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 2

                        Repeater {
                            model: Math.max(1, Math.floor(duration / 0.5))

                            Rectangle {
                                width: (parent.width - 30) / Math.max(1, Math.floor(duration / 0.5))
                                height: parent.height
                                color: "transparent"

                                Rectangle {
                                    width: 1; height: 6; color: "#444"
                                    anchors.left: parent.left; anchors.top: parent.top
                                }

                                Text {
                                    text: (index * 0.5).toFixed(1) + "s"
                                    color: "#555"
                                    font.pixelSize: 8
                                    anchors.left: parent.left; anchors.leftMargin: 2; anchors.top: parent.top; anchors.topMargin: 6
                                }
                            }
                        }
                    }
                }

                // Keyframe track
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#151515"

                    // Time indicator line
                    Rectangle {
                        x: 30 + (currentTime / Math.max(0.01, duration)) * (parent.width - 30)
                        width: 2; height: parent.height
                        color: "#E10600"
                        visible: duration > 0
                    }

                    // Keyframe diamond markers
                    Repeater {
                        model: {
                            if (!Modeler || animName.length === 0) return [];
                            var kfs = Modeler.currentAnimationKeyframes();
                            return kfs;
                        }

                        Rectangle {
                            x: 30 + (modelData.time / Math.max(0.01, duration)) * (parent.width - 30) - 5
                            y: (parent.height / 2) - 5
                            width: 10; height: 10
                            rotation: 45
                            color: modelData.time === currentTime ? "#E10600" : "#5555cc"
                            border.color: modelData.time === currentTime ? "#ff4444" : "#7777ee"
                            border.width: 1

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                onClicked: {
                                    if (Modeler) Modeler.setAnimationTime(modelData.time)
                                }
                                onDoubleClicked: {
                                    var kf = modelData
                                    keyframeEditor.timeValue = kf.time
                                    keyframeEditor.boneCount = kf.boneCount
                                    keyframeEditor.keyframeIndex = index
                                    keyframeEditor.open()
                                }
                            }

                            ToolTip.visible: hovered
                            ToolTip.text: modelData.time.toFixed(2) + "s (" + modelData.boneCount + " bones)"
                        }
                    }

                    // Fallback text when no keyframes exist
                    Text {
                        anchors.centerIn: parent
                        text: {
                            if (!Modeler) return "Modeler not available";
                            if (isPlaying) return "Playing...";
                            if (animName.length === 0) return "Create an animation to begin";
                            var kfs = Modeler.currentAnimationKeyframes();
                            if (kfs.length === 0) return "Select an object and press K to add keyframes";
                            return "";
                        }
                        color: "#444"
                        font.pixelSize: 10
                        visible: text.length > 0
                    }
                }

                // Playback controls
                Rectangle {
                    height: 32
                    color: "#181818"
                    Layout.fillWidth: true

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 4

                        AppButton {
                            text: "\u23EE"
                            width: 26; height: 22
                            font.pixelSize: 12
                            bgcolor: "transparent"; color: "#aaa"
                            ToolTip.visible: hovered; ToolTip.text: "Start"
                            onClicked: { if (Modeler) Modeler.setAnimationTime(0) }
                        }

                        AppButton {
                            text: "\u25B6"
                            width: 26; height: 22
                            font.pixelSize: 12
                            bgcolor: isPlaying ? "#E10600" : "#3e3e42"
                            color: isPlaying ? "#121212" : "#fff"
                            onClicked: { if (Modeler) Modeler.togglePlayPause() }
                            ToolTip.visible: hovered; ToolTip.text: isPlaying ? "Pause" : "Play (Space)"
                        }

                        AppButton {
                            text: "\u23F9"
                            width: 26; height: 22
                            font.pixelSize: 12
                            bgcolor: "transparent"; color: "#aaa"
                            ToolTip.visible: hovered; ToolTip.text: "Stop"
                            onClicked: {
                                if (Modeler) { Modeler.stopAnimation(); Modeler.setAnimationTime(0) }
                            }
                        }

                        Item { width: 4 }

                        Text {
                            text: Math.floor(currentTime) + " / " + Math.floor(duration) + "s"
                            color: "#999"
                            font.pixelSize: 10
                            font.family: "monospace"
                            width: 60
                        }

                        // Time scrubber
                        Slider {
                            id: timeSlider
                            Layout.fillWidth: true
                            height: 20
                            from: 0; to: Math.max(0.01, duration)
                            value: currentTime
                            onMoved: { if (Modeler) Modeler.setAnimationTime(value) }

                            background: Rectangle {
                                implicitHeight: 4
                                color: "#333"; radius: 2
                                Rectangle {
                                    width: parent.width * (parent.parent.value / parent.parent.to)
                                    height: parent.height
                                    color: "#5555cc"; radius: 2
                                }
                            }
                            handle: Rectangle {
                                implicitWidth: 8; implicitHeight: 14; radius: 2
                                color: "#7777dd"
                                x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                                y: (parent.height - height) / 2
                            }
                        }

                        Item { width: 4 }

                        AppButton {
                            text: "\u25CF"
                            width: 26; height: 22
                            font.pixelSize: 14
                            bgcolor: "#3e3e42"; color: "#cc5555"
                            enabled: Modeler && Modeler.hasSelection && animName.length > 0
                            onClicked: { if (Modeler) Modeler.addKeyframeForSelectedObject(animName) }
                            ToolTip.visible: hovered; ToolTip.text: "Add keyframe (K)"
                        }

                        AppButton {
                            text: "\u21BA"
                            width: 26; height: 22
                            font.pixelSize: 14
                            bgcolor: "transparent"; color: "#aaa"
                            checkable: true
                            ToolTip.visible: hovered; ToolTip.text: "Loop"
                            background: Rectangle {
                                color: parent.checked ? "#334466" : (parent.hovered ? "#333" : "transparent")
                                radius: 3
                            }
                            contentItem: Text {
                                text: parent.text; color: parent.checked ? "#88aaff" : "#888"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: { if (Modeler) Modeler.setAnimationLoop(checked) }
                        }
                    }
                }
            }
        }
    }

    // Keyframe editor popup
    Popup {
        id: keyframeEditor
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: Overlay.overlay
        width: 280
        height: 160
        padding: 0

        property real timeValue: 0
        property int boneCount: 0
        property int keyframeIndex: -1

        background: Rectangle {
            color: "#2d2d2d"
            border.color: "#555"
            border.width: 1
            radius: 4
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: "Keyframe Properties"
                color: "#E10600"
                font.pixelSize: 13
                font.bold: true
            }

            RowLayout {
                spacing: 8
                Text { text: "Time:"; color: "#aaa"; font.pixelSize: 11 }
                TextField {
                    id: timeField
                    Layout.fillWidth: true
                    text: keyframeEditor.timeValue.toFixed(2)
                    validator: DoubleValidator { bottom: 0; top: 9999; decimals: 2; notation: DoubleValidator.StandardNotation }
                    color: "#ddd"
                    background: Rectangle { color: "#1a1a1a"; radius: 3; border.color: "#444"; border.width: 1 }
                    onAccepted: {
                        keyframeEditor.timeValue = parseFloat(text)
                    }
                }
                Text { text: "s"; color: "#aaa"; font.pixelSize: 11 }
            }

            RowLayout {
                spacing: 8
                Text { text: "Bones:"; color: "#aaa"; font.pixelSize: 11 }
                Text {
                    text: keyframeEditor.boneCount
                    color: "#ddd"
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                AppButton {
                    text: "Jump to"
                    bgcolor: "#E10600"
                    color: "#fff"
                    Layout.fillWidth: true
                    onClicked: {
                        if (Modeler) {
                            Modeler.setAnimationTime(parseFloat(timeField.text))
                        }
                        keyframeEditor.close()
                    }
                }

                AppButton {
                    text: "Close"
                    bgcolor: "transparent"
                    color: "#aaa"
                    Layout.fillWidth: true
                    onClicked: keyframeEditor.close()
                }
            }
        }
    }

    Connections {
        target: Modeler
        function onAnimationTimeChanged() {
            currentTime = Modeler.animationTime
            duration = Modeler.animationDuration
            timeSlider.to = Math.max(0.01, duration)
            if (!timeSlider.pressed) timeSlider.value = currentTime
        }
        function onAnimationNameChanged() {
            animName = Modeler.animationName
            duration = Modeler.animationDuration
            animCombo.model = Modeler.animationNames()
        }
        function onPlaybackStateChanged() {
            isPlaying = Modeler.isAnimating
        }
        function onAnimationLayersChanged() {
            layerList.model = Modeler ? Modeler.animationLayerList(animName) : []
        }
    }
}


