import QtQuick 2.15
import ksEditor.Modeler 1.0

ModelerStudio {
    id: modelerMain

    Component.onCompleted: {
        console.log("KS Modeler Studio initialized")
        if (Modeler) {
            Modeler.newScene()
        }
    }
}
