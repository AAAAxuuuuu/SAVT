.pragma library

function makePoint(x, y) {
    return Qt.point(x, y)
}

function edgeTouchesFocus(focusNode, edge) {
    return !!focusNode && !!edge
            && (edge.fromId === focusNode.id || edge.toId === focusNode.id)
}

function overviewHoverRelation(hoverNode, edge) {
    if (!hoverNode || hoverNode.id === undefined || !edge)
        return ""
    if (edge.toId === hoverNode.id)
        return "incoming"
    if (edge.fromId === hoverNode.id)
        return "outgoing"
    return ""
}

function emptyRailLayout() {
    return {
        "placements": ({}),
        "groupCounts": ({
            "incoming": 0,
            "outgoing": 0
        })
    }
}

function sortRailMembers(members, sortKey) {
    members.sort(function(left, right) {
        if (Math.abs(left[sortKey] - right[sortKey]) > 0.5)
            return left[sortKey] - right[sortKey]
        return String(left.edgeId).localeCompare(String(right.edgeId))
    })
}

function finalizeRailLayout(groups) {
    var layout = emptyRailLayout()
    var groupKeys = ["incoming", "outgoing"]

    for (var groupIndex = 0; groupIndex < groupKeys.length; ++groupIndex) {
        var groupKey = groupKeys[groupIndex]
        var members = groups[groupKey] || []
        layout.groupCounts[groupKey] = members.length

        for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
            var edgeKey = String(members[memberIndex].edgeId)
            layout.placements[edgeKey] = {
                "valid": true,
                "groupKey": groupKey,
                "index": memberIndex,
                "count": members.length
            }
        }
    }

    return layout
}

function buildFocusRailLayout(config) {
    var focusNode = config.focusNode
    if (!config.componentMode || !focusNode || focusNode.id === undefined)
        return emptyRailLayout()

    var groups = {
        "incoming": [],
        "outgoing": []
    }
    var visibleEdges = config.visibleEdges || []
    for (var index = 0; index < visibleEdges.length; ++index) {
        var candidate = visibleEdges[index].edge
        if (!candidate)
            continue

        var groupKey = candidate.fromId === focusNode.id
                ? "outgoing"
                : (candidate.toId === focusNode.id ? "incoming" : "")
        if (!groupKey)
            continue

        var otherNodeId = candidate.fromId === focusNode.id ? candidate.toId : candidate.fromId
        var otherRect = config.nodeRectById(otherNodeId)
        if (!otherRect.valid)
            continue

        groups[groupKey].push({
            "edgeId": candidate.id,
            "sortValue": otherRect.y + otherRect.height / 2
        })
    }

    sortRailMembers(groups.incoming, "sortValue")
    sortRailMembers(groups.outgoing, "sortValue")
    return finalizeRailLayout(groups)
}

function buildOverviewHoverRailLayout(config) {
    var hoverNode = config.hoverNode
    if (!hoverNode || hoverNode.id === undefined || config.componentMode)
        return emptyRailLayout()

    var groups = {
        "incoming": [],
        "outgoing": []
    }
    var edges = config.edges || []
    for (var index = 0; index < edges.length; ++index) {
        var candidate = edges[index]
        if (!candidate || !config.isOverviewPrimaryEdge(candidate))
            continue

        var relation = config.overviewHoverRelation(candidate)
        if (!relation)
            continue

        var otherNodeId = relation === "outgoing" ? candidate.toId : candidate.fromId
        var otherRect = config.nodeRectById(otherNodeId)
        if (!otherRect.valid)
            continue

        groups[relation].push({
            "edgeId": candidate.id,
            "sortValue": otherRect.x + otherRect.width / 2
        })
    }

    sortRailMembers(groups.incoming, "sortValue")
    sortRailMembers(groups.outgoing, "sortValue")
    return finalizeRailLayout(groups)
}

function railPlacement(layout, edge, groupKey) {
    if (!groupKey || !edge || !layout)
        return {"valid": false, "groupKey": groupKey || "", "index": 0, "count": 0}

    var placement = layout.placements ? layout.placements[String(edge.id)] : null
    if (placement && placement.groupKey === groupKey)
        return placement

    var counts = layout.groupCounts || ({})
    return {"valid": false, "groupKey": groupKey, "index": 0, "count": counts[groupKey] || 0}
}

function buildEndpointPeerOffsets(config) {
    var output = {
        "fromId": ({}),
        "toId": ({})
    }
    var grouped = {
        "fromId": ({}),
        "toId": ({})
    }
    var visibleEdges = config.visibleEdges || []
    var nodeRectById = config.nodeRectById

    for (var index = 0; index < visibleEdges.length; ++index) {
        var candidate = visibleEdges[index].edge
        if (!candidate)
            continue

        var endpointKeys = ["fromId", "toId"]
        for (var endpointIndex = 0; endpointIndex < endpointKeys.length; ++endpointIndex) {
            var endpointKey = endpointKeys[endpointIndex]
            var peerKey = endpointKey === "fromId" ? "toId" : "fromId"
            var nodeKey = String(candidate[endpointKey])
            if (!grouped[endpointKey][nodeKey])
                grouped[endpointKey][nodeKey] = []

            var endpointRect = nodeRectById ? nodeRectById(candidate[endpointKey]) : null
            var peerRect = nodeRectById ? nodeRectById(candidate[peerKey]) : null
            var endpointCenter = endpointRect && endpointRect.valid ? nodeCenter(endpointRect) : null
            var peerCenter = peerRect && peerRect.valid ? nodeCenter(peerRect) : null
            var sortPrimary = index
            var sortSecondary = 0

            if (endpointCenter && peerCenter) {
                var verticalRelation = Math.abs(peerCenter.y - endpointCenter.y) > Math.abs(peerCenter.x - endpointCenter.x)
                sortPrimary = verticalRelation ? peerCenter.x : peerCenter.y
                sortSecondary = verticalRelation ? peerCenter.y : peerCenter.x
            }

            grouped[endpointKey][nodeKey].push({
                "edgeId": String(candidate.id),
                "sortPrimary": sortPrimary,
                "sortSecondary": sortSecondary
            })
        }
    }

    var allEndpointKeys = ["fromId", "toId"]
    for (var keyIndex = 0; keyIndex < allEndpointKeys.length; ++keyIndex) {
        var key = allEndpointKeys[keyIndex]
        var groups = grouped[key]
        for (var nodeId in groups) {
            var endpointMembers = groups[nodeId]
            endpointMembers.sort(function(left, right) {
                if (Math.abs(left.sortPrimary - right.sortPrimary) > 0.5)
                    return left.sortPrimary - right.sortPrimary
                if (Math.abs(left.sortSecondary - right.sortSecondary) > 0.5)
                    return left.sortSecondary - right.sortSecondary
                return left.edgeId.localeCompare(right.edgeId)
            })
            for (var edgeIndex = 0; edgeIndex < endpointMembers.length; ++edgeIndex)
                output[key][endpointMembers[edgeIndex].edgeId] = edgeIndex - (endpointMembers.length - 1) / 2
        }
    }

    return output
}

function endpointPeerOffset(offsets, edge, endpointKey, step) {
    if (!offsets || !edge || !endpointKey)
        return 0

    var endpointOffsets = offsets[endpointKey] || ({})
    var relativeOffset = endpointOffsets[String(edge.id)]
    return relativeOffset === undefined ? 0 : relativeOffset * step
}

function overviewDefaultEdgeScore(edge, layout) {
    if (!edge)
        return -999999

    var score = 0
    var kind = String(edge.kind || "").toLowerCase()
    var layerById = layout && layout.layerById ? layout.layerById : ({})
    var rootId = layout ? layout.rootId : undefined
    var fromLayer = layerById[edge.fromId]
    var toLayer = layerById[edge.toId]
    var layerDelta = fromLayer !== undefined && toLayer !== undefined
            ? toLayer - fromLayer
            : 1

    if (edge.fromId === rootId)
        score += 36
    if (layerDelta > 0)
        score += 30
    if (layerDelta === 1)
        score += 18
    if (layerDelta === 0)
        score += 6
    if (layerDelta < 0)
        score -= 18
    if (Math.abs(layerDelta) > 1)
        score -= Math.min(18, (Math.abs(layerDelta) - 1) * 6)

    if (kind === "activates")
        score += 16
    else if (kind === "enables")
        score += 12
    else if (kind === "uses_infrastructure")
        score += 6

    var weight = Number(edge.weight !== undefined ? edge.weight : edge.confidence)
    if (isFinite(weight))
        score += Math.min(10, Math.max(0, weight * 4))

    return score
}

function edgeIdText(edge) {
    return String(edge && edge.id !== undefined ? edge.id : "")
}

function collectVisibleEdges(config) {
    var output = []
    var edgeList = config.edgeList || []
    if (config.relationshipFocusActive)
        return output

    if (!config.componentMode) {
        var overviewNode = !config.showAll
                && config.hoveredNode && config.hoveredNode.id !== undefined
                ? config.hoveredNode
                : null
        var overviewLimit = Math.min(config.overviewEdgeLimit || 12, 12)

        if (!config.showAll && !overviewNode) {
            var candidates = []
            var layout = config.overviewMindMapLayout || ({})
            for (var candidateIndex = 0; candidateIndex < edgeList.length; ++candidateIndex) {
                var candidateEdge = edgeList[candidateIndex]
                if (!config.isOverviewPrimaryEdge(candidateEdge))
                    continue

                candidates.push({
                    "edge": candidateEdge,
                    "score": overviewDefaultEdgeScore(candidateEdge, layout)
                })
            }

            candidates.sort(function(left, right) {
                if (Math.abs(left.score - right.score) > 0.01)
                    return right.score - left.score
                return edgeIdText(left.edge).localeCompare(edgeIdText(right.edge))
            })

            var outgoingCounts = {}
            var incomingCounts = {}
            var maxOutgoingPerNode = 2
            var maxIncomingPerNode = 2

            for (var pickIndex = 0; pickIndex < candidates.length && output.length < overviewLimit; ++pickIndex) {
                var pickedEdge = candidates[pickIndex].edge
                var fromKey = String(pickedEdge.fromId)
                var toKey = String(pickedEdge.toId)
                if ((outgoingCounts[fromKey] || 0) >= maxOutgoingPerNode)
                    continue
                if ((incomingCounts[toKey] || 0) >= maxIncomingPerNode)
                    continue

                outgoingCounts[fromKey] = (outgoingCounts[fromKey] || 0) + 1
                incomingCounts[toKey] = (incomingCounts[toKey] || 0) + 1
                output.push({
                    "edge": pickedEdge,
                    "laneIndex": output.length
                })
            }

            if (output.length === 0 && candidates.length > 0) {
                for (var fallbackIndex = 0; fallbackIndex < candidates.length && output.length < overviewLimit; ++fallbackIndex) {
                    output.push({
                        "edge": candidates[fallbackIndex].edge,
                        "laneIndex": output.length
                    })
                }
            }

            return output
        }

        for (var overviewIndex = 0; overviewIndex < edgeList.length; ++overviewIndex) {
            var primaryEdge = edgeList[overviewIndex]
            if (!config.isOverviewPrimaryEdge(primaryEdge))
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

    if (!config.showAll)
        return output

    var node = config.selectedNode || config.hoveredNode
    var limit = node ? config.focusEdgeLimit : config.overviewEdgeLimit
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

function pointDistance(left, right) {
    var dx = right.x - left.x
    var dy = right.y - left.y
    return Math.sqrt(dx * dx + dy * dy)
}

function interpolatePoint(fromPoint, toPoint, amount) {
    var t = Math.max(0, Math.min(1, amount))
    return makePoint(fromPoint.x + (toPoint.x - fromPoint.x) * t,
                     fromPoint.y + (toPoint.y - fromPoint.y) * t)
}

function mixColor(left, right, amount) {
    var t = Math.max(0, Math.min(1, amount))
    return Qt.rgba(left.r * (1 - t) + right.r * t,
                   left.g * (1 - t) + right.g * t,
                   left.b * (1 - t) + right.b * t,
                   left.a * (1 - t) + right.a * t)
}

function routePoints(route) {
    if (!route)
        return []
    if (route.points && route.points.length >= 2)
        return route.points
    return [route.p0, route.p1, route.p2, route.p3]
}

function routeEndPoint(route) {
    var points = routePoints(route)
    return points.length > 0 ? points[points.length - 1] : makePoint(0, 0)
}

function routeMetric(route) {
    if (!route)
        return 999999
    var points = routePoints(route)
    var length = 0
    for (var index = 0; index < points.length - 1; ++index)
        length += pointDistance(points[index], points[index + 1])
    return length
}

function routeSegmentDescriptor(entry, segmentIndex, laneBucketSize, minSegmentLength) {
    var route = entry ? entry.route : null
    if (!route || route.style === "curved")
        return null

    var points = routePoints(route)
    if (points.length < 4)
        return null
    if (segmentIndex <= 0 || segmentIndex >= points.length - 2)
        return null

    var p1 = points[segmentIndex]
    var p2 = points[segmentIndex + 1]

    var vertical = Math.abs(p1.x - p2.x) < 0.5
    var horizontal = Math.abs(p1.y - p2.y) < 0.5
    if (!vertical && !horizontal)
        return null

    var segmentLength = vertical
            ? Math.abs(p2.y - p1.y)
            : Math.abs(p2.x - p1.x)
    if (segmentLength < minSegmentLength)
        return null

    var lane = vertical ? p1.x : p1.y
    var start = vertical ? Math.min(p1.y, p2.y) : Math.min(p1.x, p2.x)
    var end = vertical ? Math.max(p1.y, p2.y) : Math.max(p1.x, p2.x)

    return {
        "entry": entry,
        "orientation": vertical ? "v" : "h",
        "lane": lane,
        "laneKey": Math.round(lane / laneBucketSize),
        "start": start,
        "end": end,
        "center": (start + end) / 2,
        "segmentIndex": segmentIndex,
        "edgeId": String(entry.edge && entry.edge.id !== undefined ? entry.edge.id : "")
    }
}

function routeSegmentDescriptors(entry, laneBucketSize, minSegmentLength) {
    var route = entry ? entry.route : null
    var points = routePoints(route)
    var descriptors = []
    for (var segmentIndex = 1; segmentIndex < points.length - 2; ++segmentIndex) {
        var descriptor = routeSegmentDescriptor(entry, segmentIndex, laneBucketSize, minSegmentLength)
        if (descriptor)
            descriptors.push(descriptor)
    }
    return descriptors
}

function cloneRouteWithSegmentOffset(route, segmentIndex, offset) {
    if (!route || Math.abs(offset) < 0.1)
        return route

    var points = routePoints(route)
    if (points.length < 4 || segmentIndex <= 0 || segmentIndex >= points.length - 2)
        return route

    var nextPoints = points.slice(0)
    var p1 = points[segmentIndex]
    var p2 = points[segmentIndex + 1]

    if (Math.abs(p1.x - p2.x) < 0.5) {
        nextPoints[segmentIndex] = makePoint(p1.x + offset, p1.y)
        nextPoints[segmentIndex + 1] = makePoint(p2.x + offset, p2.y)
        return {
            "p0": nextPoints[0],
            "p1": nextPoints.length > 2 ? nextPoints[1] : nextPoints[0],
            "p2": nextPoints.length > 2 ? nextPoints[nextPoints.length - 2] : nextPoints[1],
            "p3": nextPoints[nextPoints.length - 1],
            "points": nextPoints,
            "style": route.style
        }
    }

    if (Math.abs(p1.y - p2.y) < 0.5) {
        nextPoints[segmentIndex] = makePoint(p1.x, p1.y + offset)
        nextPoints[segmentIndex + 1] = makePoint(p2.x, p2.y + offset)
        return {
            "p0": nextPoints[0],
            "p1": nextPoints.length > 2 ? nextPoints[1] : nextPoints[0],
            "p2": nextPoints.length > 2 ? nextPoints[nextPoints.length - 2] : nextPoints[1],
            "p3": nextPoints[nextPoints.length - 1],
            "points": nextPoints,
            "style": route.style
        }
    }

    return route
}

function routeStatsAreNoWorse(newStats, oldStats) {
    if (!newStats || !oldStats)
        return true
    if (newStats.hitCount > oldStats.hitCount)
        return false
    return newStats.hitLength <= oldStats.hitLength + 18
}

function applySpreadCluster(cluster, config) {
    if (!cluster || cluster.length < 2)
        return

    cluster.sort(function(left, right) {
        if (Math.abs(left.center - right.center) > 0.5)
            return left.center - right.center
        return left.edgeId.localeCompare(right.edgeId)
    })

    var laneStep = config.laneStep || 14
    var maxOffset = config.maxOffset || 42
    var centerIndex = (cluster.length - 1) / 2

    for (var index = 0; index < cluster.length; ++index) {
        var descriptor = cluster[index]
        var offset = (index - centerIndex) * laneStep
        offset = Math.max(-maxOffset, Math.min(maxOffset, offset))
        if (Math.abs(offset) < 0.1)
            continue

        var nextRoute = cloneRouteWithSegmentOffset(descriptor.entry.route, descriptor.segmentIndex, offset)
        if (config.routeObjectHitStats) {
            var oldStats = config.routeObjectHitStats(descriptor.entry.route, descriptor.entry.edge)
            var newStats = config.routeObjectHitStats(nextRoute, descriptor.entry.edge)
            if (!routeStatsAreNoWorse(newStats, oldStats))
                continue
        }

        if (config.routeObjectAnyNodeHitStats) {
            var oldAnyNodeStats = config.routeObjectAnyNodeHitStats(descriptor.entry.route, descriptor.entry.edge)
            var newAnyNodeStats = config.routeObjectAnyNodeHitStats(nextRoute, descriptor.entry.edge)
            if (newAnyNodeStats.hitCount > oldAnyNodeStats.hitCount
                    || newAnyNodeStats.hitLength > oldAnyNodeStats.hitLength + 1) {
                continue
            }
        }

        descriptor.entry.route = nextRoute
    }
}

function spreadOverlappingRouteMiddles(entries, config) {
    config = config || {}
    var laneBucketSize = config.laneBucketSize || 8
    var minSegmentLength = config.minSegmentLength || 36
    var overlapPadding = config.overlapPadding || 18
    var groups = {}

    for (var index = 0; index < entries.length; ++index) {
        var descriptors = routeSegmentDescriptors(entries[index], laneBucketSize, minSegmentLength)
        for (var descriptorIndex = 0; descriptorIndex < descriptors.length; ++descriptorIndex) {
            var descriptor = descriptors[descriptorIndex]
            var key = descriptor.orientation + ":" + descriptor.laneKey
            if (!groups[key])
                groups[key] = []
            groups[key].push(descriptor)
        }
    }

    for (var groupKey in groups) {
        var members = groups[groupKey]
        members.sort(function(left, right) {
            if (Math.abs(left.start - right.start) > 0.5)
                return left.start - right.start
            if (Math.abs(left.end - right.end) > 0.5)
                return left.end - right.end
            return left.edgeId.localeCompare(right.edgeId)
        })

        var cluster = []
        var clusterEnd = -999999
        for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
            var member = members[memberIndex]
            if (cluster.length === 0 || member.start <= clusterEnd + overlapPadding) {
                cluster.push(member)
                clusterEnd = Math.max(clusterEnd, member.end)
                continue
            }

            applySpreadCluster(cluster, config)
            cluster = [member]
            clusterEnd = member.end
        }

        applySpreadCluster(cluster, config)
    }

    return entries
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

function nodeCenter(rect) {
    return makePoint(rect.x + rect.width / 2, rect.y + rect.height / 2)
}

function portPoint(rect, side, lane) {
    if (side === "right")
        return makePoint(rect.x + rect.width, rect.y + rect.height / 2 + lane)
    if (side === "left")
        return makePoint(rect.x, rect.y + rect.height / 2 + lane)
    if (side === "bottom")
        return makePoint(rect.x + rect.width / 2 + lane, rect.y + rect.height)
    return makePoint(rect.x + rect.width / 2 + lane, rect.y)
}

function overviewEdgeBaseColor(edge) {
    var kind = String(edge && edge.kind ? edge.kind : "").toLowerCase()
    if (kind === "activates")
        return Qt.rgba(0.20, 0.68, 0.90, 0.92)
    if (kind === "uses_infrastructure")
        return Qt.rgba(0.92, 0.62, 0.18, 0.88)
    if (kind === "enables")
        return Qt.rgba(0.00, 0.48, 1.00, 0.92)
    return Qt.rgba(0.42, 0.42, 1.00, 0.9)
}

function edgeColor(config) {
    var edge = config.edge
    if (!config.componentMode) {
        var hoverRelation = overviewHoverRelation(config.hoverNode, edge)
        if (hoverRelation === "incoming")
            return config.tokens.signalRaspberry
        if (hoverRelation === "outgoing")
            return config.tokens.signalCobalt
        if (config.isOverviewPrimaryEdge(edge))
            return overviewEdgeBaseColor(edge)
        return mixColor(Qt.rgba(0.33, 0.43, 0.56, 0.62), overviewEdgeBaseColor(edge), 0.32)
    }

    if (config.focusNode && edge.toId === config.focusNode.id)
        return config.tokens.signalRaspberry
    return config.tokens.signalCobalt
}

function edgeBundleColor(baseColor, relativeIndex, count) {
    if (!baseColor || count <= 1)
        return baseColor

    var halfSpan = Math.max(1, (count - 1) / 2)
    var normalized = Math.max(-1, Math.min(1, relativeIndex / halfSpan))
    var coolAccent = Qt.rgba(0.12, 0.72, 1.0, baseColor.a)
    var warmAccent = Qt.rgba(0.62, 0.42, 1.0, baseColor.a)
    var accent = normalized >= 0 ? coolAccent : warmAccent
    var accentMix = 0.08 + Math.abs(normalized) * 0.16
    var toned = mixColor(baseColor, accent, accentMix)
    return normalized >= 0
            ? mixColor(toned, Qt.rgba(1, 1, 1, baseColor.a), 0.05)
            : mixColor(toned, Qt.rgba(0.10, 0.18, 0.34, baseColor.a), 0.06)
}

function edgeVisuals(config) {
    var lineOpacity = config.componentMode
            ? (config.hasFocus ? (config.emphasized ? 0.96 : 0.34) : 0.7)
            : (config.overviewPrimary
               ? (config.hasHover
                  ? (config.overviewRelation === "incoming" || config.overviewRelation === "outgoing" ? 0.94 : 0.22)
                  : 0.84)
               : (config.emphasized ? 0.82 : 0.22))
    var haloOpacity = config.componentMode
            ? (config.hasFocus ? (config.emphasized ? 0.24 : 0.09) : 0.16)
            : (config.overviewPrimary
               ? (config.hasHover ? 0.1 : 0.13)
               : (config.emphasized ? 0.1 : 0.04))

    return {
        "lineOpacity": lineOpacity,
        "haloOpacity": haloOpacity,
        "haloColor": config.componentMode
                ? Qt.rgba(1, 1, 1, haloOpacity)
                : Qt.rgba(config.lineColor.r, config.lineColor.g, config.lineColor.b, haloOpacity * 0.42),
        "haloStrokeWidth": config.componentMode
                ? (config.hasFocus ? 5.4 : 4.2)
                : (config.overviewPrimary ? 4.0 : 3.0),
        "lineStrokeWidth": config.componentMode
                ? (config.hasFocus ? 2.6 : 2.0)
                : (config.overviewPrimary ? 2.2 : 1.6),
        "z": config.overviewPrimary ? 1 : (config.emphasized ? 2 : 0)
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

function segmentHitRectLength(startPoint, endPoint, rect) {
    var minX = Math.min(startPoint.x, endPoint.x)
    var maxX = Math.max(startPoint.x, endPoint.x)
    var minY = Math.min(startPoint.y, endPoint.y)
    var maxY = Math.max(startPoint.y, endPoint.y)

    if (Math.abs(startPoint.y - endPoint.y) < 0.5) {
        if (startPoint.y <= rect.y || startPoint.y >= rect.y + rect.height)
            return 0
        return Math.max(0, Math.min(maxX, rect.x + rect.width) - Math.max(minX, rect.x))
    }

    if (Math.abs(startPoint.x - endPoint.x) < 0.5) {
        if (startPoint.x <= rect.x || startPoint.x >= rect.x + rect.width)
            return 0
        return Math.max(0, Math.min(maxY, rect.y + rect.height) - Math.max(minY, rect.y))
    }

    return 0
}

function routeHitStats(points, edge, nodes, margin, nodeRectProvider, includeEdgeEndpoints) {
    var stats = {
        "hitCount": 0,
        "hitLength": 0
    }

    for (var segmentIndex = 0; segmentIndex < points.length - 1; ++segmentIndex) {
        for (var nodeIndex = 0; nodeIndex < nodes.length; ++nodeIndex) {
            var node = nodes[nodeIndex]
            if (!includeEdgeEndpoints && (node.id === edge.fromId || node.id === edge.toId))
                continue

            var rect = nodeRectProvider(nodeIndex, margin)
            if (!segmentHitsRect(points[segmentIndex], points[segmentIndex + 1], rect))
                continue

            stats.hitCount += 1
            stats.hitLength += segmentHitRectLength(points[segmentIndex], points[segmentIndex + 1], rect)
        }
    }

    return stats
}

function routeHitsNodes(points, edge, nodes, margin, nodeRectProvider) {
    return routeHitStats(points, edge, nodes, margin, nodeRectProvider, false).hitCount > 0
}

function routeObjectHitsNodes(route, edge, nodes, margin, nodeRectProvider) {
    if (!route)
        return false
    return routeHitsNodes(routePoints(route), edge, nodes, margin, nodeRectProvider)
}

function routeObjectHitStats(route, edge, nodes, margin, nodeRectProvider) {
    if (!route)
        return {"hitCount": 999999, "hitLength": 999999}
    return routeHitStats(routePoints(route), edge, nodes, margin, nodeRectProvider, false)
}

function routeObjectAnyNodeHitStats(route, edge, nodes, margin, nodeRectProvider) {
    if (!route)
        return {"hitCount": 999999, "hitLength": 999999}
    return routeHitStats(routePoints(route), edge, nodes, margin, nodeRectProvider, true)
}

function verticalLane(config) {
    var fromRect = config.fromRect
    var toRect = config.toRect
    var laneIndex = config.laneIndex
    var nodes = config.nodes || []
    var nodeRectProvider = config.nodeRectProvider
    var sceneWidth = config.sceneWidth

    var minY = Math.min(fromRect.y, toRect.y) - 24
    var maxY = Math.max(fromRect.y + fromRect.height, toRect.y + toRect.height) + 24
    var minLeft = Math.min(fromRect.x, toRect.x)
    var maxRight = Math.max(fromRect.x + fromRect.width, toRect.x + toRect.width)

    for (var index = 0; index < nodes.length; ++index) {
        var rect = nodeRectProvider(index, 18)
        if (!rectIntersectsY(rect, minY, maxY))
            continue

        minLeft = Math.min(minLeft, rect.x)
        maxRight = Math.max(maxRight, rect.x + rect.width)
    }

    var gap = 46 + Math.abs(edgeLane(laneIndex)) * 0.7
    var leftX = minLeft - gap
    var rightX = maxRight + gap
    var leftClear = leftX > 16
    var rightClear = rightX < sceneWidth - 16
    var fromCenterX = fromRect.x + fromRect.width / 2
    var toCenterX = toRect.x + toRect.width / 2
    var leftCost = Math.abs(fromRect.x - leftX) + Math.abs(toRect.x - leftX)
    var rightCost = Math.abs(fromRect.x + fromRect.width - rightX) + Math.abs(toRect.x + toRect.width - rightX)

    if (rightClear && (!leftClear || rightCost <= leftCost || toCenterX >= fromCenterX))
        return {"side": "right", "value": rightX}
    if (leftClear)
        return {"side": "left", "value": leftX}

    return rightCost <= leftCost
            ? {"side": "right", "value": Math.max(maxRight + 24, sceneWidth - 18)}
            : {"side": "left", "value": Math.min(minLeft - 24, 18)}
}

function horizontalLane(config) {
    var fromRect = config.fromRect
    var toRect = config.toRect
    var laneIndex = config.laneIndex
    var nodes = config.nodes || []
    var nodeRectProvider = config.nodeRectProvider
    var sceneHeight = config.sceneHeight

    var minX = Math.min(fromRect.x, toRect.x) - 24
    var maxX = Math.max(fromRect.x + fromRect.width, toRect.x + toRect.width) + 24
    var minTop = Math.min(fromRect.y, toRect.y)
    var maxBottom = Math.max(fromRect.y + fromRect.height, toRect.y + toRect.height)

    for (var index = 0; index < nodes.length; ++index) {
        var rect = nodeRectProvider(index, 18)
        if (!rectIntersectsX(rect, minX, maxX))
            continue

        minTop = Math.min(minTop, rect.y)
        maxBottom = Math.max(maxBottom, rect.y + rect.height)
    }

    var gap = 42 + Math.abs(edgeLane(laneIndex)) * 0.7
    var topY = minTop - gap
    var bottomY = maxBottom + gap
    var topClear = topY > 16
    var bottomClear = bottomY < sceneHeight - 16
    var topCost = Math.abs(fromRect.y - topY) + Math.abs(toRect.y - topY)
    var bottomCost = Math.abs(fromRect.y + fromRect.height - bottomY) + Math.abs(toRect.y + toRect.height - bottomY)

    if (bottomClear && (!topClear || bottomCost <= topCost))
        return {"side": "bottom", "value": bottomY}
    if (topClear)
        return {"side": "top", "value": topY}

    return bottomCost <= topCost
            ? {"side": "bottom", "value": Math.max(maxBottom + 24, sceneHeight - 18)}
            : {"side": "top", "value": Math.min(minTop - 24, 18)}
}

function directRoute(config) {
    var fromRect = config.fromRect
    var toRect = config.toRect
    var lane = edgeLane(config.laneIndex)
    var vertical = config.vertical
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
        return [startPoint, makePoint(startPoint.x, midY), makePoint(endPoint.x, midY), endPoint]
    }

    var sourceSide = toCenterX >= fromCenterX ? "right" : "left"
    var targetSide = toCenterX >= fromCenterX ? "left" : "right"
    var start = portPoint(fromRect, sourceSide, lane)
    var end = portPoint(toRect, targetSide, -lane)
    var midX = (start.x + end.x) / 2
    return [start, makePoint(midX, start.y), makePoint(midX, end.y), end]
}

function bypassRoute(config) {
    var fromRect = config.fromRect
    var toRect = config.toRect
    var laneIndex = config.laneIndex
    var lane = edgeLane(laneIndex)
    var vertical = config.vertical

    if (vertical) {
        var xLane = verticalLane(config)
        var startPoint = portPoint(fromRect, xLane.side, lane)
        var endPoint = portPoint(toRect, xLane.side, -lane)
        return [startPoint, makePoint(xLane.value, startPoint.y), makePoint(xLane.value, endPoint.y), endPoint]
    }

    var yLane = horizontalLane(config)
    var start = portPoint(fromRect, yLane.side, lane)
    var end = portPoint(toRect, yLane.side, -lane)
    return [start, makePoint(start.x, yLane.value), makePoint(end.x, yLane.value), end]
}
