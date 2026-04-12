#include "savt/ui/ReportService.h"

#include "savt/ui/ReadingGuideService.h"
#include "savt/ui/SemanticReadinessService.h"

namespace savt::ui {

QVariantList ReportService::buildSystemContextCards(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph) {
    return ReadingGuideService::buildGuideCards(report, overview, graph);
}

QVariantMap ReportService::buildSystemContextData(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph,
    const QString& projectRootPath) {
    const core::ArchitectureRuleReport ruleReport =
        core::evaluateArchitectureRules(graph, overview);
    return buildSystemContextData(
        report,
        overview,
        graph,
        ruleReport,
        projectRootPath);
}

QVariantMap ReportService::buildSystemContextData(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph,
    const core::ArchitectureRuleReport& ruleReport,
    const QString& projectRootPath) {
    return ReadingGuideService::buildGuide(
        report,
        overview,
        graph,
        ruleReport,
        projectRootPath);
}

QString ReportService::buildStatusMessage(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& capabilityGraph,
    const layout::LayoutResult& layoutResult) {
    Q_UNUSED(layoutResult);
    return SemanticReadinessService::buildStatusMessage(
        SemanticReadinessService::buildStatus(report, overview, capabilityGraph));
}

}  // namespace savt::ui
