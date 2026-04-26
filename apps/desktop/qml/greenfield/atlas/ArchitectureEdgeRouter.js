.pragma library
.import "ArchitectureEdgeUtils.js" as EdgeUtils

function makeOrthogonalRoute(p0, p1, p2, p3) {
    return {
        "p0": p0,
        "p1": p1,
        "p2": p2,
        "p3": p3,
        "style": "orthogonal"
    }
}

function makePolylineRoute(points, style) {
    var cleaned = []
    for (var index = 0; index < points.length; ++index) {
        var point = points[index]
        if (!point)
            continue
        if (cleaned.length > 0) {
            var previous = cleaned[cleaned.length - 1]
            if (Math.abs(previous.x - point.x) < 0.5 && Math.abs(previous.y - point.y) < 0.5)
                continue
        }
        cleaned.push(point)
    }

    while (cleaned.length < 2)
        cleaned.push(Qt.point(0, 0))

    return {
        "p0": cleaned[0],
        "p1": cleaned.length > 2 ? cleaned[1] : cleaned[0],
        "p2": cleaned.length > 2 ? cleaned[cleaned.length - 2] : cleaned[1],
        "p3": cleaned[cleaned.length - 1],
        "points": cleaned,
        "style": style || "orthogonal"
    }
}

function routeStats(route, edge, routeObjectHitsFn, routeObjectStatsFn) {
    if (routeObjectStatsFn) {
        var stats = routeObjectStatsFn(route, edge)
        if (stats)
            return stats
    }

    return {
        "hitCount": routeObjectHitsFn && routeObjectHitsFn(route, edge) ? 1 : 0,
        "hitLength": 0
    }
}

function chooseBestCandidate(candidates, edge, routeObjectHitsFn, routeObjectStatsFn) {
    candidates.sort(function(left, right) {
        var leftStats = routeStats(left.route, edge, routeObjectHitsFn, routeObjectStatsFn)
        var rightStats = routeStats(right.route, edge, routeObjectHitsFn, routeObjectStatsFn)
        if (leftStats.hitCount !== rightStats.hitCount)
            return leftStats.hitCount - rightStats.hitCount
        if (Math.abs(leftStats.hitLength - rightStats.hitLength) > 0.5)
            return leftStats.hitLength - rightStats.hitLength
        if (left.preferred !== right.preferred)
            return left.preferred - right.preferred
        return EdgeUtils.routeMetric(left.route) - EdgeUtils.routeMetric(right.route)
    })

    return candidates.length > 0 ? candidates[0].route : null
}

function hoveredOverviewPrimaryRoute(config) {
    var edge = config.edge
    if (config.componentMode || !config.hoverNode || config.hoverNode.id === undefined || !config.isOverviewPrimaryEdge(edge))
        return null

    var relation = config.overviewHoverRelation(edge)
    if (!relation)
        return null

    var focusRect = config.nodeRectById(config.hoverNode.id)
    var otherRect = config.nodeRectById(relation === "outgoing" ? edge.toId : edge.fromId)
    if (!focusRect.valid || !otherRect.valid)
        return null

    var placement = config.overviewHoverRailPlacement(edge)
    if (!placement.valid)
        return null

    var fromLayer = config.overviewMindMapLayout.layerById ? config.overviewMindMapLayout.layerById[edge.fromId] : undefined
    var toLayer = config.overviewMindMapLayout.layerById ? config.overviewMindMapLayout.layerById[edge.toId] : undefined
    var verticalRelation = fromLayer !== undefined && toLayer !== undefined && fromLayer !== toLayer
    var side = relation === "outgoing"
               ? (verticalRelation ? (toLayer > fromLayer ? "bottom" : "top") : "right")
               : (verticalRelation ? (fromLayer < toLayer ? "top" : "bottom") : "left")
    var direction = side === "bottom" || side === "right" ? 1 : -1
    var railSpacing = 26
    var portSpacing = 12
    var centerOffset = placement.count > 1 ? (placement.index - (placement.count - 1) / 2) : 0
    var otherCenter = EdgeUtils.nodeCenter(otherRect)
    var focusPortOffset = centerOffset * portSpacing
    var peerPortOffset = centerOffset * (portSpacing * 0.7)
    var focusPort = EdgeUtils.portPoint(focusRect, side, focusPortOffset)

    if (verticalRelation) {
        var railBaseY = (side === "top" ? focusRect.y : focusRect.y + focusRect.height) + direction * 52
        var railY = railBaseY + direction * centerOffset * railSpacing
        var peerSide = railY <= otherCenter.y ? "top" : "bottom"
        var peerPort = EdgeUtils.portPoint(otherRect, peerSide, peerPortOffset)

        if (relation === "outgoing") {
            return makeOrthogonalRoute(
                        focusPort,
                        Qt.point(focusPort.x, railY),
                        Qt.point(peerPort.x, railY),
                        peerPort)
        }

        return makeOrthogonalRoute(
                    peerPort,
                    Qt.point(peerPort.x, railY),
                    Qt.point(focusPort.x, railY),
                    focusPort)
    }

    var railBaseX = (side === "left" ? focusRect.x : focusRect.x + focusRect.width) + direction * 52
    var railX = railBaseX + direction * centerOffset * railSpacing
    var peerHorizontalSide = railX <= otherCenter.x ? "left" : "right"
    var horizontalPeerPort = EdgeUtils.portPoint(otherRect, peerHorizontalSide, peerPortOffset)

    if (relation === "outgoing") {
        return makeOrthogonalRoute(
                    focusPort,
                    Qt.point(railX, focusPort.y),
                    Qt.point(railX, horizontalPeerPort.y),
                    horizontalPeerPort)
    }

    return makeOrthogonalRoute(
                horizontalPeerPort,
                Qt.point(railX, horizontalPeerPort.y),
                Qt.point(railX, focusPort.y),
                focusPort)
}

function focusedComponentRoute(config) {
    var edge = config.edge
    var focus = config.focusNode
    if (!config.componentMode || !focus || !config.edgeTouchesFocus(edge))
        return null

    var focusIsSource = edge.fromId === focus.id
    var focusRect = config.nodeRectById(focus.id)
    var otherRect = config.nodeRectById(focusIsSource ? edge.toId : edge.fromId)
    if (!focusRect.valid || !otherRect.valid)
        return null

    var placement = config.focusRailPlacement(edge)
    if (!placement.valid)
        return null

    var side = placement.groupKey === "outgoing" ? "left" : "right"
    var direction = side === "left" ? -1 : 1
    var railSpacing = 22
    var portSpacing = 11
    var centerOffset = placement.count > 1 ? (placement.index - (placement.count - 1) / 2) : 0
    var otherCenter = EdgeUtils.nodeCenter(otherRect)
    var focusPortOffset = centerOffset * portSpacing
    var peerPortOffset = centerOffset * (portSpacing * 0.75)
    var railBaseX = (side === "left" ? focusRect.x : focusRect.x + focusRect.width) + direction * 40
    var railX = railBaseX + direction * centerOffset * railSpacing
    var peerSide = railX <= otherCenter.x ? "left" : "right"

    var focusPort = EdgeUtils.portPoint(focusRect, side, focusPortOffset)
    var peerPort = EdgeUtils.portPoint(otherRect, peerSide, peerPortOffset)

    if (focusIsSource) {
        return makeOrthogonalRoute(
                    focusPort,
                    Qt.point(railX, focusPort.y),
                    Qt.point(railX, peerPort.y),
                    peerPort)
    }

    return makeOrthogonalRoute(
                peerPort,
                Qt.point(railX, peerPort.y),
                Qt.point(railX, focusPort.y),
                focusPort)
}

function routeFromScenePoints(config) {
    var edge = config.edge
    if (!config.componentMode)
        return null

    if (!edge || !edge.routePoints || edge.routePoints.length < 2)
        return null

    var focusedRoute = focusedComponentRoute({
        "edge": edge,
        "componentMode": config.componentMode,
        "focusNode": config.focusNode,
        "edgeTouchesFocus": config.edgeTouchesFocus,
        "nodeRectById": config.nodeRectById,
        "focusRailPlacement": config.focusRailPlacement
    })
    if (focusedRoute)
        return focusedRoute

    var points = edge.routePoints
    var startPoint = Qt.point(points[0].x, points[0].y)
    var sourceOffset = config.endpointPeerOffset(edge, "fromId", config.componentMode ? 10 : 12)
    var targetOffset = config.endpointPeerOffset(edge, "toId", config.componentMode ? 8 : 10)

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

        return {
            "p0": startPoint,
            "p1": p1,
            "p2": p2,
            "p3": p3,
            "style": config.componentMode ? "curved" : "orthogonal"
        }
    }

    var endPoint = Qt.point(points[points.length - 1].x, points[points.length - 1].y)
    var laneOffset = config.componentMode ? EdgeUtils.edgeLane(config.laneIndex) * 0.18 : 0
    sourceOffset += laneOffset
    targetOffset -= laneOffset * 0.6
    var horizontal = Math.abs(endPoint.x - startPoint.x) >= Math.abs(endPoint.y - startPoint.y)

    if (horizontal) {
        var bundleBaseX = startPoint.x + (endPoint.x - startPoint.x) * 0.34
        var bundleX = bundleBaseX + (sourceOffset - targetOffset) * 0.9
        var minStubX = Math.min(startPoint.x, endPoint.x) + 28
        var maxStubX = Math.max(startPoint.x, endPoint.x) - 28
        bundleX = Math.max(minStubX, Math.min(maxStubX, bundleX))

        return {
            "p0": Qt.point(startPoint.x, startPoint.y + sourceOffset),
            "p1": Qt.point(bundleX, startPoint.y + sourceOffset),
            "p2": Qt.point(bundleX, endPoint.y + targetOffset),
            "p3": Qt.point(endPoint.x, endPoint.y + targetOffset),
            "style": config.componentMode ? "curved" : "orthogonal"
        }
    }

    var bundleBaseY = startPoint.y + (endPoint.y - startPoint.y) * 0.5
    var bundleY = bundleBaseY + (sourceOffset - targetOffset) * 0.8
    var minStubY = Math.min(startPoint.y, endPoint.y) + 26
    var maxStubY = Math.max(startPoint.y, endPoint.y) - 26
    bundleY = Math.max(minStubY, Math.min(maxStubY, bundleY))

    return {
        "p0": Qt.point(startPoint.x + sourceOffset, startPoint.y),
        "p1": Qt.point(startPoint.x + sourceOffset, bundleY),
        "p2": Qt.point(endPoint.x + targetOffset, bundleY),
        "p3": Qt.point(endPoint.x + targetOffset, endPoint.y),
        "style": config.componentMode ? "curved" : "orthogonal"
    }
}

function layoutNumber(layout, mapName, nodeId, fallbackValue) {
    var map = layout && layout[mapName] ? layout[mapName] : null
    var value = map ? Number(map[nodeId]) : NaN
    return isFinite(value) ? value : fallbackValue
}

function layoutPlacement(layout, mapName, key) {
    var map = layout && layout[mapName] ? layout[mapName] : null
    return map && map[key] ? map[key] : null
}

function placementRelative(placement) {
    if (!placement || placement.count === undefined)
        return 0
    return placement.index - (placement.count - 1) / 2
}

function compactBundleOffset(relative, count, maxOffset) {
    var numericRelative = Number(relative)
    if (!isFinite(numericRelative) || count <= 1)
        return 0

    var step = count > 8 ? 2.6 : (count > 4 ? 3.2 : 4.0)
    return Math.max(-maxOffset, Math.min(maxOffset, numericRelative * step))
}

function overviewExpressForwardRoute(config, fromRect, toRect, metrics, preferred) {
    var outgoingOffset = metrics.outgoingOffset
    var incomingOffset = metrics.incomingOffset
    var topY = Math.min(fromRect.y, toRect.y)
    var bottomY = Math.max(fromRect.y + fromRect.height, toRect.y + toRect.height)
    var sceneHeight = Number(config.overviewMindMapLayout && config.overviewMindMapLayout.height || 0)
    var sourceAboveTarget = EdgeUtils.nodeCenter(fromRect).y <= EdgeUtils.nodeCenter(toRect).y
    var canUseTop = topY > 104
    var canUseBottom = !sceneHeight || bottomY < sceneHeight - 104
    var useTop = sourceAboveTarget
            ? (canUseTop || !canUseBottom)
            : (!canUseBottom && canUseTop)

    var side = useTop ? "top" : "bottom"
    var sourcePort = EdgeUtils.portPoint(fromRect, side, outgoingOffset)
    var targetPort = EdgeUtils.portPoint(toRect, side, incomingOffset)

    var trackStep = metrics.trackCount > 8 ? 8 : 10
    var routeY = useTop
            ? Math.min(topY - 42, topY - 76 + (metrics.trackRelative || 0) * trackStep)
            : Math.max(bottomY + 42, bottomY + 76 + (metrics.trackRelative || 0) * trackStep)

    return {
        "route": makePolylineRoute([
                                        sourcePort,
                                        Qt.point(sourcePort.x, routeY),
                                        Qt.point(targetPort.x, routeY),
                                        targetPort
                                    ],
                                    "orthogonal"),
        "preferred": preferred
    }
}

function overviewForwardBusCandidate(config, fromRect, toRect, metrics, preferred) {
    var outgoingOffset = metrics.outgoingOffset
    var incomingOffset = metrics.incomingOffset
    var sourcePort = EdgeUtils.portPoint(fromRect, "right", outgoingOffset)
    var targetPort = EdgeUtils.portPoint(toRect, "left", incomingOffset)
    var targetBusBaseX = layoutNumber(config.overviewMindMapLayout, "incomingBusXByNodeId",
                                      config.edge.toId, targetPort.x - 52)
    var gap = targetPort.x - sourcePort.x
    if (gap > 42 && Math.abs(sourcePort.y - targetPort.y) < 10) {
        return {
            "route": makePolylineRoute([sourcePort, targetPort], "orthogonal"),
            "preferred": preferred
        }
    }

    var targetBusOffset = compactBundleOffset(metrics.incomingRelative || 0,
                                             metrics.incomingCount || 1,
                                             12)
    var targetBusX = Math.min(targetPort.x - 30, targetBusBaseX + targetBusOffset)
    if (targetBusX <= sourcePort.x + 28)
        targetBusX = sourcePort.x + Math.max(34, gap * 0.46)

    var points = [
        sourcePort,
        Qt.point(targetBusX, sourcePort.y),
        Qt.point(targetBusX, targetPort.y),
        targetPort
    ]
    return {
        "route": makePolylineRoute(points, "orthogonal"),
        "preferred": preferred
    }
}

function overviewBackflowCandidate(config, fromRect, toRect, metrics, preferred) {
    var outgoingOffset = metrics.outgoingOffset
    var incomingOffset = metrics.incomingOffset
    var topY = Math.min(fromRect.y, toRect.y)
    var bottomY = Math.max(fromRect.y + fromRect.height, toRect.y + toRect.height)
    var sceneHeight = Number(config.overviewMindMapLayout && config.overviewMindMapLayout.height || 0)
    var topClear = topY > 92
    var bottomClear = !sceneHeight || bottomY < sceneHeight - 92
    var useTop = topClear && (!bottomClear || (metrics.trackRelative || 0) <= 0)
    if (!topClear && bottomClear)
        useTop = false
    var trackStep = metrics.trackCount > 6 ? 18 : 22
    var busY = useTop
            ? Math.min(topY - 36, topY - 76 + (metrics.trackRelative || 0) * trackStep)
            : Math.max(bottomY + 36, bottomY + 76 + (metrics.trackRelative || 0) * trackStep)
    var side = useTop ? "top" : "bottom"
    var sourcePort = EdgeUtils.portPoint(fromRect, side, outgoingOffset)
    var targetPort = EdgeUtils.portPoint(toRect, side, incomingOffset)

    return {
        "route": makePolylineRoute([
                                        sourcePort,
                                        Qt.point(sourcePort.x, busY),
                                        Qt.point(targetPort.x, busY),
                                        targetPort
                                    ],
                                    "orthogonal"),
        "preferred": preferred
    }
}

function overviewSameLayerBusCandidate(config, fromRect, toRect, metrics, preferred) {
    var outgoingOffset = metrics.outgoingOffset
    var incomingOffset = metrics.incomingOffset
    var sourcePort = EdgeUtils.portPoint(fromRect, "right", outgoingOffset)
    var targetPort = EdgeUtils.portPoint(toRect, "right", incomingOffset)
    var trackStep = metrics.trackCount > 6 ? 18 : 22
    var busX = Math.max(fromRect.x + fromRect.width, toRect.x + toRect.width)
            + 72 + (metrics.trackRelative || 0) * trackStep

    return {
        "route": makePolylineRoute([
                                        sourcePort,
                                        Qt.point(busX, sourcePort.y),
                                        Qt.point(busX, targetPort.y),
                                        targetPort
                                    ],
                                    "orthogonal"),
        "preferred": preferred
    }
}

function componentEndpointOffset(config, edge, endpointKey) {
    if (config.endpointPeerOffset)
        return config.endpointPeerOffset(edge, endpointKey, 8)
    return 0
}

function overviewMindMapRoute(config) {
    var edge = config.edge
    if (config.componentMode)
        return null

    var hoveredRoute = hoveredOverviewPrimaryRoute({
        "edge": edge,
        "componentMode": config.componentMode,
        "hoverNode": config.hoverNode,
        "isOverviewPrimaryEdge": config.isOverviewPrimaryEdge,
        "overviewHoverRelation": config.overviewHoverRelation,
        "overviewHoverRailPlacement": config.overviewHoverRailPlacement,
        "overviewMindMapLayout": config.overviewMindMapLayout,
        "nodeRectById": config.nodeRectById
    })
    if (hoveredRoute)
        return hoveredRoute

    var fromRect = config.nodeRectById(edge.fromId)
    var toRect = config.nodeRectById(edge.toId)
    if (!fromRect.valid || !toRect.valid)
        return null

    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var outgoingSlot = config.overviewMindMapLayout.outgoingSlotByEdgeId
                       ? config.overviewMindMapLayout.outgoingSlotByEdgeId[edge.id]
                       : null
    var incomingSlot = config.overviewMindMapLayout.incomingSlotByEdgeId
                       ? config.overviewMindMapLayout.incomingSlotByEdgeId[edge.id]
                       : null
    var outgoingCount = config.overviewMindMapLayout.outgoingCountByNodeId
                        ? (config.overviewMindMapLayout.outgoingCountByNodeId[edge.fromId] || 1)
                        : 1
    var incomingCount = config.overviewMindMapLayout.incomingCountByNodeId
                        ? (config.overviewMindMapLayout.incomingCountByNodeId[edge.toId] || 1)
                        : 1
    var outgoingRelative = outgoingSlot ? (outgoingSlot.index - (outgoingCount - 1) / 2) : 0
    var incomingRelative = incomingSlot ? (incomingSlot.index - (incomingCount - 1) / 2) : 0
    var bundleRelative = outgoingSlot && incomingSlot
                       ? (outgoingRelative * 0.55 + incomingRelative * 0.45)
                       : (outgoingSlot ? outgoingRelative : incomingRelative)
    var routeTrackPlacement = layoutPlacement(config.overviewMindMapLayout, "routeTrackByEdgeId", edge.id)
    var trackRelative = placementRelative(routeTrackPlacement)
    var trackCount = routeTrackPlacement && routeTrackPlacement.count !== undefined
            ? routeTrackPlacement.count
            : 1
    var fromLayer = config.overviewMindMapLayout.layerById
                    ? config.overviewMindMapLayout.layerById[edge.fromId]
                    : undefined
    var toLayer = config.overviewMindMapLayout.layerById
                  ? config.overviewMindMapLayout.layerById[edge.toId]
                  : undefined
    var layerDelta = fromLayer !== undefined && toLayer !== undefined
            ? toLayer - fromLayer
            : (toCenter.x >= fromCenter.x ? 1 : -1)
    var portSpacing = Math.max(8, Math.min(12, 8 + (Math.max(outgoingCount, incomingCount) - 1) * 0.7))
    var busMetrics = {
        "fromCenter": fromCenter,
        "toCenter": toCenter,
        "outgoingOffset": outgoingRelative * portSpacing,
        "incomingOffset": incomingRelative * portSpacing,
        "outgoingRelative": outgoingRelative,
        "incomingRelative": incomingRelative,
        "outgoingCount": outgoingCount,
        "incomingCount": incomingCount,
        "bundleRelative": bundleRelative,
        "trackRelative": trackRelative,
        "trackCount": trackCount
    }

    if (layerDelta > 1)
        return overviewExpressForwardRoute(config, fromRect, toRect, busMetrics, 0).route
    if (layerDelta > 0)
        return overviewForwardBusCandidate(config, fromRect, toRect, busMetrics, 0).route
    if (layerDelta < 0)
        return overviewBackflowCandidate(config, fromRect, toRect, busMetrics, 0).route
    return overviewSameLayerBusCandidate(config, fromRect, toRect, busMetrics, 0).route
}

function componentOverviewRoute(config) {
    var edge = config.edge
    if (!config.componentMode || config.relationshipFocusActive)
        return null

    var fromRect = config.nodeRectById(edge.fromId)
    var toRect = config.nodeRectById(edge.toId)
    if (!fromRect.valid || !toRect.valid)
        return null

    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var sourceOffset = componentEndpointOffset(config, edge, "fromId")
    var targetOffset = componentEndpointOffset(config, edge, "toId")
    var horizontalRelation = Math.abs(toCenter.x - fromCenter.x) >= Math.abs(toCenter.y - fromCenter.y) * 0.62

    if (horizontalRelation) {
        var forward = toCenter.x >= fromCenter.x
        var sourceSide = forward ? "right" : "left"
        var targetSide = forward ? "left" : "right"
        var sourcePort = EdgeUtils.portPoint(fromRect, sourceSide, sourceOffset)
        var targetPort = EdgeUtils.portPoint(toRect, targetSide, targetOffset)
        var direction = forward ? 1 : -1
        var gap = Math.abs(targetPort.x - sourcePort.x)

        if (gap > 46 && Math.abs(sourcePort.y - targetPort.y) < 10)
            return makePolylineRoute([sourcePort, targetPort], "orthogonal")

        var railOffset = compactBundleOffset(targetOffset / 8, 5, 10)
        var railX = targetPort.x - direction * (38 + railOffset)
        if (forward && railX <= sourcePort.x + 32)
            railX = sourcePort.x + Math.max(36, (targetPort.x - sourcePort.x) * 0.48)
        else if (!forward && railX >= sourcePort.x - 32)
            railX = sourcePort.x - Math.max(36, (sourcePort.x - targetPort.x) * 0.48)

        return makePolylineRoute([
                                     sourcePort,
                                     Qt.point(railX, sourcePort.y),
                                     Qt.point(railX, targetPort.y),
                                     targetPort
                                 ],
                                 "orthogonal")
    }

    return makePolylineRoute(config.bypassRoute(fromRect, toRect, config.laneIndex, true),
                             "orthogonal")
}

function dragPreviewRoute(config, edge, fromRect, toRect) {
    var fromCenterX = fromRect.x + fromRect.width / 2
    var fromCenterY = fromRect.y + fromRect.height / 2
    var toCenterX = toRect.x + toRect.width / 2
    var toCenterY = toRect.y + toRect.height / 2
    var vertical = Math.abs(toCenterY - fromCenterY) >= Math.abs(toCenterX - fromCenterX) * 0.72
    var direct = config.directRoute(edge, fromRect, toRect, config.laneIndex, vertical)
    return makeOrthogonalRoute(direct[0], direct[1], direct[2], direct[3])
}

function routeEdge(config) {
    var edge = config.edge

    if (config.draggingNodeCard) {
        var draggingFromRect = config.nodeRectById(edge.fromId)
        var draggingToRect = config.nodeRectById(edge.toId)
        if (draggingFromRect.valid && draggingToRect.valid)
            return dragPreviewRoute(config, edge, draggingFromRect, draggingToRect)
    }

    var overviewRoute = overviewMindMapRoute(config)
    if (overviewRoute)
        return overviewRoute

    var componentRoute = componentOverviewRoute(config)
    if (componentRoute)
        return componentRoute

    var mappedRoute = routeFromScenePoints(config)
    if (mappedRoute)
        return mappedRoute

    var fromRect = config.nodeRectById(edge.fromId)
    var toRect = config.nodeRectById(edge.toId)
    if (!fromRect.valid || !toRect.valid) {
        var emptyPoint = Qt.point(0, 0)
        return {"p0": emptyPoint, "p1": emptyPoint, "p2": emptyPoint, "p3": emptyPoint}
    }

    var fromCenterX = fromRect.x + fromRect.width / 2
    var fromCenterY = fromRect.y + fromRect.height / 2
    var toCenterX = toRect.x + toRect.width / 2
    var toCenterY = toRect.y + toRect.height / 2
    var vertical = Math.abs(toCenterY - fromCenterY) >= Math.abs(toCenterX - fromCenterX) * 0.72
    var directPrimary = EdgeUtils.orthogonalRouteObject(
                config.directRoute(edge, fromRect, toRect, config.laneIndex, vertical))
    var directAlternate = EdgeUtils.orthogonalRouteObject(
                config.directRoute(edge, fromRect, toRect, config.laneIndex, !vertical))
    var bypassPrimary = EdgeUtils.orthogonalRouteObject(
                config.bypassRoute(fromRect, toRect, config.laneIndex, vertical))
    var bypassAlternate = EdgeUtils.orthogonalRouteObject(
                config.bypassRoute(fromRect, toRect, config.laneIndex, !vertical))

    return chooseBestCandidate([
                                   { "route": directPrimary, "preferred": 0 },
                                   { "route": directAlternate, "preferred": 1 },
                                   { "route": bypassPrimary, "preferred": 2 },
                                   { "route": bypassAlternate, "preferred": 3 }
                               ],
                               edge,
                               config.routeObjectHitsModules,
                               config.routeObjectHitStats)
}
