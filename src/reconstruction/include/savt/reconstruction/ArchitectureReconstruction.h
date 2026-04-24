#pragma once

#include "savt/core/ArchitectureAggregation.h"
#include "savt/core/ComponentGraph.h"
#include "savt/layout/LayeredGraphLayout.h"

#include <cstddef>
#include <functional>
#include <unordered_map>

namespace savt::reconstruction {

struct ArchitectureReconstructionOptions {
    bool buildModuleLayout = true;
    bool buildCapabilitySceneLayout = true;
    bool buildCapabilityDrilldowns = true;
    std::function<bool()> cancellationRequested;
};

struct CapabilityDrilldown {
    savt::core::ComponentGraph graph;
    savt::layout::ComponentSceneLayoutResult layout;
};

struct ArchitectureReconstructionResult {
    savt::layout::CapabilitySceneLayoutResult capabilitySceneLayout;
    savt::layout::LayoutResult moduleLayout;
    std::unordered_map<std::size_t, CapabilityDrilldown> capabilityDrilldowns;
    bool canceled = false;
};

CapabilityDrilldown buildCapabilityDrilldown(
    const savt::core::AnalysisReport& report,
    const savt::core::ArchitectureOverview& overview,
    const savt::core::CapabilityGraph& capabilityGraph,
    std::size_t capabilityId);

ArchitectureReconstructionResult buildArchitectureReconstruction(
    const savt::core::AnalysisReport& report,
    const ArchitectureReconstructionOptions& options = {});

ArchitectureReconstructionResult buildArchitectureReconstruction(
    const savt::core::AnalysisReport& report,
    const savt::core::ArchitectureAggregation& aggregation,
    const ArchitectureReconstructionOptions& options = {});

}  // namespace savt::reconstruction
