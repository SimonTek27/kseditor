import QtQuick
import QtQuick3D

Texture {
    id: root
    property color lineColor: "#3f3f46"
    property color backgroundColor: "#2a2a30"
    property int gridSize: 20

    width: 512
    height: 512

    sourceItem: Item {
        width: 512
        height: 512
        visible: true

        Canvas {
            id: gridCanvas
            anchors.fill: parent
            renderStrategy: Canvas.Cooperative

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.fillStyle = root.backgroundColor
                ctx.fillRect(0, 0, width, height)
                ctx.strokeStyle = root.lineColor
                ctx.lineWidth = 1
                for (var i = 0; i <= root.gridSize; i++) {
                    var pos = i * width / root.gridSize
                    ctx.beginPath()
                    ctx.moveTo(pos, 0)
                    ctx.lineTo(pos, height)
                    ctx.stroke()
                    ctx.beginPath()
                    ctx.moveTo(0, pos)
                    ctx.lineTo(width, pos)
                    ctx.stroke()
                }
            }

            Component.onCompleted: {
                requestPaint()
                paintTimer.start()
            }
        }

        Timer {
            id: paintTimer
            interval: 100
            repeat: true
            running: false
            onTriggered: {
                gridCanvas.requestPaint()
                count++
                if (count > 5) {
                    paintTimer.stop()
                    count = 0
                }
            }
            property int count: 0
        }
    }
}
