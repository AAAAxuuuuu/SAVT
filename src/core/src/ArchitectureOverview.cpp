#include "savt/core/ArchitectureOverview.h"

#include "savt/core/ProjectAnalysisConfig.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <numeric>
#include <optional>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace savt::core {
namespace {

struct ModuleAggregate {
    OverviewNode node;
    std::vector<const SymbolNode*> topCandidates;
    std::unordered_set<std::size_t> rawNodeIds;
};

struct ComponentInfo {
    std::size_t id = 0;
    std::vector<std::size_t> nodeIds;
    std::unordered_set<std::size_t> outgoingComponentIds;
    std::size_t incomingCount = 0;
    std::size_t representativeNodeId = 0;
    bool hasEntryNode = false;
    bool cycle = false;
    std::size_t stageIndex = 0;
};

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool containsToken(const std::string& text, const std::initializer_list<const char*> tokens) {
    const std::string lowered = toLower(text);
    return std::any_of(tokens.begin(), tokens.end(), [&](const char* token) {
        return lowered.find(token) != std::string::npos;
    });
}

std::string joinStrings(const std::vector<std::string>& values, const std::size_t limit) {
    if (values.empty()) {
        return "none";
    }

    std::ostringstream output;
    const std::size_t count = std::min(limit, values.size());
    for (std::size_t index = 0; index < count; ++index) {
        if (index > 0) {
            output << ", ";
        }
        output << values[index];
    }
    if (values.size() > count) {
        output << ", +" << (values.size() - count) << " more";
    }
    return output.str();
}

std::string lowerFileExtension(const std::string& path) {
    const std::size_t separator = path.find_last_of('.');
    if (separator == std::string::npos) {
        return {};
    }

    std::string extension = path.substr(separator);
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return extension;
}

void recordFileEvidence(OverviewNode& node, const std::string& relativePath) {
    node.filePaths.push_back(relativePath);

    const std::string extension = lowerFileExtension(relativePath);
    if (extension == ".qml") {
        ++node.qmlFileCount;
        return;
    }
    if (extension == ".html" || extension == ".htm" || extension == ".css") {
        ++node.webFileCount;
        return;
    }
    if (extension == ".py" || extension == ".js" || extension == ".mjs" || extension == ".cjs" ||
        extension == ".ts" || extension == ".tsx" || extension == ".mts" || extension == ".cts") {
        ++node.scriptFileCount;
        return;
    }
    if (extension == ".json") {
        ++node.dataFileCount;
        return;
    }
    if (extension == ".h" || extension == ".hh" || extension == ".hpp" || extension == ".hxx" ||
        extension == ".c" || extension == ".cc" || extension == ".cpp" || extension == ".cxx") {
        ++node.sourceFileCount;
    }
}

std::string buildTechnologySummary(const OverviewNode& node) {
    std::vector<std::string> technologies;
    if (node.sourceFileCount > 0) {
        technologies.push_back(std::to_string(node.sourceFileCount) + " C/C++");
    }
    if (node.qmlFileCount > 0) {
        technologies.push_back(std::to_string(node.qmlFileCount) + " QML");
    }
    if (node.webFileCount > 0) {
        technologies.push_back(std::to_string(node.webFileCount) + " Web");
    }
    if (node.scriptFileCount > 0) {
        technologies.push_back(std::to_string(node.scriptFileCount) + " Script");
    }
    if (node.dataFileCount > 0) {
        technologies.push_back(std::to_string(node.dataFileCount) + " Data");
    }
    return joinStrings(technologies, 5);
}

std::string folderKeyForModule(const std::string& moduleName) {
    const std::size_t separator = moduleName.find('/');
    if (separator == std::string::npos) {
        return moduleName;
    }
    return moduleName.substr(0, separator);
}

int symbolPriority(const SymbolKind kind) {
    switch (kind) {
    case SymbolKind::Class:
    case SymbolKind::Struct:
    case SymbolKind::Enum:
        return 0;
    case SymbolKind::Function:
        return 1;
    case SymbolKind::Method:
        return 2;
    case SymbolKind::Field:
        return 3;
    default:
        return 4;
    }
}

bool isNumericOnly(const std::string_view value) {
    return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isdigit(c) != 0;
    });
}

std::string buildNodeEvidenceText(const OverviewNode& node) {
    std::ostringstream output;
    output << node.name << ' ' << node.folderKey << ' ';
    for (const std::string& path : node.filePaths) {
        output << path << ' ';
    }
    for (const std::string& symbol : node.topSymbols) {
        output << symbol << ' ';
    }
    return output.str();
}

bool nodeMatchesConfigPrefix(const OverviewNode& node, const std::string& prefix) {
    if (matchesProjectConfigPrefix(node.name, prefix) ||
        matchesProjectConfigPrefix(node.folderKey, prefix)) {
        return true;
    }

    return std::any_of(node.filePaths.begin(), node.filePaths.end(), [&](const std::string& filePath) {
        return matchesProjectConfigPrefix(filePath, prefix);
    });
}

bool isDependencyLikeModule(const OverviewNode& node) {
    const std::string evidence = buildNodeEvidenceText(node);
    if (containsToken(evidence, {"qxlsx", "third_party", "thirdparty", "3rdparty", "vendor", "external", "dependency", "dependencies"})) {
        return true;
    }

    const bool hasLibrarySignal = containsToken(evidence, {"library", "package", "xlsx"});
    const bool hasSampleSignal = containsToken(evidence, {"sample", "samples", "example", "examples", "demo", "demos",
                                                          "helloworld", "helloandroid", "issuetest", "testexcel",
                                                          "showconsole", "readcolor", "webserver", "pump", "whattype"});
    return hasLibrarySignal && hasSampleSignal;
}

bool isWorkspaceAuxiliaryModule(const OverviewNode& node) {
    return isNumericOnly(node.folderKey);
}

bool isEntrypointSymbol(const SymbolNode& node) {
    if (node.kind != SymbolKind::Function && node.kind != SymbolKind::Method) {
        return false;
    }

    const std::string lowered = toLower(node.displayName);
    return lowered == "main" || lowered == "wwinmain" || lowered == "winmain" || lowered == "qmlmain";
}

bool isLikelyEntryModule(const OverviewNode& node) {
    if (isDependencyLikeModule(node) || isWorkspaceAuxiliaryModule(node)) {
        return false;
    }

    for (const std::string& path : node.filePaths) {
        if (containsToken(path, {"/main.cpp", "/main.cc", "/main.cxx", "/main.c"})) {
            return true;
        }
        if (containsToken(path, {"/main.js", "/main.mjs", "/main.cjs",
                                 "/main.ts", "/main.tsx", "/main.mts", "/main.cts",
                                 "/index.js", "/index.mjs", "/index.cjs",
                                 "/index.ts", "/index.tsx", "/index.mts", "/index.cts",
                                 "/app.js", "/app.mjs", "/app.cjs",
                                 "/app.ts", "/app.tsx", "/app.mts", "/app.cts",
                                 "/server.js", "/server.mjs", "/server.cjs",
                                 "/server.ts", "/server.tsx", "/server.mts", "/server.cts",
                                 "/bin/www", "application.java", "/main.java",
                                 "application.kt", "/main.kt"})) {
            return true;
        }
    }

    const std::string evidence = buildNodeEvidenceText(node);
    return containsToken(evidence, {"/bootstrap", "/startup", "application", "logindlg", "mainwindow"});
}

std::string refineModuleDisplayName(const OverviewNode& node) {
    const std::size_t separator = node.name.find_last_of('/');
    if (separator == std::string::npos || node.topSymbols.empty()) {
        return node.name;
    }

    const std::string suffix = node.name.substr(separator + 1);
    const std::string loweredSuffix = toLower(suffix);
    for (const std::string& symbol : node.topSymbols) {
        if (toLower(symbol) == loweredSuffix) {
            return node.name.substr(0, separator + 1) + symbol;
        }
    }

    return node.name;
}

std::string classifyModuleRole(const OverviewNode& node) {
    if (isDependencyLikeModule(node)) {
        return "dependency";
    }
    if (isWorkspaceAuxiliaryModule(node)) {
        return "tooling";
    }

    const std::string evidence = buildNodeEvidenceText(node);

    const bool scriptOnlyModule =
        node.scriptFileCount > 0 && node.sourceFileCount == 0 &&
        node.qmlFileCount == 0 && node.webFileCount == 0;
    const bool backendInteractionSignal = containsToken(
        evidence,
        {"express", "koa", "nestjs", "fastify", "hapi", "router", "route",
         "controller", "middleware", "endpoint", "server", "http", "api"});
    const bool backendAnalysisSignal = containsToken(
        evidence,
        {"service", "domain", "logic", "validator", "handler", "facade",
         "backend", "processor", "analysis"});
    const bool backendStorageSignal = containsToken(
        evidence,
        {"repository", "repo", "model", "schema", "database", "db",
         "mongoose", "sequelize", "typeorm", "prisma", "redis", "cache"});
    const bool frontendSignal = containsToken(
        evidence,
        {"react", "vue", "next", "nuxt", "component", "page", "layout",
         "hooks", "frontend", "client"});

    if (node.qmlFileCount > 0 && node.webFileCount >= node.sourceFileCount) {
        return "presentation";
    }
    if (node.webFileCount > 0 && node.sourceFileCount == 0 && node.scriptFileCount == 0) {
        return "web";
    }
    if (scriptOnlyModule) {
        if (backendStorageSignal) {
            return "storage";
        }
        if (backendInteractionSignal) {
            return "interaction";
        }
        if (backendAnalysisSignal) {
            return "analysis";
        }
        if (frontendSignal) {
            return "web";
        }
        if (containsToken(evidence, {"test", "spec", "mock", "fixture", "script", "build", "cli", "migration", "seed"})) {
            return "tooling";
        }
    }
    if (node.dataFileCount > 0 && node.sourceFileCount == 0 && node.qmlFileCount == 0 && node.webFileCount == 0) {
        return "data";
    }

    if (containsToken(evidence, {"ui", "view", "widget", "window", "qml", "page", "screen", "desktop", "dialog", "mainwindow", "logindlg"})) {
        return "presentation";
    }
    if (containsToken(evidence, {"controller", "handler", "route", "endpoint", "command", "facade", "backend", "management"})) {
        return "interaction";
    }
    if (containsToken(evidence, {"service", "analyzer", "analysis", "parser", "indexer", "engine", "compiler", "algorithm", "graph", "prediction", "flowanalysis", "passengerflow"})) {
        return "analysis";
    }
    if (containsToken(evidence, {"layout", "render", "canvas", "scene", "map", "web", "chart", "trend", "dynamic", "dashboard"})) {
        return "visualization";
    }
    if (containsToken(evidence, {"storage", "sqlite", "database", "repository", "cache", "persist", "resource", "data", "config", "csv", "xlsx"})) {
        return "storage";
    }
    if (containsToken(evidence, {"tool", "script", "build", "generate"})) {
        return "tooling";
    }
    if (containsToken(evidence, {"core", "model", "entity", "domain", "schema", "types"})) {
        return "core_model";
    }
    if (node.qmlFileCount > 0) {
        return "presentation";
    }
    if (node.webFileCount > 0) {
        return "web";
    }
    if (node.scriptFileCount > 0) {
        return "tooling";
    }
    if (node.dataFileCount > 0) {
        return "data";
    }
    if (node.classCount + node.structCount + node.enumCount >= node.functionCount + node.methodCount) {
        return "type_centric";
    }
    if (node.functionCount + node.methodCount > 0) {
        return "procedure_centric";
    }
    return "mixed";
}

std::optional<std::string> configuredRoleOverride(
    const OverviewNode& node,
    const ProjectAnalysisConfig& config) {
    std::optional<std::string> role;
    for (const ProjectRoleOverrideRule& rule : config.roleOverrides) {
        if (nodeMatchesConfigPrefix(node, rule.matchPrefix)) {
            role = rule.role;
        }
    }
    return role;
}

std::optional<bool> configuredEntryOverride(
    const OverviewNode& node,
    const ProjectAnalysisConfig& config) {
    std::optional<bool> entry;
    for (const ProjectEntryOverrideRule& rule : config.entryOverrides) {
        if (nodeMatchesConfigPrefix(node, rule.matchPrefix)) {
            entry = rule.entry;
        }
    }
    return entry;
}

std::string stageNameForIndex(const std::size_t stageIndex, const std::size_t maxStageIndex) {
    if (stageIndex == 0) {
        return "entry";
    }
    if (stageIndex == maxStageIndex) {
        return maxStageIndex == 0 ? "entry" : "leaf";
    }
    if (stageIndex == 1) {
        return "core";
    }
    return "downstream_" + std::to_string(stageIndex);
}

std::string joinTopSymbols(const std::vector<std::string>& symbols) {
    return joinStrings(symbols, symbols.size());
}

std::string buildSummary(const OverviewNode& node) {
    std::ostringstream output;
    if (node.kind == OverviewNodeKind::EntryPointModule) {
        output << "entry ";
    }
    output << node.role << " module in stage " << node.stageName
           << " with " << node.fileCount << " file(s), "
           << node.classCount << " class(es), "
           << node.structCount << " struct(s), "
           << node.enumCount << " enum(s), "
           << node.functionCount << " function(s), "
           << node.methodCount << " method(s)";
    if (node.fieldCount > 0) {
        output << ", and " << node.fieldCount << " field(s)";
    }
    if (node.cycleParticipant) {
        output << ". Participates in a cyclic dependency group";
    }
    if (node.reachableFromEntry) {
        output << ". Reachable from an entry module";
    }
    output << ". Tech mix: " << buildTechnologySummary(node) << ".";
    output << " Example files: " << joinStrings(node.filePaths, 4) << ".";
    output << " Top symbols: " << joinTopSymbols(node.topSymbols) << ".";
    return output.str();
}

std::string buildFolderGroupSummary(const std::string& folderKey, const std::size_t count) {
    std::ostringstream output;
    output << "Folder cluster '" << folderKey << "' containing " << count << " module(s).";
    return output.str();
}

std::string buildRoleGroupSummary(const std::string& role, const std::size_t count) {
    std::ostringstream output;
    output << "Role cluster '" << role << "' containing " << count << " module(s).";
    return output.str();
}

std::string buildCycleGroupSummary(const std::size_t count) {
    std::ostringstream output;
    output << "Strongly connected module cluster containing " << count << " module(s).";
    return output.str();
}

std::string buildFlowSummary(
    const std::string& entryName,
    const std::string& destinationName,
    const std::size_t stageSpan,
    const std::size_t totalWeight,
    const std::size_t nodeCount) {
    std::ostringstream output;
    output << "Starts at " << entryName << ", traverses " << nodeCount << " module(s) across "
           << stageSpan << " stage hop(s), and reaches " << destinationName
           << " with cumulative dependency weight " << totalWeight << ".";
    return output.str();
}

OverviewNode* findOverviewNode(std::vector<OverviewNode>& nodes, const std::size_t nodeId) {
    const auto iterator = std::find_if(nodes.begin(), nodes.end(), [nodeId](const OverviewNode& node) {
        return node.id == nodeId;
    });
    return iterator == nodes.end() ? nullptr : &(*iterator);
}

const OverviewNode* findOverviewNode(const std::vector<OverviewNode>& nodes, const std::size_t nodeId) {
    const auto iterator = std::find_if(nodes.begin(), nodes.end(), [nodeId](const OverviewNode& node) {
        return node.id == nodeId;
    });
    return iterator == nodes.end() ? nullptr : &(*iterator);
}

}  // namespace

const char* toString(const OverviewNodeKind kind) {
    switch (kind) {
    case OverviewNodeKind::Module:
        return "module";
    case OverviewNodeKind::EntryPointModule:
        return "entry_point_module";
    }

    return "unknown";
}

const char* toString(const OverviewEdgeKind kind) {
    switch (kind) {
    case OverviewEdgeKind::DependsOn:
        return "depends_on";
    }

    return "unknown";
}

const char* toString(const OverviewGroupKind kind) {
    switch (kind) {
    case OverviewGroupKind::FolderCluster:
        return "folder_cluster";
    case OverviewGroupKind::RoleCluster:
        return "role_cluster";
    case OverviewGroupKind::StronglyConnectedCluster:
        return "strongly_connected_cluster";
    }

    return "unknown";
}

ArchitectureOverview buildArchitectureOverview(const AnalysisReport& report) {
    ArchitectureOverview overview;
    const ProjectAnalysisConfig projectConfig = loadProjectAnalysisConfig(report.rootPath);
    std::size_t roleOverrideMatchCount = 0;
    std::size_t entryOverrideMatchCount = 0;

    std::unordered_map<std::string, std::size_t> filePathToFileNodeId;
    for (const SymbolNode& node : report.nodes) {
        if (node.kind == SymbolKind::File) {
            filePathToFileNodeId.emplace(node.qualifiedName, node.id);
        }
    }

    std::unordered_map<std::size_t, std::size_t> fileToModule;
    std::unordered_map<std::size_t, ModuleAggregate> moduleAggregates;
    for (const SymbolNode& node : report.nodes) {
        if (node.kind == SymbolKind::Module) {
            ModuleAggregate aggregate;
            aggregate.node.id = node.id;
            aggregate.node.kind = OverviewNodeKind::Module;
            aggregate.node.name = node.displayName;
            aggregate.node.folderKey = folderKeyForModule(node.displayName);
            aggregate.rawNodeIds.emplace(node.id);
            moduleAggregates.emplace(node.id, std::move(aggregate));
        }
    }

    for (const SymbolEdge& edge : report.edges) {
        if (edge.kind != EdgeKind::Contains) {
            continue;
        }

        const SymbolNode* fromNode = findNodeById(report, edge.fromId);
        const SymbolNode* toNode = findNodeById(report, edge.toId);
        if (fromNode == nullptr || toNode == nullptr) {
            continue;
        }

        if (fromNode->kind == SymbolKind::Module && toNode->kind == SymbolKind::File) {
            fileToModule[toNode->id] = fromNode->id;
            if (auto moduleIt = moduleAggregates.find(fromNode->id); moduleIt != moduleAggregates.end()) {
                ++moduleIt->second.node.fileCount;
                moduleIt->second.rawNodeIds.emplace(toNode->id);
                recordFileEvidence(moduleIt->second.node, toNode->qualifiedName);
            }
        }
    }

    for (const SymbolNode& node : report.nodes) {
        if (node.kind == SymbolKind::Module || node.kind == SymbolKind::File) {
            continue;
        }
        if (node.filePath.empty()) {
            continue;
        }

        const auto fileIt = filePathToFileNodeId.find(node.filePath);
        if (fileIt == filePathToFileNodeId.end()) {
            continue;
        }
        const auto moduleIt = fileToModule.find(fileIt->second);
        if (moduleIt == fileToModule.end()) {
            continue;
        }
        auto aggregateIt = moduleAggregates.find(moduleIt->second);
        if (aggregateIt == moduleAggregates.end()) {
            continue;
        }

        ModuleAggregate& aggregate = aggregateIt->second;
        aggregate.rawNodeIds.emplace(node.id);
        switch (node.kind) {
        case SymbolKind::Class:
            ++aggregate.node.classCount;
            aggregate.topCandidates.push_back(&node);
            break;
        case SymbolKind::Struct:
            ++aggregate.node.structCount;
            aggregate.topCandidates.push_back(&node);
            break;
        case SymbolKind::Enum:
            ++aggregate.node.enumCount;
            aggregate.topCandidates.push_back(&node);
            break;
        case SymbolKind::Field:
            ++aggregate.node.fieldCount;
            break;
        case SymbolKind::Function:
            ++aggregate.node.functionCount;
            aggregate.topCandidates.push_back(&node);
            break;
        case SymbolKind::Method:
            ++aggregate.node.methodCount;
            aggregate.topCandidates.push_back(&node);
            break;
        default:
            break;
        }

        if (isEntrypointSymbol(node) || containsToken(node.filePath, {"/main.cpp", "/main.cc", "/main.cxx", "/main.c"})) {
            aggregate.node.kind = OverviewNodeKind::EntryPointModule;
        }
    }

    for (auto& [moduleId, aggregate] : moduleAggregates) {
        std::sort(aggregate.topCandidates.begin(), aggregate.topCandidates.end(), [](const SymbolNode* left, const SymbolNode* right) {
            const int priorityDelta = symbolPriority(left->kind) - symbolPriority(right->kind);
            if (priorityDelta != 0) {
                return priorityDelta < 0;
            }
            if (left->filePath != right->filePath) {
                return left->filePath < right->filePath;
            }
            if (left->line != right->line) {
                return left->line < right->line;
            }
            return left->displayName < right->displayName;
        });

        std::unordered_set<std::string> seenNames;
        for (const SymbolNode* candidate : aggregate.topCandidates) {
            if (candidate == nullptr || candidate->displayName.empty()) {
                continue;
            }
            if (!seenNames.emplace(candidate->displayName).second) {
                continue;
            }
            aggregate.node.topSymbols.push_back(candidate->displayName);
            if (aggregate.node.topSymbols.size() >= 5) {
                break;
            }
        }

        aggregate.node.rawNodeIds.assign(aggregate.rawNodeIds.begin(), aggregate.rawNodeIds.end());
        std::sort(aggregate.node.rawNodeIds.begin(), aggregate.node.rawNodeIds.end());
        std::sort(aggregate.node.filePaths.begin(), aggregate.node.filePaths.end());
        aggregate.node.name = refineModuleDisplayName(aggregate.node);
        aggregate.node.role = classifyModuleRole(aggregate.node);
        if (const auto configuredRole = configuredRoleOverride(aggregate.node, projectConfig); configuredRole.has_value()) {
            aggregate.node.role = *configuredRole;
            ++roleOverrideMatchCount;
        }
        const auto configuredEntry = configuredEntryOverride(aggregate.node, projectConfig);
        const bool suppressImplicitEntry =
            aggregate.node.role == "dependency" ||
            isDependencyLikeModule(aggregate.node) || isWorkspaceAuxiliaryModule(aggregate.node);
        if (suppressImplicitEntry && aggregate.node.kind == OverviewNodeKind::EntryPointModule &&
            (!configuredEntry.has_value() || !configuredEntry.value())) {
            aggregate.node.kind = OverviewNodeKind::Module;
        }
        if (!configuredEntry.has_value() &&
            aggregate.node.kind != OverviewNodeKind::EntryPointModule &&
            isLikelyEntryModule(aggregate.node)) {
            aggregate.node.kind = OverviewNodeKind::EntryPointModule;
        }
        if (configuredEntry.has_value()) {
            aggregate.node.kind = *configuredEntry ? OverviewNodeKind::EntryPointModule : OverviewNodeKind::Module;
            ++entryOverrideMatchCount;
        }
        overview.nodes.push_back(std::move(aggregate.node));
    }

    std::sort(overview.nodes.begin(), overview.nodes.end(), [](const OverviewNode& left, const OverviewNode& right) {
        if (left.kind != right.kind) {
            return left.kind == OverviewNodeKind::EntryPointModule;
        }
        return left.name < right.name;
    });

    std::unordered_map<std::size_t, OverviewNode*> overviewNodeById;
    for (OverviewNode& node : overview.nodes) {
        overviewNodeById.emplace(node.id, &node);
    }

    overview.diagnostics.insert(
        overview.diagnostics.end(),
        projectConfig.diagnostics.begin(),
        projectConfig.diagnostics.end());
    if (projectConfig.loaded) {
        if (roleOverrideMatchCount > 0) {
            overview.diagnostics.push_back(
                "Project config applied " + std::to_string(roleOverrideMatchCount) +
                " role override(s) during overview generation.");
        }
        if (entryOverrideMatchCount > 0) {
            overview.diagnostics.push_back(
                "Project config applied " + std::to_string(entryOverrideMatchCount) +
                " entry override(s) during overview generation.");
        }
    }

    std::unordered_map<std::size_t, std::vector<OverviewEdge>> adjacency;
    std::unordered_map<std::size_t, std::vector<std::pair<std::size_t, std::size_t>>> reverseAdjacency;
    std::unordered_map<std::size_t, std::size_t> indegree;
    std::unordered_set<std::size_t> selfLoopNodes;

    for (const SymbolEdge& edge : report.edges) {
        if (edge.kind != EdgeKind::DependsOn) {
            continue;
        }
        const auto fromIt = overviewNodeById.find(edge.fromId);
        const auto toIt = overviewNodeById.find(edge.toId);
        if (fromIt == overviewNodeById.end() || toIt == overviewNodeById.end()) {
            continue;
        }

        overview.edges.push_back(OverviewEdge{edge.fromId, edge.toId, OverviewEdgeKind::DependsOn, edge.weight});
        adjacency[edge.fromId].push_back(overview.edges.back());
        reverseAdjacency[edge.toId].push_back({edge.fromId, edge.weight});
        ++fromIt->second->outgoingDependencyCount;
        ++toIt->second->incomingDependencyCount;
        ++indegree[edge.toId];
        if (edge.fromId == edge.toId) {
            selfLoopNodes.emplace(edge.fromId);
        }
    }

    std::vector<std::size_t> entryNodeIds;
    for (const OverviewNode& node : overview.nodes) {
        if (node.kind == OverviewNodeKind::EntryPointModule) {
            entryNodeIds.push_back(node.id);
        }
    }
    if (entryNodeIds.empty()) {
        for (const OverviewNode& node : overview.nodes) {
            if (!indegree.contains(node.id)) {
                entryNodeIds.push_back(node.id);
            }
        }
    }

    std::unordered_set<std::size_t> reachableFromEntry;
    std::queue<std::size_t> reachableQueue;
    for (const std::size_t entryNodeId : entryNodeIds) {
        if (reachableFromEntry.emplace(entryNodeId).second) {
            reachableQueue.push(entryNodeId);
        }
    }
    while (!reachableQueue.empty()) {
        const std::size_t currentId = reachableQueue.front();
        reachableQueue.pop();
        for (const OverviewEdge& edge : adjacency[currentId]) {
            if (reachableFromEntry.emplace(edge.toId).second) {
                reachableQueue.push(edge.toId);
            }
        }
    }

    std::unordered_map<std::size_t, std::size_t> tarjanIndex;
    std::unordered_map<std::size_t, std::size_t> lowLink;
    std::unordered_set<std::size_t> onStack;
    std::vector<std::size_t> tarjanStack;
    std::vector<ComponentInfo> components;
    std::unordered_map<std::size_t, std::size_t> nodeToComponentId;
    std::size_t nextTarjanIndex = 0;

    std::function<void(std::size_t)> strongConnect = [&](const std::size_t nodeId) {
        tarjanIndex[nodeId] = nextTarjanIndex;
        lowLink[nodeId] = nextTarjanIndex;
        ++nextTarjanIndex;
        tarjanStack.push_back(nodeId);
        onStack.emplace(nodeId);

        for (const OverviewEdge& edge : adjacency[nodeId]) {
            if (!tarjanIndex.contains(edge.toId)) {
                strongConnect(edge.toId);
                lowLink[nodeId] = std::min(lowLink[nodeId], lowLink[edge.toId]);
            } else if (onStack.contains(edge.toId)) {
                lowLink[nodeId] = std::min(lowLink[nodeId], tarjanIndex[edge.toId]);
            }
        }

        if (lowLink[nodeId] != tarjanIndex[nodeId]) {
            return;
        }

        ComponentInfo component;
        component.id = components.size();
        while (!tarjanStack.empty()) {
            const std::size_t memberNodeId = tarjanStack.back();
            tarjanStack.pop_back();
            onStack.erase(memberNodeId);
            component.nodeIds.push_back(memberNodeId);
            nodeToComponentId[memberNodeId] = component.id;
            if (memberNodeId == nodeId) {
                break;
            }
        }
        std::sort(component.nodeIds.begin(), component.nodeIds.end());
        components.push_back(std::move(component));
    };

    for (const OverviewNode& node : overview.nodes) {
        if (!tarjanIndex.contains(node.id)) {
            strongConnect(node.id);
        }
    }

    std::unordered_map<std::size_t, OverviewGroup*> groupById;
    std::size_t nextGroupId = 1;

    std::unordered_map<std::string, std::vector<std::size_t>> folderGroups;
    std::unordered_map<std::string, std::vector<std::size_t>> roleGroups;
    for (OverviewNode& node : overview.nodes) {
        folderGroups[node.folderKey].push_back(node.id);
        roleGroups[node.role].push_back(node.id);
    }

    auto makeGroup = [&](const OverviewGroupKind kind,
                         std::string name,
                         std::string summary,
                         std::vector<std::size_t> nodeIds,
                         const std::size_t hierarchyLevel,
                         const bool defaultExpanded,
                         const std::size_t parentGroupId = 0) -> std::size_t {
        OverviewGroup group;
        group.id = nextGroupId++;
        group.kind = kind;
        group.name = std::move(name);
        group.summary = std::move(summary);
        group.nodeIds = std::move(nodeIds);
        std::sort(group.nodeIds.begin(), group.nodeIds.end());
        group.parentGroupId = parentGroupId;
        group.hierarchyLevel = hierarchyLevel;
        group.defaultExpanded = defaultExpanded;
        overview.groups.push_back(std::move(group));
        groupById.emplace(overview.groups.back().id, &overview.groups.back());
        return overview.groups.back().id;
    };

    std::unordered_map<std::string, std::size_t> folderKeyToGroupId;
    for (auto& [folderKey, nodeIds] : folderGroups) {
        const bool hasEntry = std::any_of(nodeIds.begin(), nodeIds.end(), [&](const std::size_t nodeId) {
            const OverviewNode* node = findOverviewNode(overview.nodes, nodeId);
            return node != nullptr && node->kind == OverviewNodeKind::EntryPointModule;
        });
        const std::size_t groupId = makeGroup(
            OverviewGroupKind::FolderCluster,
            folderKey,
            buildFolderGroupSummary(folderKey, nodeIds.size()),
            nodeIds,
            0,
            hasEntry || nodeIds.size() <= 3);
        folderKeyToGroupId.emplace(folderKey, groupId);
    }

    for (auto& [role, nodeIds] : roleGroups) {
        const std::size_t groupId = makeGroup(
            OverviewGroupKind::RoleCluster,
            role,
            buildRoleGroupSummary(role, nodeIds.size()),
            nodeIds,
            1,
            false);
        for (const std::size_t nodeId : nodeIds) {
            if (OverviewNode* node = findOverviewNode(overview.nodes, nodeId); node != nullptr) {
                node->roleGroupId = groupId;
            }
        }
    }

    for (ComponentInfo& component : components) {
        component.cycle = component.nodeIds.size() > 1;
        if (!component.cycle && !component.nodeIds.empty() && selfLoopNodes.contains(component.nodeIds.front())) {
            component.cycle = true;
        }

        component.representativeNodeId = component.nodeIds.front();
        std::size_t bestScore = 0;
        for (const std::size_t nodeId : component.nodeIds) {
            const OverviewNode* node = findOverviewNode(overview.nodes, nodeId);
            if (node == nullptr) {
                continue;
            }
            component.hasEntryNode = component.hasEntryNode ||
                                     node->kind == OverviewNodeKind::EntryPointModule ||
                                     std::find(entryNodeIds.begin(), entryNodeIds.end(), nodeId) != entryNodeIds.end();
            const std::size_t score = node->incomingDependencyCount + node->outgoingDependencyCount +
                                      (node->kind == OverviewNodeKind::EntryPointModule ? 100 : 0);
            if (score >= bestScore) {
                bestScore = score;
                component.representativeNodeId = nodeId;
            }
        }
    }

    for (const OverviewEdge& edge : overview.edges) {
        const std::size_t fromComponentId = nodeToComponentId[edge.fromId];
        const std::size_t toComponentId = nodeToComponentId[edge.toId];
        if (fromComponentId == toComponentId) {
            continue;
        }

        if (components[fromComponentId].outgoingComponentIds.emplace(toComponentId).second) {
            ++components[toComponentId].incomingCount;
        }
    }

    std::queue<std::size_t> componentQueue;
    std::vector<std::size_t> remainingIncoming(components.size(), 0);
    for (const ComponentInfo& component : components) {
        remainingIncoming[component.id] = component.incomingCount;
    }

    for (const ComponentInfo& component : components) {
        if (component.hasEntryNode || remainingIncoming[component.id] == 0) {
            componentQueue.push(component.id);
        }
    }

    std::unordered_set<std::size_t> queuedComponents;
    std::vector<std::size_t> componentOrder;
    while (!componentQueue.empty()) {
        const std::size_t componentId = componentQueue.front();
        componentQueue.pop();
        if (!queuedComponents.emplace(componentId).second) {
            continue;
        }
        componentOrder.push_back(componentId);
        for (const std::size_t nextComponentId : components[componentId].outgoingComponentIds) {
            if (remainingIncoming[nextComponentId] > 0) {
                --remainingIncoming[nextComponentId];
            }
            if (remainingIncoming[nextComponentId] == 0) {
                componentQueue.push(nextComponentId);
            }
        }
    }
    for (const ComponentInfo& component : components) {
        if (!queuedComponents.contains(component.id)) {
            componentOrder.push_back(component.id);
        }
    }

    for (const std::size_t componentId : componentOrder) {
        ComponentInfo& component = components[componentId];
        std::size_t stageIndex = component.hasEntryNode ? 0 : 0;
        for (const std::size_t nodeId : component.nodeIds) {
            const auto reverseIt = reverseAdjacency.find(nodeId);
            if (reverseIt == reverseAdjacency.end()) {
                continue;
            }
            for (const auto& [previousNodeId, weight] : reverseIt->second) {
                static_cast<void>(weight);
                const std::size_t previousComponentId = nodeToComponentId[previousNodeId];
                if (previousComponentId == componentId) {
                    continue;
                }
                stageIndex = std::max(stageIndex, components[previousComponentId].stageIndex + 1);
            }
        }
        component.stageIndex = stageIndex;
    }

    const std::size_t maxStageIndex = components.empty()
                                          ? 0
                                          : std::max_element(components.begin(), components.end(), [](const ComponentInfo& left, const ComponentInfo& right) {
                                                return left.stageIndex < right.stageIndex;
                                            })->stageIndex;

    overview.stages.resize(maxStageIndex + 1);
    for (std::size_t stageIndex = 0; stageIndex <= maxStageIndex; ++stageIndex) {
        overview.stages[stageIndex].index = stageIndex;
        overview.stages[stageIndex].name = stageNameForIndex(stageIndex, maxStageIndex);
    }

    for (const ComponentInfo& component : components) {
        std::size_t cycleGroupId = 0;
        if (component.cycle) {
            cycleGroupId = makeGroup(
                OverviewGroupKind::StronglyConnectedCluster,
                "cycle_" + std::to_string(component.id),
                buildCycleGroupSummary(component.nodeIds.size()),
                component.nodeIds,
                2,
                true);
        }

        for (const std::size_t nodeId : component.nodeIds) {
            if (OverviewNode* node = findOverviewNode(overview.nodes, nodeId); node != nullptr) {
                node->stageIndex = component.stageIndex;
                node->stageName = overview.stages[component.stageIndex].name;
                node->reachableFromEntry = reachableFromEntry.contains(nodeId);
                node->cycleParticipant = component.cycle;
                node->cycleGroupId = cycleGroupId;
                if (const auto folderGroupIt = folderKeyToGroupId.find(node->folderKey); folderGroupIt != folderKeyToGroupId.end()) {
                    node->folderGroupId = folderGroupIt->second;
                }
                overview.stages[component.stageIndex].nodeIds.push_back(nodeId);
            }
        }
    }

    for (OverviewStage& stage : overview.stages) {
        std::sort(stage.nodeIds.begin(), stage.nodeIds.end());
    }

    std::unordered_map<std::size_t, std::size_t> bestScore;
    std::unordered_map<std::size_t, std::size_t> bestPreviousComponent;
    for (const ComponentInfo& component : components) {
        bestScore[component.id] = component.hasEntryNode ? 1 : 0;
    }

    for (const std::size_t componentId : componentOrder) {
        for (const std::size_t nextComponentId : components[componentId].outgoingComponentIds) {
            std::size_t edgeWeight = 1;
            for (const std::size_t fromNodeId : components[componentId].nodeIds) {
                for (const OverviewEdge& edge : adjacency[fromNodeId]) {
                    if (nodeToComponentId[edge.toId] == nextComponentId) {
                        edgeWeight = std::max(edgeWeight, edge.weight);
                    }
                }
            }

            const std::size_t candidateScore = bestScore[componentId] + edgeWeight;
            if (candidateScore > bestScore[nextComponentId]) {
                bestScore[nextComponentId] = candidateScore;
                bestPreviousComponent[nextComponentId] = componentId;
            }
        }
    }

    std::vector<std::pair<std::size_t, std::size_t>> rankedComponents;
    for (const ComponentInfo& component : components) {
        if (!component.hasEntryNode && bestScore[component.id] == 0) {
            continue;
        }
        rankedComponents.push_back({component.id, bestScore[component.id]});
    }
    std::sort(rankedComponents.begin(), rankedComponents.end(), [](const auto& left, const auto& right) {
        if (left.second != right.second) {
            return left.second > right.second;
        }
        return left.first < right.first;
    });

    std::unordered_set<std::string> seenFlowSignatures;
    std::size_t nextFlowId = 1;
    for (const auto& [componentId, score] : rankedComponents) {
        if (overview.flows.size() >= 5) {
            break;
        }

        std::vector<std::size_t> componentPath;
        std::size_t cursor = componentId;
        componentPath.push_back(cursor);
        while (bestPreviousComponent.contains(cursor)) {
            cursor = bestPreviousComponent[cursor];
            componentPath.push_back(cursor);
        }
        std::reverse(componentPath.begin(), componentPath.end());

        std::string signature;
        std::vector<std::size_t> flowNodeIds;
        for (const std::size_t pathComponentId : componentPath) {
            const std::size_t representativeNodeId = components[pathComponentId].representativeNodeId;
            if (!signature.empty()) {
                signature += ">";
            }
            signature += std::to_string(representativeNodeId);
            if (flowNodeIds.empty() || flowNodeIds.back() != representativeNodeId) {
                flowNodeIds.push_back(representativeNodeId);
            }
        }
        if (flowNodeIds.size() < 2 || !seenFlowSignatures.emplace(signature).second) {
            continue;
        }

        const OverviewNode* entryNode = findOverviewNode(overview.nodes, flowNodeIds.front());
        const OverviewNode* destinationNode = findOverviewNode(overview.nodes, flowNodeIds.back());
        if (entryNode == nullptr || destinationNode == nullptr) {
            continue;
        }

        OverviewFlow flow;
        flow.id = nextFlowId++;
        flow.name = entryNode->name + " -> " + destinationNode->name;
        flow.nodeIds = std::move(flowNodeIds);
        flow.totalWeight = score;
        flow.stageSpan = destinationNode->stageIndex >= entryNode->stageIndex
                             ? destinationNode->stageIndex - entryNode->stageIndex
                             : 0;
        flow.summary = buildFlowSummary(
            entryNode->name,
            destinationNode->name,
            flow.stageSpan,
            flow.totalWeight,
            flow.nodeIds.size());
        overview.flows.push_back(std::move(flow));
    }

    for (OverviewNode& node : overview.nodes) {
        node.summary = buildSummary(node);
    }

    std::sort(overview.groups.begin(), overview.groups.end(), [](const OverviewGroup& left, const OverviewGroup& right) {
        if (left.hierarchyLevel != right.hierarchyLevel) {
            return left.hierarchyLevel < right.hierarchyLevel;
        }
        if (left.kind != right.kind) {
            return static_cast<int>(left.kind) < static_cast<int>(right.kind);
        }
        return left.name < right.name;
    });

    if (overview.nodes.empty()) {
        overview.diagnostics.push_back("No module-level overview nodes could be derived from the analysis report.");
    } else {
        const std::size_t entryCount = std::count_if(overview.nodes.begin(), overview.nodes.end(), [](const OverviewNode& node) {
            return node.kind == OverviewNodeKind::EntryPointModule;
        });
        const std::size_t nonCppModuleCount = std::count_if(overview.nodes.begin(), overview.nodes.end(), [](const OverviewNode& node) {
            return node.qmlFileCount > 0 || node.webFileCount > 0 || node.scriptFileCount > 0 || node.dataFileCount > 0;
        });
        const double averageFilesPerModule = overview.nodes.empty()
                                                 ? 0.0
                                                 : static_cast<double>(report.discoveredFiles) / static_cast<double>(overview.nodes.size());

        std::ostringstream summary;
        summary << "Derived " << overview.nodes.size() << " overview module(s) from " << report.discoveredFiles
                << " discovered file(s); average " << averageFilesPerModule << " file(s) per module.";
        overview.diagnostics.push_back(summary.str());

        if (entryCount == 0) {
            overview.diagnostics.push_back("No explicit entry module was detected; overview entry points are inferred from dependency roots only.");
        }
        if (nonCppModuleCount > 0) {
            overview.diagnostics.push_back(
                "Cross-artifact inventory captured QML/Web/Python/Data-backed modules in addition to C/C++ modules.");
        }
        if (report.discoveredFiles >= 12 && overview.nodes.size() <= 3) {
            overview.diagnostics.push_back(
                "Overview remains very coarse relative to project size; module inference may still need deeper project-specific splitting.");
        }
    }
    if (overview.flows.empty() && !overview.nodes.empty()) {
        overview.diagnostics.push_back("No multi-module main flow could be derived from the current dependency graph.");
    }

    return overview;
}

std::string formatArchitectureOverview(
    const ArchitectureOverview& overview,
    const std::size_t maxNodes,
    const std::size_t maxEdges,
    const std::size_t maxDiagnostics) {
    std::ostringstream output;
    output << "[overview]\n";
    output << "overview nodes: " << overview.nodes.size() << "\n";
    output << "overview edges: " << overview.edges.size() << "\n";
    output << "overview groups: " << overview.groups.size() << "\n";
    output << "overview stages: " << overview.stages.size() << "\n";
    output << "overview flows: " << overview.flows.size() << "\n";

    const std::size_t nodeLimit = std::min(maxNodes, overview.nodes.size());
    for (std::size_t index = 0; index < nodeLimit; ++index) {
        const OverviewNode& node = overview.nodes[index];
        output << "- [" << toString(node.kind) << "] " << node.name
               << " role=" << node.role
               << " stage=" << node.stageName
               << " in=" << node.incomingDependencyCount
               << " out=" << node.outgoingDependencyCount;
        if (node.cycleParticipant) {
            output << " cycle=yes";
        }
        output << "\n";
        output << "  summary: " << node.summary << "\n";
    }
    if (overview.nodes.size() > nodeLimit) {
        output << "... " << (overview.nodes.size() - nodeLimit) << " more overview nodes\n";
    }

    output << "\n[overview edges]\n";
    const std::size_t edgeLimit = std::min(maxEdges, overview.edges.size());
    if (edgeLimit == 0) {
        output << "- none\n";
    } else {
        std::unordered_map<std::size_t, const OverviewNode*> nodeIndex;
        for (const OverviewNode& node : overview.nodes) {
            nodeIndex.emplace(node.id, &node);
        }

        for (std::size_t index = 0; index < edgeLimit; ++index) {
            const OverviewEdge& edge = overview.edges[index];
            const auto fromIt = nodeIndex.find(edge.fromId);
            const auto toIt = nodeIndex.find(edge.toId);
            if (fromIt == nodeIndex.end() || toIt == nodeIndex.end()) {
                continue;
            }

            output << "- " << toString(edge.kind) << ": "
                   << fromIt->second->name << " -> " << toIt->second->name
                   << " (weight=" << edge.weight << ")\n";
        }
        if (overview.edges.size() > edgeLimit) {
            output << "... " << (overview.edges.size() - edgeLimit) << " more overview edges\n";
        }
    }

    output << "\n[overview groups]\n";
    const std::size_t groupLimit = std::min<std::size_t>(20, overview.groups.size());
    if (groupLimit == 0) {
        output << "- none\n";
    } else {
        for (std::size_t index = 0; index < groupLimit; ++index) {
            const OverviewGroup& group = overview.groups[index];
            output << "- [" << toString(group.kind) << "] " << group.name
                   << " nodes=" << group.nodeIds.size()
                   << " expanded=" << (group.defaultExpanded ? "yes" : "no") << "\n";
            output << "  summary: " << group.summary << "\n";
        }
        if (overview.groups.size() > groupLimit) {
            output << "... " << (overview.groups.size() - groupLimit) << " more overview groups\n";
        }
    }

    output << "\n[overview stages]\n";
    if (overview.stages.empty()) {
        output << "- none\n";
    } else {
        for (const OverviewStage& stage : overview.stages) {
            output << "- stage " << stage.index << " (" << stage.name << "): "
                   << stage.nodeIds.size() << " module(s)\n";
        }
    }

    output << "\n[overview flows]\n";
    const std::size_t flowLimit = std::min<std::size_t>(10, overview.flows.size());
    if (flowLimit == 0) {
        output << "- none\n";
    } else {
        std::unordered_map<std::size_t, const OverviewNode*> nodeIndex;
        for (const OverviewNode& node : overview.nodes) {
            nodeIndex.emplace(node.id, &node);
        }
        for (std::size_t index = 0; index < flowLimit; ++index) {
            const OverviewFlow& flow = overview.flows[index];
            output << "- " << flow.name << " (weight=" << flow.totalWeight
                   << ", stage_span=" << flow.stageSpan << ")\n";
            output << "  summary: " << flow.summary << "\n";
            output << "  path: ";
            for (std::size_t pathIndex = 0; pathIndex < flow.nodeIds.size(); ++pathIndex) {
                const auto nodeIt = nodeIndex.find(flow.nodeIds[pathIndex]);
                if (nodeIt == nodeIndex.end()) {
                    continue;
                }
                if (pathIndex > 0) {
                    output << " -> ";
                }
                output << nodeIt->second->name;
            }
            output << "\n";
        }
        if (overview.flows.size() > flowLimit) {
            output << "... " << (overview.flows.size() - flowLimit) << " more overview flows\n";
        }
    }

    output << "\n[overview diagnostics]\n";
    const std::size_t diagnosticLimit = std::min(maxDiagnostics, overview.diagnostics.size());
    if (diagnosticLimit == 0) {
        output << "- none\n";
    } else {
        for (std::size_t index = 0; index < diagnosticLimit; ++index) {
            output << "- " << overview.diagnostics[index] << "\n";
        }
        if (overview.diagnostics.size() > diagnosticLimit) {
            output << "... " << (overview.diagnostics.size() - diagnosticLimit) << " more diagnostics\n";
        }
    }

    return output.str();
}

}  // namespace savt::core
