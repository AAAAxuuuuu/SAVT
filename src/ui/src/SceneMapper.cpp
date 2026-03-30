#include "savt/ui/SceneMapper.h"

#include <QVariantMap>

#include <unordered_map>
#include <vector>

namespace savt::ui {

namespace {

constexpr std::size_t kMaxDetailedEdgeRouteCount = 72;

bool shouldIncludeDetailedEdgeRoutes(
    const layout::CapabilitySceneLayoutResult& layout) {
    return layout.edges.size() <= kMaxDetailedEdgeRouteCount;
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
    const bool includeDetailedEdgeRoutes = shouldIncludeDetailedEdgeRoutes(layout);

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
        item.insert(QStringLiteral("fileCount"), static_cast<qulonglong>(node.fileCount));
        item.insert(QStringLiteral("pinned"), node.defaultPinned);
        item.insert(QStringLiteral("x"), nodeLayout.x);
        item.insert(QStringLiteral("y"), nodeLayout.y);
        item.insert(QStringLiteral("width"), nodeLayout.width);
        item.insert(QStringLiteral("height"), nodeLayout.height);
        item.insert(
            QStringLiteral("layoutBounds"),
            toVariantRect(nodeLayout.x, nodeLayout.y, nodeLayout.width, nodeLayout.height));
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
        item.insert(QStringLiteral("weight"), static_cast<qulonglong>(edgeIt->second->weight));
        if (includeDetailedEdgeRoutes && !edgeLayout.routePoints.empty()) {
            item.insert(QStringLiteral("routePoints"), toVariantPointList(edgeLayout.routePoints));
        }
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
        item.insert(QStringLiteral("hasBounds"), groupLayout.hasBounds);
        item.insert(QStringLiteral("x"), groupLayout.x);
        item.insert(QStringLiteral("y"), groupLayout.y);
        item.insert(QStringLiteral("width"), groupLayout.width);
        item.insert(QStringLiteral("height"), groupLayout.height);
        item.insert(
            QStringLiteral("layoutBounds"),
            toVariantRect(groupLayout.x, groupLayout.y, groupLayout.width, groupLayout.height));
        scene.groupItems.push_back(item);
    }

    return scene;
}

}  // namespace savt::ui
