#pragma once

#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"

#include <cstddef>
#include <string>
#include <vector>

namespace savt::core {

enum class ComponentNodeKind {
    Entry,
    Component,
    Support
};

enum class ComponentEdgeKind {
    Activates,
    Coordinates,
    UsesSupport
};

enum class ComponentGroupKind {
    Stage
};

struct ComponentNode {
    std::size_t id = 0;
    std::size_t capabilityId = 0;
    ComponentNodeKind kind = ComponentNodeKind::Component;
    std::string name;
    std::string role;
    std::string responsibility;
    std::string summary;
    std::string stageName;
    std::string folderHint;
    std::vector<std::size_t> overviewNodeIds;
    std::vector<std::string> moduleNames;
    std::vector<std::string> exampleFiles;
    std::vector<std::string> topSymbols;
    std::vector<std::string> collaboratorNames;
    std::size_t fileCount = 0;
    std::size_t sourceFileCount = 0;
    std::size_t qmlFileCount = 0;
    std::size_t webFileCount = 0;
    std::size_t scriptFileCount = 0;
    std::size_t dataFileCount = 0;
    std::size_t stageIndex = 0;
    std::size_t incomingEdgeCount = 0;
    std::size_t outgoingEdgeCount = 0;
    std::size_t visualPriority = 0;
    bool reachableFromEntry = false;
    bool cycleParticipant = false;
    bool defaultPinned = false;
    bool defaultVisible = true;
    std::size_t stageGroupId = 0;
    CapabilityNode::EvidencePackage evidence;
};

struct ComponentEdge {
    std::size_t id = 0;
    std::size_t fromId = 0;
    std::size_t toId = 0;
    ComponentEdgeKind kind = ComponentEdgeKind::Coordinates;
    std::size_t weight = 1;
    std::string summary;
    bool defaultVisible = true;
    CapabilityNode::EvidencePackage evidence;
};

struct ComponentGroup {
    std::size_t id = 0;
    ComponentGroupKind kind = ComponentGroupKind::Stage;
    std::string name;
    std::string summary;
    std::vector<std::size_t> nodeIds;
    std::size_t stageIndex = 0;
    bool defaultExpanded = true;
    bool defaultVisible = true;
};

struct ComponentGraph {
    std::size_t capabilityId = 0;
    std::string capabilityName;
    std::string capabilitySummary;
    std::vector<ComponentNode> nodes;
    std::vector<ComponentEdge> edges;
    std::vector<ComponentGroup> groups;
    std::vector<std::string> diagnostics;
};

const char* toString(ComponentNodeKind kind);
const char* toString(ComponentEdgeKind kind);
const char* toString(ComponentGroupKind kind);

ComponentGraph buildComponentGraphForCapability(
    const ArchitectureOverview& overview,
    const CapabilityGraph& capabilityGraph,
    std::size_t capabilityId);
ComponentGraph buildComponentGraphForCapability(
    const AnalysisReport& report,
    const ArchitectureOverview& overview,
    const CapabilityGraph& capabilityGraph,
    std::size_t capabilityId);

}  // namespace savt::core
