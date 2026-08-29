import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "kschat/ChatPage"

Item {
    id: chatPageRoot
    width: 1280
    height: 720

    ChatPage {
        anchors.fill: parent
    }
}
