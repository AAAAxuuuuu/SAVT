#pragma once

#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/ArchitectureRuleEngine.h"
#include "savt/core/CapabilityGraph.h"

#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace savt::ui {

class ReadingGuideService {
public:
    static QVariantMap buildGuide(
        const savt::core::AnalysisReport& report,
        const savt::core::ArchitectureOverview& overview,
        const savt::core::CapabilityGraph& graph,
        const QString& projectRootPath);

    static QVariantMap buildGuide(
        const savt::core::AnalysisReport& report,
        const savt::core::ArchitectureOverview& overview,
        const savt::core::CapabilityGraph& graph,
        const savt::core::ArchitectureRuleReport& ruleReport,
        const QString& projectRootPath);

    static QVariantList buildGuideCards(
        const savt::core::AnalysisReport& report,
        const savt::core::ArchitectureOverview& overview,
        const savt::core::CapabilityGraph& graph);
};

}  // namespace savt::ui
