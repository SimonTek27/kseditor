import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#1e1e1e"

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 16

        Text {
            text: "Livery Editor"
            color: "#ffffff"
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "Module not yet available in QML.\nUse the embedded C++ LiveryEditor widget."
            color: "#888888"
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
