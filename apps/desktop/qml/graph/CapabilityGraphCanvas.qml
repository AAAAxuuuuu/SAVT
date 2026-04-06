import QtQuick

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject selectionState

    signal drillRequested(var node)

    function edgeRoutePoints(edge) {
        if (edge && edge.routePoints && edge.routePoints.length >= 2)
            return edge.routePoints
        return []
    }

    function edgeTouchesSelection(edge) {
        if (selectionState.selectedCapabilityEdge && selectionState.selectedCapabilityEdge.id !== undefined)
            return selectionState.selectedCapabilityEdge.id === edge.id
        return selectionState.selectedCapabilityNode &&
               (edge.fromId === selectionState.selectedCapabilityNode.id || edge.toId === selectionState.selectedCapabilityNode.id)
    }

    function drawEdge(ctx, edge, edgeCanvas) {
        var points = edgeRoutePoints(edge)
        if (!points || points.length < 2)
            return

        var emphasized = edgeTouchesSelection(edge)
        var hasSelection = (selectionState.selectedCapabilityNode && selectionState.selectedCapabilityNode.id !== undefined) ||
                           (selectionState.selectedCapabilityEdge && selectionState.selectedCapabilityEdge.id !== undefined)
        var cornerRadius = emphasized ? 16 : 12
        var lineWidth = emphasized ? 3.1 : 1.55
        var strokeAlpha = hasSelection ? (emphasized ? 0.94 : 0.14) : 0.6
        var haloAlpha = hasSelection ? (emphasized ? 0.26 : 0.06) : 0.16

        ctx.beginPath()
        edgeCanvas.drawRoundedOrthogonalPath(ctx, points, cornerRadius)
        ctx.strokeStyle = "#FFFFFF"
        ctx.lineWidth = lineWidth + (emphasized ? 4.0 : 2.6)
        ctx.globalAlpha = haloAlpha
        ctx.stroke()

        ctx.beginPath()
        edgeCanvas.drawRoundedOrthogonalPath(ctx, points, cornerRadius)
        ctx.strokeStyle = theme.edgeColor(edge.kind)
        ctx.lineWidth = lineWidth
        ctx.globalAlpha = strokeAlpha
        ctx.stroke()
    }

    GraphViewport {
        anchors.fill: parent
        theme: root.theme
        nodes: root.selectionState.capabilityNodes
        groups: root.selectionState.capabilityGroups
        visibleEdges: root.selectionState.visibleCapabilityEdges()
        sceneBounds: root.selectionState.capabilitySceneBounds()
        nodeDelegate: capabilityNodeDelegate
        nodeDisplayRectProvider: function(node) { return root.selectionState.nodeDisplayRect(node) }
        nodeVisibleProvider: function(nodeId) { return root.selectionState.nodeShouldBeDisplayed(nodeId) }
        nodeOpacityProvider: function(node) { return root.selectionState.capabilityNodeOpacity(node) }
        nodeSelectedProvider: function(node) {
            return root.selectionState.selectedCapabilityNode &&
                   root.selectionState.selectedCapabilityNode.id === node.id
        }
        nodeKindLabelProvider: function(kind) { return root.selectionState.displayNodeKind(kind) }
        edgePainter: function(ctx, edge, edgeCanvas) { root.drawEdge(ctx, edge, edgeCanvas) }
        minSceneWidth: 820
        minSceneHeight: 420
        gridStep: 48
        emptyText: "当前还没有可展示的能力地图。"
        emptyHint: root.analysisController.analyzing
                   ? "分析完成后这里会生成 L2 业务能力地图。"
                   : "先选择项目并执行一次分析。"
        onNodeClicked: root.selectionState.selectCapabilityNode(node)
        onNodeDoubleClicked: root.drillRequested(node)
    }

    Component {
        id: capabilityNodeDelegate
        CapabilityNodeItem { }
    }

}
