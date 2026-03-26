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

}  // namespace

QString formatCapabilityReportMarkdown(const core::CapabilityGraph& graph) {
    const std::vector<const core::CapabilityNode*> nodes = visibleNodesInMarkdownOrder(graph);
    if (nodes.empty()) {
        return QStringLiteral("# 暂无分析结果\n\n请选择项目目录并点击“开始分析”。");
    }

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

    QString report;
    report.reserve(4096);
    report += QStringLiteral("# 项目架构全景报告\n\n");
    report += QStringLiteral("> 由 SAVT 静态分析引擎自动生成，适合快速建立项目上下文。\n\n---\n\n");
    report += QStringLiteral("## 项目概览\n\n");
    report += QStringLiteral("| 指标 | 数值 |\n| --- | --- |\n");
    report += QStringLiteral("| 识别模块数 | **%1 个** |\n").arg(nodes.size());
    report += QStringLiteral("| 涉及源码文件 | **%1 个** |\n").arg(totalFiles);
    report += QStringLiteral("| 子模块总数 | **%1 个** |\n").arg(totalModules);
    if (!entryNames.isEmpty()) {
        report += QStringLiteral("| 发起入口 | %1 |\n").arg(entryNames.join(QStringLiteral("、")));
    }
    if (!infrastructureNames.isEmpty()) {
        report += QStringLiteral("| 后台支撑 | %1 |\n").arg(infrastructureNames.join(QStringLiteral("、")));
    }
    report += QStringLiteral("\n---\n\n");

    report += QStringLiteral("## 模块详情\n\n");
    for (const core::CapabilityNode* node : nodes) {
        const QString name = QString::fromStdString(node->name).trimmed();
        const QString role = formatNodeHeadingRole(*node);
        const QString responsibility = QString::fromStdString(node->responsibility).trimmed();
        const QString summary = QString::fromStdString(node->summary).trimmed();
        const QString folderHint = QString::fromStdString(node->folderHint).trimmed();
        const QStringList topSymbols = toQStringList(node->topSymbols);
        const QStringList exampleFiles = toQStringList(node->exampleFiles);
        const QStringList collaboratorNames = toQStringList(node->collaboratorNames);

        report += QStringLiteral("### %1 %2\n\n").arg(role, name.isEmpty() ? QStringLiteral("未命名模块") : name);
        if (!responsibility.isEmpty()) {
            report += QStringLiteral("**职责：** %1\n\n").arg(responsibility);
        }
        if (!summary.isEmpty() && summary != responsibility) {
            report += QStringLiteral("%1\n\n").arg(summary);
        }

        report += QStringLiteral("- 源文件：**%1 个**").arg(node->fileCount);
        if (!folderHint.isEmpty()) {
            report += QStringLiteral("（`%1`）").arg(folderHint);
        }
        report += QStringLiteral("\n");
        report += QStringLiteral("- 被依赖：**%1 次** / 依赖他人：**%2 次**\n")
                      .arg(node->incomingEdgeCount)
                      .arg(node->outgoingEdgeCount);
        if (!collaboratorNames.isEmpty()) {
            report += QStringLiteral("- 协作模块：%1\n").arg(collaboratorNames.join(QStringLiteral("、")));
        }

        if (!topSymbols.isEmpty()) {
            report += QStringLiteral("\n**核心类 / 符号：**\n");
            const int limit = qMin(topSymbols.size(), 8);
            for (int index = 0; index < limit; ++index) {
                report += QStringLiteral("- `%1`\n").arg(topSymbols[index]);
            }
            if (topSymbols.size() > limit) {
                report += QStringLiteral("- *...共 %1 个符号*\n").arg(topSymbols.size());
            }
        }

        if (!exampleFiles.isEmpty()) {
            report += QStringLiteral("\n**代表文件：**\n");
            const int limit = qMin(exampleFiles.size(), 5);
            for (int index = 0; index < limit; ++index) {
                report += QStringLiteral("- `%1`\n").arg(exampleFiles[index]);
            }
        }

        report += QStringLiteral("\n---\n\n");
    }

    return report;
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
