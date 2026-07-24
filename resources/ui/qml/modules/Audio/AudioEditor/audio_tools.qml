import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: audioTools
    visible: false

    property real volume: 0.8
    property real pan: 0.0
    property bool muted: false

    function formatTime(seconds) {
        var mins = Math.floor(seconds / 60);
        var secs = Math.floor(seconds % 60);
        var ms = Math.floor((seconds % 1) * 100);
        return mins.toString().padStart(2, '0') + ":" +
               secs.toString().padStart(2, '0') + "." +
               ms.toString().padStart(2, '0');
    }

    function dbToLinear(db) {
        return Math.pow(10, db / 20);
    }

    function linearToDb(linear) {
        if (linear <= 0) return -96;
        return 20 * Math.log10(linear);
    }
}