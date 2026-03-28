#include "savt/ui/ReportService.h"

#include <QFileInfo>
#include <QStringList>

#include <algorithm>
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
    const QFileInfo fileInfo(rootPath);
    return fileInfo.fileName().isEmpty() ? QStringLiteral("当前项目") : fileInfo.fileName();
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

QString joinPreview(const QStringList& values, const int limit, const QString& emptyText = QString()) {
    const QStringList items = deduplicateQStringList(values);
    if (items.isEmpty()) {
        return emptyText;
    }

    const int count = std::min(limit, static_cast<int>(items.size()));
    QString preview = items.mid(0, count).join(QStringLiteral("、"));
    if (items.size() > count) {
        preview += QStringLiteral(" 等另外 %1 项").arg(items.size() - count);
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

QString capabilityEvidenceBlob(const core::CapabilityGraph& graph) {
    QStringList parts;
    for (const core::CapabilityNode& node : graph.nodes) {
        parts.push_back(QString::fromStdString(node.name));
        parts.push_back(QString::fromStdString(node.dominantRole));
        parts.push_back(QString::fromStdString(node.responsibility));
        parts.push_back(QString::fromStdString(node.summary));
        parts.push_back(QString::fromStdString(node.folderHint));
        parts.append(toQStringList(node.moduleNames));
        parts.append(toQStringList(node.exampleFiles));
        parts.append(toQStringList(node.topSymbols));
        parts.append(toQStringList(node.collaboratorNames));
    }
    return parts.join(QLatin1Char(' ')).toLower();
}

bool graphContainsRole(const core::CapabilityGraph& graph, const QStringList& roles) {
    for (const core::CapabilityNode& node : graph.nodes) {
        if (roles.contains(QString::fromStdString(node.dominantRole))) {
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
    const QString evidence = capabilityEvidenceBlob(graph);
    QStringList hints;
    if (textContainsAnyToken(
            evidence,
            {QStringLiteral("admin"), QStringLiteral("editor"), QStringLiteral("manage"),
             QStringLiteral("management"), QStringLiteral("config")})) {
        hints.push_back(QStringLiteral("管理员或配置维护人员"));
    }
    if (textContainsAnyToken(
            evidence,
            {QStringLiteral("qml"), QStringLiteral("ui"), QStringLiteral("screen"),
             QStringLiteral("window"), QStringLiteral("page"), QStringLiteral("dialog"),
             QStringLiteral("map"), QStringLiteral("webview")})) {
        hints.push_back(QStringLiteral("终端用户或业务操作人员"));
    }
    if (textContainsAnyToken(
            evidence,
            {QStringLiteral("tool"), QStringLiteral("build"), QStringLiteral("script"),
             QStringLiteral("generator"), QStringLiteral("cmake")})) {
        hints.push_back(QStringLiteral("开发者或构建人员"));
    }
    if (hints.isEmpty()) {
        hints.push_back(graphContainsRole(
                            graph,
                            {QStringLiteral("presentation"), QStringLiteral("experience"),
                             QStringLiteral("web")})
                            ? QStringLiteral("终端用户或业务操作人员")
                            : QStringLiteral("开发者或维护人员"));
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
    if (totals.qmlFiles > 0 && totals.sourceFiles > 0 && (totals.webFiles > 0 || hasPresentation)) {
        return QStringLiteral("一个带图形界面、后端处理和资源数据的 Qt/QML 应用");
    }
    if (totals.qmlFiles > 0 && totals.sourceFiles > 0) {
        return QStringLiteral("一个以 QML 界面配合 C++ 后端的桌面应用");
    }
    if (totals.webFiles > 0 && totals.sourceFiles > 0) {
        return QStringLiteral("一个同时包含本地后端和网页前端的混合应用");
    }
    if (totals.sourceFiles > 0 && hasBackend && hasStorage) {
        return QStringLiteral("一个包含核心处理与数据支撑层的本地应用工程");
    }
    if (totals.sourceFiles > 0 && totals.scriptFiles > 0) {
        return QStringLiteral("一个以 C/C++ 为主、脚本辅助处理的工程");
    }
    if (totals.sourceFiles > 0) {
        return QStringLiteral("一个以 C/C++ 为主的工程");
    }
    if (totals.scriptFiles > 0) {
        return QStringLiteral("一个以脚本和资源处理为主的工程");
    }
    return QStringLiteral("一个仍需要更多证据来确认形态的项目");
}

QString inferEntrySummary(const core::ArchitectureOverview& overview, const core::CapabilityGraph& graph) {
    if (const core::CapabilityNode* entryNode = primaryEntryNode(graph); entryNode != nullptr) {
        return QStringLiteral("当前识别到最像主入口的是“%1”，它会先发起流程，再把工作交给后续模块。")
            .arg(QString::fromStdString(entryNode->name));
    }
    if (!overview.flows.empty() && !overview.flows.front().summary.empty()) {
        return QStringLiteral("还没有检测到特别稳定的显式入口，但现有依赖图显示主线大致是：%1")
            .arg(QString::fromStdString(overview.flows.front().summary));
    }
    return QStringLiteral(
        "当前还没有识别到稳定的显式入口，这通常意味着入口文件、QML 根组件或启动链路还需要继续细化。");
}

struct SystemContextSignals {
    int coreModuleCount = 0;
    bool hasUI = false;
    bool hasDatabase = false;
    bool hasNetwork = false;
    QStringList topModules;
};

QString capabilityNodeEvidence(const core::CapabilityNode& node) {
    QStringList parts;
    parts << QString::fromStdString(node.name) << QString::fromStdString(node.dominantRole)
          << QString::fromStdString(node.responsibility) << QString::fromStdString(node.summary)
          << QString::fromStdString(node.folderHint);
    parts.append(toQStringList(node.moduleNames));
    parts.append(toQStringList(node.topSymbols));
    parts.append(toQStringList(node.exampleFiles));
    return parts.join(QLatin1Char(' ')).toLower();
}

std::vector<const core::CapabilityNode*> visibleNodesForSummary(const core::CapabilityGraph& graph) {
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
    SystemContextSignals contextSignals;
    const auto visibleNodes = visibleNodesForSummary(graph);
    contextSignals.coreModuleCount = static_cast<int>(visibleNodes.size());

    QStringList topModules;
    for (const core::CapabilityNode* node : visibleNodes) {
        const QString evidence = capabilityNodeEvidence(*node);
        if (evidence.contains(QStringLiteral("ui")) || evidence.contains(QStringLiteral("qml")) ||
            evidence.contains(QStringLiteral("view")) || evidence.contains(QStringLiteral("window")) ||
            evidence.contains(QStringLiteral("widget")) || evidence.contains(QStringLiteral("界面"))) {
            contextSignals.hasUI = true;
        }
        if (evidence.contains(QStringLiteral("db")) || evidence.contains(QStringLiteral("database")) ||
            evidence.contains(QStringLiteral("sql")) || evidence.contains(QStringLiteral("repo")) ||
            evidence.contains(QStringLiteral("storage")) || evidence.contains(QStringLiteral("存储"))) {
            contextSignals.hasDatabase = true;
        }
        if (evidence.contains(QStringLiteral("net")) ||
            evidence.contains(QStringLiteral("network")) || evidence.contains(QStringLiteral("http")) ||
            evidence.contains(QStringLiteral("api")) || evidence.contains(QStringLiteral("client")) ||
            evidence.contains(QStringLiteral("socket")) || evidence.contains(QStringLiteral("网络"))) {
            contextSignals.hasNetwork = true;
        }

        if (node->kind == core::CapabilityNodeKind::Entry) {
            continue;
        }

        const QString name = QString::fromStdString(node->name).trimmed();
        if (!name.isEmpty() && !topModules.contains(name)) {
            topModules.push_back(name);
            if (topModules.size() >= 3) {
                break;
            }
        }
    }

    if (topModules.isEmpty()) {
        for (const core::CapabilityNode* node : visibleNodes) {
            const QString name = QString::fromStdString(node->name).trimmed();
            if (!name.isEmpty() && !topModules.contains(name)) {
                topModules.push_back(name);
                if (topModules.size() >= 3) {
                    break;
                }
            }
        }
    }

    contextSignals.topModules = topModules;
    return contextSignals;
}

QString semanticFailureExplanation(const core::AnalysisReport& report) {
    const QString statusCode = QString::fromStdString(report.semanticStatusCode).trimmed();
    if (statusCode == QStringLiteral("missing_compile_commands")) {
        return QStringLiteral("未找到 compile_commands.json。请先为被分析项目生成编译数据库。");
    }
    if (statusCode == QStringLiteral("backend_unavailable")) {
        return QStringLiteral("当前 SAVT 构建未包含 Clang/LibTooling 语义后端。请重新配置并启用 SAVT_ENABLE_CLANG_TOOLING。");
    }
    if (statusCode == QStringLiteral("llvm_not_found")) {
        return QStringLiteral("已启用语义后端，但没有找到可用的 LLVM/Clang 安装。请检查 SAVT_LLVM_ROOT。");
    }
    if (statusCode == QStringLiteral("llvm_headers_missing")) {
        return QStringLiteral("已找到 LLVM 目录，但缺少 clang-c/Index.h。请检查开发头文件是否完整。");
    }
    if (statusCode == QStringLiteral("libclang_not_found")) {
        return QStringLiteral("已找到 LLVM 目录，但缺少 libclang。请检查安装包是否包含 libclang。");
    }
    if (statusCode == QStringLiteral("compilation_database_load_failed")) {
        return QStringLiteral("已找到 compile_commands.json，但 libclang 无法加载它。请确认它位于实际构建目录且内容有效。");
    }
    if (statusCode == QStringLiteral("compilation_database_empty")) {
        return QStringLiteral("已找到 compile_commands.json，但其中没有任何编译命令。");
    }
    if (statusCode == QStringLiteral("system_headers_unresolved")) {
        return QStringLiteral("已找到 compile_commands.json，但当前编译命令无法解析系统头或标准库。请确认它由真实工具链生成。");
    }
    if (statusCode == QStringLiteral("translation_unit_parse_failed")) {
        return QStringLiteral("已找到 compile_commands.json，但没有任何项目翻译单元成功解析。请检查编译命令、工作目录和源码是否匹配。");
    }

    const QString semanticStatusMessage =
        QString::fromStdString(report.semanticStatusMessage).trimmed();
    return semanticStatusMessage.isEmpty()
               ? QStringLiteral("未能进入语义分析。请检查编译数据库、LLVM/Clang 和当前分析环境。")
               : semanticStatusMessage;
}

}  // namespace

QVariantList ReportService::buildSystemContextCards(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph) {
    const auto contextSignals = summarizeSystemContext(graph);
    const QString inputSummary = contextSignals.hasNetwork
                                     ? QStringLiteral("主要通过网络 API、Socket 或外部配置文件接收输入。")
                                     : QStringLiteral("主要通过本地系统接口、用户操作或本地文件加载接收输入。");
    const QString outputSummary = contextSignals.hasUI
                                      ? QStringLiteral("通过图形用户界面 (GUI) 向用户进行可视化呈现与交互。")
                                      : QStringLiteral("主要将处理结果输出到控制台、日志或本地存储中。");

    QVariantList cards;
    cards.push_back(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("项目形态")},
        {QStringLiteral("summary"),
         QStringLiteral("当前项目已被引擎解析为 %1 个核心模块，整体更像%2。")
             .arg(contextSignals.coreModuleCount)
             .arg(inferProjectKindSummary(report, overview, graph))}});
    cards.push_back(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("输入 / 输出")},
        {QStringLiteral("summary"), QStringLiteral("%1 %2").arg(inputSummary, outputSummary)}});
    cards.push_back(QVariantMap{
        {QStringLiteral("name"), QStringLiteral("建议先看")},
        {QStringLiteral("summary"),
         contextSignals.topModules.isEmpty()
             ? inferEntrySummary(overview, graph)
             : QStringLiteral("建议优先查看“%1”。")
                   .arg(contextSignals.topModules.join(QStringLiteral("”、“")))}}); 
    return cards;
}

QVariantMap ReportService::buildSystemContextData(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph,
    const QString& projectRootPath) {
    Q_UNUSED(report);
    const QString projectName = projectNameFromRootPath(projectRootPath);
    const auto contextSignals = summarizeSystemContext(graph);
    const auto totals = summarizeOverviewFiles(overview);

    QStringList technologyClues;
    technologyClues.push_back(QStringLiteral("底层采用 C++ 编写"));
    if (contextSignals.hasUI || totals.qmlFiles > 0) {
        technologyClues.push_back(QStringLiteral("包含 Qt/QML 等前端界面层"));
    }
    if (contextSignals.hasDatabase) {
        technologyClues.push_back(QStringLiteral("涉及数据持久化或本地存储"));
    }
    if (contextSignals.hasNetwork) {
        technologyClues.push_back(QStringLiteral("包含网络通信或外部接口集成"));
    }
    if (totals.scriptFiles > 0) {
        technologyClues.push_back(QStringLiteral("带有脚本和辅助生成流程"));
    }
    const QString technologySummary = technologyClues.join(QStringLiteral("，")) + QStringLiteral("。");

    QVariantMap data;
    data.insert(QStringLiteral("projectName"), projectName);
    data.insert(QStringLiteral("title"), QStringLiteral("L1 系统上下文 (System Context)"));
    data.insert(
        QStringLiteral("headline"),
        QStringLiteral("当前项目已被引擎解析为 %1 个核心模块。").arg(contextSignals.coreModuleCount));
    data.insert(
        QStringLiteral("audienceSummary"),
        joinPreview(
            inferAudienceHints(graph), 3, QStringLiteral("开发者、维护者或需要快速理解系统的人")));
    data.insert(
        QStringLiteral("purposeSummary"),
        contextSignals.topModules.isEmpty()
            ? QStringLiteral("当前还在继续收敛项目的核心业务边界。")
            : QStringLiteral("最值得优先理解的模块包括：“%1”。")
                  .arg(contextSignals.topModules.join(QStringLiteral("”、“"))));
    data.insert(QStringLiteral("entrySummary"), inferEntrySummary(overview, graph));
    data.insert(
        QStringLiteral("inputSummary"),
        contextSignals.hasNetwork ? QStringLiteral("主要通过网络 API、Socket 或外部配置文件接收输入。")
                                  : QStringLiteral("主要通过本地系统接口、用户操作或本地文件加载接收输入。"));
    data.insert(
        QStringLiteral("outputSummary"),
        contextSignals.hasUI ? QStringLiteral("通过图形用户界面 (GUI) 向用户进行可视化呈现与交互。")
                             : QStringLiteral("主要将处理结果输出到控制台、日志或本地存储中。"));
    data.insert(QStringLiteral("technologySummary"), technologySummary);
    data.insert(
        QStringLiteral("containerSummary"),
        contextSignals.topModules.isEmpty()
            ? QStringLiteral("当前还没有提炼出稳定的核心模块，建议先从入口和主流程开始查看。")
            : QStringLiteral("最核心的部分包括：“%1”等。建议优先查看这几块。")
                  .arg(contextSignals.topModules.join(QStringLiteral("”、“"))));
    data.insert(QStringLiteral("containerNames"), contextSignals.topModules);
    return data;
}

QString ReportService::buildStatusMessage(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& capabilityGraph,
    const layout::LayoutResult& layoutResult) {
    Q_UNUSED(layoutResult);
    const auto visibleNodeCount = static_cast<qulonglong>(std::count_if(
        capabilityGraph.nodes.begin(),
        capabilityGraph.nodes.end(),
        [](const core::CapabilityNode& node) { return node.defaultVisible; }));
    const auto visibleEdgeCount = static_cast<qulonglong>(std::count_if(
        capabilityGraph.edges.begin(),
        capabilityGraph.edges.end(),
        [](const core::CapabilityEdge& edge) { return edge.defaultVisible; }));
    const QString semanticExplanation = semanticFailureExplanation(report);
    if (report.semanticAnalysisRequested && !report.semanticAnalysisEnabled &&
        report.parsedFiles == 0) {
        return QStringLiteral("未能进入语义分析：%1").arg(semanticExplanation);
    }
    if (report.parsedFiles > 0) {
        QString message = QStringLiteral("已读取 %1 个源码文件，并整理出 %2 个默认可见模块、%3 条主要关系和 %4 个分组。")
                              .arg(static_cast<qulonglong>(report.parsedFiles))
                              .arg(visibleNodeCount)
                              .arg(visibleEdgeCount)
                              .arg(static_cast<qulonglong>(capabilityGraph.groups.size()));
        if (report.semanticAnalysisRequested && !report.semanticAnalysisEnabled) {
            message += QStringLiteral(" 当前未进入语义后端，结果已回退到语法级分析。原因：%1")
                           .arg(semanticExplanation);
        } else if (report.semanticAnalysisEnabled) {
            message += QStringLiteral(" 当前已进入 Clang/LibTooling 语义后端。");
        }
        if (overview.nodes.empty()) {
            message += QStringLiteral(" 当前还没有提炼出稳定的高层模块，建议优先查看入口和主流程。");
        }
        return message;
    }
    return QStringLiteral("还没分析到源码文件。请确认项目目录正确，然后重新开始分析。");
}

}  // namespace savt::ui
