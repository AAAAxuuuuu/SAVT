#pragma once

#include "savt/core/ArchitectureGraph.h"

#include <cstddef>
#include <string>
#include <vector>

namespace savt::core {

enum class OverviewNodeKind {
    Module,
    EntryPointModule
};

enum class OverviewEdgeKind {
    DependsOn
};

enum class OverviewGroupKind {
    FolderCluster,
    RoleCluster,
    StronglyConnectedCluster
};

struct OverviewNode {
    std::size_t id = 0;
    OverviewNodeKind kind = OverviewNodeKind::Module;
    std::string name;
    std::string role;
    std::string summary;
    std::vector<std::string> topSymbols;
    std::vector<std::string> filePaths;
    std::vector<std::size_t> rawNodeIds;
    std::size_t fileCount = 0;
    std::size_t sourceFileCount = 0;
    std::size_t qmlFileCount = 0;
    std::size_t webFileCount = 0;
    std::size_t scriptFileCount = 0;
    std::size_t dataFileCount = 0;
    std::size_t classCount = 0;
    std::size_t structCount = 0;
    std::size_t enumCount = 0;
    std::size_t fieldCount = 0;
    std::size_t functionCount = 0;
    std::size_t methodCount = 0;
    std::size_t incomingDependencyCount = 0;
    std::size_t outgoingDependencyCount = 0;
    std::size_t stageIndex = 0;
    std::string stageName;
    std::string folderKey;
    bool reachableFromEntry = false;
    bool cycleParticipant = false;
    std::size_t folderGroupId = 0;
    std::size_t roleGroupId = 0;
    std::size_t cycleGroupId = 0;
};

struct OverviewEdge {
    std::size_t fromId = 0;
    std::size_t toId = 0;
    OverviewEdgeKind kind = OverviewEdgeKind::DependsOn;
    std::size_t weight = 1;
};

struct OverviewGroup {
    std::size_t id = 0;
    OverviewGroupKind kind = OverviewGroupKind::FolderCluster;
    std::string name;
    std::string summary;
    std::vector<std::size_t> nodeIds;
    std::size_t parentGroupId = 0;
    std::size_t hierarchyLevel = 0;
    bool defaultExpanded = false;
};

struct OverviewStage {
    std::size_t index = 0;
    std::string name;
    std::vector<std::size_t> nodeIds;
};

struct OverviewFlow {
    std::size_t id = 0;
    std::string name;
    std::string summary;
    std::vector<std::size_t> nodeIds;
    std::size_t totalWeight = 0;
    std::size_t stageSpan = 0;
};

struct ArchitectureOverview {
    std::vector<OverviewNode> nodes;
    std::vector<OverviewEdge> edges;
    std::vector<OverviewGroup> groups;
    std::vector<OverviewStage> stages;
    std::vector<OverviewFlow> flows;
    std::vector<std::string> diagnostics;
};

const char* toString(OverviewNodeKind kind);
const char* toString(OverviewEdgeKind kind);
const char* toString(OverviewGroupKind kind);

ArchitectureOverview buildArchitectureOverview(const AnalysisReport& report);
std::string formatArchitectureOverview(
    const ArchitectureOverview& overview,
    std::size_t maxNodes = 20,
    std::size_t maxEdges = 40,
    std::size_t maxDiagnostics = 20);

}  // namespace savt::core
