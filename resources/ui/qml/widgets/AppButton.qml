import QtQuick 2.15
import QtQuick.Controls

// AppButton: custom button supporting bgcolor and color properties
// Replaces the invalid use of bgcolor on standard QtQuick.Controls Button
Button {
    id: AppButton

    property color bgcolor: "transparent"
    property color color: "#ffffff"

    contentItem: Text {
        text: AppButton.text
        color: AppButton.color
        font: AppButton.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: AppButton.hovered ? Qt.lighter(AppButton.bgcolor, 1.15)
                                : AppButton.pressed ? Qt.darker(AppButton.bgcolor, 1.2)
                                                   : AppButton.bgcolor
        radius: 3
        border.color: AppButton.bgcolor === "transparent" ? "transparent" : Qt.darker(AppButton.bgcolor, 1.3)
        border.width: AppButton.bgcolor === "transparent" ? 0 : 1
    }
}
