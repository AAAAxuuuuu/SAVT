#include "savt/ui/AnalysisOrchestrator.h"

#include "savt/ui/AnalysisTextFormatter.h"
#include "savt/ui/AstPreviewService.h"
#include "savt/ui/IncrementalAnalysisPipeline.h"
#include "savt/ui/ReportService.h"
#include "savt/ui/SceneMapper.h"

#include <QDir>
#include <QFileInfo>

#include <filesystem>
#include <stdexcept>

namespace savt::ui {

namespace {

void clearPendingPresentation(PendingAnalysisResult& result) {
    result.astFileItems.clear();
    result.selectedAstFilePath.clear();
    result.astPreviewTitle = QStringLiteral("AST 预览");
    result.astPreviewSummary = QStringLiteral(
        "完成项目分析后，会在这里展示所选源码文件的 Tree-sitter AST。");
    result.astPreviewText = QStringLiteral(
        "当前还没有可预览的源码文件。\n\n先执行一次项目分析，然后从文件下拉框里选择想查看的 C++ 源文件。\n界面会展示该文件的语法树层级、节点范围和基础统计信息。");
    const auto preview = AstPreviewService::buildEmptyPreview();
    result.astPreviewTitle = preview.title;
    result.astPreviewSummary = preview.summary;
    result.astPreviewText = preview.text;
    result.capabilityScene = {};
    result.componentSceneCatalog.clear();
    result.systemContextData.clear();
    result.systemContextCards.clear();
}

bool shouldAbortAnalysis(
    QPromise<void>& promise,
    PendingAnalysisResult& result,
    const QString& phaseLabel = QString()) {
    if (!promise.isCanceled()) {
        return false;
    }

    result.canceled = true;
    result.statusMessage = phaseLabel.isEmpty()
                               ? QStringLiteral("已停止当前分析。你可以重新开始。")
                               : QStringLiteral("已在“%1”阶段停止当前分析。你可以重新开始。")
                                     .arg(phaseLabel);
    result.analysisReport = QStringLiteral(
        "本次分析已手动停止。\n\n为了避免把未完成的中间结果展示成最终结论，当前界面不会保留这次分析的半成品。\n你可以直接再次开始分析。");
    clearPendingPresentation(result);
    return true;
}

void populatePendingPresentation(
    QPromise<void>& promise,
    const QString& cleanedPath,
    const core::AnalysisReport& report,
    const core::ArchitectureOverview& overview,
    const core::CapabilityGraph& capabilityGraph,
    const layout::CapabilitySceneLayoutResult& capabilitySceneLayout,
    const QVariantMap& componentSceneCatalog,
    const layout::LayoutResult& layoutResult,
    PendingAnalysisResult& result) {
    promise.setProgressValueAndText(96, QStringLiteral("整理 AST 预览..."));
    if (promise.isCanceled()) {
        return;
    }

    result.astFileItems = AstPreviewService::buildAstFileItems(report);
    result.selectedAstFilePath =
        AstPreviewService::chooseDefaultAstFilePath(result.astFileItems);
    const auto astPreview =
        AstPreviewService::buildPreview(cleanedPath, result.selectedAstFilePath);
    result.astPreviewTitle = astPreview.title;
    result.astPreviewSummary = astPreview.summary;
    result.astPreviewText = astPreview.text;
    if (promise.isCanceled()) {
        return;
    }

    promise.setProgressValueAndText(98, QStringLiteral("整理可视化数据..."));
    const auto sceneData =
        SceneMapper::buildCapabilitySceneData(capabilityGraph, capabilitySceneLayout);
    if (promise.isCanceled()) {
        return;
    }

    result.statusMessage =
        ReportService::buildStatusMessage(report, overview, capabilityGraph, layoutResult);
    result.capabilityScene = sceneData;
    result.componentSceneCatalog = componentSceneCatalog;
    result.analysisReport = formatCapabilityReportMarkdown(report, capabilityGraph);
    result.systemContextReport = formatSystemContextReportMarkdown(capabilityGraph);
    result.systemContextData =
        ReportService::buildSystemContextData(report, overview, capabilityGraph, cleanedPath);
    result.systemContextCards =
        ReportService::buildSystemContextCards(report, overview, capabilityGraph);
}

}  // namespace

QString AnalysisOrchestrator::defaultProjectRootPath() {
    QDir candidate(QDir::cleanPath(QDir::currentPath()));
    for (int depth = 0; depth < 5; ++depth) {
        const QFileInfo cmakeFile(candidate.filePath(QStringLiteral("CMakeLists.txt")));
        const QFileInfo srcDirectory(candidate.filePath(QStringLiteral("src")));
        if (cmakeFile.exists() && cmakeFile.isFile() && srcDirectory.exists() &&
            srcDirectory.isDir()) {
            return candidate.absolutePath();
        }
        if (!candidate.cdUp()) {
            break;
        }
    }
    return QDir::cleanPath(QDir::currentPath());
}

void AnalysisOrchestrator::run(
    QPromise<void>& promise,
    const QString& cleanedPath,
    const std::shared_ptr<PendingAnalysisResult>& output) {
    PendingAnalysisResult result;
    clearPendingPresentation(result);

    try {
        promise.setProgressRange(0, 100);
        promise.setProgressValueAndText(5, QStringLiteral("扫描项目目录..."));
        if (shouldAbortAnalysis(promise, result, QStringLiteral("扫描项目目录"))) {
            if (output) {
                *output = std::move(result);
            }
            return;
        }

        analyzer::AnalyzerOptions options;
        options.precision = analyzer::AnalyzerPrecision::SemanticPreferred;
        options.cancellationRequested = [&promise]() { return promise.isCanceled(); };
        const auto artifacts = IncrementalAnalysisPipeline::analyze(
            std::filesystem::path(cleanedPath.toStdString()),
            options,
            [&promise](const int progressValue, const std::string& label) {
                promise.setProgressValueAndText(
                    progressValue,
                    QString::fromStdString(label));
            });
        if (artifacts.canceled ||
            shouldAbortAnalysis(
                promise,
                result,
                QString::fromStdString(artifacts.canceledPhase))) {
            if (output) {
                *output = std::move(result);
            }
            return;
        }

        QVariantMap componentSceneCatalog;
        for (const core::CapabilityNode& capabilityNode : artifacts.capabilityGraph.nodes) {
            const auto graphIt = artifacts.componentGraphs.find(capabilityNode.id);
            const auto layoutIt = artifacts.componentLayouts.find(capabilityNode.id);
            if (graphIt == artifacts.componentGraphs.end() ||
                layoutIt == artifacts.componentLayouts.end()) {
                continue;
            }
            const auto componentScene =
                SceneMapper::buildComponentSceneData(graphIt->second, layoutIt->second);
            componentSceneCatalog.insert(
                QString::number(static_cast<qulonglong>(capabilityNode.id)),
                SceneMapper::toVariantMap(componentScene));
        }
        if (shouldAbortAnalysis(promise, result, QStringLiteral("生成 L3 组件视图"))) {
            if (output) {
                *output = std::move(result);
            }
            return;
        }

        populatePendingPresentation(
            promise,
            cleanedPath,
            artifacts.report,
            artifacts.overview,
            artifacts.capabilityGraph,
            artifacts.capabilitySceneLayout,
            componentSceneCatalog,
            artifacts.moduleLayout,
            result);
        if (shouldAbortAnalysis(promise, result, QStringLiteral("整理分析结果"))) {
            if (output) {
                *output = std::move(result);
            }
            return;
        }

        promise.setProgressValueAndText(100, QStringLiteral("分析完成，正在准备界面..."));
    } catch (const std::exception& exception) {
        result.statusMessage =
            QStringLiteral("分析失败：%1").arg(QString::fromUtf8(exception.what()));
        result.analysisReport = QStringLiteral(
                                    "分析过程中发生异常。\n\n错误信息：%1\n\n请检查项目路径、源码文件和当前构建环境是否可用。")
                                    .arg(QString::fromUtf8(exception.what()));
        clearPendingPresentation(result);
    } catch (...) {
        result.statusMessage = QStringLiteral("分析失败：发生未知异常。");
        result.analysisReport = QStringLiteral(
            "分析过程中发生未知异常。\n\n请检查项目路径、源码文件和当前构建环境是否可用。");
        clearPendingPresentation(result);
    }

    if (output) {
        *output = std::move(result);
    }
}

}  // namespace savt::ui
