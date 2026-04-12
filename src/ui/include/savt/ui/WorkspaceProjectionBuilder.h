#pragma once

#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/core/ComponentGraph.h"
#include "savt/layout/LayeredGraphLayout.h"
#include "savt/ui/IncrementalAnalysisPipeline.h"
#include "savt/ui/SceneMapper.h"

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <cstddef>
#include <unordered_map>

namespace savt::ui {

struct WorkspaceProjection {
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
};

class WorkspaceProjectionBuilder {
public:
    static WorkspaceProjection build(
        const QString& projectRootPath,
        const AnalysisArtifacts& artifacts);
};

}  // namespace savt::ui
