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

QString formatRatio(const std::size_t count, const std::size_t total) {
    const double ratio = total == 0 ? 0.0 : (100.0 * static_cast<double>(count) / static_cast<double>(total));
    return QStringLiteral("%1%").arg(QString::number(ratio, 'f', 1));
}

QString precisionModeLabel(const core::AnalysisReport& report) {
    const QString precisionLevel = QString::fromStdString(report.precisionLevel);
    if (precisionLevel == QStringLiteral("semantic")) {
        return QStringLiteral("语义分析");
    }
    if (precisionLevel == QStringLiteral("syntax_fallback")) {
        return QStringLiteral("语义失败后回退到语法分析");
    }
    if (precisionLevel == QStringLiteral("blocked_semantic_required")) {
        return QStringLiteral("语义阻断");
    }
    if (precisionLevel == QStringLiteral("semantic_startup_failed")) {
        return QStringLiteral("语义启动失败");
    }
    if (precisionLevel == QStringLiteral("syntax")) {
        return QStringLiteral("语法分析");
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
    output.reserve(4096);
    output += QStringLiteral("# 项目架构全景报告\n\n");
    output += QStringLiteral("> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。\n\n---\n\n");
    output += QStringLiteral("## 分析引擎状态\n\n");
    output += QStringLiteral("| 项目 | 值 |\n| --- | --- |\n");
    output += QStringLiteral("| 当前精度 | %1 |\n").arg(precisionModeLabel(analysisReport));
    output += QStringLiteral("| 主引擎 | %1 |\n")
                  .arg(QString::fromStdString(analysisReport.primaryEngine.empty() ? std::string("none") : analysisReport.primaryEngine));
    output += QStringLiteral("| 语义状态 | %1 |\n").arg(semanticBackendLabel(analysisReport));
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
    if (!analysisReport.diagnostics.empty()) {
        output += QStringLiteral("\n**诊断日志：**\n");
        const int limit = qMin(static_cast<int>(analysisReport.diagnostics.size()), 10);
        for (int index = 0; index < limit; ++index) {
            output += QStringLiteral("- %1\n")
                          .arg(QString::fromStdString(analysisReport.diagnostics[static_cast<std::size_t>(index)]));
        }
        if (static_cast<int>(analysisReport.diagnostics.size()) > limit) {
            output += QStringLiteral("- *...共 %1 条诊断*\n").arg(analysisReport.diagnostics.size());
        }
    }
    output += QStringLiteral("\n---\n\n");

    if (nodes.empty()) {
        output += QStringLiteral("## 暂无模块结果\n\n");
        output += QStringLiteral("当前还没有提炼出可显示的模块视图。请先检查上面的分析引擎状态、编译数据库和诊断日志。\n");
        return output;
    }

    output += QStringLiteral("## 项目概览\n\n");
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

    output += QStringLiteral("## 模块详情\n\n");
    for (const core::CapabilityNode* node : nodes) {
        const QString name = QString::fromStdString(node->name).trimmed();
        const QString role = formatNodeHeadingRole(*node);
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
        if (!collaboratorNames.isEmpty()) {
            output += QStringLiteral("- 协作模块：%1\n").arg(collaboratorNames.join(QStringLiteral("、")));
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
    QString report = QStringLiteral("# 系统上下文 (L1)\n\n");

    QStringList entries;
    QStringList capabilities;
    QStringList infrastructures;

    for (const core::CapabilityNode* node : nodes) {
        const QString name = QString::fromStdString(node->name).trimmed();
        const QString role = QString::fromStdString(node->dominantRole).trimmed();
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
