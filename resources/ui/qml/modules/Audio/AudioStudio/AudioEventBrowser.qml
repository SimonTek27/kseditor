import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import ksEditor.Audio 1.0
import "../../../widgets"

Item {
    id: eventBrowser
    anchors.fill: parent

    readonly property color cAccent: "#E10600"
    readonly property color cMuted: "#666666"
    readonly property color cText: "#cccccc"
    readonly property color cBorder: "#333333"

    property string selectedEvent: ""
    property var eventCategories: buildEventCategories()

    function buildEventCategories() {
        if (typeof eventDefs !== "undefined" && eventDefs) {
            var result = [];
            var cats = eventDefs.categories;
            for (var ci = 0; ci < cats.length; ci++) {
                var evs = eventDefs.eventsByCategory(cats[ci]);
                result.push({ name: cats[ci], icon: "\u266B", collapsed: ci >= 8, events: evs });
            }
            return result;
        }
        return [
            { name: "Engine", icon: "\u2699", collapsed: false, events: ["engine_int","engine_ext","turbo","turbo_ext","limiter","gear_ext","gear_int","gear_grind","starter_ext","starter_int","ignition_ext","ignition_int","misc_int"] },
            { name: "Body", icon: "\uF0C2", collapsed: false, events: ["door","horn","bodywork","chassis_ext","chassis_int"] },
            { name: "Backfire", icon: "\uF0E7", collapsed: false, events: ["backfire_ext","backfire_int"] },
            { name: "Tires", icon: "\uF1B9", collapsed: false, events: ["skid_ext","skid_int","wheel","tractioncontrol_ext","tractioncontrol_int"] },
            { name: "Transmission", icon: "\uF085", collapsed: false, events: ["transmission","transmission_ext"] },
            { name: "Brakes", icon: "\uF0A7", collapsed: false, events: ["brakes"] },
            { name: "Hybrid", icon: "\uF0E7", collapsed: false, events: ["hybrid_ext","hybrid_int"] },
            { name: "Environment", icon: "\uF74E", collapsed: false, events: ["wind"] },
            { name: "CSP Rain", icon: "\uF743", collapsed: true, events: ["rain_amb","rain_amb_thunder","rain_car_ext","rain_car_int","rain_grass","rain_gravel","rain_skid_ext","rain_skid_int"] },
            { name: "CSP Vehicle", icon: "\uF1B9", collapsed: true, events: ["turn_signal_ext__off","turn_signal_int__off","turn_signal_int","wiper_car_ext","wiper_car_ext_vintage","wiper_car_int","wiper_car_int_vintage","handbrake_int"] },
            { name: "CSP Wind", icon: "\uF74E", collapsed: true, events: ["external_wind"] },
            { name: "CSP Surfaces", icon: "\uF7A0", collapsed: true, events: ["csp_surfaces_skid","csp_surfaces_force","csp_surfaces_rocks","csp_surfaces_ice"] }
        ];
    }

    function eventInfo(ev) {
        if (typeof eventDefs !== "undefined" && eventDefs) {
            var info = eventDefs.eventInfo(ev);
            if (info && info.name) return info;
        }
        var map = {
            "engine_int":     { cat: "Engine", desc: "Engine interior" },
            "engine_ext":     { cat: "Engine", desc: "Engine exterior" },
            "turbo":          { cat: "Engine", desc: "Turbo" },
            "turbo_ext":      { cat: "Engine", desc: "Turbo exterior" },
            "limiter":        { cat: "Engine", desc: "Rev limiter" },
            "gear_ext":       { cat: "Engine", desc: "Gear change ext" },
            "gear_int":       { cat: "Engine", desc: "Gear change int" },
            "gear_grind":     { cat: "Engine", desc: "Gear grinding" },
            "starter_ext":    { cat: "Engine", desc: "Starter ext" },
            "starter_int":    { cat: "Engine", desc: "Starter int" },
            "ignition_ext":   { cat: "Engine", desc: "Ignition ext" },
            "ignition_int":   { cat: "Engine", desc: "Ignition int" }
        };
        return map[ev] || { cat: "Unknown", desc: ev };
    }

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        Rectangle {
            height: 32; color: "#252526"; Layout.fillWidth: true
            Text {
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                text: "CAR AUDIO EVENTS"; color: cAccent; font.pixelSize: 11; font.bold: true
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            Rectangle {
                Layout.preferredWidth: 200; Layout.fillHeight: true
                color: "#1e1e1e"; border.color: cBorder; border.width: 1

                ScrollView {
                    anchors.fill: parent; anchors.margins: 6; clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width - 12; spacing: 2

                        Text { text: "CATEGORIES"; color: cMuted; font.pixelSize: 9; font.bold: true; leftPadding: 4; topPadding: 4 }

                        Repeater {
                            model: eventCategories
                            delegate: ColumnLayout {
                                width: parent.width; spacing: 1

                                AppButton {
                                    height: 22; text: modelData.icon + "  " + modelData.name + " (" + modelData.events.length + ")"
                                    font.pixelSize: 9; bgcolor: "#2e2e32"; color: "#aaa"
                                    onClicked: {
                                        var arr = eventCategories
                                        arr[index].collapsed = !arr[index].collapsed
                                        eventCategories = arr
                                    }
                                }

                                Repeater {
                                    model: modelData.collapsed ? [] : modelData.events
                                    delegate: AppButton {
                                        height: 20; text: "  " + modelData; font.pixelSize: 8; leftPadding: 16
                                        bgcolor: selectedEvent === modelData ? cAccent : "transparent"
                                        color: selectedEvent === modelData ? "#121212" : "#888"
                                        onClicked: {
                                            selectedEvent = modelData
                                            if (AudioBridge) AudioBridge.loadAudio("events/" + modelData + ".wav")
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: "#1a1a1a"; border.color: cBorder; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16; spacing: 8
                    visible: selectedEvent !== ""

                    Text { text: selectedEvent; color: cAccent; font.pixelSize: 18; font.bold: true }
                    Text { text: eventInfo(selectedEvent).desc; color: cMuted; font.pixelSize: 11 }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    RowLayout { Text { text: "Category:"; color: cMuted; font.pixelSize: 10 }
                        Text { text: eventInfo(selectedEvent).cat; color: cText; font.pixelSize: 10 } }
                    RowLayout { Text { text: "Volume:"; color: cMuted; font.pixelSize: 10; Layout.preferredWidth: 50 }
                        Slider { from: 0; to: 100; value: 80; Layout.fillWidth: true; height: 16 } }
                    RowLayout { Text { text: "Looping:"; color: cMuted; font.pixelSize: 10 }
                        Rectangle { width: 50; height: 18; radius: 2; color: "#3e3e42"
                            Text { anchors.centerIn: parent; text: "Off"; color: "#888"; font.pixelSize: 8 } } }

                    Rectangle { height: 1; color: cBorder; Layout.fillWidth: true }

                    RowLayout {
                        spacing: 8
                        AppButton { height: 28; text: "Play"; bgcolor: cAccent; color: "#121212"
                            onClicked: { if (AudioBridge) AudioBridge.play() } }
                        AppButton { height: 28; text: "Stop"; bgcolor: "transparent"; color: cText
                            onClicked: { if (AudioBridge) AudioBridge.stop() } }
                        AppButton { height: 28; text: "Browse..."; bgcolor: "transparent"; color: cText
                            onClicked: fileDialog.open() }
                        Item { Layout.fillWidth: true }
                        AppButton { height: 28; text: "Assign to Event"; bgcolor: cAccent; color: "#121212" }
                    }

                    FileDialog {
                        id: fileDialog
                        title: "Select Audio File"
                        nameFilters: ["Audio files (*.wav *.mp3 *.ogg *.flac)", "All files (*)"]
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "Select an event from the left panel"
                    color: cMuted; font.pixelSize: 12
                    visible: selectedEvent === ""
                }
            }
        }
    }
}
