import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.KsOffice 1.0

Item {
    id: messageList

    ListView {
        id: listView
        anchors.fill: parent
        anchors.margins: 8
        clip: true
        spacing: 2

        model: KsOffice.messages

        highlightFollowsCurrentItem: false

        delegate: OfficeMessageBubble {
            width: listView.width
            msgId: modelData.id || ""
            authorName: modelData.authorName || ""
            authorId: modelData.authorId || ""
            content: modelData.content || ""
            timestamp: modelData.timestamp || ""
            isDeleted: modelData.isDeleted || false
            isOwnMessage: modelData.authorId === KsOffice.userName
            reactions: modelData.reactions || ({})
        }

        onCountChanged: {
            if (count > 0) positionViewAtEnd()
        }

        Text {
            anchors.centerIn: parent
            text: "No messages yet.\nSend a message to start chatting!"
            color: "#666666"
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            visible: listView.count === 0
        }
    }

    ScrollBar {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        policy: ScrollBar.AsNeeded
        interactive: true
    }
}
