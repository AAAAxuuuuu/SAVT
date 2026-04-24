#pragma once

#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/layout/LayeredGraphLayout.h"
#include "savt/ui/SceneMapper.h"

#include <QPromise>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace savt::ui {

struct PendingAnalysisResult {
    savt::core::AnalysisReport report;
    savt::core::ArchitectureOverview overview;
    savt::core::CapabilityGraph capabilityGraph;
    QString statusMessage;
    QString analysisReport;
    QString systemContextReport;
    QVariantList astFileItems;
    QString selectedAstFilePath;
    QString astPreviewTitle;
    QString astPreviewSummary;
    QString astPreviewText;
    savt::layout::CapabilitySceneLayoutResult capabilitySceneLayout;
    CapabilitySceneData capabilityScene;
    QVariantMap componentSceneCatalog;
    QVariantMap systemContextData;
    QVariantList systemContextCards;
    bool canceled = false;
};

class AnalysisOrchestrator {
public:
    static QString defaultProjectRootPath();
    static void run(
        QPromise<void>& promise,
        const QString& cleanedPath,
        savt::analyzer::AnalyzerPrecision precision,
        const std::shared_ptr<PendingAnalysisResult>& output);
};

}  // namespace savt::ui
