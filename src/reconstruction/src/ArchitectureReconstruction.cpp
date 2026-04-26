#include "savt/reconstruction/ArchitectureReconstruction.h"

namespace savt::reconstruction {

namespace {

bool isCancellationRequested(const ArchitectureReconstructionOptions& options) {
    return options.cancellationRequested && options.cancellationRequested();
}

void publishProgress(
    const ArchitectureReconstructionOptions& options,
    const std::size_t completed,
    const std::size_t total,
    const std::string& label) {
    if (options.progressReporter && total > 0) {
        options.progressReporter(completed, total, label);
    }
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
    const std::size_t totalWork =
        (options.buildCapabilitySceneLayout ? 1u : 0u) +
        (options.buildModuleLayout ? 1u : 0u) +
        (options.buildCapabilityDrilldowns ? aggregation.capabilityGraph.nodes.size() : 0u);
    std::size_t completedWork = 0;

    if (isCancellationRequested(options)) {
        result.canceled = true;
        return result;
    }
    publishProgress(options, completedWork, totalWork, "准备架构视图...");

    if (options.buildCapabilitySceneLayout) {
        result.capabilitySceneLayout =
            layoutEngine.layoutCapabilityScene(aggregation.capabilityGraph);
        ++completedWork;
        publishProgress(options, completedWork, totalWork, "计算能力图布局...");
    }

    if (isCancellationRequested(options)) {
        result.canceled = true;
        return result;
    }

    if (options.buildModuleLayout) {
        result.moduleLayout = layoutEngine.layoutModules(report);
        ++completedWork;
        publishProgress(options, completedWork, totalWork, "计算模块布局...");
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
        ++completedWork;
        publishProgress(options, completedWork, totalWork, "生成能力域钻取视图...");
    }

    return result;
}

}  // namespace savt::reconstruction
