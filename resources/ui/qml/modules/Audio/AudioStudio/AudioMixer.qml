import QtQuick 2.15

Item {
    id: mixerRoot
    anchors.fill: parent

    Loader {
        anchors.fill: parent
        source: "../AudioEditor/audio_Mixer.qml"
    }
}
