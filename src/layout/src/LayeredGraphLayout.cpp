#include "savt/layout/LayeredGraphLayout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace savt::layout {
namespace {

struct ModuleNodeInfo {
    std::size_t id = 0;
    std::string name;
    std::size_t indegree = 0;
    std::size_t outdegree = 0;
};

struct SceneRect {
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

struct CapabilityNodePlacement {
    SceneRect rect;
    std::size_t laneIndex = 0;
    std::size_t orderInLane = 0;
};

int laneIndexForNode(const savt::core::CapabilityNode& node) {
    switch (node.kind) {
    case savt::core::CapabilityNodeKind::Entry:
        return 0;
    case savt::core::CapabilityNodeKind::Capability:
        return 1;
    case savt::core::CapabilityNodeKind::Infrastructure:
        return 2;
    }
    return 1;
}

double importanceScaleForNode(
    const savt::core::CapabilityNode& node,
    const CapabilitySceneLayoutOptions& options) {
    if (options.collaboratorScaleDivisor <= 0.0) {
        return 1.0;
    }
    const double normalized =
        std::min(1.0, static_cast<double>(node.collaboratorCount) /
                          options.collaboratorScaleDivisor);
    return 1.0 + normalized * options.maxImportanceBoost;
}

double rectLeft(const SceneRect& rect) {
    return rect.x;
}

double rectRight(const SceneRect& rect) {
    return rect.x + rect.width;
}

double rectMidY(const SceneRect& rect) {
    return rect.y + rect.height * 0.5;
}

void appendRoutePoint(
    std::vector<ScenePoint>& points,
    const double x,
    const double y) {
    if (!points.empty()) {
        const ScenePoint& last = points.back();
        if (std::abs(last.x - x) < 0.5 && std::abs(last.y - y) < 0.5) {
            return;
        }
    }
    points.push_back(ScenePoint{x, y});
}

std::vector<ScenePoint> capabilityEdgeRoute(
    const CapabilityNodePlacement& fromPlacement,
    const CapabilityNodePlacement& toPlacement,
    const CapabilitySceneLayoutOptions& options) {
    std::vector<ScenePoint> points;
    const double fromMidY = rectMidY(fromPlacement.rect);
    const double toMidY = rectMidY(toPlacement.rect);

    if (fromPlacement.rect.x == toPlacement.rect.x &&
        fromPlacement.rect.y == toPlacement.rect.y &&
        fromPlacement.rect.width == toPlacement.rect.width &&
        fromPlacement.rect.height == toPlacement.rect.height) {
        const double startX = rectRight(fromPlacement.rect);
        const double railX = startX + options.sameLaneRailOffset;
        const double loopY = fromMidY + std::max(18.0, fromPlacement.rect.height * 0.28);
        appendRoutePoint(points, startX, fromMidY);
        appendRoutePoint(points, railX, fromMidY);
        appendRoutePoint(points, railX, loopY);
        appendRoutePoint(points, startX, loopY);
        return points;
    }

    if (toPlacement.laneIndex < fromPlacement.laneIndex) {
        const double startX = rectLeft(fromPlacement.rect);
        const double railX = startX - options.edgeStub;
        const double endX = rectRight(toPlacement.rect);
        appendRoutePoint(points, startX, fromMidY);
        appendRoutePoint(points, railX, fromMidY);
        appendRoutePoint(points, railX, toMidY);
        appendRoutePoint(points, endX, toMidY);
        return points;
    }

    if (toPlacement.laneIndex > fromPlacement.laneIndex) {
        const double startX = rectRight(fromPlacement.rect);
        const double railX = startX + options.edgeStub;
        const double endX = rectLeft(toPlacement.rect);
        appendRoutePoint(points, startX, fromMidY);
        appendRoutePoint(points, railX, fromMidY);
        appendRoutePoint(points, railX, toMidY);
        appendRoutePoint(points, endX, toMidY);
        return points;
    }

    const bool useLeftRail = toPlacement.orderInLane < fromPlacement.orderInLane;
    const double startX = useLeftRail ? rectLeft(fromPlacement.rect) : rectRight(fromPlacement.rect);
    const double endX = useLeftRail ? rectLeft(toPlacement.rect) : rectRight(toPlacement.rect);
    const double railX = useLeftRail
                             ? std::min(rectLeft(fromPlacement.rect), rectLeft(toPlacement.rect)) - options.sameLaneRailOffset
                             : std::max(rectRight(fromPlacement.rect), rectRight(toPlacement.rect)) + options.sameLaneRailOffset;
    appendRoutePoint(points, startX, fromMidY);
    appendRoutePoint(points, railX, fromMidY);
    appendRoutePoint(points, railX, toMidY);
    appendRoutePoint(points, endX, toMidY);
    return points;
}

void expandGroupBounds(
    CapabilitySceneGroupLayout& group,
    const SceneRect& rect,
    const double padding) {
    const double minX = rect.x - padding;
    const double minY = rect.y - padding;
    const double maxX = rect.x + rect.width + padding;
    const double maxY = rect.y + rect.height + padding;

    if (!group.hasBounds) {
        group.hasBounds = true;
        group.x = minX;
        group.y = minY;
        group.width = maxX - minX;
        group.height = maxY - minY;
        return;
    }

    const double currentMaxX = group.x + group.width;
    const double currentMaxY = group.y + group.height;
    const double mergedMinX = std::min(group.x, minX);
    const double mergedMinY = std::min(group.y, minY);
    const double mergedMaxX = std::max(currentMaxX, maxX);
    const double mergedMaxY = std::max(currentMaxY, maxY);
    group.x = mergedMinX;
    group.y = mergedMinY;
    group.width = mergedMaxX - mergedMinX;
    group.height = mergedMaxY - mergedMinY;
}

void expandGroupBounds(
    ComponentSceneGroupLayout& group,
    const SceneRect& rect,
    const double padding) {
    const double minX = rect.x - padding;
    const double minY = rect.y - padding;
    const double maxX = rect.x + rect.width + padding;
    const double maxY = rect.y + rect.height + padding;

    if (!group.hasBounds) {
        group.hasBounds = true;
        group.x = minX;
        group.y = minY;
        group.width = maxX - minX;
        group.height = maxY - minY;
        return;
    }

    const double currentMaxX = group.x + group.width;
    const double currentMaxY = group.y + group.height;
    const double mergedMinX = std::min(group.x, minX);
    const double mergedMinY = std::min(group.y, minY);
    const double mergedMaxX = std::max(currentMaxX, maxX);
    const double mergedMaxY = std::max(currentMaxY, maxY);
    group.x = mergedMinX;
    group.y = mergedMinY;
    group.width = mergedMaxX - mergedMinX;
    group.height = mergedMaxY - mergedMinY;
}

}  // namespace

LayoutResult LayeredGraphLayout::layoutModules(
    const savt::core::AnalysisReport& report,
    const double horizontalSpacing,
    const double verticalSpacing) const {
    LayoutResult result;

    std::unordered_map<std::size_t, ModuleNodeInfo> modules;
    for (const savt::core::SymbolNode& node : report.nodes) {
        if (node.kind != savt::core::SymbolKind::Module) {
            continue;
        }

        modules.emplace(node.id, ModuleNodeInfo{node.id, node.displayName, 0, 0});
    }

    if (modules.empty()) {
        result.diagnostics.push_back("No module nodes found for layout.");
        return result;
    }

    std::unordered_map<std::size_t, std::vector<std::size_t>> adjacency;
    std::unordered_map<std::size_t, std::unordered_set<std::size_t>> dedup;

    for (const savt::core::SymbolEdge& edge : report.edges) {
        if (edge.kind != savt::core::EdgeKind::DependsOn) {
            continue;
        }

        auto fromIt = modules.find(edge.fromId);
        auto toIt = modules.find(edge.toId);
        if (fromIt == modules.end() || toIt == modules.end()) {
            continue;
        }

        if (dedup[edge.fromId].insert(edge.toId).second) {
            adjacency[edge.fromId].push_back(edge.toId);
            ++fromIt->second.outdegree;
            ++toIt->second.indegree;
        }
    }

    std::unordered_map<std::size_t, std::size_t> workingIndegree;
    for (const auto& [id, info] : modules) {
        workingIndegree.emplace(id, info.indegree);
    }

    std::vector<std::size_t> zeroIndegree;
    zeroIndegree.reserve(modules.size());
    for (const auto& [id, info] : modules) {
        if (info.indegree == 0) {
            zeroIndegree.push_back(id);
        }
    }

    auto sortByName = [&modules](std::vector<std::size_t>& values) {
        std::sort(values.begin(), values.end(), [&modules](const std::size_t left, const std::size_t right) {
            return modules.at(left).name < modules.at(right).name;
        });
    };

    sortByName(zeroIndegree);

    std::vector<std::vector<std::size_t>> layers;
    std::size_t processedCount = 0;

    while (!zeroIndegree.empty()) {
        std::vector<std::size_t> current = zeroIndegree;
        zeroIndegree.clear();
        sortByName(current);
        layers.push_back(current);

        for (const std::size_t nodeId : current) {
            ++processedCount;
            for (const std::size_t nextId : adjacency[nodeId]) {
                std::size_t& indegree = workingIndegree[nextId];
                if (indegree > 0) {
                    --indegree;
                    if (indegree == 0) {
                        zeroIndegree.push_back(nextId);
                    }
                }
            }
        }
    }

    if (processedCount < modules.size()) {
        std::vector<std::size_t> cyclicNodes;
        cyclicNodes.reserve(modules.size() - processedCount);
        for (const auto& [id, indegree] : workingIndegree) {
            if (indegree > 0) {
                cyclicNodes.push_back(id);
            }
        }
        sortByName(cyclicNodes);
        layers.push_back(cyclicNodes);
        result.diagnostics.push_back(
            "Cycle detected in module dependency graph. Cyclic modules were grouped into the final layer.");
    }

    std::size_t maxLayerSize = 0;
    for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
        std::vector<std::size_t>& layer = layers[layerIndex];
        std::sort(layer.begin(), layer.end(), [&modules](const std::size_t left, const std::size_t right) {
            const ModuleNodeInfo& leftInfo = modules.at(left);
            const ModuleNodeInfo& rightInfo = modules.at(right);
            if (leftInfo.outdegree != rightInfo.outdegree) {
                return leftInfo.outdegree > rightInfo.outdegree;
            }
            return leftInfo.name < rightInfo.name;
        });

        maxLayerSize = std::max(maxLayerSize, layer.size());
        for (std::size_t order = 0; order < layer.size(); ++order) {
            result.nodes.push_back(PositionedNode{
                layer[order],
                layerIndex,
                order,
                horizontalSpacing * static_cast<double>(layerIndex),
                verticalSpacing * static_cast<double>(order)});
        }
    }

    result.layerCount = layers.size();
    result.width = layers.empty() ? 0.0 : horizontalSpacing * static_cast<double>(layers.size());
    result.height = maxLayerSize == 0 ? 0.0 : verticalSpacing * static_cast<double>(maxLayerSize);
    return result;
}

CapabilitySceneLayoutResult LayeredGraphLayout::layoutCapabilityScene(
    const savt::core::CapabilityGraph& graph,
    const CapabilitySceneLayoutOptions& options) const {
    CapabilitySceneLayoutResult result;
    result.diagnostics = graph.diagnostics;

    std::array<std::vector<const savt::core::CapabilityNode*>, 3> lanes;
    for (const savt::core::CapabilityNode& node : graph.nodes) {
        if (node.defaultVisible) {
            lanes[static_cast<std::size_t>(laneIndexForNode(node))].push_back(&node);
        }
    }

    for (auto& lane : lanes) {
        std::sort(lane.begin(), lane.end(), [](const savt::core::CapabilityNode* left, const savt::core::CapabilityNode* right) {
            if (left->defaultPinned != right->defaultPinned) {
                return left->defaultPinned;
            }
            if (left->visualPriority != right->visualPriority) {
                return left->visualPriority > right->visualPriority;
            }
            return left->name < right->name;
        });
    }

    std::unordered_map<std::size_t, CapabilityNodePlacement> nodePlacements;
    double maxBottom = options.marginY;
    double currentX = options.marginX;

    for (std::size_t laneIndex = 0; laneIndex < lanes.size(); ++laneIndex) {
        double currentY = options.marginY;
        double maxLaneWidth = options.baseNodeWidth;

        for (const savt::core::CapabilityNode* nodePtr : lanes[laneIndex]) {
            const double importanceScale = importanceScaleForNode(*nodePtr, options);
            maxLaneWidth = std::max(maxLaneWidth, options.baseNodeWidth * importanceScale);
        }

        for (std::size_t order = 0; order < lanes[laneIndex].size(); ++order) {
            const savt::core::CapabilityNode& node = *lanes[laneIndex][order];
            const double importanceScale = importanceScaleForNode(node, options);
            const double nodeWidth = options.baseNodeWidth * importanceScale;
            const double nodeHeight = options.baseNodeHeight * importanceScale;
            const double x = currentX + (maxLaneWidth - nodeWidth) / 2.0;
            const double y = currentY;

            result.nodes.push_back(CapabilitySceneNodeLayout{
                node.id,
                laneIndex,
                order,
                x,
                y,
                nodeWidth,
                nodeHeight,
                importanceScale});
            nodePlacements.emplace(node.id, CapabilityNodePlacement{
                                                SceneRect{x, y, nodeWidth, nodeHeight},
                                                laneIndex,
                                                order});
            maxBottom = std::max(maxBottom, y + nodeHeight);
            currentY += nodeHeight + options.rowGap;
        }

        currentX += maxLaneWidth + options.columnGap;
    }

    if (result.nodes.empty()) {
        result.diagnostics.push_back("No visible capability nodes found for capability scene layout.");
    }

    for (const savt::core::CapabilityEdge& edge : graph.edges) {
        if (!edge.defaultVisible) {
            continue;
        }

        const auto fromPlacementIt = nodePlacements.find(edge.fromId);
        const auto toPlacementIt = nodePlacements.find(edge.toId);
        if (fromPlacementIt == nodePlacements.end() || toPlacementIt == nodePlacements.end()) {
            continue;
        }

        CapabilitySceneEdgeLayout edgeLayout;
        edgeLayout.edgeId = edge.id;
        edgeLayout.fromId = edge.fromId;
        edgeLayout.toId = edge.toId;
        edgeLayout.routePoints =
            capabilityEdgeRoute(fromPlacementIt->second, toPlacementIt->second, options);
        result.edges.push_back(std::move(edgeLayout));
    }

    for (const savt::core::CapabilityGroup& group : graph.groups) {
        CapabilitySceneGroupLayout groupLayout;
        groupLayout.groupId = group.id;
        for (const std::size_t nodeId : group.nodeIds) {
            const auto placementIt = nodePlacements.find(nodeId);
            if (placementIt == nodePlacements.end()) {
                continue;
            }
            groupLayout.visibleNodeIds.push_back(nodeId);
            expandGroupBounds(groupLayout, placementIt->second.rect, options.groupPadding);
        }
        std::sort(groupLayout.visibleNodeIds.begin(), groupLayout.visibleNodeIds.end());
        result.groups.push_back(std::move(groupLayout));
    }

    std::sort(result.groups.begin(), result.groups.end(), [](const CapabilitySceneGroupLayout& left, const CapabilitySceneGroupLayout& right) {
        return left.groupId < right.groupId;
    });

    result.width = currentX - options.columnGap + options.marginX;
    result.height = std::max(maxBottom + options.marginY, options.minSceneHeight);
    return result;
}

ComponentSceneLayoutResult LayeredGraphLayout::layoutComponentScene(
    const savt::core::ComponentGraph& graph,
    const ComponentSceneLayoutOptions& options) const {
    ComponentSceneLayoutResult result;
    result.diagnostics = graph.diagnostics;

    if (graph.nodes.empty()) {
        result.diagnostics.push_back("No component nodes found for L3 layout.");
        return result;
    }

    std::size_t maxStageIndex = 0;
    for (const savt::core::ComponentNode& node : graph.nodes) {
        maxStageIndex = std::max(maxStageIndex, node.stageIndex);
    }

    std::vector<std::vector<const savt::core::ComponentNode*>> stages(maxStageIndex + 1);
    for (const savt::core::ComponentNode& node : graph.nodes) {
        if (node.defaultVisible) {
            stages[node.stageIndex].push_back(&node);
        }
    }

    for (auto& stageNodes : stages) {
        std::sort(stageNodes.begin(), stageNodes.end(), [](const savt::core::ComponentNode* left, const savt::core::ComponentNode* right) {
            if (left->defaultPinned != right->defaultPinned) {
                return left->defaultPinned;
            }
            if (left->visualPriority != right->visualPriority) {
                return left->visualPriority > right->visualPriority;
            }
            return left->name < right->name;
        });
    }

    std::unordered_map<std::size_t, SceneRect> nodeRects;
    double currentX = options.marginX;
    double maxBottom = options.marginY;

    for (std::size_t stageIndex = 0; stageIndex < stages.size(); ++stageIndex) {
        double currentY = options.marginY;
        for (std::size_t order = 0; order < stages[stageIndex].size(); ++order) {
            const savt::core::ComponentNode& node = *stages[stageIndex][order];
            result.nodes.push_back(ComponentSceneNodeLayout{
                node.id,
                stageIndex,
                order,
                currentX,
                currentY,
                options.baseNodeWidth,
                options.baseNodeHeight});
            nodeRects.emplace(node.id, SceneRect{currentX, currentY, options.baseNodeWidth, options.baseNodeHeight});
            maxBottom = std::max(maxBottom, currentY + options.baseNodeHeight);
            currentY += options.baseNodeHeight + options.rowGap;
        }
        currentX += options.baseNodeWidth + options.columnGap;
    }

    for (const savt::core::ComponentEdge& edge : graph.edges) {
        if (!edge.defaultVisible) {
            continue;
        }
        const auto fromRectIt = nodeRects.find(edge.fromId);
        const auto toRectIt = nodeRects.find(edge.toId);
        if (fromRectIt == nodeRects.end() || toRectIt == nodeRects.end()) {
            continue;
        }

        const SceneRect& fromRect = fromRectIt->second;
        const SceneRect& toRect = toRectIt->second;
        ComponentSceneEdgeLayout edgeLayout;
        edgeLayout.edgeId = edge.id;
        edgeLayout.fromId = edge.fromId;
        edgeLayout.toId = edge.toId;
        edgeLayout.routePoints = {
            ScenePoint{fromRect.x + fromRect.width, fromRect.y + fromRect.height * 0.5},
            ScenePoint{toRect.x, toRect.y + toRect.height * 0.5}};
        result.edges.push_back(std::move(edgeLayout));
    }

    for (const savt::core::ComponentGroup& group : graph.groups) {
        ComponentSceneGroupLayout groupLayout;
        groupLayout.groupId = group.id;
        for (const std::size_t nodeId : group.nodeIds) {
            const auto rectIt = nodeRects.find(nodeId);
            if (rectIt == nodeRects.end()) {
                continue;
            }
            groupLayout.visibleNodeIds.push_back(nodeId);
            expandGroupBounds(groupLayout, rectIt->second, options.groupPadding);
        }
        std::sort(groupLayout.visibleNodeIds.begin(), groupLayout.visibleNodeIds.end());
        result.groups.push_back(std::move(groupLayout));
    }

    result.width = std::max(currentX - options.columnGap + options.marginX, options.baseNodeWidth + options.marginX * 2.0);
    result.height = std::max(maxBottom + options.marginY, options.minSceneHeight);
    return result;
}

std::string formatLayoutResult(
    const LayoutResult& result,
    const savt::core::AnalysisReport& report,
    const std::size_t maxNodes) {
    std::ostringstream output;

    output << "[module layout]\n";
    output << "layers: " << result.layerCount << "\n";
    output << "canvas: " << result.width << " x " << result.height << "\n";
    output << "positioned modules: " << result.nodes.size() << "\n";

    const std::size_t limit = std::min(maxNodes, result.nodes.size());
    for (std::size_t index = 0; index < limit; ++index) {
        const PositionedNode& positioned = result.nodes[index];
        const savt::core::SymbolNode* node = savt::core::findNodeById(report, positioned.nodeId);
        if (node == nullptr) {
            continue;
        }

        output << "- " << node->displayName
               << " layer=" << positioned.layer
               << " order=" << positioned.order
               << " pos=(" << positioned.x << ", " << positioned.y << ")\n";
    }
    if (result.nodes.size() > limit) {
        output << "... " << (result.nodes.size() - limit) << " more positioned modules\n";
    }

    output << "\n[layout diagnostics]\n";
    if (result.diagnostics.empty()) {
        output << "- none\n";
    } else {
        for (const std::string& diagnostic : result.diagnostics) {
            output << "- " << diagnostic << "\n";
        }
    }

    return output.str();
}

std::string formatCapabilitySceneLayoutResult(
    const CapabilitySceneLayoutResult& result,
    const std::size_t maxNodes,
    const std::size_t maxEdges,
    const std::size_t maxGroups) {
    std::ostringstream output;

    output << "[capability scene layout]\n";
    output << "canvas: " << result.width << " x " << result.height << "\n";
    output << "nodes: " << result.nodes.size() << "\n";
    output << "edges: " << result.edges.size() << "\n";
    output << "groups: " << result.groups.size() << "\n";

    const std::size_t nodeLimit = std::min(maxNodes, result.nodes.size());
    for (std::size_t index = 0; index < nodeLimit; ++index) {
        const CapabilitySceneNodeLayout& node = result.nodes[index];
        output << "- node " << node.nodeId
               << " lane=" << node.laneIndex
               << " order=" << node.orderInLane
               << " rect=(" << node.x << ", " << node.y << ", "
               << node.width << ", " << node.height << ")\n";
    }
    if (result.nodes.size() > nodeLimit) {
        output << "... " << (result.nodes.size() - nodeLimit) << " more nodes\n";
    }

    const std::size_t edgeLimit = std::min(maxEdges, result.edges.size());
    for (std::size_t index = 0; index < edgeLimit; ++index) {
        const CapabilitySceneEdgeLayout& edge = result.edges[index];
        output << "- edge " << edge.edgeId
               << " " << edge.fromId << " -> " << edge.toId;
        if (!edge.routePoints.empty()) {
            output << " routeStart=(" << edge.routePoints.front().x << ", "
                   << edge.routePoints.front().y << ")"
                   << " routeEnd=(" << edge.routePoints.back().x << ", "
                   << edge.routePoints.back().y << ")";
        }
        output << "\n";
    }
    if (result.edges.size() > edgeLimit) {
        output << "... " << (result.edges.size() - edgeLimit) << " more edges\n";
    }

    std::vector<CapabilitySceneGroupLayout> sortedGroups = result.groups;
    auto compareVisibleNodeIds = [](const std::vector<std::size_t>& left, const std::vector<std::size_t>& right) {
        return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
    };
    std::sort(sortedGroups.begin(), sortedGroups.end(), [&](const CapabilitySceneGroupLayout& left, const CapabilitySceneGroupLayout& right) {
        if (left.hasBounds != right.hasBounds) {
            return left.hasBounds > right.hasBounds;
        }
        if (left.x != right.x) {
            return left.x < right.x;
        }
        if (left.y != right.y) {
            return left.y < right.y;
        }
        if (left.width != right.width) {
            return left.width < right.width;
        }
        if (left.height != right.height) {
            return left.height < right.height;
        }
        if (left.visibleNodeIds != right.visibleNodeIds) {
            return compareVisibleNodeIds(left.visibleNodeIds, right.visibleNodeIds);
        }
        return left.groupId < right.groupId;
    });

    const std::size_t groupLimit = std::min(maxGroups, sortedGroups.size());
    for (std::size_t index = 0; index < groupLimit; ++index) {
        const CapabilitySceneGroupLayout& group = sortedGroups[index];
        output << "- group#" << (index + 1)
               << " visibleNodes=" << group.visibleNodeIds.size();
        if (group.hasBounds) {
            output << " bounds=(" << group.x << ", " << group.y << ", "
                   << group.width << ", " << group.height << ")";
        } else {
            output << " bounds=<none>";
        }
        output << "\n";
    }
    if (result.groups.size() > groupLimit) {
        output << "... " << (result.groups.size() - groupLimit) << " more groups\n";
    }

    output << "\n[scene diagnostics]\n";
    if (result.diagnostics.empty()) {
        output << "- none\n";
    } else {
        for (const std::string& diagnostic : result.diagnostics) {
            output << "- " << diagnostic << "\n";
        }
    }

    return output.str();
}

}  // namespace savt::layout
