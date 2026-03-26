#pragma once

#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/layout/LayeredGraphLayout.h"

#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace savt::ui {

class ReportService {
public:
    static QString buildStatusMessage(
        const savt::core::AnalysisReport& report,
        const savt::core::ArchitectureOverview& overview,
        const savt::core::CapabilityGraph& capabilityGraph,
        const savt::layout::LayoutResult& layoutResult);

    static QVariantList buildSystemContextCards(
        const savt::core::AnalysisReport& report,
        const savt::core::ArchitectureOverview& overview,
        const savt::core::CapabilityGraph& graph);

    static QVariantMap buildSystemContextData(
        const savt::core::AnalysisReport& report,
        const savt::core::ArchitectureOverview& overview,
        const savt::core::CapabilityGraph& graph,
        const QString& projectRootPath);
};

}  // namespace savt::ui
