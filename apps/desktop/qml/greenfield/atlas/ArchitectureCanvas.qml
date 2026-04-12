import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

Item {
    id: root

    required property QtObject tokens
    property var scene: ({})
    property var selectedNode: null
    property string emptyText: "运行分析后，架构图会出现在这里。"
    property bool componentMode: false
    property var hoverNode: null
    property bool showAllEdges: false

    readonly property int focusEdgeLimit: 18
    readonly property int overviewEdgeLimit: 26
    readonly property var nodes: (scene.nodes || [])
    readonly property var edges: (scene.edges || [])
    readonly property var bounds: (scene.bounds || ({}))
    readonly property real sceneWidth: Math.max(1, bounds.width || 980)
    readonly property real sceneHeight: Math.max(1, bounds.height || 560)
    readonly property real baseScale: Math.min(1.0, Math.max(0.38, Math.min((width - 120) / sceneWidth, (height - 100) / sceneHeight)))
    readonly property real effectiveScale: baseScale * zoom
    readonly property real contentX: Math.max(40, (width - sceneWidth * effectiveScale) / 2) + panX
    readonly property real contentY: Math.max(38, (height - sceneHeight * effectiveScale) / 2) + panY
    readonly property var visibleEdges: collectVisibleEdges(edges, selectedNode, hoverNode, showAllEdges)
    property real zoom: 1.0
    property real panX: 0
    property real panY: 0

    signal nodeSelected(var node)
    signal nodeDrilled(var node)
    signal blankClicked()

    function resetView() {
        zoom = 1.0
        panX = 0
        panY = 0
    }

    function zoomAt(viewX, viewY, factor) {
        var nextZoom = Math.max(0.38, Math.min(2.8, zoom * factor))
        if (Math.abs(nextZoom - zoom) < 0.001)
            return

        var oldSceneX = (viewX - contentX) / effectiveScale
        var oldSceneY = (viewY - contentY) / effectiveScale
        var nextScale = baseScale * nextZoom
        var nextCenterX = Math.max(40, (width - sceneWidth * nextScale) / 2)
        var nextCenterY = Math.max(38, (height - sceneHeight * nextScale) / 2)

        zoom = nextZoom
        panX = viewX - nextCenterX - oldSceneX * nextScale
        panY = viewY - nextCenterY - oldSceneY * nextScale
    }

    function focusNode() {
        return selectedNode || hoverNode
    }

    function nodeById(nodeId) {
        for (var index = 0; index < nodes.length; ++index) {
            if (nodes[index].id === nodeId)
                return nodes[index]
        }
        return null
    }

    function nodeIndexById(nodeId) {
        for (var index = 0; index < nodes.length; ++index) {
            if (nodes[index].id === nodeId)
                return index
        }
        return -1
    }

    function sceneX(item, fallbackIndex) {
        if (item && item.x !== undefined)
            return item.x
        return 64 + (fallbackIndex % 3) * 320
    }

    function sceneY(item, fallbackIndex) {
        if (item && item.y !== undefined)
            return item.y
        return 90 + Math.floor(fallbackIndex / 3) * 170
    }

    function nodeWidth(item) {
        return Math.max(230, Math.min(280, item && item.width ? item.width : 240))
    }

    function nodeHeight(item) {
        return Math.max(118, Math.min(160, item && item.height ? item.height : 126))
    }

    function nodeRectById(nodeId) {
        var node = nodeById(nodeId)
        var index = nodeIndexById(nodeId)
        if (!node || index < 0)
            return {"valid": false, "x": 0, "y": 0, "width": 0, "height": 0}

        return {
            "valid": true,
            "x": sceneX(node, index),
            "y": sceneY(node, index),
            "width": nodeWidth(node),
            "height": nodeHeight(node)
        }
    }

    function edgeTouchesFocus(edge) {
        var node = focusNode()
        return node && (edge.fromId === node.id || edge.toId === node.id)
    }

    function collectVisibleEdges(edgeList, selected, hovered, showAll) {
        var output = []
        var node = selected || hovered
        if (!node && !showAll)
            return output

        var limit = node ? focusEdgeLimit : overviewEdgeLimit
        for (var index = 0; index < edgeList.length && output.length < limit; ++index) {
            var edge = edgeList[index]
            if (node && !(edge.fromId === node.id || edge.toId === node.id))
                continue

            output.push({
                "edge": edge,
                "laneIndex": output.length
            })
        }
        return output
    }

    function edgeLane(laneIndex) {
        return ((laneIndex % 11) - 5) * 5
    }

    function nodeRectAt(nodeIndex, margin) {
        var node = nodes[nodeIndex]
        return {
            "x": sceneX(node, nodeIndex) - margin,
            "y": sceneY(node, nodeIndex) - margin,
            "width": nodeWidth(node) + margin * 2,
            "height": nodeHeight(node) + margin * 2
        }
    }

    function rectIntersectsY(rect, minY, maxY) {
        return rect.y < maxY && rect.y + rect.height > minY
    }

    function rectIntersectsX(rect, minX, maxX) {
        return rect.x < maxX && rect.x + rect.width > minX
    }

    function segmentHitsRect(startPoint, endPoint, rect) {
        var minX = Math.min(startPoint.x, endPoint.x)
        var maxX = Math.max(startPoint.x, endPoint.x)
        var minY = Math.min(startPoint.y, endPoint.y)
        var maxY = Math.max(startPoint.y, endPoint.y)

        if (Math.abs(startPoint.y - endPoint.y) < 0.5)
            return startPoint.y > rect.y
                    && startPoint.y < rect.y + rect.height
                    && maxX > rect.x
                    && minX < rect.x + rect.width

        if (Math.abs(startPoint.x - endPoint.x) < 0.5)
            return startPoint.x > rect.x
                    && startPoint.x < rect.x + rect.width
                    && maxY > rect.y
                    && minY < rect.y + rect.height

        return false
    }

    function routeHitsModules(points, edge) {
        for (var segmentIndex = 0; segmentIndex < points.length - 1; ++segmentIndex) {
            for (var nodeIndex = 0; nodeIndex < nodes.length; ++nodeIndex) {
                var node = nodes[nodeIndex]
                if (node.id === edge.fromId || node.id === edge.toId)
                    continue

                if (segmentHitsRect(points[segmentIndex], points[segmentIndex + 1], nodeRectAt(nodeIndex, 12)))
                    return true
            }
        }
        return false
    }

    function verticalLane(fromRect, toRect, laneIndex) {
        var minY = Math.min(fromRect.y, toRect.y) - 24
        var maxY = Math.max(fromRect.y + fromRect.height, toRect.y + toRect.height) + 24
        var minLeft = Math.min(fromRect.x, toRect.x)
        var maxRight = Math.max(fromRect.x + fromRect.width, toRect.x + toRect.width)

        for (var index = 0; index < nodes.length; ++index) {
            var rect = nodeRectAt(index, 18)
            if (!rectIntersectsY(rect, minY, maxY))
                continue

            minLeft = Math.min(minLeft, rect.x)
            maxRight = Math.max(maxRight, rect.x + rect.width)
        }

        var gap = 46 + Math.abs(edgeLane(laneIndex)) * 0.7
        var leftX = minLeft - gap
        var rightX = maxRight + gap
        var leftClear = leftX > 16
        var rightClear = rightX < root.sceneWidth - 16
        var fromCenterX = fromRect.x + fromRect.width / 2
        var toCenterX = toRect.x + toRect.width / 2
        var leftCost = Math.abs(fromRect.x - leftX) + Math.abs(toRect.x - leftX)
        var rightCost = Math.abs(fromRect.x + fromRect.width - rightX) + Math.abs(toRect.x + toRect.width - rightX)

        if (rightClear && (!leftClear || rightCost <= leftCost || toCenterX >= fromCenterX))
            return {"side": "right", "value": rightX}
        if (leftClear)
            return {"side": "left", "value": leftX}

        return rightCost <= leftCost
                ? {"side": "right", "value": Math.max(maxRight + 24, root.sceneWidth - 18)}
                : {"side": "left", "value": Math.min(minLeft - 24, 18)}
    }

    function horizontalLane(fromRect, toRect, laneIndex) {
        var minX = Math.min(fromRect.x, toRect.x) - 24
        var maxX = Math.max(fromRect.x + fromRect.width, toRect.x + toRect.width) + 24
        var minTop = Math.min(fromRect.y, toRect.y)
        var maxBottom = Math.max(fromRect.y + fromRect.height, toRect.y + toRect.height)

        for (var index = 0; index < nodes.length; ++index) {
            var rect = nodeRectAt(index, 18)
            if (!rectIntersectsX(rect, minX, maxX))
                continue

            minTop = Math.min(minTop, rect.y)
            maxBottom = Math.max(maxBottom, rect.y + rect.height)
        }

        var gap = 42 + Math.abs(edgeLane(laneIndex)) * 0.7
        var topY = minTop - gap
        var bottomY = maxBottom + gap
        var topClear = topY > 16
        var bottomClear = bottomY < root.sceneHeight - 16
        var topCost = Math.abs(fromRect.y - topY) + Math.abs(toRect.y - topY)
        var bottomCost = Math.abs(fromRect.y + fromRect.height - bottomY) + Math.abs(toRect.y + toRect.height - bottomY)

        if (bottomClear && (!topClear || bottomCost <= topCost))
            return {"side": "bottom", "value": bottomY}
        if (topClear)
            return {"side": "top", "value": topY}

        return bottomCost <= topCost
                ? {"side": "bottom", "value": Math.max(maxBottom + 24, root.sceneHeight - 18)}
                : {"side": "top", "value": Math.min(minTop - 24, 18)}
    }

    function directRoute(edge, fromRect, toRect, laneIndex, vertical) {
        var lane = edgeLane(laneIndex)
        var fromCenterX = fromRect.x + fromRect.width / 2
        var fromCenterY = fromRect.y + fromRect.height / 2
        var toCenterX = toRect.x + toRect.width / 2
        var toCenterY = toRect.y + toRect.height / 2

        if (vertical) {
            var fromSide = toCenterY >= fromCenterY ? "bottom" : "top"
            var toSide = toCenterY >= fromCenterY ? "top" : "bottom"
            var startPoint = portPoint(fromRect, fromSide, lane)
            var endPoint = portPoint(toRect, toSide, -lane)
            var midY = (startPoint.y + endPoint.y) / 2
            return [startPoint, Qt.point(startPoint.x, midY), Qt.point(endPoint.x, midY), endPoint]
        }

        var sourceSide = toCenterX >= fromCenterX ? "right" : "left"
        var targetSide = toCenterX >= fromCenterX ? "left" : "right"
        var start = portPoint(fromRect, sourceSide, lane)
        var end = portPoint(toRect, targetSide, -lane)
        var midX = (start.x + end.x) / 2
        return [start, Qt.point(midX, start.y), Qt.point(midX, end.y), end]
    }

    function bypassRoute(fromRect, toRect, laneIndex, vertical) {
        var lane = edgeLane(laneIndex)
        if (vertical) {
            var xLane = verticalLane(fromRect, toRect, laneIndex)
            var startPoint = portPoint(fromRect, xLane.side, lane)
            var endPoint = portPoint(toRect, xLane.side, -lane)
            return [startPoint, Qt.point(xLane.value, startPoint.y), Qt.point(xLane.value, endPoint.y), endPoint]
        }

        var yLane = horizontalLane(fromRect, toRect, laneIndex)
        var start = portPoint(fromRect, yLane.side, lane)
        var end = portPoint(toRect, yLane.side, -lane)
        return [start, Qt.point(start.x, yLane.value), Qt.point(end.x, yLane.value), end]
    }

    function routeEdge(edge, laneIndex) {
        var fromRect = nodeRectById(edge.fromId)
        var toRect = nodeRectById(edge.toId)
        if (!fromRect.valid || !toRect.valid) {
            var emptyPoint = Qt.point(0, 0)
            return {"p0": emptyPoint, "p1": emptyPoint, "p2": emptyPoint, "p3": emptyPoint}
        }

        var fromCenterX = fromRect.x + fromRect.width / 2
        var fromCenterY = fromRect.y + fromRect.height / 2
        var toCenterX = toRect.x + toRect.width / 2
        var toCenterY = toRect.y + toRect.height / 2
        var vertical = Math.abs(toCenterY - fromCenterY) >= Math.abs(toCenterX - fromCenterX) * 0.72
        var direct = directRoute(edge, fromRect, toRect, laneIndex, vertical)
        var route = routeHitsModules(direct, edge) ? bypassRoute(fromRect, toRect, laneIndex, vertical) : direct

        return {"p0": route[0], "p1": route[1], "p2": route[2], "p3": route[3]}
    }

    function portPoint(rect, side, lane) {
        if (side === "right")
            return Qt.point(rect.x + rect.width, rect.y + rect.height / 2 + lane)
        if (side === "left")
            return Qt.point(rect.x, rect.y + rect.height / 2 + lane)
        if (side === "bottom")
            return Qt.point(rect.x + rect.width / 2 + lane, rect.y + rect.height)
        return Qt.point(rect.x + rect.width / 2 + lane, rect.y)
    }

    function arrowWing(tip, tail, size, direction) {
        var angle = Math.atan2(tip.y - tail.y, tip.x - tail.x)
        if (!isFinite(angle))
            angle = -Math.PI / 2

        return Qt.point(tip.x - size * Math.cos(angle + direction * Math.PI / 6),
                        tip.y - size * Math.sin(angle + direction * Math.PI / 6))
    }

    function edgeColor(edge) {
        var node = focusNode()
        if (node && edge.toId === node.id)
            return tokens.signalRaspberry
        return tokens.signalCobalt
    }

    Canvas {
        id: gridCanvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            var step = Math.max(12, 24 * root.effectiveScale)
            var startX = ((root.contentX % step) + step) % step
            var startY = ((root.contentY % step) + step) % step

            ctx.reset()
            ctx.fillStyle = root.tokens.graphCanvas
            ctx.fillRect(0, 0, width, height)
            ctx.fillStyle = root.tokens.graphGrid
            for (var x = startX; x < width; x += step) {
                for (var y = startY; y < height; y += step) {
                    ctx.beginPath()
                    ctx.arc(x, y, 1.1, 0, Math.PI * 2)
                    ctx.fill()
                }
            }
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    onContentXChanged: gridCanvas.requestPaint()
    onContentYChanged: gridCanvas.requestPaint()
    onEffectiveScaleChanged: gridCanvas.requestPaint()
    onSceneChanged: {
        root.resetView()
        gridCanvas.requestPaint()
    }

    MouseArea {
        id: canvasMouse
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        property real lastX: 0
        property real lastY: 0
        property bool dragged: false

        onPressed: function(mouse) {
            lastX = mouse.x
            lastY = mouse.y
            dragged = false
        }

        onPositionChanged: function(mouse) {
            if (!pressed)
                return

            var dx = mouse.x - lastX
            var dy = mouse.y - lastY
            if (Math.abs(dx) + Math.abs(dy) > 1)
                dragged = true
            root.panX += dx
            root.panY += dy
            lastX = mouse.x
            lastY = mouse.y
        }

        onClicked: function(mouse) {
            if (!dragged)
                root.blankClicked()
        }

        onWheel: function(wheel) {
            root.zoomAt(wheel.x, wheel.y, wheel.angleDelta.y > 0 ? 1.12 : 0.88)
            wheel.accepted = true
        }
    }

    Item {
        id: contentLayer
        x: root.contentX
        y: root.contentY
        width: root.sceneWidth
        height: root.sceneHeight
        scale: root.effectiveScale
        transformOrigin: Item.TopLeft
        z: 2

        Repeater {
            model: root.visibleEdges

            Shape {
                id: edgeShape

                property var edge: modelData.edge
                property int laneIndex: modelData.laneIndex
                property var route: root.routeEdge(edge, laneIndex)
                property point p0: route.p0
                property point p1: route.p1
                property point p2: route.p2
                property point p3: route.p3
                property real strokeScale: Math.max(0.38, root.effectiveScale)
                property point arrowLeft: root.arrowWing(p3, p2, 9 / strokeScale, -1)
                property point arrowRight: root.arrowWing(p3, p2, 9 / strokeScale, 1)
                property color lineColor: root.edgeColor(edge)

                anchors.fill: parent
                z: 0
                opacity: root.focusNode() ? 0.82 : 0.22
                antialiasing: true

                ShapePath {
                    strokeWidth: (root.focusNode() ? 2.0 : 1.2) / edgeShape.strokeScale
                    strokeColor: edgeShape.lineColor
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    startX: edgeShape.p0.x
                    startY: edgeShape.p0.y

                    PathLine { x: edgeShape.p1.x; y: edgeShape.p1.y }
                    PathLine { x: edgeShape.p2.x; y: edgeShape.p2.y }
                    PathLine { x: edgeShape.p3.x; y: edgeShape.p3.y }
                }

                ShapePath {
                    strokeWidth: 0
                    strokeColor: "transparent"
                    fillColor: edgeShape.lineColor
                    startX: edgeShape.p3.x
                    startY: edgeShape.p3.y

                    PathLine { x: edgeShape.arrowLeft.x; y: edgeShape.arrowLeft.y }
                    PathLine { x: edgeShape.arrowRight.x; y: edgeShape.arrowRight.y }
                    PathLine { x: edgeShape.p3.x; y: edgeShape.p3.y }
                }
            }
        }

        Repeater {
            model: root.nodes

            Rectangle {
                id: card
                clip: true
                property bool hovered: false
                readonly property bool selected: root.selectedNode && root.selectedNode.id === modelData.id
                readonly property var evidence: modelData.evidence || ({})
                readonly property bool risky: (modelData.riskLevel || "") === "high"
                                        || (modelData.riskSignals || []).length > 0

                x: root.sceneX(modelData, index)
                y: root.sceneY(modelData, index)
                width: root.nodeWidth(modelData)
                height: root.nodeHeight(modelData)
                radius: root.tokens.radius8
                color: root.tokens.base0
                border.color: selected ? root.tokens.signalCobalt : root.tokens.border1
                border.width: selected ? 2 : 1
                z: hovered || selected ? 10 : 2
                opacity: root.selectedNode && !selected ? 0.58 : 1.0

                Behavior on opacity { NumberAnimation { duration: 140 } }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -3
                    radius: root.tokens.radius8
                    color: "transparent"
                    border.color: selected ? Qt.rgba(0, 0.478, 1, 0.15) : "transparent"
                    border.width: selected ? 3 : 0
                    z: -1
                }

                Rectangle {
                    visible: card.risky && card.hovered
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.topMargin: -10
                    anchors.rightMargin: -10
                    width: 104
                    height: 24
                    radius: root.tokens.radius8
                    color: root.tokens.signalCoral

                    Label {
                        anchors.centerIn: parent
                        text: "需要关注"
                        color: "#FFFFFF"
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 10
                        font.weight: Font.Bold
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 7

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 10
                            Layout.preferredHeight: 10
                            radius: 5
                            color: modelData.reachableFromEntry ? root.tokens.signalTeal
                                   : root.componentMode ? root.tokens.signalRaspberry
                                   : root.tokens.signalCobalt
                        }

                        Label {
                            Layout.fillWidth: true
                            text: modelData.name || "未命名节点"
                            elide: Text.ElideRight
                            color: root.tokens.text1
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        text: modelData.summary || modelData.responsibility || modelData.role || "暂无描述。"
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        color: root.tokens.text3
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 12
                        lineHeight: 1.18
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.tokens.border1
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: modelData.kind || modelData.role || "Node"
                            color: root.tokens.text3
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: (modelData.fileCount || modelData.sourceFileCount || 0) + " Files"
                            color: root.tokens.text3
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 11
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: {
                        card.hovered = true
                        root.hoverNode = modelData
                    }
                    onExited: {
                        card.hovered = false
                        if (root.hoverNode && root.hoverNode.id === modelData.id)
                            root.hoverNode = null
                    }
                    onClicked: root.nodeSelected(modelData)
                    onDoubleClicked: root.nodeDrilled(modelData)
                    onWheel: function(wheel) {
                        var point = card.mapToItem(root, wheel.x, wheel.y)
                        root.zoomAt(point.x, point.y, wheel.angleDelta.y > 0 ? 1.12 : 0.88)
                        wheel.accepted = true
                    }
                }
            }
        }
    }

    Label {
        anchors.centerIn: parent
        visible: root.nodes.length === 0
        text: root.emptyText
        color: root.tokens.text3
        font.family: root.tokens.textFontFamily
        font.pixelSize: 18
        font.weight: Font.Normal
    }

    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 18
        width: 272
        height: 38
        radius: root.tokens.radius8
        color: Qt.rgba(1, 1, 1, 0.86)
        border.color: root.tokens.border1
        z: 30

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 6

            ToolButton {
                text: "-"
                onClicked: root.zoomAt(root.width / 2, root.height / 2, 0.86)
            }

            Label {
                Layout.fillWidth: true
                text: Math.round(root.zoom * 100) + "%"
                horizontalAlignment: Text.AlignHCenter
                color: root.tokens.text2
                font.family: root.tokens.textFontFamily
                font.pixelSize: 12
            }

            ToolButton {
                text: "+"
                onClicked: root.zoomAt(root.width / 2, root.height / 2, 1.16)
            }

            ToolButton {
                text: "Fit"
                onClicked: root.resetView()
            }

            ToolButton {
                text: root.showAllEdges ? "全线" : "聚焦"
                onClicked: root.showAllEdges = !root.showAllEdges
            }
        }
    }

    Label {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 18
        text: "拖动画布平移 · 滚轮缩放 · 悬停/选中查看依赖方向 · 双击节点下钻"
        color: root.tokens.text3
        font.family: root.tokens.textFontFamily
        font.pixelSize: 11
        z: 30
    }
}
