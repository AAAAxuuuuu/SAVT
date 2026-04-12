#pragma once

#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"

#include <QString>
#include <QVariantMap>

namespace savt::ui {

class SemanticReadinessService {
public:
    static QVariantMap buildStatus(
        const savt::core::AnalysisReport& report,
        const savt::core::ArchitectureOverview& overview,
        const savt::core::CapabilityGraph& capabilityGraph);

    static QString buildStatusMessage(const QVariantMap& status);
};

}  // namespace savt::ui
