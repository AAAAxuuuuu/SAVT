#pragma once

#include "savt/core/ArchitectureRuleEngine.h"

namespace savt::core {

struct ArchitectureAggregation {
    ArchitectureOverview overview;
    CapabilityGraph capabilityGraph;
    ArchitectureRuleReport ruleReport;
};

ArchitectureAggregation buildArchitectureAggregation(const AnalysisReport& report);

}  // namespace savt::core
