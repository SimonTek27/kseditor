import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: audioStudio
    color: "#1e1e1e"

    property string activeSystem: "Engine"

    function buildSystems() {
        if (typeof eventDefs !== "undefined" && eventDefs) {
            var result = [];
            var cats = eventDefs.categories;
            for (var ci = 0; ci < cats.length; ci++) {
                var evs = eventDefs.eventsByCategory(cats[ci]);
                result.push({ key: cats[ci], icon: "\u266A", events: evs });
            }
            return result;
        }
        return [
            { key: "Engine",       icon: "\u2699", events: ["engine_int","engine_ext","turbo","turbo_ext","limiter","gear_ext","gear_int","gear_grind","starter_ext","starter_int","ignition_ext","ignition_int","misc_int"] },
            { key: "Body",         icon: "\uF0C2", events: ["door","horn","bodywork","chassis_ext","chassis_int"] },
            { key: "Backfire",     icon: "\uF0E7", events: ["backfire_ext","backfire_int"] },
            { key: "Tires",        icon: "\uF1B9", events: ["skid_ext","skid_int","wheel","tractioncontrol_ext","tractioncontrol_int"] },
            { key: "Transmission", icon: "\uF085", events: ["transmission","transmission_ext"] },
            { key: "Brakes",       icon: "\uF0A7", events: ["brakes"] },
            { key: "Hybrid",       icon: "\uF0E7", events: ["hybrid_ext","hybrid_int"] },
            { key: "Environment",  icon: "\uF74E", events: ["wind"] },
            { key: "CSP Rain",     icon: "\uF743", events: ["rain_amb","rain_amb_thunder","rain_car_ext","rain_car_int","rain_grass","rain_gravel","rain_skid_ext","rain_skid_int"] },
            { key: "CSP Vehicle",  icon: "\uF1B9", events: ["turn_signal_ext__off","turn_signal_int__off","turn_signal_int","wiper_car_ext","wiper_car_ext_vintage","wiper_car_int","wiper_car_int_vintage","handbrake_int"] },
            { key: "CSP Wind",     icon: "\uF74E", events: ["external_wind"] },
            { key: "CSP Surfaces", icon: "\uF7A0", events: ["csp_surfaces_skid","csp_surfaces_force","csp_surfaces_rocks","csp_surfaces_ice"] }
        ];
    }

    property var systems: buildSystems()

    function systemEvents(sys) {
        var sysArr = systems;
        for (var i = 0; i < sysArr.length; i++)
            if (sysArr[i].key === sys) return sysArr[i].events;
        return [];
    }

    readonly property color cAccent: "#E10600"
    readonly property color cBg: "#121212"
    readonly property color cPanel: "#252526"
    readonly property color cBorder: "#333333"
    readonly property color cText: "#cccccc"
    readonly property color cMuted: "#666666"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 36; color: "#252526"; Layout.fillWidth: true
            border.color: cBorder; border.width: 1
            RowLayout {
                anchors.fill: parent; anchors.margins: 8
                Text { text: "CAR AUDIO SYSTEMS"; color: cAccent; font.pixelSize: 12; font.bold: true }
                Item { Layout.fillWidth: true }
                AppButton { text: "\u25B6 Preview"; height: 24; font.pixelSize: 10; bgcolor: cAccent; color: "#121212" }
                AppButton { text: "Export Bank"; height: 24; font.pixelSize: 10; bgcolor: "transparent"; color: cText }
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            spacing: 0

            // Left: System list
            Rectangle {
                width: 140; Layout.fillHeight: true
                color: cPanel; border.color: cBorder; border.width: 1
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 6; spacing: 2
                    Text { text: "SYSTEMS"; color: cMuted; font.pixelSize: 9; font.bold: true; leftPadding: 4 }

                    Repeater {
                        model: systems
                        AppButton {
                            height: 24; text: modelData.icon + "  " + modelData.key; font.pixelSize: 9
                            bgcolor: activeSystem === modelData.key ? cAccent : "#3e3e42"
                            color: activeSystem === modelData.key ? "#121212" : cText
                            onClicked: activeSystem = modelData.key
                        }
                    }

                    Item { Layout.fillHeight: true }
                    AppButton { text: "Mixer"; height: 28; bgcolor: "transparent"; color: cText }
                }
            }

            // Center: Event parameters
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: "#1a1a1a"; border.color: cBorder; border.width: 1
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 12; spacing: 8
                    Text { text: activeSystem.toUpperCase() + " EVENTS"; color: cAccent; font.pixelSize: 12; font.bold: true }

                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: systemEvents(activeSystem)
                        delegate: Rectangle {
                            width: ListView.view.width; height: 26; color: index % 2 === 0 ? "#1e1e1e" : "#181818"
                            radius: 2
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 4; spacing: 8
                                Text { text: "\u266A"; color: cAccent; font.pixelSize: 10; width: 16 }
                                Text { text: modelData; color: cText; font.pixelSize: 10; font.bold: true; Layout.preferredWidth: 120 }
                                Slider { from: 0; to: 100; value: 80; Layout.fillWidth: true; height: 16
                                    background: Rectangle { x: 0; y: 6; width: parent.width; height: 4; radius: 2; color: "#2a2a2a" } }
                                Text { text: "80%"; color: cAccent; font.pixelSize: 9; width: 30 }
                                Rectangle { width: 16; height: 16; radius: 2; color: "#3a3a3a"
                                    Text { anchors.centerIn: parent; text: "\u25B6"; color: cMuted; font.pixelSize: 8 }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor } }
                                Rectangle { width: 16; height: 16; radius: 2; color: "#3a3a3a"
                                    Text { anchors.centerIn: parent; text: "S"; color: cMuted; font.pixelSize: 8 }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor } }
                                Rectangle { width: 16; height: 16; radius: 2; color: "#3a3a3a"
                                    Text { anchors.centerIn: parent; text: "M"; color: cMuted; font.pixelSize: 8 }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor } }
                            }
                        }
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    }

                    // Bank status
                    RowLayout { spacing: 16
                        Text { text: "Bank:"; color: cMuted; font.pixelSize: 9 }
                        Text { text: activeSystem.toLowerCase() + ".bank"; color: cText; font.pixelSize: 9 }
                        Rectangle { height: 14; width: 30; radius: 2; color: "#1a3a1a"
                            Text { anchors.centerIn: parent; text: "OK"; color: "#80ff80"; font.pixelSize: 8 } }
                    }
                }
            }

            // Right: System controls
            Rectangle {
                width: 160; Layout.fillHeight: true
                color: cPanel; border.color: cBorder; border.width: 1
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 10; spacing: 8
                    Text { text: "CONTROLS"; color: cMuted; font.pixelSize: 9; font.bold: true }

                    ColumnLayout { spacing: 6; Layout.fillWidth: true
                        Repeater {
                            model: {
                                if (activeSystem === "Engine") return ["Volume:80%", "Pitch:1.0", "RPM Range", "Doppler:On"];
                                if (activeSystem === "Tires") return ["Volume:70%", "Surface:Grip", "Skid Threshold", "Roll Noise"];
                                if (activeSystem === "CSP Rain") return ["Intensity:50%", "Wetness:30%", "Surface:Asphalt"];
                                return ["Volume:80%", "Pitch:1.0"];
                            }
                            Text { text: modelData; color: cMuted; font.pixelSize: 9; leftPadding: 4 }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    AppButton { text: "Reset System"; height: 28; bgcolor: "transparent"; color: cText }
                    AppButton { text: "Export"; height: 28; bgcolor: cAccent; color: "#121212" }
                }
            }
        }
    }
}
