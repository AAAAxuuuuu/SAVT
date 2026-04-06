import QtQuick
import QtQuick.Controls
import "../common"

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

    function drawEdge(ctx, edge) {
        var points = edgeRoutePoints(edge)
        if (!points || points.length < 2)
            return

        var emphasized = edgeTouchesSelection(edge)
        var hasSelection = (selectionState.selectedComponentNode && selectionState.selectedComponentNode.id !== undefined) ||
                           (selectionState.selectedComponentEdge && selectionState.selectedComponentEdge.id !== undefined)

        ctx.beginPath()
        ctx.strokeStyle = theme.edgeColor(edge.kind)
        ctx.lineWidth = emphasized ? 2.8 : 1.3
        ctx.globalAlpha = hasSelection ? (emphasized ? 0.9 : 0.14) : 0.46
        ctx.moveTo(points[0].x, points[0].y)

        for (var i = 1; i < points.length; ++i)
            ctx.lineTo(points[i].x, points[i].y)

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
        edgePainter: function(ctx, edge) { root.drawEdge(ctx, edge) }
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
