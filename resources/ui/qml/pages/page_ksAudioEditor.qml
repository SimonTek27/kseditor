import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import ksEditor.Audio 1.0
import ksEditor.AudioEffects 1.0

Rectangle {
    id: root
    color: "#1e1e1e"

    Loader {
        anchors.fill: parent
        source: "qrc:/qml/modules/Audio/AudioEditor/audio_Editor.qml"
    }
}
