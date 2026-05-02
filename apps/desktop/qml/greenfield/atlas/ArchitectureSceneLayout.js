.pragma library

var READABLE_DEBUG_TAG = "readable-component-detail-v13-isolated-grid"
var READABLE_DEBUG_ENABLED = true

function readableDebug(prefix, message) {
    if (!READABLE_DEBUG_ENABLED)
        return
    console.log("[READABLE-" + String(prefix) + "]", READABLE_DEBUG_TAG, String(message))
}

function readableNodeLabel(node, fallbackId) {
    if (!node)
        return String(fallbackId)
    return String(node.name || node.label || node.title || fallbackId || node.id || "?")
}

function readablePositionText(pos) {
    if (!pos)
        return "<no-pos>"
    return "x=" + Math.round(Number(pos.x) || 0) + " y=" + Math.round(Number(pos.y) || 0) + " w=" + Math.round(Number(pos.width) || 0) + " h=" + Math.round(Number(pos.height) || 0) + " lane=" + String(pos.laneIndex) + " order=" + String(pos.order)
}

function emptyOverviewLayout() {
    return {
        "positions": ({}),
        "primaryEdgeIds": ({}),
        "secondaryEdgeIds": ({}),
        "outgoingSlotByEdgeId": ({}),
        "incomingSlotByEdgeId": ({}),
        "outgoingStemSlotByEdgeId": ({}),
        "incomingStemSlotByEdgeId": ({}),
        "outgoingCountByNodeId": ({}),
        "incomingCountByNodeId": ({}),
        "outgoingBusXByNodeId": ({}),
        "incomingBusXByNodeId": ({}),
        "fanOutBundleByEdgeId": ({}),
        "fanInBundleByEdgeId": ({}),
        "routeTrackByEdgeId": ({}),
        "routeTrackGroupKeyByEdgeId": ({}),
        "routeTrackCountByGroupKey": ({}),
        "layerById": ({}),
        "layerCount": 0,
        "rootId": undefined,
        "leftIds": [],
        "rightIds": [],
        "width": 980,
        "height": 560
    }
}

function emptyComponentLayout() {
    return {
        "groups": [],
        "positions": ({}),
        "sceneWidth": 980,
        "sceneHeight": 560,
        "cardWidth": 420,
        "cardHeight": 300,
        "gapX": 64,
        "gapY": 42,
        "sectionGap": 116,
        "headerHeight": 34,
        "maxRows": 1,
        "groupIndexByNodeId": ({}),
        "outgoingSlotByEdgeId": ({}),
        "incomingSlotByEdgeId": ({}),
        "outgoingCountByNodeId": ({}),
        "incomingCountByNodeId": ({}),
        "outgoingBusXByNodeId": ({}),
        "incomingBusXByNodeId": ({}),
        "fanOutBundleByEdgeId": ({}),
        "fanInBundleByEdgeId": ({}),
        "routeTrackByEdgeId": ({}),
        "routeTrackGroupKeyByEdgeId": ({}),
        "routeTrackCountByGroupKey": ({}),
        "groupBoundsByIndex": []
    }
}

function numericOrFallback(value, fallbackValue) {
    var numericValue = Number(value)
    return isFinite(numericValue) ? numericValue : fallbackValue
}

function boundedValue(value, minimumValue, maximumValue) {
    return Math.max(minimumValue, Math.min(maximumValue, value))
}

function pushUnique(list, value) {
    if (list.indexOf(value) < 0)
        list.push(value)
}

function safeNodeName(node) {
    return String(node && node.name ? node.name : "")
}

function safeNodeId(value) {
    return String(value)
}

function overviewPeerSortY(positions, peerId, fallbackYById) {
    var position = positions[peerId]
    return position ? position.y : (fallbackYById[peerId] || 0)
}

function overviewPeerSortX(positions, peerId, fallbackXById) {
    var position = positions[peerId]
    return position ? position.x : (fallbackXById[peerId] || 0)
}

function overviewNodeTraffic(incomingById, outgoingById, nodeId) {
    return (incomingById[nodeId] ? incomingById[nodeId].length : 0)
        + (outgoingById[nodeId] ? outgoingById[nodeId].length : 0)
}


function readableName(node) {
    return String(node && node.name ? node.name : "")
}

function readableLowerName(node) {
    return readableName(node).toLowerCase()
}

function readableIs(node, keyword) {
    return readableLowerName(node).indexOf(String(keyword).toLowerCase()) >= 0
}

function readableEdgeKey(edge) {
    return String(edge && edge.id !== undefined ? edge.id : "")
}

function readableSemanticSlot(node, degreeScore) {
    var name = readableLowerName(node)
    if (name.indexOf("apps desktop") >= 0 || name.indexOf("desktop") >= 0)
        return "leftTop"
    if (name === "tmp" || name.indexOf("tmp") >= 0)
        return "leftMiddle"
    if (name.indexOf("output") >= 0)
        return "leftBottom"
    if (name.indexOf("src ui") >= 0 || name.indexOf("ui") >= 0)
        return "centerLeft"
    if (name.indexOf("analyzer") >= 0)
        return "topCenter"
    if (name.indexOf("layout") >= 0)
        return "centerRight"
    if (name.indexOf("parser") >= 0)
        return "bottomLeft"
    if (name.indexOf("core") >= 0)
        return "bottomCenter"
    if (name.indexOf("reconstruction") >= 0)
        return "bottomRight"
    if (name.indexOf("src ai") >= 0 || name.indexOf(" ai") >= 0 || name.indexOf("ai") === 0)
        return "rightTop"
    if (name.indexOf("config") >= 0)
        return "rightBottom"
    return ""
}

function buildReadableOverviewLayout(config, empty) {
    var nodes = config && config.nodes ? config.nodes : []
    var edges = config && config.edges ? config.edges : []
    var bounds = config && config.bounds ? config.bounds : ({})
    var rootNode = config ? config.rootNode : null
    readableDebug("LAYOUT", "readable-overview-start nodes=" + String(nodes.length) + " edges=" + String(edges.length) + " boundsW=" + String(bounds.width || 0) + " boundsH=" + String(bounds.height || 0) + " root=" + String(rootNode && rootNode.id !== undefined ? rootNode.id : ""))
    if (!nodes.length || !rootNode || rootNode.id === undefined) {
        readableDebug("LAYOUT", "readable-overview-empty reason=missing-nodes-or-root")
        return empty
    }

    var positions = {}
    var primaryEdgeIds = {}
    var secondaryEdgeIds = {}
    var outgoingSlotByEdgeId = {}
    var incomingSlotByEdgeId = {}
    var outgoingStemSlotByEdgeId = {}
    var incomingStemSlotByEdgeId = {}
    var outgoingCountByNodeId = {}
    var incomingCountByNodeId = {}
    var outgoingBusXByNodeId = {}
    var incomingBusXByNodeId = {}
    var fanOutBundleByEdgeId = {}
    var fanInBundleByEdgeId = {}
    var routeTrackByEdgeId = {}
    var routeTrackGroupKeyByEdgeId = {}
    var routeTrackCountByGroupKey = {}
    var layerById = {}
    var widthById = {}
    var heightById = {}
    var nodeById = {}
    var degreeById = {}
    var directedDegreeById = {}
    var incomingById = {}
    var outgoingById = {}
    var adjacency = {}
    var nodeIds = []
    var maxNodeW = 0
    var maxNodeH = 0

    for (var nodeIndex = 0; nodeIndex < nodes.length; ++nodeIndex) {
        var node = nodes[nodeIndex]
        if (!node || node.id === undefined)
            continue
        var nodeId = node.id
        nodeIds.push(nodeId)
        nodeById[nodeId] = node
        degreeById[nodeId] = 0
        directedDegreeById[nodeId] = 0
        incomingById[nodeId] = []
        outgoingById[nodeId] = []
        adjacency[nodeId] = []
        widthById[nodeId] = config.nodeWidth ? config.nodeWidth(node) : 560
        heightById[nodeId] = config.nodeHeight ? config.nodeHeight(node) : 330
        maxNodeW = Math.max(maxNodeW, widthById[nodeId])
        maxNodeH = Math.max(maxNodeH, heightById[nodeId])
    }

    readableDebug("LAYOUT", "node-scan complete validNodes=" + String(nodeIds.length) + " maxNodeW=" + String(maxNodeW) + " maxNodeH=" + String(maxNodeH))

    function addAdjacency(a, b) {
        if (a === undefined || b === undefined || a === b)
            return
        if (!adjacency[a])
            adjacency[a] = []
        if (adjacency[a].indexOf(b) < 0)
            adjacency[a].push(b)
    }

    var seenUndirected = {}
    var validEdges = []
    for (var edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
        var edge = edges[edgeIndex]
        if (!edge || edge.id === undefined)
            continue
        if (!nodeById[edge.fromId] || !nodeById[edge.toId] || edge.fromId === edge.toId)
            continue
        validEdges.push(edge)
        primaryEdgeIds[edge.id] = true
        outgoingById[edge.fromId].push(edge)
        incomingById[edge.toId].push(edge)
        directedDegreeById[edge.fromId] = (directedDegreeById[edge.fromId] || 0) + 1
        directedDegreeById[edge.toId] = (directedDegreeById[edge.toId] || 0) + 1
        var a = String(edge.fromId)
        var b = String(edge.toId)
        var pairKey = a < b ? a + "<>" + b : b + "<>" + a
        if (!seenUndirected[pairKey]) {
            seenUndirected[pairKey] = true
            degreeById[edge.fromId] = (degreeById[edge.fromId] || 0) + 1
            degreeById[edge.toId] = (degreeById[edge.toId] || 0) + 1
            addAdjacency(edge.fromId, edge.toId)
            addAdjacency(edge.toId, edge.fromId)
        }
    }

    var connectedIds = []
    var isolatedIds = []
    var defaultFocusId = undefined
    for (var dbgDegreeIndex = 0; dbgDegreeIndex < nodeIds.length; ++dbgDegreeIndex) {
        var dbgDegreeId = nodeIds[dbgDegreeIndex]
        var degree = degreeById[dbgDegreeId] || 0
        if (degree > 0)
            connectedIds.push(dbgDegreeId)
        else
            isolatedIds.push(dbgDegreeId)
        if (degree > 0 && (defaultFocusId === undefined
                || degree > (degreeById[defaultFocusId] || 0)
                || (degree === (degreeById[defaultFocusId] || 0)
                    && (directedDegreeById[dbgDegreeId] || 0) > (directedDegreeById[defaultFocusId] || 0))
                || (degree === (degreeById[defaultFocusId] || 0)
                    && (directedDegreeById[dbgDegreeId] || 0) === (directedDegreeById[defaultFocusId] || 0)
                    && readableName(nodeById[dbgDegreeId]).localeCompare(readableName(nodeById[defaultFocusId])) < 0)))
            defaultFocusId = dbgDegreeId
        readableDebug("LAYOUT", "degree id=" + String(dbgDegreeId) + " name=" + readableNodeLabel(nodeById[dbgDegreeId], dbgDegreeId) + " degree=" + String(degree) + " directed=" + String(directedDegreeById[dbgDegreeId] || 0) + " incoming=" + String((incomingById[dbgDegreeId] || []).length) + " outgoing=" + String((outgoingById[dbgDegreeId] || []).length))
    }

    readableDebug("LAYOUT", "default-focus id=" + String(defaultFocusId !== undefined ? defaultFocusId : "") + " name=" + readableNodeLabel(nodeById[defaultFocusId], defaultFocusId) + " connected=" + String(connectedIds.length) + " isolated=" + String(isolatedIds.length))

    connectedIds.sort(function(leftId, rightId) {
        var degreeDelta = (degreeById[rightId] || 0) - (degreeById[leftId] || 0)
        if (degreeDelta !== 0)
            return degreeDelta
        var directedDelta = (directedDegreeById[rightId] || 0) - (directedDegreeById[leftId] || 0)
        if (directedDelta !== 0)
            return directedDelta
        return readableName(nodeById[leftId]).localeCompare(readableName(nodeById[rightId]))
    })
    isolatedIds.sort(function(leftId, rightId) {
        return readableName(nodeById[leftId]).localeCompare(readableName(nodeById[rightId]))
    })

    var centerXById = {}
    var centerYById = {}
    var distById = {}
    var parentById = {}

    if (defaultFocusId !== undefined) {
        var queue = [defaultFocusId]
        distById[defaultFocusId] = 0
        parentById[defaultFocusId] = undefined
        for (var qi = 0; qi < queue.length; ++qi) {
            var currentId = queue[qi]
            var neighbors = adjacency[currentId] || []
            neighbors.sort(function(leftId, rightId) {
                var d = (degreeById[rightId] || 0) - (degreeById[leftId] || 0)
                if (d !== 0)
                    return d
                return readableName(nodeById[leftId]).localeCompare(readableName(nodeById[rightId]))
            })
            for (var ni = 0; ni < neighbors.length; ++ni) {
                var nextId = neighbors[ni]
                if (distById[nextId] !== undefined)
                    continue
                distById[nextId] = (distById[currentId] || 0) + 1
                parentById[nextId] = currentId
                queue.push(nextId)
            }
        }
    }

    var mainIds = []
    var sideComponentIds = []
    for (var connectedIndex = 0; connectedIndex < connectedIds.length; ++connectedIndex) {
        var connectedId = connectedIds[connectedIndex]
        if (distById[connectedId] === undefined)
            sideComponentIds.push(connectedId)
        else
            mainIds.push(connectedId)
    }

    var baseRadiusX = Math.max(maxNodeW * 1.78, 1180)
    var baseRadiusY = Math.max(maxNodeH * 1.92, 790)
    if (defaultFocusId !== undefined) {
        centerXById[defaultFocusId] = 0
        centerYById[defaultFocusId] = 0
    }

    var groupsByDist = {}
    for (var mainIndex = 0; mainIndex < mainIds.length; ++mainIndex) {
        var mainId = mainIds[mainIndex]
        var dist = distById[mainId] || 0
        if (dist <= 0)
            continue
        var distKey = String(Math.min(3, dist))
        if (!groupsByDist[distKey])
            groupsByDist[distKey] = []
        groupsByDist[distKey].push(mainId)
    }

    function sortLayoutIds(list) {
        list.sort(function(leftId, rightId) {
            var d = (degreeById[rightId] || 0) - (degreeById[leftId] || 0)
            if (d !== 0)
                return d
            var dd = (directedDegreeById[rightId] || 0) - (directedDegreeById[leftId] || 0)
            if (dd !== 0)
                return dd
            return readableName(nodeById[leftId]).localeCompare(readableName(nodeById[rightId]))
        })
    }

    for (var dk in groupsByDist) {
        var group = groupsByDist[dk]
        sortLayoutIds(group)
        var distNumber = Math.max(1, Number(dk) || 1)
        var radiusX = baseRadiusX * (1.18 + (distNumber - 1) * 0.86)
        var radiusY = baseRadiusY * (1.18 + (distNumber - 1) * 0.78)
        var count = group.length
        var startAngle = -Math.PI / 2 - Math.PI / Math.max(6, count * 2)
        for (var gi = 0; gi < count; ++gi) {
            var nodeIdAtAngle = group[gi]
            var angle = startAngle + (2 * Math.PI * gi / Math.max(1, count))
            if (distNumber % 2 === 0)
                angle += Math.PI / Math.max(3, count)
            centerXById[nodeIdAtAngle] = Math.cos(angle) * radiusX
            centerYById[nodeIdAtAngle] = Math.sin(angle) * radiusY
            readableDebug("LAYOUT", "initial-ring id=" + String(nodeIdAtAngle) + " name=" + readableNodeLabel(nodeById[nodeIdAtAngle], nodeIdAtAngle) + " dist=" + String(distById[nodeIdAtAngle]) + " angle=" + String(Math.round(angle * 1000) / 1000) + " cx=" + String(Math.round(centerXById[nodeIdAtAngle])) + " cy=" + String(Math.round(centerYById[nodeIdAtAngle])))
        }
    }

    // Additional connected components that are not attached to the default focus are placed on an outer belt.
    sortLayoutIds(sideComponentIds)
    for (var sideIndex = 0; sideIndex < sideComponentIds.length; ++sideIndex) {
        var sideId = sideComponentIds[sideIndex]
        var sideAngle = -Math.PI / 2 + 2 * Math.PI * sideIndex / Math.max(1, sideComponentIds.length)
        centerXById[sideId] = Math.cos(sideAngle) * baseRadiusX * 2.55
        centerYById[sideId] = Math.sin(sideAngle) * baseRadiusY * 2.35
        distById[sideId] = 3
        readableDebug("LAYOUT", "side-component id=" + String(sideId) + " name=" + readableNodeLabel(nodeById[sideId], sideId) + " cx=" + String(Math.round(centerXById[sideId])) + " cy=" + String(Math.round(centerYById[sideId])))
    }

    function ensureCenter(id) {
        if (centerXById[id] === undefined)
            centerXById[id] = 0
        if (centerYById[id] === undefined)
            centerYById[id] = 0
    }
    for (connectedIndex = 0; connectedIndex < connectedIds.length; ++connectedIndex)
        ensureCenter(connectedIds[connectedIndex])

    // Deterministic force refinement: keep the highest-degree node as the visual anchor,
    // pull related nodes toward readable distances, and push cards away to avoid overlap.
    var movableIds = connectedIds.slice(0)
    var idealEdgeLength = Math.max(maxNodeW * 1.46, 900)
    for (var iter = 0; iter < 140; ++iter) {
        var forceX = {}
        var forceY = {}
        for (var mi = 0; mi < movableIds.length; ++mi) {
            forceX[movableIds[mi]] = 0
            forceY[movableIds[mi]] = 0
        }

        for (var pi = 0; pi < movableIds.length; ++pi) {
            var aId = movableIds[pi]
            for (var pj = pi + 1; pj < movableIds.length; ++pj) {
                var bId = movableIds[pj]
                var dx = centerXById[bId] - centerXById[aId]
                var dy = centerYById[bId] - centerYById[aId]
                var distance = Math.sqrt(dx * dx + dy * dy)
                if (distance < 1) {
                    distance = 1
                    dx = 1
                    dy = 0
                }
                var nx = dx / distance
                var ny = dy / distance
                var desiredX = (widthById[aId] + widthById[bId]) / 2 + 390
                var desiredY = (heightById[aId] + heightById[bId]) / 2 + 300
                var overlapX = desiredX - Math.abs(dx)
                var overlapY = desiredY - Math.abs(dy)
                if (overlapX > 0 && overlapY > 0) {
                    var push = Math.min(58, Math.max(10, Math.min(overlapX, overlapY) * 0.16))
                    if (overlapX < overlapY) {
                        nx = dx >= 0 ? 1 : -1
                        ny = 0
                    } else {
                        nx = 0
                        ny = dy >= 0 ? 1 : -1
                    }
                    if (aId !== defaultFocusId) {
                        forceX[aId] -= nx * push
                        forceY[aId] -= ny * push
                    }
                    if (bId !== defaultFocusId) {
                        forceX[bId] += nx * push
                        forceY[bId] += ny * push
                    }
                } else {
                    var repel = Math.min(22, 160000 / (distance * distance))
                    if (aId !== defaultFocusId) {
                        forceX[aId] -= nx * repel
                        forceY[aId] -= ny * repel
                    }
                    if (bId !== defaultFocusId) {
                        forceX[bId] += nx * repel
                        forceY[bId] += ny * repel
                    }
                }
            }
        }

        for (edgeIndex = 0; edgeIndex < validEdges.length; ++edgeIndex) {
            edge = validEdges[edgeIndex]
            if (degreeById[edge.fromId] <= 0 || degreeById[edge.toId] <= 0)
                continue
            var fromId = edge.fromId
            var toId = edge.toId
            dx = centerXById[toId] - centerXById[fromId]
            dy = centerYById[toId] - centerYById[fromId]
            distance = Math.sqrt(dx * dx + dy * dy)
            if (distance < 1) {
                distance = 1
                dx = 1
                dy = 0
            }
            nx = dx / distance
            ny = dy / distance
            var desiredLength = idealEdgeLength + Math.min(360, Math.abs((distById[fromId] || 0) - (distById[toId] || 0)) * 120)
            var pull = Math.max(-16, Math.min(16, (distance - desiredLength) * 0.018))
            if (fromId !== defaultFocusId) {
                forceX[fromId] += nx * pull
                forceY[fromId] += ny * pull
            }
            if (toId !== defaultFocusId) {
                forceX[toId] -= nx * pull
                forceY[toId] -= ny * pull
            }
        }

        for (mi = 0; mi < movableIds.length; ++mi) {
            var moveId = movableIds[mi]
            if (moveId === defaultFocusId) {
                centerXById[moveId] = 0
                centerYById[moveId] = 0
                continue
            }
            var fx = Math.max(-62, Math.min(62, forceX[moveId] || 0))
            var fy = Math.max(-62, Math.min(62, forceY[moveId] || 0))
            centerXById[moveId] += fx
            centerYById[moveId] += fy
        }
    }

    // Final overlap sweeps with card-size awareness.
    for (iter = 0; iter < 90; ++iter) {
        var moved = false
        for (pi = 0; pi < movableIds.length; ++pi) {
            aId = movableIds[pi]
            for (pj = pi + 1; pj < movableIds.length; ++pj) {
                bId = movableIds[pj]
                dx = centerXById[bId] - centerXById[aId]
                dy = centerYById[bId] - centerYById[aId]
                var minDX = (widthById[aId] + widthById[bId]) / 2 + 330
                var minDY = (heightById[aId] + heightById[bId]) / 2 + 250
                overlapX = minDX - Math.abs(dx)
                overlapY = minDY - Math.abs(dy)
                if (overlapX <= 0 || overlapY <= 0)
                    continue
                moved = true
                if (overlapX < overlapY) {
                    var sx = dx >= 0 ? 1 : -1
                    if (aId !== defaultFocusId)
                        centerXById[aId] -= sx * overlapX * 0.54
                    if (bId !== defaultFocusId)
                        centerXById[bId] += sx * overlapX * 0.54
                } else {
                    var sy = dy >= 0 ? 1 : -1
                    if (aId !== defaultFocusId)
                        centerYById[aId] -= sy * overlapY * 0.54
                    if (bId !== defaultFocusId)
                        centerYById[bId] += sy * overlapY * 0.54
                }
            }
        }
        if (!moved)
            break
    }

    var connectedMinX = 0
    var connectedMinY = 0
    var connectedMaxX = 0
    var connectedMaxY = 0
    var hasConnected = connectedIds.length > 0
    if (hasConnected) {
        connectedMinX = 999999
        connectedMinY = 999999
        connectedMaxX = -999999
        connectedMaxY = -999999
        for (connectedIndex = 0; connectedIndex < connectedIds.length; ++connectedIndex) {
            connectedId = connectedIds[connectedIndex]
            connectedMinX = Math.min(connectedMinX, centerXById[connectedId] - widthById[connectedId] / 2)
            connectedMinY = Math.min(connectedMinY, centerYById[connectedId] - heightById[connectedId] / 2)
            connectedMaxX = Math.max(connectedMaxX, centerXById[connectedId] + widthById[connectedId] / 2)
            connectedMaxY = Math.max(connectedMaxY, centerYById[connectedId] + heightById[connectedId] / 2)
        }
    }

    var marginX = 150
    var marginY = 132
    var isolatedGapX = Math.max(190, maxNodeW * 0.34)
    var isolatedGapY = Math.max(136, maxNodeH * 0.45)
    var isolatedColumnW = 0
    var isolatedMaxH = 0
    for (var isoSizeIndex = 0; isoSizeIndex < isolatedIds.length; ++isoSizeIndex) {
        isolatedColumnW = Math.max(isolatedColumnW, widthById[isolatedIds[isoSizeIndex]])
        isolatedMaxH = Math.max(isolatedMaxH, heightById[isolatedIds[isoSizeIndex]])
    }

    function isolatedGridColumnCount(count, connectedPresent) {
        if (count <= 0)
            return 0
        if (!connectedPresent) {
            if (count <= 2)
                return count
            if (count <= 6)
                return 3
            if (count <= 12)
                return 4
            if (count <= 24)
                return 4
            return 5
        }
        if (count <= 3)
            return 1
        if (count <= 10)
            return 2
        return 3
    }

    var isolatedColumns = isolatedGridColumnCount(isolatedIds.length, hasConnected)
    var isolatedRows = isolatedColumns > 0 ? Math.ceil(isolatedIds.length / isolatedColumns) : 0
    var isolatedPanelWidth = isolatedColumns > 0
        ? isolatedColumns * isolatedColumnW + Math.max(0, isolatedColumns - 1) * isolatedGapX
        : 0

    var graphOffsetX = marginX
        + (hasConnected && isolatedIds.length > 0 ? isolatedPanelWidth + 300 : 0)
        - (hasConnected ? connectedMinX : 0)
    var graphOffsetY = marginY - (hasConnected ? connectedMinY : 0)

    for (connectedIndex = 0; connectedIndex < connectedIds.length; ++connectedIndex) {
        connectedId = connectedIds[connectedIndex]
        positions[connectedId] = {
            "x": centerXById[connectedId] - widthById[connectedId] / 2 + graphOffsetX,
            "y": centerYById[connectedId] - heightById[connectedId] / 2 + graphOffsetY,
            "width": widthById[connectedId],
            "height": heightById[connectedId],
            "laneIndex": Math.round((centerXById[connectedId] - connectedMinX) / Math.max(1, maxNodeW + 180)),
            "order": Math.round((centerYById[connectedId] - connectedMinY) / Math.max(1, maxNodeH + 120))
        }
        readableDebug("LAYOUT", "force-node id=" + String(connectedId) + " name=" + readableNodeLabel(nodeById[connectedId], connectedId) + " dist=" + String(distById[connectedId]) + " degree=" + String(degreeById[connectedId]) + " " + readablePositionText(positions[connectedId]))
    }

    for (var isoIndex = 0; isoIndex < isolatedIds.length; ++isoIndex) {
        var isoId = isolatedIds[isoIndex]
        var isoColumn = isolatedColumns > 0 ? isoIndex % isolatedColumns : 0
        var isoRow = isolatedColumns > 0 ? Math.floor(isoIndex / isolatedColumns) : isoIndex
        var isoX = marginX + isoColumn * (isolatedColumnW + isolatedGapX)
        var isoY = marginY + isoRow * (isolatedMaxH + isolatedGapY)
        positions[isoId] = {
            "x": isoX,
            "y": isoY,
            "width": widthById[isoId],
            "height": heightById[isoId],
            "laneIndex": -1 + isoColumn,
            "order": isoRow
        }
        readableDebug("LAYOUT", "isolated-grid-node id=" + String(isoId) + " name=" + readableNodeLabel(nodeById[isoId], isoId) + " column=" + String(isoColumn) + " row=" + String(isoRow) + " " + readablePositionText(positions[isoId]))
    }

    readableDebug("LAYOUT", "isolated-grid-summary isolated=" + String(isolatedIds.length)
        + " connected=" + String(connectedIds.length)
        + " columns=" + String(isolatedColumns)
        + " rows=" + String(isolatedRows)
        + " validEdges=" + String(validEdges.length)
        + " rawEdges=" + String(edges.length))

    function sortPeerEdges(edgeList, peerField) {
        edgeList.sort(function(left, right) {
            var lp = positions[left ? left[peerField] : undefined] || { "x": 0, "y": 0 }
            var rp = positions[right ? right[peerField] : undefined] || { "x": 0, "y": 0 }
            if (Math.abs(lp.y - rp.y) > 0.5)
                return lp.y - rp.y
            if (Math.abs(lp.x - rp.x) > 0.5)
                return lp.x - rp.x
            return readableEdgeKey(left).localeCompare(readableEdgeKey(right))
        })
    }

    for (nodeIndex = 0; nodeIndex < nodeIds.length; ++nodeIndex) {
        var id = nodeIds[nodeIndex]
        var outEdges = outgoingById[id] || []
        var inEdges = incomingById[id] || []
        sortPeerEdges(outEdges, "toId")
        sortPeerEdges(inEdges, "fromId")
        outgoingCountByNodeId[id] = outEdges.length
        incomingCountByNodeId[id] = inEdges.length
        for (var outIndex = 0; outIndex < outEdges.length; ++outIndex) {
            outgoingSlotByEdgeId[outEdges[outIndex].id] = { "index": outIndex, "count": outEdges.length }
            outgoingStemSlotByEdgeId[outEdges[outIndex].id] = { "index": outIndex, "count": outEdges.length }
        }
        for (var inIndex = 0; inIndex < inEdges.length; ++inIndex) {
            incomingSlotByEdgeId[inEdges[inIndex].id] = { "index": inIndex, "count": inEdges.length }
            incomingStemSlotByEdgeId[inEdges[inIndex].id] = { "index": inIndex, "count": inEdges.length }
        }

        var pos = positions[id]
        if (pos) {
            outgoingBusXByNodeId[id] = pos.x + widthById[id] + 72
            incomingBusXByNodeId[id] = pos.x - 72
            layerById[id] = pos.laneIndex || 0
        }
    }

    var trackGroups = {}
    function routeTrackGroupKey(edge) {
        var fp = positions[edge.fromId] || { "x": 0, "y": 0 }
        var tp = positions[edge.toId] || { "x": 0, "y": 0 }
        var horizontal = Math.abs((tp.x || 0) - (fp.x || 0)) >= Math.abs((tp.y || 0) - (fp.y || 0))
        return (horizontal ? "h" : "v") + ":" + String(Math.round(((fp.y || 0) + ((fp.height || 0) / 2) + (tp.y || 0) + ((tp.height || 0) / 2)) / 240))
    }
    for (edgeIndex = 0; edgeIndex < validEdges.length; ++edgeIndex) {
        edge = validEdges[edgeIndex]
        var groupKey = routeTrackGroupKey(edge)
        if (!trackGroups[groupKey])
            trackGroups[groupKey] = []
        trackGroups[groupKey].push(edge)
    }
    for (groupKey in trackGroups) {
        var members = trackGroups[groupKey]
        members.sort(function(left, right) {
            return readableEdgeKey(left).localeCompare(readableEdgeKey(right))
        })
        routeTrackCountByGroupKey[groupKey] = members.length
        for (var trackIndex = 0; trackIndex < members.length; ++trackIndex) {
            routeTrackByEdgeId[members[trackIndex].id] = { "index": trackIndex, "count": members.length }
            routeTrackGroupKeyByEdgeId[members[trackIndex].id] = groupKey
        }
        readableDebug("LAYOUT", "route-track-group key=" + groupKey + " members=" + String(members.length))
    }

    var maxX = 0
    var maxY = 0
    for (nodeIndex = 0; nodeIndex < nodeIds.length; ++nodeIndex) {
        id = nodeIds[nodeIndex]
        if (!positions[id])
            continue
        maxX = Math.max(maxX, positions[id].x + widthById[id])
        maxY = Math.max(maxY, positions[id].y + heightById[id])
    }

    readableDebug("LAYOUT", "final-scene width=" + String(Math.max(bounds.width || 980, maxX + marginX)) + " height=" + String(Math.max(bounds.height || 560, maxY + marginY)) + " positions=" + String(nodeIds.length))
    for (var dbgFinalIndex = 0; dbgFinalIndex < nodeIds.length; ++dbgFinalIndex) {
        var dbgFinalId = nodeIds[dbgFinalIndex]
        readableDebug("LAYOUT", "final-node id=" + String(dbgFinalId) + " name=" + readableNodeLabel(nodeById[dbgFinalId], dbgFinalId) + " degree=" + String(degreeById[dbgFinalId] || 0) + " directed=" + String(directedDegreeById[dbgFinalId] || 0) + " defaultFocus=" + String(dbgFinalId === defaultFocusId) + " isolated=" + String((degreeById[dbgFinalId] || 0) === 0) + " " + readablePositionText(positions[dbgFinalId]))
    }

    return {
        "positions": positions,
        "primaryEdgeIds": primaryEdgeIds,
        "secondaryEdgeIds": secondaryEdgeIds,
        "outgoingSlotByEdgeId": outgoingSlotByEdgeId,
        "incomingSlotByEdgeId": incomingSlotByEdgeId,
        "outgoingStemSlotByEdgeId": outgoingStemSlotByEdgeId,
        "incomingStemSlotByEdgeId": incomingStemSlotByEdgeId,
        "outgoingCountByNodeId": outgoingCountByNodeId,
        "incomingCountByNodeId": incomingCountByNodeId,
        "outgoingBusXByNodeId": outgoingBusXByNodeId,
        "incomingBusXByNodeId": incomingBusXByNodeId,
        "fanOutBundleByEdgeId": fanOutBundleByEdgeId,
        "fanInBundleByEdgeId": fanInBundleByEdgeId,
        "routeTrackByEdgeId": routeTrackByEdgeId,
        "routeTrackGroupKeyByEdgeId": routeTrackGroupKeyByEdgeId,
        "routeTrackCountByGroupKey": routeTrackCountByGroupKey,
        "layerById": layerById,
        "layerCount": Math.max(1, connectedIds.length ? 4 : 1),
        "rootId": rootNode.id,
        "leftIds": [],
        "rightIds": [],
        "readableDefaultFocusId": defaultFocusId,
        "readableDegreeById": degreeById,
        "readableDirectedDegreeById": directedDegreeById,
        "readableIsolatedIds": isolatedIds,
        "width": Math.max(bounds.width || 980, maxX + marginX),
        "height": Math.max(bounds.height || 560, maxY + marginY)
    }
}

function buildOverviewLayout(config) {
    var empty = emptyOverviewLayout()
    var nodes = config && config.nodes ? config.nodes : []
    var edges = config && config.edges ? config.edges : []
    var bounds = config && config.bounds ? config.bounds : ({})
    var rootNode = config ? config.rootNode : null
    if (!nodes.length || !rootNode || rootNode.id === undefined)
        return empty

    // Final readability-first overview layout:
    // no fan-in / fan-out bundling; place central modules first, then route every relation independently.
    return buildReadableOverviewLayout(config, empty)

    var positions = {}
    var primaryEdgeIds = {}
    var secondaryEdgeIds = {}
    var outgoingSlotByEdgeId = {}
    var incomingSlotByEdgeId = {}
    var outgoingStemSlotByEdgeId = {}
    var incomingStemSlotByEdgeId = {}
    var outgoingCountByNodeId = {}
    var incomingCountByNodeId = {}
    var outgoingBusXByNodeId = {}
    var incomingBusXByNodeId = {}
    var fanOutBundleByEdgeId = {}
    var fanInBundleByEdgeId = {}
    var routeTrackByEdgeId = {}
    var routeTrackGroupKeyByEdgeId = {}
    var routeTrackCountByGroupKey = {}
    var layerById = {}
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
    var laneGapAfterById = {}
    var maxLayerIndex = 0
    var maxLaneSize = 0
    var maxNodeHeight = 0
    var contentHeight = 0
    var minX = 999999
    var minY = 999999
    var maxX = -999999
    var maxY = -999999
    var rootId = rootNode.id
    var marginX = 82
    var marginY = 82
    var baseLaneGapX = 228

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
        var nameCompare = safeNodeName(leftNode).localeCompare(safeNodeName(rightNode))
        if (nameCompare !== 0)
            return nameCompare

        return safeNodeId(leftId).localeCompare(safeNodeId(rightId))
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
                forwardLaneIds.sort(function (leftId, rightId) {
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
            backwardLaneIds.sort(function (leftId, rightId) {
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

    function sortPeerEdges(edgeList, peerField) {
        edgeList.sort(function (left, right) {
            var leftPeerId = left ? left[peerField] : undefined
            var rightPeerId = right ? right[peerField] : undefined
            var leftSortY = overviewPeerSortY(positions, leftPeerId, fallbackYById)
            var rightSortY = overviewPeerSortY(positions, rightPeerId, fallbackYById)
            if (Math.abs(leftSortY - rightSortY) > 0.5)
                return leftSortY - rightSortY

            var leftSortX = overviewPeerSortX(positions, leftPeerId, fallbackXById)
            var rightSortX = overviewPeerSortX(positions, rightPeerId, fallbackXById)
            if (Math.abs(leftSortX - rightSortX) > 0.5)
                return leftSortX - rightSortX

            return safeNodeId(left ? left.id : "").localeCompare(safeNodeId(right ? right.id : ""))
        })
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

        var fromPosition = positions[edge.fromId] || { "y": 0 }
        var toPosition = positions[edge.toId] || { "y": 0 }
        return "same:" + fromLayerValue + ":" + (toPosition.y >= fromPosition.y ? "down" : "up")
    }

    function routeTrackSortMetrics(edge) {
        var fromPosition = positions[edge.fromId] || { "x": 0, "y": 0 }
        var toPosition = positions[edge.toId] || { "x": 0, "y": 0 }
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
        widthById[nodeId] = config.nodeWidth ? config.nodeWidth(node) : 560
        heightById[nodeId] = config.nodeHeight ? config.nodeHeight(node) : 330
        maxNodeHeight = Math.max(maxNodeHeight, heightById[nodeId])
        fallbackXById[nodeId] = nodeX
        fallbackYById[nodeId] = nodeY
        rawLaneById[nodeId] = rawLane
        outgoingById[nodeId] = []
        incomingById[nodeId] = []

        if (isFinite(rawLane))
            pushUnique(distinctRawLanes, rawLane)
    }

    for (var edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
        var edge = edges[edgeIndex]
        if (!edge || edge.id === undefined)
            continue
        if (!nodeByIdMap[edge.fromId] || !nodeByIdMap[edge.toId])
            continue

        primaryEdgeIds[edge.id] = true
        outgoingById[edge.fromId].push(edge)
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
        pushUnique(normalizedRawLanes, resolvedLane)
    }

    normalizedRawLanes.sort(function (left, right) { return left - right })
    if (!normalizedRawLanes.length)
        normalizedRawLanes.push(0)

    var normalizedLaneByRaw = {}
    for (var normalizedIndex = 0; normalizedIndex < normalizedRawLanes.length; ++normalizedIndex) {
        normalizedLaneByRaw[safeNodeId(normalizedRawLanes[normalizedIndex])] = normalizedIndex
        laneBuckets.push([])
    }

    for (nodeIndex = 0; nodeIndex < nodeIds.length; ++nodeIndex) {
        nodeId = nodeIds[nodeIndex]
        var layerIndex = normalizedLaneByRaw[safeNodeId(rawLaneById[nodeId])]
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

    for (var sweepIndex = 0; sweepIndex < 5; ++sweepIndex) {
        sweepLaneOrder(1)
        sweepLaneOrder(-1)
    }

    for (laneIndex = 0; laneIndex < laneBuckets.length; ++laneIndex) {
        var laneIds = laneBuckets[laneIndex]
        maxLaneSize = Math.max(maxLaneSize, laneIds.length)
        var laneWidth = 0
        var laneHeight = 0
        var laneBundleWidth = 0
        for (var laneNodeIndex = 0; laneNodeIndex < laneIds.length; ++laneNodeIndex) {
            nodeId = laneIds[laneNodeIndex]
            var traffic = overviewNodeTraffic(incomingById, outgoingById, nodeId)
            var maxBundleCount = Math.max(incomingById[nodeId].length, outgoingById[nodeId].length)
            laneWidth = Math.max(laneWidth, widthById[nodeId] || 0)
            laneBundleWidth = Math.max(laneBundleWidth, 96 + maxBundleCount * 9)
            laneHeight += heightById[nodeId] || 0

            if (laneNodeIndex < laneIds.length - 1) {
                var nextNodeId = laneIds[laneNodeIndex + 1]
                var nextTraffic = overviewNodeTraffic(incomingById, outgoingById, nextNodeId)
                var gapY = 128 + Math.min(164, Math.max(traffic, nextTraffic) * 12)
                laneGapAfterById[nodeId] = gapY
                laneHeight += gapY
            }
        }

        laneWidthByIndex[laneIndex] = Math.max(620, laneWidth + laneBundleWidth)
        laneHeightByIndex[laneIndex] = laneHeight
        contentHeight = Math.max(contentHeight, laneHeight)
        if (laneIndex < laneBuckets.length - 1)
            corridorLoadByGap[laneIndex] = 0
    }

    if (maxLaneSize > 1 && maxNodeHeight > 0) {
        var spreadGapY = 224 + Math.max(0, maxLaneSize - 2) * 26
        var spreadHeight = maxLaneSize * maxNodeHeight + (maxLaneSize - 1) * spreadGapY
        contentHeight = Math.max(contentHeight, spreadHeight)
    }

    for (edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
        edge = edges[edgeIndex]
        if (!edge || !nodeByIdMap[edge.fromId] || !nodeByIdMap[edge.toId])
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
            var corridorBonus = Math.min(236, corridorLoad * 18)
            currentX += baseLaneGapX + corridorBonus
        }
    }

    for (laneIndex = 0; laneIndex < laneBuckets.length; ++laneIndex) {
        laneIds = laneBuckets[laneIndex]
        laneIds.sort(function (leftId, rightId) {
            var leftOrder = orderById[leftId] !== undefined ? orderById[leftId] : 0
            var rightOrder = orderById[rightId] !== undefined ? orderById[rightId] : 0
            if (leftOrder !== rightOrder)
                return leftOrder - rightOrder
            return compareNodeIdsByFallback(leftId, rightId)
        })

        var laneTop = marginY + Math.max(0, (contentHeight - laneHeightByIndex[laneIndex]) / 2)
        var laneLeft = laneXByIndex[laneIndex]
        var laneContentWidth = laneWidthByIndex[laneIndex]

        for (var laneNodeIndex = 0; laneNodeIndex < laneIds.length; ++laneNodeIndex) {
            nodeId = laneIds[laneNodeIndex]
            var nodeW = widthById[nodeId] || 560
            var nodeH = heightById[nodeId] || 330
            var positionedX = laneLeft + Math.max(0, (laneContentWidth - nodeW) / 2)
            var packedY = laneTop
            var distributedY = marginY + Math.max(0, (contentHeight - nodeH) / 2)
            if (laneIds.length > 1)
                distributedY = marginY + (Math.max(0, contentHeight - nodeH) * laneNodeIndex
                    / Math.max(1, laneIds.length - 1))
            var positionedY = laneIds.length > 1
                ? packedY * 0.08 + distributedY * 0.92
                : packedY

            positions[nodeId] = {
                "x": positionedX,
                "y": positionedY,
                "width": nodeW,
                "height": nodeH,
                "laneIndex": laneIndex,
                "order": laneNodeIndex
            }
            minX = Math.min(minX, positionedX)
            maxX = Math.max(maxX, positionedX + nodeW)
            minY = Math.min(minY, positionedY)
            maxY = Math.max(maxY, positionedY + nodeH)
            laneTop += nodeH
            if (laneNodeIndex < laneIds.length - 1)
                laneTop += laneGapAfterById[nodeId] || 92
        }
    }

    if (!isFinite(minX) || !isFinite(minY) || !isFinite(maxX) || !isFinite(maxY)) {
        minX = 0
        minY = 0
        maxX = bounds.width || 980
        maxY = bounds.height || 560
    }

    for (nodeIndex = 0; nodeIndex < nodeIds.length; ++nodeIndex) {
        nodeId = nodeIds[nodeIndex]
        var outgoingEdges = outgoingById[nodeId] || []
        var incomingEdges = incomingById[nodeId] || []

        sortPeerEdges(outgoingEdges, "toId")
        outgoingCountByNodeId[nodeId] = outgoingEdges.length
        for (var order = 0; order < outgoingEdges.length; ++order)
            outgoingSlotByEdgeId[outgoingEdges[order].id] = { "index": order, "count": outgoingEdges.length }

        sortPeerEdges(incomingEdges, "fromId")
        incomingCountByNodeId[nodeId] = incomingEdges.length
        for (order = 0; order < incomingEdges.length; ++order)
            incomingSlotByEdgeId[incomingEdges[order].id] = { "index": order, "count": incomingEdges.length }
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
            : nodeRight + 188
        var previousLaneRight = layerIndex > 0
            ? laneRightByIndex[layerIndex - 1]
            : nodeLeft - 188

        outgoingBusXByNodeId[nodeId] = Math.min(nodeRight + 92,
            Math.max(nodeRight + 56, nextLaneLeft - 86))
        incomingBusXByNodeId[nodeId] = Math.max(nodeLeft - 92,
            Math.min(nodeLeft - 56, previousLaneRight + 86))
    }

    var routeTrackGroups = {}
    for (edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
        edge = edges[edgeIndex]
        if (!edge || edge.id === undefined || !nodeByIdMap[edge.fromId] || !nodeByIdMap[edge.toId])
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
        trackMembers.sort(function (left, right) {
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
            return safeNodeId(left.id).localeCompare(safeNodeId(right.id))
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

    function overviewNodeMetrics(nodeId) {
        var position = positions[nodeId] || { "x": 0, "y": 0 }
        var nodeWidth = widthById[nodeId] || 0
        var nodeHeight = heightById[nodeId] || 0
        return {
            "centerX": position.x + nodeWidth / 2,
            "centerY": position.y + nodeHeight / 2,
            "top": position.y,
            "bottom": position.y + nodeHeight,
            "left": position.x,
            "right": position.x + nodeWidth
        }
    }

    function axisOverlapAmount(startA, endA, startB, endB) {
        return Math.min(endA, endB) - Math.max(startA, startB)
    }

    function axisOverlapRatio(overlapAmount, spanA, spanB) {
        if (!isFinite(overlapAmount) || overlapAmount <= 0)
            return 0
        var denominator = Math.max(1, Math.min(Math.abs(spanA), Math.abs(spanB)))
        return overlapAmount / denominator
    }

    function axisGapAmount(startA, endA, startB, endB) {
        return Math.max(0, Math.max(startA, startB) - Math.min(endA, endB))
    }

    function overviewTargetZone(edge) {
        if (!edge || edge.fromId === undefined || edge.toId === undefined || edge.fromId === edge.toId)
            return ""

        var sourceMetrics = overviewNodeMetrics(edge.fromId)
        var targetMetrics = overviewNodeMetrics(edge.toId)
        var deltaX = targetMetrics.centerX - sourceMetrics.centerX
        var deltaY = targetMetrics.centerY - sourceMetrics.centerY
        var horizontalOverlap = axisOverlapAmount(sourceMetrics.left,
            sourceMetrics.right,
            targetMetrics.left,
            targetMetrics.right)
        var verticalOverlap = axisOverlapAmount(sourceMetrics.top,
            sourceMetrics.bottom,
            targetMetrics.top,
            targetMetrics.bottom)
        var horizontalOverlapRatio = axisOverlapRatio(horizontalOverlap,
            sourceMetrics.right - sourceMetrics.left,
            targetMetrics.right - targetMetrics.left)
        var verticalOverlapRatio = axisOverlapRatio(verticalOverlap,
            sourceMetrics.bottom - sourceMetrics.top,
            targetMetrics.bottom - targetMetrics.top)
        var canApproachVertically = horizontalOverlapRatio >= 0.16
            || Math.abs(deltaY) >= Math.abs(deltaX) * 0.94
        var canApproachHorizontally = verticalOverlapRatio >= 0.16
            || Math.abs(deltaX) >= Math.abs(deltaY) * 0.94

        if (canApproachVertically && !canApproachHorizontally)
            return deltaY >= 0 ? "top" : "bottom"
        if (canApproachHorizontally && !canApproachVertically)
            return deltaX >= 0 ? "left" : "right"

        var effectiveVerticalDistance = Math.max(0, Math.abs(deltaY) - Math.max(0, horizontalOverlap) * 0.88)
        var effectiveHorizontalDistance = Math.max(0, Math.abs(deltaX) - Math.max(0, verticalOverlap) * 0.88)

        if (effectiveHorizontalDistance >= effectiveVerticalDistance * 1.08)
            return deltaX >= 0 ? "left" : "right"
        if (effectiveVerticalDistance >= effectiveHorizontalDistance * 1.08)
            return deltaY >= 0 ? "top" : "bottom"

        if (Math.abs(deltaX) >= Math.abs(deltaY))
            return deltaX >= 0 ? "left" : "right"
        return deltaY >= 0 ? "top" : "bottom"
    }

    function zonePrimaryValue(metrics, zone) {
        return zone === "left" || zone === "right" ? metrics.centerY : metrics.centerX
    }

    function zoneSecondaryValue(metrics, zone) {
        return zone === "left" || zone === "right" ? metrics.centerX : metrics.centerY
    }

    function sortZoneMembers(members, zone) {
        members.sort(function (left, right) {
            var leftMetrics = overviewNodeMetrics(left.fromId)
            var rightMetrics = overviewNodeMetrics(right.fromId)
            var leftPrimary = zonePrimaryValue(leftMetrics, zone)
            var rightPrimary = zonePrimaryValue(rightMetrics, zone)
            if (Math.abs(leftPrimary - rightPrimary) > 0.5)
                return leftPrimary - rightPrimary

            var leftSecondary = zoneSecondaryValue(leftMetrics, zone)
            var rightSecondary = zoneSecondaryValue(rightMetrics, zone)
            if (Math.abs(leftSecondary - rightSecondary) > 0.5)
                return leftSecondary - rightSecondary

            return safeNodeId(left.id).localeCompare(safeNodeId(right.id))
        })
    }

    function zoneClusterThreshold(targetId, zone) {
        var targetWidth = widthById[targetId] || 560
        var targetHeight = heightById[targetId] || 330
        var base = zone === "left" || zone === "right"
            ? targetHeight * 0.54
            : targetWidth * 0.28
        return boundedValue(base, 118, 208)
    }

    function isOverviewLocalDirectEdge(edge, zone) {
        if (!edge)
            return false

        var sourceMetrics = overviewNodeMetrics(edge.fromId)
        var targetMetrics = overviewNodeMetrics(edge.toId)
        var sourceWidth = Math.max(1, sourceMetrics.right - sourceMetrics.left)
        var sourceHeight = Math.max(1, sourceMetrics.bottom - sourceMetrics.top)
        var targetWidth = Math.max(1, targetMetrics.right - targetMetrics.left)
        var targetHeight = Math.max(1, targetMetrics.bottom - targetMetrics.top)
        var overlapX = axisOverlapAmount(sourceMetrics.left,
            sourceMetrics.right,
            targetMetrics.left,
            targetMetrics.right)
        var overlapY = axisOverlapAmount(sourceMetrics.top,
            sourceMetrics.bottom,
            targetMetrics.top,
            targetMetrics.bottom)
        var overlapRatioX = axisOverlapRatio(overlapX, sourceWidth, targetWidth)
        var overlapRatioY = axisOverlapRatio(overlapY, sourceHeight, targetHeight)
        var gapX = axisGapAmount(sourceMetrics.left,
            sourceMetrics.right,
            targetMetrics.left,
            targetMetrics.right)
        var gapY = axisGapAmount(sourceMetrics.top,
            sourceMetrics.bottom,
            targetMetrics.top,
            targetMetrics.bottom)
        var directDistance = Math.abs(sourceMetrics.centerX - targetMetrics.centerX)
            + Math.abs(sourceMetrics.centerY - targetMetrics.centerY)
        var sourceLayer = layerById[edge.fromId]
        var targetLayer = layerById[edge.toId]
        var layerDelta = sourceLayer !== undefined && targetLayer !== undefined
            ? Math.abs(targetLayer - sourceLayer)
            : 99
        var localDistanceLimit = boundedValue((sourceWidth + targetWidth) * 0.34
                + (sourceHeight + targetHeight) * 0.28,
            210,
            520)

        if (zone === "left" || zone === "right") {
            var nearHorizontal = gapX <= boundedValue((sourceWidth + targetWidth) * 0.22, 118, 236)
            var alignedVertical = overlapRatioY >= 0.18
                || Math.abs(sourceMetrics.centerY - targetMetrics.centerY) <= Math.max(sourceHeight, targetHeight) * 0.72
            if (nearHorizontal && alignedVertical && (layerDelta <= 1 || directDistance <= localDistanceLimit))
                return true
        } else {
            var nearVertical = gapY <= boundedValue((sourceHeight + targetHeight) * 0.20, 108, 220)
            var alignedHorizontal = overlapRatioX >= 0.18
                || Math.abs(sourceMetrics.centerX - targetMetrics.centerX) <= Math.max(sourceWidth, targetWidth) * 0.72
            if (nearVertical && alignedHorizontal && (layerDelta <= 1 || directDistance <= localDistanceLimit))
                return true
        }

        return directDistance <= localDistanceLimit * 0.84
            && layerDelta <= 1
            && (overlapRatioX >= 0.22 || overlapRatioY >= 0.22)
    }

    function filterOverviewBundleEligibleMembers(members, zone) {
        if (!members || members.length < 2)
            return []

        var eligible = []
        for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
            var member = members[memberIndex]
            if (isOverviewLocalDirectEdge(member, zone))
                continue
            eligible.push(member)
        }
        return eligible
    }

    function clusteredTargetZoneGroups(members, zone) {
        if (!members.length)
            return []

        var sortedMembers = members.slice()
        sortZoneMembers(sortedMembers, zone)

        var threshold = zoneClusterThreshold(sortedMembers[0].toId, zone)
        var groups = []
        var currentGroup = []
        var currentMin = 0
        var currentMax = 0
        var currentSecondary = 0

        for (var memberIndex = 0; memberIndex < sortedMembers.length; ++memberIndex) {
            var member = sortedMembers[memberIndex]
            var sourceMetrics = overviewNodeMetrics(member.fromId)
            var primary = zonePrimaryValue(sourceMetrics, zone)
            var secondary = zoneSecondaryValue(sourceMetrics, zone)

            if (!currentGroup.length) {
                currentGroup.push(member)
                currentMin = primary
                currentMax = primary
                currentSecondary = secondary
                continue
            }

            var nextMin = Math.min(currentMin, primary)
            var nextMax = Math.max(currentMax, primary)
            var nextSpan = nextMax - nextMin
            var gapFromTail = Math.abs(primary - currentMax)
            var secondaryDrift = Math.abs(secondary - currentSecondary)
            var shouldSplit = gapFromTail > threshold
                || (currentGroup.length > 1 && nextSpan > threshold * 1.04)
                || (nextSpan > threshold * 0.68 && secondaryDrift > threshold * 0.72)
                || (gapFromTail > threshold * 0.58 && secondaryDrift > threshold * 0.48)

            if (shouldSplit) {
                groups.push(currentGroup)
                currentGroup = [member]
                currentMin = primary
                currentMax = primary
                currentSecondary = secondary
                continue
            }

            currentGroup.push(member)
            currentMin = nextMin
            currentMax = nextMax
            currentSecondary = (currentSecondary * (currentGroup.length - 1) + secondary) / currentGroup.length
        }

        if (currentGroup.length)
            groups.push(currentGroup)

        return groups
    }

    var horizontalLaneYs = []
    var verticalLaneXs = []
    var outerLaneOffset = 56
    horizontalLaneYs.push({ "y": minY - outerLaneOffset, "outer": true })
    for (var rowIndex = 0; rowIndex < Math.max(0, maxLaneSize - 1); ++rowIndex) {
        var rowBottom = -999999
        var nextRowTop = 999999
        var foundRowGap = false
        for (var summaryLaneIndex = 0; summaryLaneIndex < laneBuckets.length; ++summaryLaneIndex) {
            var summaryLaneIds = laneBuckets[summaryLaneIndex]
            if (rowIndex >= summaryLaneIds.length - 1)
                continue

            var currentMetrics = overviewNodeMetrics(summaryLaneIds[rowIndex])
            var nextMetrics = overviewNodeMetrics(summaryLaneIds[rowIndex + 1])
            rowBottom = Math.max(rowBottom, currentMetrics.bottom)
            nextRowTop = Math.min(nextRowTop, nextMetrics.top)
            foundRowGap = true
        }

        if (!foundRowGap)
            continue

        var rowGap = nextRowTop - rowBottom
        horizontalLaneYs.push({
            "y": rowGap > 14 ? rowBottom + rowGap / 2 : rowBottom + 44,
            "outer": false,
            "row": rowIndex
        })
    }
    horizontalLaneYs.push({ "y": maxY + outerLaneOffset, "outer": true })

    verticalLaneXs.push({ "x": minX - outerLaneOffset, "outer": true })
    for (laneIndex = 0; laneIndex < laneBuckets.length - 1; ++laneIndex) {
        var gapLeft = laneRightByIndex[laneIndex]
        var gapRight = laneXByIndex[laneIndex + 1]
        verticalLaneXs.push({
            "x": gapRight > gapLeft ? (gapLeft + gapRight) / 2 : (gapLeft + outerLaneOffset),
            "outer": false,
            "lane": laneIndex
        })
    }
    verticalLaneXs.push({ "x": maxX + outerLaneOffset, "outer": true })

    function clusterSpan(members) {
        var left = 999999
        var right = -999999
        var top = 999999
        var bottom = -999999

        for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
            var sourceMetrics = overviewNodeMetrics(members[memberIndex].fromId)
            var targetMetrics = overviewNodeMetrics(members[memberIndex].toId)
            left = Math.min(left, sourceMetrics.left, targetMetrics.left)
            right = Math.max(right, sourceMetrics.right, targetMetrics.right)
            top = Math.min(top, sourceMetrics.top, targetMetrics.top)
            bottom = Math.max(bottom, sourceMetrics.bottom, targetMetrics.bottom)
        }

        return {
            "left": left,
            "right": right,
            "top": top,
            "bottom": bottom
        }
    }

    function preferredHorizontalCorridorY(members) {
        var sum = 0
        var weight = 0
        for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
            var sourceMetrics = overviewNodeMetrics(members[memberIndex].fromId)
            var targetMetrics = overviewNodeMetrics(members[memberIndex].toId)
            sum += sourceMetrics.centerY * 1.0 + targetMetrics.centerY * 0.45
            weight += 1.45
        }
        return weight > 0 ? sum / weight : (minY + maxY) / 2
    }

    function preferredVerticalCorridorX(members) {
        var sum = 0
        var weight = 0
        for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
            var sourceMetrics = overviewNodeMetrics(members[memberIndex].fromId)
            var targetMetrics = overviewNodeMetrics(members[memberIndex].toId)
            sum += sourceMetrics.centerX * 1.0 + targetMetrics.centerX * 0.45
            weight += 1.45
        }
        return weight > 0 ? sum / weight : (minX + maxX) / 2
    }

    function corridorDetourAmount(sourceValue, targetValue, corridorValue) {
        return Math.abs(corridorValue - sourceValue)
            + Math.abs(targetValue - corridorValue)
            - Math.abs(targetValue - sourceValue)
    }

    function pushCorridorCandidate(candidates, axisKey, coord, options) {
        if (!isFinite(coord))
            return

        var candidateOptions = options || ({})
        for (var index = 0; index < candidates.length; ++index) {
            if (Math.abs(Number(candidates[index][axisKey]) - coord) < 6)
                return
        }

        var candidate = {}
        candidate[axisKey] = coord
        candidate.outer = !!candidateOptions.outer
        candidate.local = !!candidateOptions.local
        candidate.shell = candidateOptions.shell !== undefined ? Number(candidateOptions.shell) : 0
        candidate.depth = candidateOptions.depth !== undefined ? Number(candidateOptions.depth) : 0
        candidate.bias = Number(candidateOptions.bias || 0)
        candidates.push(candidate)
    }

    function expandedClusterClearance(spanStart, spanEnd) {
        return boundedValue((spanEnd - spanStart) * 0.16 + 28, 42, 92)
    }

    function buildHorizontalCorridorCandidates(members) {
        var span = clusterSpan(members)
        var candidates = []
        for (var laneIndex = 0; laneIndex < horizontalLaneYs.length; ++laneIndex)
            pushCorridorCandidate(candidates, "y", Number(horizontalLaneYs[laneIndex].y), horizontalLaneYs[laneIndex])

        var clearance = expandedClusterClearance(span.top, span.bottom)
        pushCorridorCandidate(candidates, "y", span.top - clearance, {
            "local": true,
            "shell": 1,
            "depth": clearance,
            "bias": -22
        })
        pushCorridorCandidate(candidates, "y", span.bottom + clearance, {
            "local": true,
            "shell": 1,
            "depth": clearance,
            "bias": -22
        })
        pushCorridorCandidate(candidates, "y", span.top - clearance * 1.42, {
            "local": true,
            "shell": 2,
            "depth": clearance * 1.42,
            "bias": 26
        })
        pushCorridorCandidate(candidates, "y", span.bottom + clearance * 1.42, {
            "local": true,
            "shell": 2,
            "depth": clearance * 1.42,
            "bias": 26
        })
        return {
            "span": span,
            "clearance": clearance,
            "candidates": candidates
        }
    }

    function buildVerticalCorridorCandidates(members) {
        var span = clusterSpan(members)
        var candidates = []
        for (var laneIndex = 0; laneIndex < verticalLaneXs.length; ++laneIndex)
            pushCorridorCandidate(candidates, "x", Number(verticalLaneXs[laneIndex].x), verticalLaneXs[laneIndex])

        var clearance = expandedClusterClearance(span.left, span.right)
        pushCorridorCandidate(candidates, "x", span.left - clearance, {
            "local": true,
            "shell": 1,
            "depth": clearance,
            "bias": -22
        })
        pushCorridorCandidate(candidates, "x", span.right + clearance, {
            "local": true,
            "shell": 1,
            "depth": clearance,
            "bias": -22
        })
        pushCorridorCandidate(candidates, "x", span.left - clearance * 1.42, {
            "local": true,
            "shell": 2,
            "depth": clearance * 1.42,
            "bias": 26
        })
        pushCorridorCandidate(candidates, "x", span.right + clearance * 1.42, {
            "local": true,
            "shell": 2,
            "depth": clearance * 1.42,
            "bias": 26
        })
        return {
            "span": span,
            "clearance": clearance,
            "candidates": candidates
        }
    }

    function corridorInteriorPenalty(coord, spanStart, spanEnd) {
        var inset = boundedValue((spanEnd - spanStart) * 0.12, 18, 42)
        var innerStart = spanStart + inset
        var innerEnd = spanEnd - inset
        if (coord <= innerStart || coord >= innerEnd)
            return 0

        var edgeDistance = Math.min(coord - innerStart, innerEnd - coord)
        return 340 + edgeDistance * 2.6
    }

    function clusterCongestionWeight(memberCount) {
        var normalizedCount = Math.max(0, Number(memberCount) || 0)
        return 1 + Math.min(1.55, normalizedCount * 0.24)
    }

    function corridorUsageBucket(coord) {
        return String(Math.round(Number(coord) / 34))
    }

    function corridorUsagePenalty(coord, usageByBucket) {
        if (!usageByBucket)
            return 0

        var centerBucket = Math.round(Number(coord) / 34)
        var penalty = 0
        for (var delta = -1; delta <= 1; ++delta) {
            var weight = delta === 0 ? 1.0 : 0.46
            penalty += Number(usageByBucket[String(centerBucket + delta)] || 0) * weight
        }
        return penalty * 110
    }

    function corridorCorePenalty(coord, minimumCoord, maximumCoord, candidate) {
        if (candidate && candidate.local)
            return 0

        var center = (minimumCoord + maximumCoord) / 2
        var halfSpan = Math.max(1, (maximumCoord - minimumCoord) / 2)
        var normalized = 1 - boundedValue(Math.abs(coord - center) / (halfSpan * 0.92), 0, 1)
        return Math.pow(Math.max(0, normalized), 1.8) * 120
    }

    function noteCorridorUsage(usageByBucket, coord) {
        if (!usageByBucket || !isFinite(coord))
            return

        var key = corridorUsageBucket(coord)
        usageByBucket[key] = (usageByBucket[key] || 0) + 1
    }

    function localCorridorAdjustment(candidate, preferredDepth, memberCount) {
        if (!candidate || !candidate.local)
            return 0

        var members = Math.max(1, Number(memberCount) || 1)
        var reward = 28 + Math.min(62, (members - 1) * 11)
        var shell = candidate.shell !== undefined ? Math.max(1, Number(candidate.shell)) : 1
        if (shell > 1)
            reward -= 14 + (shell - 1) * 18

        var depth = candidate.depth !== undefined ? Math.max(0, Number(candidate.depth)) : 0
        var depthPenalty = Math.max(0, depth - preferredDepth) * 0.92
        return depthPenalty - reward
    }

    function chooseHorizontalCorridorY(members, occupiedCorridors, usageByBucket) {
        var preferredY = preferredHorizontalCorridorY(members)
        var candidateData = buildHorizontalCorridorCandidates(members)
        var span = candidateData.span
        var preferredDepth = Number(candidateData.clearance || 0)
        var congestionWeight = clusterCongestionWeight(members.length)
        var candidates = candidateData.candidates
        var bestLane = candidates.length ? candidates[0] : { "y": preferredY }
        var bestScore = Infinity

        for (var laneCandidateIndex = 0; laneCandidateIndex < candidates.length; ++laneCandidateIndex) {
            var laneCandidate = candidates[laneCandidateIndex]
            var laneY = Number(laneCandidate.y)
            var score = Math.abs(laneY - preferredY) * 1.08 + Number(laneCandidate.bias || 0)

            for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
                var sourceMetrics = overviewNodeMetrics(members[memberIndex].fromId)
                var targetMetrics = overviewNodeMetrics(members[memberIndex].toId)
                score += Math.abs(laneY - sourceMetrics.centerY) * 0.18
                score += Math.abs(laneY - targetMetrics.centerY) * 0.26

                var verticalDetour = corridorDetourAmount(sourceMetrics.centerY,
                    targetMetrics.centerY,
                    laneY)
                score += verticalDetour * 1.06

                var monotoneBandTop = Math.min(sourceMetrics.centerY, targetMetrics.centerY) - 18
                var monotoneBandBottom = Math.max(sourceMetrics.centerY, targetMetrics.centerY) + 18
                if (laneY < monotoneBandTop)
                    score += (monotoneBandTop - laneY) * 0.82
                else if (laneY > monotoneBandBottom)
                    score += (laneY - monotoneBandBottom) * 0.82
            }

            score += corridorInteriorPenalty(laneY, span.top, span.bottom) * congestionWeight
            score += localCorridorAdjustment(laneCandidate, preferredDepth, members.length)
            score += corridorUsagePenalty(laneY, usageByBucket)
            score += corridorCorePenalty(laneY, minY, maxY, laneCandidate)

            if (laneCandidate.outer)
                score += 92

            for (var occupiedIndex = 0; occupiedCorridors && occupiedIndex < occupiedCorridors.length; ++occupiedIndex) {
                var occupied = occupiedCorridors[occupiedIndex]
                var overlap = Math.min(span.right, occupied.right) - Math.max(span.left, occupied.left)
                if (overlap <= 16)
                    continue

                var delta = Math.abs(laneY - Number(occupied.coord))
                if (delta < 26)
                    score += 1280 + overlap * 0.24
                else if (delta < 92)
                    score += 220 + overlap * 0.08
            }

            if (score < bestScore - 0.5) {
                bestScore = score
                bestLane = laneCandidate
            }
        }

        return Number(bestLane.y)
    }

    function chooseVerticalCorridorX(members, occupiedCorridors, usageByBucket) {
        var preferredX = preferredVerticalCorridorX(members)
        var candidateData = buildVerticalCorridorCandidates(members)
        var span = candidateData.span
        var preferredDepth = Number(candidateData.clearance || 0)
        var congestionWeight = clusterCongestionWeight(members.length)
        var candidates = candidateData.candidates
        var bestLane = candidates.length ? candidates[0] : { "x": preferredX }
        var bestScore = Infinity

        for (var laneCandidateIndex = 0; laneCandidateIndex < candidates.length; ++laneCandidateIndex) {
            var laneCandidate = candidates[laneCandidateIndex]
            var laneX = Number(laneCandidate.x)
            var score = Math.abs(laneX - preferredX) * 1.08 + Number(laneCandidate.bias || 0)

            for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
                var sourceMetrics = overviewNodeMetrics(members[memberIndex].fromId)
                var targetMetrics = overviewNodeMetrics(members[memberIndex].toId)
                score += Math.abs(laneX - sourceMetrics.centerX) * 0.18
                score += Math.abs(laneX - targetMetrics.centerX) * 0.26

                var horizontalDetour = corridorDetourAmount(sourceMetrics.centerX,
                    targetMetrics.centerX,
                    laneX)
                score += horizontalDetour * 1.06

                var monotoneBandLeft = Math.min(sourceMetrics.centerX, targetMetrics.centerX) - 18
                var monotoneBandRight = Math.max(sourceMetrics.centerX, targetMetrics.centerX) + 18
                if (laneX < monotoneBandLeft)
                    score += (monotoneBandLeft - laneX) * 0.82
                else if (laneX > monotoneBandRight)
                    score += (laneX - monotoneBandRight) * 0.82
            }

            score += corridorInteriorPenalty(laneX, span.left, span.right) * congestionWeight
            score += localCorridorAdjustment(laneCandidate, preferredDepth, members.length)
            score += corridorUsagePenalty(laneX, usageByBucket)
            score += corridorCorePenalty(laneX, minX, maxX, laneCandidate)

            if (laneCandidate.outer)
                score += 92

            for (var occupiedIndex = 0; occupiedCorridors && occupiedIndex < occupiedCorridors.length; ++occupiedIndex) {
                var occupied = occupiedCorridors[occupiedIndex]
                var overlap = Math.min(span.bottom, occupied.bottom) - Math.max(span.top, occupied.top)
                if (overlap <= 16)
                    continue

                var delta = Math.abs(laneX - Number(occupied.coord))
                if (delta < 26)
                    score += 1280 + overlap * 0.24
                else if (delta < 92)
                    score += 220 + overlap * 0.08
            }

            if (score < bestScore - 0.5) {
                bestScore = score
                bestLane = laneCandidate
            }
        }

        return Number(bestLane.x)
    }

    function sourceSideForHorizontalBundle(edge, corridorY) {
        var sourceMetrics = overviewNodeMetrics(edge.fromId)
        var targetMetrics = overviewNodeMetrics(edge.toId)
        var deltaX = targetMetrics.centerX - sourceMetrics.centerX
        var deltaY = corridorY - sourceMetrics.centerY
        if (Math.abs(deltaX) >= Math.abs(deltaY) * 0.82)
            return deltaX >= 0 ? "right" : "left"
        return deltaY >= 0 ? "bottom" : "top"
    }

    function sourceSideForVerticalBundle(edge, corridorX) {
        var sourceMetrics = overviewNodeMetrics(edge.fromId)
        var targetMetrics = overviewNodeMetrics(edge.toId)
        var deltaX = corridorX - sourceMetrics.centerX
        var deltaY = targetMetrics.centerY - sourceMetrics.centerY
        if (Math.abs(deltaY) >= Math.abs(deltaX) * 0.82)
            return deltaY >= 0 ? "bottom" : "top"
        return deltaX >= 0 ? "right" : "left"
    }

    function clusterLeaderEdgeId(members, orientation, corridorCoord) {
        var leaderEdgeId = members.length > 0 ? members[0].id : ""
        var bestScore = Infinity
        for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
            var member = members[memberIndex]
            var sourceMetrics = overviewNodeMetrics(member.fromId)
            var targetMetrics = overviewNodeMetrics(member.toId)
            var score = orientation === "horizontal"
                ? Math.abs(sourceMetrics.centerY - corridorCoord) * 0.78
                    + Math.abs(sourceMetrics.centerX - targetMetrics.centerX) * 0.18
                : Math.abs(sourceMetrics.centerX - corridorCoord) * 0.78
                    + Math.abs(sourceMetrics.centerY - targetMetrics.centerY) * 0.18
            if (score < bestScore) {
                bestScore = score
                leaderEdgeId = member.id
            }
        }
        return leaderEdgeId
    }

    var occupiedHorizontalCorridors = []
    var occupiedVerticalCorridors = []
    var horizontalCorridorUsage = {}
    var verticalCorridorUsage = {}
    var zoneOrder = ["left", "right", "top", "bottom"]

    for (nodeIndex = 0; nodeIndex < nodeIds.length; ++nodeIndex) {
        nodeId = nodeIds[nodeIndex]
        var targetIncomingEdges = incomingById[nodeId] || []
        if (!targetIncomingEdges.length)
            continue

        var zoneGroups = {}
        for (edgeIndex = 0; edgeIndex < targetIncomingEdges.length; ++edgeIndex) {
            edge = targetIncomingEdges[edgeIndex]
            var targetZone = overviewTargetZone(edge)
            if (!targetZone.length)
                continue
            if (!zoneGroups[targetZone])
                zoneGroups[targetZone] = []
            zoneGroups[targetZone].push(edge)
        }

        for (var zoneOrderIndex = 0; zoneOrderIndex < zoneOrder.length; ++zoneOrderIndex) {
            var zone = zoneOrder[zoneOrderIndex]
            var zoneMembers = zoneGroups[zone]
            if (!zoneMembers || !zoneMembers.length)
                continue

            zoneMembers = filterOverviewBundleEligibleMembers(zoneMembers, zone)
            if (zoneMembers.length < 2)
                continue

            var zoneClusters = clusteredTargetZoneGroups(zoneMembers, zone)
            var bundledZoneClusters = []
            for (var zoneClusterIndex = 0; zoneClusterIndex < zoneClusters.length; ++zoneClusterIndex) {
                if (zoneClusters[zoneClusterIndex].length < 2)
                    continue
                sortZoneMembers(zoneClusters[zoneClusterIndex], zone)
                bundledZoneClusters.push(zoneClusters[zoneClusterIndex])
            }
            if (!bundledZoneClusters.length)
                continue

            var targetMetrics = overviewNodeMetrics(nodeId)
            var clusterCount = bundledZoneClusters.length
            for (var clusterIndex = 0; clusterIndex < bundledZoneClusters.length; ++clusterIndex) {
                var clusterMembers = bundledZoneClusters[clusterIndex]
                var clusterRelative = clusterCount > 1 ? (clusterIndex - (clusterCount - 1) / 2) : 0
                var span = clusterSpan(clusterMembers)
                var bundleKey = safeNodeId(nodeId) + ":" + zone + ":cluster:" + String(clusterIndex)
                var orientation = zone === "left" || zone === "right" ? "horizontal" : "vertical"
                var corridorY = NaN
                var corridorX = NaN
                var targetTrunkX = NaN
                var targetTrunkY = NaN

                if (orientation === "horizontal") {
                    corridorY = chooseHorizontalCorridorY(clusterMembers,
                        occupiedHorizontalCorridors,
                        horizontalCorridorUsage)
                    targetTrunkX = zone === "left"
                        ? targetMetrics.left - (58 + clusterRelative * 22)
                        : targetMetrics.right + (58 + clusterRelative * 22)
                    occupiedHorizontalCorridors.push({
                        "coord": corridorY,
                        "left": span.left,
                        "right": span.right
                    })
                    noteCorridorUsage(horizontalCorridorUsage, corridorY)
                } else {
                    corridorX = chooseVerticalCorridorX(clusterMembers,
                        occupiedVerticalCorridors,
                        verticalCorridorUsage)
                    targetTrunkY = zone === "top"
                        ? targetMetrics.top - (58 + clusterRelative * 22)
                        : targetMetrics.bottom + (58 + clusterRelative * 22)
                    occupiedVerticalCorridors.push({
                        "coord": corridorX,
                        "top": span.top,
                        "bottom": span.bottom
                    })
                    noteCorridorUsage(verticalCorridorUsage, corridorX)
                }

                var leaderEdgeId = clusterLeaderEdgeId(clusterMembers,
                    orientation,
                    orientation === "horizontal" ? corridorY : corridorX)
                for (var clusterMemberIndex = 0; clusterMemberIndex < clusterMembers.length; ++clusterMemberIndex) {
                    var clusterMember = clusterMembers[clusterMemberIndex]
                    fanInBundleByEdgeId[clusterMember.id] = {
                        "key": bundleKey,
                        "index": clusterMemberIndex,
                        "count": clusterMembers.length,
                        "clusterIndex": clusterIndex,
                        "clusterCount": clusterCount,
                        "orientation": orientation,
                        "targetApproachSide": zone,
                        "corridorY": corridorY,
                        "corridorX": corridorX,
                        "targetTrunkX": targetTrunkX,
                        "targetTrunkY": targetTrunkY,
                        "leaderEdgeId": leaderEdgeId,
                        "sourceSide": orientation === "horizontal"
                            ? sourceSideForHorizontalBundle(clusterMember, corridorY)
                            : sourceSideForVerticalBundle(clusterMember, corridorX)
                    }
                }
            }
        }
    }

    function assignStemSlots(groups, output) {
        for (var groupKey in groups) {
            var members = groups[groupKey]
            members.sort(function (left, right) {
                if (Math.abs(left.sortPrimary - right.sortPrimary) > 0.5)
                    return left.sortPrimary - right.sortPrimary
                if (Math.abs(left.sortSecondary - right.sortSecondary) > 0.5)
                    return left.sortSecondary - right.sortSecondary
                return String(left.edgeId).localeCompare(String(right.edgeId))
            })

            for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
                output[members[memberIndex].edgeId] = {
                    "index": memberIndex,
                    "count": members.length,
                    "side": members[memberIndex].side
                }
            }
        }
    }

    var outgoingStemGroups = {}
    var incomingStemGroups = {}
    for (edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
        edge = edges[edgeIndex]
        if (!edge || edge.id === undefined)
            continue

        var targetBundle = fanInBundleByEdgeId[edge.id]
        if (!targetBundle)
            continue

        var sourceMetricsForStem = overviewNodeMetrics(edge.fromId)
        var targetMetricsForStem = overviewNodeMetrics(edge.toId)
        var outgoingStemKey = safeNodeId(edge.fromId) + ":" + String(targetBundle.sourceSide || "")
        if (!outgoingStemGroups[outgoingStemKey])
            outgoingStemGroups[outgoingStemKey] = []
        outgoingStemGroups[outgoingStemKey].push({
            "edgeId": edge.id,
            "side": String(targetBundle.sourceSide || ""),
            "sortPrimary": targetBundle.orientation === "horizontal"
                ? Number(targetBundle.corridorY)
                : Number(targetBundle.corridorX),
            "sortSecondary": targetBundle.orientation === "horizontal"
                ? targetMetricsForStem.centerX
                : targetMetricsForStem.centerY
        })

        var leaderEdgeId = targetBundle.leaderEdgeId !== undefined
            ? safeNodeId(targetBundle.leaderEdgeId)
            : safeNodeId(edge.id)
        if (leaderEdgeId !== safeNodeId(edge.id))
            continue

        var incomingStemKey = safeNodeId(edge.toId) + ":" + String(targetBundle.targetApproachSide || "")
        if (!incomingStemGroups[incomingStemKey])
            incomingStemGroups[incomingStemKey] = []
        incomingStemGroups[incomingStemKey].push({
            "edgeId": edge.id,
            "side": String(targetBundle.targetApproachSide || ""),
            "sortPrimary": targetBundle.orientation === "horizontal"
                ? Number(targetBundle.corridorY)
                : Number(targetBundle.corridorX),
            "sortSecondary": targetBundle.orientation === "horizontal"
                ? sourceMetricsForStem.centerX
                : sourceMetricsForStem.centerY
        })
    }

    assignStemSlots(outgoingStemGroups, outgoingStemSlotByEdgeId)
    assignStemSlots(incomingStemGroups, incomingStemSlotByEdgeId)

    return {
        "positions": positions,
        "primaryEdgeIds": primaryEdgeIds,
        "secondaryEdgeIds": secondaryEdgeIds,
        "outgoingSlotByEdgeId": outgoingSlotByEdgeId,
        "incomingSlotByEdgeId": incomingSlotByEdgeId,
        "outgoingStemSlotByEdgeId": outgoingStemSlotByEdgeId,
        "incomingStemSlotByEdgeId": incomingStemSlotByEdgeId,
        "outgoingCountByNodeId": outgoingCountByNodeId,
        "incomingCountByNodeId": incomingCountByNodeId,
        "outgoingBusXByNodeId": outgoingBusXByNodeId,
        "incomingBusXByNodeId": incomingBusXByNodeId,
        "fanOutBundleByEdgeId": fanOutBundleByEdgeId,
        "fanInBundleByEdgeId": fanInBundleByEdgeId,
        "routeTrackByEdgeId": routeTrackByEdgeId,
        "routeTrackGroupKeyByEdgeId": routeTrackGroupKeyByEdgeId,
        "routeTrackCountByGroupKey": routeTrackCountByGroupKey,
        "layerById": layerById,
        "layerCount": maxLayerIndex + 1,
        "rootId": rootId,
        "leftIds": [],
        "rightIds": [],
        "width": Math.max(980, maxX + marginX),
        "height": Math.max(560, maxY + marginY)
    }
}

function componentNeighborBias(nodeId, outgoingById, incomingById, groupIndexByNodeId, fallbackYById) {
    var sum = 0
    var weight = 0
    var seen = {}
    var incomingEdges = incomingById[nodeId] || []
    var outgoingEdges = outgoingById[nodeId] || []

    function consider(peerId) {
        if (peerId === undefined || peerId === null || seen[peerId])
            return
        seen[peerId] = true
        var peerGroup = groupIndexByNodeId[peerId]
        if (peerGroup === undefined)
            return
        sum += peerGroup * 1000 + (fallbackYById[peerId] || 0) * 0.1
        weight += 1
    }

    for (var index = 0; index < incomingEdges.length; ++index)
        consider(incomingEdges[index].fromId)
    for (index = 0; index < outgoingEdges.length; ++index)
        consider(outgoingEdges[index].toId)

    return weight > 0 ? sum / weight : null
}

function buildComponentLayout(config) {
    var empty = emptyComponentLayout()
    var nodes = config && config.nodes ? config.nodes : []
    var edges = config && config.edges ? config.edges : []
    if (!nodes.length)
        return empty

    readableDebug("LAYOUT", "component-detail-v9-start nodes=" + String(nodes.length)
        + " edges=" + String(edges.length)
        + " viewWidth=" + String(config && config.viewWidth !== undefined ? config.viewWidth : ""))

    var nodeById = {}
    var validNodes = []
    var outgoingById = {}
    var incomingById = {}
    var neighborById = {}
    var directedDegreeById = {}
    var degreeById = {}
    var incomingCountByNodeId = {}
    var outgoingCountByNodeId = {}
    var groupIndexByNodeId = {}
    var positions = {}
    var outgoingSlotByEdgeId = {}
    var incomingSlotByEdgeId = {}
    var outgoingBusXByNodeId = {}
    var incomingBusXByNodeId = {}
    var fanOutBundleByEdgeId = {}
    var fanInBundleByEdgeId = {}
    var routeTrackByEdgeId = {}
    var routeTrackGroupKeyByEdgeId = {}
    var routeTrackCountByGroupKey = {}
    var groupBoundsByIndex = []

    var viewWidth = Math.max(980, numericOrFallback(config.viewWidth, 1320))
    var nodeCount = nodes.length
    var cardWidth = nodeCount > 42 ? 300 : (nodeCount > 28 ? 326 : (nodeCount > 18 ? 348 : 380))
    var cardHeight = nodeCount > 42 ? 184 : (nodeCount > 28 ? 198 : (nodeCount > 18 ? 212 : 230))
    // v10: detail graphs with 20-60 nodes need more breathing room than a file grid.
    // Keep card size stable, but expand node-to-node spacing and ring radii so straight
    // dependency lines do not all cut through the same central band.
    var gapX = nodeCount > 42 ? 184 : (nodeCount > 28 ? 206 : (nodeCount > 18 ? 226 : 246))
    var gapY = nodeCount > 42 ? 150 : (nodeCount > 28 ? 174 : (nodeCount > 18 ? 192 : 208))
    var marginX = 150
    var marginY = 140
    var sectionGap = 260
    var headerHeight = 34

    for (var nodeIndex = 0; nodeIndex < nodes.length; ++nodeIndex) {
        var node = nodes[nodeIndex]
        if (!node || node.id === undefined)
            continue
        var key = safeNodeId(node.id)
        nodeById[key] = node
        validNodes.push(node)
        outgoingById[key] = []
        incomingById[key] = []
        neighborById[key] = {}
        directedDegreeById[key] = 0
        degreeById[key] = 0
    }

    for (var edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
        var edge = edges[edgeIndex]
        if (!edge || edge.id === undefined)
            continue
        var fromKey = safeNodeId(edge.fromId)
        var toKey = safeNodeId(edge.toId)
        if (!nodeById[fromKey] || !nodeById[toKey] || fromKey === toKey)
            continue
        outgoingById[fromKey].push(edge)
        incomingById[toKey].push(edge)
        neighborById[fromKey][toKey] = true
        neighborById[toKey][fromKey] = true
        directedDegreeById[fromKey] = (directedDegreeById[fromKey] || 0) + 1
        directedDegreeById[toKey] = (directedDegreeById[toKey] || 0) + 1
    }

    for (nodeIndex = 0; nodeIndex < validNodes.length; ++nodeIndex) {
        node = validNodes[nodeIndex]
        key = safeNodeId(node.id)
        var neighborCount = 0
        var neighbors = neighborById[key] || {}
        for (var neighborKey in neighbors)
            ++neighborCount
        degreeById[key] = neighborCount
        incomingCountByNodeId[node.id] = incomingById[key] ? incomingById[key].length : 0
        outgoingCountByNodeId[node.id] = outgoingById[key] ? outgoingById[key].length : 0
        readableDebug("LAYOUT", "component-degree id=" + key
            + " name=" + safeNodeName(node)
            + " undirected=" + String(neighborCount)
            + " directed=" + String(directedDegreeById[key] || 0)
            + " incoming=" + String(incomingCountByNodeId[node.id])
            + " outgoing=" + String(outgoingCountByNodeId[node.id]))
    }

    function nodeTitle(node) {
        if (config.titleText)
            return String(config.titleText(node) || "")
        return safeNodeName(node)
    }

    function nodeScope(node) {
        if (config.scopeText)
            return String(config.scopeText(node) || "")
        return safeNodeName(node)
    }

    function nodeGroupKey(node) {
        if (config.groupKey)
            return String(config.groupKey(node) || "other")
        return "other"
    }

    function entryNameBonus(node) {
        var text = (nodeTitle(node) + " " + nodeScope(node) + " " + safeNodeName(node)).toLowerCase()
        var bonus = 0
        var terms = ["main", "index", "app", "entry", "surface", "canvas", "view", "page", "router", "controller", "service", "shell"]
        for (var i = 0; i < terms.length; ++i) {
            if (text.indexOf(terms[i]) >= 0)
                bonus += 18 - Math.min(12, i)
        }
        return bonus
    }

    function focusScore(node) {
        var nodeKey = safeNodeId(node.id)
        var connectivity = config.connectivityScore ? Number(config.connectivityScore(node) || 0) : 0
        return (degreeById[nodeKey] || 0) * 120
            + (directedDegreeById[nodeKey] || 0) * 16
            + (outgoingCountByNodeId[node.id] || 0) * 8
            + (incomingCountByNodeId[node.id] || 0) * 5
            + entryNameBonus(node)
            + connectivity * 0.4
    }

    var connectedNodes = []
    var isolatedNodes = []
    var focusNode = null
    var bestScore = -999999
    for (nodeIndex = 0; nodeIndex < validNodes.length; ++nodeIndex) {
        node = validNodes[nodeIndex]
        key = safeNodeId(node.id)
        if ((degreeById[key] || 0) > 0)
            connectedNodes.push(node)
        else
            isolatedNodes.push(node)
        var score = focusScore(node)
        if ((degreeById[key] || 0) > 0 && (!focusNode || score > bestScore + 0.01)) {
            focusNode = node
            bestScore = score
        }
    }
    if (!focusNode && validNodes.length)
        focusNode = validNodes[0]
    var focusKey = focusNode ? safeNodeId(focusNode.id) : ""
    readableDebug("LAYOUT", "component-default-focus id=" + focusKey
        + " name=" + String(focusNode ? safeNodeName(focusNode) : "")
        + " connected=" + String(connectedNodes.length)
        + " isolated=" + String(isolatedNodes.length))

    function bfsDistanceFromFocus() {
        var distance = {}
        if (!focusKey.length)
            return distance
        var queue = [focusKey]
        distance[focusKey] = 0
        for (var qi = 0; qi < queue.length; ++qi) {
            var current = queue[qi]
            var nextMap = neighborById[current] || {}
            for (var nextKey in nextMap) {
                if (distance[nextKey] !== undefined)
                    continue
                distance[nextKey] = distance[current] + 1
                queue.push(nextKey)
            }
        }
        return distance
    }

    var distanceById = bfsDistanceFromFocus()

    function compareDetailNodes(left, right) {
        var leftKey = safeNodeId(left.id)
        var rightKey = safeNodeId(right.id)
        var leftDist = distanceById[leftKey] !== undefined ? distanceById[leftKey] : 99
        var rightDist = distanceById[rightKey] !== undefined ? distanceById[rightKey] : 99
        if (leftDist !== rightDist)
            return leftDist - rightDist
        var leftGroup = nodeGroupKey(left)
        var rightGroup = nodeGroupKey(right)
        var groupCompare = leftGroup.localeCompare(rightGroup)
        if (groupCompare !== 0)
            return groupCompare
        var leftScore = focusScore(left)
        var rightScore = focusScore(right)
        if (Math.abs(leftScore - rightScore) > 0.01)
            return rightScore - leftScore
        return nodeTitle(left).localeCompare(nodeTitle(right))
    }

    connectedNodes.sort(compareDetailNodes)
    isolatedNodes.sort(function(left, right) {
        var leftGroup = nodeGroupKey(left)
        var rightGroup = nodeGroupKey(right)
        var groupCompare = leftGroup.localeCompare(rightGroup)
        if (groupCompare !== 0)
            return groupCompare
        return nodeTitle(left).localeCompare(nodeTitle(right))
    })

    var connectedPosition = {}
    var movableKeys = []
    var centerX = 0
    var centerY = 0
    if (focusNode) {
        connectedPosition[focusKey] = { "cx": centerX, "cy": centerY, "fixed": true }
        movableKeys.push(focusKey)
    }

    var ringNodes = []
    for (nodeIndex = 0; nodeIndex < connectedNodes.length; ++nodeIndex) {
        node = connectedNodes[nodeIndex]
        key = safeNodeId(node.id)
        if (key === focusKey)
            continue
        ringNodes.push(node)
    }

    var baseRadiusX = cardWidth * (connectedNodes.length > 35 ? 2.45 : 2.78) + gapX * 1.36
    var baseRadiusY = cardHeight * (connectedNodes.length > 35 ? 2.30 : 2.62) + gapY * 1.30
    var ringGapX = cardWidth * 1.94 + gapX * 1.48
    var ringGapY = cardHeight * 1.84 + gapY * 1.42
    var assigned = 0
    var ring = 1
    while (assigned < ringNodes.length) {
        var radiusX = baseRadiusX + (ring - 1) * ringGapX
        var radiusY = baseRadiusY + (ring - 1) * ringGapY
        var capacity = Math.max(6, Math.floor(2 * Math.PI * Math.sqrt(radiusX * radiusX + radiusY * radiusY) / (cardWidth * 1.68)))
        capacity = Math.min(capacity, ringNodes.length - assigned)
        if (ring === 1)
            capacity = Math.min(capacity, Math.max(6, Math.min(12, ringNodes.length - assigned)))
        var startAngle = -Math.PI / 2 - (ring % 2) * Math.PI / Math.max(6, capacity * 2)
        for (var ringIndex = 0; ringIndex < capacity; ++ringIndex) {
            node = ringNodes[assigned + ringIndex]
            key = safeNodeId(node.id)
            var angle = startAngle + (2 * Math.PI * ringIndex / Math.max(1, capacity))
            var dist = distanceById[key] !== undefined ? distanceById[key] : 99
            var jitter = ((safeNodeId(node.id).length % 5) - 2) * 0.045
            var cx = Math.cos(angle + jitter) * radiusX
            var cy = Math.sin(angle + jitter) * radiusY
            connectedPosition[key] = { "cx": cx, "cy": cy, "fixed": false }
            movableKeys.push(key)
            readableDebug("LAYOUT", "component-ring-node id=" + key
                + " name=" + safeNodeName(node)
                + " ring=" + String(ring)
                + " dist=" + String(dist)
                + " angle=" + String(Math.round(angle * 1000) / 1000)
                + " cx=" + String(Math.round(cx))
                + " cy=" + String(Math.round(cy)))
        }
        assigned += capacity
        ++ring
    }

    function connectedRect(keyValue) {
        var pos = connectedPosition[keyValue]
        return {
            "left": pos.cx - cardWidth / 2,
            "right": pos.cx + cardWidth / 2,
            "top": pos.cy - cardHeight / 2,
            "bottom": pos.cy + cardHeight / 2,
            "cx": pos.cx,
            "cy": pos.cy
        }
    }

    function resolveConnectedCollisions(iterations) {
        var minGapX = Math.max(148, gapX * 0.92)
        var minGapY = Math.max(118, gapY * 0.86)
        for (var iter = 0; iter < iterations; ++iter) {
            var moved = false
            for (var a = 0; a < movableKeys.length; ++a) {
                for (var b = a + 1; b < movableKeys.length; ++b) {
                    var leftKey = movableKeys[a]
                    var rightKey = movableKeys[b]
                    var leftPos = connectedPosition[leftKey]
                    var rightPos = connectedPosition[rightKey]
                    if (!leftPos || !rightPos)
                        continue
                    var dx = rightPos.cx - leftPos.cx
                    var dy = rightPos.cy - leftPos.cy
                    var requiredX = cardWidth + minGapX
                    var requiredY = cardHeight + minGapY
                    var overlapX = requiredX - Math.abs(dx)
                    var overlapY = requiredY - Math.abs(dy)
                    if (overlapX <= 0 || overlapY <= 0)
                        continue

                    var pushX = 0
                    var pushY = 0
                    if (overlapX < overlapY) {
                        pushX = (dx >= 0 ? 1 : -1) * (overlapX / 2 + 1)
                    } else {
                        pushY = (dy >= 0 ? 1 : -1) * (overlapY / 2 + 1)
                    }

                    if (leftPos.fixed) {
                        rightPos.cx += pushX * 2
                        rightPos.cy += pushY * 2
                    } else if (rightPos.fixed) {
                        leftPos.cx -= pushX * 2
                        leftPos.cy -= pushY * 2
                    } else {
                        leftPos.cx -= pushX
                        leftPos.cy -= pushY
                        rightPos.cx += pushX
                        rightPos.cy += pushY
                    }
                    moved = true
                }
            }
            if (!moved)
                break
        }
    }

    resolveConnectedCollisions(90)

    var connectedMinX = 0
    var connectedMinY = 0
    var connectedMaxX = 0
    var connectedMaxY = 0
    var hasConnectedBounds = false
    for (var posKey in connectedPosition) {
        var rect = connectedRect(posKey)
        if (!hasConnectedBounds) {
            connectedMinX = rect.left
            connectedMaxX = rect.right
            connectedMinY = rect.top
            connectedMaxY = rect.bottom
            hasConnectedBounds = true
        } else {
            connectedMinX = Math.min(connectedMinX, rect.left)
            connectedMaxX = Math.max(connectedMaxX, rect.right)
            connectedMinY = Math.min(connectedMinY, rect.top)
            connectedMaxY = Math.max(connectedMaxY, rect.bottom)
        }
    }

    function componentIsolatedColumnCount(count, connectedPresent) {
        if (count <= 0)
            return 0
        if (!connectedPresent) {
            if (count <= 2)
                return count
            if (count <= 8)
                return 3
            if (count <= 24)
                return 4
            return 5
        }
        if (count <= 3)
            return 1
        if (count <= 12)
            return 2
        return 3
    }

    var isolatedColumns = componentIsolatedColumnCount(isolatedNodes.length, hasConnectedBounds)
    var isolatedPanelWidth = isolatedColumns > 0
        ? isolatedColumns * cardWidth + Math.max(0, isolatedColumns - 1) * gapX + marginX
        : 0
    var connectedShiftX = marginX + (hasConnectedBounds ? isolatedPanelWidth : 0) + (hasConnectedBounds ? -connectedMinX : 0)
    var connectedShiftY = marginY + (hasConnectedBounds ? -connectedMinY : 0)

    for (posKey in connectedPosition) {
        var connectedNode = nodeById[posKey]
        if (!connectedNode)
            continue
        var finalX = connectedPosition[posKey].cx - cardWidth / 2 + connectedShiftX
        var finalY = connectedPosition[posKey].cy - cardHeight / 2 + connectedShiftY
        positions[posKey] = {
            "x": finalX,
            "y": finalY,
            "groupIndex": 1,
            "column": Math.round(finalX / Math.max(1, cardWidth + gapX)),
            "row": Math.round(finalY / Math.max(1, cardHeight + gapY)),
            "distance": distanceById[posKey] !== undefined ? distanceById[posKey] : 99,
            "defaultFocus": posKey === focusKey,
            "isolated": false
        }
        groupIndexByNodeId[connectedNode.id] = 1
        readableDebug("LAYOUT", "component-position id=" + posKey
            + " name=" + safeNodeName(connectedNode)
            + " x=" + String(Math.round(finalX))
            + " y=" + String(Math.round(finalY))
            + " focus=" + String(posKey === focusKey)
            + " distance=" + String(distanceById[posKey] !== undefined ? distanceById[posKey] : 99))
    }

    var isolatedRowsPerColumn = isolatedColumns > 0 ? Math.ceil(isolatedNodes.length / isolatedColumns) : 0
    for (nodeIndex = 0; nodeIndex < isolatedNodes.length; ++nodeIndex) {
        node = isolatedNodes[nodeIndex]
        key = safeNodeId(node.id)
        var isoColumn = isolatedColumns > 0 ? nodeIndex % isolatedColumns : 0
        var isoRow = isolatedColumns > 0 ? Math.floor(nodeIndex / isolatedColumns) : nodeIndex
        var isoX = marginX + isoColumn * (cardWidth + gapX)
        var isoY = marginY + isoRow * (cardHeight + gapY)
        positions[key] = {
            "x": isoX,
            "y": isoY,
            "groupIndex": 0,
            "column": isoColumn,
            "row": isoRow,
            "distance": 99,
            "defaultFocus": false,
            "isolated": true
        }
        groupIndexByNodeId[node.id] = 0
        readableDebug("LAYOUT", "component-isolated id=" + key
            + " name=" + safeNodeName(node)
            + " x=" + String(Math.round(isoX))
            + " y=" + String(Math.round(isoY)))
    }

    function sortPeerEdges(edgeList, peerField) {
        edgeList.sort(function(left, right) {
            var leftPeer = positions[safeNodeId(left[peerField])] || { "x": 0, "y": 0 }
            var rightPeer = positions[safeNodeId(right[peerField])] || { "x": 0, "y": 0 }
            if (Math.abs(leftPeer.y - rightPeer.y) > 0.5)
                return leftPeer.y - rightPeer.y
            if (Math.abs(leftPeer.x - rightPeer.x) > 0.5)
                return leftPeer.x - rightPeer.x
            return safeNodeId(left.id).localeCompare(safeNodeId(right.id))
        })
    }

    for (nodeIndex = 0; nodeIndex < validNodes.length; ++nodeIndex) {
        node = validNodes[nodeIndex]
        key = safeNodeId(node.id)
        var outgoingEdges = outgoingById[key] || []
        var incomingEdges = incomingById[key] || []
        sortPeerEdges(outgoingEdges, "toId")
        sortPeerEdges(incomingEdges, "fromId")
        outgoingCountByNodeId[node.id] = outgoingEdges.length
        incomingCountByNodeId[node.id] = incomingEdges.length
        for (var outgoingIndex = 0; outgoingIndex < outgoingEdges.length; ++outgoingIndex)
            outgoingSlotByEdgeId[outgoingEdges[outgoingIndex].id] = { "index": outgoingIndex, "count": outgoingEdges.length }
        for (var incomingIndex = 0; incomingIndex < incomingEdges.length; ++incomingIndex)
            incomingSlotByEdgeId[incomingEdges[incomingIndex].id] = { "index": incomingIndex, "count": incomingEdges.length }
        var placement = positions[key]
        if (placement) {
            outgoingBusXByNodeId[node.id] = placement.x + cardWidth + 48
            incomingBusXByNodeId[node.id] = placement.x - 48
        }
    }

    var maxX = marginX + cardWidth
    var maxY = marginY + cardHeight
    for (posKey in positions) {
        var p = positions[posKey]
        maxX = Math.max(maxX, Number(p.x) + cardWidth)
        maxY = Math.max(maxY, Number(p.y) + cardHeight)
    }
    var sceneWidth = Math.max(980, maxX + marginX)
    var sceneHeight = Math.max(560, maxY + marginY)

    var connectedGroup = { "key": "connected", "title": "关系图", "nodes": connectedNodes, "left": marginX + isolatedPanelWidth, "width": Math.max(1, sceneWidth - isolatedPanelWidth - marginX * 2), "columns": 1, "rows": 1 }
    var isolatedGroup = { "key": "isolated", "title": "无关系节点", "nodes": isolatedNodes, "left": marginX, "width": Math.max(cardWidth, isolatedPanelWidth), "columns": isolatedColumns, "rows": isolatedRowsPerColumn }
    var groups = isolatedNodes.length ? [isolatedGroup, connectedGroup] : [connectedGroup]
    groupBoundsByIndex[0] = { "left": marginX, "right": marginX + isolatedPanelWidth, "top": marginY, "bottom": sceneHeight - marginY }
    groupBoundsByIndex[1] = { "left": marginX + isolatedPanelWidth, "right": sceneWidth - marginX, "top": marginY, "bottom": sceneHeight - marginY }

    function routeTrackGroupKey(edge) {
        if (!edge)
            return ""
        var fromPlacement = positions[safeNodeId(edge.fromId)]
        var toPlacement = positions[safeNodeId(edge.toId)]
        if (!fromPlacement || !toPlacement)
            return ""
        var fx = fromPlacement.x + cardWidth / 2
        var fy = fromPlacement.y + cardHeight / 2
        var tx = toPlacement.x + cardWidth / 2
        var ty = toPlacement.y + cardHeight / 2
        return Math.abs(tx - fx) >= Math.abs(ty - fy)
            ? "h:" + String(Math.round((fy + ty) / 180))
            : "v:" + String(Math.round((fx + tx) / 220))
    }

    var routeTrackGroups = {}
    for (edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
        edge = edges[edgeIndex]
        if (!edge || edge.id === undefined)
            continue
        var routeKey = routeTrackGroupKey(edge)
        if (!routeKey.length)
            continue
        if (!routeTrackGroups[routeKey])
            routeTrackGroups[routeKey] = []
        routeTrackGroups[routeKey].push(edge)
    }

    for (var routeGroupKey in routeTrackGroups) {
        var members = routeTrackGroups[routeGroupKey]
        members.sort(function(left, right) {
            var leftFrom = positions[safeNodeId(left.fromId)] || { "x": 0, "y": 0 }
            var rightFrom = positions[safeNodeId(right.fromId)] || { "x": 0, "y": 0 }
            if (Math.abs(leftFrom.y - rightFrom.y) > 0.5)
                return leftFrom.y - rightFrom.y
            if (Math.abs(leftFrom.x - rightFrom.x) > 0.5)
                return leftFrom.x - rightFrom.x
            return safeNodeId(left.id).localeCompare(safeNodeId(right.id))
        })
        routeTrackCountByGroupKey[routeGroupKey] = members.length
        for (var trackIndex = 0; trackIndex < members.length; ++trackIndex) {
            routeTrackByEdgeId[members[trackIndex].id] = { "index": trackIndex, "count": members.length }
            routeTrackGroupKeyByEdgeId[members[trackIndex].id] = routeGroupKey
        }
    }

    readableDebug("LAYOUT", "component-detail-v9-complete focus=" + focusKey
        + " connected=" + String(connectedNodes.length)
        + " isolated=" + String(isolatedNodes.length)
        + " sceneWidth=" + String(Math.round(sceneWidth))
        + " sceneHeight=" + String(Math.round(sceneHeight)))

    return {
        "groups": groups,
        "positions": positions,
        "sceneWidth": sceneWidth,
        "sceneHeight": sceneHeight,
        "cardWidth": cardWidth,
        "cardHeight": cardHeight,
        "gapX": gapX,
        "gapY": gapY,
        "sectionGap": sectionGap,
        "headerHeight": headerHeight,
        "maxRows": Math.max(1, connectedNodes.length + isolatedNodes.length),
        "groupIndexByNodeId": groupIndexByNodeId,
        "outgoingSlotByEdgeId": outgoingSlotByEdgeId,
        "incomingSlotByEdgeId": incomingSlotByEdgeId,
        "outgoingCountByNodeId": outgoingCountByNodeId,
        "incomingCountByNodeId": incomingCountByNodeId,
        "outgoingBusXByNodeId": outgoingBusXByNodeId,
        "incomingBusXByNodeId": incomingBusXByNodeId,
        "fanOutBundleByEdgeId": fanOutBundleByEdgeId,
        "fanInBundleByEdgeId": fanInBundleByEdgeId,
        "routeTrackByEdgeId": routeTrackByEdgeId,
        "routeTrackGroupKeyByEdgeId": routeTrackGroupKeyByEdgeId,
        "routeTrackCountByGroupKey": routeTrackCountByGroupKey,
        "groupBoundsByIndex": groupBoundsByIndex,
        "readableDefaultFocusId": focusKey,
        "componentDetailGraph": true
    }
}
