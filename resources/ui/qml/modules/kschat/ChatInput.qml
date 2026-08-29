import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.KsChat 1.0

Rectangle {
    id: chatInput
    height: replyBar.height + inputRow.height + 16
    color: "#1a1a1a"
    border.color: "#333333"
    border.width: 1

    property string replyTo: ""

    // Reply bar
    Rectangle {
        id: replyBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: replyTo.length > 0 ? 28 : 0
        color: "#252526"
        visible: height > 0

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8

            Rectangle { width: 3; height: 16; color: "#E10600" }

            Text {
                text: "Replying to message"
                color: "#cccccc"
                font.pixelSize: 11
                Layout.fillWidth: true
            }

            AppButton {
                height: 20; width: 20
                text: "x"
                bgcolor: "transparent"
                color: "#888888"
                onClicked: chatInput.replyTo = ""
            }
        }
    }

    // Input row
    RowLayout {
        id: inputRow
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: replyBar.bottom
        anchors.margins: 8
        spacing: 8

        // Plus button for attachments
        Rectangle {
            width: 32; height: 32; radius: 16
            color: plusMouse.containsMouse ? "#3e3e42" : "#2a2a2a"

            Text {
                anchors.centerIn: parent
                text: "+"
                color: "#888888"
                font.pixelSize: 18
                font.bold: true
            }

            MouseArea {
                id: plusMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: attachMenu.popup()
            }
        }

        // Text input
        Rectangle {
            Layout.fillWidth: true
            height: 36
            color: "#2a2a2a"
            border.color: inputField.activeFocus ? "#E10600" : "#3e3e42"
            border.width: 1
            radius: 18

            TextInput {
                id: inputField
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                color: "#ffffff"
                font.pixelSize: 13
                clip: true
                verticalAlignment: TextInput.AlignVCenter

                property int cursorPosition: length

                Keys.onReturnPressed: {
                    if (text.length > 0) {
                        KsChat.sendMessage(text, chatInput.replyTo)
                        text = ""
                        chatInput.replyTo = ""
                    }
                }

                Keys.onEnterPressed: {
                    if (text.length > 0) {
                        KsChat.sendMessage(text, chatInput.replyTo)
                        text = ""
                        chatInput.replyTo = ""
                    }
                }

                onTextChanged: {
                    if (text.length > 0) {
                        KsChat.sendTyping()
                    }
                }
            }

            // Placeholder
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                text: "Message #" + getActiveChannelName()
                color: "#666666"
                font.pixelSize: 13
                visible: inputField.text.length === 0 && !inputField.activeFocus

                function getActiveChannelName() {
                    var channels = KsChat.channels
                    for (var i = 0; i < channels.length; i++) {
                        if (channels[i].id === KsChat.activeChannelId)
                            return channels[i].name
                    }
                    return ""
                }
            }
        }

        // Send button
        Rectangle {
            width: 36; height: 36; radius: 18
            color: sendMouse.containsMouse && inputField.text.length > 0 ? "#E10600" :
                   inputField.text.length > 0 ? "#c00500" : "#2a2a2a"

            Text {
                anchors.centerIn: parent
                text: "\u2191"
                color: "#ffffff"
                font.pixelSize: 18
                font.bold: true
            }

            MouseArea {
                id: sendMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (inputField.text.length > 0) {
                        KsChat.sendMessage(inputField.text, chatInput.replyTo)
                        inputField.text = ""
                        chatInput.replyTo = ""
                    }
                }
            }
        }
    }

    Menu {
        id: attachMenu

        MenuItem {
            text: "Send File..."
            onTriggered: fileDialog.open()
        }
        MenuItem {
            text: "Send Image..."
            onTriggered: imageDialog.open()
        }
        MenuSeparator {}
        MenuItem {
            text: "Code Block"
            onTriggered: {
                inputField.text += "```\n\n```"
                inputField.cursorPosition = inputField.text.length - 4
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Select File"
        nameFilters: ["All files (*)"]
        onAccepted: {
            // Handle file upload
        }
    }

    FileDialog {
        id: imageDialog
        title: "Select Image"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.gif *.webp)"]
        onAccepted: {
            // Handle image upload
        }
    }
}
