import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import ksEditor.KsChat 1.0

Rectangle {
    id: bubble
    width: parent ? parent.width : 400
    height: contentColumn.height + 16
    color: hoverArea.containsMouse ? "#1e1e1e" : "transparent"
    radius: 4

    property string msgId: ""
    property string authorName: ""
    property string authorId: ""
    property string content: ""
    property string timestamp: ""
    property bool isDeleted: false
    property bool isOwnMessage: false
    property var reactions: ({})

    property var userColor: {
        var user = KsChat.getUser(authorId)
        return user.color || "#666666"
    }

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.top: parent.top
        anchors.topMargin: 8
        spacing: 4

        // Header row
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Avatar
            Rectangle {
                width: 28; height: 28; radius: 14
                color: bubble.userColor
                Text {
                    anchors.centerIn: parent
                    text: bubble.authorName.length > 0 ? bubble.authorName[0].toUpperCase() : "?"
                    color: "#ffffff"
                    font.pixelSize: 12
                    font.bold: true
                }
            }

            // Name + timestamp
            Text {
                text: bubble.authorName
                color: bubble.userColor
                font.pixelSize: 13
                font.bold: true
            }

            Text {
                text: bubble.timestamp
                color: "#666666"
                font.pixelSize: 10
            }

            Text {
                text: bubble.isDeleted ? "[deleted]" : ""
                color: "#666666"
                font.pixelSize: 10
                font.italic: true
            }

            Item { Layout.fillWidth: true }

            // Context menu button
            Rectangle {
                width: 20; height: 20; radius: 4
                color: menuBtn.containsMouse ? "#3e3e42" : "transparent"
                visible: !bubble.isDeleted

                Text {
                    anchors.centerIn: parent
                    text: "..."
                    color: "#888888"
                    font.pixelSize: 12
                }

                MouseArea {
                    id: menuBtn
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: contextMenu.popup()
                }
            }
        }

        // Message content
        Text {
            Layout.fillWidth: true
            text: bubble.isDeleted ? "This message has been deleted." : bubble.content
            color: bubble.isDeleted ? "#666666" : "#e0e0e0"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            textFormat: Text.PlainText
        }

        // Reactions
        Flow {
            Layout.fillWidth: true
            spacing: 4
            visible: Object.keys(bubble.reactions).length > 0

            Repeater {
                model: Object.keys(bubble.reactions)

                Rectangle {
                    width: reactionRow.width + 12
                    height: 22
                    radius: 11
                    color: "#2a2a2a"
                    border.color: "#3e3e42"
                    border.width: 1

                    RowLayout {
                        id: reactionRow
                        anchors.centerIn: parent
                        spacing: 4

                        Text {
                            text: modelData
                            font.pixelSize: 12
                        }

                        Text {
                            text: bubble.reactions[modelData]
                            color: "#888888"
                            font.pixelSize: 10
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            KsChat.addReaction(bubble.msgId, modelData)
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.RightButton
        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                contextMenu.popup()
            }
        }
    }

    Menu {
        id: contextMenu

        MenuItem {
            text: "Copy"
            onTriggered: {
                // Copy to clipboard
            }
        }
        MenuItem {
            text: "Reply"
            onTriggered: {
                chatInput.replyTo = bubble.msgId
            }
        }
        MenuItem {
            text: "Edit"
            visible: bubble.isOwnMessage
            onTriggered: {
                // Start edit mode
            }
        }
        MenuSeparator {}
        MenuItem {
            text: "Add Reaction"
            onTriggered: {
                reactionPicker.popup()
            }
        }
        MenuSeparator {}
        MenuItem {
            text: "Delete"
            visible: bubble.isOwnMessage
            onTriggered: {
                KsChat.deleteMessage(bubble.msgId)
            }
        }
    }

    Menu {
        id: reactionPicker

        Repeater {
            model: ["👍", "❤️", "😂", "😮", "😢", "🎉", "🔥", "✅"]

            MenuItem {
                text: modelData
                onTriggered: KsChat.addReaction(bubble.msgId, modelData)
            }
        }
    }
}
