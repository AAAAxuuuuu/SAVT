import QtQuick
import QtQuick.Controls
import "../common"

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

    function drawEdge(ctx, edge) {
        var points = edgeRoutePoints(edge)
        if (!points || points.length < 2)
            return

        var emphasized = edgeTouchesSelection(edge)
        var hasSelection = (selectionState.selectedCapabilityNode && selectionState.selectedCapabilityNode.id !== undefined) ||
                           (selectionState.selectedCapabilityEdge && selectionState.selectedCapabilityEdge.id !== undefined)

        ctx.beginPath()
        ctx.strokeStyle = theme.edgeColor(edge.kind)
        ctx.lineWidth = emphasized ? 3.0 : 1.4
        ctx.globalAlpha = hasSelection ? (emphasized ? 0.92 : 0.12) : 0.52
        ctx.moveTo(points[0].x, points[0].y)

        for (var i = 1; i < points.length; ++i)
            ctx.lineTo(points[i].x, points[i].y)

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
        edgePainter: function(ctx, edge) { root.drawEdge(ctx, edge) }
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
