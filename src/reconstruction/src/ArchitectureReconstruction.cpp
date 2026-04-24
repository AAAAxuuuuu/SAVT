#include "savt/reconstruction/ArchitectureReconstruction.h"

namespace savt::reconstruction {

namespace {

bool isCancellationRequested(const ArchitectureReconstructionOptions& options) {
    return options.cancellationRequested && options.cancellationRequested();
}

}  // namespace

CapabilityDrilldown buildCapabilityDrilldown(
    const savt::core::AnalysisReport& report,
    const savt::core::ArchitectureOverview& overview,
    const savt::core::CapabilityGraph& capabilityGraph,
    const std::size_t capabilityId) {
    savt::layout::LayeredGraphLayout layoutEngine;

    CapabilityDrilldown drilldown;
    drilldown.graph = savt::core::buildComponentGraphForCapability(
        report,
        overview,
        capabilityGraph,
        capabilityId);
    drilldown.layout = layoutEngine.layoutComponentScene(drilldown.graph);
    return drilldown;
}

ArchitectureReconstructionResult buildArchitectureReconstruction(
    const savt::core::AnalysisReport& report,
    const ArchitectureReconstructionOptions& options) {
    const savt::core::ArchitectureAggregation aggregation =
        savt::core::buildArchitectureAggregation(report);
    return buildArchitectureReconstruction(report, aggregation, options);
}

ArchitectureReconstructionResult buildArchitectureReconstruction(
    const savt::core::AnalysisReport& report,
    const savt::core::ArchitectureAggregation& aggregation,
    const ArchitectureReconstructionOptions& options) {
    ArchitectureReconstructionResult result;
    savt::layout::LayeredGraphLayout layoutEngine;

    if (isCancellationRequested(options)) {
        result.canceled = true;
        return result;
    }

    if (options.buildCapabilitySceneLayout) {
        result.capabilitySceneLayout =
            layoutEngine.layoutCapabilityScene(aggregation.capabilityGraph);
    }

    if (isCancellationRequested(options)) {
        result.canceled = true;
        return result;
    }

    if (options.buildModuleLayout) {
        result.moduleLayout = layoutEngine.layoutModules(report);
    }

    if (!options.buildCapabilityDrilldowns) {
        return result;
    }

    for (const savt::core::CapabilityNode& capabilityNode : aggregation.capabilityGraph.nodes) {
        if (isCancellationRequested(options)) {
            result.canceled = true;
            return result;
        }

        result.capabilityDrilldowns.emplace(
            capabilityNode.id,
            buildCapabilityDrilldown(
                report,
                aggregation.overview,
                aggregation.capabilityGraph,
                capabilityNode.id));
    }

    return result;
}

}  // namespace savt::reconstruction
