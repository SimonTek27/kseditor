import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: ksBasePanel
    color: "#252526"
    border.color: "#333333"
    border.width: 1
}

Rectangle {
    id: ksEffectBase
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1
}

RowLayout {
    id: ksSliderRow
    property string label: ""
    property real from: 0
    property real to: 100
    property real value: 50
    property string valueSuffix: ""
    property color valueColor: "#E10600"
    property bool showValue: true

    Text { text: label; color: "#888888" }
    Slider { Layout.fillWidth: true; from: ksSliderRow.from; to: ksSliderRow.to; value: ksSliderRow.value }
    Text { text: value + valueSuffix; color: valueColor; visible: showValue }
}

Rectangle {
    id: ksSetupPanel
    color: "#252526"
    border.color: "#333333"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        Text { text: "SETUP"; color: "#888888"; font.pixelSize: 10 }
    }
}

Rectangle {
    id: ksDialog
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1
}

Rectangle {
    id: ksSection
    color: "#252526"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
    }
}

RowLayout {
    id: AppButtonRow
    property var buttons: []
    property color bgColor: "#3e3e42"

    Button { height: 28; bgcolor: bgColor; text: "Button 1" }
    Button { height: 28; bgcolor: bgColor; text: "Button 2" }
}

Rectangle {
    id: widgetEditorCard
    color: "#252526"
    border.color: "#333333"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        Text { text: "Editor"; color: "#ffffff"; font.pixelSize: 12 }
    }
}

Rectangle {
    id: widgetModuleCard
    color: "#252526"
    border.color: "#333333"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        Text { text: "Module"; color: "#ffffff"; font.pixelSize: 12 }
    }
}