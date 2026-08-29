import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.KsChat 1.0

Rectangle {
    id: chatPage
    width: 1280
    height: 720
    color: "#121212"

    property bool showUserList: true
    property bool showChannelList: true

    // --- Top Bar ---
    Rectangle {
        id: topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 48
        color: "#1e1e1e"
        border.color: "#333333"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12

            // Server name / connection
            Rectangle {
                width: 24; height: 24; radius: 4
                color: KsChat.connected ? "#10b981" : "#E10600"
            }

            Text {
                text: KsChat.connected ? "ksChat Connected" : "ksChat Disconnected"
                color: "#ffffff"
                font.pixelSize: 14
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            // Connection controls
            RowLayout {
                spacing: 8

                TextField {
                    id: hostField
                    text: KsChat.host
                    placeholderText: "Host"
                    implicitWidth: 120
                    implicitHeight: 30
                    font.pixelSize: 11
                    color: "#ffffff"
                    background: Rectangle { color: "#2a2a2a"; border.color: "#3e3e42"; border.width: 1 }
                    onEditingFinished: KsChat.host = text
                }

                TextField {
                    id: portField
                    text: KsChat.port
                    placeholderText: "Port"
                    implicitWidth: 60
                    implicitHeight: 30
                    font.pixelSize: 11
                    color: "#ffffff"
                    background: Rectangle { color: "#2a2a2a"; border.color: "#3e3e42"; border.width: 1 }
                    validator: IntValidator { bottom: 1; top: 65535 }
                    onEditingFinished: KsChat.port = parseInt(text)
                }

                TextField {
                    id: userNameField
                    text: KsChat.userName
                    placeholderText: "Username"
                    implicitWidth: 120
                    implicitHeight: 30
                    font.pixelSize: 11
                    color: "#ffffff"
                    background: Rectangle { color: "#2a2a2a"; border.color: "#3e3e42"; border.width: 1 }
                    onEditingFinished: KsChat.userName = text
                }

                AppButton {
                    height: 30
                    text: KsChat.connected ? "Disconnect" : "Connect"
                    bgcolor: KsChat.connected ? "#E10600" : "#10b981"
                    color: "#ffffff"
                    onClicked: {
                        if (KsChat.connected) KsChat.disconnectFromServer()
                        else KsChat.connectToServer()
                    }
                }

                AppButton {
                    height: 30
                    text: KsChat.isServerRunning ? "Stop Server" : "Start Server"
                    bgcolor: KsChat.isServerRunning ? "#b91c1c" : "#3e3e42"
                    color: "#ffffff"
                    onClicked: {
                        if (KsChat.isServerRunning) KsChat.stopServer()
                        else KsChat.startServer()
                    }
                }

                Rectangle {
                    width: 1; height: 24
                    color: "#3e3e42"
                }

                AppButton {
                    height: 30
                    text: "Channels"
                    bgcolor: showChannelList ? "#E10600" : "#3e3e42"
                    color: "#ffffff"
                    onClicked: showChannelList = !showChannelList
                }

                AppButton {
                    height: 30
                    text: "Users"
                    bgcolor: showUserList ? "#E10600" : "#3e3e42"
                    color: "#ffffff"
                    onClicked: showUserList = !showUserList
                }
            }
        }
    }

    // --- Main Content ---
    RowLayout {
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: statusBar.top
        spacing: 0

        // --- Channel Sidebar ---
        Rectangle {
            id: channelSidebar
            width: showChannelList ? 220 : 0
            Layout.fillHeight: true
            color: "#1a1a1a"
            border.color: "#333333"
            border.width: 1
            visible: showChannelList

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                // Channel header
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "CHANNELS"
                        color: "#888888"
                        font.pixelSize: 11
                        font.bold: true
                        Layout.fillWidth: true
                    }
                    AppButton {
                        height: 22; width: 22
                        text: "+"
                        bgcolor: "#3e3e42"
                        color: "#ffffff"
                        onClicked: newChannelDialog.open()
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333333" }

                // Channel list
                ListView {
                    id: channelListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: KsChat.channels

                    delegate: Rectangle {
                        width: channelListView.width
                        height: 32
                        color: modelData.id === KsChat.activeChannelId ? "#E10600" :
                               channelMouse.containsMouse ? "#2a2a2a" : "transparent"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8

                            Text {
                                text: "# " + modelData.name
                                color: modelData.id === KsChat.activeChannelId ? "#ffffff" : "#cccccc"
                                font.pixelSize: 12
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                visible: modelData.unreadCount > 0
                                width: 18; height: 18; radius: 9
                                color: "#E10600"
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.unreadCount
                                    color: "#ffffff"
                                    font.pixelSize: 9
                                    font.bold: true
                                }
                            }
                        }

                        MouseArea {
                            id: channelMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: KsChat.activeChannelId = modelData.id
                        }
                    }
                }
            }
        }

        // --- Chat Area ---
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#121212"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 0

                // Channel header
                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#1a1a1a"
                    border.color: "#333333"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12

                        Text {
                            text: {
                                var channels = KsChat.channels
                                for (var i = 0; i < channels.length; i++) {
                                    if (channels[i].id === KsChat.activeChannelId)
                                        return "# " + channels[i].name
                                }
                                return "Select a channel"
                            }
                            color: "#ffffff"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: KsChat.userCount + " online"
                            color: "#888888"
                            font.pixelSize: 11
                        }
                    }
                }

                // Messages
                MessageList {
                    id: messageList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                // Typing indicator
                Rectangle {
                    id: typingIndicator
                    Layout.fillWidth: true
                    height: typingText.text.length > 0 ? 20 : 0
                    color: "transparent"
                    visible: height > 0

                    Text {
                        id: typingText
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.bottom: parent.bottom
                        text: ""
                        color: "#888888"
                        font.pixelSize: 10
                        font.italic: true
                    }
                }

                // Input
                ChatInput {
                    id: chatInput
                    Layout.fillWidth: true
                }
            }
        }

        // --- User Sidebar ---
        Rectangle {
            id: userSidebar
            width: showUserList ? 200 : 0
            Layout.fillHeight: true
            color: "#1a1a1a"
            border.color: "#333333"
            border.width: 1
            visible: showUserList

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                Text {
                    text: "ONLINE — " + KsChat.userCount
                    color: "#888888"
                    font.pixelSize: 11
                    font.bold: true
                    Layout.fillWidth: true
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333333" }

                ListView {
                    id: userListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: KsChat.users

                    delegate: Rectangle {
                        width: userListView.width
                        height: 36
                        color: userMouse.containsMouse ? "#2a2a2a" : "transparent"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            spacing: 8

                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: modelData.status === 0 ? "#10b981" :
                                       modelData.status === 1 ? "#f59e0b" :
                                       modelData.status === 2 ? "#E10600" : "#666666"
                            }

                            Rectangle {
                                width: 24; height: 24; radius: 12
                                color: modelData.color || "#666666"
                                Text {
                                    anchors.centerIn: parent
                                    text: (modelData.name || "?")[0].toUpperCase()
                                    color: "#ffffff"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Text {
                                    text: modelData.name || "Unknown"
                                    color: "#ffffff"
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: modelData.statusText || ""
                                    color: "#888888"
                                    font.pixelSize: 9
                                    visible: text.length > 0
                                }
                            }
                        }

                        MouseArea {
                            id: userMouse
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }
                }
            }
        }
    }

    // --- Status Bar ---
    Rectangle {
        id: statusBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 24
        color: "#1e1e1e"
        border.color: "#333333"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8

            Text {
                text: KsChat.connected ?
                    ("Connected to " + KsChat.host + ":" + KsChat.port) :
                    (KsChat.isServerRunning ? "Server running on port " + KsChat.port : "Disconnected")
                color: KsChat.connected ? "#10b981" : KsChat.isServerRunning ? "#f59e0b" : "#666666"
                font.pixelSize: 10
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "ksChat v1.0 — Collaboration Module"
                color: "#666666"
                font.pixelSize: 10
            }
        }
    }

    // --- New Channel Dialog ---
    Dialog {
        id: newChannelDialog
        title: "Create Channel"
        modal: true
        anchors.centerIn: parent
        width: 350
        height: 180

        background: Rectangle { color: "#1e1e1e"; border.color: "#3e3e42"; border.width: 1 }

        contentItem: ColumnLayout {
            spacing: 12

            Text { text: "Channel Name:"; color: "#cccccc"; font.pixelSize: 12 }

            TextField {
                id: newChannelName
                Layout.fillWidth: true
                placeholderText: "e.g. general"
                color: "#ffffff"
                background: Rectangle { color: "#2a2a2a"; border.color: "#3e3e42"; border.width: 1 }
            }

            Text { text: "Type:"; color: "#cccccc"; font.pixelSize: 12 }

            RowLayout {
                spacing: 8
                ComboBox {
                    id: channelTypeCombo
                    model: ["Group", "Project", "Direct"]
                    implicitWidth: 150
                }
            }

            RowLayout {
                spacing: 8
                Item { Layout.fillWidth: true }
                Button {
                    text: "Cancel"
                    onClicked: newChannelDialog.close()
                }
                Button {
                    text: "Create"
                    enabled: newChannelName.text.length > 0
                    onClicked: {
                        var types = [1, 2, 0] // Group=1, Project=2, Direct=0
                        KsChat.createChannel(newChannelName.text, types[channelTypeCombo.currentIndex])
                        newChannelName.text = ""
                        newChannelDialog.close()
                    }
                }
            }
        }
    }

    // --- Connections ---
    Connections {
        target: KsChat
        function onTypingIndicator(channelId, userId) {
            if (channelId === KsChat.activeChannelId) {
                var user = KsChat.getUser(userId)
                typingText.text = user.name + " is typing..."
                typingTimer.restart()
            }
        }
        function onErrorOccurred(error) {
            console.log("ksChat error:", error)
        }
    }

    Timer {
        id: typingTimer
        interval: 3000
        onTriggered: typingText.text = ""
    }
}
