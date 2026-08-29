import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../../widgets"
import ksEditor.KsOffice 1.0

Rectangle {
    id: officePage
    width: 1280
    height: 720
    color: "#121212"

    property bool showUserList: true
    property bool showChannelList: true
    property int activeTab: 0

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

            Rectangle {
                width: 24; height: 24; radius: 4
                color: KsOffice.connected ? "#10b981" : "#E10600"
            }

            Text {
                text: KsOffice.connected ? "ksOffice Connected" : "ksOffice Disconnected"
                color: "#ffffff"
                font.pixelSize: 14
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                spacing: 8

                TextField {
                    id: hostField
                    text: KsOffice.host
                    placeholderText: "Host"
                    implicitWidth: 120
                    implicitHeight: 30
                    font.pixelSize: 11
                    color: "#ffffff"
                    background: Rectangle { color: "#2a2a2a"; border.color: "#3e3e42"; border.width: 1 }
                    onEditingFinished: KsOffice.host = text
                }

                TextField {
                    id: portField
                    text: KsOffice.port
                    placeholderText: "Port"
                    implicitWidth: 60
                    implicitHeight: 30
                    font.pixelSize: 11
                    color: "#ffffff"
                    background: Rectangle { color: "#2a2a2a"; border.color: "#3e3e42"; border.width: 1 }
                    validator: IntValidator { bottom: 1; top: 65535 }
                    onEditingFinished: KsOffice.port = parseInt(text)
                }

                TextField {
                    id: userNameField
                    text: KsOffice.userName
                    placeholderText: "Username"
                    implicitWidth: 120
                    implicitHeight: 30
                    font.pixelSize: 11
                    color: "#ffffff"
                    background: Rectangle { color: "#2a2a2a"; border.color: "#3e3e42"; border.width: 1 }
                    onEditingFinished: KsOffice.userName = text
                }

                AppButton {
                    height: 30
                    text: KsOffice.connected ? "Disconnect" : "Connect"
                    bgcolor: KsOffice.connected ? "#E10600" : "#10b981"
                    color: "#ffffff"
                    onClicked: {
                        if (KsOffice.connected) KsOffice.disconnectFromServer()
                        else KsOffice.connectToServer()
                    }
                }

                AppButton {
                    height: 30
                    text: KsOffice.isServerRunning ? "Stop Server" : "Start Server"
                    bgcolor: KsOffice.isServerRunning ? "#b91c1c" : "#3e3e42"
                    color: "#ffffff"
                    onClicked: {
                        if (KsOffice.isServerRunning) KsOffice.stopServer()
                        else KsOffice.startServer()
                    }
                }

                Rectangle { width: 1; height: 24; color: "#3e3e42" }

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

    RowLayout {
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: statusBar.top
        spacing: 0

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

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "CHANNELS"; color: "#888888"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                    AppButton {
                        height: 22; width: 22; text: "+"; bgcolor: "#3e3e42"; color: "#ffffff"
                        onClicked: newChannelDialog.open()
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333333" }

                ListView {
                    id: channelListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: KsOffice.channels

                    delegate: Rectangle {
                        width: channelListView.width
                        height: 32
                        color: modelData.id === KsOffice.activeChannelId ? "#E10600" :
                               channelMouse.containsMouse ? "#2a2a2a" : "transparent"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8; anchors.rightMargin: 8

                            Text {
                                text: "# " + modelData.name
                                color: modelData.id === KsOffice.activeChannelId ? "#ffffff" : "#cccccc"
                                font.pixelSize: 12
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                visible: modelData.unreadCount > 0
                                width: 18; height: 18; radius: 9; color: "#E10600"
                                Text { anchors.centerIn: parent; text: modelData.unreadCount; color: "#ffffff"; font.pixelSize: 9; font.bold: true }
                            }
                        }

                        MouseArea {
                            id: channelMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: KsOffice.activeChannelId = modelData.id
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#121212"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 36
                    color: "#1a1a1a"
                    border.color: "#333333"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12; anchors.rightMargin: 12

                        Repeater {
                            model: ["Chat", "Documents", "Spreadsheet"]

                            Rectangle {
                                width: 100; height: 28; radius: 4
                                color: index === activeTab ? "#E10600" : tabMouse.containsMouse ? "#2a2a2a" : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: index === activeTab ? "#ffffff" : "#888888"
                                    font.pixelSize: 12
                                    font.bold: index === activeTab
                                }

                                MouseArea {
                                    id: tabMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: activeTab = index
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: activeTab

                    ColumnLayout {
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            height: 40
                            color: "#1a1a1a"
                            border.color: "#333333"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12; anchors.rightMargin: 12

                                Text {
                                    text: {
                                        var channels = KsOffice.channels
                                        for (var i = 0; i < channels.length; i++) {
                                            if (channels[i].id === KsOffice.activeChannelId)
                                                return "# " + channels[i].name
                                        }
                                        return "Select a channel"
                                    }
                                    color: "#ffffff"; font.pixelSize: 14; font.bold: true
                                }

                                Item { Layout.fillWidth: true }
                                Text { text: KsOffice.userCount + " online"; color: "#888888"; font.pixelSize: 11 }
                            }
                        }

                        OfficeMessageList {
                            id: messageList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }

                        Rectangle {
                            id: typingIndicator
                            Layout.fillWidth: true
                            height: typingText.text.length > 0 ? 20 : 0
                            color: "transparent"
                            visible: height > 0

                            Text {
                                id: typingText
                                anchors.left: parent.left; anchors.leftMargin: 12; anchors.bottom: parent.bottom
                                text: ""; color: "#888888"; font.pixelSize: 10; font.italic: true
                            }
                        }

                        OfficeChatInput {
                            id: chatInput
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        color: "#121212"
                        Loader {
                            anchors.fill: parent
                            source: activeTab === 1 ? "qrc:///qml/pages/page_ksDocumentEditor.qml" : ""
                            active: activeTab === 1
                        }
                    }

                    Rectangle {
                        color: "#121212"
                        Loader {
                            anchors.fill: parent
                            source: activeTab === 2 ? "qrc:///qml/pages/page_ksSpreadsheetEditor.qml" : ""
                            active: activeTab === 2
                        }
                    }
                }
            }
        }

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

                Text { text: "ONLINE — " + KsOffice.userCount; color: "#888888"; font.pixelSize: 11; font.bold: true; Layout.fillWidth: true }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#333333" }

                ListView {
                    id: userListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: KsOffice.users

                    delegate: Rectangle {
                        width: userListView.width
                        height: 36
                        color: userMouse.containsMouse ? "#2a2a2a" : "transparent"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6; anchors.rightMargin: 6; spacing: 8

                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: modelData.status === 0 ? "#10b981" : modelData.status === 1 ? "#f59e0b" : modelData.status === 2 ? "#E10600" : "#666666"
                            }

                            Rectangle {
                                width: 24; height: 24; radius: 12
                                color: modelData.color || "#666666"
                                Text { anchors.centerIn: parent; text: (modelData.name || "?")[0].toUpperCase(); color: "#ffffff"; font.pixelSize: 11; font.bold: true }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 0
                                Text { text: modelData.name || "Unknown"; color: "#ffffff"; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true }
                                Text { text: modelData.statusText || ""; color: "#888888"; font.pixelSize: 9; visible: text.length > 0 }
                            }
                        }

                        MouseArea { id: userMouse; anchors.fill: parent; hoverEnabled: true }
                    }
                }
            }
        }
    }

    Rectangle {
        id: statusBar
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 24
        color: "#1e1e1e"
        border.color: "#333333"
        border.width: 1

        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 8; anchors.rightMargin: 8

            Text {
                text: KsOffice.connected ?
                    ("Connected to " + KsOffice.host + ":" + KsOffice.port) :
                    (KsOffice.isServerRunning ? "Server running on port " + KsOffice.port : "Disconnected")
                color: KsOffice.connected ? "#10b981" : KsOffice.isServerRunning ? "#f59e0b" : "#666666"
                font.pixelSize: 10
            }

            Item { Layout.fillWidth: true }
            Text { text: "ksOffice v1.0 — Collaboration Module"; color: "#666666"; font.pixelSize: 10 }
        }
    }

    Dialog {
        id: newChannelDialog
        title: "Create Channel"
        modal: true
        anchors.centerIn: parent
        width: 350; height: 180
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
                ComboBox { id: channelTypeCombo; model: ["Group", "Project", "Direct"]; implicitWidth: 150 }
            }

            RowLayout {
                spacing: 8
                Item { Layout.fillWidth: true }
                Button { text: "Cancel"; onClicked: newChannelDialog.close() }
                Button {
                    text: "Create"
                    enabled: newChannelName.text.length > 0
                    onClicked: {
                        var types = [1, 2, 0]
                        KsOffice.createChannel(newChannelName.text, types[channelTypeCombo.currentIndex])
                        newChannelName.text = ""
                        newChannelDialog.close()
                    }
                }
            }
        }
    }

    Connections {
        target: KsOffice
        function onTypingIndicator(channelId, userId) {
            if (channelId === KsOffice.activeChannelId) {
                var user = KsOffice.getUser(userId)
                typingText.text = user.name + " is typing..."
                typingTimer.restart()
            }
        }
        function onErrorOccurred(error) {
            console.log("ksOffice error:", error)
        }
    }

    Timer { id: typingTimer; interval: 3000; onTriggered: typingText.text = "" }
}
