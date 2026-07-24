import QtQuick 2.15
import ksEditor.Audio 1.0
import ksEditor.AudioEngine 1.0

AudioStudio {
    id: audioStudioMain

    Component.onCompleted: {
        console.log("KS Audio Studio initialized")
        if (AudioEngine) {
            statusMessage = "Audio engine ready"
        }
    }
}
