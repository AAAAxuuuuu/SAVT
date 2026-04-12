#include "savt/ui/SemanticReadinessService.h"

#include <QDir>

#include <algorithm>

namespace savt::ui {

namespace {

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

QString semanticActionHint(const core::AnalysisReport& report) {
    const QString statusCode = QString::fromStdString(report.semanticStatusCode).trimmed();
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
    return QStringLiteral(
        "如果你需要更深的类型和调用关系，请准备 compile_commands.json，并在后续分析入口启用语义优先模式后重跑。");
}

qulonglong visibleNodeCount(const core::CapabilityGraph& capabilityGraph) {
    return static_cast<qulonglong>(std::count_if(
        capabilityGraph.nodes.begin(),
        capabilityGraph.nodes.end(),
        [](const core::CapabilityNode& node) { return node.defaultVisible; }));
}

qulonglong visibleEdgeCount(const core::CapabilityGraph& capabilityGraph) {
    return static_cast<qulonglong>(std::count_if(
        capabilityGraph.edges.begin(),
        capabilityGraph.edges.end(),
        [](const core::CapabilityEdge& edge) { return edge.defaultVisible; }));
}

qulonglong cacheHitCount(const core::AnalysisReport& report) {
    return static_cast<qulonglong>(std::count_if(
        report.diagnostics.begin(),
        report.diagnostics.end(),
        [](const std::string& diagnostic) {
            return diagnostic.starts_with("Incremental cache") &&
                   diagnostic.find(" hit:") != std::string::npos;
        }));
}

}  // namespace

QVariantMap SemanticReadinessService::buildStatus(
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& capabilityGraph) {
    const qulonglong visibleNodes = visibleNodeCount(capabilityGraph);
    const qulonglong visibleEdges = visibleEdgeCount(capabilityGraph);
    const qulonglong cacheHits = cacheHitCount(report);

    QString modeKey;
    QString modeLabel;
    QString badgeTone;
    QString headline;
    QString reason;
    QString impact;
    QString action;
    QString confidenceSummary;
    bool needsAttention = false;

    if (report.semanticAnalysisEnabled) {
        modeKey = QStringLiteral("semantic-ready");
        modeLabel = QStringLiteral("语义就绪");
        badgeTone = QStringLiteral("success");
        headline = QStringLiteral("当前为语义级分析");
        reason = QStringLiteral("已成功进入 Clang/LibTooling 语义后端。");
        impact = QStringLiteral("调用关系、类型信息和跨文件符号证据更完整，适合继续做结构判断。");
        action = QStringLiteral("可以继续依据能力地图和组件工作台下钻到更具体的代码入口。");
        confidenceSummary = QStringLiteral("当前结果更适合支撑结构判断与后续改造定位。");
    } else if (report.semanticAnalysisRequested) {
        modeKey = QStringLiteral("semantic-preferred");
        modeLabel = QStringLiteral("语义优先");
        badgeTone = QStringLiteral("warning");
        headline = QStringLiteral("当前为语义优先分析（未就绪）");
        reason = semanticFailureExplanation(report);
        impact = QStringLiteral("当前结果已回退到语法级，跨文件调用、类型归属和部分关系推断可能不完整。");
        action = semanticActionHint(report);
        confidenceSummary = QStringLiteral("当前结果适合建立阅读路径，但还不适合把关系强度当成最终结论。");
        needsAttention = true;
    } else {
        modeKey = QStringLiteral("syntax-only");
        modeLabel = QStringLiteral("语法级首扫");
        badgeTone = QStringLiteral("info");
        headline = QStringLiteral("当前为语法级分析");
        reason = QStringLiteral("当前首页首次分析固定走快速首扫模式，尚未请求语义后端。");
        impact = QStringLiteral("当前结果更适合回答项目是什么、入口在哪、主模块有哪些，不适合过早下结论到类型与调用细节。");
        action = QStringLiteral("如果你需要更深的类型和调用关系，请准备 compile_commands.json，并在后续分析入口启用语义优先模式后重跑。");
        confidenceSummary = QStringLiteral("当前结果适合做新人导览和阅读路径，不适合直接当成最终精度结论。");
    }

    QString summary;
    if (report.parsedFiles > 0) {
        summary = QStringLiteral("已读取 %1 个源码文件，整理出 %2 个默认可见模块和 %3 条主要关系。")
                      .arg(static_cast<qulonglong>(report.parsedFiles))
                      .arg(visibleNodes)
                      .arg(visibleEdges);
        if (report.semanticAnalysisEnabled) {
            summary += QStringLiteral(" 当前已进入 Clang/LibTooling 语义后端。");
        } else if (report.semanticAnalysisRequested) {
            summary += QStringLiteral(" 当前未进入语义后端，结果回退到语法级分析。");
        } else {
            summary += QStringLiteral(" 当前仍处于语法级快速首扫。");
        }
        if (overview.nodes.empty()) {
            summary += QStringLiteral(" 当前还没有提炼出稳定的高层模块。");
        }
        if (cacheHits > 0) {
            summary += QStringLiteral(" 本次命中 %1 层增量缓存。").arg(cacheHits);
        }
    } else {
        summary = QStringLiteral("还没有分析到源码文件。请确认项目目录正确，然后重新开始分析。");
        needsAttention = true;
    }

    QVariantMap status;
    status.insert(QStringLiteral("modeKey"), modeKey);
    status.insert(QStringLiteral("modeLabel"), modeLabel);
    status.insert(QStringLiteral("badgeTone"), badgeTone);
    status.insert(QStringLiteral("headline"), headline);
    status.insert(QStringLiteral("summary"), summary);
    status.insert(QStringLiteral("reason"), reason);
    status.insert(QStringLiteral("impact"), impact);
    status.insert(QStringLiteral("action"), action);
    status.insert(QStringLiteral("confidenceSummary"), confidenceSummary);
    status.insert(QStringLiteral("statusCode"), QString::fromStdString(report.semanticStatusCode));
    status.insert(
        QStringLiteral("compilationDatabasePath"),
        report.compilationDatabasePath.empty()
            ? QString()
            : QDir::toNativeSeparators(
                  QString::fromStdString(report.compilationDatabasePath)));
    status.insert(QStringLiteral("semanticRequested"), report.semanticAnalysisRequested);
    status.insert(QStringLiteral("semanticEnabled"), report.semanticAnalysisEnabled);
    status.insert(QStringLiteral("needsAttention"), needsAttention);
    return status;
}

QString SemanticReadinessService::buildStatusMessage(const QVariantMap& status) {
    const QString summary = status.value(QStringLiteral("summary")).toString().trimmed();
    if (!summary.isEmpty()) {
        return summary;
    }
    return status.value(QStringLiteral("headline")).toString().trimmed();
}

}  // namespace savt::ui
