import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../widgets"
import ksEditor.Collaboration 1.0

Rectangle {
    id: collabEditor
    width: 1280
    height: 720
    color: "#1a1a1a"

    property string currentProject: ""
    property bool isConnected: false
    property string activePanel: "session"

    ListModel { id: historyModel }
    ListModel { id: conflictsModel }
    ListModel { id: chatModel }

    // Project selector
    Rectangle {
        Layout.fillWidth: true
        height: 80
        color: "#252526"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20

            ComboBox {
                id: projectCombo
                model: ["Project A", "Project B", "Project C", "Project D"]
                preferredWidth: 300
                onActivated: {
                    currentProject = modelData
                    if (CollabEditor) {
                        CollabEditor.loadProject(modelData)
                    }
                }
            }

            Button {
                text: "Refresh"
                flat: true
                onClicked: {
                    // Refresh projects
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            // --- Toolbar ---
            Rectangle {
                Layout.fillWidth: true
                height: 50
                color: "#1e1e1e"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10

                    AppButton {
                        text: isConnected ? "Disconnect" : "Connect"
                        flat: true
                        onClicked: {
                            if (CollabEditor) {
                                if (isConnected) {
                                    CollabEditor.disconnect()
                                } else {
                                    CollabEditor.setHost(hostField.text)
                                    CollabEditor.setPort(parseInt(portField.text))
                                    CollabEditor.setUserName(userNameField.text)
                                    CollabEditor.connectToServer()
                                }
                                isConnected = !isConnected
                            }
                        }
                    }
                }
            }

            // --- Main Content ---
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // --- Left Panel ---
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

                        AppButton {
                            height: 28
                            text: "Session"
                            bgcolor: activePanel === "session" ? "#E10600" : "#3e3e42"
                            color: activePanel === "session" ? "#121212" : "#ffffff"
                            onClicked: activePanel = "session"
                        }
                        AppButton {
                            height: 28
                            text: "Users"
                            bgcolor: activePanel === "users" ? "#E10600" : "#3e3e42"
                            color: activePanel === "users" ? "#121212" : "#ffffff"
                            onClicked: activePanel = "users"
                        }
                        AppButton {
                            height: 28
                            text: "Chat"
                            bgcolor: activePanel === "chat" ? "#E10600" : "#3e3e42"
                            color: activePanel === "chat" ? "#121212" : "#ffffff"
                            onClicked: activePanel = "chat"
                        }
                        AppButton {
                            height: 28
                            text: "History"
                            bgcolor: activePanel === "history" ? "#E10600" : "#3e3e42"
                            color: activePanel === "history" ? "#121212" : "#ffffff"
                            onClicked: activePanel = "history"
                        }
                        AppButton {
                            height: 28
                            text: "Conflicts"
                            bgcolor: activePanel === "conflicts" ? "#E10600" : "#3e3e42"
                            color: activePanel === "conflicts" ? "#121212" : "#ffffff"
                            onClicked: activePanel = "conflicts"
                        }
                        AppButton {
                            height: 28
                            text: "Permissions"
                            bgcolor: activePanel === "permissions" ? "#E10600" : "#3e3e42"
                            color: activePanel === "permissions" ? "#121212" : "#ffffff"
                            onClicked: activePanel = "permissions"
                        }
                    }
                }

                // --- Center Panel ---
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#2a2a2a"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        anchors.rightMargin: 20
                        spacing: 15

                        // Session panel
                        if (activePanel === "session") {
                            Text {
                                text: "SESSION MANAGER"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                color: "#252526"
                                border.color: "#3e3e42"
                                border.width: 1

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    RowLayout {
                                        Text { text: "Host:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                        TextField { id: hostField; text: CollabEditor ? CollabEditor.host : "localhost"; Layout.fillWidth: true }
                                    }

                                    RowLayout {
                                        Text { text: "Port:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                        TextField { id: portField; text: CollabEditor ? String(CollabEditor.port) : "8080"; width: 80 }
                                    }

                                    RowLayout {
                                        Text { text: "User:"; color: "#bbbbbb"; Layout.preferredWidth: 70 }
                                        TextField { id: userNameField; text: CollabEditor ? CollabEditor.userName : "User"; Layout.fillWidth: true }
                                    }
                                }
                            }
                        }

                        if (activePanel === "users") {
                            Text {
                                text: "ONLINE USERS"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "#252526"
                                border.color: "#3e3e42"
                                border.width: 1

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 5
                                    spacing: 2

                                    Repeater {
                                        model: CollabEditor ? CollabEditor.users : []
                                        delegate: Rectangle {
                                            height: 28
                                            color: "#2d2d2d"
                                            Layout.fillWidth: true

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.margins: 8

                                                Rectangle { width: 8; height: 8; radius: 4; color: modelData.isOnline ? "#E10600" : "#666" }
                                                Text { text: modelData.name; color: "#ffffff"; Layout.fillWidth: true }
                                                Text { text: modelData.role === 3 ? "Owner" : modelData.role === 2 ? "Admin" : modelData.role === 1 ? "Editor" : "Viewer"; color: "#888888"; font.pixelSize: 10 }
                                            }
                                        }
                                    }

                                    Rectangle {
                                        height: 28
                                        color: "transparent"
                                        Layout.fillWidth: true
                                        visible: !CollabEditor || CollabEditor.users.length === 0

                                        Text {
                                            anchors.centerIn: parent
                                            text: "No users connected"
                                            color: "#666"
                                            font.pixelSize: 10
                                        }
                                    }
                                }
                            }
                        }

                        if (activePanel === "chat") {
                            Text {
                                text: "CHAT"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "#1e1e1e"
                                border.color: "#333333"
                                border.width: 1

                                ScrollView {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    clip: true

                                    ColumnLayout {
                                        spacing: 2
                                        Repeater {
                                            model: chatModel
                                            delegate: Text {
                                                text: modelData.text
                                                color: "#cccccc"
                                                font.pixelSize: 10
                                                wrapMode: Text.WordWrap
                                                Layout.fillWidth: true
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    TextField {
                                        id: chatInput
                                        Layout.fillWidth: true
                                        placeholderText: "Type a message..."
                                        onAccepted: {
                                            if (CollabEditor && text.length > 0) {
                                                CollabEditor.sendChatMessage(text)
                                                text = ""
                                            }
                                        }
                                    }
                                    AppButton {
                                        height: 36
                                        text: "Send"
                                        bgcolor: "#E10600"
                                        color: "#121212"
                                        onClicked: {
                                            if (CollabEditor && chatInput.text.length > 0) {
                                                CollabEditor.sendChatMessage(chatInput.text)
                                                chatInput.text = ""
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (activePanel === "history") {
                            Text {
                                text: "CHANGES HISTORY"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "#252526"
                                border.color: "#333333"
                                border.width: 1

                                ListView {
                                    anchors.fill: parent
                                    anchors.margins: 5
                                    clip: true
                                    model: historyModel

                                    Component.onCompleted: {
                                        if (CollabEditor) {
                                            var data = CollabEditor.getHistory()
                                            for (var i = 0; i < data.length; i++)
                                                historyModel.append(data[i])
                                        }
                                    }

                                    delegate: Rectangle {
                                        height: 24
                                        color: "#2d2d2d"
                                        width: parent.width
                                        Text {
                                            text: (modelData.timestamp || "") + " " + (modelData.user || "") + " " + (modelData.description || "")
                                            color: "#888"
                                            font.pixelSize: 10
                                            anchors.left: parent.left
                                            anchors.leftMargin: 8
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: "No history available"
                                        color: "#666"
                                        font.pixelSize: 10
                                        visible: historyModel.count === 0
                                    }
                                }
                            }
                        }

                        if (activePanel === "conflicts") {
                            Text {
                                text: "CONFLICT RESOLVER"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "#252526"
                                border.color: "#333333"
                                border.width: 1

                                ListView {
                                    anchors.fill: parent
                                    anchors.margins: 5
                                    clip: true
                                    model: conflictsModel

                                    Component.onCompleted: {
                                        if (CollabEditor) {
                                            var data = CollabEditor.getConflicts()
                                            for (var i = 0; i < data.length; i++)
                                                conflictsModel.append(data[i])
                                        }
                                    }

                                    delegate: Rectangle {
                                        height: 28
                                        color: "#2d2d2d"
                                        width: parent.width
                                        Text {
                                            text: modelData.description || "Unknown conflict"
                                            color: "#E10600"
                                            font.pixelSize: 11
                                            anchors.left: parent.left
                                            anchors.leftMargin: 8
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: "No conflicts"
                                        color: "#666"
                                        font.pixelSize: 12
                                        visible: conflictsModel.count === 0
                                    }
                                }
                            }

                            AppButton {
                                height: 32
                                text: "Auto-Resolve"
                                bgcolor: "#E10600"
                                color: "#121212"
                                onClicked: {
                                    if (CollabEditor) CollabEditor.resolveConflicts()
                                }
                            }
                        }

                        if (activePanel === "permissions") {
                            Text {
                                text: "PERMISSIONS"
                                color: "white"
                                font.bold: true
                                font.pixelSize: 14
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                color: "#252526"
                                border.color: "#3e3e42"
                                border.width: 1

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 8

                                    RowLayout {
                                        CheckBox {
                                            id: canEditPerm
                                            checked: CollabEditor ? CollabEditor.getPermissions().canEdit : true
                                            onCheckedChanged: { if (CollabEditor) CollabEditor.setPermission("self", "canEdit", checked) }
                                        }
                                        Text { text: "Can Edit"; color: "#bbbbbb" }
                                    }
                                    RowLayout {
                                        CheckBox {
                                            id: canDeletePerm
                                            checked: CollabEditor ? CollabEditor.getPermissions().canDelete : true
                                            onCheckedChanged: { if (CollabEditor) CollabEditor.setPermission("self", "canDelete", checked) }
                                        }
                                        Text { text: "Can Delete"; color: "#bbbbbb" }
                                    }
                                    RowLayout {
                                        CheckBox {
                                            id: canInvitePerm
                                            checked: CollabEditor ? CollabEditor.getPermissions().canInvite : false
                                            onCheckedChanged: { if (CollabEditor) CollabEditor.setPermission("self", "canInvite", checked) }
                                        }
                                        Text { text: "Can Invite"; color: "#bbbbbb" }
                                    }
                                    RowLayout {
                                        CheckBox {
                                            id: adminPerm
                                            checked: CollabEditor ? CollabEditor.getPermissions().admin : false
                                            onCheckedChanged: { if (CollabEditor) CollabEditor.setPermission("self", "admin", checked) }
                                        }
                                        Text { text: "Admin"; color: "#bbbbbb" }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // --- Status Bar ---
            Rectangle {
                height: 24
                color: "#252526"
                Layout.fillWidth: true

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 4

                    Text {
                        text: isConnected ? ("Connected to " + hostField.text + ":" + portField.text) : "Disconnected"
                        color: isConnected ? "#10b981" : "#E10600"
                        font.pixelSize: 10
                    }
                    Item { Layout.fillWidth: true }
                    Text { text: "ksEditor v1.0 - Collaboration"; color: "#666"; font.pixelSize: 10 }
                }
            }

            // Connections
            Connections {
                target: CollabEditor
                function onChatMessageReceived(userId, message) {
                    chatModel.append({text: userId + ": " + message})
                }
                function onErrorOccurred(msg) {
                    chatModel.append({text: "ERROR: " + msg})
                }
                function onUserJoined(id, name) {
                    chatModel.append({text: "* " + name + " joined *"})
                }
                function onUserLeft(id, name) {
                    chatModel.append({text: "* " + name + " left *"})
                }
            }
        }
    }
}