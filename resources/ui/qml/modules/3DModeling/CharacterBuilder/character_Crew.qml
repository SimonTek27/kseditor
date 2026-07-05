import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../../widgets"

Rectangle {
    id: crewEditor
    width: 600
    height: 500
    color: "#1e1e1e"
    border.color: "#333333"
    border.width: 1

    property string activeCrew: "pit"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 36
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8

                Text {
                    text: "CREW & CROWD"
                    color: "#ff6600"
                    font.pixelSize: 12
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                KsButton { text: "Generate"; height: 24; font.pixelSize: 10; bgcolor: "#E10600"; color: "#121212" }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            ColumnLayout {
                width: 120

                KsButton { height: 32; text: "Pit Crew"; bgcolor: activeCrew === "pit" ? "#E10600" : "#3e3e42"; color: activeCrew === "pit" ? "#121212" : "#ffffff" }
                KsButton { height: 32; text: "Marshal"; bgcolor: activeCrew === "marshal" ? "#E10600" : "#3e3e42"; color: activeCrew === "marshal" ? "#121212" : "#ffffff" }
                KsButton { height: 32; text: "Crowd"; bgcolor: activeCrowd === "crowd" ? "#E10600" : "#3e3e42"; color: activeCrew === "crowd" ? "#121212" : "#ffffff" }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 6

                Text { text: "PIT CREW"; color: "#ff6600"; font.pixelSize: 12; font.bold: true }

                RowLayout {
                    Text { text: "Count:"; color: "#888888"; width: 70 }
                    Slider { from: 1; to: 20; value: 12; Layout.fillWidth: true }
                    Text { text: "12"; color: "#E10600" }
                }

                RowLayout { CheckBox { checked: true }; Text { text: "Jack Man"; color: "#888888" } }
                RowLayout { CheckBox { checked: true }; Text { text: "Lifter"; color: "#888888" } }
                RowLayout { CheckBox { checked: true }; Text { text: "Wheel Gunner"; color: "#888888" } }
                RowLayout { CheckBox { checked: true }; Text { text: "Fueler"; color: "#888888" } }
                RowLayout { CheckBox { checked: true }; Text { text: "Cleaner"; color: "#888888" } }

                Text { text: "ANIMATION"; color: "#666666"; font.pixelSize: 10; font.bold: true }
                RowLayout {
                    KsButton { text: "Start Sequence"; height: 28; bgcolor: "#E10600"; color: "#121212" }
                    KsButton { text: "Pit Stop Sim"; height: 28; bgcolor: "transparent"; color: "#ffffff" }
                }
            }
        }
    }
}
