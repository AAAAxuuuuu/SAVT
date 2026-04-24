#include "savt/ui/AnalysisOrchestrator.h"

#include "savt/ui/AstPreviewService.h"
#include "savt/ui/IncrementalAnalysisPipeline.h"
#include "savt/ui/WorkspaceProjectionBuilder.h"

#include <QDir>
#include <QFileInfo>

#include <filesystem>
#include <stdexcept>
#include <utility>

namespace savt::ui {

namespace {

void clearPendingPresentation(PendingAnalysisResult& result) {
    result.astFileItems.clear();
    result.selectedAstFilePath.clear();

    const auto preview = AstPreviewService::buildEmptyPreview();
    result.astPreviewTitle = preview.title;
    result.astPreviewSummary = preview.summary;
    result.astPreviewText = preview.text;

    result.capabilitySceneLayout = {};
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
                               ? QStringLiteral("Analysis stopped. You can restart it at any time.")
                               : QStringLiteral("Analysis stopped during \"%1\". You can restart it at any time.")
                                     .arg(phaseLabel);
    result.analysisReport = QStringLiteral(
        "The current analysis was canceled before completion.\n\n"
        "To avoid presenting partial output as a final result, the UI does not keep half-finished artifacts from this run.");
    clearPendingPresentation(result);
    return true;
}

void populatePendingPresentation(
    const AnalysisArtifacts& artifacts,
    WorkspaceProjection projection,
    PendingAnalysisResult& result) {
    result.report = artifacts.report;
    result.overview = artifacts.overview;
    result.capabilityGraph = artifacts.capabilityGraph;
    result.statusMessage = std::move(projection.statusMessage);
    result.analysisReport = std::move(projection.analysisReport);
    result.systemContextReport = std::move(projection.systemContextReport);
    result.astFileItems = std::move(projection.astFileItems);
    result.selectedAstFilePath = std::move(projection.selectedAstFilePath);
    result.astPreviewTitle = std::move(projection.astPreviewTitle);
    result.astPreviewSummary = std::move(projection.astPreviewSummary);
    result.astPreviewText = std::move(projection.astPreviewText);
    result.capabilitySceneLayout = std::move(projection.capabilitySceneLayout);
    result.capabilityScene = std::move(projection.capabilityScene);
    result.componentSceneCatalog = std::move(projection.componentSceneCatalog);
    result.systemContextData = std::move(projection.systemContextData);
    result.systemContextCards = std::move(projection.systemContextCards);
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
    const analyzer::AnalyzerPrecision precision,
    const std::shared_ptr<PendingAnalysisResult>& output) {
    PendingAnalysisResult result;
    clearPendingPresentation(result);

    try {
        promise.setProgressRange(0, 100);
        promise.setProgressValueAndText(5, QStringLiteral("Scanning project files..."));
        if (shouldAbortAnalysis(promise, result, QStringLiteral("Scanning project files"))) {
            if (output) {
                *output = std::move(result);
            }
            return;
        }

        analyzer::AnalyzerOptions options;
        options.precision = precision;
        options.cancellationRequested = [&promise]() { return promise.isCanceled(); };

        const auto artifacts = IncrementalAnalysisPipeline::analyze(
            std::filesystem::path(cleanedPath.toStdString()),
            options,
            [&promise](const int progressValue, const std::string& label) {
                promise.setProgressValueAndText(progressValue, QString::fromStdString(label));
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

        promise.setProgressValueAndText(96, QStringLiteral("Organizing workspace guide..."));
        if (shouldAbortAnalysis(
                promise,
                result,
                QStringLiteral("Organizing workspace guide"))) {
            if (output) {
                *output = std::move(result);
            }
            return;
        }

        auto projection = WorkspaceProjectionBuilder::build(cleanedPath, artifacts);

        populatePendingPresentation(
            artifacts,
            std::move(projection),
            result);

        if (shouldAbortAnalysis(
                promise,
                result,
                QStringLiteral("Organizing analysis results"))) {
            if (output) {
                *output = std::move(result);
            }
            return;
        }

        promise.setProgressValueAndText(100, QStringLiteral("Analysis complete. Preparing UI..."));
    } catch (const std::exception& exception) {
        result.statusMessage = QStringLiteral("Analysis failed: %1")
                                   .arg(QString::fromUtf8(exception.what()));
        result.analysisReport = QStringLiteral(
                                    "The analysis failed with an exception.\n\n"
                                    "Error: %1\n\n"
                                    "Please check the project path, source files, and build environment.")
                                    .arg(QString::fromUtf8(exception.what()));
        clearPendingPresentation(result);
    } catch (...) {
        result.statusMessage = QStringLiteral("Analysis failed with an unknown exception.");
        result.analysisReport = QStringLiteral(
            "The analysis failed with an unknown exception.\n\n"
            "Please check the project path, source files, and build environment.");
        clearPendingPresentation(result);
    }

    if (output) {
        *output = std::move(result);
    }
}

}  // namespace savt::ui
