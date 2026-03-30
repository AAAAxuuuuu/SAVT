#pragma once

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
    QString statusMessage;
    QString analysisReport;
    QString systemContextReport;
    QVariantList astFileItems;
    QString selectedAstFilePath;
    QString astPreviewTitle;
    QString astPreviewSummary;
    QString astPreviewText;
    CapabilitySceneData capabilityScene;
    std::shared_ptr<const savt::core::CapabilityGraph> capabilityGraph;
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
        const std::shared_ptr<PendingAnalysisResult>& output);
};

}  // namespace savt::ui
