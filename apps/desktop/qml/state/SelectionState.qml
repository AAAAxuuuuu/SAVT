import QtQuick
import "../logic/GraphSelection.js" as GraphSelection
import "../logic/InspectorFormatter.js" as InspectorFormatter

QtObject {
    id: root

    property var capabilityScene: ({})
    property var componentSceneCatalog: ({})
    property string pageId: "project.overview"

    property string selectedCapabilityNodeId: ""
    property string selectedCapabilityEdgeId: ""
    property string selectedComponentNodeId: ""
    property string selectedComponentEdgeId: ""

    property string relationshipViewMode: "focused"
    property string componentRelationshipViewMode: "focused"
    property string componentFilterText: ""

    readonly property bool componentViewActive: pageId === "project.componentWorkbench" || pageId === "levels.l3"

    readonly property var capabilityNodes: capabilityScene.nodes || []
    readonly property var capabilityEdges: capabilityScene.edges || []
    readonly property var capabilityGroups: capabilityScene.groups || []
    readonly property var capabilityIndexBundle: GraphSelection.buildIndexes(capabilityNodes, capabilityEdges)
    readonly property var capabilityNodeIndex: capabilityIndexBundle.nodeIndex
    readonly property var capabilityEdgeIndex: capabilityIndexBundle.edgeIndex
    readonly property var capabilityNeighborIndex: capabilityIndexBundle.neighborIndex
    readonly property var capabilityEdgesByNodeIndex: capabilityIndexBundle.edgesByNodeIndex

    readonly property var selectedCapabilityNode: capabilityNodeIndex[selectedCapabilityNodeId] || null
    readonly property var selectedCapabilityEdge: capabilityEdgeIndex[selectedCapabilityEdgeId] || null

    readonly property var selectedCapabilityComponentScene: selectedCapabilityNode && selectedCapabilityNode.id !== undefined
                                                          ? (componentSceneCatalog[GraphSelection.idKey(selectedCapabilityNode.id)] || ({}))
                                                          : ({})
    readonly property var componentNodes: selectedCapabilityComponentScene.nodes || []
    readonly property var componentEdges: selectedCapabilityComponentScene.edges || []
    readonly property var componentGroups: selectedCapabilityComponentScene.groups || []
    readonly property var componentIndexBundle: GraphSelection.buildIndexes(componentNodes, componentEdges)
    readonly property var componentNodeIndex: componentIndexBundle.nodeIndex
    readonly property var componentEdgeIndex: componentIndexBundle.edgeIndex
    readonly property var componentNeighborIndex: componentIndexBundle.neighborIndex
    readonly property var componentEdgesByNodeIndex: componentIndexBundle.edgesByNodeIndex

    readonly property var selectedComponentNode: componentNodeIndex[selectedComponentNodeId] || null
    readonly property var selectedComponentEdge: componentEdgeIndex[selectedComponentEdgeId] || null

    readonly property var selectedInspectorNode: componentViewActive && selectedComponentNode
                                                 ? selectedComponentNode
                                                 : selectedCapabilityNode
    readonly property var selectedInspectorEdge: componentViewActive && selectedComponentEdge
                                                 ? selectedComponentEdge
                                                 : selectedCapabilityEdge

    function capabilityNodeById(nodeId) {
        return capabilityNodeIndex[GraphSelection.idKey(nodeId)] || null
    }

    function capabilityEdgeById(edgeId) {
        return capabilityEdgeIndex[GraphSelection.idKey(edgeId)] || null
    }

    function componentNodeById(nodeId) {
        return componentNodeIndex[GraphSelection.idKey(nodeId)] || null
    }

    function componentEdgeById(edgeId) {
        return componentEdgeIndex[GraphSelection.idKey(edgeId)] || null
    }

    function selectCapabilityNode(node) {
        if (!node || node.id === undefined)
            return
        selectedCapabilityNodeId = GraphSelection.idKey(node.id)
        selectedCapabilityEdgeId = ""
        selectedComponentNodeId = ""
        selectedComponentEdgeId = ""
    }

    function selectCapabilityEdge(edge) {
        if (!edge || edge.id === undefined)
            return
        selectedCapabilityEdgeId = GraphSelection.idKey(edge.id)
        selectedCapabilityNodeId = ""
        selectedComponentNodeId = ""
        selectedComponentEdgeId = ""
    }

    function selectComponentNode(node) {
        if (!node || node.id === undefined)
            return
        selectedComponentNodeId = GraphSelection.idKey(node.id)
        selectedComponentEdgeId = ""
    }

    function selectComponentEdge(edge) {
        if (!edge || edge.id === undefined)
            return
        selectedComponentEdgeId = GraphSelection.idKey(edge.id)
        selectedComponentNodeId = ""
    }

    function clearComponentFocus() {
        selectedComponentNodeId = ""
        selectedComponentEdgeId = ""
    }

    function clearSelection() {
        selectedCapabilityNodeId = ""
        selectedCapabilityEdgeId = ""
        selectedComponentNodeId = ""
        selectedComponentEdgeId = ""
    }

    function selectInspectorEndpointNode(node) {
        if (componentViewActive && componentNodeById(node.id)) {
            selectComponentNode(node)
            return
        }
        if (capabilityNodeById(node.id))
            selectCapabilityNode(node)
    }

    function selectInspectorRelationshipEdge(edge) {
        if (componentViewActive && componentEdgeById(edge.id)) {
            selectComponentEdge(edge)
            return
        }
        if (capabilityEdgeById(edge.id))
            selectCapabilityEdge(edge)
    }

    function drillIntoCapabilityNode(node) {
        selectCapabilityNode(node)
        componentFilterText = ""
    }

    function displayNodeKind(kind) {
        return InspectorFormatter.displayNodeKind(kind)
    }

    function nodeTooltipSummary(node) {
        if (!node)
            return ""
        return (node.summary || "").length > 0 ? node.summary : (node.name || "")
    }

    function nodeDisplayRect(node) {
        return GraphSelection.nodeDisplayRect(node)
    }

    function capabilitySceneBounds() {
        return GraphSelection.sceneBounds(capabilityScene)
    }

    function componentSceneBounds() {
        return GraphSelection.sceneBounds(selectedCapabilityComponentScene)
    }

    function selectedComponentSceneTitle() {
        return selectedCapabilityComponentScene.title || (selectedCapabilityNode ? selectedCapabilityNode.name : "组件工作台")
    }

    function selectedComponentSceneSummary() {
        return selectedCapabilityComponentScene.summary || ""
    }

    function componentSceneDiagnostics() {
        return selectedCapabilityComponentScene.diagnostics || []
    }

    function currentCapabilitySceneWidth() {
        return Math.max(capabilitySceneBounds().width || 0, 1080)
    }

    function currentCapabilitySceneHeight() {
        return Math.max(capabilitySceneBounds().height || 0, 560)
    }

    function currentComponentSceneWidth() {
        return Math.max(componentSceneBounds().width || 0, 920)
    }

    function currentComponentSceneHeight() {
        return Math.max(componentSceneBounds().height || 0, 460)
    }

    function nodeShouldBeDisplayed(nodeId) {
        return capabilityNodeById(nodeId) !== null
    }

    function componentNodeShouldBeDisplayed(nodeId) {
        var node = componentNodeById(nodeId)
        return node !== null && GraphSelection.nodeMatchesFilter(node, componentFilterText)
    }

    function capabilityNodeOpacity(node) {
        return GraphSelection.capabilityNodeOpacity(node, selectedCapabilityNode, selectedCapabilityEdge, capabilityNeighborIndex)
    }

    function componentNodeOpacity(node) {
        return GraphSelection.componentNodeOpacity(node, selectedComponentNode, selectedComponentEdge, componentNeighborIndex, componentFilterText)
    }

    function visibleCapabilityEdges() {
        return GraphSelection.visibleCapabilityEdges(capabilityEdges, capabilityEdgesByNodeIndex, selectedCapabilityNode, selectedCapabilityEdge, relationshipViewMode)
    }

    function visibleComponentEdges() {
        return GraphSelection.visibleComponentEdges(componentEdges, componentEdgesByNodeIndex, componentNodes, selectedComponentNode, selectedComponentEdge, componentRelationshipViewMode, componentFilterText)
    }

    function selectedNodeRelationshipItems() {
        if (componentViewActive && selectedComponentNode)
            return GraphSelection.relationshipItemsForNode(selectedComponentNode, componentEdgesByNodeIndex)
        if (selectedCapabilityNode)
            return GraphSelection.relationshipItemsForNode(selectedCapabilityNode, capabilityEdgesByNodeIndex)
        return []
    }

    function selectedEdgeEndpointNodes() {
        if (componentViewActive && selectedComponentEdge)
            return GraphSelection.endpointNodes(selectedComponentEdge, componentNodeIndex)
        if (selectedCapabilityEdge)
            return GraphSelection.endpointNodes(selectedCapabilityEdge, capabilityNodeIndex)
        return []
    }

    onSelectedCapabilityNodeIdChanged: {
        selectedComponentNodeId = ""
        selectedComponentEdgeId = ""
    }

    onComponentNodesChanged: {
        if (selectedComponentNodeId.length === 0 &&
                selectedComponentEdgeId.length === 0 &&
                componentNodes.length === 1) {
            selectedComponentNodeId = GraphSelection.idKey(componentNodes[0].id)
        }
    }
}
