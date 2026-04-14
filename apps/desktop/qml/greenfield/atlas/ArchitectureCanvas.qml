import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import "../foundation"

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
    readonly property var overviewMindMapLayout: buildOverviewMindMapLayout()
    readonly property bool componentOverviewMode: componentMode
    readonly property var componentOverviewGroups: buildComponentOverviewGroups()
    readonly property real componentOverviewGapX: 34
    readonly property real componentOverviewGapY: 26
    readonly property real componentOverviewSectionGap: 34
    readonly property real componentOverviewHeaderHeight: 34
    readonly property real componentOverviewCardWidth: Math.max(220, Math.min(272,
                                                                               (Math.max(860, width - 220) - 144 - Math.max(0, componentOverviewGroups.length - 1) * componentOverviewGapX) / Math.max(1, componentOverviewGroups.length)))
    readonly property real componentOverviewCardHeight: 126
    readonly property real sceneWidth: componentOverviewMode
                                       ? Math.max(980, 72 * 2 + componentOverviewGroups.length * componentOverviewCardWidth + Math.max(0, componentOverviewGroups.length - 1) * componentOverviewGapX)
                                       : Math.max(980, overviewMindMapLayout.width || 0, bounds.width || 980)
    readonly property real sceneHeight: componentOverviewMode
                                        ? Math.max(560, componentOverviewSceneHeight())
                                        : Math.max(560, overviewMindMapLayout.height || 0, bounds.height || 560)
    readonly property real baseScale: Math.min(1.0, Math.max(0.38, Math.min((width - 120) / sceneWidth, (height - 100) / sceneHeight)))
    readonly property real effectiveScale: baseScale * zoom
    readonly property real contentX: Math.max(40, (width - sceneWidth * effectiveScale) / 2) + panX
    readonly property real contentY: Math.max(38, (height - sceneHeight * effectiveScale) / 2) + panY
    readonly property var visibleEdges: collectVisibleEdges(edges, selectedNode, hoverNode, showAllEdges)
    readonly property bool relationshipFocusActive: !!selectedNode && selectedNode.id !== undefined
    readonly property var focusedIncomingRelations: relationshipFocusItems("incoming")
    readonly property var focusedOutgoingRelations: relationshipFocusItems("outgoing")
    readonly property int componentCount: componentMode ? nodes.length : 0
    property real zoom: 1.0
    property real panX: 0
    property real panY: 0
    property var manualNodePositions: ({})
    property bool draggingNodeCard: false

    signal nodeSelected(var node)
    signal nodeDrilled(var node)
    signal blankClicked()

    function resetView() {
        zoom = 1.0
        panX = 0
        panY = 0
        manualNodePositions = ({})
        draggingNodeCard = false
    }

    function nodePositionKey(nodeId) {
        return String(nodeId)
    }

    function manualNodePosition(nodeId) {
        if (nodeId === undefined || nodeId === null)
            return null
        var key = nodePositionKey(nodeId)
        return manualNodePositions && manualNodePositions[key] ? manualNodePositions[key] : null
    }

    function setManualNodePosition(nodeId, x, y, width, height) {
        if (nodeId === undefined || nodeId === null)
            return

        var nextPositions = Object.assign({}, manualNodePositions || ({}))
        nextPositions[nodePositionKey(nodeId)] = {"x": x, "y": y}
        manualNodePositions = nextPositions
    }

    function clearManualNodePositions() {
        manualNodePositions = ({})
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

    function relationshipFocusItems(direction) {
        if (!relationshipFocusActive)
            return []

        var output = []
        for (var index = 0; index < edges.length; ++index) {
            var edge = edges[index]
            var matches = direction === "incoming"
                          ? edge.toId === selectedNode.id
                          : edge.fromId === selectedNode.id
            if (!matches)
                continue

            var otherNodeId = direction === "incoming" ? edge.fromId : edge.toId
            var node = nodeById(otherNodeId)
            if (!node)
                continue

            var rect = nodeRectById(otherNodeId)
            output.push({
                "edge": edge,
                "node": node,
                "sortY": rect.valid ? rect.y + rect.height / 2 : 0
            })
        }

        output.sort(function(left, right) {
            if (Math.abs(left.sortY - right.sortY) > 0.5)
                return left.sortY - right.sortY
            return String(left.node.name || "").localeCompare(String(right.node.name || ""))
        })
        return output
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

    function overviewRootNode() {
        if (nodes.length === 0)
            return null

        var bestNode = nodes[0]
        var bestScore = -999999
        for (var index = 0; index < nodes.length; ++index) {
            var node = nodes[index]
            var outgoing = 0
            var incoming = 0
            for (var edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
                if (edges[edgeIndex].fromId === node.id)
                    outgoing += 1
                if (edges[edgeIndex].toId === node.id)
                    incoming += 1
            }

            var score = outgoing * 4 - incoming
            if (node.reachableFromEntry || node.kind === "entry" || node.kind === "entry_component")
                score += 64
            if (String(node.role || "").toLowerCase().indexOf("core") >= 0)
                score += 3

            if (score > bestScore) {
                bestScore = score
                bestNode = node
            }
        }
        return bestNode
    }

    function sortedNeighborIds(nodeIds) {
        var copy = nodeIds.slice(0)
        copy.sort(function(leftId, rightId) {
            var left = nodeById(leftId) || {}
            var right = nodeById(rightId) || {}
            return String(left.name || "").localeCompare(String(right.name || ""))
        })
        return copy
    }

    function isOverviewPrimaryEdge(edge) {
        return !componentMode && !!edge && !!(overviewMindMapLayout.primaryEdgeIds && overviewMindMapLayout.primaryEdgeIds[edge.id])
    }

    function buildOverviewMindMapLayout() {
        if (componentMode || nodes.length === 0)
            return {"positions": ({}), "primaryEdgeIds": ({}), "secondaryEdgeIds": ({})}

        var rootNode = overviewRootNode()
        if (!rootNode || rootNode.id === undefined)
            return {"positions": ({}), "primaryEdgeIds": ({}), "secondaryEdgeIds": ({})}

        var positions = {}
        var primaryEdgeIds = {}
        var secondaryEdgeIds = {}
        var outgoingSlotByEdgeId = {}
        var incomingSlotByEdgeId = {}
        var outgoingCountByNodeId = {}
        var incomingCountByNodeId = {}
        var layerById = {}
        var rootId = rootNode.id
        var marginX = 96
        var marginY = 96
        var minX = 999999
        var maxX = -999999
        var minY = 999999
        var maxY = -999999
        var maxLayerIndex = 0

        for (var nodeIndex = 0; nodeIndex < nodes.length; ++nodeIndex) {
            var node = nodes[nodeIndex]
            var nodeX = node && node.x !== undefined ? Number(node.x) : (64 + (nodeIndex % 3) * 320)
            var nodeY = node && node.y !== undefined ? Number(node.y) : (90 + Math.floor(nodeIndex / 3) * 170)
            var width = nodeWidth(node)
            var height = nodeHeight(node)
            var laneIndex = node && node.layoutLaneIndex !== undefined ? Number(node.layoutLaneIndex) : 0

            positions[node.id] = {"x": nodeX, "y": nodeY}
            layerById[node.id] = laneIndex
            maxLayerIndex = Math.max(maxLayerIndex, laneIndex)
            minX = Math.min(minX, nodeX)
            maxX = Math.max(maxX, nodeX + width)
            minY = Math.min(minY, nodeY)
            maxY = Math.max(maxY, nodeY + height)
        }

        if (!isFinite(minX) || !isFinite(minY) || !isFinite(maxX) || !isFinite(maxY)) {
            minX = 0
            minY = 0
            maxX = bounds.width || 980
            maxY = bounds.height || 560
        }

        function sortPeerEdges(edgeList, pivotField, peerField) {
            edgeList.sort(function(left, right) {
                var leftPeer = positions[left[peerField]]
                var rightPeer = positions[right[peerField]]
                var leftPivot = positions[left[pivotField]]
                var rightPivot = positions[right[pivotField]]
                var leftSortY = leftPeer ? leftPeer.y : (leftPivot ? leftPivot.y : 0)
                var rightSortY = rightPeer ? rightPeer.y : (rightPivot ? rightPivot.y : 0)
                if (Math.abs(leftSortY - rightSortY) > 0.5)
                    return leftSortY - rightSortY

                var leftSortX = leftPeer ? leftPeer.x : (leftPivot ? leftPivot.x : 0)
                var rightSortX = rightPeer ? rightPeer.x : (rightPivot ? rightPivot.x : 0)
                if (Math.abs(leftSortX - rightSortX) > 0.5)
                    return leftSortX - rightSortX

                return String(left.id).localeCompare(String(right.id))
            })
        }

        var outgoingById = {}
        var incomingById = {}
        for (var edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
            var edge = edges[edgeIndex]
            primaryEdgeIds[edge.id] = true
            if (!outgoingById[edge.fromId])
                outgoingById[edge.fromId] = []
            if (!incomingById[edge.toId])
                incomingById[edge.toId] = []
            outgoingById[edge.fromId].push(edge)
            incomingById[edge.toId].push(edge)
        }

        for (var nodeIdKey in outgoingById) {
            if (!outgoingById.hasOwnProperty(nodeIdKey))
                continue
            var outgoingEdges = outgoingById[nodeIdKey]
            sortPeerEdges(outgoingEdges, "fromId", "toId")
            outgoingCountByNodeId[nodeIdKey] = outgoingEdges.length
            for (var order = 0; order < outgoingEdges.length; ++order)
                outgoingSlotByEdgeId[outgoingEdges[order].id] = {"index": order, "count": outgoingEdges.length}
        }

        for (nodeIdKey in incomingById) {
            if (!incomingById.hasOwnProperty(nodeIdKey))
                continue
            var incomingEdges = incomingById[nodeIdKey]
            sortPeerEdges(incomingEdges, "toId", "fromId")
            incomingCountByNodeId[nodeIdKey] = incomingEdges.length
            for (order = 0; order < incomingEdges.length; ++order)
                incomingSlotByEdgeId[incomingEdges[order].id] = {"index": order, "count": incomingEdges.length}
        }

        return {
            "positions": positions,
            "primaryEdgeIds": primaryEdgeIds,
            "secondaryEdgeIds": secondaryEdgeIds,
            "outgoingSlotByEdgeId": outgoingSlotByEdgeId,
            "incomingSlotByEdgeId": incomingSlotByEdgeId,
            "outgoingCountByNodeId": outgoingCountByNodeId,
            "incomingCountByNodeId": incomingCountByNodeId,
            "layerById": layerById,
            "layerCount": maxLayerIndex + 1,
            "rootId": rootNode.id,
            "leftIds": [],
            "rightIds": [],
            "width": Math.max(980, bounds.width || 0, maxX + marginX),
            "height": Math.max(560, bounds.height || 0, maxY + marginY)
        }
    }

    function componentOverviewGroupKey(node) {
        if (!node)
            return "other"

        var kind = String(node.kind || "").toLowerCase()
        var role = String(node.role || "").toLowerCase()
        var stageName = String(node.stageName || "").toLowerCase()
        var name = String(node.name || "").toLowerCase()

        if (node.reachableFromEntry || kind === "entry" || kind === "entry_component" || stageName.indexOf("entry") >= 0)
            return "entry"

        if (role.indexOf("presentation") >= 0 || role.indexOf("visual") >= 0 ||
                name.indexOf("ui") >= 0 || name.indexOf("view") >= 0 || name.indexOf("page") >= 0)
            return "experience"

        if (kind === "infrastructure" || kind === "support_component" ||
                role.indexOf("support") >= 0 || role.indexOf("infra") >= 0)
            return "support"

        if (role.indexOf("analysis") >= 0 || role.indexOf("core") >= 0 ||
                kind === "service" || kind === "capability")
            return "core"

        return "other"
    }

    function componentOverviewGroupTitle(groupKey) {
        if (groupKey === "entry")
            return "入口协同"
        if (groupKey === "experience")
            return "界面交互"
        if (groupKey === "core")
            return "核心处理"
        if (groupKey === "support")
            return "支撑基础"
        return "其他组件"
    }

    function buildComponentOverviewGroups() {
        var order = ["entry", "experience", "core", "support", "other"]
        var groupsByKey = {}
        for (var orderIndex = 0; orderIndex < order.length; ++orderIndex) {
            groupsByKey[order[orderIndex]] = {
                "key": order[orderIndex],
                "title": componentOverviewGroupTitle(order[orderIndex]),
                "nodes": []
            }
        }

        for (var index = 0; index < nodes.length; ++index) {
            var node = nodes[index]
            groupsByKey[componentOverviewGroupKey(node)].nodes.push(node)
        }

        var groups = []
        for (var groupIndex = 0; groupIndex < order.length; ++groupIndex) {
            var group = groupsByKey[order[groupIndex]]
            group.nodes.sort(function(left, right) {
                return String(left.name || "").localeCompare(String(right.name || ""))
            })
            if (group.nodes.length > 0)
                groups.push(group)
        }
        return groups
    }

    function componentOverviewSectionLeft(groupIndex) {
        return 72 + groupIndex * (componentOverviewCardWidth + componentOverviewGapX)
    }

    function componentOverviewMaxRows() {
        var maxRows = 1
        for (var index = 0; index < componentOverviewGroups.length; ++index) {
            maxRows = Math.max(maxRows, componentOverviewGroups[index].nodes.length)
        }
        return maxRows
    }

    function componentOverviewPlacement(nodeId, fallbackIndex) {
        for (var groupIndex = 0; groupIndex < componentOverviewGroups.length; ++groupIndex) {
            var group = componentOverviewGroups[groupIndex]
            for (var nodeIndex = 0; nodeIndex < group.nodes.length; ++nodeIndex) {
                if (group.nodes[nodeIndex].id !== nodeId)
                    continue

                return {
                    "x": componentOverviewSectionLeft(groupIndex),
                    "y": 96 + componentOverviewHeaderHeight + 16
                         + nodeIndex * (componentOverviewCardHeight + componentOverviewGapY),
                    "groupIndex": groupIndex
                }
            }
        }

        return {
            "x": componentOverviewSectionLeft(0),
            "y": 96 + componentOverviewHeaderHeight + 16
                 + fallbackIndex * (componentOverviewCardHeight + componentOverviewGapY),
            "groupIndex": -1
        }
    }

    function componentOverviewSceneHeight() {
        var rows = componentOverviewMaxRows()
        return 96
               + componentOverviewHeaderHeight
               + 16
               + rows * componentOverviewCardHeight
               + Math.max(0, rows - 1) * componentOverviewGapY
               + 72
    }

    function sceneX(item, fallbackIndex) {
        var manualPosition = item ? manualNodePosition(item.id) : null
        if (manualPosition)
            return manualPosition.x
        if (componentOverviewMode) {
            return componentOverviewPlacement(item ? item.id : fallbackIndex, fallbackIndex).x
        }
        if (!componentMode && item && item.id !== undefined && overviewMindMapLayout.positions && overviewMindMapLayout.positions[item.id])
            return overviewMindMapLayout.positions[item.id].x
        if (item && item.x !== undefined)
            return item.x
        return 64 + (fallbackIndex % 3) * 320
    }

    function sceneY(item, fallbackIndex) {
        var manualPosition = item ? manualNodePosition(item.id) : null
        if (manualPosition)
            return manualPosition.y
        if (componentOverviewMode) {
            return componentOverviewPlacement(item ? item.id : fallbackIndex, fallbackIndex).y
        }
        if (!componentMode && item && item.id !== undefined && overviewMindMapLayout.positions && overviewMindMapLayout.positions[item.id])
            return overviewMindMapLayout.positions[item.id].y
        if (item && item.y !== undefined)
            return item.y
        return 90 + Math.floor(fallbackIndex / 3) * 170
    }

    function nodeWidth(item) {
        if (componentOverviewMode)
            return componentOverviewCardWidth
        return Math.max(230, Math.min(280, item && item.width ? item.width : 240))
    }

    function nodeHeight(item) {
        if (componentOverviewMode)
            return componentOverviewCardHeight
        return Math.max(136, Math.min(176, item && item.height ? item.height : 142))
    }

    function localizedMetaText(value) {
        var raw = String(value || "")
        var key = raw.toLowerCase()
        if (!key.length)
            return "节点"
        if (key === "entry")
            return "入口"
        if (key === "entry_component")
            return "入口组件"
        if (key === "capability")
            return "能力模块"
        if (key === "service")
            return "服务"
        if (key === "infrastructure")
            return "基础设施"
        if (key === "support_component")
            return "支撑组件"
        if (key === "presentation")
            return "展示层"
        if (key === "visual")
            return "可视层"
        if (key === "analysis")
            return "分析层"
        if (key === "core")
            return "核心模块"
        if (key === "support")
            return "支撑模块"
        if (key === "experience")
            return "交互层"
        if (key === "other")
            return "其他"
        if (key === "node")
            return "节点"
        if (key === "report_context")
            return "报告上下文"
        return raw
    }

    function cardMetaLabel(item) {
        if (!item)
            return "节点"
        if (componentMode && (item.scopeLabel || "").length > 0)
            return localizedMetaText(item.kind || item.role || "node")
        if ((item.scopeLabel || "").length > 0)
            return localizedMetaText(item.scopeLabel)
        return localizedMetaText(item.kind || item.role || "node")
    }

    function cardFileCountLabel(item) {
        var count = item ? (item.fileCount || item.sourceFileCount || 0) : 0
        return count + " 个文件"
    }

    function componentOverviewColumns(viewWidth, compact) {
        var minCardWidth = compact ? 148 : 184
        return Math.max(2, Math.min(compact ? 5 : 6, Math.floor((Math.max(1, viewWidth) + 10) / minCardWidth)))
    }

    function componentOverviewTileWidth(viewWidth, compact) {
        var gap = compact ? 8 : 12
        var columns = componentOverviewColumns(viewWidth, compact)
        return Math.max(compact ? 136 : 164, Math.floor((viewWidth - Math.max(0, columns - 1) * gap) / columns))
    }

    function componentOverviewTileHeight(compact) {
        return compact ? 58 : 82
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

    function overviewHoverRelation(edge) {
        if (!hoverNode || hoverNode.id === undefined || !edge)
            return ""
        if (edge.toId === hoverNode.id)
            return "incoming"
        if (edge.fromId === hoverNode.id)
            return "outgoing"
        return ""
    }

    function collectVisibleEdges(edgeList, selected, hovered, showAll) {
        var output = []
        if (relationshipFocusActive)
            return output

        if (!componentMode) {
            var overviewNode = hovered && hovered.id !== undefined ? hovered : null
            for (var overviewIndex = 0; overviewIndex < edgeList.length; ++overviewIndex) {
                var primaryEdge = edgeList[overviewIndex]
                if (!isOverviewPrimaryEdge(primaryEdge))
                    continue
                if (overviewNode && !(primaryEdge.fromId === overviewNode.id || primaryEdge.toId === overviewNode.id))
                    continue

                output.push({
                    "edge": primaryEdge,
                    "laneIndex": output.length
                })
            }
            return output
        }

        if (!showAll)
            return output

        var node = selected || hovered
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

    function focusOverlayMetrics() {
        var overlayWidth = Math.min(width - 56, 1120)
        var overlayHeight = Math.max(440, height - 116)
        overlayHeight = Math.min(overlayHeight, height - 54)
        var x = (width - overlayWidth) / 2
        var y = 34
        var sideWidth = Math.min(270, overlayWidth * 0.265)
        var centerWidth = Math.min(360, overlayWidth * 0.33)
        var railGap = Math.max(82, (overlayWidth - sideWidth * 2 - centerWidth - 64) / 2)
        var overviewHeight = 0
        return {
            "x": x,
            "y": y,
            "width": overlayWidth,
            "height": overlayHeight,
            "sideWidth": sideWidth,
            "centerWidth": centerWidth,
            "railGap": railGap,
            "overviewHeight": overviewHeight
        }
    }

    function focusRelationGap(count) {
        return count > 5 ? 10 : 14
    }

    function focusRelationCardHeight(count) {
        var metrics = focusOverlayMetrics()
        var usableHeight = Math.max(220, metrics.height - 98 - metrics.overviewHeight)
        return Math.max(62, Math.min(92, (usableHeight - Math.max(0, count - 1) * focusRelationGap(count)) / Math.max(1, count)))
    }

    function focusRelationY(index, count) {
        var metrics = focusOverlayMetrics()
        var cardHeight = focusRelationCardHeight(count)
        var gap = focusRelationGap(count)
        var contentHeight = count * cardHeight + Math.max(0, count - 1) * gap
        var top = metrics.y + (metrics.height - metrics.overviewHeight - contentHeight) / 2
        return top + index * (cardHeight + gap)
    }

    function focusCenterRect() {
        var metrics = focusOverlayMetrics()
        var width = metrics.centerWidth
        var height = componentMode ? 188 : 176
        return {
            "x": metrics.x + (metrics.width - width) / 2,
            "y": metrics.y + (metrics.height - metrics.overviewHeight - height) / 2,
            "width": width,
            "height": height
        }
    }

    function focusRelationRect(direction, index, count) {
        var metrics = focusOverlayMetrics()
        var width = metrics.sideWidth
        var height = focusRelationCardHeight(count)
        var x = direction === "incoming"
                ? metrics.x + 18
                : metrics.x + metrics.width - width - 18
        return {
            "x": x,
            "y": focusRelationY(index, count),
            "width": width,
            "height": height
        }
    }

    function relationAnchorPoint(direction, index, count) {
        var rect = focusRelationRect(direction, index, count)
        return Qt.point(direction === "incoming" ? rect.x + rect.width : rect.x,
                        rect.y + rect.height / 2)
    }

    function centerAnchorPoint(direction) {
        var rect = focusCenterRect()
        return Qt.point(direction === "incoming" ? rect.x : rect.x + rect.width,
                        rect.y + rect.height / 2)
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

    function routeObjectHitsModules(route, edge) {
        if (!route)
            return false
        return routeHitsModules([route.p0, route.p1, route.p2, route.p3], edge)
    }

    function routeMetric(route) {
        if (!route)
            return 999999
        return pointDistance(route.p0, route.p1)
               + pointDistance(route.p1, route.p2)
               + pointDistance(route.p2, route.p3)
    }

    function orthogonalRouteObject(points) {
        return {
            "p0": points[0],
            "p1": points[1],
            "p2": points[2],
            "p3": points[3],
            "style": "orthogonal"
        }
    }

    function endpointPeerOffset(edge, endpointKey, step) {
        var matches = []
        for (var index = 0; index < visibleEdges.length; ++index) {
            var candidate = visibleEdges[index].edge
            if (candidate && candidate[endpointKey] === edge[endpointKey])
                matches.push(candidate)
        }

        if (matches.length <= 1)
            return 0

        for (var matchIndex = 0; matchIndex < matches.length; ++matchIndex) {
            if (matches[matchIndex].id === edge.id)
                return (matchIndex - (matches.length - 1) / 2) * step
        }

        return 0
    }

    function nodeCenter(rect) {
        return Qt.point(rect.x + rect.width / 2, rect.y + rect.height / 2)
    }

    function focusRailGroupKey(edge) {
        var focus = focusNode()
        if (!focus || !componentMode)
            return ""
        if (edge.fromId === focus.id)
            return "outgoing"
        if (edge.toId === focus.id)
            return "incoming"
        return ""
    }

    function focusRailMembers(groupKey) {
        var focus = focusNode()
        if (!focus || !groupKey)
            return []

        var matches = []
        for (var index = 0; index < visibleEdges.length; ++index) {
            var candidate = visibleEdges[index].edge
            if (!candidate || focusRailGroupKey(candidate) !== groupKey)
                continue

            var otherNodeId = candidate.fromId === focus.id ? candidate.toId : candidate.fromId
            var otherRect = nodeRectById(otherNodeId)
            if (!otherRect.valid)
                continue

            matches.push({
                "edge": candidate,
                "otherNodeId": otherNodeId,
                "sortY": otherRect.y + otherRect.height / 2
            })
        }

        matches.sort(function(left, right) {
            if (Math.abs(left.sortY - right.sortY) > 0.5)
                return left.sortY - right.sortY
            return String(left.edge.id).localeCompare(String(right.edge.id))
        })
        return matches
    }

    function focusRailPlacement(edge) {
        var groupKey = focusRailGroupKey(edge)
        if (!groupKey)
            return {"valid": false, "groupKey": "", "index": 0, "count": 0}

        var members = focusRailMembers(groupKey)
        for (var index = 0; index < members.length; ++index) {
            if (members[index].edge.id === edge.id) {
                return {
                    "valid": true,
                    "groupKey": groupKey,
                    "index": index,
                    "count": members.length
                }
            }
        }

        return {"valid": false, "groupKey": groupKey, "index": 0, "count": members.length}
    }

    function overviewHoverRailMembers(groupKey) {
        if (!hoverNode || hoverNode.id === undefined || componentMode || !groupKey)
            return []

        var matches = []
        for (var index = 0; index < edges.length; ++index) {
            var candidate = edges[index]
            if (!candidate || !isOverviewPrimaryEdge(candidate))
                continue

            var relation = overviewHoverRelation(candidate)
            if (relation !== groupKey)
                continue

            var otherNodeId = relation === "outgoing" ? candidate.toId : candidate.fromId
            var otherRect = nodeRectById(otherNodeId)
            if (!otherRect.valid)
                continue

            matches.push({
                "edge": candidate,
                "otherNodeId": otherNodeId,
                "sortX": otherRect.x + otherRect.width / 2
            })
        }

        matches.sort(function(left, right) {
            if (Math.abs(left.sortX - right.sortX) > 0.5)
                return left.sortX - right.sortX
            return String(left.edge.id).localeCompare(String(right.edge.id))
        })
        return matches
    }

    function overviewHoverRailPlacement(edge) {
        var groupKey = overviewHoverRelation(edge)
        if (!groupKey)
            return {"valid": false, "groupKey": "", "index": 0, "count": 0}

        var members = overviewHoverRailMembers(groupKey)
        for (var index = 0; index < members.length; ++index) {
            if (members[index].edge.id === edge.id) {
                return {
                    "valid": true,
                    "groupKey": groupKey,
                    "index": index,
                    "count": members.length
                }
            }
        }

        return {"valid": false, "groupKey": groupKey, "index": 0, "count": members.length}
    }

    function hoveredOverviewPrimaryRoute(edge) {
        if (componentMode || !hoverNode || hoverNode.id === undefined || !isOverviewPrimaryEdge(edge))
            return null

        var relation = overviewHoverRelation(edge)
        if (!relation)
            return null

        var focusRect = nodeRectById(hoverNode.id)
        var otherRect = nodeRectById(relation === "outgoing" ? edge.toId : edge.fromId)
        if (!focusRect.valid || !otherRect.valid)
            return null

        var placement = overviewHoverRailPlacement(edge)
        if (!placement.valid)
            return null

        var fromLayer = overviewMindMapLayout.layerById ? overviewMindMapLayout.layerById[edge.fromId] : undefined
        var toLayer = overviewMindMapLayout.layerById ? overviewMindMapLayout.layerById[edge.toId] : undefined
        var verticalRelation = fromLayer !== undefined && toLayer !== undefined && fromLayer !== toLayer
        var side = relation === "outgoing"
                   ? (verticalRelation ? (toLayer > fromLayer ? "bottom" : "top") : "right")
                   : (verticalRelation ? (fromLayer < toLayer ? "top" : "bottom") : "left")
        var direction = side === "bottom" || side === "right" ? 1 : -1
        var railSpacing = 26
        var portSpacing = 12
        var centerOffset = placement.count > 1 ? (placement.index - (placement.count - 1) / 2) : 0
        var otherCenter = nodeCenter(otherRect)
        var focusPortOffset = centerOffset * portSpacing
        var peerPortOffset = centerOffset * (portSpacing * 0.7)
        var focusPort = portPoint(focusRect, side, focusPortOffset)

        if (verticalRelation) {
            var railBaseY = (side === "top" ? focusRect.y : focusRect.y + focusRect.height) + direction * 52
            var railY = railBaseY + direction * centerOffset * railSpacing
            var peerSide = railY <= otherCenter.y ? "top" : "bottom"
            var peerPort = portPoint(otherRect, peerSide, peerPortOffset)

            if (relation === "outgoing") {
                return {
                    "p0": focusPort,
                    "p1": Qt.point(focusPort.x, railY),
                    "p2": Qt.point(peerPort.x, railY),
                    "p3": peerPort
                }
            }

            return {
                "p0": peerPort,
                "p1": Qt.point(peerPort.x, railY),
                "p2": Qt.point(focusPort.x, railY),
                "p3": focusPort
            }
        }

        var railBaseX = (side === "left" ? focusRect.x : focusRect.x + focusRect.width) + direction * 52
        var railX = railBaseX + direction * centerOffset * railSpacing
        var peerHorizontalSide = railX <= otherCenter.x ? "left" : "right"
        var horizontalPeerPort = portPoint(otherRect, peerHorizontalSide, peerPortOffset)

        if (relation === "outgoing") {
            return {
                "p0": focusPort,
                "p1": Qt.point(railX, focusPort.y),
                "p2": Qt.point(railX, horizontalPeerPort.y),
                "p3": horizontalPeerPort
            }
        }

        return {
            "p0": horizontalPeerPort,
            "p1": Qt.point(railX, horizontalPeerPort.y),
            "p2": Qt.point(railX, focusPort.y),
            "p3": focusPort
        }
    }

    function focusedComponentRoute(edge) {
        var focus = focusNode()
        if (!componentMode || !focus || !edgeTouchesFocus(edge))
            return null

        var focusIsSource = edge.fromId === focus.id
        var focusRect = nodeRectById(focus.id)
        var otherRect = nodeRectById(focusIsSource ? edge.toId : edge.fromId)
        if (!focusRect.valid || !otherRect.valid)
            return null

        var placement = focusRailPlacement(edge)
        if (!placement.valid)
            return null

        var side = placement.groupKey === "outgoing" ? "left" : "right"
        var direction = side === "left" ? -1 : 1
        var railSpacing = 22
        var portSpacing = 11
        var centerOffset = placement.count > 1 ? (placement.index - (placement.count - 1) / 2) : 0
        var focusCenter = nodeCenter(focusRect)
        var otherCenter = nodeCenter(otherRect)
        var focusPortOffset = centerOffset * portSpacing
        var peerPortOffset = centerOffset * (portSpacing * 0.75)
        var railBaseX = (side === "left" ? focusRect.x : focusRect.x + focusRect.width) + direction * 40
        var railX = railBaseX + direction * centerOffset * railSpacing
        var peerSide = railX <= otherCenter.x ? "left" : "right"

        var focusPort = portPoint(focusRect, side, focusPortOffset)
        var peerPort = portPoint(otherRect, peerSide, peerPortOffset)

        if (focusIsSource) {
            return {
                "p0": focusPort,
                "p1": Qt.point(railX, focusPort.y),
                "p2": Qt.point(railX, peerPort.y),
                "p3": peerPort
            }
        }

        return {
            "p0": peerPort,
            "p1": Qt.point(railX, peerPort.y),
            "p2": Qt.point(railX, focusPort.y),
            "p3": focusPort
        }
    }

    function routeFromScenePoints(edge, laneIndex) {
        if (!componentMode)
            return null

        if (!edge || !edge.routePoints || edge.routePoints.length < 2)
            return null

        var focusedRoute = focusedComponentRoute(edge)
        if (focusedRoute)
            return focusedRoute

        var points = edge.routePoints
        var startPoint = Qt.point(points[0].x, points[0].y)
        var sourceOffset = endpointPeerOffset(edge, "fromId", componentMode ? 10 : 12)
        var targetOffset = endpointPeerOffset(edge, "toId", componentMode ? 8 : 10)
        if (points.length >= 4) {
            var p1 = Qt.point(points[1].x, points[1].y)
            var p2 = Qt.point(points[2].x, points[2].y)
            var p3 = Qt.point(points[points.length - 1].x, points[points.length - 1].y)

            if (Math.abs(p1.x - startPoint.x) >= Math.abs(p1.y - startPoint.y)) {
                startPoint.y += sourceOffset
                p1.y += sourceOffset
            } else {
                startPoint.x += sourceOffset
                p1.x += sourceOffset
            }

            if (Math.abs(p3.x - p2.x) >= Math.abs(p3.y - p2.y)) {
                p2.y += targetOffset
                p3.y += targetOffset
            } else {
                p2.x += targetOffset
                p3.x += targetOffset
            }

            var routed = {
                "p0": startPoint,
                "p1": p1,
                "p2": p2,
                "p3": p3,
                "style": componentMode ? "curved" : "orthogonal"
            }

            if (!componentMode && routeObjectHitsModules(routed, edge)) {
                var fromRect = nodeRectById(edge.fromId)
                var toRect = nodeRectById(edge.toId)
                if (fromRect.valid && toRect.valid) {
                    var vertical = Math.abs(routed.p3.y - routed.p0.y) >= Math.abs(routed.p3.x - routed.p0.x) * 0.72
                    var bypass = bypassRoute(fromRect, toRect, laneIndex, vertical)
                    return {
                        "p0": bypass[0],
                        "p1": bypass[1],
                        "p2": bypass[2],
                        "p3": bypass[3],
                        "style": "orthogonal"
                    }
                }
            }

            return routed
        }

        var endPoint = Qt.point(points[points.length - 1].x, points[points.length - 1].y)
        var laneOffset = componentMode ? edgeLane(laneIndex) * 0.18 : 0
        sourceOffset += laneOffset
        targetOffset -= laneOffset * 0.6
        var horizontal = Math.abs(endPoint.x - startPoint.x) >= Math.abs(endPoint.y - startPoint.y)

        if (horizontal) {
            var bundleBaseX = startPoint.x + (endPoint.x - startPoint.x) * 0.34
            var bundleX = bundleBaseX + (sourceOffset - targetOffset) * 0.9
            var minStubX = Math.min(startPoint.x, endPoint.x) + 28
            var maxStubX = Math.max(startPoint.x, endPoint.x) - 28
            bundleX = Math.max(minStubX, Math.min(maxStubX, bundleX))

            var horizontalRoute = {
                "p0": Qt.point(startPoint.x, startPoint.y + sourceOffset),
                "p1": Qt.point(bundleX, startPoint.y + sourceOffset),
                "p2": Qt.point(bundleX, endPoint.y + targetOffset),
                "p3": Qt.point(endPoint.x, endPoint.y + targetOffset),
                "style": componentMode ? "curved" : "orthogonal"
            }
            if (!componentMode && routeObjectHitsModules(horizontalRoute, edge)) {
                var fromRect = nodeRectById(edge.fromId)
                var toRect = nodeRectById(edge.toId)
                if (fromRect.valid && toRect.valid) {
                    var bypass = bypassRoute(fromRect, toRect, laneIndex, false)
                    return {
                        "p0": bypass[0],
                        "p1": bypass[1],
                        "p2": bypass[2],
                        "p3": bypass[3],
                        "style": "orthogonal"
                    }
                }
            }
            return horizontalRoute
        }

        var bundleBaseY = startPoint.y + (endPoint.y - startPoint.y) * 0.5
        var bundleY = bundleBaseY + (sourceOffset - targetOffset) * 0.8
        var minStubY = Math.min(startPoint.y, endPoint.y) + 26
        var maxStubY = Math.max(startPoint.y, endPoint.y) - 26
        bundleY = Math.max(minStubY, Math.min(maxStubY, bundleY))

        var verticalRoute = {
            "p0": Qt.point(startPoint.x + sourceOffset, startPoint.y),
            "p1": Qt.point(startPoint.x + sourceOffset, bundleY),
            "p2": Qt.point(endPoint.x + targetOffset, bundleY),
            "p3": Qt.point(endPoint.x + targetOffset, endPoint.y),
            "style": componentMode ? "curved" : "orthogonal"
        }
        if (!componentMode && routeObjectHitsModules(verticalRoute, edge)) {
            var sourceRect = nodeRectById(edge.fromId)
            var sinkRect = nodeRectById(edge.toId)
            if (sourceRect.valid && sinkRect.valid) {
                var bypassVertical = bypassRoute(sourceRect, sinkRect, laneIndex, true)
                return {
                    "p0": bypassVertical[0],
                    "p1": bypassVertical[1],
                    "p2": bypassVertical[2],
                    "p3": bypassVertical[3],
                    "style": "orthogonal"
                }
            }
        }
        return verticalRoute
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

    function overviewMindMapRoute(edge, laneIndex) {
        if (componentMode)
            return null

        var hoveredRoute = hoveredOverviewPrimaryRoute(edge)
        if (hoveredRoute)
            return hoveredRoute

        var fromRect = nodeRectById(edge.fromId)
        var toRect = nodeRectById(edge.toId)
        if (!fromRect.valid || !toRect.valid)
            return null

        var fromCenter = nodeCenter(fromRect)
        var toCenter = nodeCenter(toRect)
        var outgoingSlot = overviewMindMapLayout.outgoingSlotByEdgeId
                           ? overviewMindMapLayout.outgoingSlotByEdgeId[edge.id]
                           : null
        var incomingSlot = overviewMindMapLayout.incomingSlotByEdgeId
                           ? overviewMindMapLayout.incomingSlotByEdgeId[edge.id]
                           : null
        var outgoingCount = overviewMindMapLayout.outgoingCountByNodeId
                            ? (overviewMindMapLayout.outgoingCountByNodeId[edge.fromId] || 1)
                            : 1
        var incomingCount = overviewMindMapLayout.incomingCountByNodeId
                            ? (overviewMindMapLayout.incomingCountByNodeId[edge.toId] || 1)
                            : 1
        var outgoingOffset = outgoingSlot ? (outgoingSlot.index - (outgoingCount - 1) / 2) * 14 : 0
        var incomingOffset = incomingSlot ? (incomingSlot.index - (incomingCount - 1) / 2) * 14 : 0
        var preferVertical = Math.abs(toCenter.y - fromCenter.y) >= Math.abs(toCenter.x - fromCenter.x) * 0.72
        var candidates = []

        var startVerticalSide = toCenter.y >= fromCenter.y ? "bottom" : "top"
        var endVerticalSide = toCenter.y >= fromCenter.y ? "top" : "bottom"
        var verticalStart = portPoint(fromRect, startVerticalSide, outgoingOffset)
        var verticalEnd = portPoint(toRect, endVerticalSide, incomingOffset)
        var verticalMidY = (verticalStart.y + verticalEnd.y) / 2
        candidates.push({
            "route": {
                "p0": verticalStart,
                "p1": Qt.point(verticalStart.x, verticalMidY),
                "p2": Qt.point(verticalEnd.x, verticalMidY),
                "p3": verticalEnd,
                "style": "orthogonal"
            },
            "preferred": preferVertical ? 0 : 1
        })

        var horizontalDirection = toCenter.x >= fromCenter.x ? 1 : -1
        var horizontalStart = portPoint(fromRect, horizontalDirection > 0 ? "right" : "left", outgoingOffset)
        var horizontalEnd = portPoint(toRect, horizontalDirection > 0 ? "left" : "right", incomingOffset)
        var horizontalMidX = (horizontalStart.x + horizontalEnd.x) / 2
        candidates.push({
            "route": {
                "p0": horizontalStart,
                "p1": Qt.point(horizontalMidX, horizontalStart.y),
                "p2": Qt.point(horizontalMidX, horizontalEnd.y),
                "p3": horizontalEnd,
                "style": "orthogonal"
            },
            "preferred": preferVertical ? 1 : 0
        })

        var bypassHorizontal = orthogonalRouteObject(bypassRoute(fromRect, toRect, laneIndex, false))
        var bypassVertical = orthogonalRouteObject(bypassRoute(fromRect, toRect, laneIndex, true))
        candidates.push({"route": bypassHorizontal, "preferred": preferVertical ? 3 : 2})
        candidates.push({"route": bypassVertical, "preferred": preferVertical ? 2 : 3})

        candidates.sort(function(left, right) {
            var leftHits = routeObjectHitsModules(left.route, edge) ? 1 : 0
            var rightHits = routeObjectHitsModules(right.route, edge) ? 1 : 0
            if (leftHits !== rightHits)
                return leftHits - rightHits
            if (left.preferred !== right.preferred)
                return left.preferred - right.preferred
            return routeMetric(left.route) - routeMetric(right.route)
        })

        return candidates.length > 0 ? candidates[0].route : null
    }

    function routeEdge(edge, laneIndex) {
        var overviewRoute = overviewMindMapRoute(edge, laneIndex)
        if (overviewRoute)
            return overviewRoute

        var mappedRoute = routeFromScenePoints(edge, laneIndex)
        if (mappedRoute)
            return mappedRoute

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

    function pointDistance(left, right) {
        var dx = right.x - left.x
        var dy = right.y - left.y
        return Math.sqrt(dx * dx + dy * dy)
    }

    function interpolatePoint(fromPoint, toPoint, amount) {
        var t = Math.max(0, Math.min(1, amount))
        return Qt.point(fromPoint.x + (toPoint.x - fromPoint.x) * t,
                        fromPoint.y + (toPoint.y - fromPoint.y) * t)
    }

    function mixColor(left, right, amount) {
        var t = Math.max(0, Math.min(1, amount))
        return Qt.rgba(left.r * (1 - t) + right.r * t,
                       left.g * (1 - t) + right.g * t,
                       left.b * (1 - t) + right.b * t,
                       left.a * (1 - t) + right.a * t)
    }

    function edgeCurveLeadControl(route, fromStart) {
        var startPoint = fromStart ? route.p0 : route.p3
        var anchorPoint = fromStart ? route.p1 : route.p2
        var bendPoint = fromStart ? route.p2 : route.p1
        var straightSpan = pointDistance(startPoint, anchorPoint)
        var bendSpan = pointDistance(anchorPoint, bendPoint)
        var straightRatio = straightSpan > 56 ? 0.94 : (straightSpan > 24 ? 0.86 : 0.7)
        var bendRatio = bendSpan > 88 ? 0.08 : 0.14
        var straightLead = interpolatePoint(startPoint, anchorPoint, straightRatio)
        return interpolatePoint(straightLead, bendPoint, bendRatio)
    }

    function overviewEdgeBaseColor(edge) {
        var kind = String(edge && edge.kind ? edge.kind : "").toLowerCase()
        if (kind === "activates")
            return Qt.rgba(0.20, 0.64, 0.74, 0.9)
        if (kind === "uses_infrastructure")
            return Qt.rgba(0.60, 0.55, 0.46, 0.88)
        if (kind === "enables")
            return Qt.rgba(0.14, 0.47, 0.98, 0.9)
        return Qt.rgba(0.24, 0.49, 0.86, 0.88)
    }

    function edgeColor(edge) {
        if (!componentMode) {
            var hoverRelation = overviewHoverRelation(edge)
            if (hoverRelation === "incoming")
                return tokens.signalRaspberry
            if (hoverRelation === "outgoing")
                return tokens.signalCobalt
            if (isOverviewPrimaryEdge(edge))
                return overviewEdgeBaseColor(edge)
            return mixColor(Qt.rgba(0.33, 0.43, 0.56, 0.62), overviewEdgeBaseColor(edge), 0.32)
        }

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
        enabled: !root.draggingNodeCard
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
    }

    WheelHandler {
        id: mouseWheelZoom
        target: null
        acceptedDevices: PointerDevice.Mouse
        onWheel: function(event) {
            root.zoomAt(point.position.x, point.position.y,
                        event.angleDelta.y > 0 ? 1.12 : 0.88)
        }
    }

    WheelHandler {
        id: touchpadPan
        target: null
        acceptedDevices: PointerDevice.TouchPad
        onWheel: function(event) {
            var deltaX = event.pixelDelta.x
            var deltaY = event.pixelDelta.y
            if (Math.abs(deltaX) < 0.01 && Math.abs(deltaY) < 0.01) {
                deltaX = event.angleDelta.x / 4.0
                deltaY = event.angleDelta.y / 4.0
            }

            root.panX += deltaX
            root.panY += deltaY
        }
    }

    PinchHandler {
        id: canvasPinch
        target: null
        acceptedDevices: PointerDevice.TouchPad | PointerDevice.TouchScreen
        property real lastScale: 1.0

        onActiveChanged: {
            if (active)
                lastScale = 1.0
        }

        onScaleChanged: {
            if (!active)
                return

            var factor = scale / lastScale
            if (Math.abs(factor - 1.0) < 0.001)
                return

            root.zoomAt(centroid.position.x, centroid.position.y, factor)
            lastScale = scale
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
        opacity: root.relationshipFocusActive ? (root.componentMode ? 0.08 : 0.04) : 1.0

        Behavior on opacity { NumberAnimation { duration: 140 } }

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
                property bool emphasized: root.edgeTouchesFocus(edge)
                property real strokeScale: Math.max(0.38, root.effectiveScale)
                property real arrowSize: (root.componentMode ? 10.5 : 9) / strokeScale
                property point arrowLeft: root.arrowWing(p3, p2, arrowSize, -1)
                property point arrowRight: root.arrowWing(p3, p2, arrowSize, 1)
                property point curveControl1: root.edgeCurveLeadControl(route, true)
                property point curveControl2: root.edgeCurveLeadControl(route, false)
                property color lineColor: root.edgeColor(edge)
                property bool overviewPrimary: root.isOverviewPrimaryEdge(edge)
                property bool overviewCurved: route.style === "curved"
                property string overviewRelation: root.overviewHoverRelation(edge)
                property real lineOpacity: root.componentMode
                                           ? (root.focusNode() ? (emphasized ? 0.96 : 0.34) : 0.7)
                                           : (overviewPrimary
                                              ? (root.hoverNode
                                                 ? (overviewRelation === "incoming" || overviewRelation === "outgoing" ? 0.94 : 0.22)
                                                 : 0.84)
                                              : (emphasized ? 0.82 : 0.22))
                property real haloOpacity: root.componentMode
                                           ? (root.focusNode() ? (emphasized ? 0.24 : 0.09) : 0.16)
                                           : (overviewPrimary
                                              ? (root.hoverNode ? 0.1 : 0.13)
                                              : (emphasized ? 0.1 : 0.04))
                property color haloColor: root.componentMode
                                          ? Qt.rgba(1, 1, 1, haloOpacity)
                                          : Qt.rgba(edgeShape.lineColor.r, edgeShape.lineColor.g, edgeShape.lineColor.b, haloOpacity * 0.42)
                property color inactiveColor: "transparent"

                anchors.fill: parent
                z: overviewPrimary ? 1 : (emphasized ? 2 : 0)
                opacity: lineOpacity
                antialiasing: true

                ShapePath {
                    strokeWidth: (root.componentMode ? (root.focusNode() ? 5.4 : 4.2) : (overviewPrimary ? 4.0 : 3.0)) / edgeShape.strokeScale
                    strokeColor: edgeShape.overviewCurved ? edgeShape.inactiveColor
                                                          : (root.componentMode ? edgeShape.inactiveColor : edgeShape.haloColor)
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
                    strokeWidth: (root.componentMode ? (root.focusNode() ? 5.4 : 4.2) : (overviewPrimary ? 4.0 : 3.0)) / edgeShape.strokeScale
                    strokeColor: edgeShape.overviewCurved ? edgeShape.haloColor
                                                          : (root.componentMode ? edgeShape.haloColor : edgeShape.inactiveColor)
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    startX: edgeShape.p0.x
                    startY: edgeShape.p0.y

                    PathCubic {
                        control1X: edgeShape.curveControl1.x
                        control1Y: edgeShape.curveControl1.y
                        control2X: edgeShape.curveControl2.x
                        control2Y: edgeShape.curveControl2.y
                        x: edgeShape.p3.x
                        y: edgeShape.p3.y
                    }
                }

                ShapePath {
                    strokeWidth: (root.componentMode ? (root.focusNode() ? 2.6 : 2.0) : (overviewPrimary ? 2.2 : 1.6)) / edgeShape.strokeScale
                    strokeColor: edgeShape.overviewCurved ? edgeShape.inactiveColor
                                                          : (root.componentMode ? edgeShape.inactiveColor : edgeShape.lineColor)
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
                    strokeWidth: (root.componentMode ? (root.focusNode() ? 2.6 : 2.0) : (overviewPrimary ? 2.2 : 1.6)) / edgeShape.strokeScale
                    strokeColor: edgeShape.overviewCurved ? edgeShape.lineColor
                                                          : (root.componentMode ? edgeShape.lineColor : edgeShape.inactiveColor)
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    startX: edgeShape.p0.x
                    startY: edgeShape.p0.y

                    PathCubic {
                        control1X: edgeShape.curveControl1.x
                        control1Y: edgeShape.curveControl1.y
                        control2X: edgeShape.curveControl2.x
                        control2Y: edgeShape.curveControl2.y
                        x: edgeShape.p3.x
                        y: edgeShape.p3.y
                    }
                }

                ShapePath {
                    strokeWidth: 0
                    strokeColor: "transparent"
                    fillColor: root.componentMode ? edgeShape.haloColor : edgeShape.haloColor
                    startX: edgeShape.p3.x
                    startY: edgeShape.p3.y

                    PathLine { x: edgeShape.arrowLeft.x; y: edgeShape.arrowLeft.y }
                    PathLine { x: edgeShape.arrowRight.x; y: edgeShape.arrowRight.y }
                    PathLine { x: edgeShape.p3.x; y: edgeShape.p3.y }
                }

                ShapePath {
                    strokeWidth: 0
                    strokeColor: "transparent"
                    fillColor: root.componentMode ? edgeShape.lineColor : edgeShape.lineColor
                    startX: edgeShape.p3.x
                    startY: edgeShape.p3.y

                    PathLine { x: edgeShape.arrowLeft.x; y: edgeShape.arrowLeft.y }
                    PathLine { x: edgeShape.arrowRight.x; y: edgeShape.arrowRight.y }
                    PathLine { x: edgeShape.p3.x; y: edgeShape.p3.y }
                }
            }
        }

        Repeater {
            model: root.componentOverviewMode ? root.componentOverviewGroups : []

            Rectangle {
                x: root.componentOverviewSectionLeft(index)
                y: 96
                width: root.componentOverviewCardWidth
                height: root.componentOverviewHeaderHeight
                radius: root.tokens.radius8
                color: Qt.rgba(1, 1, 1, 0.84)
                border.color: Qt.rgba(0.12, 0.18, 0.28, 0.06)
                visible: !root.relationshipFocusActive

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 10
                        Layout.preferredHeight: 10
                        radius: width / 2
                        color: modelData.key === "entry"
                               ? root.tokens.signalTeal
                               : modelData.key === "experience"
                                 ? root.tokens.signalRaspberry
                                 : modelData.key === "core"
                                   ? root.tokens.signalCobalt
                                   : modelData.key === "support"
                                     ? Qt.rgba(0.96, 0.58, 0.18, 0.88)
                                     : Qt.rgba(0.48, 0.54, 0.68, 0.82)
                    }

                    Label {
                        text: modelData.title
                        color: root.tokens.text1
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    Rectangle {
                        radius: height / 2
                        color: Qt.rgba(0.14, 0.48, 1, 0.08)
                        border.color: Qt.rgba(0.14, 0.48, 1, 0.18)
                        implicitWidth: sectionCountLabel.implicitWidth + 16
                        implicitHeight: 24

                        Label {
                            id: sectionCountLabel
                            anchors.centerIn: parent
                            text: modelData.nodes.length + " 个"
                            color: root.tokens.signalCobalt
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                    }

                    Label {
                        text: modelData.key === "entry"
                              ? "优先看到入口与触发点"
                              : modelData.key === "experience"
                                ? "界面、交互和展示层"
                                : modelData.key === "core"
                                  ? "核心处理与业务能力"
                                  : modelData.key === "support"
                                    ? "基础设施与通用支撑"
                                    : "暂未归类的剩余组件"
                        color: root.tokens.text3
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                    }
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
                            radius: width / 2
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
                        Layout.preferredHeight: 50
                        text: modelData.summary || modelData.responsibility || modelData.role || "暂无描述。"
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
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
                        spacing: 8

                        Rectangle {
                            visible: root.componentMode && (modelData.scopeLabel || "").length > 0
                            radius: height / 2
                            color: Qt.rgba(0.14, 0.48, 1, 0.08)
                            border.color: Qt.rgba(0.14, 0.48, 1, 0.16)
                            implicitWidth: scopeLabel.implicitWidth + 14
                            implicitHeight: 22

                            Label {
                                id: scopeLabel
                                anchors.centerIn: parent
                                text: modelData.scopeLabel || ""
                                color: root.tokens.signalCobalt
                                font.family: root.tokens.textFontFamily
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                        }

                        Label {
                            text: root.cardMetaLabel(modelData)
                            color: root.tokens.text3
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Label {
                            text: root.cardFileCountLabel(modelData)
                            color: root.tokens.text3
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 11
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton
                    pressAndHoldInterval: 260
                    cursorShape: dragArmed ? Qt.ClosedHandCursor : Qt.PointingHandCursor
                    property bool dragArmed: false
                    property bool dragMoved: false
                    property real pressSceneX: 0
                    property real pressSceneY: 0
                    property real pressCardX: 0
                    property real pressCardY: 0
                    onPressed: dragMoved = false
                    onEntered: {
                        card.hovered = true
                        root.hoverNode = modelData
                    }
                    onExited: {
                        card.hovered = false
                        if (root.hoverNode && root.hoverNode.id === modelData.id)
                            root.hoverNode = null
                    }
                    onPressAndHold: function(mouse) {
                        if (root.componentMode || root.relationshipFocusActive)
                            return
                        var scenePoint = card.mapToItem(contentLayer, mouse.x, mouse.y)
                        dragArmed = true
                        dragMoved = true
                        pressSceneX = scenePoint.x
                        pressSceneY = scenePoint.y
                        pressCardX = card.x
                        pressCardY = card.y
                        root.draggingNodeCard = true
                    }
                    onPositionChanged: function(mouse) {
                        if (!dragArmed || !pressed)
                            return

                        var scenePoint = card.mapToItem(contentLayer, mouse.x, mouse.y)
                        var nextX = pressCardX + (scenePoint.x - pressSceneX)
                        var nextY = pressCardY + (scenePoint.y - pressSceneY)
                        if (Math.abs(nextX - pressCardX) + Math.abs(nextY - pressCardY) > 1)
                            dragMoved = true
                        root.setManualNodePosition(modelData.id, nextX, nextY, card.width, card.height)
                    }
                    onReleased: {
                        dragArmed = false
                        root.draggingNodeCard = false
                    }
                    onCanceled: {
                        dragArmed = false
                        root.draggingNodeCard = false
                    }
                    onClicked: function(mouse) {
                        if (dragMoved || dragArmed)
                            return
                        root.nodeSelected(modelData)
                    }
                    onDoubleClicked: function(mouse) {
                        if (dragMoved || dragArmed)
                            return
                        root.nodeDrilled(modelData)
                    }
                }
            }
        }
    }

    Item {
        id: relationshipFocusLayer
        anchors.fill: parent
        visible: root.relationshipFocusActive
        z: 18

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: root.blankClicked()
        }

        Rectangle {
            anchors.fill: parent
            color: root.componentMode ? Qt.rgba(0.985, 0.988, 0.994, 0.94) : Qt.rgba(0.97, 0.975, 0.985, 0.82)
        }

        Canvas {
            id: relationshipCanvas
            anchors.fill: parent

            function drawCurve(ctx, startPoint, endPoint, color, direction) {
                var deltaX = Math.abs(endPoint.x - startPoint.x)
                var bend = Math.max(56, Math.min(132, deltaX * 0.42))
                var c1x = startPoint.x + (direction === "incoming" ? bend : -bend)
                var c2x = endPoint.x + (direction === "incoming" ? -bend * 0.55 : bend * 0.55)
                var c1y = startPoint.y
                var c2y = endPoint.y

                ctx.beginPath()
                ctx.moveTo(startPoint.x, startPoint.y)
                ctx.bezierCurveTo(c1x, c1y, c2x, c2y, endPoint.x, endPoint.y)
                ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.92)
                ctx.lineWidth = 6
                ctx.stroke()

                ctx.beginPath()
                ctx.moveTo(startPoint.x, startPoint.y)
                ctx.bezierCurveTo(c1x, c1y, c2x, c2y, endPoint.x, endPoint.y)
                ctx.strokeStyle = color
                ctx.lineWidth = 2.6
                ctx.stroke()

                var tail = Qt.point(endPoint.x + (direction === "incoming" ? -18 : 18), endPoint.y)
                var arrowLeft = root.arrowWing(endPoint, tail, 11, -1)
                var arrowRight = root.arrowWing(endPoint, tail, 11, 1)
                ctx.beginPath()
                ctx.moveTo(endPoint.x, endPoint.y)
                ctx.lineTo(arrowLeft.x, arrowLeft.y)
                ctx.lineTo(arrowRight.x, arrowRight.y)
                ctx.closePath()
                ctx.fillStyle = color
                ctx.fill()
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()

                var incomingTarget = root.centerAnchorPoint("incoming")
                for (var index = 0; index < root.focusedIncomingRelations.length; ++index) {
                    drawCurve(
                                ctx,
                                root.relationAnchorPoint("incoming", index, root.focusedIncomingRelations.length),
                                incomingTarget,
                                root.tokens.signalRaspberry,
                                "incoming")
                }

                var outgoingStart = root.centerAnchorPoint("outgoing")
                for (var outIndex = 0; outIndex < root.focusedOutgoingRelations.length; ++outIndex) {
                    drawCurve(
                                ctx,
                                outgoingStart,
                                root.relationAnchorPoint("outgoing", outIndex, root.focusedOutgoingRelations.length),
                                root.tokens.signalCobalt,
                                "outgoing")
                }
            }

            Connections {
                target: root

                function onSelectedNodeChanged() { relationshipCanvas.requestPaint() }
                function onSceneChanged() { relationshipCanvas.requestPaint() }
                function onWidthChanged() { relationshipCanvas.requestPaint() }
                function onHeightChanged() { relationshipCanvas.requestPaint() }
            }
        }

        Rectangle {
            id: focusFrame
            readonly property var metrics: root.focusOverlayMetrics()
            anchors.horizontalCenter: parent.horizontalCenter
            y: metrics.y
            width: metrics.width
            height: metrics.height
            radius: root.tokens.radiusXxl
            color: root.componentMode ? Qt.rgba(1, 1, 1, 0.54) : Qt.rgba(1, 1, 1, 0.72)
            border.color: Qt.rgba(0.12, 0.18, 0.28, 0.07)

            Rectangle {
                readonly property var rect: root.focusCenterRect()
                x: rect.x - focusFrame.metrics.x
                y: rect.y - focusFrame.metrics.y
                width: rect.width
                height: rect.height
                radius: root.tokens.radius8
                color: root.tokens.base0
                border.color: root.tokens.signalCobalt
                border.width: 2

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 10

                    Rectangle {
                        radius: height / 2
                        color: root.componentMode ? Qt.rgba(0.83, 0.34, 0.92, 0.1) : Qt.rgba(0.14, 0.48, 1, 0.1)
                        border.color: root.componentMode ? Qt.rgba(0.83, 0.34, 0.92, 0.26) : Qt.rgba(0.14, 0.48, 1, 0.24)
                        implicitWidth: modeLabel.implicitWidth + 18
                        implicitHeight: 26

                        Label {
                            id: modeLabel
                            anchors.centerIn: parent
                            text: root.componentMode ? "组件关系聚焦" : "架构关系聚焦"
                            color: root.componentMode ? root.tokens.signalRaspberry : root.tokens.signalCobalt
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.selectedNode ? root.selectedNode.name || "未命名节点" : ""
                        color: root.tokens.text1
                        elide: Text.ElideRight
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 22
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.selectedNode ? (root.selectedNode.summary || root.selectedNode.responsibility || root.selectedNode.role || "当前焦点节点") : ""
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        color: root.tokens.text3
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 13
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            radius: height / 2
                            color: Qt.rgba(0.17, 0.46, 0.94, 0.1)
                            border.color: Qt.rgba(0.17, 0.46, 0.94, 0.22)
                            implicitWidth: label1.implicitWidth + 18
                            implicitHeight: 28

                            Label {
                                id: label1
                                anchors.centerIn: parent
                                text: "输入 " + root.focusedIncomingRelations.length
                                color: root.tokens.signalRaspberry
                                font.family: root.tokens.textFontFamily
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }
                        }

                        Rectangle {
                            radius: height / 2
                            color: Qt.rgba(0.09, 0.56, 0.92, 0.1)
                            border.color: Qt.rgba(0.09, 0.56, 0.92, 0.22)
                            implicitWidth: label2.implicitWidth + 18
                            implicitHeight: 28

                            Label {
                                id: label2
                                anchors.centerIn: parent
                                text: "输出 " + root.focusedOutgoingRelations.length
                                color: root.tokens.signalCobalt
                                font.family: root.tokens.textFontFamily
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.componentMode ? "背景保留全部组件全景图，左右节点负责关系切换。" : "点击左右节点切换焦点；再点一次中间卡片可进入组件探测实验室。"
                        horizontalAlignment: Text.AlignRight
                        color: root.tokens.text3
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: !root.componentMode && !!root.selectedNode && root.selectedNode.id !== undefined
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (root.selectedNode && root.selectedNode.id !== undefined)
                            root.nodeDrilled(root.selectedNode)
                    }
                }
            }

            Repeater {
                model: root.focusedIncomingRelations

                Rectangle {
                    readonly property var rect: root.focusRelationRect("incoming", index, root.focusedIncomingRelations.length)
                    readonly property var relationNode: modelData.node
                    x: rect.x - focusFrame.metrics.x
                    y: rect.y - focusFrame.metrics.y
                    width: rect.width
                    height: rect.height
                    radius: root.tokens.radius8
                    color: root.tokens.base0
                    border.color: Qt.rgba(0.83, 0.34, 0.92, 0.34)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 5

                        Label {
                            Layout.fillWidth: true
                            text: relationNode.name || "未命名节点"
                            elide: Text.ElideRight
                            color: root.tokens.text1
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: relationNode.summary || relationNode.responsibility || relationNode.role || "上游关系节点"
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            wrapMode: Text.WordWrap
                            color: root.tokens.text3
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 11
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.nodeSelected(modelData.node)
                    }
                }
            }

            Repeater {
                model: root.focusedOutgoingRelations

                Rectangle {
                    readonly property var rect: root.focusRelationRect("outgoing", index, root.focusedOutgoingRelations.length)
                    readonly property var relationNode: modelData.node
                    x: rect.x - focusFrame.metrics.x
                    y: rect.y - focusFrame.metrics.y
                    width: rect.width
                    height: rect.height
                    radius: root.tokens.radius8
                    color: root.tokens.base0
                    border.color: Qt.rgba(0.14, 0.48, 1, 0.28)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 5

                        Label {
                            Layout.fillWidth: true
                            text: relationNode.name || "未命名节点"
                            elide: Text.ElideRight
                            color: root.tokens.text1
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: relationNode.summary || relationNode.responsibility || relationNode.role || "下游关系节点"
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            wrapMode: Text.WordWrap
                            color: root.tokens.text3
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 11
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.nodeSelected(modelData.node)
                    }
                }
            }

        }
    }

    Rectangle {
        visible: root.componentOverviewMode && root.nodes.length > 0 && !root.relationshipFocusActive
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 18
        width: 332
        height: 102
        radius: root.tokens.radiusXl
        color: Qt.rgba(1, 1, 1, 0.9)
        border.color: Qt.rgba(0.12, 0.18, 0.28, 0.08)
        z: 24

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: "组件全景图"
                    color: root.tokens.text1
                    font.family: root.tokens.displayFontFamily
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }

                Rectangle {
                    radius: height / 2
                    color: Qt.rgba(0.14, 0.48, 1, 0.08)
                    border.color: Qt.rgba(0.14, 0.48, 1, 0.18)
                    implicitWidth: idleComponentCount.implicitWidth + 16
                    implicitHeight: 24

                    Label {
                        id: idleComponentCount
                        anchors.centerIn: parent
                        text: root.componentCount + " 个"
                        color: root.tokens.signalCobalt
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }
                }

                Item { Layout.fillWidth: true }
            }

            Label {
                Layout.fillWidth: true
                text: "当前是无连线组件全景图，并按入口、界面、核心、支撑分区展示；先扫全貌，再点组件进入关系聚焦。"
                color: root.tokens.text3
                font.family: root.tokens.textFontFamily
                font.pixelSize: 11
                wrapMode: Text.WordWrap
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

            ActionButton {
                tokens: root.tokens
                text: "-"
                compact: true
                square: true
                fixedWidth: 28
                tone: "secondary"
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

            ActionButton {
                tokens: root.tokens
                text: "+"
                compact: true
                square: true
                fixedWidth: 28
                tone: "secondary"
                onClicked: root.zoomAt(root.width / 2, root.height / 2, 1.16)
            }

            ActionButton {
                tokens: root.tokens
                text: "Fit"
                compact: true
                fixedWidth: 44
                tone: "secondary"
                onClicked: root.resetView()
            }

            ActionButton {
                tokens: root.tokens
                visible: root.componentMode
                text: root.showAllEdges ? "隐线" : "显线"
                compact: true
                fixedWidth: 48
                tone: "secondary"
                onClicked: root.showAllEdges = !root.showAllEdges
            }
        }
    }

}
