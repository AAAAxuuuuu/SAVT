#pragma once

#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"

#include <cstddef>
#include <string>
#include <vector>

namespace savt::core {

enum class ArchitectureRuleKind {
    Cycle,
    Hub,
    Isolated
};

enum class ArchitectureRuleSeverity {
    Info,
    Warning,
    Critical
};

struct ArchitectureRuleFinding {
    ArchitectureRuleKind kind = ArchitectureRuleKind::Hub;
    ArchitectureRuleSeverity severity = ArchitectureRuleSeverity::Info;
    std::vector<std::size_t> nodeIds;
    std::vector<std::string> reasonCodes;
};

struct ArchitectureRuleReport {
    std::vector<ArchitectureRuleFinding> findings;
    std::vector<std::string> diagnostics;
};

const char* toString(ArchitectureRuleKind kind);
const char* toString(ArchitectureRuleSeverity severity);

ArchitectureRuleReport evaluateArchitectureRules(
    const CapabilityGraph& graph,
    const ArchitectureOverview& overview);

}  // namespace savt::core
