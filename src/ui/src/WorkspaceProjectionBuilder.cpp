#include "savt/ui/WorkspaceProjectionBuilder.h"

#include "savt/core/ProjectAnalysisConfig.h"
#include "savt/ui/AnalysisTextFormatter.h"
#include "savt/ui/AstPreviewService.h"
#include "savt/ui/ReadingGuideService.h"
#include "savt/ui/SemanticReadinessService.h"

#include <QDir>

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

QVariantList toVariantStringList(const std::vector<std::string>& values) {
    QVariantList items;
    items.reserve(static_cast<qsizetype>(values.size()));
    for (const std::string& value : values) {
        const QString text = QString::fromStdString(value).trimmed();
        if (!text.isEmpty())
            items.push_back(text);
    }
    return items;
}

QVariantList buildModuleMergeItems(
    const std::vector<core::ProjectModuleMergeRule>& rules) {
    QVariantList items;
    items.reserve(static_cast<qsizetype>(rules.size()));
    for (const auto& rule : rules) {
        QVariantMap item;
        item.insert(QStringLiteral("match"), QString::fromStdString(rule.matchPrefix));
        item.insert(QStringLiteral("target"), QString::fromStdString(rule.targetModule));
        items.push_back(item);
    }
    return items;
}

QVariantList buildRoleOverrideItems(
    const std::vector<core::ProjectRoleOverrideRule>& rules) {
    QVariantList items;
    items.reserve(static_cast<qsizetype>(rules.size()));
    for (const auto& rule : rules) {
        QVariantMap item;
        item.insert(QStringLiteral("match"), QString::fromStdString(rule.matchPrefix));
        item.insert(QStringLiteral("role"), QString::fromStdString(rule.role));
        items.push_back(item);
    }
    return items;
}

QVariantList buildEntryOverrideItems(
    const std::vector<core::ProjectEntryOverrideRule>& rules) {
    QVariantList items;
    items.reserve(static_cast<qsizetype>(rules.size()));
    for (const auto& rule : rules) {
        QVariantMap item;
        item.insert(QStringLiteral("match"), QString::fromStdString(rule.matchPrefix));
        item.insert(QStringLiteral("entry"), rule.entry);
        items.push_back(item);
    }
    return items;
}

QVariantList buildDependencyFoldItems(
    const std::vector<core::ProjectDependencyFoldRule>& rules) {
    QVariantList items;
    items.reserve(static_cast<qsizetype>(rules.size()));
    for (const auto& rule : rules) {
        QVariantMap item;
        item.insert(QStringLiteral("match"), QString::fromStdString(rule.matchPrefix));
        item.insert(QStringLiteral("collapse"), rule.collapse);
        item.insert(QStringLiteral("hideByDefault"), rule.hideByDefault);
        items.push_back(item);
    }
    return items;
}

QVariantList buildConfigSearchPaths(const QString& projectRootPath) {
    const QDir rootDir(projectRootPath);
    QVariantList items;
    for (const QString& relativePath :
         {QStringLiteral("config/savt.project.json"),
          QStringLiteral("config/savt-project.json"),
          QStringLiteral(".savt/project.json"),
          QStringLiteral("savt.project.json")}) {
        items.push_back(QDir::toNativeSeparators(rootDir.filePath(relativePath)));
    }
    return items;
}

QVariantMap buildProjectConfigData(const QString& projectRootPath) {
    const auto config = core::loadProjectAnalysisConfig(
        std::filesystem::path(QDir::fromNativeSeparators(projectRootPath).toStdString()));

    QVariantMap counts;
    counts.insert(QStringLiteral("ignore"), static_cast<qulonglong>(config.ignoreDirectories.size()));
    counts.insert(QStringLiteral("merge"), static_cast<qulonglong>(config.moduleMerges.size()));
    counts.insert(QStringLiteral("role"), static_cast<qulonglong>(config.roleOverrides.size()));
    counts.insert(QStringLiteral("entry"), static_cast<qulonglong>(config.entryOverrides.size()));
    counts.insert(QStringLiteral("fold"), static_cast<qulonglong>(config.dependencyFoldRules.size()));

    QVariantMap data;
    data.insert(QStringLiteral("loaded"), config.loaded);
    data.insert(QStringLiteral("configPath"), QString::fromStdString(config.configPath));
    data.insert(QStringLiteral("diagnostics"), toVariantStringList(config.diagnostics));
    data.insert(QStringLiteral("ignoreDirectories"), toVariantStringList(config.ignoreDirectories));
    data.insert(QStringLiteral("moduleMerges"), buildModuleMergeItems(config.moduleMerges));
    data.insert(QStringLiteral("roleOverrides"), buildRoleOverrideItems(config.roleOverrides));
    data.insert(QStringLiteral("entryOverrides"), buildEntryOverrideItems(config.entryOverrides));
    data.insert(QStringLiteral("dependencyFoldRules"), buildDependencyFoldItems(config.dependencyFoldRules));
    data.insert(QStringLiteral("searchPaths"), buildConfigSearchPaths(projectRootPath));
    data.insert(QStringLiteral("counts"), counts);

    QString statusLabel;
    QString headline;
    QString summary;
    if (config.loaded) {
        statusLabel = QStringLiteral("已加载");
        headline = QStringLiteral("当前项目已启用项目级分析配置。");
        summary = QStringLiteral(
            "这份配置会参与目录忽略、模块归并、角色覆盖、入口识别和依赖折叠。");
    } else if (!config.configPath.empty()) {
        statusLabel = QStringLiteral("存在但未生效");
        headline = QStringLiteral("检测到了配置文件，但它还没有成功生效。");
        summary = config.diagnostics.empty()
            ? QStringLiteral("请检查配置格式、版本和字段内容。")
            : QString::fromStdString(config.diagnostics.front());
    } else {
        statusLabel = QStringLiteral("未发现");
        headline = QStringLiteral("当前项目还没有项目级分析配置。");
        summary = QStringLiteral(
            "如果默认启发式已经不够用，可以在项目根目录添加 SAVT 配置文件来纠偏。");
    }

    data.insert(QStringLiteral("statusLabel"), statusLabel);
    data.insert(QStringLiteral("headline"), headline);
    data.insert(QStringLiteral("summary"), summary);
    return data;
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
    projection.systemContextData.insert(
        QStringLiteral("projectConfig"),
        buildProjectConfigData(projectRootPath));

    projection.systemContextCards =
        ReadingGuideService::buildGuideCards(
            artifacts.report,
            artifacts.overview,
            artifacts.capabilityGraph);
    return projection;
}

}  // namespace savt::ui
