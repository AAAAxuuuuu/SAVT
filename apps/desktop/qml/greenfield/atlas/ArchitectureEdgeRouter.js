.pragma library
.import "ArchitectureEdgeUtils.js" as EdgeUtils

var READABLE_DEBUG_TAG = "readable-component-detail-v10-spaced-dimmed"
var READABLE_DEBUG_ENABLED = true

function readableDebug(prefix, message) {
    if (!READABLE_DEBUG_ENABLED)
        return
    console.log("[READABLE-" + String(prefix) + "]", READABLE_DEBUG_TAG, String(message))
}

function readableEdgeLabel(edge) {
    if (!edge)
        return "<null-edge>"
    var fromName = String(edge.fromName || edge.fromLabel || edge.fromId || "?")
    var toName = String(edge.toName || edge.toLabel || edge.toId || "?")
    return "id=" + String(edge.id !== undefined ? edge.id : "?") + " " + fromName + " -> " + toName + " bidirectional=" + String(!!edge.bidirectional)
}

function readablePointText(point) {
    if (!point)
        return "<null>"
    return "(" + Math.round(Number(point.x) || 0) + "," + Math.round(Number(point.y) || 0) + ")"
}

function readableRouteText(route) {
    if (!route)
        return "<null-route>"
    var pts = route.points && route.points.length >= 2 ? route.points : [route.p0, route.p1, route.p2, route.p3]
    var text = []
    for (var i = 0; i < pts.length; ++i)
        text.push(readablePointText(pts[i]))
    return "style=" + String(route.style || "") + " suppressArrow=" + String(!!route.suppressArrow) + " bidirectional=" + String(!!route.bidirectional) + " points=" + String(pts.length) + " " + text.join(" -> ")
}

var componentRouteDebugSeen = ({})

function shouldDebugComponentEdge(edge) {
    if (!edge)
        return false

    var fromName = String(edge.fromName || "")
    var toName = String(edge.toName || "")
    var watchTerms = ["Src Ui", "Layout", "Analyzer", "Parser", "Core", "Apps Desktop", "Output"]
    for (var index = 0; index < watchTerms.length; ++index) {
        if (fromName.indexOf(watchTerms[index]) >= 0 || toName.indexOf(watchTerms[index]) >= 0)
            return true
    }
    return false
}

function debugComponentRoute(edge, branch, detail) {
    if (!shouldDebugComponentEdge(edge))
        return

    var key = String(branch) + ":" + String(edge && edge.id !== undefined ? edge.id : "")
    if (componentRouteDebugSeen[key])
        return

    componentRouteDebugSeen[key] = true
    console.log("[component-route]",
        branch,
        "edge=" + String(edge && edge.id !== undefined ? edge.id : ""),
        "from=" + String(edge && edge.fromName ? edge.fromName : edge && edge.fromId),
        "to=" + String(edge && edge.toName ? edge.toName : edge && edge.toId),
        detail || "")
}

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
    for (var index = 0; index < candidates.length; ++index)
        candidates[index].stats = routeStats(candidates[index].route, edge, routeObjectHitsFn, routeObjectStatsFn)

    candidates.sort(function (left, right) {
        var leftStats = left.stats
        var rightStats = right.stats
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

function stemPlacementRelative(placement) {
    if (!placement || placement.index === undefined || placement.count === undefined)
        return 0
    return Number(placement.index) - (Number(placement.count) - 1) / 2
}

function rectSideSpan(rect, side) {
    return side === "left" || side === "right" ? rect.height : rect.width
}

function distributedPortOffset(rect, side, relative, count, minimumInset, coverageRatio) {
    return EdgeUtils.distributedSideOffset(relative,
        count,
        rectSideSpan(rect, side),
        minimumInset === undefined ? 18 : minimumInset,
        coverageRatio === undefined ? 0.68 : coverageRatio)
}

function amplifiedDistributedPortOffset(rect, side, relative, count, minimumInset, coverageRatio, amplify) {
    var inset = minimumInset === undefined ? 18 : Number(minimumInset)
    var coverage = coverageRatio === undefined ? 0.68 : Number(coverageRatio)
    var offset = distributedPortOffset(rect, side, relative, count, inset, coverage)
    var span = rectSideSpan(rect, side)
    var availableSpan = Math.max(18, Math.min(span * coverage, span - inset * 2))
    var halfSpread = Math.max(10, availableSpan / 2)
    var factor = amplify === undefined ? 1 : Number(amplify)
    return Math.max(-halfSpread, Math.min(halfSpread, offset * factor))
}

function overviewStemOffset(rect, side, placement, clusterRelative) {
    var stemRelative = stemPlacementRelative(placement) + clusterRelative * 0.82
    var stemCount = placement && placement.count !== undefined ? Number(placement.count) : 1
    var sideVertical = side === "left" || side === "right"
    var coverageBase = sideVertical ? 0.78 : 0.72
    var coverage = Math.min(0.9, coverageBase + Math.max(0, stemCount - 2) * 0.035)
    var minimumInset = stemCount >= 5 ? 12 : (stemCount >= 3 ? 14 : 18)
    var amplify = stemCount <= 1 ? 1 : Math.min(1.34, 1.14 + Math.max(0, stemCount - 2) * 0.05)
    return amplifiedDistributedPortOffset(rect,
        side,
        stemRelative,
        stemCount,
        minimumInset,
        coverage,
        amplify)
}

function overviewRoleBandBias(rect, side, role) {
    var span = side === "left" || side === "right" ? rect.height : rect.width
    var band = Math.max(18, Math.min(42, span * 0.2))

    if (side === "bottom")
        return role === "incoming" ? band : -band

    if (side === "top")
        return role === "incoming" ? -band : band

    return role === "incoming" ? -band : band
}

function overviewSourcePort(fromRect, fanInBundle, outgoingStemPlacement, clusterRelative) {
    var sourceSide = String(fanInBundle.sourceSide || "right")
    var sourceOffset = overviewRoleBandBias(fromRect, sourceSide, "outgoing")
        + overviewStemOffset(fromRect, sourceSide, outgoingStemPlacement, clusterRelative * 0.45)
    return EdgeUtils.portPoint(fromRect, sourceSide, sourceOffset)
}

function overviewTargetPort(toRect, fanInBundle, incomingStemPlacement, clusterRelative) {
    var targetSide = String(fanInBundle.targetApproachSide || "left")
    var targetOffset = overviewRoleBandBias(toRect, targetSide, "incoming")
        + overviewStemOffset(toRect, targetSide, incomingStemPlacement, clusterRelative)
    return EdgeUtils.portPoint(toRect, targetSide, targetOffset)
}

function overviewBundledRoute(config, edge, fromRect, toRect) {
    var layout = config.overviewMindMapLayout || {}
    var fanInBundle = layoutPlacement(layout, "fanInBundleByEdgeId", edge.id)
    if (!fanInBundle)
        return null

    var outgoingStemPlacement = layoutPlacement(layout, "outgoingStemSlotByEdgeId", edge.id)
    var incomingStemPlacement = layoutPlacement(layout, "incomingStemSlotByEdgeId", edge.id)
    var clusterCount = fanInBundle.clusterCount !== undefined ? Math.max(1, Number(fanInBundle.clusterCount)) : 1
    var clusterIndex = fanInBundle.clusterIndex !== undefined ? Number(fanInBundle.clusterIndex) : 0
    var clusterRelative = clusterCount > 1 ? (clusterIndex - (clusterCount - 1) / 2) : 0
    var bundleLeaderEdgeId = fanInBundle.leaderEdgeId !== undefined ? String(fanInBundle.leaderEdgeId) : ""
    var edgeId = edge && edge.id !== undefined ? String(edge.id) : ""
    var isBundleLeader = !bundleLeaderEdgeId.length || bundleLeaderEdgeId === edgeId

    var sourcePort = overviewSourcePort(fromRect, fanInBundle, outgoingStemPlacement, clusterRelative)
    var targetPort = overviewTargetPort(toRect, fanInBundle, incomingStemPlacement, clusterRelative)
    var points = [sourcePort]
    var branchPoints = [sourcePort]
    var trunkPoints = []
    var joinPoint = null

    if (fanInBundle.orientation === "vertical") {
        var corridorX = Number(fanInBundle.corridorX)
        var targetTrunkY = Number(fanInBundle.targetTrunkY)
        if (!isFinite(corridorX))
            corridorX = (sourcePort.x + targetPort.x) / 2
        if (!isFinite(targetTrunkY))
            targetTrunkY = String(fanInBundle.targetApproachSide || "") === "top"
                ? toRect.y - 58
                : toRect.y + toRect.height + 58

        var sourceSideVertical = String(fanInBundle.sourceSide || "")
        if (sourceSideVertical === "left" || sourceSideVertical === "right") {
            var corridorStartVertical = Qt.point(corridorX, sourcePort.y)
            points.push(corridorStartVertical)
            branchPoints.push(corridorStartVertical)
        } else {
            var sourceStubY = sourcePort.y + (sourceSideVertical === "top" ? -34 : 34)
            var sourceStubVertical = Qt.point(sourcePort.x, sourceStubY)
            var sourceLaneVertical = Qt.point(corridorX, sourceStubY)
            points.push(sourceStubVertical)
            points.push(sourceLaneVertical)
            branchPoints.push(sourceStubVertical)
            branchPoints.push(sourceLaneVertical)
        }

        joinPoint = Qt.point(corridorX, targetTrunkY)
        points.push(joinPoint)
        branchPoints.push(joinPoint)
        trunkPoints.push(joinPoint)
        if (Math.abs(targetPort.x - corridorX) > 0.5) {
            var targetLaneVertical = Qt.point(targetPort.x, targetTrunkY)
            points.push(targetLaneVertical)
            trunkPoints.push(targetLaneVertical)
        }
        points.push(targetPort)
        trunkPoints.push(targetPort)
    } else {
        var corridorY = Number(fanInBundle.corridorY)
        var targetTrunkX = Number(fanInBundle.targetTrunkX)
        if (!isFinite(corridorY))
            corridorY = (sourcePort.y + targetPort.y) / 2
        if (!isFinite(targetTrunkX))
            targetTrunkX = String(fanInBundle.targetApproachSide || "") === "left"
                ? toRect.x - 58
                : toRect.x + toRect.width + 58

        var sourceSideHorizontal = String(fanInBundle.sourceSide || "")
        if (sourceSideHorizontal === "top" || sourceSideHorizontal === "bottom") {
            var corridorStartHorizontal = Qt.point(sourcePort.x, corridorY)
            points.push(corridorStartHorizontal)
            branchPoints.push(corridorStartHorizontal)
        } else {
            var sourceStubX = sourcePort.x + (sourceSideHorizontal === "left" ? -34 : 34)
            var sourceStubHorizontal = Qt.point(sourceStubX, sourcePort.y)
            var sourceLaneHorizontal = Qt.point(sourceStubX, corridorY)
            points.push(sourceStubHorizontal)
            points.push(sourceLaneHorizontal)
            branchPoints.push(sourceStubHorizontal)
            branchPoints.push(sourceLaneHorizontal)
        }

        joinPoint = Qt.point(targetTrunkX, corridorY)
        points.push(joinPoint)
        branchPoints.push(joinPoint)
        trunkPoints.push(joinPoint)
        if (Math.abs(targetPort.y - corridorY) > 0.5) {
            var targetLaneHorizontal = Qt.point(targetTrunkX, targetPort.y)
            points.push(targetLaneHorizontal)
            trunkPoints.push(targetLaneHorizontal)
        }
        points.push(targetPort)
        trunkPoints.push(targetPort)
    }

    var route = makePolylineRoute(points, "orthogonal")
    route.bundleRole = isBundleLeader ? "targetFanInLeader" : "targetFanInBranch"
    route.bundleKey = String(fanInBundle.key || "")
    route.suppressArrow = !isBundleLeader
    route.bundleJoinPoint = joinPoint
    route.bundleBranchPoints = branchPoints
    route.bundleTrunkPoints = trunkPoints
    return route
}

function overviewNaturalDirectRoute(config, edge, fromRect, toRect) {
    var layout = config.overviewMindMapLayout || {}
    var fanInBundle = layoutPlacement(layout, "fanInBundleByEdgeId", edge.id)
    if (!fanInBundle)
        return null

    var outgoingStemPlacement = layoutPlacement(layout, "outgoingStemSlotByEdgeId", edge.id)
    var incomingStemPlacement = layoutPlacement(layout, "incomingStemSlotByEdgeId", edge.id)
    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var deltaX = toCenter.x - fromCenter.x
    var deltaY = toCenter.y - fromCenter.y
    var horizontalOverlap = Math.min(fromRect.x + fromRect.width, toRect.x + toRect.width)
        - Math.max(fromRect.x, toRect.x)
    var verticalOverlap = Math.min(fromRect.y + fromRect.height, toRect.y + toRect.height)
        - Math.max(fromRect.y, toRect.y)
    var sourceLaneOffset = overviewPlacementLaneOffset(outgoingStemPlacement, 18)
    var targetLaneOffset = -overviewPlacementLaneOffset(incomingStemPlacement, 18)
    var candidates = []
    var strongVertical = horizontalOverlap > Math.min(fromRect.width, toRect.width) * 0.12
        || Math.abs(deltaY) >= Math.abs(deltaX) * 0.92
    var strongHorizontal = verticalOverlap > Math.min(fromRect.height, toRect.height) * 0.12
        || Math.abs(deltaX) >= Math.abs(deltaY) * 0.92

    if (strongVertical || !strongHorizontal) {
        candidates = candidates.concat(overviewNaturalVerticalCandidates(config,
            edge,
            fromRect,
            toRect,
            sourceLaneOffset,
            targetLaneOffset))
    }
    if (strongHorizontal || !strongVertical) {
        candidates = candidates.concat(overviewNaturalHorizontalCandidates(config,
            edge,
            fromRect,
            toRect,
            sourceLaneOffset,
            targetLaneOffset))
    }

    if (!candidates.length)
        return null

    var route = chooseBestCandidate(candidates, edge, config.routeObjectHitsModules, config.routeObjectHitStats)
    if (route)
        route.overviewNatural = true
    return route
}

function componentEndpointOffset(config, edge, endpointKey) {
    if (config.endpointPeerOffset)
        return config.endpointPeerOffset(edge, endpointKey, 12)
    return 0
}

function routeTurnCount(route) {
    if (!route)
        return 999999

    var points = routePointList(route)
    return Math.max(0, points.length - 2)
}

function routePointList(route) {
    if (!route)
        return []
    if (route.points && route.points.length >= 2)
        return route.points
    return [route.p0, route.p1, route.p2, route.p3]
}

function clampNumber(value, minimumValue, maximumValue) {
    return Math.max(minimumValue, Math.min(maximumValue, value))
}

function pickNaturalLane(primaryValue, secondaryValue, minimumValue, maximumValue) {
    if (primaryValue >= minimumValue && primaryValue <= maximumValue)
        return primaryValue
    if (secondaryValue >= minimumValue && secondaryValue <= maximumValue)
        return secondaryValue
    return clampNumber((primaryValue + secondaryValue) / 2, minimumValue, maximumValue)
}

function overviewPlacementLaneOffset(placement, maximumOffset) {
    var count = placement && placement.count !== undefined ? Number(placement.count) : 1
    var virtualSpan = Math.max(24, (maximumOffset || 16) * 3.4 + Math.max(0, count - 1) * 16)
    return EdgeUtils.distributedSideOffset(placementRelative(placement), count, virtualSpan, 8, 0.90)
}

function routeAxisTravel(route) {
    var points = routePointList(route)
    var horizontal = 0
    var vertical = 0
    for (var index = 0; index < points.length - 1; ++index) {
        var current = points[index]
        var next = points[index + 1]
        horizontal += Math.abs(next.x - current.x)
        vertical += Math.abs(next.y - current.y)
    }
    return {
        "horizontal": horizontal,
        "vertical": vertical
    }
}

function routeDetourLength(route) {
    var points = routePointList(route)
    if (points.length < 2)
        return 999999

    var first = points[0]
    var last = points[points.length - 1]
    var required = Math.abs(last.x - first.x) + Math.abs(last.y - first.y)
    return Math.max(0, EdgeUtils.routeMetric(route) - required)
}

function routeBoundingExcursion(route) {
    var points = routePointList(route)
    if (points.length < 2)
        return 999999

    var minX = points[0].x
    var maxX = points[0].x
    var minY = points[0].y
    var maxY = points[0].y
    for (var index = 1; index < points.length; ++index) {
        minX = Math.min(minX, points[index].x)
        maxX = Math.max(maxX, points[index].x)
        minY = Math.min(minY, points[index].y)
        maxY = Math.max(maxY, points[index].y)
    }

    var first = points[0]
    var last = points[points.length - 1]
    var endpointSpanX = Math.abs(last.x - first.x)
    var endpointSpanY = Math.abs(last.y - first.y)
    return Math.max(0, (maxX - minX) - endpointSpanX)
        + Math.max(0, (maxY - minY) - endpointSpanY)
}

function routeReverseLength(route) {
    var points = routePointList(route)
    if (points.length < 2)
        return 999999

    var first = points[0]
    var last = points[points.length - 1]
    var directionX = last.x > first.x + 0.5 ? 1 : (last.x < first.x - 0.5 ? -1 : 0)
    var directionY = last.y > first.y + 0.5 ? 1 : (last.y < first.y - 0.5 ? -1 : 0)
    var reverseLength = 0

    for (var index = 0; index < points.length - 1; ++index) {
        var deltaX = points[index + 1].x - points[index].x
        var deltaY = points[index + 1].y - points[index].y
        if (directionX !== 0 && deltaX * directionX < -0.5)
            reverseLength += Math.abs(deltaX)
        if (directionY !== 0 && deltaY * directionY < -0.5)
            reverseLength += Math.abs(deltaY)
    }

    return reverseLength
}

function routePreferenceScore(config, edge, route, preferredBias) {
    var stats = routeStats(route, edge, config.routeObjectHitsModules, config.routeObjectHitStats)
    var anyNodeStats = config.routeObjectAnyNodeHitStats
        ? config.routeObjectAnyNodeHitStats(route, edge)
        : { "hitCount": 0, "hitLength": 0 }
    var detourLength = routeDetourLength(route)
    var boundingExcursion = routeBoundingExcursion(route)
    var reverseLength = routeReverseLength(route)
    var turns = routeTurnCount(route)
    var routeLength = EdgeUtils.routeMetric(route)
    var bundlePenalty = route.bundleRole ? (32 + Math.max(0, turns - 3) * 14) : 0
    var preferencePenalty = preferredBias !== undefined ? preferredBias * 6 : 0

    return stats.hitCount * 200000
        + stats.hitLength * 1500
        + anyNodeStats.hitCount * 120000
        + anyNodeStats.hitLength * 1100
        + reverseLength * 52
        + detourLength * 30
        + boundingExcursion * 22
        + turns * 36
        + routeLength
        + bundlePenalty
        + preferencePenalty
}

function compareRoutePreference(config, edge, leftRoute, rightRoute, leftBias, rightBias) {
    if (!leftRoute && !rightRoute)
        return 0
    if (!leftRoute)
        return 1
    if (!rightRoute)
        return -1

    var leftScore = routePreferenceScore(config, edge, leftRoute, leftBias)
    var rightScore = routePreferenceScore(config, edge, rightRoute, rightBias)
    if (Math.abs(leftScore - rightScore) > 0.5)
        return leftScore < rightScore ? -1 : 1
    return 0
}

function overviewNaturalVerticalCandidates(config, edge, fromRect, toRect, sourceLaneOffset, targetLaneOffset) {
    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var verticalDistance = toCenter.y - fromCenter.y
    if (Math.abs(verticalDistance) < 24)
        return []

    var sourceSide = verticalDistance >= 0 ? "bottom" : "top"
    var targetSide = verticalDistance >= 0 ? "top" : "bottom"
    var sourcePort = EdgeUtils.portPoint(fromRect, sourceSide, sourceLaneOffset || 0)
    var targetPort = EdgeUtils.portPoint(toRect, targetSide, targetLaneOffset || 0)
    var candidates = []
    var overlapInset = 16
    var overlapMinX = Math.max(fromRect.x + overlapInset, toRect.x + overlapInset)
    var overlapMaxX = Math.min(fromRect.x + fromRect.width - overlapInset,
        toRect.x + toRect.width - overlapInset)

    if (overlapMaxX >= overlapMinX) {
        var laneX = pickNaturalLane(sourcePort.x, targetPort.x, overlapMinX, overlapMaxX)
        candidates.push({
            "route": makePolylineRoute([
                sourcePort,
                Qt.point(laneX, sourcePort.y),
                Qt.point(laneX, targetPort.y),
                targetPort
            ],
                "orthogonal"),
            "preferred": 0
        })
    }

    if (config.directRoute) {
        candidates.push({
            "route": EdgeUtils.orthogonalRouteObject(
                config.directRoute(edge, fromRect, toRect, config.laneIndex, true)),
            "preferred": 1
        })
    }

    return candidates
}

function overviewNaturalHorizontalCandidates(config, edge, fromRect, toRect, sourceLaneOffset, targetLaneOffset) {
    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var horizontalDistance = toCenter.x - fromCenter.x
    if (Math.abs(horizontalDistance) < 24)
        return []

    var sourceSide = horizontalDistance >= 0 ? "right" : "left"
    var targetSide = horizontalDistance >= 0 ? "left" : "right"
    var sourcePort = EdgeUtils.portPoint(fromRect, sourceSide, sourceLaneOffset || 0)
    var targetPort = EdgeUtils.portPoint(toRect, targetSide, targetLaneOffset || 0)
    var candidates = []
    var overlapInset = 16
    var overlapMinY = Math.max(fromRect.y + overlapInset, toRect.y + overlapInset)
    var overlapMaxY = Math.min(fromRect.y + fromRect.height - overlapInset,
        toRect.y + toRect.height - overlapInset)

    if (overlapMaxY >= overlapMinY) {
        var laneY = pickNaturalLane(sourcePort.y, targetPort.y, overlapMinY, overlapMaxY)
        candidates.push({
            "route": makePolylineRoute([
                sourcePort,
                Qt.point(sourcePort.x, laneY),
                Qt.point(targetPort.x, laneY),
                targetPort
            ],
                "orthogonal"),
            "preferred": 0
        })
    }

    if (config.directRoute) {
        candidates.push({
            "route": EdgeUtils.orthogonalRouteObject(
                config.directRoute(edge, fromRect, toRect, config.laneIndex, false)),
            "preferred": 1
        })
    }

    return candidates
}

function componentNaturalVerticalCandidates(config, edge, fromRect, toRect, routeContext) {
    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var verticalDistance = toCenter.y - fromCenter.y
    if (Math.abs(verticalDistance) < 28)
        return []

    var sourceSide = verticalDistance >= 0 ? "bottom" : "top"
    var targetSide = verticalDistance >= 0 ? "top" : "bottom"
    var sourcePort = EdgeUtils.portPoint(fromRect, sourceSide, routeContext.sourceOffset || 0)
    var targetPort = EdgeUtils.portPoint(toRect, targetSide, routeContext.targetOffset || 0)
    var candidates = []
    var overlapInset = 18
    var overlapMinX = Math.max(fromRect.x + overlapInset, toRect.x + overlapInset)
    var overlapMaxX = Math.min(fromRect.x + fromRect.width - overlapInset,
        toRect.x + toRect.width - overlapInset)

    if (overlapMaxX >= overlapMinX) {
        var laneX = pickNaturalLane(sourcePort.x, targetPort.x, overlapMinX, overlapMaxX)
        candidates.push({
            "route": makePolylineRoute([
                sourcePort,
                Qt.point(laneX, sourcePort.y),
                Qt.point(laneX, targetPort.y),
                targetPort
            ],
                "orthogonal"),
            "preferred": 0
        })
    }

    candidates.push({
        "route": EdgeUtils.orthogonalRouteObject(
            config.directRoute(edge, fromRect, toRect, config.laneIndex, true)),
        "preferred": 2
    })

    return candidates
}

function componentNaturalHorizontalCandidates(config, edge, fromRect, toRect, routeContext) {
    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var horizontalDistance = toCenter.x - fromCenter.x
    if (Math.abs(horizontalDistance) < 28)
        return []

    var sourceSide = horizontalDistance >= 0 ? "right" : "left"
    var targetSide = horizontalDistance >= 0 ? "left" : "right"
    var sourcePort = EdgeUtils.portPoint(fromRect, sourceSide, routeContext.sourceOffset || 0)
    var targetPort = EdgeUtils.portPoint(toRect, targetSide, routeContext.targetOffset || 0)
    var candidates = []
    var overlapInset = 18
    var overlapMinY = Math.max(fromRect.y + overlapInset, toRect.y + overlapInset)
    var overlapMaxY = Math.min(fromRect.y + fromRect.height - overlapInset,
        toRect.y + toRect.height - overlapInset)

    if (overlapMaxY >= overlapMinY) {
        var laneY = pickNaturalLane(sourcePort.y, targetPort.y, overlapMinY, overlapMaxY)
        candidates.push({
            "route": makePolylineRoute([
                sourcePort,
                Qt.point(sourcePort.x, laneY),
                Qt.point(targetPort.x, laneY),
                targetPort
            ],
                "orthogonal"),
            "preferred": 0
        })
    }

    candidates.push({
        "route": EdgeUtils.orthogonalRouteObject(
            config.directRoute(edge, fromRect, toRect, config.laneIndex, false)),
        "preferred": 2
    })

    return candidates
}

function componentNaturalDirectRoute(config, edge, fromRect, toRect, routeContext) {
    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var deltaX = toCenter.x - fromCenter.x
    var deltaY = toCenter.y - fromCenter.y
    var horizontalOverlap = Math.min(fromRect.x + fromRect.width, toRect.x + toRect.width)
        - Math.max(fromRect.x, toRect.x)
    var verticalOverlap = Math.min(fromRect.y + fromRect.height, toRect.y + toRect.height)
        - Math.max(fromRect.y, toRect.y)
    var strongVertical = horizontalOverlap > Math.min(fromRect.width, toRect.width) * 0.22
        || Math.abs(deltaY) >= Math.abs(deltaX) * 1.22
    var strongHorizontal = verticalOverlap > Math.min(fromRect.height, toRect.height) * 0.22
        || Math.abs(deltaX) >= Math.abs(deltaY) * 1.22

    if (!strongVertical && !strongHorizontal)
        return null

    var candidates = []
    if (strongVertical)
        candidates = candidates.concat(componentNaturalVerticalCandidates(config, edge, fromRect, toRect, routeContext))
    if (strongHorizontal)
        candidates = candidates.concat(componentNaturalHorizontalCandidates(config, edge, fromRect, toRect, routeContext))
    if (!candidates.length)
        return null

    var route = chooseBestCandidate(candidates, edge, config.routeObjectHitsModules, config.routeObjectHitStats)
    if (route)
        route.componentNatural = true
    return route
}

function preferNaturalRoute(config, edge, naturalRoute, otherRoute) {
    return compareRoutePreference(config, edge, naturalRoute, otherRoute, 0, 3) < 0
}



function readableBoundaryPoint(rect, towardPoint) {
    var cx = rect.x + rect.width / 2
    var cy = rect.y + rect.height / 2
    var dx = Number(towardPoint.x) - cx
    var dy = Number(towardPoint.y) - cy
    if (Math.abs(dx) < 0.001 && Math.abs(dy) < 0.001)
        return Qt.point(cx, cy)

    var halfW = Math.max(1, rect.width / 2)
    var halfH = Math.max(1, rect.height / 2)
    var scaleX = Math.abs(dx) > 0.001 ? halfW / Math.abs(dx) : 999999
    var scaleY = Math.abs(dy) > 0.001 ? halfH / Math.abs(dy) : 999999
    var scale = Math.min(scaleX, scaleY)
    return Qt.point(cx + dx * scale, cy + dy * scale)
}

function readableOutsidePoint(rect, towardPoint, clearance) {
    var boundary = readableBoundaryPoint(rect, towardPoint)
    var cx = rect.x + rect.width / 2
    var cy = rect.y + rect.height / 2
    var dx = boundary.x - cx
    var dy = boundary.y - cy
    var len = Math.sqrt(dx * dx + dy * dy)
    if (len < 0.001)
        return boundary
    var gap = clearance === undefined ? 20 : Number(clearance)
    return Qt.point(boundary.x + dx / len * gap, boundary.y + dy / len * gap)
}

function readableStraightRoute(config, edge, fromRect, toRect) {
    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var arrowClearance = edge && edge.bidirectional ? 24 : 18
    var start = readableOutsidePoint(fromRect, toCenter, arrowClearance)
    var end = readableOutsidePoint(toRect, fromCenter, arrowClearance)

    // If multiple normalized edges still share a pair, keep direct-line semantics
    // but add a tiny perpendicular lane offset so the routes remain readable.
    var laneOffset = 0
    if (edge && edge.parallelIndex !== undefined && edge.parallelCount !== undefined) {
        var relative = Number(edge.parallelIndex) - (Number(edge.parallelCount) - 1) / 2
        if (isFinite(relative))
            laneOffset = relative * 18
    }

    if (Math.abs(laneOffset) > 0.01) {
        var dx = end.x - start.x
        var dy = end.y - start.y
        var len = Math.sqrt(dx * dx + dy * dy)
        if (len > 0.001) {
            var nx = -dy / len
            var ny = dx / len
            start = Qt.point(start.x + nx * laneOffset, start.y + ny * laneOffset)
            end = Qt.point(end.x + nx * laneOffset, end.y + ny * laneOffset)
        }
    }

    var route = makePolylineRoute([start, end], "straight")
    route.bidirectional = !!edge.bidirectional
    route.readableDirectLine = true
    route.readableRouter = "generic-force-straight"
    readableDebug("ROUTER", "straight-route " + readableEdgeLabel(edge)
        + " fromCenter=" + readablePointText(fromCenter)
        + " toCenter=" + readablePointText(toCenter)
        + " start=" + readablePointText(start)
        + " end=" + readablePointText(end)
        + " bidirectional=" + String(!!edge.bidirectional)
        + " arrowClearance=" + String(arrowClearance)
        + " " + readableRouteText(route))
    return route
}

function readableEndpointOffset(config, edge, endpointKey, step) {
    var value = config.endpointPeerOffset
        ? config.endpointPeerOffset(edge, endpointKey, step)
        : EdgeUtils.edgeLane(config.laneIndex) * 0.7
    readableDebug("ROUTER", "endpoint-offset edge=" + readableEdgeLabel(edge) + " endpointKey=" + String(endpointKey) + " step=" + String(step) + " value=" + String(value))
    return value
}

function readablePortSide(fromRect, toRect, fromIsSource) {
    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var dx = toCenter.x - fromCenter.x
    var dy = toCenter.y - fromCenter.y
    if (Math.abs(dx) >= Math.abs(dy) * 0.82) {
        if (fromIsSource)
            return dx >= 0 ? "right" : "left"
        return dx >= 0 ? "left" : "right"
    }
    if (fromIsSource)
        return dy >= 0 ? "bottom" : "top"
    return dy >= 0 ? "top" : "bottom"
}

function readableRouteCandidate(config, edge, fromRect, toRect, horizontalFirst, sourceOffset, targetOffset, trackOffset) {
    var sourceSide = readablePortSide(fromRect, toRect, true)
    var targetSide = readablePortSide(fromRect, toRect, false)
    var start = EdgeUtils.portPoint(fromRect, sourceSide, sourceOffset)
    var end = EdgeUtils.portPoint(toRect, targetSide, -targetOffset)

    var points = [start]
    if (horizontalFirst) {
        var midX = (start.x + end.x) / 2 + trackOffset
        if (sourceSide === "left" || sourceSide === "right") {
            points.push(Qt.point(midX, start.y))
            points.push(Qt.point(midX, end.y))
        } else {
            var firstY = start.y + (end.y >= start.y ? 1 : -1) * (42 + Math.abs(trackOffset) * 0.25)
            points.push(Qt.point(start.x, firstY))
            points.push(Qt.point(midX, firstY))
            points.push(Qt.point(midX, end.y))
        }
    } else {
        var midY = (start.y + end.y) / 2 + trackOffset
        if (sourceSide === "top" || sourceSide === "bottom") {
            points.push(Qt.point(start.x, midY))
            points.push(Qt.point(end.x, midY))
        } else {
            var firstX = start.x + (end.x >= start.x ? 1 : -1) * (46 + Math.abs(trackOffset) * 0.25)
            points.push(Qt.point(firstX, start.y))
            points.push(Qt.point(firstX, midY))
            points.push(Qt.point(end.x, midY))
        }
    }
    points.push(end)

    var route = makePolylineRoute(points, "orthogonal")
    route.bidirectional = !!edge.bidirectional
    route.readableSourceSide = sourceSide
    route.readableTargetSide = targetSide
    route.readableHorizontalFirst = horizontalFirst
    readableDebug("ROUTER", "candidate edge=" + readableEdgeLabel(edge) + " horizontalFirst=" + String(horizontalFirst) + " sourceSide=" + sourceSide + " targetSide=" + targetSide + " sourceOffset=" + String(sourceOffset) + " targetOffset=" + String(targetOffset) + " trackOffset=" + String(trackOffset) + " " + readableRouteText(route))
    return route
}

function readableOverviewRoute(config, edge, fromRect, toRect) {
    // Final readable graph mode: mirror draw.io-style relation maps.
    // Use a single straight segment between rectangle boundary points.
    // Direction is encoded by arrowheads; bidirectional normalized edges are rendered with two arrowheads.
    return readableStraightRoute(config, edge, fromRect, toRect)
}

function overviewMindMapRoute(config) {
    var edge = config.edge
    readableDebug("ROUTER", "overview-attempt componentMode=" + String(!!config.componentMode) + " laneIndex=" + String(config.laneIndex) + " " + readableEdgeLabel(edge))
    if (config.componentMode) {
        readableDebug("ROUTER", "overview-skip componentMode=true " + readableEdgeLabel(edge))
        return null
    }

    var fromRect = config.nodeRectById(edge.fromId)
    var toRect = config.nodeRectById(edge.toId)
    if (!fromRect.valid || !toRect.valid) {
        readableDebug("ROUTER", "overview-skip invalid-rect fromValid=" + String(!!fromRect.valid) + " toValid=" + String(!!toRect.valid) + " " + readableEdgeLabel(edge))
        return null
    }

    return readableOverviewRoute(config, edge, fromRect, toRect)
}

function componentTargetFanInRoute(config, edge, fromRect, toRect, routeContext) {
    var layout = config.componentOverviewLayout || {}
    var fanInBundle = layoutPlacement(layout, "fanInBundleByEdgeId", edge.id)
    if (!fanInBundle || fanInBundle.count === undefined || Number(fanInBundle.count) <= 1)
        return null

    var fromCenter = EdgeUtils.nodeCenter(fromRect)
    var toCenter = EdgeUtils.nodeCenter(toRect)
    var forward = toCenter.x >= fromCenter.x
    var sourceSide = forward ? "right" : "left"
    var targetApproachSide = String(fanInBundle.targetApproachSide || (forward ? "left" : "right"))
    if (targetApproachSide !== "left" && targetApproachSide !== "right")
        targetApproachSide = forward ? "left" : "right"

    var sourceOffset = routeContext.sourceOffset || 0
    var targetOffset = routeContext.targetOffset || 0
    var trackRelative = routeContext.trackRelative || 0
    var clusterCount = fanInBundle.clusterCount !== undefined ? Math.max(1, Number(fanInBundle.clusterCount)) : 1
    var clusterIndex = fanInBundle.clusterIndex !== undefined ? Number(fanInBundle.clusterIndex) : 0
    var clusterRelative = clusterCount > 1 ? clusterIndex - (clusterCount - 1) / 2 : 0
    var branchRelative = fanInBundle.index !== undefined && fanInBundle.count !== undefined
        ? Number(fanInBundle.index) - (Number(fanInBundle.count) - 1) / 2
        : 0

    // V3: target-local fan-in + branch-safe lanes.
    // The first refactor used a long shared horizontal corridor across the canvas.
    // V2 moved the bundle to a short target-side spine, but branch horizontals could
    // still run through cards because they used the source center Y.  V3 keeps the
    // target-side spine, while each source first moves to a safe lane above/below
    // its card before traveling horizontally to the target spine.
    var sourcePort = EdgeUtils.portPoint(fromRect,
        sourceSide,
        sourceOffset + distributedPortOffset(fromRect,
            sourceSide,
            branchRelative,
            Number(fanInBundle.count),
            20,
            0.70))

    // One target entry per target/proximity cluster.  Do not add branchRelative here;
    // otherwise a fan-in group becomes several independent target entrances again.
    var targetPort = EdgeUtils.portPoint(toRect,
        targetApproachSide,
        targetOffset + distributedPortOffset(toRect,
            targetApproachSide,
            clusterRelative,
            clusterCount,
            22,
            0.72))

    var targetDirection = targetApproachSide === "left" ? -1 : 1
    var sourceDirection = sourceSide === "left" ? -1 : 1

    var trunkGap = 40 + Math.abs(clusterRelative) * 18 + Math.min(20, Math.max(0, Number(fanInBundle.count) - 2) * 4)
    var trunkX = targetPort.x + targetDirection * trunkGap
    var sourceStubX = sourcePort.x + sourceDirection * (36 + Math.abs(trackRelative) * 8)
    var corridorY = Number(fanInBundle.corridorY)
    if (!isFinite(corridorY))
        corridorY = (sourcePort.y + targetPort.y) / 2
    var branchLaneY = corridorY
    var joinPoint = Qt.point(trunkX, branchLaneY)
    var targetJoin = Qt.point(trunkX, targetPort.y)
    var edgeIdStr = edge && edge.id !== undefined ? String(edge.id) : ""
    var leaderId = fanInBundle.leaderEdgeId !== undefined ? String(fanInBundle.leaderEdgeId) : ""
    var isLeader = !leaderId.length || leaderId === edgeIdStr

    var points = [
        sourcePort,
        Qt.point(sourceStubX, sourcePort.y),
        Qt.point(sourceStubX, branchLaneY),
        joinPoint,
        targetJoin
    ]

    points.push(targetPort)

    var branchPoints = [
        sourcePort,
        Qt.point(sourceStubX, sourcePort.y),
        Qt.point(sourceStubX, branchLaneY),
        joinPoint
    ]
    var trunkPoints = [
        joinPoint,
        targetJoin,
        targetPort
    ]

    var route = makePolylineRoute(points, "orthogonal")
    route.bundleRole = isLeader ? "targetFanInLeader" : "targetFanInBranch"
    route.bundleKey = String(fanInBundle.key || "")
    route.suppressArrow = !isLeader
    route.bundleJoinPoint = joinPoint
    route.bundleBranchPoints = branchPoints
    route.bundleTrunkPoints = trunkPoints

    debugComponentRoute(edge,
        "target-fan-in-v4",
        "target=" + String(edge.toId)
        + " count=" + String(fanInBundle.count)
        + " cluster=" + String(fanInBundle.clusterIndex)
        + "/" + String(fanInBundle.clusterCount)
        + " leader=" + String(isLeader)
        + " trunkX=" + String(trunkX)
        + " corridorY=" + String(branchLaneY))

    return route
}

function componentOverviewRoute(config) {
    var edge = config.edge
    if (!config.componentMode || config.relationshipFocusActive) {
        debugComponentRoute(edge,
            "skip-component-detail-v9",
            "componentMode=" + String(!!config.componentMode)
            + " focusActive=" + String(!!config.relationshipFocusActive))
        return null
    }

    var fromRect = config.nodeRectById(edge.fromId)
    var toRect = config.nodeRectById(edge.toId)
    if (!fromRect.valid || !toRect.valid)
        return null

    // Component/detail mode now follows the same readability-first rule as the
    // architecture overview: one independent straight relation per normalized edge.
    // No fan-in/fan-out bundling, no group rails, no hover-only alternate route.
    var route = readableStraightRoute(config, edge, fromRect, toRect)
    route.componentDetailGraph = true
    route.readableRouter = "component-detail-straight-v9"
    debugComponentRoute(edge,
        "component-detail-straight-v9",
        readableRouteText(route))
    return route
}

function dragPreviewRoute(config, edge, fromRect, toRect) {
    var fromCenterX = fromRect.x + fromRect.width / 2
    var fromCenterY = fromRect.y + fromRect.height / 2
    var toCenterX = toRect.x + toRect.width / 2
    var toCenterY = toRect.y + toRect.height / 2
    var vertical = Math.abs(toCenterY - fromCenterY) >= Math.abs(toCenterX - fromCenterX) * 0.72
    var direct = config.directRoute(edge, fromRect, toRect, config.laneIndex, vertical)
    var route = makeOrthogonalRoute(direct[0], direct[1], direct[2], direct[3])
    route.bidirectional = !!edge.bidirectional
    readableDebug("ROUTER", "overview-selected vertical=" + String(vertical) + " fromRect=(" + Math.round(fromRect.x) + "," + Math.round(fromRect.y) + "," + Math.round(fromRect.width) + "," + Math.round(fromRect.height) + ") toRect=(" + Math.round(toRect.x) + "," + Math.round(toRect.y) + "," + Math.round(toRect.width) + "," + Math.round(toRect.height) + ") " + readableEdgeLabel(edge) + " " + readableRouteText(route))
    return route
}

function routeEdge(config) {
    var edge = config.edge
    readableDebug("ROUTER", "route-start componentMode=" + String(!!config.componentMode) + " focus=" + String(!!config.relationshipFocusActive) + " dragging=" + String(!!config.draggingNodeCard) + " laneIndex=" + String(config.laneIndex) + " " + readableEdgeLabel(edge))

    // Single-layer mode: dragging never switches to a special preview route.
    // The same route pipeline runs for overview, hover, and drag; only visual emphasis changes.

    var overviewRoute = overviewMindMapRoute(config)
    if (overviewRoute) {
        readableDebug("ROUTER", "route-selected type=overview " + readableEdgeLabel(edge) + " " + readableRouteText(overviewRoute))
        return overviewRoute
    }

    var componentRoute = componentOverviewRoute(config)
    if (componentRoute) {
        readableDebug("ROUTER", "route-selected type=component " + readableEdgeLabel(edge) + " " + readableRouteText(componentRoute))
        return componentRoute
    }

    var mappedRoute = routeFromScenePoints(config)
    if (mappedRoute) {
        readableDebug("ROUTER", "route-selected type=scene-points " + readableEdgeLabel(edge) + " " + readableRouteText(mappedRoute))
        return mappedRoute
    }

    var fromRect = config.nodeRectById(edge.fromId)
    var toRect = config.nodeRectById(edge.toId)
    if (!fromRect.valid || !toRect.valid) {
        var emptyPoint = Qt.point(0, 0)
        var emptyRoute = { "p0": emptyPoint, "p1": emptyPoint, "p2": emptyPoint, "p3": emptyPoint }
        readableDebug("ROUTER", "route-selected type=empty-invalid-rect " + readableEdgeLabel(edge) + " " + readableRouteText(emptyRoute))
        return emptyRoute
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

    var chosenFallback = chooseBestCandidate([
        { "route": directPrimary, "preferred": 0 },
        { "route": directAlternate, "preferred": 1 },
        { "route": bypassPrimary, "preferred": 2 },
        { "route": bypassAlternate, "preferred": 3 }
    ],
        edge,
        config.routeObjectHitsModules,
        config.routeObjectHitStats)
    readableDebug("ROUTER", "route-selected type=fallback-competition vertical=" + String(vertical) + " " + readableEdgeLabel(edge) + " chosen=" + readableRouteText(chosenFallback) + " directPrimary=" + readableRouteText(directPrimary) + " directAlternate=" + readableRouteText(directAlternate) + " bypassPrimary=" + readableRouteText(bypassPrimary) + " bypassAlternate=" + readableRouteText(bypassAlternate))
    return chosenFallback
}
