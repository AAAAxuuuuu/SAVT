#pragma once

#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"

#include <cstddef>
#include <string>
#include <vector>

namespace savt::core {

enum class CapabilityNodeKind {
    Entry,
    Capability,
    Infrastructure
};

enum class CapabilityEdgeKind {
    Activates,
    Enables,
    UsesInfrastructure
};

enum class CapabilityGroupKind {
    Lane,
    RoleCluster,
    CollapsedSupportCluster
};

struct CapabilityNode {
    std::size_t id = 0;
    CapabilityNodeKind kind = CapabilityNodeKind::Capability;
    std::string name;
    std::string dominantRole;
    std::string responsibility;
    std::string summary;
    std::string folderHint;
    std::vector<std::size_t> overviewNodeIds;
    std::vector<std::string> moduleNames;
    std::vector<std::string> topSymbols;
    std::vector<std::string> supportingRoles;
    std::vector<std::string> exampleFiles;
    std::vector<std::string> collaboratorNames;
    std::vector<std::string> technologyTags;
    std::vector<std::string> riskSignals;
    std::size_t moduleCount = 0;
    std::size_t fileCount = 0;
    std::size_t sourceFileCount = 0;
    std::size_t qmlFileCount = 0;
    std::size_t webFileCount = 0;
    std::size_t scriptFileCount = 0;
    std::size_t dataFileCount = 0;
    std::size_t stageStart = 0;
    std::size_t stageEnd = 0;
    bool reachableFromEntry = false;
    bool cycleParticipant = false;
    std::size_t incomingEdgeCount = 0;
    std::size_t outgoingEdgeCount = 0;
    std::size_t aggregatedDependencyWeight = 0;
    std::size_t flowParticipationCount = 0;
    std::size_t collaboratorCount = 0;
    std::size_t riskScore = 0;
    std::string riskLevel;
    std::size_t visualPriority = 0;
    bool defaultVisible = true;
    bool defaultPinned = false;
    bool defaultCollapsed = false;
    std::size_t laneGroupId = 0;
    std::size_t detailGroupId = 0;
    struct EvidencePackage {
        std::vector<std::string> facts;
        std::vector<std::string> rules;
        std::vector<std::string> conclusions;
        std::vector<std::string> sourceFiles;
        std::vector<std::string> symbols;
        std::vector<std::string> modules;
        std::vector<std::string> relationships;
        std::string confidenceLabel;
        std::string confidenceReason;
    } evidence;
};

struct CapabilityEdge {
    std::size_t id = 0;
    std::size_t fromId = 0;
    std::size_t toId = 0;
    CapabilityEdgeKind kind = CapabilityEdgeKind::Enables;
    std::size_t weight = 1;
    std::string summary;
    bool defaultVisible = true;
    CapabilityNode::EvidencePackage evidence;
};

struct CapabilityFlow {
    std::size_t id = 0;
    std::string name;
    std::string summary;
    std::vector<std::size_t> nodeIds;
    std::size_t totalWeight = 0;
    std::size_t stageSpan = 0;
};

struct CapabilityGroup {
    std::size_t id = 0;
    CapabilityGroupKind kind = CapabilityGroupKind::Lane;
    std::string name;
    std::string summary;
    std::vector<std::size_t> nodeIds;
    bool defaultExpanded = true;
    bool defaultVisible = true;
    std::size_t hiddenNodeCount = 0;
};

struct CapabilityGraph {
    std::vector<CapabilityNode> nodes;
    std::vector<CapabilityEdge> edges;
    std::vector<CapabilityFlow> flows;
    std::vector<CapabilityGroup> groups;
    std::vector<std::string> diagnostics;
};

const char* toString(CapabilityNodeKind kind);
const char* toString(CapabilityEdgeKind kind);
const char* toString(CapabilityGroupKind kind);

CapabilityGraph buildCapabilityGraph(const AnalysisReport& report, const ArchitectureOverview& overview);
std::string formatCapabilityGraph(
    const CapabilityGraph& graph,
    std::size_t maxNodes = 16,
    std::size_t maxEdges = 32,
    std::size_t maxDiagnostics = 20);

}  // namespace savt::core
