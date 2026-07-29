import QtQuick 2.15
import ksEditor.Physics 1.0

PhysicsStudio {
    id: physicsStudioMain

    Component.onCompleted: {
        if (Physics) {
            Physics.statusMessage = "Physics engine ready"
        }
    }
}
