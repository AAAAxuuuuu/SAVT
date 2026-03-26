#pragma once

#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/layout/LayeredGraphLayout.h"

#include <QPromise>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <functional>
#include <memory>

namespace savt::ui {

struct PendingAnalysisResult {
    QString statusMessage;
    QString analysisReport;
    QString systemContextReport;
    QVariantList astFileItems;
    QString selectedAstFilePath;
    QString astPreviewTitle;
    QString astPreviewSummary;
    QString astPreviewText;
    QVariantList nodeItems;
    QVariantList edgeItems;
    QVariantList groupItems;
    double sceneWidth = 0.0;
    double sceneHeight = 0.0;
    QVariantMap systemContextData;
    QVariantList systemContextCards;
    bool canceled = false;
};

class AnalysisOrchestrator {
public:
    using ResultAssembler = std::function<void(
        QPromise<void>& promise,
        const QString& projectRootPath,
        const savt::core::AnalysisReport& report,
        const savt::core::ArchitectureOverview& overview,
        const savt::core::CapabilityGraph& capabilityGraph,
        const savt::layout::CapabilitySceneLayoutResult& capabilitySceneLayout,
        const savt::layout::LayoutResult& layoutResult,
        PendingAnalysisResult& result)>;

    static QString defaultProjectRootPath();
    static void run(
        QPromise<void>& promise,
        const QString& cleanedPath,
        ResultAssembler assembler,
        const std::shared_ptr<PendingAnalysisResult>& output);
};

}  // namespace savt::ui
