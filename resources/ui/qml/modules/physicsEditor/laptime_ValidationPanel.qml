import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import ksEditor.LapTimeValidation 1.0

Rectangle {
    id: validationPanel
    color: "#1e1e1e"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 36
            color: "#252526"
            Layout.fillWidth: true

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: "LAP TIME VALIDATION"
                color: "#E10600"
                font.pixelSize: 12
                font.bold: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 8

            Text {
                text: "Validation harness provides lap time predictions with fuel weight correction, tire wear modeling, track evolution, and confidence scoring based on sector history."
                color: "#888"
                font.pixelSize: 10
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Item { height: 8 }

            Rectangle {
                color: "#2d2d2d"
                radius: 4
                Layout.fillWidth: true
                height: 80

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    Text { text: "Sector History"; color: "#ccc"; font.pixelSize: 10; font.bold: true }
                    Text {
                        text: "Best sectors: S1=" + (LapTimeValidation.bestSector1 < 1e8 ? LapTimeValidation.bestSector1.toFixed(3) : "--")
                            + "  S2=" + (LapTimeValidation.bestSector2 < 1e8 ? LapTimeValidation.bestSector2.toFixed(3) : "--")
                            + "  S3=" + (LapTimeValidation.bestSector3 < 1e8 ? LapTimeValidation.bestSector3.toFixed(3) : "--")
                        color: "#aaa"; font.pixelSize: 10
                    }
                    Text {
                        text: "Projected best lap: " + (LapTimeValidation.projectedBestLap < 1e8 ? LapTimeValidation.projectedBestLap.toFixed(3) + "s" : "--")
                        color: "#22c55e"; font.pixelSize: 12; font.bold: true
                    }
                }
            }

            Item { height: 4 }

            Rectangle {
                color: "#2d2d2d"
                radius: 4
                Layout.fillWidth: true
                height: 60

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 16

                    ColumnLayout {
                        Text { text: "Validation Count"; color: "#888"; font.pixelSize: 9 }
                        Text { text: LapTimeValidation.validationCount; color: "#fff"; font.pixelSize: 14 }
                    }
                    ColumnLayout {
                        Text { text: "Fuel Penalty Model"; color: "#888"; font.pixelSize: 9 }
                        Text { text: "Active"; color: "#22c55e"; font.pixelSize: 14 }
                    }
                    ColumnLayout {
                        Text { text: "Tire Wear Model"; color: "#888"; font.pixelSize: 9 }
                        Text { text: "Active"; color: "#22c55e"; font.pixelSize: 14 }
                    }
                }
            }
        }
    }
}
