#pragma once

#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"

#include <QString>

namespace savt::ui {

QString formatCapabilityReportMarkdown(
    const savt::core::AnalysisReport& report,
    const savt::core::CapabilityGraph& graph);
QString formatSystemContextReportMarkdown(const savt::core::CapabilityGraph& graph);
QString formatPrecisionSummary(
    const savt::core::AnalysisReport& report,
    const savt::core::ArchitectureOverview& overview,
    const savt::core::CapabilityGraph& graph);

}  // namespace savt::ui
