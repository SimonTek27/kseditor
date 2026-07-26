import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Window
import ksEditor.FontCreator 1.0

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    minimumWidth: 900
    minimumHeight: 500
    title: "KS Font Editor"
    visible: true
    color: "#121212"

    Loader {
        anchors.fill: parent
        source: "qrc:///qml/modules/font_KSFontCreator.qml"
    }
}
