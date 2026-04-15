#include "savt/ui/ReadingGuideService.h"

#include "savt/core/ArchitectureRuleEngine.h"

#include <QFileInfo>
#include <QStringList>

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace savt::ui {

namespace {

QStringList toQStringList(const std::vector<std::string>& values) {
    QStringList items;
    for (const std::string& value : values) {
        const QString cleaned = QString::fromStdString(value).trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned)) {
            items.push_back(cleaned);
        }
    }
    return items;
}

QString projectNameFromRootPath(const QString& rootPath) {
    const QFileInfo info(rootPath);
    return info.fileName().isEmpty() ? QStringLiteral("Current project") : info.fileName();
}

QStringList deduplicateQStringList(const QStringList& values) {
    QStringList items;
    for (const QString& value : values) {
        const QString cleaned = value.trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned)) {
            items.push_back(cleaned);
        }
    }
    return items;
}

QString stripTrailingPunctuation(QString text) {
    text = text.trimmed();
    const QString punctuation = QStringLiteral("。.!！?？;；,，:：");
    while (!text.isEmpty() && punctuation.contains(text.back())) {
        text.chop(1);
    }
    return text.trimmed();
}

QString ensureSentence(QString text) {
    text = text.trimmed();
    if (text.isEmpty()) {
        return {};
    }

    if (text.endsWith(QStringLiteral("。")) || text.endsWith(QStringLiteral("！")) ||
        text.endsWith(QStringLiteral("？")) || text.endsWith(QLatin1Char('.')) ||
        text.endsWith(QLatin1Char('!')) || text.endsWith(QLatin1Char('?'))) {
        return text;
    }
    return text + QStringLiteral("。");
}

QString quoteValue(const QString& value) {
    const QString cleaned = value.trimmed();
    return cleaned.isEmpty() ? QString() : QStringLiteral("「%1」").arg(cleaned);
}

QString joinChinesePreview(
    const QStringList& values,
    const int limit,
    const QString& emptyText = QString(),
    const bool quoteItems = false) {
    const QStringList items = deduplicateQStringList(values);
    if (items.isEmpty()) {
        return emptyText;
    }

    const int count = std::min(limit, static_cast<int>(items.size()));
    QStringList previewItems;
    previewItems.reserve(count);
    for (int index = 0; index < count; ++index) {
        const QString item = items[index].trimmed();
        previewItems.push_back(quoteItems ? quoteValue(item) : item);
    }

    QString preview = previewItems.join(QStringLiteral("、"));
    if (items.size() > count) {
        preview += quoteItems ? QStringLiteral("等 %1 个模块").arg(items.size())
                              : QStringLiteral("等 %1 项").arg(items.size());
    }
    return preview;
}

QString joinPreview(
    const QStringList& values,
    const int limit,
    const QString& emptyText = QString()) {
    const QStringList items = deduplicateQStringList(values);
    if (items.isEmpty()) {
        return emptyText;
    }

    const int count = std::min(limit, static_cast<int>(items.size()));
    QString preview = items.mid(0, count).join(QStringLiteral(", "));
    if (items.size() > count) {
        preview += QStringLiteral(" and %1 more").arg(items.size() - count);
    }
    return preview;
}

bool textContainsAnyToken(const QString& source, const QStringList& tokens) {
    const QString lowered = source.toLower();
    for (const QString& token : tokens) {
        const QString cleaned = token.trimmed().toLower();
        if (!cleaned.isEmpty() && lowered.contains(cleaned)) {
            return true;
        }
    }
    return false;
}

QString capabilityEvidenceBlob(const core::CapabilityNode& node) {
    QStringList parts;
    parts << QString::fromStdString(node.name) << QString::fromStdString(node.dominantRole)
          << QString::fromStdString(node.responsibility) << QString::fromStdString(node.summary)
          << QString::fromStdString(node.folderHint);
    parts.append(toQStringList(node.moduleNames));
    parts.append(toQStringList(node.topSymbols));
    parts.append(toQStringList(node.exampleFiles));
    parts.append(toQStringList(node.collaboratorNames));
    parts.append(toQStringList(node.technologyTags));
    parts.append(toQStringList(node.riskSignals));
    return parts.join(QLatin1Char(' ')).toLower();
}

QString graphEvidenceBlob(const core::CapabilityGraph& graph) {
    QStringList parts;
    for (const core::CapabilityNode& node : graph.nodes) {
        parts.push_back(capabilityEvidenceBlob(node));
    }
    return parts.join(QLatin1Char(' '));
}

bool graphContainsRole(const core::CapabilityGraph& graph, const QStringList& roles) {
    QStringList loweredRoles;
    for (const QString& role : roles) {
        loweredRoles.push_back(role.trimmed().toLower());
    }

    for (const core::CapabilityNode& node : graph.nodes) {
        const QString role = QString::fromStdString(node.dominantRole).trimmed().toLower();
        if (!role.isEmpty() && loweredRoles.contains(role)) {
            return true;
        }
    }
    return false;
}

const core::CapabilityNode* primaryEntryNode(const core::CapabilityGraph& graph) {
    const core::CapabilityNode* bestNode = nullptr;
    for (const core::CapabilityNode& node : graph.nodes) {
        if (node.kind != core::CapabilityNodeKind::Entry) {
            continue;
        }

        if (!bestNode || (node.defaultPinned && !bestNode->defaultPinned) ||
            node.visualPriority > bestNode->visualPriority) {
            bestNode = &node;
        }
    }
    return bestNode;
}

struct OverviewFileTotals {
    std::size_t sourceFiles = 0;
    std::size_t qmlFiles = 0;
    std::size_t webFiles = 0;
    std::size_t scriptFiles = 0;
    std::size_t dataFiles = 0;
};

OverviewFileTotals summarizeOverviewFiles(const core::ArchitectureOverview& overview) {
    OverviewFileTotals totals;
    for (const core::OverviewNode& node : overview.nodes) {
        totals.sourceFiles += node.sourceFileCount;
        totals.qmlFiles += node.qmlFileCount;
        totals.webFiles += node.webFileCount;
        totals.scriptFiles += node.scriptFileCount;
        totals.dataFiles += node.dataFileCount;
    }
    return totals;
}

QStringList inferAudienceHints(const core::CapabilityGraph& graph) {
    const QString evidence = graphEvidenceBlob(graph);
    QStringList hints;

    if (textContainsAnyToken(
            evidence,
            {QStringLiteral("admin"), QStringLiteral("editor"), QStringLiteral("manage"),
             QStringLiteral("management"), QStringLiteral("config")})) {
        hints.push_back(QStringLiteral("admins or configuration maintainers"));
    }
    if (textContainsAnyToken(
            evidence,
            {QStringLiteral("qml"), QStringLiteral("ui"), QStringLiteral("screen"),
             QStringLiteral("window"), QStringLiteral("page"), QStringLiteral("dialog"),
             QStringLiteral("map"), QStringLiteral("webview")})) {
        hints.push_back(QStringLiteral("end users or operators"));
    }
    if (textContainsAnyToken(
            evidence,
            {QStringLiteral("tool"), QStringLiteral("build"), QStringLiteral("script"),
             QStringLiteral("generator"), QStringLiteral("cmake")})) {
        hints.push_back(QStringLiteral("developers or build maintainers"));
    }

    if (hints.isEmpty()) {
        hints.push_back(graphContainsRole(
                            graph,
                            {QStringLiteral("presentation"), QStringLiteral("experience"),
                             QStringLiteral("web")})
                            ? QStringLiteral("end users or operators")
                            : QStringLiteral("developers or maintainers"));
    }

    return deduplicateQStringList(hints);
}

QStringList inferAudienceHintsZh(const core::CapabilityGraph& graph) {
    const QString evidence = graphEvidenceBlob(graph);
    QStringList hints;

    if (textContainsAnyToken(
            evidence,
            {QStringLiteral("admin"), QStringLiteral("editor"), QStringLiteral("manage"),
             QStringLiteral("management"), QStringLiteral("config")})) {
        hints.push_back(QStringLiteral("管理员或配置维护者"));
    }
    if (textContainsAnyToken(
            evidence,
            {QStringLiteral("qml"), QStringLiteral("ui"), QStringLiteral("screen"),
             QStringLiteral("window"), QStringLiteral("page"), QStringLiteral("dialog"),
             QStringLiteral("map"), QStringLiteral("webview")})) {
        hints.push_back(QStringLiteral("终端用户或操作人员"));
    }
    if (textContainsAnyToken(
            evidence,
            {QStringLiteral("tool"), QStringLiteral("build"), QStringLiteral("script"),
             QStringLiteral("generator"), QStringLiteral("cmake")})) {
        hints.push_back(QStringLiteral("开发者或构建维护者"));
    }

    if (hints.isEmpty()) {
        hints.push_back(graphContainsRole(
                            graph,
                            {QStringLiteral("presentation"), QStringLiteral("experience"),
                             QStringLiteral("web")})
                            ? QStringLiteral("终端用户或操作人员")
                            : QStringLiteral("开发者或维护者"));
    }

    return deduplicateQStringList(hints);
}

QString inferProjectKindSummary(
    const core::AnalysisReport&,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph) {
    const auto totals = summarizeOverviewFiles(overview);
    const bool hasPresentation = graphContainsRole(
        graph,
        {QStringLiteral("presentation"), QStringLiteral("experience"), QStringLiteral("web")});
    const bool hasBackend = graphContainsRole(
        graph,
        {QStringLiteral("interaction"), QStringLiteral("analysis"), QStringLiteral("workflow")});
    const bool hasStorage = graphContainsRole(
        graph,
        {QStringLiteral("storage"), QStringLiteral("data"), QStringLiteral("foundation")});

    if (totals.qmlFiles > 0 && totals.sourceFiles > 0 &&
        (totals.webFiles > 0 || hasPresentation)) {
        return QStringLiteral("a Qt/QML application with UI, backend processing, and resource data");
    }
    if (totals.qmlFiles > 0 && totals.sourceFiles > 0) {
        return QStringLiteral("a desktop application with a QML UI and a C++ backend");
    }
    if (totals.webFiles > 0 && totals.sourceFiles > 0) {
        return QStringLiteral("a mixed project with a local backend and a web frontend");
    }
    if (totals.sourceFiles > 0 && hasBackend && hasStorage) {
        return QStringLiteral("a local application with core processing and data support layers");
    }
    if (totals.sourceFiles > 0 && totals.scriptFiles > 0) {
        return QStringLiteral("a C/C++ project supported by scripts and tooling");
    }
    if (totals.sourceFiles > 0) {
        return QStringLiteral("a C/C++ focused project");
    }
    if (totals.scriptFiles > 0) {
        return QStringLiteral("a script and resource oriented project");
    }
    return QStringLiteral("a project that still needs more evidence before its shape is clear");
}

QString inferProjectKindSummaryZh(
    const core::AnalysisReport&,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph) {
    const auto totals = summarizeOverviewFiles(overview);
    const bool hasPresentation = graphContainsRole(
        graph,
        {QStringLiteral("presentation"), QStringLiteral("experience"), QStringLiteral("web")});
    const bool hasBackend = graphContainsRole(
        graph,
        {QStringLiteral("interaction"), QStringLiteral("analysis"), QStringLiteral("workflow")});
    const bool hasStorage = graphContainsRole(
        graph,
        {QStringLiteral("storage"), QStringLiteral("data"), QStringLiteral("foundation")});

    if (totals.qmlFiles > 0 && totals.sourceFiles > 0 &&
        (totals.webFiles > 0 || hasPresentation)) {
        return QStringLiteral("带界面、后端处理和资源数据的 Qt/QML 应用");
    }
    if (totals.qmlFiles > 0 && totals.sourceFiles > 0) {
        return QStringLiteral("带 QML 界面和 C++ 后端的桌面应用");
    }
    if (totals.webFiles > 0 && totals.sourceFiles > 0) {
        return QStringLiteral("本地后端和 Web 前端混合的项目");
    }
    if (totals.sourceFiles > 0 && hasBackend && hasStorage) {
        return QStringLiteral("带核心处理层和数据支撑层的本地应用");
    }
    if (totals.sourceFiles > 0 && totals.scriptFiles > 0) {
        return QStringLiteral("以 C/C++ 为核心、并配有脚本工具链的项目");
    }
    if (totals.sourceFiles > 0) {
        return QStringLiteral("以 C/C++ 为核心的项目");
    }
    if (totals.scriptFiles > 0) {
        return QStringLiteral("以脚本和资源为主的项目");
    }
    return QStringLiteral("还需要更多证据才能判断形态的项目");
}

QString inferEntrySummary(
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph) {
    if (const core::CapabilityNode* entryNode = primaryEntryNode(graph); entryNode != nullptr) {
        return QStringLiteral("The best current entry candidate is \"%1\". It appears to start the main flow before handing work to downstream modules.")
            .arg(QString::fromStdString(entryNode->name));
    }

    if (!overview.flows.empty() && !overview.flows.front().summary.empty()) {
        return QStringLiteral("No stable explicit entry has been confirmed yet, but the main flow currently looks like: %1")
            .arg(QString::fromStdString(overview.flows.front().summary));
    }

    return QStringLiteral("No stable explicit entry has been identified yet. The startup path, root QML component, or boot sequence likely needs more refinement.");
}

QString inferInputSummary(const bool hasNetwork) {
    return hasNetwork
               ? QStringLiteral("Inputs mainly arrive through network APIs, sockets, or external configuration files.")
               : QStringLiteral("Inputs mainly arrive through local system interfaces, user actions, or local files.");
}

QString inferOutputSummary(const bool hasUi) {
    return hasUi ? QStringLiteral("Results are mainly presented through a graphical user interface.")
                 : QStringLiteral("Results are mainly written to logs, the console, or local storage.");
}

QString inferInputSummaryZh(const bool hasNetwork) {
    return hasNetwork
               ? QStringLiteral("输入通常来自网络接口、Socket 或外部配置文件")
               : QStringLiteral("输入通常来自本地文件、用户操作或系统接口");
}

QString inferOutputSummaryZh(const bool hasUi) {
    return hasUi ? QStringLiteral("结果主要通过图形界面呈现")
                 : QStringLiteral("结果主要写入日志、控制台或本地存储");
}

bool isDependencyCapability(const core::CapabilityNode& node) {
    return QString::fromStdString(node.dominantRole)
               .trimmed()
               .compare(QStringLiteral("dependency"), Qt::CaseInsensitive) == 0;
}

struct SystemContextSignals {
    int coreModuleCount = 0;
    bool hasUi = false;
    bool hasDatabase = false;
    bool hasNetwork = false;
    QStringList topModules;
    std::vector<const core::CapabilityNode*> topNodes;
    std::vector<const core::CapabilityNode*> visibleNodes;
};

std::vector<const core::CapabilityNode*> visibleNodesForSummary(
    const core::CapabilityGraph& graph) {
    std::vector<const core::CapabilityNode*> nodes;
    for (const core::CapabilityNode& node : graph.nodes) {
        if (node.defaultVisible) {
            nodes.push_back(&node);
        }
    }

    std::sort(
        nodes.begin(),
        nodes.end(),
        [](const core::CapabilityNode* left, const core::CapabilityNode* right) {
            if (left->defaultPinned != right->defaultPinned) {
                return left->defaultPinned;
            }
            if (left->visualPriority != right->visualPriority) {
                return left->visualPriority > right->visualPriority;
            }
            if (left->fileCount != right->fileCount) {
                return left->fileCount > right->fileCount;
            }
            return left->name < right->name;
        });

    return nodes;
}

SystemContextSignals summarizeSystemContext(const core::CapabilityGraph& graph) {
    SystemContextSignals context;
    context.visibleNodes = visibleNodesForSummary(graph);
    context.coreModuleCount = static_cast<int>(std::count_if(
        context.visibleNodes.begin(),
        context.visibleNodes.end(),
        [](const core::CapabilityNode* node) {
            return node != nullptr && !isDependencyCapability(*node);
        }));
    if (context.coreModuleCount == 0) {
        context.coreModuleCount = static_cast<int>(context.visibleNodes.size());
    }

    for (const core::CapabilityNode* node : context.visibleNodes) {
        const QString evidence = capabilityEvidenceBlob(*node);
        if (textContainsAnyToken(
                evidence,
                {QStringLiteral("ui"), QStringLiteral("qml"), QStringLiteral("view"),
                 QStringLiteral("window"), QStringLiteral("widget"), QStringLiteral("screen")})) {
            context.hasUi = true;
        }
        if (textContainsAnyToken(
                evidence,
                {QStringLiteral("db"), QStringLiteral("database"), QStringLiteral("sql"),
                 QStringLiteral("repo"), QStringLiteral("storage")})) {
            context.hasDatabase = true;
        }
        if (textContainsAnyToken(
                evidence,
                {QStringLiteral("net"), QStringLiteral("network"), QStringLiteral("http"),
                 QStringLiteral("api"), QStringLiteral("client"), QStringLiteral("socket")})) {
            context.hasNetwork = true;
        }

        if (node->kind == core::CapabilityNodeKind::Entry ||
            isDependencyCapability(*node)) {
            continue;
        }

        const QString name = QString::fromStdString(node->name).trimmed();
        if (!name.isEmpty() && !context.topModules.contains(name)) {
            context.topModules.push_back(name);
            context.topNodes.push_back(node);
            if (context.topModules.size() >= 3) {
                break;
            }
        }
    }

    if (context.topModules.isEmpty()) {
        for (const core::CapabilityNode* node : context.visibleNodes) {
            if (isDependencyCapability(*node)) {
                continue;
            }
            const QString name = QString::fromStdString(node->name).trimmed();
            if (!name.isEmpty() && !context.topModules.contains(name)) {
                context.topModules.push_back(name);
                context.topNodes.push_back(node);
                if (context.topModules.size() >= 3) {
                    break;
                }
            }
        }
    }

    return context;
}

QString summarizeMainFlow(
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph) {
    auto summarizeFlowNames = [](const auto& flow, const auto& nodes) {
        std::unordered_map<std::size_t, QString> nodeNames;
        for (const auto& node : nodes) {
            nodeNames.emplace(node.id, QString::fromStdString(node.name).trimmed());
        }

        QStringList names;
        for (const std::size_t nodeId : flow.nodeIds) {
            const auto it = nodeNames.find(nodeId);
            if (it == nodeNames.end()) {
                continue;
            }

            const QString name = it->second.trimmed();
            if (!name.isEmpty() && !names.contains(name)) {
                names.push_back(name);
            }
        }
        return names;
    };

    if (!graph.flows.empty()) {
        const core::CapabilityFlow& flow = graph.flows.front();
        if (!flow.summary.empty()) {
            return QStringLiteral("Main path: %1")
                .arg(QString::fromStdString(flow.summary).trimmed());
        }

        const QStringList names = summarizeFlowNames(flow, graph.nodes);
        if (!names.isEmpty()) {
            return QStringLiteral("Main path: %1")
                .arg(names.join(QStringLiteral(" -> ")));
        }
    }

    if (!overview.flows.empty()) {
        const core::OverviewFlow& flow = overview.flows.front();
        if (!flow.summary.empty()) {
            return QStringLiteral("Main path: %1")
                .arg(QString::fromStdString(flow.summary).trimmed());
        }

        const QStringList names = summarizeFlowNames(flow, overview.nodes);
        if (!names.isEmpty()) {
            return QStringLiteral("Main path: %1")
                .arg(names.join(QStringLiteral(" -> ")));
        }
    }

    return {};
}

QString mainFlowDigest(
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph) {
    QString summary = summarizeMainFlow(overview, graph).trimmed();
    const QString prefix = QStringLiteral("Main path: ");
    if (summary.startsWith(prefix)) {
        summary.remove(0, prefix.size());
    }
    return stripTrailingPunctuation(summary);
}

QStringList buildTechnologyCluesZh(
    const SystemContextSignals& contextSignals,
    const OverviewFileTotals& totals) {
    QStringList clues;
    clues.push_back(QStringLiteral("以 C++ 为主"));
    if (contextSignals.hasUi || totals.qmlFiles > 0) {
        clues.push_back(QStringLiteral("带 Qt/QML 界面层"));
    }
    if (totals.webFiles > 0) {
        clues.push_back(QStringLiteral("包含 Web 资源或前端文件"));
    }
    if (contextSignals.hasDatabase) {
        clues.push_back(QStringLiteral("包含本地存储或数据支撑"));
    }
    if (contextSignals.hasNetwork) {
        clues.push_back(QStringLiteral("包含网络或外部接口集成"));
    }
    if (totals.scriptFiles > 0) {
        clues.push_back(QStringLiteral("带脚本或生成流程"));
    }
    return deduplicateQStringList(clues);
}

QString topNodeResponsibilitySummaryZh(const SystemContextSignals& contextSignals) {
    if (contextSignals.topNodes.empty() || contextSignals.topNodes.front() == nullptr) {
        return {};
    }

    const core::CapabilityNode& node = *contextSignals.topNodes.front();
    const QString name = QString::fromStdString(node.name).trimmed();
    QString detail =
        QString::fromStdString(node.summary.empty() ? node.responsibility : node.summary)
            .trimmed();
    detail = stripTrailingPunctuation(detail);
    if (name.isEmpty() || detail.isEmpty()) {
        return {};
    }

    const QStringList leadingVerbs = {
        QStringLiteral("负责"),
        QStringLiteral("用于"),
        QStringLiteral("处理"),
        QStringLiteral("组织"),
        QStringLiteral("协调"),
        QStringLiteral("管理"),
        QStringLiteral("提供"),
        QStringLiteral("实现"),
        QStringLiteral("扫描"),
        QStringLiteral("解析")
    };
    for (const QString& verb : leadingVerbs) {
        if (detail.startsWith(verb)) {
            return QStringLiteral("其中%1%2").arg(quoteValue(name), detail);
        }
    }

    return QStringLiteral("其中%1当前主要承担 %2").arg(quoteValue(name), detail);
}

QString inferEntryDigestZh(
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph) {
    if (const core::CapabilityNode* entryNode = primaryEntryNode(graph); entryNode != nullptr) {
        return QStringLiteral("当前最明确的入口是%1")
            .arg(quoteValue(QString::fromStdString(entryNode->name)));
    }

    Q_UNUSED(overview);
    return QStringLiteral("当前还没有稳定显式入口");
}

QString buildProjectOverviewZh(
    const QString& projectName,
    const QString& projectKindSummaryZh,
    const QString& audienceSummaryZh,
    const QStringList& topModules,
    const SystemContextSignals& contextSignals,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph,
    const QString& inputSummaryZh,
    const QString& outputSummaryZh,
    const QStringList& technologyCluesZh) {
    QStringList sentences;

    QString positioningSentence = QStringLiteral("%1是一个%2")
        .arg(projectName.trimmed().isEmpty() ? QStringLiteral("这个项目") : projectName.trimmed(),
             projectKindSummaryZh.trimmed().isEmpty()
                 ? QStringLiteral("还需要继续判定用途的项目")
                 : projectKindSummaryZh.trimmed());
    if (!audienceSummaryZh.trimmed().isEmpty()) {
        positioningSentence += QStringLiteral("，适合%1").arg(audienceSummaryZh.trimmed());
    }
    sentences.push_back(ensureSentence(positioningSentence));

    if (!topModules.isEmpty()) {
        QString moduleSentence = QStringLiteral("核心模块包括%1")
            .arg(joinChinesePreview(topModules, 3, QString(), true));
        const QString topResponsibility = topNodeResponsibilitySummaryZh(contextSignals);
        if (!topResponsibility.isEmpty()) {
            moduleSentence += QStringLiteral("，%1").arg(topResponsibility);
        }
        sentences.push_back(ensureSentence(moduleSentence));
    }

    const QString entryDigestZh = inferEntryDigestZh(overview, graph);
    const QString flowDigest = mainFlowDigest(overview, graph);
    if (!entryDigestZh.isEmpty() || !flowDigest.isEmpty()) {
        QString flowSentence = entryDigestZh;
        if (!flowDigest.isEmpty()) {
            flowSentence += flowSentence.isEmpty()
                                ? QStringLiteral("主路径大致是 %1").arg(flowDigest)
                                : QStringLiteral("，主路径大致是 %1").arg(flowDigest);
        }
        sentences.push_back(ensureSentence(flowSentence));
    }

    QString ioAndTechSentence =
        QStringLiteral("%1，%2").arg(inputSummaryZh.trimmed(), outputSummaryZh.trimmed());
    if (!technologyCluesZh.isEmpty()) {
        ioAndTechSentence +=
            QStringLiteral("；技术上%1").arg(technologyCluesZh.join(QStringLiteral("，")));
    }
    sentences.push_back(ensureSentence(ioAndTechSentence));

    return sentences.join(QStringLiteral(" "));
}

QString capabilityCardSummary(const core::CapabilityNode& node) {
    QString summary = QString::fromStdString(
                          node.summary.empty() ? node.responsibility : node.summary)
                          .trimmed();
    QStringList suffixes;

    if (node.cycleParticipant) {
        suffixes.push_back(QStringLiteral("cycle signal detected"));
    }

    const auto degree = node.incomingEdgeCount + node.outgoingEdgeCount;
    if (degree >= 8) {
        suffixes.push_back(QStringLiteral("high fan-in or fan-out"));
    }

    if (node.kind == core::CapabilityNodeKind::Infrastructure &&
        node.incomingEdgeCount >= 3) {
        suffixes.push_back(QStringLiteral("shared by several capabilities"));
    }

    if (!suffixes.isEmpty()) {
        if (!summary.isEmpty() && !summary.endsWith(QLatin1Char('.'))) {
            summary += QStringLiteral(". ");
        }
        summary += suffixes.join(QStringLiteral("; ")) + QStringLiteral(".");
    }

    if (summary.isEmpty()) {
        summary = QStringLiteral("Use the capability map and component detail view to confirm this module's responsibility boundary.");
    }

    return summary.trimmed();
}

std::unordered_map<std::size_t, const core::CapabilityNode*> buildCapabilityNodeIndex(
    const core::CapabilityGraph& graph) {
    std::unordered_map<std::size_t, const core::CapabilityNode*> nodeIndex;
    nodeIndex.reserve(graph.nodes.size());
    for (const core::CapabilityNode& node : graph.nodes) {
        nodeIndex.emplace(node.id, &node);
    }
    return nodeIndex;
}

QVariantList toVariantNodeIds(const std::vector<std::size_t>& nodeIds) {
    QVariantList items;
    items.reserve(static_cast<int>(nodeIds.size()));
    for (const std::size_t nodeId : nodeIds) {
        items.push_back(QVariant::fromValue(static_cast<qulonglong>(nodeId)));
    }
    return items;
}

QVariantList toVariantReasonCodes(const std::vector<std::string>& reasonCodes) {
    QVariantList items;
    items.reserve(static_cast<int>(reasonCodes.size()));
    for (const std::string& reasonCode : reasonCodes) {
        items.push_back(QString::fromStdString(reasonCode));
    }
    return items;
}

QString summarizeNodeNames(
    const std::vector<std::size_t>& nodeIds,
    const std::unordered_map<std::size_t, const core::CapabilityNode*>& nodeIndex) {
    QStringList names;
    for (const std::size_t nodeId : nodeIds) {
        const auto it = nodeIndex.find(nodeId);
        if (it == nodeIndex.end() || it->second == nullptr) {
            continue;
        }
        const QString name = QString::fromStdString(it->second->name).trimmed();
        if (!name.isEmpty() && !names.contains(name)) {
            names.push_back(name);
        }
    }

    if (names.isEmpty()) {
        return QStringLiteral("Unnamed capability");
    }

    const int count = std::min(2, static_cast<int>(names.size()));
    QString preview = names.mid(0, count).join(QStringLiteral(", "));
    if (names.size() > count) {
        preview += QStringLiteral(" +%1 more").arg(names.size() - count);
    }
    return preview;
}

QString summarizeRuleFinding(const core::ArchitectureRuleFinding& finding) {
    switch (finding.kind) {
    case core::ArchitectureRuleKind::Cycle:
        return QStringLiteral(
            "Part of a dependency cycle. Review bidirectional coupling before changing this area.");
    case core::ArchitectureRuleKind::Hub:
        return QStringLiteral(
            "Acts like a connectivity hub with high fan-in or fan-out. Check whether too much responsibility is concentrated here.");
    case core::ArchitectureRuleKind::Isolated:
        return QStringLiteral(
            "Weakly connected or isolated. Confirm whether it is intentionally standalone, stale, or missing relationships.");
    default:
        return QStringLiteral("Flagged by a structural rule.");
    }
}

QVariantList buildRuleFindings(
    const core::ArchitectureRuleReport& ruleReport) {
    QVariantList items;
    items.reserve(static_cast<int>(ruleReport.findings.size()));
    for (const core::ArchitectureRuleFinding& finding : ruleReport.findings) {
        QVariantMap item;
        item.insert(QStringLiteral("kind"), QString::fromUtf8(core::toString(finding.kind)));
        item.insert(
            QStringLiteral("severity"),
            QString::fromUtf8(core::toString(finding.severity)));
        item.insert(QStringLiteral("nodeIds"), toVariantNodeIds(finding.nodeIds));
        item.insert(
            QStringLiteral("reasonCodes"),
            toVariantReasonCodes(finding.reasonCodes));
        items.push_back(item);
    }
    return items;
}

QVariantList buildRiskSignals(
    const core::ArchitectureRuleReport& ruleReport,
    const core::CapabilityGraph& graph) {
    const auto nodeIndex = buildCapabilityNodeIndex(graph);
    QVariantList riskItems;
    const int limit = std::min<int>(3, static_cast<int>(ruleReport.findings.size()));
    for (int index = 0; index < limit; ++index) {
        const core::ArchitectureRuleFinding& finding =
            ruleReport.findings[static_cast<std::size_t>(index)];
        QVariantMap item;
        item.insert(
            QStringLiteral("title"),
            summarizeNodeNames(finding.nodeIds, nodeIndex));
        item.insert(QStringLiteral("summary"), summarizeRuleFinding(finding));
        item.insert(QStringLiteral("kind"), QString::fromUtf8(core::toString(finding.kind)));
        item.insert(
            QStringLiteral("severity"),
            QString::fromUtf8(core::toString(finding.severity)));
        item.insert(QStringLiteral("nodeIds"), toVariantNodeIds(finding.nodeIds));
        item.insert(
            QStringLiteral("reasonCodes"),
            toVariantReasonCodes(finding.reasonCodes));

        QString primaryCapabilityName;
        if (!finding.nodeIds.empty()) {
            const auto nodeIt = nodeIndex.find(finding.nodeIds.front());
            if (nodeIt != nodeIndex.end() && nodeIt->second != nullptr) {
                primaryCapabilityName = QString::fromStdString(nodeIt->second->name);
            }
        }
        item.insert(QStringLiteral("capabilityName"), primaryCapabilityName);
        riskItems.push_back(item);
    }

    return riskItems;
}

struct HotspotCandidate {
    const core::CapabilityNode* node = nullptr;
    int score = 0;
};

std::size_t totalEdgeCount(const core::CapabilityNode& node) {
    return node.incomingEdgeCount + node.outgoingEdgeCount;
}

int hotspotScore(const core::CapabilityNode& node) {
    int score = 0;
    score += std::min<int>(12, static_cast<int>(totalEdgeCount(node)));
    score += std::min<int>(8, static_cast<int>(node.fileCount / 4));
    score += std::min<int>(8, static_cast<int>(node.flowParticipationCount * 3));
    score += std::min<int>(8, static_cast<int>(node.riskScore));
    if (node.cycleParticipant) {
        score += 10;
    }

    const QString riskLevel =
        QString::fromStdString(node.riskLevel).trimmed().toLower();
    if (riskLevel == QStringLiteral("high")) {
        score += 6;
    } else if (riskLevel == QStringLiteral("medium")) {
        score += 3;
    }

    return score;
}

QString hotspotSeverity(const int score) {
    if (score >= 24) {
        return QStringLiteral("high");
    }
    if (score >= 14) {
        return QStringLiteral("medium");
    }
    return QStringLiteral("low");
}

QString hotspotSummary(const core::CapabilityNode& node) {
    QStringList reasons;
    const std::size_t degree = totalEdgeCount(node);
    if (degree >= 4) {
        reasons.push_back(QStringLiteral("touches %1 cross-module links").arg(degree));
    }
    if (node.fileCount >= 8) {
        reasons.push_back(QStringLiteral("owns %1 files").arg(node.fileCount));
    }
    if (node.flowParticipationCount >= 2) {
        reasons.push_back(QStringLiteral("shows up in %1 main flows")
                              .arg(node.flowParticipationCount));
    }
    if (node.cycleParticipant) {
        reasons.push_back(QStringLiteral("participates in a dependency cycle"));
    }

    const QString riskLevel =
        QString::fromStdString(node.riskLevel).trimmed().toLower();
    if (riskLevel == QStringLiteral("high") || node.riskScore >= 6) {
        reasons.push_back(QStringLiteral("already carries elevated structural risk"));
    } else if (riskLevel == QStringLiteral("medium") || node.riskScore >= 3) {
        reasons.push_back(QStringLiteral("already shows medium structural risk"));
    }

    if (reasons.isEmpty()) {
        return capabilityCardSummary(node);
    }

    return QStringLiteral("Worth checking early because it %1.")
        .arg(reasons.join(QStringLiteral(", ")));
}

QVariantList buildHotspotSignals(const core::CapabilityGraph& graph) {
    std::vector<HotspotCandidate> candidates;
    candidates.reserve(graph.nodes.size());
    for (const core::CapabilityNode& node : graph.nodes) {
        if (!node.defaultVisible ||
            node.kind == core::CapabilityNodeKind::Entry ||
            isDependencyCapability(node)) {
            continue;
        }

        const int score = hotspotScore(node);
        if (score < 10) {
            continue;
        }

        candidates.push_back({&node, score});
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const HotspotCandidate& left, const HotspotCandidate& right) {
            if (left.score != right.score) {
                return left.score > right.score;
            }
            if (left.node->visualPriority != right.node->visualPriority) {
                return left.node->visualPriority > right.node->visualPriority;
            }
            if (left.node->fileCount != right.node->fileCount) {
                return left.node->fileCount > right.node->fileCount;
            }
            return left.node->name < right.node->name;
        });

    QVariantList items;
    const int limit = std::min<int>(3, static_cast<int>(candidates.size()));
    for (int index = 0; index < limit; ++index) {
        const HotspotCandidate& candidate =
            candidates[static_cast<std::size_t>(index)];
        QVariantMap item;
        item.insert(
            QStringLiteral("title"),
            QString::fromStdString(candidate.node->name).trimmed());
        item.insert(QStringLiteral("summary"), hotspotSummary(*candidate.node));
        item.insert(QStringLiteral("kind"), QStringLiteral("hotspot"));
        item.insert(QStringLiteral("severity"), hotspotSeverity(candidate.score));
        item.insert(
            QStringLiteral("nodeId"),
            QVariant::fromValue(static_cast<qulonglong>(candidate.node->id)));
        item.insert(
            QStringLiteral("capabilityName"),
            QString::fromStdString(candidate.node->name).trimmed());
        items.push_back(item);
    }

    return items;
}

QString summarizeStructuredSignals(
    const QVariantList& items,
    const QString& titleField,
    const QString& bodyField,
    const int limit) {
    QStringList parts;
    const int count = std::min(limit, static_cast<int>(items.size()));
    for (int index = 0; index < count; ++index) {
        const QVariantMap item = items[static_cast<std::size_t>(index)].toMap();
        const QString title = item.value(titleField).toString().trimmed();
        const QString body = item.value(bodyField).toString().trimmed();
        QString part;
        if (!title.isEmpty() && !body.isEmpty()) {
            part = QStringLiteral("%1: %2").arg(title, body);
        } else if (!title.isEmpty()) {
            part = title;
        } else {
            part = body;
        }

        if (!part.isEmpty() && !parts.contains(part)) {
            parts.push_back(part);
        }
    }

    return parts.join(QStringLiteral("\n"));
}

QVariantList buildReadingOrder(
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph,
    const SystemContextSignals& contextSignals,
    const QVariantList& riskSignals,
    const QVariantList& hotspotSignals,
    const QString& mainFlowSummary) {
    QVariantList steps;

    QVariantMap step1;
    step1.insert(QStringLiteral("title"), QStringLiteral("1. Start with the entry"));
    step1.insert(QStringLiteral("body"), inferEntrySummary(overview, graph));
    step1.insert(QStringLiteral("pageId"), QStringLiteral("project.overview"));
    steps.push_back(step1);

    QString coreModulesBody =
        contextSignals.topModules.isEmpty()
            ? QStringLiteral("Next, open the capability map and lock onto the most stable primary capability before drilling down.")
            : QStringLiteral("Then read core modules such as \"%1\" and connect their responsibilities into one main path.")
                  .arg(contextSignals.topModules.join(QStringLiteral("\", \"")));
    if (!mainFlowSummary.isEmpty()) {
        coreModulesBody = QStringLiteral("%1 Then trace %2.")
                              .arg(coreModulesBody, mainFlowSummary.toLower());
    }

    QVariantMap step2;
    step2.insert(QStringLiteral("title"), QStringLiteral("2. Read core modules"));
    step2.insert(QStringLiteral("body"), coreModulesBody);
    step2.insert(QStringLiteral("pageId"), QStringLiteral("project.capabilityMap"));
    steps.push_back(step2);

    QString focusBody =
        QStringLiteral("Finally, open the component workbench and keep drilling into the selected capability until you reach concrete code entry points.");
    if (!riskSignals.isEmpty()) {
        const QVariantMap topRisk = riskSignals.front().toMap();
        const QString riskName = topRisk.value(QStringLiteral("title")).toString().trimmed();
        const QString riskSummary =
            topRisk.value(QStringLiteral("summary")).toString().trimmed();
        if (!riskName.isEmpty() && !riskSummary.isEmpty()) {
            focusBody = QStringLiteral("Finally, pay extra attention to \"%1\": %2")
                            .arg(riskName, riskSummary);
        }
    } else if (!hotspotSignals.isEmpty()) {
        const QVariantMap topHotspot = hotspotSignals.front().toMap();
        const QString hotspotName =
            topHotspot.value(QStringLiteral("capabilityName")).toString().trimmed();
        const QString hotspotSummaryText =
            topHotspot.value(QStringLiteral("summary")).toString().trimmed();
        if (!hotspotName.isEmpty() && !hotspotSummaryText.isEmpty()) {
            focusBody = QStringLiteral("Finally, drill into \"%1\" early: %2")
                            .arg(hotspotName, hotspotSummaryText);
        }
    }

    QVariantMap step3;
    step3.insert(QStringLiteral("title"), QStringLiteral("3. Finish with the key risk"));
    step3.insert(QStringLiteral("body"), focusBody);
    step3.insert(QStringLiteral("pageId"), QStringLiteral("project.componentWorkbench"));
    steps.push_back(step3);

    return steps;
}

}  // namespace

QVariantMap ReadingGuideService::buildGuide(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph,
    const QString& projectRootPath) {
    const core::ArchitectureRuleReport ruleReport =
        core::evaluateArchitectureRules(graph, overview);
    return buildGuide(report, overview, graph, ruleReport, projectRootPath);
}

QVariantMap ReadingGuideService::buildGuide(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph,
    const core::ArchitectureRuleReport& ruleReport,
    const QString& projectRootPath) {
    const QString projectName = projectNameFromRootPath(projectRootPath);
    const auto contextSignals = summarizeSystemContext(graph);
    const auto totals = summarizeOverviewFiles(overview);
    const QString projectKindSummary = inferProjectKindSummary(report, overview, graph);
    const QString projectKindSummaryZh =
        inferProjectKindSummaryZh(report, overview, graph);
    const QStringList topModules = contextSignals.topModules;
    const QVariantList ruleFindings = buildRuleFindings(ruleReport);
    const QVariantList riskSignals = buildRiskSignals(ruleReport, graph);
    const QVariantList hotspotSignals = buildHotspotSignals(graph);
    const QString mainFlowSummary = summarizeMainFlow(overview, graph);

    QStringList technologyClues;
    technologyClues.push_back(QStringLiteral("implemented mainly in C++"));
    if (contextSignals.hasUi || totals.qmlFiles > 0) {
        technologyClues.push_back(QStringLiteral("contains a Qt/QML UI layer"));
    }
    if (contextSignals.hasDatabase) {
        technologyClues.push_back(QStringLiteral("includes persistence or local storage concerns"));
    }
    if (contextSignals.hasNetwork) {
        technologyClues.push_back(QStringLiteral("includes networking or external interface integration"));
    }
    if (totals.scriptFiles > 0) {
        technologyClues.push_back(QStringLiteral("includes scripts or generation flows"));
    }

    const QString audienceSummary = joinPreview(
        inferAudienceHints(graph),
        3,
        QStringLiteral("developers, maintainers, or anyone onboarding to the codebase"));
    const QString audienceSummaryZh = joinChinesePreview(
        inferAudienceHintsZh(graph),
        3,
        QStringLiteral("开发者、维护者或需要快速接手代码的人"));
    const QString entrySummary = inferEntrySummary(overview, graph);
    const QString inputSummary = inferInputSummary(contextSignals.hasNetwork);
    const QString inputSummaryZh = inferInputSummaryZh(contextSignals.hasNetwork);
    const QString outputSummary = inferOutputSummary(contextSignals.hasUi);
    const QString outputSummaryZh = inferOutputSummaryZh(contextSignals.hasUi);
    const QString technologySummary = technologyClues.isEmpty()
        ? QStringLiteral("No stable technology summary is available yet.")
        : technologyClues.join(QStringLiteral("; ")) + QStringLiteral(".");
    const QStringList technologyCluesZh =
        buildTechnologyCluesZh(contextSignals, totals);

    QString purposeSummary = projectKindSummary.trimmed().isEmpty()
                                 ? QStringLiteral("Project shape still needs confirmation.")
                                 : ensureSentence(projectKindSummary);
    if (!topModules.isEmpty()) {
        purposeSummary += QStringLiteral(" Read \"%1\" first.")
                              .arg(topModules.join(QStringLiteral("\", \"")));
    }
    if (!mainFlowSummary.isEmpty()) {
        purposeSummary += QStringLiteral(" %1.").arg(mainFlowSummary);
    }
    const QString projectOverview = buildProjectOverviewZh(
        projectName,
        projectKindSummaryZh,
        audienceSummaryZh,
        topModules,
        contextSignals,
        overview,
        graph,
        inputSummaryZh,
        outputSummaryZh,
        technologyCluesZh);

    QVariantList contextSections;
    auto appendContextSection = [&](const QString& title,
                                    const QString& body,
                                    const QString& kind,
                                    const QString& pageId = QString(),
                                    const QString& capabilityName = QString()) {
        const QString cleanedTitle = title.trimmed();
        const QString cleanedBody = body.trimmed();
        if (cleanedTitle.isEmpty() || cleanedBody.isEmpty()) {
            return;
        }

        QVariantMap item;
        item.insert(QStringLiteral("title"), cleanedTitle);
        item.insert(QStringLiteral("body"), cleanedBody);
        item.insert(QStringLiteral("kind"), kind.trimmed());
        item.insert(QStringLiteral("pageId"), pageId.trimmed());
        item.insert(QStringLiteral("capabilityName"), capabilityName.trimmed());
        contextSections.push_back(item);
    };

    appendContextSection(
        QStringLiteral("项目定位"),
        purposeSummary,
        QStringLiteral("summary"),
        QStringLiteral("project.overview"));
    appendContextSection(
        QStringLiteral("适合谁看"),
        audienceSummary,
        QStringLiteral("audience"),
        QStringLiteral("project.overview"));
    appendContextSection(
        QStringLiteral("入口"),
        entrySummary,
        QStringLiteral("entry"),
        QStringLiteral("project.overview"));
    appendContextSection(
        QStringLiteral("主路径"),
        mainFlowSummary,
        QStringLiteral("main_flow"),
        QStringLiteral("project.capabilityMap"));
    appendContextSection(
        QStringLiteral("输入 / 输出"),
        QStringLiteral("%1\n%2")
            .arg(inputSummary, outputSummary)
            .trimmed(),
        QStringLiteral("io"),
        QStringLiteral("project.overview"));
    appendContextSection(
        QStringLiteral("技术线索"),
        technologySummary,
        QStringLiteral("technology"),
        QStringLiteral("project.overview"));
    appendContextSection(
        QStringLiteral("优先模块"),
        topModules.isEmpty()
            ? QStringLiteral("No stable core modules have been extracted yet.")
            : topModules.join(QStringLiteral("\n")),
        QStringLiteral("modules"),
        QStringLiteral("project.capabilityMap"));
    appendContextSection(
        QStringLiteral("结构热点"),
        summarizeStructuredSignals(
            hotspotSignals,
            QStringLiteral("title"),
            QStringLiteral("summary"),
            3),
        QStringLiteral("hotspots"),
        QStringLiteral("project.capabilityMap"),
        hotspotSignals.isEmpty()
            ? QString()
            : hotspotSignals.front()
                  .toMap()
                  .value(QStringLiteral("capabilityName"))
                  .toString());
    appendContextSection(
        QStringLiteral("关键风险"),
        summarizeStructuredSignals(
            riskSignals,
            QStringLiteral("title"),
            QStringLiteral("summary"),
            3),
        QStringLiteral("risks"),
        QStringLiteral("report.engineering"),
        riskSignals.isEmpty()
            ? QString()
            : riskSignals.front()
                  .toMap()
                  .value(QStringLiteral("capabilityName"))
                  .toString());

    QVariantMap data;
    data.insert(QStringLiteral("projectName"), projectName);
    data.insert(QStringLiteral("title"), QStringLiteral("L1 System Context"));
    data.insert(
        QStringLiteral("headline"),
        mainFlowSummary.isEmpty()
            ? QStringLiteral("%1 core modules around one main entry.")
                  .arg(contextSignals.coreModuleCount)
            : QStringLiteral("%1 core modules with a traceable main path.")
                  .arg(contextSignals.coreModuleCount));
    data.insert(
        QStringLiteral("audienceSummary"),
        audienceSummary);
    data.insert(QStringLiteral("projectOverview"), projectOverview);
    data.insert(QStringLiteral("projectOverviewBusy"), false);
    data.insert(QStringLiteral("projectOverviewSource"), QStringLiteral("heuristic"));
    data.insert(
        QStringLiteral("projectOverviewStatus"),
        QStringLiteral("来自结构推断"));
    data.insert(QStringLiteral("purposeSummary"), purposeSummary);
    data.insert(QStringLiteral("projectKindSummary"), projectKindSummary);
    data.insert(QStringLiteral("entrySummary"), entrySummary);
    data.insert(QStringLiteral("inputSummary"), inputSummary);
    data.insert(QStringLiteral("outputSummary"), outputSummary);
    data.insert(QStringLiteral("technologySummary"), technologySummary);
    data.insert(
        QStringLiteral("containerSummary"),
        topModules.isEmpty()
            ? QStringLiteral("No stable core modules have been extracted yet. Start from the entry and the main flow.")
            : QStringLiteral("The modules most worth reading first are \"%1\". Use the capability map to connect them into one path%2%3.")
                  .arg(topModules.join(QStringLiteral("\", \"")),
                       mainFlowSummary.isEmpty()
                           ? QString()
                           : QStringLiteral(", keeping %1 in view")
                                 .arg(mainFlowSummary.toLower()),
                       hotspotSignals.isEmpty()
                           ? QString()
                           : QStringLiteral("; then verify structural hotspots such as \"%1\"")
                                 .arg(hotspotSignals.front()
                                          .toMap()
                                          .value(QStringLiteral("capabilityName"))
                                          .toString()
                                          .trimmed())));
    data.insert(QStringLiteral("containerNames"), topModules);
    data.insert(QStringLiteral("topModules"), topModules);
    data.insert(QStringLiteral("mainFlowSummary"), mainFlowSummary);
    data.insert(QStringLiteral("contextSections"), contextSections);
    data.insert(QStringLiteral("ruleFindings"), ruleFindings);
    data.insert(
        QStringLiteral("readingOrder"),
        buildReadingOrder(
            overview, graph, contextSignals, riskSignals, hotspotSignals, mainFlowSummary));
    data.insert(QStringLiteral("riskSignals"), riskSignals);
    data.insert(QStringLiteral("hotspotSignals"), hotspotSignals);
    return data;
}

QVariantList ReadingGuideService::buildGuideCards(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph) {
    Q_UNUSED(report);
    Q_UNUSED(overview);

    const auto contextSignals = summarizeSystemContext(graph);
    const QVariantList hotspotSignals = buildHotspotSignals(graph);
    QVariantList cards;
    auto appendCard = [&](const QString& name,
                          const QString& summary,
                          const QString& kind,
                          const QString& capabilityName) {
        if (name.trimmed().isEmpty() || summary.trimmed().isEmpty()) {
            return;
        }

        QVariantMap card;
        card.insert(QStringLiteral("name"), name.trimmed());
        card.insert(QStringLiteral("summary"), summary.trimmed());
        card.insert(QStringLiteral("kind"), kind.trimmed());
        card.insert(
            QStringLiteral("capabilityName"),
            capabilityName.trimmed().isEmpty() ? name.trimmed() : capabilityName.trimmed());
        cards.push_back(card);
    };

    if (const core::CapabilityNode* entryNode = primaryEntryNode(graph); entryNode != nullptr) {
        const QString name = QString::fromStdString(entryNode->name).trimmed();
        appendCard(
            name,
            capabilityCardSummary(*entryNode),
            QStringLiteral("entry"),
            name);
    }

    for (const core::CapabilityNode* node : contextSignals.topNodes) {
        if (node == nullptr) {
            continue;
        }

        const QString name = QString::fromStdString(node->name).trimmed();
        appendCard(
            name,
            capabilityCardSummary(*node),
            QString::fromUtf8(core::toString(node->kind)),
            name);
    }

    const int hotspotCardLimit = std::min<int>(2, hotspotSignals.size());
    for (int index = 0; index < hotspotCardLimit; ++index) {
        const QVariantMap hotspot = hotspotSignals[index].toMap();
        const QString capabilityName =
            hotspot.value(QStringLiteral("capabilityName")).toString().trimmed();
        const QString summary = hotspot.value(QStringLiteral("summary")).toString().trimmed();
        if (capabilityName.isEmpty() || summary.isEmpty()) {
            continue;
        }

        appendCard(
            QStringLiteral("Hotspot: %1").arg(capabilityName),
            summary,
            QStringLiteral("hotspot"),
            capabilityName);
    }

    return cards;
}

}  // namespace savt::ui
