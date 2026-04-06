import QtQuick

Canvas {
    id: root

    property var edges: []
    property var edgePainter: function(ctx, edge, edgeCanvas) {}
    property bool repaintQueued: false

    function segmentLength(startPoint, endPoint) {
        var dx = endPoint.x - startPoint.x
        var dy = endPoint.y - startPoint.y
        return Math.sqrt((dx * dx) + (dy * dy))
    }

    function pointToward(originPoint, targetPoint, distance) {
        var length = root.segmentLength(originPoint, targetPoint)
        if (length < 0.001)
            return {"x": originPoint.x, "y": originPoint.y}

        var ratio = distance / length
        return {
            "x": originPoint.x + ((targetPoint.x - originPoint.x) * ratio),
            "y": originPoint.y + ((targetPoint.y - originPoint.y) * ratio)
        }
    }

    function drawRoundedOrthogonalPath(ctx, points, desiredRadius) {
        if (!points || points.length < 2)
            return

        ctx.moveTo(points[0].x, points[0].y)

        if (points.length === 2) {
            ctx.lineTo(points[1].x, points[1].y)
            return
        }

        for (var index = 1; index < points.length - 1; ++index) {
            var previousPoint = points[index - 1]
            var currentPoint = points[index]
            var nextPoint = points[index + 1]
            var previousLength = root.segmentLength(previousPoint, currentPoint)
            var nextLength = root.segmentLength(currentPoint, nextPoint)
            var cornerRadius = Math.min(desiredRadius, previousLength / 2, nextLength / 2)

            if (cornerRadius < 1) {
                ctx.lineTo(currentPoint.x, currentPoint.y)
                continue
            }

            var entryPoint = root.pointToward(currentPoint, previousPoint, cornerRadius)
            var exitPoint = root.pointToward(currentPoint, nextPoint, cornerRadius)
            ctx.lineTo(entryPoint.x, entryPoint.y)
            ctx.quadraticCurveTo(currentPoint.x, currentPoint.y, exitPoint.x, exitPoint.y)
        }

        var finalPoint = points[points.length - 1]
        ctx.lineTo(finalPoint.x, finalPoint.y)
    }

    function schedulePaint() {
        if (repaintQueued || !available || !visible)
            return
        repaintQueued = true
        Qt.callLater(function() {
            repaintQueued = false
            if (available && visible)
                root.requestPaint()
        })
    }

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.lineJoin = "round"
        ctx.lineCap = "round"

        var visibleEdges = root.edges || []
        for (var i = 0; i < visibleEdges.length; ++i)
            root.edgePainter(ctx, visibleEdges[i], root)
    }

    onEdgesChanged: schedulePaint()
    onWidthChanged: schedulePaint()
    onHeightChanged: schedulePaint()
    onAvailableChanged: schedulePaint()
    onVisibleChanged: schedulePaint()
    Component.onCompleted: schedulePaint()
}
