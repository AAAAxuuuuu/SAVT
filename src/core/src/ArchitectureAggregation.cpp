#include "savt/core/ArchitectureAggregation.h"

namespace savt::core {

ArchitectureAggregation buildArchitectureAggregation(const AnalysisReport& report) {
    ArchitectureAggregation aggregation;
    aggregation.overview = buildArchitectureOverview(report);
    aggregation.capabilityGraph =
        buildCapabilityGraph(report, aggregation.overview);
    aggregation.ruleReport = evaluateArchitectureRules(
        aggregation.capabilityGraph,
        aggregation.overview);
    return aggregation;
}

}  // namespace savt::core
