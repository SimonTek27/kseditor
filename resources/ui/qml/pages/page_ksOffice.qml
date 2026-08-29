import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "ksOffice/OfficePage"

Item {
    id: officePageRoot
    width: 1280
    height: 720

    OfficePage {
        anchors.fill: parent
    }
}
