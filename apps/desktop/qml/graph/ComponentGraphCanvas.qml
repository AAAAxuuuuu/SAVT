import QtQuick

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject selectionState

    function edgeRoutePoints(edge) {
        if (edge && edge.routePoints && edge.routePoints.length >= 2)
            return edge.routePoints
        return []
    }

    function edgeTouchesSelection(edge) {
        if (selectionState.selectedComponentEdge && selectionState.selectedComponentEdge.id !== undefined)
            return selectionState.selectedComponentEdge.id === edge.id
        return selectionState.selectedComponentNode &&
               (edge.fromId === selectionState.selectedComponentNode.id || edge.toId === selectionState.selectedComponentNode.id)
    }

    function drawEdge(ctx, edge, edgeCanvas) {
        var points = edgeRoutePoints(edge)
        if (!points || points.length < 2)
            return

        var emphasized = edgeTouchesSelection(edge)
        var hasSelection = (selectionState.selectedComponentNode && selectionState.selectedComponentNode.id !== undefined) ||
                           (selectionState.selectedComponentEdge && selectionState.selectedComponentEdge.id !== undefined)
        var cornerRadius = emphasized ? 14 : 10
        var lineWidth = emphasized ? 2.95 : 1.45
        var strokeAlpha = hasSelection ? (emphasized ? 0.9 : 0.16) : 0.54
        var haloAlpha = hasSelection ? (emphasized ? 0.24 : 0.07) : 0.14

        ctx.beginPath()
        edgeCanvas.drawRoundedOrthogonalPath(ctx, points, cornerRadius)
        ctx.strokeStyle = "#FFFFFF"
        ctx.lineWidth = lineWidth + (emphasized ? 3.6 : 2.3)
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
        nodes: root.selectionState.componentNodes
        groups: root.selectionState.componentGroups
        visibleEdges: root.selectionState.visibleComponentEdges()
        sceneBounds: root.selectionState.componentSceneBounds()
        nodeDelegate: componentNodeDelegate
        nodeDisplayRectProvider: function(node) { return root.selectionState.nodeDisplayRect(node) }
        nodeVisibleProvider: function(nodeId) { return root.selectionState.componentNodeShouldBeDisplayed(nodeId) }
        nodeOpacityProvider: function(node) { return root.selectionState.componentNodeOpacity(node) }
        nodeSelectedProvider: function(node) {
            return root.selectionState.selectedComponentNode &&
                   root.selectionState.selectedComponentNode.id === node.id
        }
        nodeKindLabelProvider: function(kind) { return root.selectionState.displayNodeKind(kind) }
        edgePainter: function(ctx, edge, edgeCanvas) { root.drawEdge(ctx, edge, edgeCanvas) }
        minSceneWidth: 740
        minSceneHeight: 400
        gridStep: 44
        emptyText: root.selectionState.selectedCapabilityNode
                   ? "当前能力域还没有可展开的组件图。"
                   : "先在能力地图里选中一个能力域。"
        emptyHint: root.selectionState.selectedCapabilityNode
                   ? (root.selectionState.componentSceneDiagnostics()[0] || "当前能力域缺少内部 overview 模块或内部依赖边，暂时无法展开更细的组件图。")
                   : "从 L2 选择能力域后，这里会保留父级上下文并展开 L3 组件关系。"
        onNodeClicked: root.selectionState.selectComponentNode(node)
    }

    Component {
        id: componentNodeDelegate
        ComponentNodeItem { }
    }

}
