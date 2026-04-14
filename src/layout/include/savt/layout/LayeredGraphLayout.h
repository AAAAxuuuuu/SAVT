#pragma once

#include "savt/core/ArchitectureGraph.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/core/ComponentGraph.h"

#include <cstddef>
#include <string>
#include <vector>

namespace savt::layout {

struct PositionedNode {
    std::size_t nodeId = 0;
    std::size_t layer = 0;
    std::size_t order = 0;
    double x = 0.0;
    double y = 0.0;
};

struct LayoutResult {
    double width = 0.0;
    double height = 0.0;
    std::size_t layerCount = 0;
    std::vector<PositionedNode> nodes;
    std::vector<std::string> diagnostics;
};

struct CapabilitySceneLayoutOptions {
    double baseNodeWidth = 272.0;
    double baseNodeHeight = 132.0;
    double columnGap = 118.0;
    double rowGap = 52.0;
    double marginX = 44.0;
    double marginY = 72.0;
    double minSceneHeight = 460.0;
    double groupPadding = 44.0;
    double edgeStub = 28.0;
    double sameLaneRailOffset = 38.0;
    double collaboratorScaleDivisor = 10.0;
    double maxImportanceBoost = 0.26;
};

struct ScenePoint {
    double x = 0.0;
    double y = 0.0;
};

struct CapabilitySceneNodeLayout {
    std::size_t nodeId = 0;
    std::size_t laneIndex = 0;
    std::size_t orderInLane = 0;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    double importanceScale = 1.0;
};

struct CapabilitySceneEdgeLayout {
    std::size_t edgeId = 0;
    std::size_t fromId = 0;
    std::size_t toId = 0;
    std::vector<ScenePoint> routePoints;
};

struct CapabilitySceneGroupLayout {
    std::size_t groupId = 0;
    bool hasBounds = false;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    std::vector<std::size_t> visibleNodeIds;
};

struct CapabilitySceneLayoutResult {
    double width = 0.0;
    double height = 0.0;
    std::vector<CapabilitySceneNodeLayout> nodes;
    std::vector<CapabilitySceneEdgeLayout> edges;
    std::vector<CapabilitySceneGroupLayout> groups;
    std::vector<std::string> diagnostics;
};

struct ComponentSceneLayoutOptions {
    double baseNodeWidth = 252.0;
    double baseNodeHeight = 124.0;
    double columnGap = 80.0;
    double rowGap = 32.0;
    double marginX = 28.0;
    double marginY = 52.0;
    double minSceneHeight = 336.0;
    double groupPadding = 34.0;
};

struct ComponentSceneNodeLayout {
    std::size_t nodeId = 0;
    std::size_t stageIndex = 0;
    std::size_t orderInStage = 0;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

struct ComponentSceneEdgeLayout {
    std::size_t edgeId = 0;
    std::size_t fromId = 0;
    std::size_t toId = 0;
    std::vector<ScenePoint> routePoints;
};

struct ComponentSceneGroupLayout {
    std::size_t groupId = 0;
    bool hasBounds = false;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    std::vector<std::size_t> visibleNodeIds;
};

struct ComponentSceneLayoutResult {
    double width = 0.0;
    double height = 0.0;
    std::vector<ComponentSceneNodeLayout> nodes;
    std::vector<ComponentSceneEdgeLayout> edges;
    std::vector<ComponentSceneGroupLayout> groups;
    std::vector<std::string> diagnostics;
};

class LayeredGraphLayout {
public:
    LayoutResult layoutModules(
        const savt::core::AnalysisReport& report,
        double horizontalSpacing = 320.0,
        double verticalSpacing = 140.0) const;

    CapabilitySceneLayoutResult layoutCapabilityScene(
        const savt::core::CapabilityGraph& graph,
        const CapabilitySceneLayoutOptions& options = {}) const;

    ComponentSceneLayoutResult layoutComponentScene(
        const savt::core::ComponentGraph& graph,
        const ComponentSceneLayoutOptions& options = {}) const;
};

std::string formatLayoutResult(
    const LayoutResult& result,
    const savt::core::AnalysisReport& report,
    std::size_t maxNodes = 40);

std::string formatCapabilitySceneLayoutResult(
    const CapabilitySceneLayoutResult& result,
    std::size_t maxNodes = 24,
    std::size_t maxEdges = 32,
    std::size_t maxGroups = 16);

}  // namespace savt::layout
