#include "savt/ui/WorkspaceProjectionBuilder.h"

#include "savt/ui/AnalysisTextFormatter.h"
#include "savt/ui/AstPreviewService.h"
#include "savt/ui/ReadingGuideService.h"
#include "savt/ui/SemanticReadinessService.h"

namespace savt::ui {

namespace {

QVariantMap buildComponentSceneCatalog(
    const core::CapabilityGraph& capabilityGraph,
    const std::unordered_map<std::size_t, core::ComponentGraph>& componentGraphs,
    const std::unordered_map<std::size_t, layout::ComponentSceneLayoutResult>& componentLayouts) {
    QVariantMap catalog;
    for (const core::CapabilityNode& capabilityNode : capabilityGraph.nodes) {
        const auto graphIt = componentGraphs.find(capabilityNode.id);
        const auto layoutIt = componentLayouts.find(capabilityNode.id);
        if (graphIt == componentGraphs.end() || layoutIt == componentLayouts.end()) {
            continue;
        }

        const auto componentScene =
            SceneMapper::buildComponentSceneData(graphIt->second, layoutIt->second);
        catalog.insert(
            QString::number(static_cast<qulonglong>(capabilityNode.id)),
            SceneMapper::toVariantMap(componentScene));
    }
    return catalog;
}

}  // namespace

WorkspaceProjection WorkspaceProjectionBuilder::build(
    const QString& projectRootPath,
    const AnalysisArtifacts& artifacts) {
    WorkspaceProjection projection;
    projection.capabilitySceneLayout = artifacts.capabilitySceneLayout;

    projection.astFileItems = AstPreviewService::buildAstFileItems(artifacts.report);
    projection.selectedAstFilePath =
        AstPreviewService::chooseDefaultAstFilePath(projection.astFileItems);
    const auto astPreview =
        AstPreviewService::buildPreview(projectRootPath, projection.selectedAstFilePath);
    projection.astPreviewTitle = astPreview.title;
    projection.astPreviewSummary = astPreview.summary;
    projection.astPreviewText = astPreview.text;

    projection.capabilityScene =
        SceneMapper::buildCapabilitySceneData(
            artifacts.capabilityGraph,
            artifacts.capabilitySceneLayout);
    projection.componentSceneCatalog =
        buildComponentSceneCatalog(
            artifacts.capabilityGraph,
            artifacts.componentGraphs,
            artifacts.componentLayouts);

    const QVariantMap semanticReadiness =
        SemanticReadinessService::buildStatus(
            artifacts.report,
            artifacts.overview,
            artifacts.capabilityGraph);
    projection.statusMessage =
        SemanticReadinessService::buildStatusMessage(semanticReadiness);
    projection.analysisReport =
        formatCapabilityReportMarkdown(artifacts.report, artifacts.capabilityGraph);
    projection.systemContextReport =
        formatSystemContextReportMarkdown(artifacts.capabilityGraph);

    projection.systemContextData =
        ReadingGuideService::buildGuide(
            artifacts.report,
            artifacts.overview,
            artifacts.capabilityGraph,
            artifacts.ruleReport,
            projectRootPath);
    projection.systemContextData.insert(
        QStringLiteral("semanticReadiness"),
        semanticReadiness);
    projection.systemContextData.insert(
        QStringLiteral("precisionSummary"),
        formatPrecisionSummary(
            artifacts.report,
            artifacts.overview,
            artifacts.capabilityGraph));
    projection.systemContextData.insert(
        QStringLiteral("layoutLayerCount"),
        QVariant::fromValue(
            static_cast<qulonglong>(artifacts.moduleLayout.layerCount)));

    projection.systemContextCards =
        ReadingGuideService::buildGuideCards(
            artifacts.report,
            artifacts.overview,
            artifacts.capabilityGraph);
    return projection;
}

}  // namespace savt::ui
