import QtQuick 2.15
import ksEditor.Modeler 1.0

ModelerStudio {
    id: modelerMain

    Component.onCompleted: {
        if (Modeler) {
            Modeler.newScene()
        }
    }
}
