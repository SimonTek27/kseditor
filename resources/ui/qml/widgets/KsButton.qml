import QtQuick 2.15
import QtQuick.Controls

// KsButton: custom button supporting bgcolor and color properties
// Replaces the invalid use of bgcolor on standard QtQuick.Controls Button
Button {
    id: ksButton

    property color bgcolor: "transparent"
    property color color: "#ffffff"

    contentItem: Text {
        text: ksButton.text
        color: ksButton.color
        font: ksButton.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: ksButton.hovered ? Qt.lighter(ksButton.bgcolor, 1.15)
                                : ksButton.pressed ? Qt.darker(ksButton.bgcolor, 1.2)
                                                   : ksButton.bgcolor
        radius: 3
        border.color: ksButton.bgcolor === "transparent" ? "transparent" : Qt.darker(ksButton.bgcolor, 1.3)
        border.width: ksButton.bgcolor === "transparent" ? 0 : 1
    }
}
