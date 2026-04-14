import QtQuick
import "../logic/InspectorFormatter.js" as InspectorFormatter

QtObject {
    required property QtObject selectionState

    readonly property var selectedNode: selectionState.selectedInspectorNode
    readonly property var selectedEdge: selectionState.selectedInspectorEdge
    readonly property string kind: selectedEdge ? "edge" : (selectedNode ? "node" : "")
    readonly property var evidencePackage: selectedEdge
                                         ? (selectedEdge.evidence || ({}))
                                         : (selectedNode ? (selectedNode.evidence || ({})) : ({}))
    readonly property bool selectedNodeIsSingleFile: selectedNode
                                                     ? InspectorFormatter.isSingleFileNode(selectedNode)
                                                     : false
    readonly property string primaryFilePath: selectedNodeIsSingleFile
                                              ? InspectorFormatter.primaryFilePath(selectedNode)
                                              : ""

    readonly property string title: InspectorFormatter.inspectorTitle(selectedNode, selectedEdge)
    readonly property string subtitle: InspectorFormatter.inspectorSubtitle(selectedNode, selectedEdge)
    readonly property string nodeKindLabel: selectedNode ? InspectorFormatter.displayNodeKind(selectedNode.kind, selectedNode) : ""
    readonly property string nodeRoleLabel: selectedNode ? (selectedNode.role || "") : ""
    readonly property string nodeFileCountLabel: selectedNode ? ("文件 " + (selectedNode.fileCount || 0)) : ""
    readonly property string edgeKindLabel: selectedEdge ? (selectedEdge.kind || "") : ""
    readonly property string edgeWeightLabel: selectedEdge ? ("权重 " + (selectedEdge.weight || 0)) : ""

    readonly property var facts: evidencePackage.facts || []
    readonly property var rules: evidencePackage.rules || []
    readonly property var conclusions: evidencePackage.conclusions || []
    readonly property var sourceFiles: InspectorFormatter.keyFiles(selectedNode, evidencePackage)
    readonly property var sourceSymbols: InspectorFormatter.keySymbols(selectedNode, evidencePackage)
    readonly property var sourceModules: evidencePackage.modules || []
    readonly property var sourceRelationships: evidencePackage.relationships || []
    readonly property string confidenceLabel: evidencePackage.confidenceLabel || ""
    readonly property string confidenceReason: evidencePackage.confidenceReason || ""

    readonly property string importanceSummary: InspectorFormatter.importanceSummary(selectedNode, selectedEdge)
    readonly property string nextStepSummary: InspectorFormatter.nextStepSummary(selectedNode, selectedEdge, selectionState.componentViewActive)
    readonly property var relationshipItems: selectionState.selectedNodeRelationshipItems()
    readonly property var endpointNodes: selectionState.selectedEdgeEndpointNodes()
    readonly property bool supportsCodeContext: kind === "node" && !!selectedNode
}
