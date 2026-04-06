import QtQuick

Canvas {
    id: root

    property var edges: []
    property var edgePainter: function(ctx, edge) {}
    property bool repaintQueued: false

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
            root.edgePainter(ctx, visibleEdges[i])
    }

    onEdgesChanged: schedulePaint()
    onWidthChanged: schedulePaint()
    onHeightChanged: schedulePaint()
    onAvailableChanged: schedulePaint()
    onVisibleChanged: schedulePaint()
    Component.onCompleted: schedulePaint()
}
