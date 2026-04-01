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
    double baseNodeWidth = 248.0;
    double baseNodeHeight = 116.0;
    double columnGap = 108.0;
    double rowGap = 30.0;
    double marginX = 40.0;
    double marginY = 28.0;
    double minSceneHeight = 360.0;
    double groupPadding = 18.0;
    double collaboratorScaleDivisor = 8.0;
    double maxImportanceBoost = 0.4;
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
    double baseNodeWidth = 232.0;
    double baseNodeHeight = 110.0;
    double columnGap = 92.0;
    double rowGap = 26.0;
    double marginX = 32.0;
    double marginY = 24.0;
    double minSceneHeight = 320.0;
    double groupPadding = 16.0;
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
