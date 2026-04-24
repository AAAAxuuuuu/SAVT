#include "savt/ui/WorkspaceProjectionBuilder.h"

#include "savt/core/ProjectAnalysisConfig.h"
#include "savt/ui/AnalysisTextFormatter.h"
#include "savt/ui/AstPreviewService.h"
#include "savt/ui/ProjectConfigRecommendationService.h"
#include "savt/ui/ReadingGuideService.h"
#include "savt/ui/SemanticReadinessService.h"

#include <QDir>
#include <QStringList>

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

QString shortCacheKey(const std::string& rawKey) {
    const QString key = QString::fromStdString(rawKey).trimmed();
    if (key.isEmpty()) {
        return QStringLiteral("No cache key");
    }
    if (key.size() <= 28) {
        return key;
    }
    return QStringLiteral("...%1").arg(key.right(28));
}

QString primaryEngineLabel(const core::AnalysisReport& report) {
    const QString engine = QString::fromStdString(report.primaryEngine).trimmed();
    if (engine == QStringLiteral("libclang-cindex")) {
        return QStringLiteral("Clang/LibTooling semantic backend");
    }
    if (engine == QStringLiteral("tree-sitter")) {
        return QStringLiteral("Tree-sitter syntax extractor");
    }
    if (engine == QStringLiteral("none")) {
        return QStringLiteral("Engine unavailable");
    }
    return engine.isEmpty() ? QStringLiteral("Unknown engine") : engine;
}

QVariantMap makeInfoItem(
    const QString& label,
    const QString& value,
    const QString& detail,
    const QString& tone) {
    QVariantMap item;
    item.insert(QStringLiteral("label"), label);
    item.insert(QStringLiteral("value"), value);
    item.insert(QStringLiteral("detail"), detail);
    item.insert(QStringLiteral("tone"), tone);
    return item;
}

QVariantMap makeStageItem(
    const QString& title,
    const QString& kicker,
    const QString& value,
    const QString& summary,
    const QString& tone) {
    QVariantMap item;
    item.insert(QStringLiteral("title"), title);
    item.insert(QStringLiteral("kicker"), kicker);
    item.insert(QStringLiteral("value"), value);
    item.insert(QStringLiteral("summary"), summary);
    item.insert(QStringLiteral("tone"), tone);
    return item;
}

struct EvidenceRollup {
    std::size_t facts = 0;
    std::size_t rules = 0;
    std::size_t conclusions = 0;
    std::size_t relationships = 0;
    std::size_t sourceFiles = 0;
    std::size_t symbols = 0;
    std::size_t modules = 0;
    std::size_t highConfidence = 0;
    std::size_t mediumConfidence = 0;
    std::size_t lowConfidence = 0;
};

EvidenceRollup summarizeEvidence(const core::CapabilityGraph& graph) {
    EvidenceRollup rollup;
    QStringList uniqueSourceFiles;
    QStringList uniqueSymbols;
    QStringList uniqueModules;

    const auto addUnique = [](QStringList& values, const std::vector<std::string>& incoming) {
        for (const std::string& raw : incoming) {
            const QString text = QString::fromStdString(raw).trimmed();
            if (!text.isEmpty() && !values.contains(text)) {
                values.push_back(text);
            }
        }
    };

    const auto classifyConfidence = [&](const std::string& rawLabel) {
        const QString label = QString::fromStdString(rawLabel).trimmed().toLower();
        if (label.contains(QStringLiteral("high")) || label.contains(QStringLiteral("strong")) ||
            label.contains(QStringLiteral("高"))) {
            ++rollup.highConfidence;
            return;
        }
        if (label.contains(QStringLiteral("medium")) || label.contains(QStringLiteral("moderate")) ||
            label.contains(QStringLiteral("mid")) || label.contains(QStringLiteral("中"))) {
            ++rollup.mediumConfidence;
            return;
        }
        if (!label.isEmpty()) {
            ++rollup.lowConfidence;
        }
    };

    for (const core::CapabilityNode& node : graph.nodes) {
        rollup.facts += node.evidence.facts.size();
        rollup.rules += node.evidence.rules.size();
        rollup.conclusions += node.evidence.conclusions.size();
        rollup.relationships += node.evidence.relationships.size();
        addUnique(uniqueSourceFiles, node.evidence.sourceFiles);
        addUnique(uniqueSymbols, node.evidence.symbols);
        addUnique(uniqueModules, node.evidence.modules);
        classifyConfidence(node.evidence.confidenceLabel);
    }

    for (const core::CapabilityEdge& edge : graph.edges) {
        rollup.facts += edge.evidence.facts.size();
        rollup.rules += edge.evidence.rules.size();
        rollup.conclusions += edge.evidence.conclusions.size();
        rollup.relationships += edge.evidence.relationships.size();
        addUnique(uniqueSourceFiles, edge.evidence.sourceFiles);
        addUnique(uniqueSymbols, edge.evidence.symbols);
        addUnique(uniqueModules, edge.evidence.modules);
    }

    rollup.sourceFiles = static_cast<std::size_t>(uniqueSourceFiles.size());
    rollup.symbols = static_cast<std::size_t>(uniqueSymbols.size());
    rollup.modules = static_cast<std::size_t>(uniqueModules.size());
    return rollup;
}

QVariantList buildAlgorithmMetrics(
    const AnalysisArtifacts& artifacts,
    const QVariantMap& semanticReadiness,
    const EvidenceRollup& evidence) {
    std::size_t semanticNodeCount = 0;
    std::size_t inferredNodeCount = 0;
    for (const core::SymbolNode& node : artifacts.report.nodes) {
        if (node.factSource == core::FactSource::Semantic) {
            ++semanticNodeCount;
        } else {
            ++inferredNodeCount;
        }
    }

    std::size_t semanticEdgeCount = 0;
    std::size_t inferredEdgeCount = 0;
    std::size_t moduleNodeCount = 0;
    for (const core::SymbolEdge& edge : artifacts.report.edges) {
        if (edge.factSource == core::FactSource::Semantic) {
            ++semanticEdgeCount;
        } else {
            ++inferredEdgeCount;
        }
    }
    for (const core::SymbolNode& node : artifacts.report.nodes) {
        if (node.kind == core::SymbolKind::Module) {
            ++moduleNodeCount;
        }
    }

    std::size_t visibleCapabilityCount = 0;
    for (const core::CapabilityNode& node : artifacts.capabilityGraph.nodes) {
        if (node.defaultVisible) {
            ++visibleCapabilityCount;
        }
    }

    std::size_t componentNodeTotal = 0;
    std::size_t componentEdgeTotal = 0;
    for (const auto& [capabilityId, graph] : artifacts.componentGraphs) {
        Q_UNUSED(capabilityId);
        componentNodeTotal += graph.nodes.size();
        componentEdgeTotal += graph.edges.size();
    }

    const int cacheHitCount = (artifacts.scanLayer.hit ? 1 : 0) +
                              (artifacts.parseLayer.hit ? 1 : 0) +
                              (artifacts.aggregateLayer.hit ? 1 : 0) +
                              (artifacts.layoutLayer.hit ? 1 : 0);

    QVariantList items;
    items.push_back(makeInfoItem(
        QStringLiteral("源码事实"),
        QStringLiteral("%1 节点 / %2 关系")
            .arg(static_cast<qulonglong>(artifacts.report.nodes.size()))
            .arg(static_cast<qulonglong>(artifacts.report.edges.size())),
        QStringLiteral("%1 个模块事实，覆盖 %2 / %3 个文件")
            .arg(static_cast<qulonglong>(moduleNodeCount))
            .arg(static_cast<qulonglong>(artifacts.report.parsedFiles))
            .arg(static_cast<qulonglong>(artifacts.report.discoveredFiles)),
        QStringLiteral("info")));
    items.push_back(makeInfoItem(
        QStringLiteral("语义覆盖"),
        QStringLiteral("%1 语义 / %2 推断")
            .arg(static_cast<qulonglong>(semanticNodeCount + semanticEdgeCount))
            .arg(static_cast<qulonglong>(inferredNodeCount + inferredEdgeCount)),
        semanticReadiness.value(QStringLiteral("headline")).toString().trimmed(),
        semanticReadiness.value(QStringLiteral("badgeTone")).toString().trimmed()));
    items.push_back(makeInfoItem(
        QStringLiteral("能力域重建"),
        QStringLiteral("%1 / %2 可见")
            .arg(static_cast<qulonglong>(artifacts.capabilityGraph.nodes.size()))
            .arg(static_cast<qulonglong>(visibleCapabilityCount)),
        QStringLiteral("%1 条能力关系，%2 条主流程")
            .arg(static_cast<qulonglong>(artifacts.capabilityGraph.edges.size()))
            .arg(static_cast<qulonglong>(artifacts.capabilityGraph.flows.size())),
        QStringLiteral("success")));
    items.push_back(makeInfoItem(
        QStringLiteral("组件钻取"),
        QStringLiteral("%1 个视图")
            .arg(static_cast<qulonglong>(artifacts.componentGraphs.size())),
        QStringLiteral("%1 个组件节点，%2 条组件关系")
            .arg(static_cast<qulonglong>(componentNodeTotal))
            .arg(static_cast<qulonglong>(componentEdgeTotal)),
        QStringLiteral("moss")));
    items.push_back(makeInfoItem(
        QStringLiteral("证据包"),
        QStringLiteral("%1 条证据")
            .arg(static_cast<qulonglong>(evidence.facts + evidence.rules + evidence.conclusions)),
        QStringLiteral("%1 条规则，%2 条结论，高置信 %3 个")
            .arg(static_cast<qulonglong>(evidence.rules))
            .arg(static_cast<qulonglong>(evidence.conclusions))
            .arg(static_cast<qulonglong>(evidence.highConfidence)),
        evidence.highConfidence > 0 ? QStringLiteral("ai") : QStringLiteral("warning")));
    items.push_back(makeInfoItem(
        QStringLiteral("增量更新"),
        QStringLiteral("%1 / 4 层命中").arg(cacheHitCount),
        QStringLiteral("%1 个布局层，%2 条缓存诊断")
            .arg(static_cast<qulonglong>(artifacts.moduleLayout.layerCount))
            .arg(static_cast<qulonglong>(artifacts.cacheDiagnostics.size())),
        cacheHitCount >= 2 ? QStringLiteral("success") : QStringLiteral("warning")));
    return items;
}

QVariantList buildAlgorithmStages(
    const AnalysisArtifacts& artifacts,
    const QVariantMap& semanticReadiness,
    const EvidenceRollup& evidence) {
    const int cacheHitCount = (artifacts.scanLayer.hit ? 1 : 0) +
                              (artifacts.parseLayer.hit ? 1 : 0) +
                              (artifacts.aggregateLayer.hit ? 1 : 0) +
                              (artifacts.layoutLayer.hit ? 1 : 0);
    QVariantList items;
    items.push_back(makeStageItem(
        QStringLiteral("源码扫描"),
        artifacts.scanLayer.hit ? QStringLiteral("缓存命中") : QStringLiteral("重新扫描"),
        QStringLiteral("%1 files").arg(static_cast<qulonglong>(artifacts.report.discoveredFiles)),
        QStringLiteral("定位候选翻译单元、入口文件和项目边界，为后续事实提取建立扫描底座。"),
        artifacts.scanLayer.hit ? QStringLiteral("success") : QStringLiteral("info")));
    items.push_back(makeStageItem(
        QStringLiteral("事实提取"),
        semanticReadiness.value(QStringLiteral("modeLabel")).toString().trimmed(),
        QStringLiteral("%1 parsed").arg(static_cast<qulonglong>(artifacts.report.parsedFiles)),
        QStringLiteral("%1，主引擎为 %2。")
            .arg(semanticReadiness.value(QStringLiteral("reason")).toString().trimmed(),
                 primaryEngineLabel(artifacts.report)),
        semanticReadiness.value(QStringLiteral("badgeTone")).toString().trimmed()));
    items.push_back(makeStageItem(
        QStringLiteral("能力聚合"),
        QStringLiteral("L1 / L2"),
        QStringLiteral("%1 nodes").arg(static_cast<qulonglong>(artifacts.capabilityGraph.nodes.size())),
        QStringLiteral("从 %1 个 overview 模块聚合出 %2 个能力域，并沉淀 %3 条主关系。")
            .arg(static_cast<qulonglong>(artifacts.overview.nodes.size()))
            .arg(static_cast<qulonglong>(artifacts.capabilityGraph.nodes.size()))
            .arg(static_cast<qulonglong>(artifacts.capabilityGraph.edges.size())),
        QStringLiteral("success")));
    items.push_back(makeStageItem(
        QStringLiteral("视图重建"),
        QStringLiteral("L3 drilldown"),
        QStringLiteral("%1 views").arg(static_cast<qulonglong>(artifacts.componentGraphs.size())),
        QStringLiteral("输出 %1 层模块布局，并为每个能力域生成可钻取组件图。")
            .arg(static_cast<qulonglong>(artifacts.moduleLayout.layerCount)),
        QStringLiteral("moss")));
    items.push_back(makeStageItem(
        QStringLiteral("证据校准"),
        evidence.highConfidence > 0 ? QStringLiteral("高置信存在") : QStringLiteral("需补强"),
        QStringLiteral("%1 facts").arg(static_cast<qulonglong>(evidence.facts)),
        QStringLiteral("累计 %1 条规则、%2 条结论，%3 个能力域达到高置信。")
            .arg(static_cast<qulonglong>(evidence.rules))
            .arg(static_cast<qulonglong>(evidence.conclusions))
            .arg(static_cast<qulonglong>(evidence.highConfidence)),
        evidence.highConfidence > 0 ? QStringLiteral("ai") : QStringLiteral("warning")));
    items.push_back(makeStageItem(
        QStringLiteral("增量更新"),
        cacheHitCount > 0 ? QStringLiteral("具备复用") : QStringLiteral("全量链路"),
        QStringLiteral("%1 / 4 hits").arg(cacheHitCount),
        QStringLiteral("扫描、解析、聚合、布局四层缓存参与本次重建，支撑交互式快速回看。"),
        cacheHitCount >= 2 ? QStringLiteral("success") : QStringLiteral("warning")));
    return items;
}

QVariantList buildCacheLayerItems(const AnalysisArtifacts& artifacts) {
    QVariantList items;

    const auto appendLayer = [&](const QString& title,
                                 const IncrementalCacheLayerState& state,
                                 const QString& recomputeSummary) {
        items.push_back(makeInfoItem(
            title,
            state.hit ? QStringLiteral("命中") : QStringLiteral("重算"),
            state.hit ? QStringLiteral("复用键 %1").arg(shortCacheKey(state.key))
                      : recomputeSummary,
            state.hit ? QStringLiteral("success") : QStringLiteral("warning")));
    };

    appendLayer(
        QStringLiteral("Scan layer"),
        artifacts.scanLayer,
        QStringLiteral("重新识别工程文件、目录边界和候选翻译单元。"));
    appendLayer(
        QStringLiteral("Parse layer"),
        artifacts.parseLayer,
        QStringLiteral("重新解析源码事实、符号与关系边。"));
    appendLayer(
        QStringLiteral("Aggregate layer"),
        artifacts.aggregateLayer,
        QStringLiteral("重新聚合 overview、能力域与规则检查结果。"));
    appendLayer(
        QStringLiteral("Layout layer"),
        artifacts.layoutLayer,
        QStringLiteral("重新计算全景布局与能力域钻取视图。"));
    return items;
}

QVariantList buildEvidenceItems(const EvidenceRollup& evidence) {
    QVariantList items;
    items.push_back(makeInfoItem(
        QStringLiteral("Facts"),
        QString::number(static_cast<qulonglong>(evidence.facts)),
        QStringLiteral("直接来源于源码、拓扑和分组线索"),
        QStringLiteral("info")));
    items.push_back(makeInfoItem(
        QStringLiteral("Rules"),
        QString::number(static_cast<qulonglong>(evidence.rules)),
        QStringLiteral("用于边界划分、角色归因与可视层级重建"),
        QStringLiteral("success")));
    items.push_back(makeInfoItem(
        QStringLiteral("Conclusions"),
        QString::number(static_cast<qulonglong>(evidence.conclusions)),
        QStringLiteral("最终写入节点 / 关系的可解释判断"),
        QStringLiteral("moss")));
    items.push_back(makeInfoItem(
        QStringLiteral("Coverage"),
        QStringLiteral("%1 files / %2 symbols")
            .arg(static_cast<qulonglong>(evidence.sourceFiles))
            .arg(static_cast<qulonglong>(evidence.symbols)),
        QStringLiteral("%1 个模块参与证据包组织")
            .arg(static_cast<qulonglong>(evidence.modules)),
        QStringLiteral("ai")));
    items.push_back(makeInfoItem(
        QStringLiteral("Confidence"),
        QStringLiteral("%1 高 / %2 中 / %3 低")
            .arg(static_cast<qulonglong>(evidence.highConfidence))
            .arg(static_cast<qulonglong>(evidence.mediumConfidence))
            .arg(static_cast<qulonglong>(evidence.lowConfidence)),
        QStringLiteral("%1 条关系证据参与校准")
            .arg(static_cast<qulonglong>(evidence.relationships)),
        evidence.highConfidence > evidence.lowConfidence
            ? QStringLiteral("success")
            : QStringLiteral("warning")));
    return items;
}

QVariantMap buildAlgorithmSummary(
    const AnalysisArtifacts& artifacts,
    const QVariantMap& semanticReadiness,
    const EvidenceRollup& evidence) {
    const int cacheHitCount = (artifacts.scanLayer.hit ? 1 : 0) +
                              (artifacts.parseLayer.hit ? 1 : 0) +
                              (artifacts.aggregateLayer.hit ? 1 : 0) +
                              (artifacts.layoutLayer.hit ? 1 : 0);
    const qulonglong rawFactCount = static_cast<qulonglong>(
        artifacts.report.nodes.size() + artifacts.report.edges.size());
    const qulonglong capabilityCount =
        static_cast<qulonglong>(artifacts.capabilityGraph.nodes.size());
    const qulonglong drilldownCount =
        static_cast<qulonglong>(artifacts.componentGraphs.size());

    QVariantMap data;
    data.insert(QStringLiteral("title"), QStringLiteral("证据驱动多粒度架构重建"));
    data.insert(
        QStringLiteral("headline"),
        QStringLiteral("%1 个源码事实 → %2 个能力域 → %3 个可钻取组件视图")
            .arg(rawFactCount)
            .arg(capabilityCount)
            .arg(drilldownCount));
    data.insert(
        QStringLiteral("summary"),
        QStringLiteral(
            "从源码扫描、事实提取、能力聚合到视图重建，当前结果已经将工程转成 L1/L2/L3 三层结构视图，并绑定证据链与增量缓存状态。"));
    data.insert(
        QStringLiteral("modeLine"),
        QStringLiteral("%1 · %2")
            .arg(semanticReadiness.value(QStringLiteral("modeLabel")).toString().trimmed(),
                 primaryEngineLabel(artifacts.report)));
    data.insert(
        QStringLiteral("cacheLine"),
        cacheHitCount > 0
            ? QStringLiteral("四层缓存命中 %1 层，支持更快的增量重建。").arg(cacheHitCount)
            : QStringLiteral("本次走全量重建链路，尚未命中增量缓存。"));
    data.insert(
        QStringLiteral("evidenceLine"),
        QStringLiteral("累计 %1 条规则 / 结论证据，高置信能力域 %2 个。")
            .arg(static_cast<qulonglong>(evidence.rules + evidence.conclusions))
            .arg(static_cast<qulonglong>(evidence.highConfidence)));
    data.insert(QStringLiteral("modeTone"), semanticReadiness.value(QStringLiteral("badgeTone")).toString().trimmed());
    data.insert(QStringLiteral("modeLabel"), semanticReadiness.value(QStringLiteral("modeLabel")).toString().trimmed());
    data.insert(QStringLiteral("engineLabel"), primaryEngineLabel(artifacts.report));
    data.insert(
        QStringLiteral("compilationDatabasePath"),
        semanticReadiness.value(QStringLiteral("compilationDatabasePath")).toString().trimmed());
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
    projection.systemContextData.insert(
        QStringLiteral("projectConfigRecommendation"),
        ProjectConfigRecommendationService::buildRecommendation(
            projectRootPath,
            artifacts.report,
            artifacts.overview,
            artifacts.capabilityGraph));
    const EvidenceRollup evidence = summarizeEvidence(artifacts.capabilityGraph);
    projection.systemContextData.insert(
        QStringLiteral("algorithmSummary"),
        buildAlgorithmSummary(artifacts, semanticReadiness, evidence));
    projection.systemContextData.insert(
        QStringLiteral("algorithmMetrics"),
        buildAlgorithmMetrics(artifacts, semanticReadiness, evidence));
    projection.systemContextData.insert(
        QStringLiteral("algorithmStages"),
        buildAlgorithmStages(artifacts, semanticReadiness, evidence));
    projection.systemContextData.insert(
        QStringLiteral("algorithmCaches"),
        buildCacheLayerItems(artifacts));
    projection.systemContextData.insert(
        QStringLiteral("algorithmEvidence"),
        buildEvidenceItems(evidence));

    projection.systemContextCards =
        ReadingGuideService::buildGuideCards(
            artifacts.report,
            artifacts.overview,
            artifacts.capabilityGraph);
    return projection;
}

}  // namespace savt::ui
