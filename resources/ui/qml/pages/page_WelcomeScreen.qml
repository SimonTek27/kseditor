import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: welcomeRoot
    width: 600
    height: 600
    color: "#252526"

    // ─── Title Bar ────────────────────────────────────────────────────────────
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 36
        color: "#1a1a1e"

        property point dragStart: Qt.point(0, 0)
        property bool dragging: false

        MouseArea {
            anchors.fill: parent
            onPressed: function(mouse) {
                if (mouse.y <= 36) {
                    titleBar.dragStart = Qt.point(mouse.x, mouse.y)
                    titleBar.dragging = true
                }
            }
            onPositionChanged: function(mouse) {
                if (titleBar.dragging) {
                    var dx = mouse.x - titleBar.dragStart.x
                    var dy = mouse.y - titleBar.dragStart.y
                    if (typeof welcomeWindow !== "undefined") {
                        welcomeWindow.x += dx
                        welcomeWindow.y += dy
                    }
                }
            }
            onReleased: titleBar.dragging = false
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: "Welcome to ksEditor"
            color: "#00ffcc"
            font.bold: true
            font.pixelSize: 13
        }

        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 6
            spacing: 4

            // Settings
            Button {
                width: 36; height: 28
                flat: true
                icon.source: "qrc:/icons/settings.svg"
                icon.width: 18; icon.height: 18
                ToolTip.text: "Settings"
                ToolTip.visible: hovered
                onClicked: WelcomeScreenBridge.openHelp()
                background: Rectangle {
                    color: parent.hovered ? "#3e3e42" : "transparent"
                    radius: 4
                }
                contentItem: Image {
                    source: parent.icon.source
                    sourceSize: Qt.size(18, 18)
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                }
            }

            // Help
            Button {
                width: 36; height: 28
                flat: true
                text: "?"
                font.bold: true
                font.pixelSize: 14
                ToolTip.text: "Help"
                ToolTip.visible: hovered
                onClicked: WelcomeScreenBridge.openHelp()
                background: Rectangle {
                    color: parent.hovered ? "#3e3e42" : "transparent"
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ccc"
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            // Minimize
            Button {
                width: 36; height: 28
                flat: true
                icon.source: "qrc:/icons/window-minimize.svg"
                icon.width: 14; icon.height: 14
                ToolTip.text: "Minimize"
                ToolTip.visible: hovered
                onClicked: {
                    if (typeof welcomeWindow !== "undefined")
                        welcomeWindow.showMinimized()
                }
                background: Rectangle {
                    color: parent.hovered ? "#3e3e42" : "transparent"
                    radius: 4
                }
                contentItem: Image {
                    source: parent.icon.source
                    sourceSize: Qt.size(14, 14)
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                }
            }

            // Close
            Button {
                width: 36; height: 28
                flat: true
                icon.source: "qrc:/icons/window-close.svg"
                icon.width: 14; icon.height: 14
                ToolTip.text: "Close"
                ToolTip.visible: hovered
                onClicked: {
                    if (typeof welcomeWindow !== "undefined")
                        welcomeWindow.close()
                }
                background: Rectangle {
                    color: parent.hovered ? "#c0392b" : "transparent"
                    radius: 4
                }
                contentItem: Image {
                    source: parent.icon.source
                    sourceSize: Qt.size(14, 14)
                    fillMode: Image.PreserveAspectFit
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                }
            }
        }
    }

    // ─── Banner ───────────────────────────────────────────────────────────────
    Rectangle {
        id: banner
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 180
        color: "#1a1a1e"

        Image {
            anchors.centerIn: parent
            source: "qrc:/assets/welcome.png"
            fillMode: Image.PreserveAspectFit
            sourceSize: Qt.size(600, 180)
        }
    }

    // ─── Content ──────────────────────────────────────────────────────────────
    ScrollView {
        anchors.top: banner.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 0

            // Subtitle
            Item { height: 20; Layout.fillWidth: true }
            Text {
                text: "Assetto Corsa Modding Suite"
                color: "#aaaaaa"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
            }

            Item { height: 20; Layout.fillWidth: true }

            GridLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                columns: 3
                columnSpacing: 10
                rowSpacing: 10

                Repeater {
                    model: [
                        { icon: "qrc:/icons/modeler.svg",      label: "3D",            tooltip: "Open 3D Modeler",       mode: "modeler" },
                        { icon: "qrc:/icons/livery.svg",       label: "Livery",        tooltip: "Livery Editor",         mode: "" },
                        { icon: "qrc:/icons/licenseplate.svg", label: "License\nPlate", tooltip: "License Plate Editor",  mode: "" },
                        { icon: "qrc:/icons/font.svg",         label: "Font\nCreator", tooltip: "Font Creator",          mode: "font" },
                        { icon: "qrc:/icons/display.svg",      label: "Display",      tooltip: "Display Editor",        mode: "" },
                        { icon: "qrc:/icons/physics.svg",      label: "Physics",      tooltip: "Physics Editor",        mode: "physics" },
                        { icon: "qrc:/icons/sound.svg",        label: "Audio\nStudio", tooltip: "Audio Studio",          mode: "audiostudio" },
                        { icon: "qrc:/icons/waveform.svg",     label: "Audio\nEditor", tooltip: "Audio Editor",          mode: "audioeditor" },
                        { icon: "qrc:/icons/preview.svg",      label: "Preview\nGenerator", tooltip: "Preview Generator", mode: "" }
                    ]
                    Button {
                        required property var modelData
                        required property int index
                        width: 90; height: 90
                        flat: true
                        ToolTip.text: modelData.tooltip
                        ToolTip.visible: hovered
                        onClicked: {
                            if (modelData.mode !== "")
                                WelcomeScreenBridge.launchApp(modelData.mode)
                        }
                        background: Rectangle {
                            color: parent.hovered ? "#3e3e42" : "#2a2a2e"
                            border.color: parent.hovered ? "#555" : "#3f3f46"
                            border.width: 1
                            radius: 6
                        }
                        contentItem: ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Image {
                                source: modelData.icon
                                sourceSize: Qt.size(32, 32)
                                fillMode: Image.PreserveAspectFit
                                Layout.alignment: Qt.AlignHCenter
                            }
                            Text {
                                text: modelData.label
                                color: "white"
                                font.pixelSize: 10
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }

            // ── RECENT PROJECTS ──────────────────────────────────────────────
            Item { height: 16; Layout.fillWidth: true }
            Text {
                text: "RECENT PROJECTS"
                color: "#888"
                font.pixelSize: 11
                font.bold: true
                font.letterSpacing: 2
                Layout.alignment: Qt.AlignHCenter
            }
            Item { height: 8; Layout.fillWidth: true }

            Rectangle {
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                Layout.maximumHeight: 150
                color: "#1a1a1e"
                border.color: "#3f3f46"
                border.width: 1
                radius: 6

                ListView {
                    id: recentList
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    model: WelcomeScreenBridge.recentProjects

                    delegate: Rectangle {
                        width: recentList.width
                        height: 32
                        color: recentMouse.containsMouse ? "#2a2a2e" : "transparent"

                        MouseArea {
                            id: recentMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onDoubleClicked: {
                                WelcomeScreenBridge.openRecent(modelData.path)
                            }
                        }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            Image {
                                source: {
                                    switch (modelData.category) {
                                    case "car": return "qrc:/icons/car-model.svg"
                                    case "track": return "qrc:/icons/track.svg"
                                    case "character": return "qrc:/icons/character.svg"
                                    case "sound": return "qrc:/icons/audio.svg"
                                    default: return "qrc:/icons/folder.svg"
                                    }
                                }
                                sourceSize: Qt.size(16, 16)
                                fillMode: Image.PreserveAspectFit
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                text: modelData.displayName
                                color: "#d4d4d8"
                                font.pixelSize: 12
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 1
                            color: "#2a2a2e"
                        }
                    }

                    // Empty state
                    Text {
                        anchors.centerIn: parent
                        text: recentList.count === 0 ? "No recent projects" : ""
                        color: "#71717a"
                        font.pixelSize: 12
                        visible: recentList.count === 0
                    }
                }
            }

            // ── Help Button ──────────────────────────────────────────────────
            Item { height: 16; Layout.fillWidth: true }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button {
                    width: 120; height: 36
                    text: "Help"
                    icon.source: "qrc:/icons/help.svg"
                    icon.width: 18; icon.height: 18
                    onClicked: WelcomeScreenBridge.openHelp()
                    background: Rectangle {
                        color: parent.hovered ? "#4e4e52" : "#3e3e42"
                        border.color: "#555"
                        border.width: 1
                        radius: 4
                    }
                    contentItem: RowLayout {
                        spacing: 6
                        Image {
                            source: parent.parent.icon.source
                            sourceSize: Qt.size(18, 18)
                            fillMode: Image.PreserveAspectFit
                        }
                        Text {
                            text: parent.parent.text
                            color: "white"
                            font.pixelSize: 13
                        }
                    }
                }
                Item { Layout.fillWidth: true }
            }
            Item { height: 20; Layout.fillWidth: true }
        }
    }
}
