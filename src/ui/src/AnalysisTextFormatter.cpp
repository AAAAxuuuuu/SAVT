#include "savt/ui/AnalysisTextFormatter.h"

#include <QStringList>

#include <algorithm>
#include <array>
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

QString normalizeSingleLine(QString text) {
    text.replace(QStringLiteral("\r"), QString());
    text.replace(QStringLiteral("\n"), QStringLiteral(" "));
    return text.simplified();
}

int laneIndexForNode(const core::CapabilityNode& node) {
    switch (node.kind) {
    case core::CapabilityNodeKind::Entry:
        return 0;
    case core::CapabilityNodeKind::Capability:
        return 1;
    case core::CapabilityNodeKind::Infrastructure:
        return 2;
    }
    return 1;
}

std::vector<const core::CapabilityNode*> visibleNodesInSceneOrder(const core::CapabilityGraph& graph) {
    std::vector<const core::CapabilityNode*> nodes;
    for (const core::CapabilityNode& node : graph.nodes) {
        if (node.defaultVisible) {
            nodes.push_back(&node);
        }
    }

    std::sort(nodes.begin(), nodes.end(), [](const core::CapabilityNode* left, const core::CapabilityNode* right) {
        const int leftLane = laneIndexForNode(*left);
        const int rightLane = laneIndexForNode(*right);
        if (leftLane != rightLane) {
            return leftLane < rightLane;
        }
        if (left->defaultPinned != right->defaultPinned) {
            return left->defaultPinned;
        }
        if (left->visualPriority != right->visualPriority) {
            return left->visualPriority > right->visualPriority;
        }
        return left->name < right->name;
    });

    return nodes;
}

std::vector<const core::CapabilityNode*> visibleNodesInMarkdownOrder(const core::CapabilityGraph& graph) {
    std::vector<const core::CapabilityNode*> nodes = visibleNodesInSceneOrder(graph);
    std::stable_sort(nodes.begin(), nodes.end(), [](const core::CapabilityNode* left, const core::CapabilityNode* right) {
        return left->visualPriority > right->visualPriority;
    });
    return nodes;
}

QString formatNodeHeadingRole(const core::CapabilityNode& node) {
    const QString role = QString::fromStdString(node.dominantRole).trimmed();
    return role.isEmpty() ? QStringLiteral("module") : role;
}

QString formatNodeSummaryLine(const core::CapabilityNode& node) {
    const QString responsibility = QString::fromStdString(node.responsibility).trimmed();
    const QString summary = QString::fromStdString(node.summary).trimmed();
    if (!responsibility.isEmpty()) {
        return responsibility;
    }
    if (!summary.isEmpty()) {
        return summary;
    }
    return QStringLiteral("当前还没有提炼出稳定职责。");
}

QString capabilityKindLabel(const core::CapabilityNode& node) {
    switch (node.kind) {
    case core::CapabilityNodeKind::Entry:
        return QStringLiteral("入口");
    case core::CapabilityNodeKind::Infrastructure:
        return QStringLiteral("支撑");
    case core::CapabilityNodeKind::Capability:
        return QStringLiteral("能力");
    }
    return QStringLiteral("模块");
}

QString capabilityRoleLabel(const core::CapabilityNode& node) {
    const QString role = QString::fromStdString(node.dominantRole).trimmed().toLower();
    if (role == QStringLiteral("analysis")) {
        return QStringLiteral("分析层");
    }
    if (role == QStringLiteral("presentation") || role == QStringLiteral("experience")) {
        return QStringLiteral("界面层");
    }
    if (role == QStringLiteral("interaction")) {
        return QStringLiteral("交互层");
    }
    if (role == QStringLiteral("storage") || role == QStringLiteral("data")) {
        return QStringLiteral("数据层");
    }
    if (role == QStringLiteral("foundation")) {
        return QStringLiteral("基础层");
    }
    if (role == QStringLiteral("workflow")) {
        return QStringLiteral("流程层");
    }
    if (role == QStringLiteral("support")) {
        return QStringLiteral("支撑层");
    }
    return role.isEmpty() ? QStringLiteral("未分类") : QString::fromStdString(node.dominantRole);
}

QString edgeKindLabel(const core::CapabilityEdgeKind kind) {
    switch (kind) {
    case core::CapabilityEdgeKind::Activates:
        return QStringLiteral("触发");
    case core::CapabilityEdgeKind::Enables:
        return QStringLiteral("协作");
    case core::CapabilityEdgeKind::UsesInfrastructure:
        return QStringLiteral("依赖支撑");
    }
    return QStringLiteral("关系");
}

QString joinPreview(const QStringList& items, const int limit) {
    if (items.isEmpty()) {
        return QString();
    }
    const int count = qMin(items.size(), limit);
    QString preview = items.mid(0, count).join(QStringLiteral("、"));
    if (items.size() > count) {
        preview += QStringLiteral(" 等 %1 项").arg(items.size());
    }
    return preview;
}

QStringList uniqueNodeNames(
    const std::vector<const core::CapabilityNode*>& nodes,
    const core::CapabilityNodeKind kind) {
    QStringList names;
    for (const core::CapabilityNode* node : nodes) {
        if (node->kind != kind) {
            continue;
        }
        const QString name = QString::fromStdString(node->name).trimmed();
        if (!name.isEmpty() && !names.contains(name)) {
            names.push_back(name);
        }
    }
    return names;
}

QString capabilityReadingHint(const core::CapabilityNode& node) {
    const QStringList files = toQStringList(node.exampleFiles);
    const QStringList symbols = toQStringList(node.topSymbols);
    if (!files.isEmpty() && !symbols.isEmpty()) {
        return QStringLiteral("建议先看 `%1`，再顺着 `%2` 继续向下读。")
            .arg(files.front(), symbols.front());
    }
    if (!files.isEmpty()) {
        return QStringLiteral("建议先看 `%1`。").arg(files.front());
    }
    if (!symbols.isEmpty()) {
        return QStringLiteral("建议先从 `%1` 这个符号开始定位。").arg(symbols.front());
    }
    return QStringLiteral("建议先结合能力图和代表文件继续下钻。");
}

QStringList buildReadingOrderPreview(const std::vector<const core::CapabilityNode*>& nodes) {
    QStringList items;
    for (const core::CapabilityNode* node : nodes) {
        if (node == nullptr) {
            continue;
        }
        const QString name = QString::fromStdString(node->name).trimmed();
        if (!name.isEmpty() && !items.contains(name)) {
            items.push_back(name);
        }
        if (items.size() >= 3) {
            break;
        }
    }
    return items;
}

std::vector<const core::CapabilityEdge*> visibleEdgesInWeightOrder(const core::CapabilityGraph& graph) {
    std::vector<const core::CapabilityEdge*> edges;
    for (const core::CapabilityEdge& edge : graph.edges) {
        if (edge.defaultVisible) {
            edges.push_back(&edge);
        }
    }

    std::sort(edges.begin(), edges.end(), [](const core::CapabilityEdge* left, const core::CapabilityEdge* right) {
        if (left->weight != right->weight) {
            return left->weight > right->weight;
        }
        return left->id < right->id;
    });
    return edges;
}

const core::CapabilityNode* findCapabilityNodeById(
    const core::CapabilityGraph& graph,
    const std::size_t nodeId) {
    for (const core::CapabilityNode& node : graph.nodes) {
        if (node.id == nodeId) {
            return &node;
        }
    }
    return nullptr;
}

QString formatEdgeSummary(
    const core::CapabilityGraph& graph,
    const core::CapabilityEdge& edge) {
    const core::CapabilityNode* fromNode = findCapabilityNodeById(graph, edge.fromId);
    const core::CapabilityNode* toNode = findCapabilityNodeById(graph, edge.toId);
    const QString fromName = fromNode ? QString::fromStdString(fromNode->name).trimmed() : QStringLiteral("未命名模块");
    const QString toName = toNode ? QString::fromStdString(toNode->name).trimmed() : QStringLiteral("未命名模块");
    const QString summary = QString::fromStdString(edge.summary).trimmed();

    QString line = QStringLiteral("- **%1** %2 **%3**")
                       .arg(fromName, edgeKindLabel(edge.kind), toName);
    if (!summary.isEmpty()) {
        line += QStringLiteral("：%1").arg(summary);
    }
    return line;
}

QString formatFlowSummary(const core::CapabilityFlow& flow) {
    const QString name = QString::fromStdString(flow.name).trimmed();
    const QString summary = QString::fromStdString(flow.summary).trimmed();
    QString line = QStringLiteral("- **%1**").arg(name.isEmpty() ? QStringLiteral("未命名流程") : name);
    if (!summary.isEmpty()) {
        line += QStringLiteral("：%1").arg(summary);
    }
    line += QStringLiteral("（跨度 %1，权重 %2）").arg(flow.stageSpan).arg(flow.totalWeight);
    return line;
}

QString formatRatio(const std::size_t count, const std::size_t total) {
    const double ratio = total == 0 ? 0.0 : (100.0 * static_cast<double>(count) / static_cast<double>(total));
    return QStringLiteral("%1%").arg(QString::number(ratio, 'f', 1));
}

QString precisionModeLabel(const core::AnalysisReport& report) {
    const QString precisionLevel = QString::fromStdString(report.precisionLevel);
    if (precisionLevel == QStringLiteral("semantic")) {
        return QStringLiteral("精确推演");
    }
    if (precisionLevel == QStringLiteral("syntax_fallback")) {
        return QStringLiteral("精确推演回退到快速建模");
    }
    if (precisionLevel == QStringLiteral("blocked_semantic_required")) {
        return QStringLiteral("精确推演阻断");
    }
    if (precisionLevel == QStringLiteral("semantic_startup_failed")) {
        return QStringLiteral("精确推演启动失败");
    }
    if (precisionLevel == QStringLiteral("syntax")) {
        return QStringLiteral("快速建模");
    }
    return precisionLevel.isEmpty() ? QStringLiteral("未知") : precisionLevel;
}

QString semanticBackendLabel(const core::AnalysisReport& report) {
    if (!report.semanticAnalysisRequested) {
        return QStringLiteral("未请求");
    }
    if (report.semanticAnalysisEnabled) {
        return QStringLiteral("已进入 Clang/LibTooling");
    }

    const QString statusCode = QString::fromStdString(report.semanticStatusCode);
    if (statusCode == QStringLiteral("missing_compile_commands")) {
        return QStringLiteral("阻断：缺少 compile_commands.json");
    }
    if (statusCode == QStringLiteral("backend_unavailable")) {
        return QStringLiteral("阻断：当前构建未包含语义后端");
    }
    if (statusCode == QStringLiteral("llvm_not_found")) {
        return QStringLiteral("阻断：未找到 LLVM/Clang");
    }
    if (statusCode == QStringLiteral("llvm_headers_missing")) {
        return QStringLiteral("阻断：缺少 clang-c 开发头");
    }
    if (statusCode == QStringLiteral("libclang_not_found")) {
        return QStringLiteral("阻断：缺少 libclang");
    }
    if (statusCode == QStringLiteral("compilation_database_load_failed")) {
        return QStringLiteral("阻断：编译数据库不可加载");
    }
    if (statusCode == QStringLiteral("compilation_database_empty")) {
        return QStringLiteral("阻断：编译数据库为空");
    }
    if (statusCode == QStringLiteral("system_headers_unresolved")) {
        return QStringLiteral("阻断：系统头不可解析");
    }
    if (statusCode == QStringLiteral("translation_unit_parse_failed")) {
        return QStringLiteral("阻断：翻译单元解析失败");
    }
    return QStringLiteral("阻断：%1").arg(statusCode.isEmpty() ? QStringLiteral("未知原因") : statusCode);
}

QString semanticActionHint(const core::AnalysisReport& report) {
    const QString statusCode = QString::fromStdString(report.semanticStatusCode);
    if (statusCode == QStringLiteral("missing_compile_commands")) {
        return QStringLiteral("请先为被分析项目生成 compile_commands.json，并确保它位于项目根目录或 build 目录。");
    }
    if (statusCode == QStringLiteral("backend_unavailable")) {
        return QStringLiteral("当前 SAVT 构建没有包含 Clang/LibTooling 语义后端，请重新配置并启用 SAVT_ENABLE_CLANG_TOOLING。");
    }
    if (statusCode == QStringLiteral("llvm_not_found")) {
        return QStringLiteral("请安装带 clang-c/Index.h 和 libclang 的 LLVM/Clang 发行包，并正确设置 SAVT_LLVM_ROOT。");
    }
    if (statusCode == QStringLiteral("llvm_headers_missing")) {
        return QStringLiteral("当前 LLVM 目录缺少 clang-c/Index.h，请检查是否安装了开发头文件。");
    }
    if (statusCode == QStringLiteral("libclang_not_found")) {
        return QStringLiteral("当前 LLVM 目录缺少 libclang，请检查安装包是否完整，或改用包含 libclang 的发行版。");
    }
    if (statusCode == QStringLiteral("compilation_database_load_failed")) {
        return QStringLiteral("请确认 compile_commands.json 位于实际构建目录，且内容为有效 JSON。");
    }
    if (statusCode == QStringLiteral("compilation_database_empty")) {
        return QStringLiteral("请重新生成 compile_commands.json，确保其中包含至少一个项目翻译单元。");
    }
    if (statusCode == QStringLiteral("system_headers_unresolved")) {
        return QStringLiteral("请确认 compile_commands.json 由真实工具链生成，并保留 SDK/标准库的搜索路径。");
    }
    if (statusCode == QStringLiteral("translation_unit_parse_failed")) {
        return QStringLiteral("请检查 compile_commands.json 中的编译命令是否和当前源码、编译器、工作目录一致。");
    }
    return {};
}

}  // namespace

QString formatCapabilityReportMarkdown(
    const core::AnalysisReport& analysisReport,
    const core::CapabilityGraph& graph) {
    const std::vector<const core::CapabilityNode*> nodes = visibleNodesInMarkdownOrder(graph);
    const std::vector<const core::CapabilityEdge*> edges = visibleEdgesInWeightOrder(graph);

    std::size_t totalFiles = 0;
    std::size_t totalModules = 0;
    QStringList entryNames;
    QStringList infrastructureNames;
    for (const core::CapabilityNode* node : nodes) {
        totalFiles += node->fileCount;
        totalModules += node->moduleCount;
        const QString name = QString::fromStdString(node->name).trimmed();
        if (node->kind == core::CapabilityNodeKind::Entry && !name.isEmpty() && !entryNames.contains(name)) {
            entryNames.push_back(name);
        }
        if (node->kind == core::CapabilityNodeKind::Infrastructure && !name.isEmpty() &&
            !infrastructureNames.contains(name)) {
            infrastructureNames.push_back(name);
        }
    }

    QString output;
    output.reserve(8192);
    output += QStringLiteral("# 项目架构全景报告\n\n");
    output += QStringLiteral("> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。\n\n---\n\n");
    output += QStringLiteral("## 分析引擎状态\n\n");
    output += QStringLiteral("| 项目 | 值 |\n| --- | --- |\n");
    output += QStringLiteral("| 当前精度 | %1 |\n").arg(precisionModeLabel(analysisReport));
    output += QStringLiteral("| 主引擎 | %1 |\n")
                  .arg(QString::fromStdString(analysisReport.primaryEngine.empty() ? std::string("none") : analysisReport.primaryEngine));
    output += QStringLiteral("| 精确推演状态 | %1 |\n").arg(semanticBackendLabel(analysisReport));
    output += QStringLiteral("| 语义状态码 | `%1` |\n")
                  .arg(QString::fromStdString(analysisReport.semanticStatusCode.empty() ? std::string("none") : analysisReport.semanticStatusCode));
    output += QStringLiteral("| 编译数据库 | %1 |\n")
                  .arg(analysisReport.compilationDatabasePath.empty()
                           ? QStringLiteral("(none)")
                           : QString::fromStdString(analysisReport.compilationDatabasePath));
    output += QStringLiteral("| 已发现源码文件 | %1 |\n").arg(analysisReport.discoveredFiles);
    output += QStringLiteral("| 已成功解析文件 | %1 |\n").arg(analysisReport.parsedFiles);

    const QString semanticMessage = normalizeSingleLine(QString::fromStdString(analysisReport.semanticStatusMessage));
    if (!semanticMessage.isEmpty()) {
        output += QStringLiteral("\n**状态说明：** %1\n").arg(semanticMessage);
    }
    if (const QString actionHint = semanticActionHint(analysisReport); !actionHint.isEmpty()) {
        output += QStringLiteral("\n**处理建议：** %1\n").arg(actionHint);
    }
    output += QStringLiteral("\n---\n\n");

    if (nodes.empty()) {
        output += QStringLiteral("## 暂无模块结果\n\n");
        output += QStringLiteral("当前还没有提炼出可显示的模块视图。请先检查上面的分析引擎状态和编译数据库。\n");
        return output;
    }

    const QStringList readingPreview = buildReadingOrderPreview(nodes);

    output += QStringLiteral("## 项目概览\n\n");
    output += QStringLiteral("当前共识别出 **%1 个可见能力模块**，覆盖 **%2 个源码文件**。")
                  .arg(nodes.size())
                  .arg(totalFiles);
    if (!readingPreview.isEmpty()) {
        output += QStringLiteral(" 建议先从 **%1** 开始，顺着主路径逐步下钻。")
                      .arg(readingPreview.join(QStringLiteral(" → ")));
    }
    output += QStringLiteral("\n\n");
    output += QStringLiteral("| 指标 | 数值 |\n| --- | --- |\n");
    output += QStringLiteral("| 识别模块数 | **%1 个** |\n").arg(nodes.size());
    output += QStringLiteral("| 涉及源码文件 | **%1 个** |\n").arg(totalFiles);
    output += QStringLiteral("| 子模块总数 | **%1 个** |\n").arg(totalModules);
    if (!entryNames.isEmpty()) {
        output += QStringLiteral("| 发起入口 | %1 |\n").arg(entryNames.join(QStringLiteral("、")));
    }
    if (!infrastructureNames.isEmpty()) {
        output += QStringLiteral("| 后台支撑 | %1 |\n").arg(infrastructureNames.join(QStringLiteral("、")));
    }
    output += QStringLiteral("\n---\n\n");

    output += QStringLiteral("## 建议先读\n\n");
    const int readLimit = qMin(static_cast<int>(nodes.size()), 3);
    for (int index = 0; index < readLimit; ++index) {
        const core::CapabilityNode& node = *nodes[static_cast<std::size_t>(index)];
        const QString name = QString::fromStdString(node.name).trimmed();
        output += QStringLiteral("%1. **%2**（%3 / %4）\n")
                      .arg(index + 1)
                      .arg(name.isEmpty() ? QStringLiteral("未命名模块") : name,
                           capabilityKindLabel(node),
                           capabilityRoleLabel(node));
        output += QStringLiteral("   - %1\n").arg(formatNodeSummaryLine(node));
        output += QStringLiteral("   - %1\n").arg(capabilityReadingHint(node));
    }
    output += QStringLiteral("\n---\n\n");

    if (!edges.empty()) {
        output += QStringLiteral("## 关键协作关系\n\n");
        const int edgeLimit = qMin(static_cast<int>(edges.size()), 8);
        for (int index = 0; index < edgeLimit; ++index) {
            output += formatEdgeSummary(graph, *edges[static_cast<std::size_t>(index)]) + QStringLiteral("\n");
        }
        output += QStringLiteral("\n---\n\n");
    }

    if (!graph.flows.empty()) {
        output += QStringLiteral("## 主要协作路径\n\n");
        const int flowLimit = qMin(static_cast<int>(graph.flows.size()), 4);
        for (int index = 0; index < flowLimit; ++index) {
            output += formatFlowSummary(graph.flows[static_cast<std::size_t>(index)]) + QStringLiteral("\n");
        }
        output += QStringLiteral("\n---\n\n");
    }

    output += QStringLiteral("## 模块详情\n\n");
    for (const core::CapabilityNode* node : nodes) {
        const QString name = QString::fromStdString(node->name).trimmed();
        const QString role = capabilityRoleLabel(*node);
        const QString responsibility = QString::fromStdString(node->responsibility).trimmed();
        const QString summary = QString::fromStdString(node->summary).trimmed();
        const QString folderHint = QString::fromStdString(node->folderHint).trimmed();
        const QStringList topSymbols = toQStringList(node->topSymbols);
        const QStringList exampleFiles = toQStringList(node->exampleFiles);
        const QStringList collaboratorNames = toQStringList(node->collaboratorNames);

        output += QStringLiteral("### %1 %2\n\n").arg(role, name.isEmpty() ? QStringLiteral("未命名模块") : name);
        if (!responsibility.isEmpty()) {
            output += QStringLiteral("**职责：** %1\n\n").arg(responsibility);
        }
        if (!summary.isEmpty() && summary != responsibility) {
            output += QStringLiteral("%1\n\n").arg(summary);
        }

        output += QStringLiteral("- 源文件：**%1 个**").arg(node->fileCount);
        if (!folderHint.isEmpty()) {
            output += QStringLiteral("（`%1`）").arg(folderHint);
        }
        output += QStringLiteral("\n");
        output += QStringLiteral("- 被依赖：**%1 次** / 依赖他人：**%2 次**\n")
                      .arg(node->incomingEdgeCount)
                      .arg(node->outgoingEdgeCount);
        output += QStringLiteral("- 类型：**%1** / 角色：**%2**\n")
                      .arg(capabilityKindLabel(*node), capabilityRoleLabel(*node));
        if (!collaboratorNames.isEmpty()) {
            output += QStringLiteral("- 协作模块：%1\n").arg(collaboratorNames.join(QStringLiteral("、")));
        }
        if (!node->riskSignals.empty()) {
            output += QStringLiteral("- 风险提示：%1\n")
                          .arg(toQStringList(node->riskSignals).join(QStringLiteral("、")));
        }

        if (!topSymbols.isEmpty()) {
            output += QStringLiteral("\n**核心类 / 符号：**\n");
            const int limit = qMin(topSymbols.size(), 8);
            for (int index = 0; index < limit; ++index) {
                output += QStringLiteral("- `%1`\n").arg(topSymbols[index]);
            }
            if (topSymbols.size() > limit) {
                output += QStringLiteral("- *...共 %1 个符号*\n").arg(topSymbols.size());
            }
        }

        if (!exampleFiles.isEmpty()) {
            output += QStringLiteral("\n**代表文件：**\n");
            const int limit = qMin(exampleFiles.size(), 5);
            for (int index = 0; index < limit; ++index) {
                output += QStringLiteral("- `%1`\n").arg(exampleFiles[index]);
            }
        }

        output += QStringLiteral("\n---\n\n");
    }

    return output;
}

QString formatSystemContextReportMarkdown(const core::CapabilityGraph& graph) {
    const std::vector<const core::CapabilityNode*> nodes = visibleNodesInSceneOrder(graph);
    const std::vector<const core::CapabilityEdge*> edges = visibleEdgesInWeightOrder(graph);
    QString report = QStringLiteral("# 系统上下文 (L1)\n\n");

    QStringList entries;
    QStringList capabilities;
    QStringList infrastructures;
    QStringList entryNames = uniqueNodeNames(nodes, core::CapabilityNodeKind::Entry);
    QStringList capabilityNames = uniqueNodeNames(nodes, core::CapabilityNodeKind::Capability);
    QStringList infrastructureNames = uniqueNodeNames(nodes, core::CapabilityNodeKind::Infrastructure);

    report += QStringLiteral("当前系统被提炼为 **%1 个默认可见模块**。").arg(nodes.size());
    if (!entryNames.isEmpty()) {
        report += QStringLiteral(" 入口主要落在 **%1**。").arg(joinPreview(entryNames, 2));
    }
    if (!capabilityNames.isEmpty()) {
        report += QStringLiteral(" 主要能力集中在 **%1**。").arg(joinPreview(capabilityNames, 3));
    }
    if (!infrastructureNames.isEmpty()) {
        report += QStringLiteral(" 后台支撑主要包括 **%1**。").arg(joinPreview(infrastructureNames, 2));
    }
    report += QStringLiteral("\n\n");

    if (!entryNames.isEmpty() || !capabilityNames.isEmpty()) {
        report += QStringLiteral("## 建议阅读顺序\n");
        int step = 1;
        if (!entryNames.isEmpty()) {
            report += QStringLiteral("%1. 先从入口模块 **%2** 开始，确认系统是从哪里发起的。\n")
                          .arg(step++)
                          .arg(joinPreview(entryNames, 2));
        }
        if (!capabilityNames.isEmpty()) {
            report += QStringLiteral("%1. 再进入核心能力 **%2**，把主路径串起来。\n")
                          .arg(step++)
                          .arg(joinPreview(capabilityNames, 3));
        }
        if (!infrastructureNames.isEmpty()) {
            report += QStringLiteral("%1. 最后回看支撑模块 **%2**，确认共享依赖和基础设施边界。\n")
                          .arg(step)
                          .arg(joinPreview(infrastructureNames, 2));
        }
        report += QStringLiteral("\n");
    }

    for (const core::CapabilityNode* node : nodes) {
        const QString name = QString::fromStdString(node->name).trimmed();
        const QString role = capabilityRoleLabel(*node);
        const QString summary = formatNodeSummaryLine(*node);

        QString line = QStringLiteral("- **%1**").arg(name.isEmpty() ? QStringLiteral("未命名模块") : name);
        if (!role.isEmpty()) {
            line += QStringLiteral(" (%1)").arg(role);
        }
        if (!summary.isEmpty()) {
            line += QStringLiteral("：%1").arg(summary);
        }
        line += QStringLiteral("\n");

        switch (node->kind) {
        case core::CapabilityNodeKind::Entry:
            entries.push_back(line);
            break;
        case core::CapabilityNodeKind::Capability:
            capabilities.push_back(line);
            break;
        case core::CapabilityNodeKind::Infrastructure:
            infrastructures.push_back(line);
            break;
        }
    }

    if (!edges.empty()) {
        report += QStringLiteral("## 主要模块关系\n");
        const int edgeLimit = qMin(static_cast<int>(edges.size()), 6);
        for (int index = 0; index < edgeLimit; ++index) {
            report += formatEdgeSummary(graph, *edges[static_cast<std::size_t>(index)]) + QStringLiteral("\n");
        }
        report += QStringLiteral("\n");
    }

    if (!entries.isEmpty()) {
        report += QStringLiteral("## 发起入口\n");
        report += entries.join(QString());
        report += QStringLiteral("\n");
    }
    if (!capabilities.isEmpty()) {
        report += QStringLiteral("## 核心能力\n");
        report += capabilities.join(QString());
        report += QStringLiteral("\n");
    }
    if (!infrastructures.isEmpty()) {
        report += QStringLiteral("## 后台支撑\n");
        report += infrastructures.join(QString());
        report += QStringLiteral("\n");
    }

    return report;
}

QString formatPrecisionSummary(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& graph) {
    std::size_t semanticNodeCount = 0;
    std::size_t inferredNodeCount = 0;
    for (const core::SymbolNode& node : report.nodes) {
        if (node.factSource == core::FactSource::Semantic) {
            ++semanticNodeCount;
        } else {
            ++inferredNodeCount;
        }
    }

    std::size_t semanticEdgeCount = 0;
    std::size_t inferredEdgeCount = 0;
    std::size_t totalSupportCount = 0;
    std::array<std::size_t, 7> edgeKindCounts = {};
    for (const core::SymbolEdge& edge : report.edges) {
        if (edge.factSource == core::FactSource::Semantic) {
            ++semanticEdgeCount;
        } else {
            ++inferredEdgeCount;
        }
        totalSupportCount += edge.supportCount;
        edgeKindCounts[static_cast<std::size_t>(edge.kind)] += 1;
    }

    const std::size_t visibleCapabilityNodes = static_cast<std::size_t>(std::count_if(
        graph.nodes.begin(), graph.nodes.end(), [](const core::CapabilityNode& node) { return node.defaultVisible; }));
    const std::size_t visibleCapabilityEdges = static_cast<std::size_t>(std::count_if(
        graph.edges.begin(), graph.edges.end(), [](const core::CapabilityEdge& edge) { return edge.defaultVisible; }));

    QString output;
    output += QStringLiteral("[precision summary]\n");
    output += QStringLiteral("primary engine: %1\n").arg(QString::fromStdString(report.primaryEngine));
    output += QStringLiteral("precision level: %1\n").arg(QString::fromStdString(report.precisionLevel));
    output += QStringLiteral("semantic requested: %1\n").arg(report.semanticAnalysisRequested ? QStringLiteral("true") : QStringLiteral("false"));
    output += QStringLiteral("semantic enabled: %1\n").arg(report.semanticAnalysisEnabled ? QStringLiteral("true") : QStringLiteral("false"));
    output += QStringLiteral("semantic status code: %1\n").arg(QString::fromStdString(report.semanticStatusCode));
    output += QStringLiteral("semantic status message: %1\n")
                  .arg(normalizeSingleLine(QString::fromStdString(report.semanticStatusMessage)));
    output += QStringLiteral("compilation database: %1\n")
                  .arg(report.compilationDatabasePath.empty()
                           ? QStringLiteral("(none)")
                           : QString::fromStdString(report.compilationDatabasePath));
    output += QStringLiteral("discovered files: %1\n").arg(report.discoveredFiles);
    output += QStringLiteral("parsed files: %1\n").arg(report.parsedFiles);
    output += QStringLiteral("raw nodes: %1\n").arg(report.nodes.size());
    output += QStringLiteral("raw edges: %1\n").arg(report.edges.size());
    output += QStringLiteral("edge support total: %1\n").arg(totalSupportCount);
    output += QStringLiteral("semantic nodes: %1 (%2)\n").arg(semanticNodeCount).arg(formatRatio(semanticNodeCount, report.nodes.size()));
    output += QStringLiteral("inferred nodes: %1 (%2)\n").arg(inferredNodeCount).arg(formatRatio(inferredNodeCount, report.nodes.size()));
    output += QStringLiteral("semantic edges: %1 (%2)\n").arg(semanticEdgeCount).arg(formatRatio(semanticEdgeCount, report.edges.size()));
    output += QStringLiteral("inferred edges: %1 (%2)\n").arg(inferredEdgeCount).arg(formatRatio(inferredEdgeCount, report.edges.size()));
    output += QStringLiteral("\n[edge kinds]\n");
    output += QStringLiteral("contains: %1\n").arg(edgeKindCounts[static_cast<std::size_t>(core::EdgeKind::Contains)]);
    output += QStringLiteral("calls: %1\n").arg(edgeKindCounts[static_cast<std::size_t>(core::EdgeKind::Calls)]);
    output += QStringLiteral("includes: %1\n").arg(edgeKindCounts[static_cast<std::size_t>(core::EdgeKind::Includes)]);
    output += QStringLiteral("inherits: %1\n").arg(edgeKindCounts[static_cast<std::size_t>(core::EdgeKind::Inherits)]);
    output += QStringLiteral("uses_type: %1\n").arg(edgeKindCounts[static_cast<std::size_t>(core::EdgeKind::UsesType)]);
    output += QStringLiteral("overrides: %1\n").arg(edgeKindCounts[static_cast<std::size_t>(core::EdgeKind::Overrides)]);
    output += QStringLiteral("depends_on: %1\n").arg(edgeKindCounts[static_cast<std::size_t>(core::EdgeKind::DependsOn)]);
    output += QStringLiteral("\n[derived views]\n");
    output += QStringLiteral("overview nodes: %1\n").arg(overview.nodes.size());
    output += QStringLiteral("overview edges: %1\n").arg(overview.edges.size());
    output += QStringLiteral("overview flows: %1\n").arg(overview.flows.size());
    output += QStringLiteral("capability nodes: %1\n").arg(graph.nodes.size());
    output += QStringLiteral("capability edges: %1\n").arg(graph.edges.size());
    output += QStringLiteral("capability flows: %1\n").arg(graph.flows.size());
    output += QStringLiteral("visible capability nodes: %1\n").arg(visibleCapabilityNodes);
    output += QStringLiteral("visible capability edges: %1\n").arg(visibleCapabilityEdges);
    return output;
}

}  // namespace savt::ui
