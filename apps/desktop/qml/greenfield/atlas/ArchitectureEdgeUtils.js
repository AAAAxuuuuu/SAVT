.pragma library

var READABLE_DEBUG_TAG = "readable-single-layer-highlight-v8-large-readable-arrows"
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
    var id = edge.id !== undefined ? String(edge.id) : "?"
    return "id=" + id + " " + fromName + " -> " + toName + " fromId=" + String(edge.fromId) + " toId=" + String(edge.toId)
}

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
            for (var edgeIndex = 0; edgeIndex < endpointMembers.length; ++edgeIndex) {
                output[key][endpointMembers[edgeIndex].edgeId] = {
                    "relative": edgeIndex - (endpointMembers.length - 1) / 2,
                    "count": endpointMembers.length
                }
            }
        }
    }

    return output
}

function endpointPeerOffset(offsets, edge, endpointKey, step) {
    if (!offsets || !edge || !endpointKey)
        return 0

    var endpointOffsets = offsets[endpointKey] || ({})
    var placement = endpointOffsets[String(edge.id)]
    if (placement === undefined)
        return 0

    if (typeof placement === "number")
        return placement * step

    var count = Math.max(1, Number(placement.count) || 1)
    var relative = Number(placement.relative)
    var virtualSpan = step * (4.4 + Math.max(0, count - 1) * 1.9)
    return distributedSideOffset(relative, count, virtualSpan, step * 0.6, 0.92)
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


function readableEdgeKey(edge) {
    return String(edge && edge.id !== undefined ? edge.id : "")
}

function readableNodeKey(value) {
    return String(value)
}

function cloneReadableEdge(edge) {
    var output = ({})
    for (var key in edge)
        output[key] = edge[key]
    return output
}

function mergeBidirectionalEdges(edgeEntries) {
    readableDebug("EDGE-NORMALIZE", "merge-start rawEntries=" + String(edgeEntries ? edgeEntries.length : 0))
    var output = []
    var pendingByPair = ({})
    var usedByEdgeId = ({})

    function pairKey(fromId, toId) {
        var left = readableNodeKey(fromId)
        var right = readableNodeKey(toId)
        return left < right ? left + "<>" + right : right + "<>" + left
    }

    function directionKey(fromId, toId) {
        return readableNodeKey(fromId) + "->" + readableNodeKey(toId)
    }

    for (var index = 0; index < edgeEntries.length; ++index) {
        var entry = edgeEntries[index]
        var edge = entry ? entry.edge : null
        if (!edge)
            continue

        var edgeId = readableEdgeKey(edge)
        if (usedByEdgeId[edgeId]) {
            readableDebug("EDGE-NORMALIZE", "skip-used " + readableEdgeLabel(edge))
            continue
        }

        var key = pairKey(edge.fromId, edge.toId)
        var dir = directionKey(edge.fromId, edge.toId)
        var reverseDir = directionKey(edge.toId, edge.fromId)
        var bucket = pendingByPair[key]

        if (bucket && bucket.byDirection && bucket.byDirection[reverseDir]) {
            readableDebug("EDGE-NORMALIZE", "bidirectional-merge current=" + readableEdgeLabel(edge) + " reverseDir=" + reverseDir)
            var reverseEntry = bucket.byDirection[reverseDir]
            var reverseEdge = reverseEntry.edge
            var mergedEdge = cloneReadableEdge(reverseEdge)
            mergedEdge.id = "bi:" + readableNodeKey(reverseEdge.fromId) + "<->" + readableNodeKey(reverseEdge.toId)
            mergedEdge.bidirectional = true
            mergedEdge.reverseEdgeId = edgeId
            mergedEdge.originalEdgeIds = [readableEdgeKey(reverseEdge), edgeId]
            mergedEdge.fromName = reverseEdge.fromName || reverseEdge.fromLabel || reverseEdge.fromId
            mergedEdge.toName = reverseEdge.toName || reverseEdge.toLabel || reverseEdge.toId
            usedByEdgeId[readableEdgeKey(reverseEdge)] = true
            usedByEdgeId[edgeId] = true
            output[reverseEntry.outputIndex] = {
                "edge": mergedEdge,
                "laneIndex": reverseEntry.outputIndex
            }
            continue
        }

        readableDebug("EDGE-NORMALIZE", "single-direction " + readableEdgeLabel(edge) + " pairKey=" + key + " dir=" + dir)
        var clonedEdge = cloneReadableEdge(edge)
        clonedEdge.bidirectional = false
        var outputIndex = output.length
        output.push({
            "edge": clonedEdge,
            "laneIndex": outputIndex
        })

        if (!bucket) {
            bucket = { "byDirection": ({}) }
            pendingByPair[key] = bucket
        }
        bucket.byDirection[dir] = {
            "edge": clonedEdge,
            "outputIndex": outputIndex
        }
    }

    var compact = []
    for (var compactIndex = 0; compactIndex < output.length; ++compactIndex) {
        if (!output[compactIndex] || !output[compactIndex].edge)
            continue
        output[compactIndex].laneIndex = compact.length
        compact.push(output[compactIndex])
    }
    readableDebug("EDGE-NORMALIZE", "merge-complete output=" + String(compact.length))
    for (var ci = 0; ci < compact.length; ++ci) {
        var ce = compact[ci] && compact[ci].edge
        readableDebug("EDGE-NORMALIZE", "merged[" + ci + "] bidirectional=" + String(!!(ce && ce.bidirectional)) + " originalEdgeIds=" + String(ce && ce.originalEdgeIds ? ce.originalEdgeIds.join(",") : "") + " " + readableEdgeLabel(ce))
    }
    return compact
}

function collectVisibleEdges(config) {
    readableDebug("EDGE-COLLECT", "start componentMode=" + String(!!config.componentMode) + " showAll=" + String(!!config.showAll) + " relationshipFocusActive=" + String(!!config.relationshipFocusActive) + " edgeList=" + String(config.edgeList ? config.edgeList.length : 0) + " selected=" + String(config.selectedNode && config.selectedNode.id !== undefined ? config.selectedNode.id : "") + " hovered=" + String(config.hoveredNode && config.hoveredNode.id !== undefined ? config.hoveredNode.id : ""))
    var rawOutput = []
    var edgeList = config.edgeList || []
    if (config.relationshipFocusActive)
        return rawOutput

    if (!config.componentMode) {
        var overviewNode = !config.showAll
                && config.hoveredNode && config.hoveredNode.id !== undefined
                ? config.hoveredNode
                : null
        var overviewLimit = Math.min(config.overviewEdgeLimit || 18, 18)

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
            var maxOutgoingPerNode = 4
            var maxIncomingPerNode = 4

            for (var pickIndex = 0; pickIndex < candidates.length && rawOutput.length < overviewLimit; ++pickIndex) {
                var pickedEdge = candidates[pickIndex].edge
                var fromKey = String(pickedEdge.fromId)
                var toKey = String(pickedEdge.toId)
                if ((outgoingCounts[fromKey] || 0) >= maxOutgoingPerNode)
                    continue
                if ((incomingCounts[toKey] || 0) >= maxIncomingPerNode)
                    continue

                outgoingCounts[fromKey] = (outgoingCounts[fromKey] || 0) + 1
                incomingCounts[toKey] = (incomingCounts[toKey] || 0) + 1
                rawOutput.push({
                    "edge": pickedEdge,
                    "laneIndex": rawOutput.length
                })
            }

            if (rawOutput.length === 0 && candidates.length > 0) {
                for (var fallbackIndex = 0; fallbackIndex < candidates.length && rawOutput.length < overviewLimit; ++fallbackIndex) {
                    rawOutput.push({
                        "edge": candidates[fallbackIndex].edge,
                        "laneIndex": rawOutput.length
                    })
                }
            }

            return mergeBidirectionalEdges(rawOutput)
        }

        for (var overviewIndex = 0; overviewIndex < edgeList.length; ++overviewIndex) {
            var primaryEdge = edgeList[overviewIndex]
            if (!config.isOverviewPrimaryEdge(primaryEdge))
                continue
            if (overviewNode && !(primaryEdge.fromId === overviewNode.id || primaryEdge.toId === overviewNode.id))
                continue

            rawOutput.push({
                "edge": primaryEdge,
                "laneIndex": rawOutput.length
            })
        }
        readableDebug("EDGE-COLLECT", "overview-rawOutput beforeMerge=" + String(rawOutput.length))
        for (var ro = 0; ro < rawOutput.length; ++ro)
            readableDebug("EDGE-COLLECT", "overview-raw[" + ro + "] lane=" + String(rawOutput[ro].laneIndex) + " " + readableEdgeLabel(rawOutput[ro].edge))
        return mergeBidirectionalEdges(rawOutput)
    }

    if (!config.showAll)
        return rawOutput

    var node = config.selectedNode || config.hoveredNode
    var limit = node ? config.focusEdgeLimit : config.overviewEdgeLimit
    for (var index = 0; index < edgeList.length && rawOutput.length < limit; ++index) {
        var edge = edgeList[index]
        if (node && !(edge.fromId === node.id || edge.toId === node.id))
            continue

        rawOutput.push({
            "edge": edge,
            "laneIndex": rawOutput.length
        })
    }
    readableDebug("EDGE-COLLECT", "component-rawOutput beforeMerge=" + String(rawOutput.length))
    for (var cr = 0; cr < rawOutput.length; ++cr)
        readableDebug("EDGE-COLLECT", "component-raw[" + cr + "] lane=" + String(rawOutput[cr].laneIndex) + " " + readableEdgeLabel(rawOutput[cr].edge))
    return mergeBidirectionalEdges(rawOutput)
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

function clampNumber(value, minimumValue, maximumValue) {
    return Math.max(minimumValue, Math.min(maximumValue, value))
}

function distributedSideOffset(relativeIndex, count, span, minimumInset, coverageRatio) {
    var numericRelative = Number(relativeIndex)
    var numericCount = Math.max(1, Number(count) || 1)
    var numericSpan = Math.max(1, Number(span) || 0)
    if (!isFinite(numericRelative) || numericCount <= 1 || !isFinite(numericSpan))
        return 0

    var safeInset = minimumInset === undefined ? 18 : Math.max(0, Number(minimumInset))
    var ratio = coverageRatio === undefined ? 0.72 : clampNumber(Number(coverageRatio), 0.24, 0.95)
    var halfCount = Math.max(1, (numericCount - 1) / 2)
    var normalized = clampNumber(numericRelative / halfCount, -1, 1)
    var easedMagnitude = Math.pow(Math.abs(normalized), 0.92)
    var eased = normalized < 0 ? -easedMagnitude : easedMagnitude
    var availableSpan = Math.max(18, Math.min(numericSpan * ratio, numericSpan - safeInset * 2))
    var desiredHalfSpread = 12 + Math.max(0, numericCount - 1) * 10
    var halfSpread = Math.max(10, Math.min(availableSpan / 2, desiredHalfSpread))
    return eased * halfSpread
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

function colorWithAlpha(color, alpha) {
    if (!color)
        return Qt.rgba(0.636, 0.636, 0.667, alpha)
    return Qt.rgba(color.r, color.g, color.b, alpha)
}

function stringHash(text) {
    var value = String(text || "")
    var hash = 0
    for (var index = 0; index < value.length; ++index)
        hash = ((hash * 131) + value.charCodeAt(index)) % 2147483647
    return Math.abs(hash)
}

function edgeIdentitySeed(edge) {
    return String(edge && edge.fromId !== undefined ? edge.fromId : "")
        + "->"
        + String(edge && edge.toId !== undefined ? edge.toId : "")
        + "#"
        + String(edge && edge.kind ? edge.kind : "")
        + "#"
        + String(edge && edge.id !== undefined ? edge.id : "")
}

function edgeIdentityAccentColor(edge, alpha) {
    var resolvedAlpha = alpha === undefined ? 1.0 : alpha
    var palette = [
        Qt.rgba(0.020, 0.540, 0.920, resolvedAlpha),
        Qt.rgba(0.000, 0.670, 0.580, resolvedAlpha),
        Qt.rgba(0.100, 0.620, 0.430, resolvedAlpha),
        Qt.rgba(0.920, 0.580, 0.180, resolvedAlpha),
        Qt.rgba(0.900, 0.380, 0.260, resolvedAlpha),
        Qt.rgba(0.180, 0.640, 0.860, resolvedAlpha)
    ]
    return palette[stringHash(edgeIdentitySeed(edge)) % palette.length]
}

function applyEdgeIdentityTone(baseColor, edge, amount) {
    if (!baseColor)
        return baseColor

    var mixAmount = amount === undefined ? 0.12 : clampNumber(amount, 0, 0.38)
    var accent = edgeIdentityAccentColor(edge, baseColor.a)
    var toned = mixColor(baseColor, accent, mixAmount)
    var variant = stringHash(edgeIdentitySeed(edge) + ":variant") % 5
    if (variant === 1)
        return mixColor(toned, Qt.rgba(1, 1, 1, baseColor.a), 0.035)
    if (variant >= 3)
        return mixColor(toned, Qt.rgba(0.08, 0.12, 0.18, baseColor.a), 0.03)
    return toned
}

function edgeSemanticColor(tokens, edge, alpha) {
    var kind = String(edge && edge.kind ? edge.kind : "").toLowerCase()
    var resolvedAlpha = alpha === undefined ? 0.82 : alpha

    if (kind === "activates")
        return colorWithAlpha(tokens ? tokens.signalCobalt : Qt.rgba(0.0, 0.478, 1.0, 1.0), resolvedAlpha)
    if (kind === "uses_infrastructure" || kind === "uses_support")
        return Qt.rgba(1.0, 0.455, 0.075, resolvedAlpha)
    if (kind === "enables" || kind === "coordinates" || kind === "depends_on")
        return Qt.rgba(0.080, 0.410, 0.800, resolvedAlpha)
    return Qt.rgba(0.210, 0.385, 0.630, resolvedAlpha)
}

function overviewEdgeBaseColor(edge, tokens) {
    var kind = String(edge && edge.kind ? edge.kind : "").toLowerCase()
    var baseColor = null
    if (kind === "activates")
        baseColor = edgeSemanticColor(tokens, edge, 0.94)
    else if (kind === "uses_infrastructure" || kind === "uses_support")
        baseColor = edgeSemanticColor(tokens, edge, 0.92)
    else if (kind === "enables" || kind === "coordinates" || kind === "depends_on")
        baseColor = edgeSemanticColor(tokens, edge, 0.90)
    else
        baseColor = edgeSemanticColor(tokens, edge, 0.88)

    var toneAmount = kind === "uses_infrastructure" || kind === "uses_support" ? 0.10 : 0.16
    return applyEdgeIdentityTone(baseColor, edge, toneAmount)
}

function edgeColor(config) {
    var edge = config.edge
    if (!config.componentMode) {
        var hoverRelation = overviewHoverRelation(config.hoverNode, edge)
        if (hoverRelation === "incoming" || hoverRelation === "outgoing")
            return colorWithAlpha(config.tokens ? config.tokens.signalCobalt : Qt.rgba(0.0, 0.478, 1.0, 1.0), 0.96)
        if (config.isOverviewPrimaryEdge(edge))
            return overviewEdgeBaseColor(edge, config.tokens)
        return mixColor(Qt.rgba(0.455, 0.565, 0.690, 0.50), overviewEdgeBaseColor(edge, config.tokens), 0.30)
    }

    if (config.focusNode && edgeTouchesFocus(config.focusNode, edge))
        return applyEdgeIdentityTone(edgeSemanticColor(config.tokens, edge, 0.92), edge, 0.08)
    return applyEdgeIdentityTone(edgeSemanticColor(config.tokens, edge, 0.72), edge, 0.05)
}

function edgeBundleColor(baseColor, relativeIndex, count) {
    if (!baseColor || count <= 1)
        return baseColor

    var halfSpan = Math.max(1, (count - 1) / 2)
    var normalized = Math.max(-1, Math.min(1, relativeIndex / halfSpan))
    var coolAccent = Qt.rgba(0.0, 0.478, 1.0, baseColor.a)
    var warmAccent = Qt.rgba(0.910, 0.430, 0.210, baseColor.a)
    var accent = normalized >= 0 ? coolAccent : warmAccent
    var accentMix = 0.02 + Math.abs(normalized) * 0.045
    var toned = mixColor(baseColor, accent, accentMix)
    return normalized >= 0
            ? mixColor(toned, Qt.rgba(1, 1, 1, baseColor.a), 0.015)
            : mixColor(toned, Qt.rgba(0.10, 0.18, 0.34, baseColor.a), 0.02)
}

function edgeVisuals(config) {
    var defaultFocusActive = !!config.defaultFocusActive
    var lineOpacity = config.componentMode
            ? (config.hasFocus ? (config.emphasized ? 0.96 : 0.34) : 0.72)
            : (config.hasHover
               ? (config.emphasized ? 0.96 : 0.16)
               : (defaultFocusActive
                  ? (config.emphasized ? 0.96 : 0.34)
                  : (config.emphasized ? 0.94 : 0.58)))
    var haloOpacity = config.componentMode
            ? (config.hasFocus ? (config.emphasized ? 0.24 : 0.09) : 0.16)
            : (config.hasHover
               ? (config.emphasized ? 0.22 : 0.04)
               : (defaultFocusActive
                  ? (config.emphasized ? 0.24 : 0.08)
                  : (config.emphasized ? 0.20 : 0.10)))
    var isStrongOverview = !config.componentMode && (config.emphasized || config.overviewPrimary)

    return {
        "lineOpacity": lineOpacity,
        "haloOpacity": haloOpacity,
        "haloColor": config.componentMode
                ? Qt.rgba(1, 1, 1, haloOpacity)
                : Qt.rgba(config.lineColor.r, config.lineColor.g, config.lineColor.b, haloOpacity * 0.55),
        "haloStrokeWidth": config.componentMode
                ? (config.hasFocus ? 5.4 : 4.2)
                : (isStrongOverview ? 4.2 : 2.8),
        "lineStrokeWidth": config.componentMode
                ? (config.hasFocus ? 2.6 : 2.0)
                : (isStrongOverview ? 2.35 : 1.45),
        "z": config.emphasized ? 4 : (config.overviewPrimary ? 2 : 0)
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
