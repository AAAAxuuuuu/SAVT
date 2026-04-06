.pragma library

function idKey(value) {
    if (value === undefined || value === null)
        return ""
    return String(value)
}

function buildIndexes(nodes, edges) {
    var nodeIndex = {}
    var edgeIndex = {}
    var neighborIndex = {}
    var edgesByNodeIndex = {}
    var listNodes = nodes || []
    var listEdges = edges || []

    for (var i = 0; i < listNodes.length; ++i) {
        var node = listNodes[i]
        nodeIndex[idKey(node.id)] = node
    }

    for (var j = 0; j < listEdges.length; ++j) {
        var edge = listEdges[j]
        edgeIndex[idKey(edge.id)] = edge
        var fromKey = idKey(edge.fromId)
        var toKey = idKey(edge.toId)

        if (!neighborIndex[fromKey])
            neighborIndex[fromKey] = {}
        if (!neighborIndex[toKey])
            neighborIndex[toKey] = {}

        neighborIndex[fromKey][toKey] = true
        neighborIndex[toKey][fromKey] = true

        if (!edgesByNodeIndex[fromKey])
            edgesByNodeIndex[fromKey] = []
        if (!edgesByNodeIndex[toKey])
            edgesByNodeIndex[toKey] = []

        edgesByNodeIndex[fromKey].push(edge)
        edgesByNodeIndex[toKey].push(edge)
    }

    return {
        "nodeIndex": nodeIndex,
        "edgeIndex": edgeIndex,
        "neighborIndex": neighborIndex,
        "edgesByNodeIndex": edgesByNodeIndex
    }
}

function nodeDisplayRect(node) {
    if (!node)
        return {"x": 0, "y": 0, "width": 0, "height": 0}
    if (node.layoutBounds)
        return node.layoutBounds
    return {
        "x": node.x || 0,
        "y": node.y || 0,
        "width": node.width || 0,
        "height": node.height || 0
    }
}

function sceneBounds(scene) {
    if (scene && scene.bounds)
        return scene.bounds
    return {"x": 0, "y": 0, "width": 0, "height": 0}
}

function nodeMatchesFilter(node, query) {
    var normalized = (query || "").trim().toLowerCase()
    if (normalized.length === 0)
        return true
    var haystack = ((node.name || "") + " " +
                    (node.role || "") + " " +
                    ((node.moduleNames || []).join(" ")) + " " +
                    ((node.topSymbols || []).join(" ")) + " " +
                    (node.summary || "")).toLowerCase()
    return haystack.indexOf(normalized) >= 0
}

function capabilityNodeOpacity(node, selectedNode, selectedEdge, neighborIndex) {
    if (!node)
        return 1.0
    if (selectedEdge && selectedEdge.id !== undefined)
        return selectedEdge.fromId === node.id || selectedEdge.toId === node.id ? 1.0 : 0.32
    if (!selectedNode || selectedNode.id === undefined)
        return 1.0
    if (selectedNode.id === node.id)
        return 1.0
    var neighbors = neighborIndex[idKey(selectedNode.id)] || {}
    return neighbors[idKey(node.id)] ? 1.0 : 0.32
}

function componentNodeOpacity(node, selectedNode, selectedEdge, neighborIndex, query) {
    if (!node)
        return 1.0
    if (!nodeMatchesFilter(node, query))
        return 0.1
    if (selectedEdge && selectedEdge.id !== undefined)
        return selectedEdge.fromId === node.id || selectedEdge.toId === node.id ? 1.0 : 0.28
    if (!selectedNode || selectedNode.id === undefined)
        return 1.0
    if (selectedNode.id === node.id)
        return 1.0
    var neighbors = neighborIndex[idKey(selectedNode.id)] || {}
    return neighbors[idKey(node.id)] ? 1.0 : 0.34
}

function visibleCapabilityEdges(edges, edgesByNodeIndex, selectedNode, selectedEdge, viewMode) {
    var listEdges = edges || []
    if (viewMode === "all")
        return listEdges.slice(0, Math.min(listEdges.length, 220))
    if (selectedEdge && selectedEdge.id !== undefined)
        return [selectedEdge]
    if (!selectedNode || selectedNode.id === undefined)
        return []
    return edgesByNodeIndex[idKey(selectedNode.id)] || []
}

function visibleComponentEdges(edges, edgesByNodeIndex, nodes, selectedNode, selectedEdge, viewMode, query) {
    var filteredNodeIds = {}
    var nodeList = nodes || []
    var edgeList = edges || []

    for (var i = 0; i < nodeList.length; ++i) {
        var node = nodeList[i]
        if (nodeMatchesFilter(node, query))
            filteredNodeIds[idKey(node.id)] = true
    }

    if (viewMode === "all") {
        return edgeList.filter(function(edge) {
            return filteredNodeIds[idKey(edge.fromId)] && filteredNodeIds[idKey(edge.toId)]
        }).slice(0, Math.min(edgeList.length, 260))
    }

    if (selectedEdge && selectedEdge.id !== undefined)
        return [selectedEdge]
    if (!selectedNode || selectedNode.id === undefined)
        return []

    return (edgesByNodeIndex[idKey(selectedNode.id)] || []).filter(function(edge) {
        return filteredNodeIds[idKey(edge.fromId)] && filteredNodeIds[idKey(edge.toId)]
    })
}

function relationshipItemsForNode(node, edgesByNodeIndex) {
    if (!node || node.id === undefined)
        return []
    var items = (edgesByNodeIndex[idKey(node.id)] || []).slice()
    items.sort(function(left, right) {
        var leftWeight = left.weight || 0
        var rightWeight = right.weight || 0
        if (leftWeight !== rightWeight)
            return rightWeight - leftWeight
        var leftSummary = left.summary || ""
        var rightSummary = right.summary || ""
        if (leftSummary < rightSummary)
            return -1
        if (leftSummary > rightSummary)
            return 1
        return 0
    })
    return items
}

function endpointNodes(edge, nodeIndex) {
    if (!edge)
        return []
    var items = []
    var fromNode = nodeIndex[idKey(edge.fromId)] || null
    var toNode = nodeIndex[idKey(edge.toId)] || null
    if (fromNode)
        items.push(fromNode)
    if (toNode)
        items.push(toNode)
    return items
}
