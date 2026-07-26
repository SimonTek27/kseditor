import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import ksEditor.Physics 1.0
import ksEditor.PhysicsProfiler 1.0
import ksEditor.Telemetry 1.0
import ksEditor.TelemetryFeedback 1.0
import ksEditor.LapTimeValidation 1.0

ApplicationWindow {
    id: studio
    width: 1400
    height: 900
    minimumWidth: 1024
    minimumHeight: 600
    title: "KS Physics Studio - " + (Physics ? Physics.currentFile : "untitled")
    visible: true
    color: "#111111"

    Loader {
        anchors.fill: parent
        source: "qrc:///qml/modules/physicsEditor/phys_Editor.qml"
    }
}
