import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import ksEditor.DisplayEditor 1.0

Rectangle {
    id: root
    color: "#1e1e1e"

    Loader {
        anchors.fill: parent
        source: "qrc:/qml/modules/car_DisplayEditor.qml"
    }
}
