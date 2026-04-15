#pragma once

#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"

#include <QString>
#include <QVariantMap>

namespace savt::ui {

class ProjectConfigRecommendationService {
public:
    static QVariantMap buildRecommendation(
        const QString& projectRootPath,
        const savt::core::AnalysisReport& report,
        const savt::core::ArchitectureOverview& overview,
        const savt::core::CapabilityGraph& capabilityGraph);

    static QVariantMap selectChoice(
        const QVariantMap& recommendation,
        const QString& choiceId,
        const QString& optionId);

    static bool writeRecommendation(
        const QString& projectRootPath,
        const QVariantMap& recommendation,
        QString* writtenPath,
        QString* errorMessage);
};

}  // namespace savt::ui
