import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ArchitectureEdgePainter.js" as EdgePainter
import "../foundation"
import "ArchitectureEdgeRouter.js" as EdgeRouter
import "ArchitectureEdgeUtils.js" as EdgeUtils

Item {
    id: root
    clip: true

    required property QtObject tokens
    property var scene: ({})
    property var selectedNode: null
    property string emptyText: "运行分析后，架构图会出现在这里。"
    property bool componentMode: false
    property var hoverNode: null
    property bool showAllEdges: true

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
    readonly property real componentOverviewSectionGap: 32
    readonly property real componentOverviewHeaderHeight: 34
    readonly property real componentOverviewCardWidth: Math.max(500, Math.min(640,
                                                                               (Math.max(1480, width - 120) - 100 - Math.max(0, componentOverviewGroups.length - 1) * componentOverviewGapX) / Math.max(1, componentOverviewGroups.length)))
    readonly property real componentOverviewCardHeight: 330
    readonly property real sceneWidth: componentOverviewMode
                                       ? Math.max(980, componentOverviewSceneWidth())
                                       : Math.max(980, overviewMindMapLayout.width || 0, nodes.length === 0 ? (bounds.width || 980) : 0)
    readonly property real sceneHeight: componentOverviewMode
                                        ? Math.max(560, componentOverviewSceneHeight())
                                        : Math.max(560, overviewMindMapLayout.height || 0, nodes.length === 0 ? (bounds.height || 560) : 0)
    readonly property real fitScale: Math.min(1.0, Math.max(0.38, Math.min((width - 48) / sceneWidth, (height - 48) / sceneHeight)))
    readonly property real baseScale: fitScale
    readonly property real effectiveScale: baseScale * zoom
    readonly property real contentX: Math.max(18, (width - sceneWidth * effectiveScale) / 2) + panX
    readonly property real contentY: Math.max(18, (height - sceneHeight * effectiveScale) / 2) + panY
    readonly property var visibleEdges: collectVisibleEdges(edges, selectedNode, hoverNode, showAllEdges)
    readonly property var focusRailLayout: buildFocusRailLayout(manualNodePositions,
                                                                activeDraggedNodeId,
                                                                activeDragX,
                                                                activeDragY)
    readonly property var overviewHoverRailLayout: buildOverviewHoverRailLayout(manualNodePositions,
                                                                                activeDraggedNodeId,
                                                                                activeDragX,
                                                                                activeDragY)
    readonly property var endpointPeerOffsets: buildEndpointPeerOffsets(manualNodePositions,
                                                                        activeDraggedNodeId,
                                                                        activeDragX,
                                                                        activeDragY)
    readonly property bool relationshipFocusActive: !!selectedNode && selectedNode.id !== undefined
    readonly property var focusedIncomingRelations: relationshipFocusItems("incoming")
    readonly property var focusedOutgoingRelations: relationshipFocusItems("outgoing")
    readonly property int componentCount: componentMode ? nodes.length : 0
    property real zoom: 1.0
    property real panX: 0
    property real panY: 0
    property var manualNodePositions: ({})
    property bool draggingNodeCard: false
    property string activeDraggedNodeId: ""
    property real activeDragX: 0
    property real activeDragY: 0

    signal nodeSelected(var node)
    signal nodeDrilled(var node)
    signal blankClicked()

    function previewText(rawText, fallbackText) {
        var text = String(rawText || "").trim()
        if (text.length === 0)
            return fallbackText || ""

        var stopMarkers = [
            "\n👉",
            "👉",
            "核心类/函数",
            "核心函数",
            "Core classes/functions",
            "Core classes",
            "Top symbols"
        ]
        for (var i = 0; i < stopMarkers.length; ++i) {
            var markerIndex = text.indexOf(stopMarkers[i])
            if (markerIndex > 0) {
                text = text.slice(0, markerIndex).trim()
                break
            }
        }

        text = text.replace(/\s+/g, " ").trim()
        return text.length > 0 ? text : (fallbackText || "")
    }

    function resetView() {
        zoom = 1.0
        panX = 0
        panY = 0
        manualNodePositions = ({})
        draggingNodeCard = false
        activeDraggedNodeId = ""
        activeDragX = 0
        activeDragY = 0
    }

    function fitView() {
        zoom = Math.max(0.38, Math.min(2.8, fitScale / Math.max(0.001, baseScale)))
        panX = 0
        panY = 0
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

    function activeDragPosition(nodeId) {
        if (nodeId === undefined || nodeId === null || activeDraggedNodeId.length === 0)
            return null
        if (nodePositionKey(nodeId) !== activeDraggedNodeId)
            return null
        return {"x": activeDragX, "y": activeDragY}
    }

    function clampNodePosition(nodeId, x, y) {
        return {
            "x": x,
            "y": y
        }
    }

    function beginNodeDrag(nodeId, x, y) {
        if (nodeId === undefined || nodeId === null)
            return
        var clampedPosition = clampNodePosition(nodeId, x, y)
        activeDraggedNodeId = nodePositionKey(nodeId)
        activeDragX = clampedPosition.x
        activeDragY = clampedPosition.y
        draggingNodeCard = true
    }

    function updateNodeDrag(nodeId, x, y) {
        if (nodeId === undefined || nodeId === null)
            return
        if (nodePositionKey(nodeId) !== activeDraggedNodeId)
            return
        var clampedPosition = clampNodePosition(nodeId, x, y)
        activeDragX = clampedPosition.x
        activeDragY = clampedPosition.y
    }

    function endNodeDrag(commitPosition) {
        if (activeDraggedNodeId.length === 0) {
            draggingNodeCard = false
            return
        }

        var draggedNodeId = activeDraggedNodeId
        var draggedX = activeDragX
        var draggedY = activeDragY

        if (commitPosition)
            setManualNodePosition(draggedNodeId, draggedX, draggedY)

        activeDraggedNodeId = ""
        activeDragX = 0
        activeDragY = 0
        draggingNodeCard = false
    }

    function setManualNodePosition(nodeId, x, y) {
        if (nodeId === undefined || nodeId === null)
            return
        var clampedPosition = clampNodePosition(nodeId, x, y)

        var nextPositions = Object.assign({}, manualNodePositions || ({}))
        nextPositions[nodePositionKey(nodeId)] = {"x": clampedPosition.x, "y": clampedPosition.y}
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
        var nextCenterX = Math.max(18, (width - sceneWidth * nextScale) / 2)
        var nextCenterY = Math.max(18, (height - sceneHeight * nextScale) / 2)

        zoom = nextZoom
        panX = viewX - nextCenterX - oldSceneX * nextScale
        panY = viewY - nextCenterY - oldSceneY * nextScale
    }

    function readableSceneSize(screenPixels, minimumScenePixels, maximumScenePixels) {
        var scenePixels = screenPixels / Math.max(0.001, effectiveScale)
        return Math.round(Math.max(minimumScenePixels, Math.min(maximumScenePixels, scenePixels)))
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
        var outgoingBusXByNodeId = {}
        var incomingBusXByNodeId = {}
        var routeTrackByEdgeId = {}
        var routeTrackGroupKeyByEdgeId = {}
        var routeTrackCountByGroupKey = {}
        var layerById = {}
        var rootId = rootNode.id
        var marginX = 64
        var marginY = 64
        var laneGapY = 104
        var baseLaneGapX = 148
        var minX = 999999
        var maxX = -999999
        var minY = 999999
        var maxY = -999999
        var maxLayerIndex = 0
        var nodeIds = []
        var nodeByIdMap = {}
        var widthById = {}
        var heightById = {}
        var fallbackXById = {}
        var fallbackYById = {}
        var rawLaneById = {}
        var distinctRawLanes = []
        var outgoingById = {}
        var incomingById = {}
        var laneBuckets = []
        var orderById = {}
        var baseOrderById = {}
        var laneWidthByIndex = []
        var laneHeightByIndex = []
        var laneXByIndex = []
        var laneRightByIndex = []
        var corridorLoadByGap = []
        var contentHeight = 0

        function numericOrFallback(value, fallbackValue) {
            var numericValue = Number(value)
            return isFinite(numericValue) ? numericValue : fallbackValue
        }

        function compareNodeIdsByFallback(leftId, rightId) {
            var leftY = fallbackYById[leftId]
            var rightY = fallbackYById[rightId]
            if (Math.abs(leftY - rightY) > 0.5)
                return leftY - rightY

            var leftX = fallbackXById[leftId]
            var rightX = fallbackXById[rightId]
            if (Math.abs(leftX - rightX) > 0.5)
                return leftX - rightX

            var leftNode = nodeByIdMap[leftId] || {}
            var rightNode = nodeByIdMap[rightId] || {}
            var nameCompare = String(leftNode.name || "").localeCompare(String(rightNode.name || ""))
            if (nameCompare !== 0)
                return nameCompare

            return String(leftId).localeCompare(String(rightId))
        }

        function graphDerivedLaneById() {
            var derived = {}
            if (rootId === undefined || !nodeByIdMap[rootId])
                return derived

            var queue = [rootId]
            derived[rootId] = 0
            for (var queueIndex = 0; queueIndex < queue.length; ++queueIndex) {
                var currentId = queue[queueIndex]
                var currentLane = derived[currentId]
                var outgoingEdges = outgoingById[currentId] || []
                var incomingEdges = incomingById[currentId] || []

                for (var outgoingIndex = 0; outgoingIndex < outgoingEdges.length; ++outgoingIndex) {
                    var outgoingEdge = outgoingEdges[outgoingIndex]
                    var nextId = outgoingEdge ? outgoingEdge.toId : undefined
                    if (nextId === undefined || !nodeByIdMap[nextId] || derived[nextId] !== undefined)
                        continue
                    derived[nextId] = currentLane + 1
                    queue.push(nextId)
                }

                for (var incomingIndex = 0; incomingIndex < incomingEdges.length; ++incomingIndex) {
                    var incomingEdge = incomingEdges[incomingIndex]
                    var previousId = incomingEdge ? incomingEdge.fromId : undefined
                    if (previousId === undefined || !nodeByIdMap[previousId] || derived[previousId] !== undefined)
                        continue
                    derived[previousId] = currentLane - 1
                    queue.push(previousId)
                }
            }

            return derived
        }

        function connectedNeighborBarycenter(nodeId, laneIndex, direction) {
            var sum = 0
            var weightSum = 0
            var seenPeers = {}
            var incomingEdges = incomingById[nodeId] || []
            var outgoingEdges = outgoingById[nodeId] || []

            function considerPeer(peerId) {
                if (peerId === undefined || peerId === null || peerId === nodeId)
                    return
                if (seenPeers[peerId])
                    return
                if (layerById[peerId] === undefined || orderById[peerId] === undefined)
                    return

                var peerLane = layerById[peerId]
                if (peerLane === laneIndex)
                    return
                if (direction > 0 && peerLane >= laneIndex)
                    return
                if (direction < 0 && peerLane <= laneIndex)
                    return

                seenPeers[peerId] = true
                var distance = Math.max(1, Math.abs(peerLane - laneIndex))
                var weight = 1 / distance
                sum += orderById[peerId] * weight
                weightSum += weight
            }

            for (var incomingIndex = 0; incomingIndex < incomingEdges.length; ++incomingIndex)
                considerPeer(incomingEdges[incomingIndex].fromId)
            for (var outgoingIndex = 0; outgoingIndex < outgoingEdges.length; ++outgoingIndex)
                considerPeer(outgoingEdges[outgoingIndex].toId)

            if (weightSum === 0 && direction !== 0)
                return connectedNeighborBarycenter(nodeId, laneIndex, 0)
            return weightSum > 0 ? sum / weightSum : null
        }

        function sweepLaneOrder(direction) {
            if (laneBuckets.length < 2)
                return

            if (direction > 0) {
                for (var laneIndex = 1; laneIndex < laneBuckets.length; ++laneIndex) {
                    var forwardLaneIds = laneBuckets[laneIndex]
                    forwardLaneIds.sort(function(leftId, rightId) {
                        var leftBarycenter = connectedNeighborBarycenter(leftId, laneIndex, direction)
                        var rightBarycenter = connectedNeighborBarycenter(rightId, laneIndex, direction)
                        if (leftBarycenter !== null || rightBarycenter !== null) {
                            if (leftBarycenter === null)
                                return 1
                            if (rightBarycenter === null)
                                return -1
                            if (Math.abs(leftBarycenter - rightBarycenter) > 0.01)
                                return leftBarycenter - rightBarycenter
                        }

                        var baseDelta = (baseOrderById[leftId] || 0) - (baseOrderById[rightId] || 0)
                        return baseDelta !== 0 ? baseDelta : compareNodeIdsByFallback(leftId, rightId)
                    })

                    for (var forwardOrder = 0; forwardOrder < forwardLaneIds.length; ++forwardOrder)
                        orderById[forwardLaneIds[forwardOrder]] = forwardOrder
                }
                return
            }

            for (laneIndex = laneBuckets.length - 2; laneIndex >= 0; --laneIndex) {
                var backwardLaneIds = laneBuckets[laneIndex]
                backwardLaneIds.sort(function(leftId, rightId) {
                    var leftBarycenter = connectedNeighborBarycenter(leftId, laneIndex, direction)
                    var rightBarycenter = connectedNeighborBarycenter(rightId, laneIndex, direction)
                    if (leftBarycenter !== null || rightBarycenter !== null) {
                        if (leftBarycenter === null)
                            return 1
                        if (rightBarycenter === null)
                            return -1
                        if (Math.abs(leftBarycenter - rightBarycenter) > 0.01)
                            return leftBarycenter - rightBarycenter
                    }

                    var baseDelta = (baseOrderById[leftId] || 0) - (baseOrderById[rightId] || 0)
                    return baseDelta !== 0 ? baseDelta : compareNodeIdsByFallback(leftId, rightId)
                })

                for (var backwardOrder = 0; backwardOrder < backwardLaneIds.length; ++backwardOrder)
                    orderById[backwardLaneIds[backwardOrder]] = backwardOrder
            }
        }

        for (var nodeIndex = 0; nodeIndex < nodes.length; ++nodeIndex) {
            var node = nodes[nodeIndex]
            if (!node || node.id === undefined)
                continue

            var nodeId = node.id
            var defaultX = 64 + (nodeIndex % 3) * 320
            var defaultY = 90 + Math.floor(nodeIndex / 3) * 170
            var nodeX = numericOrFallback(node.x, defaultX)
            var nodeY = numericOrFallback(node.y, defaultY)
            var rawLane = node && node.layoutLaneIndex !== undefined ? Number(node.layoutLaneIndex) : NaN

            nodeIds.push(nodeId)
            nodeByIdMap[nodeId] = node
            widthById[nodeId] = nodeWidth(node)
            heightById[nodeId] = nodeHeight(node)
            fallbackXById[nodeId] = nodeX
            fallbackYById[nodeId] = nodeY
            rawLaneById[nodeId] = rawLane
            outgoingById[nodeId] = []
            incomingById[nodeId] = []

            if (isFinite(rawLane) && distinctRawLanes.indexOf(rawLane) < 0)
                distinctRawLanes.push(rawLane)
        }

        for (var edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
            var edge = edges[edgeIndex]
            if (!edge || edge.id === undefined)
                continue

            primaryEdgeIds[edge.id] = true
            if (edge.fromId !== undefined && outgoingById[edge.fromId])
                outgoingById[edge.fromId].push(edge)
            if (edge.toId !== undefined && incomingById[edge.toId])
                incomingById[edge.toId].push(edge)
        }

        var derivedLaneById = graphDerivedLaneById()
        var preferDerivedLane = distinctRawLanes.length < 2
        var normalizedRawLanes = []
        for (nodeIndex = 0; nodeIndex < nodeIds.length; ++nodeIndex) {
            nodeId = nodeIds[nodeIndex]
            var resolvedLane = rawLaneById[nodeId]
            if (preferDerivedLane) {
                if (derivedLaneById[nodeId] !== undefined)
                    resolvedLane = derivedLaneById[nodeId]
            } else if (!isFinite(resolvedLane) && derivedLaneById[nodeId] !== undefined) {
                resolvedLane = derivedLaneById[nodeId]
            }

            if (!isFinite(resolvedLane))
                resolvedLane = 0

            rawLaneById[nodeId] = resolvedLane
            if (normalizedRawLanes.indexOf(resolvedLane) < 0)
                normalizedRawLanes.push(resolvedLane)
        }

        normalizedRawLanes.sort(function(left, right) { return left - right })
        if (normalizedRawLanes.length === 0)
            normalizedRawLanes.push(0)

        var normalizedLaneByRaw = {}
        for (var normalizedIndex = 0; normalizedIndex < normalizedRawLanes.length; ++normalizedIndex) {
            normalizedLaneByRaw[String(normalizedRawLanes[normalizedIndex])] = normalizedIndex
            laneBuckets.push([])
        }

        for (nodeIndex = 0; nodeIndex < nodeIds.length; ++nodeIndex) {
            nodeId = nodeIds[nodeIndex]
            var layerIndex = normalizedLaneByRaw[String(rawLaneById[nodeId])]
            if (layerIndex === undefined)
                layerIndex = 0
            layerById[nodeId] = layerIndex
            maxLayerIndex = Math.max(maxLayerIndex, layerIndex)
            laneBuckets[layerIndex].push(nodeId)
        }

        for (var laneIndex = 0; laneIndex < laneBuckets.length; ++laneIndex) {
            laneBuckets[laneIndex].sort(compareNodeIdsByFallback)
            for (var orderIndex = 0; orderIndex < laneBuckets[laneIndex].length; ++orderIndex) {
                nodeId = laneBuckets[laneIndex][orderIndex]
                baseOrderById[nodeId] = orderIndex
                orderById[nodeId] = orderIndex
            }
        }

        for (var sweepIndex = 0; sweepIndex < 4; ++sweepIndex) {
            sweepLaneOrder(1)
            sweepLaneOrder(-1)
        }

        for (laneIndex = 0; laneIndex < laneBuckets.length; ++laneIndex) {
            var laneIds = laneBuckets[laneIndex]
            var laneWidth = 0
            var laneHeight = 0
            for (var laneNodeIndex = 0; laneNodeIndex < laneIds.length; ++laneNodeIndex) {
                nodeId = laneIds[laneNodeIndex]
                laneWidth = Math.max(laneWidth, widthById[nodeId] || 0)
                laneHeight += heightById[nodeId] || 0
                if (laneNodeIndex < laneIds.length - 1)
                    laneHeight += laneGapY
            }

            laneWidthByIndex[laneIndex] = Math.max(540, laneWidth)
            laneHeightByIndex[laneIndex] = laneHeight
            contentHeight = Math.max(contentHeight, laneHeight)
            if (laneIndex < laneBuckets.length - 1)
                corridorLoadByGap[laneIndex] = 0
        }

        for (edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
            edge = edges[edgeIndex]
            if (!edge)
                continue

            var fromLayer = layerById[edge.fromId]
            var toLayer = layerById[edge.toId]
            if (fromLayer === undefined || toLayer === undefined || fromLayer === toLayer)
                continue

            var lowLayer = Math.min(fromLayer, toLayer)
            var highLayer = Math.max(fromLayer, toLayer)
            for (var gapIndex = lowLayer; gapIndex < highLayer; ++gapIndex)
                corridorLoadByGap[gapIndex] = (corridorLoadByGap[gapIndex] || 0) + 1
        }

        var currentX = marginX
        for (laneIndex = 0; laneIndex < laneBuckets.length; ++laneIndex) {
            laneXByIndex[laneIndex] = currentX
            currentX += laneWidthByIndex[laneIndex]
            laneRightByIndex[laneIndex] = currentX
            if (laneIndex < laneBuckets.length - 1) {
                var corridorLoad = corridorLoadByGap[laneIndex] || 0
                var corridorBonus = Math.min(124, corridorLoad * 12)
                currentX += baseLaneGapX + corridorBonus
            }
        }

        for (laneIndex = 0; laneIndex < laneBuckets.length; ++laneIndex) {
            laneIds = laneBuckets[laneIndex]
            var laneTop = marginY + Math.max(0, (contentHeight - laneHeightByIndex[laneIndex]) / 2)
            var laneLeft = laneXByIndex[laneIndex]
            var laneContentWidth = laneWidthByIndex[laneIndex]

            for (laneNodeIndex = 0; laneNodeIndex < laneIds.length; ++laneNodeIndex) {
                nodeId = laneIds[laneNodeIndex]
                var nodeW = widthById[nodeId] || 560
                var nodeH = heightById[nodeId] || 330
                var positionedX = laneLeft + Math.max(0, (laneContentWidth - nodeW) / 2)
                var positionedY = laneTop

                positions[nodeId] = {"x": positionedX, "y": positionedY}
                minX = Math.min(minX, positionedX)
                maxX = Math.max(maxX, positionedX + nodeW)
                minY = Math.min(minY, positionedY)
                maxY = Math.max(maxY, positionedY + nodeH)
                laneTop += nodeH + laneGapY
            }
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

        for (nodeIndex = 0; nodeIndex < nodeIds.length; ++nodeIndex) {
            nodeId = nodeIds[nodeIndex]
            var outgoingEdges = outgoingById[nodeId] || []
            var incomingEdges = incomingById[nodeId] || []

            sortPeerEdges(outgoingEdges, "fromId", "toId")
            outgoingCountByNodeId[nodeId] = outgoingEdges.length
            for (var order = 0; order < outgoingEdges.length; ++order)
                outgoingSlotByEdgeId[outgoingEdges[order].id] = {"index": order, "count": outgoingEdges.length}

            sortPeerEdges(incomingEdges, "toId", "fromId")
            incomingCountByNodeId[nodeId] = incomingEdges.length
            for (order = 0; order < incomingEdges.length; ++order)
                incomingSlotByEdgeId[incomingEdges[order].id] = {"index": order, "count": incomingEdges.length}
        }

        for (nodeIndex = 0; nodeIndex < nodeIds.length; ++nodeIndex) {
            nodeId = nodeIds[nodeIndex]
            var nodePosition = positions[nodeId]
            if (!nodePosition)
                continue

            layerIndex = layerById[nodeId] || 0
            var nodeRight = nodePosition.x + (widthById[nodeId] || 240)
            var nodeLeft = nodePosition.x
            var nextLaneLeft = layerIndex < laneBuckets.length - 1
                    ? laneXByIndex[layerIndex + 1]
                    : nodeRight + 144
            var previousLaneRight = layerIndex > 0
                    ? laneRightByIndex[layerIndex - 1]
                    : nodeLeft - 144

            outgoingBusXByNodeId[nodeId] = Math.min(nodeRight + 72,
                                                    Math.max(nodeRight + 44, nextLaneLeft - 58))
            incomingBusXByNodeId[nodeId] = Math.max(nodeLeft - 72,
                                                    Math.min(nodeLeft - 44, previousLaneRight + 58))
        }

        function routeTrackGroupKey(edge) {
            var fromLayerValue = layerById[edge.fromId]
            var toLayerValue = layerById[edge.toId]
            if (fromLayerValue === undefined || toLayerValue === undefined)
                return ""
            if (toLayerValue > fromLayerValue)
                return "forward:" + fromLayerValue + ":" + toLayerValue
            if (toLayerValue < fromLayerValue)
                return "backflow:" + fromLayerValue + ":" + toLayerValue

            var fromPosition = positions[edge.fromId] || {"y": 0}
            var toPosition = positions[edge.toId] || {"y": 0}
            return "same:" + fromLayerValue + ":" + (toPosition.y >= fromPosition.y ? "down" : "up")
        }

        function routeTrackSortMetrics(edge) {
            var fromPosition = positions[edge.fromId] || {"x": 0, "y": 0}
            var toPosition = positions[edge.toId] || {"x": 0, "y": 0}
            var fromOrder = orderById[edge.fromId] !== undefined ? orderById[edge.fromId] : 0
            var toOrder = orderById[edge.toId] !== undefined ? orderById[edge.toId] : 0
            return {
                "centerOrder": (fromOrder + toOrder) / 2,
                "centerY": (fromPosition.y + toPosition.y) / 2,
                "fromOrder": fromOrder,
                "toOrder": toOrder,
                "fromX": fromPosition.x,
                "toX": toPosition.x
            }
        }

        var routeTrackGroups = {}
        for (edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
            edge = edges[edgeIndex]
            if (!edge || edge.id === undefined)
                continue

            var groupKey = routeTrackGroupKey(edge)
            if (!groupKey.length)
                continue

            if (!routeTrackGroups[groupKey])
                routeTrackGroups[groupKey] = []
            routeTrackGroups[groupKey].push(edge)
        }

        for (var groupKey in routeTrackGroups) {
            var trackMembers = routeTrackGroups[groupKey]
            trackMembers.sort(function(left, right) {
                var leftMetrics = routeTrackSortMetrics(left)
                var rightMetrics = routeTrackSortMetrics(right)
                if (Math.abs(leftMetrics.centerOrder - rightMetrics.centerOrder) > 0.01)
                    return leftMetrics.centerOrder - rightMetrics.centerOrder
                if (Math.abs(leftMetrics.centerY - rightMetrics.centerY) > 0.5)
                    return leftMetrics.centerY - rightMetrics.centerY
                if (leftMetrics.fromOrder !== rightMetrics.fromOrder)
                    return leftMetrics.fromOrder - rightMetrics.fromOrder
                if (leftMetrics.toOrder !== rightMetrics.toOrder)
                    return leftMetrics.toOrder - rightMetrics.toOrder
                if (Math.abs(leftMetrics.fromX - rightMetrics.fromX) > 0.5)
                    return leftMetrics.fromX - rightMetrics.fromX
                if (Math.abs(leftMetrics.toX - rightMetrics.toX) > 0.5)
                    return leftMetrics.toX - rightMetrics.toX
                return String(left.id).localeCompare(String(right.id))
            })

            routeTrackCountByGroupKey[groupKey] = trackMembers.length
            for (var trackIndex = 0; trackIndex < trackMembers.length; ++trackIndex) {
                routeTrackByEdgeId[trackMembers[trackIndex].id] = {
                    "index": trackIndex,
                    "count": trackMembers.length
                }
                routeTrackGroupKeyByEdgeId[trackMembers[trackIndex].id] = groupKey
            }
        }

        return {
            "positions": positions,
            "primaryEdgeIds": primaryEdgeIds,
            "secondaryEdgeIds": secondaryEdgeIds,
            "outgoingSlotByEdgeId": outgoingSlotByEdgeId,
            "incomingSlotByEdgeId": incomingSlotByEdgeId,
            "outgoingCountByNodeId": outgoingCountByNodeId,
            "incomingCountByNodeId": incomingCountByNodeId,
            "outgoingBusXByNodeId": outgoingBusXByNodeId,
            "incomingBusXByNodeId": incomingBusXByNodeId,
            "routeTrackByEdgeId": routeTrackByEdgeId,
            "routeTrackGroupKeyByEdgeId": routeTrackGroupKeyByEdgeId,
            "routeTrackCountByGroupKey": routeTrackCountByGroupKey,
            "layerById": layerById,
            "layerCount": maxLayerIndex + 1,
            "rootId": rootNode.id,
            "leftIds": [],
            "rightIds": [],
            "width": Math.max(980, maxX + marginX),
            "height": Math.max(560, maxY + marginY)
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
        var x = 72
        for (var index = 0; index < groupIndex; ++index)
            x += componentOverviewSectionWidth(index) + componentOverviewSectionGap
        return x
    }

    function componentOverviewGroupColumnCap() {
        if (componentOverviewGroups.length <= 1)
            return 3
        if (componentOverviewGroups.length <= 3)
            return 2
        return 1
    }

    function componentOverviewGroupColumns(group) {
        var count = group && group.nodes ? group.nodes.length : 0
        if (count <= 0)
            return 1

        var preferredRows = componentOverviewGroups.length <= 1 ? 4 : 5
        return Math.max(1, Math.min(componentOverviewGroupColumnCap(),
                                    Math.ceil(count / preferredRows)))
    }

    function componentOverviewGroupRows(group) {
        var count = group && group.nodes ? group.nodes.length : 0
        if (count <= 0)
            return 1
        return Math.ceil(count / componentOverviewGroupColumns(group))
    }

    function componentOverviewSectionWidth(groupIndex) {
        var group = componentOverviewGroups[groupIndex]
        var columns = componentOverviewGroupColumns(group)
        return columns * componentOverviewCardWidth
               + Math.max(0, columns - 1) * componentOverviewGapX
    }

    function componentOverviewMaxRows() {
        var maxRows = 1
        for (var index = 0; index < componentOverviewGroups.length; ++index) {
            maxRows = Math.max(maxRows, componentOverviewGroupRows(componentOverviewGroups[index]))
        }
        return maxRows
    }

    function componentOverviewPlacement(nodeId, fallbackIndex) {
        for (var groupIndex = 0; groupIndex < componentOverviewGroups.length; ++groupIndex) {
            var group = componentOverviewGroups[groupIndex]
            for (var nodeIndex = 0; nodeIndex < group.nodes.length; ++nodeIndex) {
                if (group.nodes[nodeIndex].id !== nodeId)
                    continue

                var columns = componentOverviewGroupColumns(group)
                var row = Math.floor(nodeIndex / columns)
                var column = nodeIndex % columns
                return {
                    "x": componentOverviewSectionLeft(groupIndex)
                         + column * (componentOverviewCardWidth + componentOverviewGapX),
                    "y": 96 + componentOverviewHeaderHeight + 16
                         + row * (componentOverviewCardHeight + componentOverviewGapY),
                    "groupIndex": groupIndex
                }
            }
        }

        var fallbackColumns = componentOverviewGroups.length <= 1 ? 3 : 2
        return {
            "x": componentOverviewSectionLeft(0)
                 + (fallbackIndex % fallbackColumns) * (componentOverviewCardWidth + componentOverviewGapX),
            "y": 96 + componentOverviewHeaderHeight + 16
                 + Math.floor(fallbackIndex / fallbackColumns) * (componentOverviewCardHeight + componentOverviewGapY),
            "groupIndex": -1
        }
    }

    function componentOverviewSceneWidth() {
        var width = 72
        for (var index = 0; index < componentOverviewGroups.length; ++index) {
            width += componentOverviewSectionWidth(index)
            if (index < componentOverviewGroups.length - 1)
                width += componentOverviewSectionGap
        }
        return width + 72
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
        var dragPosition = item ? activeDragPosition(item.id) : null
        if (dragPosition)
            return dragPosition.x
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
        var dragPosition = item ? activeDragPosition(item.id) : null
        if (dragPosition)
            return dragPosition.y
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
        var preferredWidth = item && item.width ? item.width * 2.05 : 560
        return Math.max(540, Math.min(680, preferredWidth))
    }

    function nodeHeight(item) {
        if (componentOverviewMode)
            return componentOverviewCardHeight
        var preferredHeight = item && item.height ? item.height * 2.25 : 330
        return Math.max(320, Math.min(430, preferredHeight))
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

    function normalizedPathText(value) {
        return String(value || "").replace(/\s*\/\s*/g, "/").replace(/\s+/g, " ").trim()
    }

    function fileBaseName(path) {
        var normalized = normalizedPathText(path)
        if (!normalized.length)
            return ""
        var slashIndex = normalized.lastIndexOf("/")
        return slashIndex >= 0 ? normalized.slice(slashIndex + 1) : normalized
    }

    function fileDirName(path) {
        var normalized = normalizedPathText(path)
        if (!normalized.length)
            return ""
        var slashIndex = normalized.lastIndexOf("/")
        return slashIndex > 0 ? normalized.slice(0, slashIndex) : ""
    }

    function componentNodeTitle(item) {
        if (!item)
            return "未命名节点"

        var exampleFiles = item.exampleFiles || []
        if (exampleFiles.length > 0) {
            var baseName = fileBaseName(exampleFiles[0])
            if (baseName.length > 0)
                return baseName
        }

        var raw = normalizedPathText(item.name || "")
        var markerIndex = raw.indexOf("·")
        if (markerIndex > 0)
            raw = raw.slice(0, markerIndex).trim()
        if (raw.indexOf("/") >= 0)
            raw = raw.slice(raw.lastIndexOf("/") + 1).trim()
        return raw.length > 0 ? raw : "未命名节点"
    }

    function componentNodeScopeText(item) {
        if (!item)
            return ""

        var exampleFiles = item.exampleFiles || []
        if (exampleFiles.length > 0) {
            var dirName = fileDirName(exampleFiles[0])
            if (dirName.length > 0)
                return dirName
        }

        var candidates = [
            item.scopeLabel || "",
            item.folderHint || "",
            (item.moduleNames && item.moduleNames.length > 0) ? item.moduleNames[0] : ""
        ]
        for (var index = 0; index < candidates.length; ++index) {
            var candidate = normalizedPathText(candidates[index])
            if (candidate.length > 0)
                return candidate
        }
        return ""
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

    function nodeRectProvider(nodeIndex, margin) {
        return root.nodeRectAt(nodeIndex, margin)
    }

    function edgeTouchesFocus(edge) {
        return EdgeUtils.edgeTouchesFocus(root.focusNode(), edge)
    }

    function overviewHoverRelation(edge) {
        return EdgeUtils.overviewHoverRelation(root.hoverNode, edge)
    }

    function collectVisibleEdges(edgeList, selected, hovered, showAll) {
        return EdgeUtils.collectVisibleEdges({
            "edgeList": edgeList,
            "relationshipFocusActive": root.relationshipFocusActive,
            "componentMode": root.componentMode,
            "hoveredNode": hovered,
            "selectedNode": selected,
            "showAll": showAll,
            "focusEdgeLimit": root.focusEdgeLimit,
            "overviewEdgeLimit": root.overviewEdgeLimit,
            "overviewMindMapLayout": root.overviewMindMapLayout,
            "isOverviewPrimaryEdge": function(edge) { return root.isOverviewPrimaryEdge(edge) }
        })
    }

    function buildEndpointPeerOffsets(_manualNodePositions, _activeDraggedNodeId, _activeDragX, _activeDragY) {
        return EdgeUtils.buildEndpointPeerOffsets({
            "visibleEdges": root.visibleEdges,
            "nodeRectById": function(nodeId) { return root.nodeRectById(nodeId) }
        })
    }

    function edgeLane(laneIndex) {
        return EdgeUtils.edgeLane(laneIndex)
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

    function routeHitsModules(points, edge) {
        return EdgeUtils.routeHitsNodes(points, edge, root.nodes, 12, root.nodeRectProvider)
    }

    function routeObjectHitsModules(route, edge) {
        return EdgeUtils.routeObjectHitsNodes(route, edge, root.nodes, 12, root.nodeRectProvider)
    }

    function routeObjectHitStats(route, edge) {
        return EdgeUtils.routeObjectHitStats(route, edge, root.nodes, 26, root.nodeRectProvider)
    }

    function routeObjectAnyNodeHitStats(route, edge) {
        return EdgeUtils.routeObjectAnyNodeHitStats(route, edge, root.nodes, 6, root.nodeRectProvider)
    }

    function routeMetric(route) {
        return EdgeUtils.routeMetric(route)
    }

    function orthogonalRouteObject(points) {
        return EdgeUtils.orthogonalRouteObject(points)
    }

    function endpointPeerOffset(edge, endpointKey, step) {
        return EdgeUtils.endpointPeerOffset(root.endpointPeerOffsets, edge, endpointKey, step)
    }

    function nodeCenter(rect) {
        return EdgeUtils.nodeCenter(rect)
    }

    function buildFocusRailLayout(_manualNodePositions, _activeDraggedNodeId, _activeDragX, _activeDragY) {
        return EdgeUtils.buildFocusRailLayout({
            "componentMode": root.componentMode,
            "focusNode": root.focusNode(),
            "visibleEdges": root.visibleEdges,
            "nodeRectById": function(nodeId) { return root.nodeRectById(nodeId) }
        })
    }

    function buildOverviewHoverRailLayout(_manualNodePositions, _activeDraggedNodeId, _activeDragX, _activeDragY) {
        return EdgeUtils.buildOverviewHoverRailLayout({
            "componentMode": root.componentMode,
            "hoverNode": root.hoverNode,
            "edges": root.edges,
            "nodeRectById": function(nodeId) { return root.nodeRectById(nodeId) },
            "isOverviewPrimaryEdge": function(candidate) { return root.isOverviewPrimaryEdge(candidate) },
            "overviewHoverRelation": function(candidate) { return root.overviewHoverRelation(candidate) }
        })
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

    function focusRailPlacement(edge) {
        return EdgeUtils.railPlacement(root.focusRailLayout, edge, focusRailGroupKey(edge))
    }

    function overviewHoverRailPlacement(edge) {
        return EdgeUtils.railPlacement(root.overviewHoverRailLayout, edge, overviewHoverRelation(edge))
    }

    function hoveredOverviewPrimaryRoute(edge) {
        return EdgeRouter.hoveredOverviewPrimaryRoute({
            "edge": edge,
            "componentMode": root.componentMode,
            "hoverNode": root.hoverNode,
            "isOverviewPrimaryEdge": function(candidate) { return root.isOverviewPrimaryEdge(candidate) },
            "overviewHoverRelation": function(candidate) { return root.overviewHoverRelation(candidate) },
            "overviewHoverRailPlacement": function(candidate) { return root.overviewHoverRailPlacement(candidate) },
            "overviewMindMapLayout": root.overviewMindMapLayout,
            "nodeRectById": function(nodeId) { return root.nodeRectById(nodeId) }
        })
    }

    function focusedComponentRoute(edge) {
        return EdgeRouter.focusedComponentRoute({
            "edge": edge,
            "componentMode": root.componentMode,
            "focusNode": root.focusNode(),
            "edgeTouchesFocus": function(candidate) { return root.edgeTouchesFocus(candidate) },
            "nodeRectById": function(nodeId) { return root.nodeRectById(nodeId) },
            "focusRailPlacement": function(candidate) { return root.focusRailPlacement(candidate) }
        })
    }

    function routeFromScenePoints(edge, laneIndex) {
        return EdgeRouter.routeFromScenePoints({
            "edge": edge,
            "laneIndex": laneIndex,
            "componentMode": root.componentMode,
            "focusNode": root.focusNode(),
            "edgeTouchesFocus": function(candidate) { return root.edgeTouchesFocus(candidate) },
            "nodeRectById": function(nodeId) { return root.nodeRectById(nodeId) },
            "focusRailPlacement": function(candidate) { return root.focusRailPlacement(candidate) },
            "endpointPeerOffset": function(candidate, endpointKey, step) {
                return root.endpointPeerOffset(candidate, endpointKey, step)
            }
        })
    }

    function verticalLane(fromRect, toRect, laneIndex) {
        return EdgeUtils.verticalLane({
            "fromRect": fromRect,
            "toRect": toRect,
            "laneIndex": laneIndex,
            "nodes": root.nodes,
            "nodeRectProvider": root.nodeRectProvider,
            "sceneWidth": root.sceneWidth
        })
    }

    function horizontalLane(fromRect, toRect, laneIndex) {
        return EdgeUtils.horizontalLane({
            "fromRect": fromRect,
            "toRect": toRect,
            "laneIndex": laneIndex,
            "nodes": root.nodes,
            "nodeRectProvider": root.nodeRectProvider,
            "sceneHeight": root.sceneHeight
        })
    }

    function directRoute(edge, fromRect, toRect, laneIndex, vertical) {
        return EdgeUtils.directRoute({
            "edge": edge,
            "fromRect": fromRect,
            "toRect": toRect,
            "laneIndex": laneIndex,
            "vertical": vertical
        })
    }

    function bypassRoute(fromRect, toRect, laneIndex, vertical) {
        return EdgeUtils.bypassRoute({
            "fromRect": fromRect,
            "toRect": toRect,
            "laneIndex": laneIndex,
            "vertical": vertical,
            "nodes": root.nodes,
            "nodeRectProvider": root.nodeRectProvider,
            "sceneWidth": root.sceneWidth,
            "sceneHeight": root.sceneHeight
        })
    }

    function overviewMindMapRoute(edge, laneIndex) {
        return EdgeRouter.overviewMindMapRoute({
            "edge": edge,
            "laneIndex": laneIndex,
            "componentMode": root.componentMode,
            "hoverNode": root.hoverNode,
            "isOverviewPrimaryEdge": function(candidate) { return root.isOverviewPrimaryEdge(candidate) },
            "overviewHoverRelation": function(candidate) { return root.overviewHoverRelation(candidate) },
            "overviewHoverRailPlacement": function(candidate) { return root.overviewHoverRailPlacement(candidate) },
            "overviewMindMapLayout": root.overviewMindMapLayout,
            "nodeRectById": function(nodeId) { return root.nodeRectById(nodeId) },
            "bypassRoute": function(fromRect, toRect, localLaneIndex, vertical) {
                return root.bypassRoute(fromRect, toRect, localLaneIndex, vertical)
            },
            "routeObjectHitsModules": function(route, candidate) { return root.routeObjectHitsModules(route, candidate) },
            "routeObjectHitStats": function(route, candidate) { return root.routeObjectHitStats(route, candidate) }
        })
    }

    function componentOverviewRoute(edge, laneIndex) {
        return EdgeRouter.componentOverviewRoute({
            "edge": edge,
            "laneIndex": laneIndex,
            "componentMode": root.componentMode,
            "relationshipFocusActive": root.relationshipFocusActive,
            "nodeRectById": function(nodeId) { return root.nodeRectById(nodeId) },
            "directRoute": function(candidate, fromRect, toRect, localLaneIndex, vertical) {
                return root.directRoute(candidate, fromRect, toRect, localLaneIndex, vertical)
            },
            "bypassRoute": function(fromRect, toRect, localLaneIndex, vertical) {
                return root.bypassRoute(fromRect, toRect, localLaneIndex, vertical)
            },
            "routeObjectHitsModules": function(route, candidate) { return root.routeObjectHitsModules(route, candidate) },
            "routeObjectHitStats": function(route, candidate) { return root.routeObjectHitStats(route, candidate) }
        })
    }

    function routeEdge(edge, laneIndex) {
        return EdgeRouter.routeEdge({
            "edge": edge,
            "laneIndex": laneIndex,
            "componentMode": root.componentMode,
            "relationshipFocusActive": root.relationshipFocusActive,
            "draggingNodeCard": root.draggingNodeCard,
            "hoverNode": root.hoverNode,
            "focusNode": root.focusNode(),
            "edgeTouchesFocus": function(candidate) { return root.edgeTouchesFocus(candidate) },
            "isOverviewPrimaryEdge": function(candidate) { return root.isOverviewPrimaryEdge(candidate) },
            "overviewHoverRelation": function(candidate) { return root.overviewHoverRelation(candidate) },
            "overviewHoverRailPlacement": function(candidate) { return root.overviewHoverRailPlacement(candidate) },
            "focusRailPlacement": function(candidate) { return root.focusRailPlacement(candidate) },
            "overviewMindMapLayout": root.overviewMindMapLayout,
            "nodeRectById": function(nodeId) { return root.nodeRectById(nodeId) },
            "endpointPeerOffset": function(candidate, endpointKey, step) {
                return root.endpointPeerOffset(candidate, endpointKey, step)
            },
            "directRoute": function(candidate, fromRect, toRect, localLaneIndex, vertical) {
                return root.directRoute(candidate, fromRect, toRect, localLaneIndex, vertical)
            },
            "bypassRoute": function(fromRect, toRect, localLaneIndex, vertical) {
                return root.bypassRoute(fromRect, toRect, localLaneIndex, vertical)
            },
            "routeHitsModules": function(points, candidate) { return root.routeHitsModules(points, candidate) },
            "routeObjectHitsModules": function(route, candidate) { return root.routeObjectHitsModules(route, candidate) },
            "routeObjectHitStats": function(route, candidate) { return root.routeObjectHitStats(route, candidate) }
        })
    }

    function portPoint(rect, side, lane) {
        return EdgeUtils.portPoint(rect, side, lane)
    }

    function arrowWing(tip, tail, size, direction) {
        var angle = Math.atan2(tip.y - tail.y, tip.x - tail.x)
        if (!isFinite(angle))
            angle = -Math.PI / 2

        return Qt.point(tip.x - size * Math.cos(angle + direction * Math.PI / 6),
                        tip.y - size * Math.sin(angle + direction * Math.PI / 6))
    }

    function pointDistance(left, right) {
        return EdgeUtils.pointDistance(left, right)
    }

    function interpolatePoint(fromPoint, toPoint, amount) {
        return EdgeUtils.interpolatePoint(fromPoint, toPoint, amount)
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

    function edgeColor(edge) {
        return EdgeUtils.edgeColor({
            "edge": edge,
            "componentMode": root.componentMode,
            "focusNode": root.focusNode(),
            "hoverNode": root.hoverNode,
            "tokens": root.tokens,
            "isOverviewPrimaryEdge": function(candidate) { return root.isOverviewPrimaryEdge(candidate) }
        })
    }

    function overviewBundleMetrics(edge) {
        if (!edge || root.componentMode)
            return {"relative": 0, "count": 1}

        var outgoingSlot = root.overviewMindMapLayout.outgoingSlotByEdgeId
                           ? root.overviewMindMapLayout.outgoingSlotByEdgeId[edge.id]
                           : null
        var incomingSlot = root.overviewMindMapLayout.incomingSlotByEdgeId
                           ? root.overviewMindMapLayout.incomingSlotByEdgeId[edge.id]
                           : null
        var outgoingCount = outgoingSlot ? outgoingSlot.count : 1
        var incomingCount = incomingSlot ? incomingSlot.count : 1
        var outgoingRelative = outgoingSlot ? outgoingSlot.index - (outgoingCount - 1) / 2 : 0
        var incomingRelative = incomingSlot ? incomingSlot.index - (incomingCount - 1) / 2 : 0

        return {
            "relative": outgoingSlot && incomingSlot
                        ? (outgoingRelative * 0.55 + incomingRelative * 0.45)
                        : (outgoingSlot ? outgoingRelative : incomingRelative),
            "count": Math.max(outgoingCount, incomingCount)
        }
    }

    function buildEdgeRenderEntries() {
        var entries = []
        var focus = root.focusNode()
        var hovered = root.hoverNode
        var strokeScale = Math.max(0.38, root.effectiveScale)

        for (var index = 0; index < root.visibleEdges.length; ++index) {
            var edgeEntry = root.visibleEdges[index]
            var edge = edgeEntry.edge
            if (!edge)
                continue

            var route = root.routeEdge(edge, edgeEntry.laneIndex)
            var emphasized = root.edgeTouchesFocus(edge)
            var overviewPrimary = root.isOverviewPrimaryEdge(edge)
            var overviewRelation = root.overviewHoverRelation(edge)
            var bundleMetrics = root.overviewBundleMetrics(edge)
            var lineColor = root.edgeColor(edge)
            if (!root.componentMode && overviewPrimary)
                lineColor = EdgeUtils.edgeBundleColor(lineColor, bundleMetrics.relative, bundleMetrics.count)
            var visualState = EdgeUtils.edgeVisuals({
                "componentMode": root.componentMode,
                "hasFocus": !!focus,
                "hasHover": !!hovered,
                "emphasized": emphasized,
                "overviewPrimary": overviewPrimary,
                "overviewRelation": overviewRelation,
                "lineColor": lineColor
            })

            entries.push({
                "edge": edge,
                "laneIndex": edgeEntry.laneIndex,
                "route": route,
                "emphasized": emphasized,
                "lineColor": lineColor,
                "haloColor": visualState.haloColor,
                "overviewPrimary": overviewPrimary,
                "overviewRelation": overviewRelation,
                "bundleRelative": bundleMetrics.relative,
                "bundleCount": bundleMetrics.count,
                "visualState": visualState,
                "strokeScale": strokeScale,
                "arrowSize": (root.componentMode ? 10.4 : 9.2) / strokeScale,
                "haloArrowSize": (root.componentMode ? 14.0 : 12.2) / strokeScale
            })
        }

        var sortedEntries = EdgePainter.sortRenderEntries(entries)
        return EdgeUtils.spreadOverlappingRouteMiddles(sortedEntries, {
            "laneStep": root.componentMode ? 14 : 20,
            "maxOffset": root.componentMode ? 38 : 64,
            "laneBucketSize": root.componentMode ? 7 : 8,
            "minSegmentLength": 24,
            "overlapPadding": root.componentMode ? 18 : 30,
            "routeObjectHitStats": function(route, edge) {
                return root.routeObjectHitStats(route, edge)
            },
            "routeObjectAnyNodeHitStats": function(route, edge) {
                return root.routeObjectAnyNodeHitStats(route, edge)
            }
        })
    }

    function edgeCanvasFrame(_manualNodePositions, _activeDraggedNodeId, _activeDragX, _activeDragY) {
        var padding = componentMode ? 340 : 300
        var minX = -padding
        var minY = -padding
        var maxX = sceneWidth + padding
        var maxY = sceneHeight + padding

        for (var index = 0; index < root.nodes.length; ++index) {
            var node = root.nodes[index]
            var x = root.sceneX(node, index)
            var y = root.sceneY(node, index)
            var width = root.nodeWidth(node)
            var height = root.nodeHeight(node)

            minX = Math.min(minX, x - padding)
            minY = Math.min(minY, y - padding)
            maxX = Math.max(maxX, x + width + padding)
            maxY = Math.max(maxY, y + height + padding)
        }

        return {
            "x": minX,
            "y": minY,
            "width": Math.max(1, maxX - minX),
            "height": Math.max(1, maxY - minY)
        }
    }

    Canvas {
        id: atmosphereCanvas
        anchors.fill: parent

        function paintGlow(ctx, cx, cy, radius, color, coreAlpha, edgeAlpha) {
            var gradient = ctx.createRadialGradient(cx, cy, 0, cx, cy, radius)
            gradient.addColorStop(0.0, Qt.rgba(color.r, color.g, color.b, coreAlpha))
            gradient.addColorStop(0.44, Qt.rgba(color.r, color.g, color.b, coreAlpha * 0.42))
            gradient.addColorStop(1.0, Qt.rgba(color.r, color.g, color.b, edgeAlpha))
            ctx.fillStyle = gradient
            ctx.beginPath()
            ctx.arc(cx, cy, radius, 0, Math.PI * 2)
            ctx.fill()
        }

        onPaint: {
            var ctx = getContext("2d")
            var gradient = ctx.createLinearGradient(0, 0, 0, height)

            ctx.reset()
            gradient.addColorStop(0.0, Qt.rgba(1, 1, 1, 0.28))
            gradient.addColorStop(1.0, Qt.rgba(1, 1, 1, 0.08))
            ctx.fillStyle = gradient
            ctx.fillRect(0, 0, width, height)

            paintGlow(ctx, width * 0.16, height * 0.14, Math.max(width, height) * 0.16,
                      root.tokens.signalMoss, 0.22, 0.0)
            paintGlow(ctx, width * 0.84, height * 0.12, Math.max(width, height) * 0.15,
                      root.tokens.signalRaspberry, 0.16, 0.0)
            paintGlow(ctx, width * 0.52, height * 0.72, Math.max(width, height) * 0.24,
                      root.tokens.signalCobalt, 0.1, 0.0)
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
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
                    ctx.arc(x, y, 1.0, 0, Math.PI * 2)
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

        Canvas {
            id: edgeCanvas
            property var frame: root.edgeCanvasFrame(root.manualNodePositions,
                                                    root.activeDraggedNodeId,
                                                    root.activeDragX,
                                                    root.activeDragY)
            x: frame.x
            y: frame.y
            width: frame.width
            height: frame.height
            property bool repaintQueued: false

            function schedulePaint() {
                if (repaintQueued || !available || !visible)
                    return
                repaintQueued = true
                Qt.callLater(function() {
                    repaintQueued = false
                    if (available && visible)
                        edgeCanvas.requestPaint()
                })
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.lineJoin = "round"
                ctx.lineCap = "round"
                ctx.translate(-edgeCanvas.frame.x, -edgeCanvas.frame.y)

                var entries = root.buildEdgeRenderEntries()

                function drawRouteStroke(entry, route) {
                    if (!route)
                        return

                    ctx.globalAlpha = entry.visualState.lineOpacity

                    ctx.beginPath()
                    EdgePainter.drawRoutePath(ctx, route, root.componentMode, entry.emphasized)
                    ctx.strokeStyle = entry.haloColor
                    ctx.lineWidth = entry.visualState.haloStrokeWidth / entry.strokeScale
                    ctx.stroke()

                    ctx.beginPath()
                    EdgePainter.drawRoutePath(ctx, route, root.componentMode, entry.emphasized)
                    ctx.strokeStyle = entry.lineColor
                    ctx.lineWidth = entry.visualState.lineStrokeWidth / entry.strokeScale
                    ctx.stroke()
                }

                function drawRouteArrow(entry, route) {
                    if (!route)
                        return

                    var arrowTip = EdgePainter.routeEndPoint(route)
                    var arrowTail = EdgePainter.routeArrowTail(route)
                    var haloArrowLeft = root.arrowWing(arrowTip, arrowTail, entry.haloArrowSize, -1)
                    var haloArrowRight = root.arrowWing(arrowTip, arrowTail, entry.haloArrowSize, 1)
                    var arrowLeft = root.arrowWing(arrowTip, arrowTail, entry.arrowSize, -1)
                    var arrowRight = root.arrowWing(arrowTip, arrowTail, entry.arrowSize, 1)

                    ctx.globalAlpha = entry.visualState.lineOpacity

                    ctx.beginPath()
                    ctx.moveTo(arrowTip.x, arrowTip.y)
                    ctx.lineTo(haloArrowLeft.x, haloArrowLeft.y)
                    ctx.lineTo(haloArrowRight.x, haloArrowRight.y)
                    ctx.closePath()
                    ctx.fillStyle = entry.haloColor
                    ctx.fill()

                    ctx.beginPath()
                    ctx.moveTo(arrowTip.x, arrowTip.y)
                    ctx.lineTo(arrowLeft.x, arrowLeft.y)
                    ctx.lineTo(arrowRight.x, arrowRight.y)
                    ctx.closePath()
                    ctx.fillStyle = entry.lineColor
                    ctx.fill()
                }

                for (var index = 0; index < entries.length; ++index) {
                    var entry = entries[index]
                    if (!entry.route)
                        continue

                    drawRouteStroke(entry, entry.route)
                    drawRouteArrow(entry, entry.route)
                }

                ctx.globalAlpha = 1.0
            }

            onWidthChanged: schedulePaint()
            onHeightChanged: schedulePaint()
            onXChanged: schedulePaint()
            onYChanged: schedulePaint()
            onAvailableChanged: schedulePaint()
            onVisibleChanged: schedulePaint()
            Component.onCompleted: schedulePaint()

            Connections {
                target: root

                function onVisibleEdgesChanged() { edgeCanvas.schedulePaint() }
                function onHoverNodeChanged() { edgeCanvas.schedulePaint() }
                function onSelectedNodeChanged() { edgeCanvas.schedulePaint() }
                function onDraggingNodeCardChanged() { edgeCanvas.schedulePaint() }
                function onActiveDraggedNodeIdChanged() { edgeCanvas.schedulePaint() }
                function onActiveDragXChanged() { edgeCanvas.schedulePaint() }
                function onActiveDragYChanged() { edgeCanvas.schedulePaint() }
                function onManualNodePositionsChanged() { edgeCanvas.schedulePaint() }
                function onEffectiveScaleChanged() { edgeCanvas.schedulePaint() }
                function onComponentModeChanged() { edgeCanvas.schedulePaint() }
                function onRelationshipFocusActiveChanged() { edgeCanvas.schedulePaint() }
                function onSceneChanged() { edgeCanvas.schedulePaint() }
            }
        }

        Repeater {
            model: root.componentOverviewMode ? root.componentOverviewGroups : []

            Rectangle {
                x: root.componentOverviewSectionLeft(index)
                y: 96
                width: root.componentOverviewSectionWidth(index)
                height: root.componentOverviewHeaderHeight
                radius: root.tokens.radius8
                color: Qt.rgba(1, 1, 1, 0.72)
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

                    Item { Layout.fillWidth: true }

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
                radius: root.tokens.radiusLg + 2
                color: "transparent"
                border.color: selected ? root.tokens.signalCobalt : root.tokens.border1
                border.width: selected ? 2 : 1
                z: hovered || selected ? 10 : 2
                opacity: root.selectedNode && !selected ? 0.58 : 1.0

                Behavior on opacity { NumberAnimation { duration: 140 } }

                Rectangle {
                    anchors.fill: parent
                    radius: card.radius
                    color: root.tokens.base0
                    opacity: card.selected ? 0.99 : (card.hovered ? 0.98 : 0.965)
                }

                Rectangle {
                    anchors.fill: parent
                    radius: card.radius
                    color: hovered || selected ? Qt.rgba(1, 1, 1, 0.96) : Qt.rgba(1, 1, 1, 0.92)

                    gradient: Gradient {
                        GradientStop {
                            position: 0.0
                            color: selected
                                   ? Qt.rgba(1, 1, 1, 0.99)
                                   : (card.hovered ? Qt.rgba(1, 1, 1, 0.98) : Qt.rgba(1, 1, 1, 0.94))
                        }

                        GradientStop {
                            position: 1.0
                            color: selected
                                   ? Qt.rgba(0.94, 0.97, 1.0, 0.98)
                                   : (card.hovered ? Qt.rgba(0.97, 0.98, 1.0, 0.95) : Qt.rgba(1, 1, 1, 0.9))
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -4
                    radius: card.radius + 2
                    color: "transparent"
                    border.color: selected
                                  ? Qt.rgba(root.tokens.signalCobalt.r, root.tokens.signalCobalt.g, root.tokens.signalCobalt.b, 0.18)
                                  : (card.hovered
                                     ? Qt.rgba(root.tokens.signalCobalt.r, root.tokens.signalCobalt.g, root.tokens.signalCobalt.b, 0.08)
                                     : "transparent")
                    border.width: selected ? 4 : (card.hovered ? 2 : 0)
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
                    color: "transparent"

                    gradient: Gradient {
                        GradientStop { position: 0.0; color: root.tokens.signalCoral }
                        GradientStop { position: 1.0; color: Qt.rgba(0.95, 0.27, 0.34, 0.94) }
                    }

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
                    anchors.margins: root.readableSceneSize(15, 20, 38)
                    spacing: root.readableSceneSize(7, 10, 22)

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: root.readableSceneSize(8, 12, 20)
                            Layout.preferredHeight: root.readableSceneSize(8, 12, 20)
                            Layout.alignment: Qt.AlignTop
                            radius: width / 2
                            color: modelData.reachableFromEntry ? root.tokens.signalTeal
                                   : root.componentMode ? root.tokens.signalRaspberry
                                   : root.tokens.signalCobalt
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.componentMode
                                  ? root.componentNodeTitle(modelData)
                                  : (modelData.name || "未命名节点")
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            color: root.tokens.text1
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: root.readableSceneSize(15, 20, 42)
                            font.weight: Font.DemiBold
                            lineHeight: 1.12
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.readableSceneSize(88, 102, 220)
                        text: root.previewText(
                                  modelData.summary || modelData.responsibility || modelData.role,
                                  "暂无描述。")
                        wrapMode: Text.WordWrap
                        maximumLineCount: 4
                        elide: Text.ElideRight
                        color: root.tokens.text3
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: root.readableSceneSize(13, 17, 36)
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
                            visible: root.componentMode && root.componentNodeScopeText(modelData).length > 0
                            radius: height / 2
                            color: Qt.rgba(0.14, 0.48, 1, 0.08)
                            border.color: Qt.rgba(0.14, 0.48, 1, 0.16)
                            implicitWidth: scopeLabel.implicitWidth + 14
                            implicitHeight: 22

                            Label {
                                id: scopeLabel
                                anchors.centerIn: parent
                                text: root.componentNodeScopeText(modelData)
                                color: root.tokens.signalCobalt
                                font.family: root.tokens.textFontFamily
                                font.pixelSize: root.readableSceneSize(10, 14, 28)
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                        }

                        Label {
                            visible: !root.componentMode
                            text: root.cardMetaLabel(modelData)
                            color: root.tokens.text3
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: root.readableSceneSize(10, 14, 28)
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Label {
                            text: root.cardFileCountLabel(modelData)
                            color: root.tokens.text3
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: root.readableSceneSize(10, 14, 28)
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton
                    cursorShape: dragArmed ? Qt.ClosedHandCursor : Qt.PointingHandCursor
                    property bool dragArmed: false
                    property bool dragMoved: false
                    property real pressSceneX: 0
                    property real pressSceneY: 0
                    property real pressCardX: 0
                    property real pressCardY: 0
                    property real dragThreshold: 7
                    onPressed: function(mouse) {
                        dragMoved = false
                        dragArmed = false
                        var scenePoint = card.mapToItem(contentLayer, mouse.x, mouse.y)
                        pressSceneX = scenePoint.x
                        pressSceneY = scenePoint.y
                        pressCardX = card.x
                        pressCardY = card.y
                    }
                    onEntered: {
                        card.hovered = true
                        root.hoverNode = modelData
                    }
                    onExited: {
                        card.hovered = false
                        if (root.hoverNode && root.hoverNode.id === modelData.id)
                            root.hoverNode = null
                    }
                    onPositionChanged: function(mouse) {
                        if (!pressed || root.componentMode || root.relationshipFocusActive)
                            return

                        var scenePoint = card.mapToItem(contentLayer, mouse.x, mouse.y)
                        if (!dragArmed) {
                            if (Math.abs(scenePoint.x - pressSceneX) < dragThreshold &&
                                    Math.abs(scenePoint.y - pressSceneY) < dragThreshold) {
                                return
                            }
                            dragArmed = true
                            dragMoved = true
                            root.beginNodeDrag(modelData.id, pressCardX, pressCardY)
                        }

                        var nextX = pressCardX + (scenePoint.x - pressSceneX)
                        var nextY = pressCardY + (scenePoint.y - pressSceneY)
                        if (Math.abs(nextX - pressCardX) + Math.abs(nextY - pressCardY) > 1)
                            dragMoved = true
                        root.updateNodeDrag(modelData.id, nextX, nextY)
                    }
                    onReleased: {
                        root.endNodeDrag(dragArmed)
                        dragArmed = false
                    }
                    onCanceled: {
                        dragArmed = false
                        root.endNodeDrag(false)
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

        WheelHandler {
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

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: root.blankClicked()
        }

        Rectangle {
            anchors.fill: parent
            color: root.componentMode ? Qt.rgba(0.978, 0.984, 0.994, 0.94) : Qt.rgba(0.972, 0.978, 0.988, 0.84)
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
            color: root.componentMode ? Qt.rgba(1, 1, 1, 0.6) : Qt.rgba(1, 1, 1, 0.76)
            border.color: Qt.rgba(0.12, 0.18, 0.28, 0.07)

            Label {
                x: 24
                y: 18
                width: Math.min(parent.width * 0.48, 420)
                text: root.componentMode ? "背景保留全部组件全景图，左右节点负责关系切换。" : "点击左右节点切换焦点；再点一次中间卡片可进入组件探测实验室。"
                color: root.tokens.text3
                font.family: root.tokens.textFontFamily
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }

            Rectangle {
                readonly property var rect: root.focusCenterRect()
                x: rect.x - focusFrame.metrics.x
                y: rect.y - focusFrame.metrics.y
                width: rect.width
                height: rect.height
                radius: root.tokens.radius8
                color: root.tokens.panelStrong
                border.color: root.tokens.signalCobalt
                border.width: 2

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

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

                        Item { Layout.fillWidth: true }

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
                        text: root.selectedNode
                              ? root.previewText(
                                    root.selectedNode.summary || root.selectedNode.responsibility || root.selectedNode.role,
                                    "当前焦点节点")
                              : ""
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        color: root.tokens.text3
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 13
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
                    color: root.tokens.panelStrong
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
                            text: root.previewText(
                                      relationNode.summary || relationNode.responsibility || relationNode.role,
                                      "上游关系节点")
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
                    color: root.tokens.panelStrong
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
                            text: root.previewText(
                                      relationNode.summary || relationNode.responsibility || relationNode.role,
                                      "下游关系节点")
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
        height: 40
        radius: root.tokens.radiusLg
        color: Qt.rgba(1, 1, 1, 0.74)
        border.color: root.tokens.border1
        z: 30

        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.84) }
            GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.64) }
        }

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
                hint: "缩小架构画布，查看更多上下文。"
                tone: "secondary"
                onClicked: root.zoomAt(root.width / 2, root.height / 2, 0.86)
            }

            Label {
                Layout.fillWidth: true
                text: Math.round(root.effectiveScale * 100) + "%"
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
                hint: "放大架构画布，查看节点和连线细节。"
                tone: "secondary"
                onClicked: root.zoomAt(root.width / 2, root.height / 2, 1.16)
            }

            ActionButton {
                tokens: root.tokens
                text: "Fit"
                compact: true
                fixedWidth: 44
                hint: "自动适配视图，把当前架构图完整放回画布中。"
                tone: "secondary"
                onClicked: root.fitView()
            }

            ActionButton {
                tokens: root.tokens
                visible: root.componentMode
                text: root.showAllEdges ? "隐线" : "显线"
                compact: true
                fixedWidth: 48
                hint: root.showAllEdges ? "隐藏非关键连线，降低画布噪声。" : "显示更多依赖连线，检查组件之间的真实关系。"
                tone: "secondary"
                onClicked: root.showAllEdges = !root.showAllEdges
            }
        }
    }

}
