#include "savt/ui/SceneMapper.h"

#include <QVariantMap>

#include <unordered_map>
#include <vector>

namespace savt::ui {

namespace {

QVariantList toVariantStringList(const std::vector<std::string>& values) {
    QVariantList items;
    items.reserve(static_cast<qsizetype>(values.size()));
    for (const std::string& value : values) {
        items.push_back(QString::fromStdString(value));
    }
    return items;
}

QVariantList toVariantPointList(const std::vector<savt::layout::ScenePoint>& points) {
    QVariantList items;
    items.reserve(static_cast<qsizetype>(points.size()));
    for (const savt::layout::ScenePoint& point : points) {
        QVariantMap item;
        item.insert(QStringLiteral("x"), point.x);
        item.insert(QStringLiteral("y"), point.y);
        items.push_back(item);
    }
    return items;
}

QVariantMap toVariantRect(const double x, const double y, const double width, const double height) {
    QVariantMap item;
    item.insert(QStringLiteral("x"), x);
    item.insert(QStringLiteral("y"), y);
    item.insert(QStringLiteral("width"), width);
    item.insert(QStringLiteral("height"), height);
    return item;
}

QVariantMap toVariantEvidencePackage(const core::CapabilityNode::EvidencePackage& evidence) {
    QVariantMap item;
    item.insert(QStringLiteral("facts"), toVariantStringList(evidence.facts));
    item.insert(QStringLiteral("rules"), toVariantStringList(evidence.rules));
    item.insert(QStringLiteral("conclusions"), toVariantStringList(evidence.conclusions));
    item.insert(QStringLiteral("sourceFiles"), toVariantStringList(evidence.sourceFiles));
    item.insert(QStringLiteral("symbols"), toVariantStringList(evidence.symbols));
    item.insert(QStringLiteral("modules"), toVariantStringList(evidence.modules));
    item.insert(QStringLiteral("relationships"), toVariantStringList(evidence.relationships));
    item.insert(QStringLiteral("confidenceLabel"), QString::fromStdString(evidence.confidenceLabel));
    item.insert(QStringLiteral("confidenceReason"), QString::fromStdString(evidence.confidenceReason));
    return item;
}

}  // namespace

QVariantMap SceneMapper::toVariantMap(const CapabilitySceneData& scene) {
    QVariantMap sceneMap;
    sceneMap.insert(QStringLiteral("nodes"), scene.nodeItems);
    sceneMap.insert(QStringLiteral("edges"), scene.edgeItems);
    sceneMap.insert(QStringLiteral("groups"), scene.groupItems);
    sceneMap.insert(QStringLiteral("bounds"), toVariantRect(0.0, 0.0, scene.sceneWidth, scene.sceneHeight));
    return sceneMap;
}

CapabilitySceneData SceneMapper::buildCapabilitySceneData(
    const core::CapabilityGraph& graph,
    const layout::CapabilitySceneLayoutResult& layout) {
    CapabilitySceneData scene;
    scene.sceneWidth = layout.width;
    scene.sceneHeight = layout.height;

    std::unordered_map<std::size_t, const core::CapabilityNode*> nodeIndex;
    nodeIndex.reserve(graph.nodes.size());
    for (const core::CapabilityNode& node : graph.nodes) {
        nodeIndex.emplace(node.id, &node);
    }

    std::unordered_map<std::size_t, const core::CapabilityEdge*> edgeIndex;
    edgeIndex.reserve(graph.edges.size());
    for (const core::CapabilityEdge& edge : graph.edges) {
        edgeIndex.emplace(edge.id, &edge);
    }

    std::unordered_map<std::size_t, const core::CapabilityGroup*> groupIndex;
    groupIndex.reserve(graph.groups.size());
    for (const core::CapabilityGroup& group : graph.groups) {
        groupIndex.emplace(group.id, &group);
    }

    for (const savt::layout::CapabilitySceneNodeLayout& nodeLayout : layout.nodes) {
        const auto nodeIt = nodeIndex.find(nodeLayout.nodeId);
        if (nodeIt == nodeIndex.end()) {
            continue;
        }
        const core::CapabilityNode& node = *nodeIt->second;

        QVariantMap item;
        item.insert(QStringLiteral("id"), static_cast<qulonglong>(node.id));
        item.insert(QStringLiteral("kind"), QString::fromStdString(core::toString(node.kind)));
        item.insert(QStringLiteral("name"), QString::fromStdString(node.name));
        item.insert(QStringLiteral("role"), QString::fromStdString(node.dominantRole));
        item.insert(QStringLiteral("responsibility"), QString::fromStdString(node.responsibility));
        item.insert(QStringLiteral("summary"), QString::fromStdString(node.summary));
        item.insert(QStringLiteral("folderHint"), QString::fromStdString(node.folderHint));
        item.insert(QStringLiteral("moduleNames"), toVariantStringList(node.moduleNames));
        item.insert(QStringLiteral("exampleFiles"), toVariantStringList(node.exampleFiles));
        item.insert(QStringLiteral("topSymbols"), toVariantStringList(node.topSymbols));
        item.insert(QStringLiteral("collaboratorNames"), toVariantStringList(node.collaboratorNames));
        item.insert(QStringLiteral("supportingRoles"), toVariantStringList(node.supportingRoles));
        item.insert(QStringLiteral("technologyTags"), toVariantStringList(node.technologyTags));
        item.insert(QStringLiteral("riskSignals"), toVariantStringList(node.riskSignals));
        item.insert(QStringLiteral("moduleCount"), static_cast<qulonglong>(node.moduleCount));
        item.insert(QStringLiteral("fileCount"), static_cast<qulonglong>(node.fileCount));
        item.insert(QStringLiteral("sourceFileCount"), static_cast<qulonglong>(node.sourceFileCount));
        item.insert(QStringLiteral("qmlFileCount"), static_cast<qulonglong>(node.qmlFileCount));
        item.insert(QStringLiteral("webFileCount"), static_cast<qulonglong>(node.webFileCount));
        item.insert(QStringLiteral("scriptFileCount"), static_cast<qulonglong>(node.scriptFileCount));
        item.insert(QStringLiteral("dataFileCount"), static_cast<qulonglong>(node.dataFileCount));
        item.insert(QStringLiteral("stageStart"), static_cast<qulonglong>(node.stageStart));
        item.insert(QStringLiteral("stageEnd"), static_cast<qulonglong>(node.stageEnd));
        item.insert(QStringLiteral("incomingEdgeCount"), static_cast<qulonglong>(node.incomingEdgeCount));
        item.insert(QStringLiteral("outgoingEdgeCount"), static_cast<qulonglong>(node.outgoingEdgeCount));
        item.insert(QStringLiteral("aggregatedDependencyWeight"), static_cast<qulonglong>(node.aggregatedDependencyWeight));
        item.insert(QStringLiteral("flowParticipationCount"), static_cast<qulonglong>(node.flowParticipationCount));
        item.insert(QStringLiteral("collaboratorCount"), static_cast<qulonglong>(node.collaboratorCount));
        item.insert(QStringLiteral("riskScore"), static_cast<qulonglong>(node.riskScore));
        item.insert(QStringLiteral("riskLevel"), QString::fromStdString(node.riskLevel));
        item.insert(QStringLiteral("priority"), static_cast<qulonglong>(node.visualPriority));
        item.insert(QStringLiteral("pinned"), node.defaultPinned);
        item.insert(QStringLiteral("collapsed"), node.defaultCollapsed);
        item.insert(QStringLiteral("reachableFromEntry"), node.reachableFromEntry);
        item.insert(QStringLiteral("cycleParticipant"), node.cycleParticipant);
        item.insert(QStringLiteral("laneGroupId"), static_cast<qulonglong>(node.laneGroupId));
        item.insert(QStringLiteral("detailGroupId"), static_cast<qulonglong>(node.detailGroupId));
        item.insert(QStringLiteral("importanceScale"), nodeLayout.importanceScale);
        item.insert(QStringLiteral("layoutLaneIndex"), static_cast<qulonglong>(nodeLayout.laneIndex));
        item.insert(QStringLiteral("layoutOrder"), static_cast<qulonglong>(nodeLayout.orderInLane));
        item.insert(QStringLiteral("x"), nodeLayout.x);
        item.insert(QStringLiteral("y"), nodeLayout.y);
        item.insert(QStringLiteral("width"), nodeLayout.width);
        item.insert(QStringLiteral("height"), nodeLayout.height);
        item.insert(QStringLiteral("layoutBounds"), toVariantRect(nodeLayout.x, nodeLayout.y, nodeLayout.width, nodeLayout.height));
        item.insert(QStringLiteral("evidence"), toVariantEvidencePackage(node.evidence));
        scene.nodeItems.push_back(item);
    }

    for (const savt::layout::CapabilitySceneEdgeLayout& edgeLayout : layout.edges) {
        const auto edgeIt = edgeIndex.find(edgeLayout.edgeId);
        const auto fromNodeIt = nodeIndex.find(edgeLayout.fromId);
        const auto toNodeIt = nodeIndex.find(edgeLayout.toId);
        if (edgeIt == edgeIndex.end() || fromNodeIt == nodeIndex.end() || toNodeIt == nodeIndex.end()) {
            continue;
        }

        QVariantMap item;
        item.insert(QStringLiteral("id"), static_cast<qulonglong>(edgeLayout.edgeId));
        item.insert(QStringLiteral("fromId"), static_cast<qulonglong>(edgeLayout.fromId));
        item.insert(QStringLiteral("toId"), static_cast<qulonglong>(edgeLayout.toId));
        item.insert(QStringLiteral("kind"), QString::fromStdString(core::toString(edgeIt->second->kind)));
        item.insert(QStringLiteral("summary"), QString::fromStdString(edgeIt->second->summary));
        item.insert(QStringLiteral("weight"), static_cast<qulonglong>(edgeIt->second->weight));
        item.insert(QStringLiteral("fromName"), QString::fromStdString(fromNodeIt->second->name));
        item.insert(QStringLiteral("toName"), QString::fromStdString(toNodeIt->second->name));
        item.insert(QStringLiteral("routePoints"), toVariantPointList(edgeLayout.routePoints));
        item.insert(QStringLiteral("routePointCount"), static_cast<qulonglong>(edgeLayout.routePoints.size()));
        if (!edgeLayout.routePoints.empty()) {
            item.insert(QStringLiteral("x1"), edgeLayout.routePoints.front().x);
            item.insert(QStringLiteral("y1"), edgeLayout.routePoints.front().y);
            item.insert(QStringLiteral("x2"), edgeLayout.routePoints.back().x);
            item.insert(QStringLiteral("y2"), edgeLayout.routePoints.back().y);
        }
        item.insert(QStringLiteral("evidence"), toVariantEvidencePackage(edgeIt->second->evidence));
        scene.edgeItems.push_back(item);
    }

    for (const savt::layout::CapabilitySceneGroupLayout& groupLayout : layout.groups) {
        const auto groupIt = groupIndex.find(groupLayout.groupId);
        if (groupIt == groupIndex.end()) {
            continue;
        }
        const core::CapabilityGroup& group = *groupIt->second;

        QVariantMap item;
        item.insert(QStringLiteral("id"), static_cast<qulonglong>(group.id));
        item.insert(QStringLiteral("kind"), QString::fromStdString(core::toString(group.kind)));
        item.insert(QStringLiteral("name"), QString::fromStdString(group.name));
        item.insert(QStringLiteral("summary"), QString::fromStdString(group.summary));
        item.insert(QStringLiteral("expanded"), group.defaultExpanded);
        item.insert(QStringLiteral("visible"), group.defaultVisible);
        item.insert(QStringLiteral("hiddenCount"), static_cast<qulonglong>(group.hiddenNodeCount));
        item.insert(QStringLiteral("nodeCount"), static_cast<qulonglong>(group.nodeIds.size()));
        item.insert(QStringLiteral("layoutVisibleNodeCount"), static_cast<qulonglong>(groupLayout.visibleNodeIds.size()));
        item.insert(QStringLiteral("hasBounds"), groupLayout.hasBounds);
        item.insert(QStringLiteral("x"), groupLayout.x);
        item.insert(QStringLiteral("y"), groupLayout.y);
        item.insert(QStringLiteral("width"), groupLayout.width);
        item.insert(QStringLiteral("height"), groupLayout.height);
        item.insert(QStringLiteral("layoutBounds"), toVariantRect(groupLayout.x, groupLayout.y, groupLayout.width, groupLayout.height));
        QVariantList visibleNodeIds;
        visibleNodeIds.reserve(static_cast<qsizetype>(groupLayout.visibleNodeIds.size()));
        for (const std::size_t nodeId : groupLayout.visibleNodeIds) {
            visibleNodeIds.push_back(static_cast<qulonglong>(nodeId));
        }
        item.insert(QStringLiteral("layoutVisibleNodeIds"), visibleNodeIds);
        scene.groupItems.push_back(item);
    }

    return scene;
}

}  // namespace savt::ui
