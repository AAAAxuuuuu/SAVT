#include "savt/core/CapabilityGraph.h"
#include "savt/core/ProjectAnalysisConfig.h"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace savt::core {
namespace {

using EvidencePackage = CapabilityNode::EvidencePackage;

struct CapabilityAggregate {
    CapabilityNode node;
    std::unordered_map<std::string, std::size_t> roleHistogram;
    std::unordered_set<std::size_t> overviewNodeIdSet;
    std::unordered_set<std::string> moduleNameSet;
    std::unordered_set<std::string> topSymbolSet;
    std::unordered_set<std::string> supportingRoleSet;
    std::unordered_set<std::string> exampleFileSet;
};

struct CapabilityNodeBuildTrace {
    std::string roleBucket;
    bool usedFolderScope = false;
    bool hiddenByStrictPruning = false;
    std::vector<std::string> matchedRules;
};

struct CapabilityEdgeAggregate {
    CapabilityEdge edge;
    std::unordered_set<std::string> sourceFileSet;
    std::unordered_set<std::string> symbolSet;
    std::unordered_set<std::string> moduleSet;
    std::unordered_set<std::string> relationshipSet;
    std::size_t supportingOverviewEdgeCount = 0;
};

enum class CapabilityAggregationMode {
    ModuleScoped,
    FolderRoleScoped,
    RoleScoped
};

std::string humanize(std::string value) {
    for (char& character : value) {
        if (character == '_' || character == '/' || character == '-' || character == '.') {
            character = ' ';
        }
    }

    bool capitalizeNext = true;
    for (char& character : value) {
        if (std::isspace(static_cast<unsigned char>(character))) {
            capitalizeNext = true;
            continue;
        }
        if (capitalizeNext) {
            character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
            capitalizeNext = false;
        }
    }
    return value;
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

template <typename Set>
std::vector<std::string> sortedVectorFromSet(const Set& values, const std::size_t limit = 0) {
    std::vector<std::string> items(values.begin(), values.end());
    std::sort(items.begin(), items.end());
    if (limit > 0 && items.size() > limit) {
        items.resize(limit);
    }
    return items;
}

bool capabilityMatchesConfigPrefix(const CapabilityNode& node, const std::string& prefix) {
    if (matchesProjectConfigPrefix(node.name, prefix) ||
        matchesProjectConfigPrefix(node.folderHint, prefix)) {
        return true;
    }

    const auto matchesIn = [&](const std::vector<std::string>& values) {
        return std::any_of(values.begin(), values.end(), [&](const std::string& value) {
            return matchesProjectConfigPrefix(value, prefix);
        });
    };

    return matchesIn(node.moduleNames) || matchesIn(node.exampleFiles);
}

const char* toString(const CapabilityAggregationMode mode) {
    switch (mode) {
    case CapabilityAggregationMode::ModuleScoped:
        return "module_scoped";
    case CapabilityAggregationMode::FolderRoleScoped:
        return "folder_role_scoped";
    case CapabilityAggregationMode::RoleScoped:
        return "role_scoped";
    }

    return "unknown";
}

CapabilityAggregationMode chooseAggregationMode(const ArchitectureOverview& overview) {
    const std::size_t crossArtifactModuleCount = static_cast<std::size_t>(std::count_if(
        overview.nodes.begin(),
        overview.nodes.end(),
        [](const OverviewNode& node) {
            return node.qmlFileCount > 0 || node.webFileCount > 0 || node.scriptFileCount > 0 || node.dataFileCount > 0;
        }));

    if (overview.nodes.size() <= 16) {
        return CapabilityAggregationMode::ModuleScoped;
    }

    // Small mixed-language repos benefit from module detail, but larger ones
    // should aggregate sooner to avoid exploding the scene.
    if (crossArtifactModuleCount >= 3 && overview.nodes.size() <= 24) {
        return CapabilityAggregationMode::ModuleScoped;
    }

    if (overview.nodes.size() <= 48) {
        return CapabilityAggregationMode::FolderRoleScoped;
    }
    return CapabilityAggregationMode::RoleScoped;
}

CapabilityNodeKind classifyCapabilityKind(const OverviewNode& node) {
    if (node.kind == OverviewNodeKind::EntryPointModule) {
        return CapabilityNodeKind::Entry;
    }

    if (node.role == "dependency" || node.role == "storage" || node.role == "data" || node.role == "tooling" ||
        node.role == "core_model" || node.role == "type_centric") {
        return CapabilityNodeKind::Infrastructure;
    }

    if (node.stageName == "leaf" && node.outgoingDependencyCount == 0) {
        return CapabilityNodeKind::Infrastructure;
    }

    return CapabilityNodeKind::Capability;
}

std::string bucketRole(const OverviewNode& node, const CapabilityNodeKind kind) {
    if (kind == CapabilityNodeKind::Entry) {
        return "entry";
    }

    if (node.role == "presentation" || node.role == "interaction") {
        return "experience";
    }
    if (node.role == "analysis") {
        return "analysis";
    }
    if (node.role == "visualization") {
        return "visualization";
    }
    if (node.role == "web") {
        return "web";
    }
    if (node.role == "storage") {
        return "storage";
    }
    if (node.role == "data") {
        return "data";
    }
    if (node.role == "tooling") {
        return "tooling";
    }
    if (node.role == "dependency") {
        return "dependency";
    }
    if (node.role == "core_model" || node.role == "type_centric") {
        return "foundation";
    }
    if (node.role == "procedure_centric") {
        return "workflow";
    }
    return kind == CapabilityNodeKind::Infrastructure ? "support" : "capability";
}

bool shouldUseFolderScope(
    const OverviewNode& node,
    const CapabilityNodeKind kind,
    const std::unordered_map<std::string, std::size_t>& folderCounts) {
    if (kind == CapabilityNodeKind::Entry || node.folderKey.empty()) {
        return false;
    }

    const auto iterator = folderCounts.find(node.folderKey);
    if (iterator == folderCounts.end() || iterator->second <= 1) {
        return false;
    }

    return node.folderKey != node.name;
}

std::string clusterKeyForNode(
    const OverviewNode& node,
    const CapabilityNodeKind kind,
    const std::string& roleBucket,
    const CapabilityAggregationMode aggregationMode,
    const bool useFolderScope) {
    if (kind == CapabilityNodeKind::Entry) {
        return "entry:" + std::to_string(node.id);
    }

    const std::string prefix = kind == CapabilityNodeKind::Infrastructure ? "infra:" : "cap:";
    if (aggregationMode == CapabilityAggregationMode::ModuleScoped) {
        return prefix + "module:" + std::to_string(node.id);
    }
    if (aggregationMode == CapabilityAggregationMode::FolderRoleScoped && useFolderScope) {
        return prefix + node.folderKey + ":" + roleBucket;
    }
    return prefix + roleBucket;
}

std::string clusterNameForNode(
    const OverviewNode& node,
    const CapabilityNodeKind kind,
    const std::string& roleBucket,
    const CapabilityAggregationMode aggregationMode,
    const bool useFolderScope) {
    if (kind == CapabilityNodeKind::Entry || aggregationMode == CapabilityAggregationMode::ModuleScoped) {
        return humanize(node.name);
    }

    const std::string roleLabel = humanize(roleBucket);
    if (aggregationMode == CapabilityAggregationMode::FolderRoleScoped && useFolderScope) {
        const std::string folderLabel = humanize(node.folderKey);
        return folderLabel + " " + roleLabel + (kind == CapabilityNodeKind::Infrastructure ? " Infrastructure" : " Capability");
    }
    return roleLabel + (kind == CapabilityNodeKind::Infrastructure ? " Infrastructure" : " Capability");
}

CapabilityNode* findCapabilityNode(std::vector<CapabilityNode>& nodes, const std::size_t nodeId) {
    const auto iterator = std::find_if(nodes.begin(), nodes.end(), [nodeId](const CapabilityNode& node) {
        return node.id == nodeId;
    });
    return iterator == nodes.end() ? nullptr : &(*iterator);
}

const CapabilityNode* findCapabilityNode(const std::vector<CapabilityNode>& nodes, const std::size_t nodeId) {
    const auto iterator = std::find_if(nodes.begin(), nodes.end(), [nodeId](const CapabilityNode& node) {
        return node.id == nodeId;
    });
    return iterator == nodes.end() ? nullptr : &(*iterator);
}

std::string dominantRoleFromHistogram(const std::unordered_map<std::string, std::size_t>& histogram) {
    std::string bestRole = "capability";
    std::size_t bestCount = 0;
    for (const auto& [role, count] : histogram) {
        if (count > bestCount || (count == bestCount && role < bestRole)) {
            bestRole = role;
            bestCount = count;
        }
    }
    return bestRole;
}

std::string laneNameForKind(const CapabilityNodeKind kind) {
    switch (kind) {
    case CapabilityNodeKind::Entry:
        return "entry lane";
    case CapabilityNodeKind::Capability:
        return "capability lane";
    case CapabilityNodeKind::Infrastructure:
        return "infrastructure lane";
    }

    return "lane";
}

std::string utf8Literal(const char8_t* text) {
    return std::string(reinterpret_cast<const char*>(text));
}

std::string confidenceLabelFromScore(const std::size_t score) {
    if (score >= 75) {
        return "high";
    }
    if (score >= 45) {
        return "medium";
    }
    return "low";
}

std::size_t computeNodeConfidenceScore(
    const CapabilityNode& node,
    const CapabilityAggregationMode aggregationMode) {
    std::size_t score = 0;
    score += std::min<std::size_t>(node.fileCount * 8, 32);
    score += std::min<std::size_t>(node.topSymbols.size() * 12, 24);
    score += std::min<std::size_t>(node.collaboratorCount * 4, 16);
    score += node.reachableFromEntry ? 10 : 0;
    score += node.kind == CapabilityNodeKind::Entry ? 18 : 0;
    if (aggregationMode == CapabilityAggregationMode::ModuleScoped) {
        score += 18;
    } else if (aggregationMode == CapabilityAggregationMode::FolderRoleScoped) {
        score += 8;
    }
    if (node.moduleCount > 3) {
        score = score > 14 ? score - 14 : 0;
    } else if (node.moduleCount == 1) {
        score += 10;
    }
    if (node.exampleFiles.empty()) {
        score = score > 10 ? score - 10 : 0;
    }
    return std::min<std::size_t>(score, 100);
}

std::size_t computeEdgeConfidenceScore(
    const CapabilityEdge& edge,
    const CapabilityEdgeAggregate& aggregate) {
    std::size_t score = 22;
    score += std::min<std::size_t>(edge.weight * 12, 36);
    score += std::min<std::size_t>(aggregate.supportingOverviewEdgeCount * 10, 20);
    score += std::min<std::size_t>(aggregate.sourceFileSet.size() * 4, 12);
    score += std::min<std::size_t>(aggregate.symbolSet.size() * 4, 10);
    return std::min<std::size_t>(score, 100);
}


// Heuristic labels for Vibe Coder.
std::string applyVibeCoderHeuristics(CapabilityNode& node) {
    std::string evidence = node.name;
    for (const std::string& sym : node.topSymbols) {
        evidence += " " + sym;
    }
    for (const std::string& mod : node.moduleNames) {
        evidence += " " + mod;
    }
    for (const std::string& file : node.exampleFiles) {
        evidence += " " + file;
    }
    std::transform(evidence.begin(), evidence.end(), evidence.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    bool hasOnlyWebAssets = true;
    for (const std::string& file : node.exampleFiles) {
        std::string lf = file;
        std::transform(lf.begin(), lf.end(), lf.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lf.find(".css") == std::string::npos &&
            lf.find(".html") == std::string::npos &&
            lf.find(".htm") == std::string::npos) {
            hasOnlyWebAssets = false;
            break;
        }
    }
    if (!node.exampleFiles.empty() && hasOnlyWebAssets) {
        node.dominantRole = utf8Literal(u8"\U0001F5BC\uFE0F Web \u8d44\u6e90");
        node.responsibility = utf8Literal(u8"\u63d0\u4f9b\u9875\u9762\u6837\u5f0f\u4e0e\u7ed3\u6784\u6587\u4ef6\uff0c\u4e3a\u524d\u7aef\u754c\u9762\u7684\u5c55\u793a\u6548\u679c\u63d0\u4f9b\u652f\u6491\u3002");
        node.summary = node.responsibility;
        return "Heuristic label rule: the node only exposed web asset files, so it was labeled as web resources.";
    }

    bool matched = true;

    if (evidence.find("graph") != std::string::npos ||
        evidence.find("node") != std::string::npos ||
        evidence.find("edge") != std::string::npos) {
        // 鈽?淇锛氫繚鐣欒矾寰勪笂涓嬫枃锛岄槻姝㈠悓鍚嶅啿绐?
        const std::string& refMod = node.moduleNames.empty() ? node.name : node.moduleNames.front();
        const std::size_t refSep = refMod.find_last_of('/');
        const std::string context = humanize(
            refSep != std::string::npos && refSep + 1 < refMod.size()
                ? refMod.substr(refSep + 1) : refMod);
        node.name = utf8Literal(u8"\u56fe\u6570\u636e\u7ba1\u7406") + " \u2014 " + context;
        node.dominantRole = utf8Literal(u8"\U0001F9F1 \u6838\u5fc3\u6570\u636e\u7ed3\u6784");
        node.responsibility = utf8Literal(u8"\u8d1f\u8d23\u52a0\u8f7d\u3001\u7ba1\u7406\u8282\u70b9\u4e0e\u8fb9\u7b49\u56fe\u7ed3\u6784\u6570\u636e\uff0c\u662f\u540e\u7eed\u7b97\u6cd5\u548c\u4e1a\u52a1\u5206\u6790\u7684\u5e95\u5c42\u6570\u636e\u6765\u6e90\u3002");
        node.visualPriority += 240;
    } else if (evidence.find("algorithm") != std::string::npos ||
               evidence.find("astar") != std::string::npos ||
               evidence.find("a*") != std::string::npos) {
        const std::string& refModAlgo = node.moduleNames.empty() ? node.name : node.moduleNames.front();
        const std::size_t refSepAlgo = refModAlgo.find_last_of('/');
        const std::string algoContext = humanize(
            refSepAlgo != std::string::npos && refSepAlgo + 1 < refModAlgo.size()
                ? refModAlgo.substr(refSepAlgo + 1) : refModAlgo);
        node.name = utf8Literal(u8"\u8def\u5f84\u5bfb\u4f18\u7b97\u6cd5") + " \u2014 " + algoContext;
        node.dominantRole = utf8Literal(u8"\U0001F9EE \u7b97\u6cd5\u5f15\u64ce");
        node.responsibility = utf8Literal(u8"\u8d1f\u8d23\u6267\u884c\u6838\u5fc3\u5bfb\u8def\u3001\u8bc4\u5206\u6216\u4f18\u5316\u903b\u8f91\uff0c\u51b3\u5b9a\u7cfb\u7edf\u5982\u4f55\u4ece\u4e00\u5806\u53ef\u80fd\u8def\u5f84\u91cc\u627e\u5230\u6700\u4f18\u89e3\u3002");
        node.visualPriority += 220;
    } else if (!hasOnlyWebAssets &&
               (evidence.find("view") != std::string::npos || evidence.find("window") != std::string::npos ||
               evidence.find("ui") != std::string::npos || evidence.find("qml") != std::string::npos ||
               evidence.find("widget") != std::string::npos)) {
        node.dominantRole = utf8Literal(u8"\U0001F3A8 \u754c\u9762\u5c55\u793a (UI)");
        node.responsibility = utf8Literal(u8"\u8d1f\u8d23\u6e32\u67d3\u754c\u9762\u548c\u5904\u7406\u7528\u6237\u7684\u70b9\u51fb/\u6ed1\u52a8\u7b49\u64cd\u4f5c\u3002");
        node.visualPriority += 200;
    } else if (evidence.find("db") != std::string::npos || evidence.find("sql") != std::string::npos ||
               evidence.find("repo") != std::string::npos || evidence.find("storage") != std::string::npos ||
               evidence.find("database") != std::string::npos) {
        node.dominantRole = utf8Literal(u8"\U0001F4BE \u6570\u636e\u5b58\u50a8 (Database)");
        node.responsibility = utf8Literal(u8"\u8d1f\u8d23\u5c06\u6570\u636e\u6301\u4e45\u5316\u4fdd\u5b58\u5230\u672c\u5730\u6570\u636e\u5e93\u6216\u6587\u4ef6\u4e2d\u3002");
    } else if (evidence.find("net") != std::string::npos || evidence.find("http") != std::string::npos ||
               evidence.find("api") != std::string::npos || evidence.find("client") != std::string::npos ||
               evidence.find("socket") != std::string::npos) {
        node.dominantRole = utf8Literal(u8"\U0001F310 \u7f51\u7edc\u901a\u4fe1 (Network)");
        node.responsibility = utf8Literal(u8"\u8d1f\u8d23\u4e0e\u5916\u90e8\u670d\u52a1\u5668\u3001\u5927\u6a21\u578b\u6216\u7b2c\u4e09\u65b9\u63a5\u53e3\u8fdb\u884c\u6570\u636e\u4ea4\u4e92\u3002");
    } else if (evidence.find("parser") != std::string::npos || evidence.find("ast") != std::string::npos ||
               evidence.find("analyzer") != std::string::npos || evidence.find("core") != std::string::npos) {
        node.dominantRole = utf8Literal(u8"\U0001F9E0 \u6838\u5fc3\u7b97\u6cd5 (Core Engine)");
        node.responsibility = utf8Literal(u8"\u9879\u76ee\u7684\u5fc3\u810f\uff0c\u8d1f\u8d23\u6700\u6838\u5fc3\u7684\u4e1a\u52a1\u8ba1\u7b97\u3001\u6570\u636e\u5206\u6790\u6216\u903b\u8f91\u5904\u7406\u3002");
    } else if (evidence.find("util") != std::string::npos || evidence.find("helper") != std::string::npos ||
               evidence.find("const") != std::string::npos || evidence.find("common") != std::string::npos) {
        node.dominantRole = utf8Literal(u8"\U0001F527 \u8f85\u52a9\u5de5\u5177 (Utils)");
        node.responsibility = utf8Literal(u8"\u63d0\u4f9b\u901a\u7528\u7684\u8fb9\u89d2\u6599\u529f\u80fd\uff08\u5982\u65f6\u95f4\u8f6c\u6362\u3001\u5b57\u7b26\u4e32\u5904\u7406\u7b49\uff09\u3002");
        node.defaultVisible = false;
        node.visualPriority = 0;
    } else {
        matched = false;
    }

    if (matched) {
        std::ostringstream output;
        output << node.responsibility << utf8Literal(u8" \u5305\u542b\u4e86 ") << node.fileCount
               << utf8Literal(u8" \u4e2a\u6e90\u7801\u6587\u4ef6\u3002");
        if (!node.topSymbols.empty()) {
            output << utf8Literal(u8"\n\U0001F449 \u6838\u5fc3\u7c7b/\u51fd\u6570: ")
                   << joinStrings(node.topSymbols, 3);
        }
        node.summary = output.str();
    }
    if (!matched) {
        return {};
    }
    return "Heuristic label rule: node title and responsibility were refined from symbol, module, and file-name keywords.";
}
std::size_t strictL2PruningWeight(const CapabilityNode& node) {
    std::size_t weight = node.visualPriority * 100 + node.collaboratorCount * 10 + node.fileCount;
    if (node.kind == CapabilityNodeKind::Entry) {
        weight += 1000000;
    }
    return weight;
}

void applyStrictL2Pruning(CapabilityGraph& graph) {
    if (graph.nodes.size() <= 40) {
        return;
    }

    std::vector<CapabilityNode*> rankedNodes;
    rankedNodes.reserve(graph.nodes.size());
    for (CapabilityNode& node : graph.nodes) {
        if (!node.defaultVisible && node.kind != CapabilityNodeKind::Entry) {
            continue;
        }
        rankedNodes.push_back(&node);
    }
    if (rankedNodes.size() <= 40) {
        return;
    }

    std::sort(rankedNodes.begin(), rankedNodes.end(), [](const CapabilityNode* left, const CapabilityNode* right) {
        const std::size_t leftWeight = strictL2PruningWeight(*left);
        const std::size_t rightWeight = strictL2PruningWeight(*right);
        if (leftWeight != rightWeight) {
            return leftWeight > rightWeight;
        }
        return left->name < right->name;
    });

    std::unordered_set<std::size_t> keepNodeIds;
    for (std::size_t index = 0; index < 40; ++index) {
        keepNodeIds.emplace(rankedNodes[index]->id);
    }

    for (CapabilityNode& node : graph.nodes) {
        node.defaultVisible = node.kind == CapabilityNodeKind::Entry || keepNodeIds.contains(node.id);
    }
}

std::string buildRoleResponsibility(const CapabilityNode& node) {
    if (node.kind == CapabilityNodeKind::Entry) {
        if (!node.moduleNames.empty()) {
            return utf8Literal(u8"\u4f5c\u4e3a\u9879\u76ee\u7684\u542f\u52a8\u5165\u53e3\uff0c\u8d1f\u8d23\u521d\u59cb\u5316\u8fd0\u884c\u73af\u5883\u5e76\u5c06\u63a7\u5236\u6743\u4ea4\u7ed9\u6838\u5fc3\u6a21\u5757\u3002");
        }
        return utf8Literal(u8"\u9879\u76ee\u7684\u4e3b\u5165\u53e3\u6a21\u5757\uff0c\u8d1f\u8d23\u542f\u52a8\u6d41\u7a0b\u5e76\u534f\u8c03\u5404\u5b50\u7cfb\u7edf\u7684\u521d\u59cb\u5316\u3002");
    }

    if (node.kind == CapabilityNodeKind::Infrastructure) {
        if (node.dominantRole == "dependency") {
            return utf8Literal(u8"\u7b2c\u4e09\u65b9\u6216\u5916\u90e8\u4f9d\u8d56\u5e93\uff0c\u4e3a\u4e3b\u7cfb\u7edf\u63d0\u4f9b\u53ef\u590d\u7528\u7684\u57fa\u7840\u80fd\u529b\uff0c\u4e0d\u5c5e\u4e8e\u6838\u5fc3\u4e1a\u52a1\u4ee3\u7801\u3002");
        }
        if (node.dominantRole == "foundation") {
            return utf8Literal(u8"\u63d0\u4f9b\u5171\u4eab\u7684\u6570\u636e\u6a21\u578b\u3001\u63a5\u53e3\u5951\u7ea6\u548c\u9886\u57df\u7c7b\u578b\uff0c\u4f9b\u4e0a\u5c42\u6a21\u5757\u76f4\u63a5\u4f9d\u8d56\u3002");
        }
        if (node.dominantRole == "storage" || node.dominantRole == "data") {
            return utf8Literal(u8"\u8d1f\u8d23\u6301\u4e45\u5316\u5b58\u50a8\u914d\u7f6e\u3001\u8d44\u6e90\u6216\u9879\u76ee\u8fd0\u884c\u6570\u636e\uff0c\u4f9b\u5176\u4ed6\u6a21\u5757\u8bfb\u53d6\u548c\u5199\u5165\u3002");
        }
        if (node.dominantRole == "tooling") {
            return utf8Literal(u8"\u6784\u5efa\u6216\u9884\u5904\u7406\u9636\u6bb5\u7684\u8f85\u52a9\u6a21\u5757\uff0c\u5728\u4e3b\u8fd0\u884c\u65f6\u6d41\u7a0b\u4e4b\u5916\u5b8c\u6210\u8f6c\u6362\u3001\u9a8c\u8bc1\u6216\u751f\u6210\u5de5\u4f5c\u3002");
        }
        return utf8Literal(u8"\u63d0\u4f9b\u5e95\u5c42\u652f\u6491\u670d\u52a1\uff0c\u4fdd\u6301\u6838\u5fc3\u80fd\u529b\u6d41\u7a0b\u7684\u7a33\u5b9a\u6027\u548c\u53ef\u590d\u7528\u6027\u3002");
    }

    if (node.dominantRole == "experience") {
        return utf8Literal(u8"\u8d1f\u8d23\u754c\u9762\u6e32\u67d3\u548c\u7528\u6237\u4ea4\u4e92\uff0c\u5c06\u7528\u6237\u64cd\u4f5c\u8f6c\u5316\u4e3a\u7cfb\u7edf\u5185\u90e8\u7684\u4e1a\u52a1\u52a8\u4f5c\u3002");
    }
    if (node.dominantRole == "analysis") {
        return utf8Literal(u8"\u8d1f\u8d23\u6838\u5fc3\u7684\u6570\u636e\u5206\u6790\u6216\u4e1a\u52a1\u903b\u8f91\u5904\u7406\uff0c\u5c06\u8f93\u5165\u8f6c\u5316\u4e3a\u7ed3\u6784\u5316\u7684\u7ed3\u679c\u3002");
    }
    if (node.dominantRole == "visualization") {
        return utf8Literal(u8"\u5c06\u540e\u7aef\u6570\u636e\u8f6c\u5316\u4e3a\u53ef\u4f9b\u754c\u9762\u76f4\u63a5\u6e32\u67d3\u7684\u56fe\u5f62\u7ed3\u6784\u6216\u5c55\u793a\u683c\u5f0f\u3002");
    }
    if (node.dominantRole == "web") {
        return utf8Literal(u8"\u63d0\u4f9b\u57fa\u4e8e Web \u6280\u672f\u7684\u9875\u9762\u6216\u4ea4\u4e92\u80fd\u529b\uff0c\u4e0e\u684c\u9762\u4e3b\u5e94\u7528\u534f\u540c\u5de5\u4f5c\u3002");
    }
    if (node.dominantRole == "workflow") {
        return utf8Literal(u8"\u5728\u5165\u53e3\u5c42\u4e0e\u5e95\u5c42\u652f\u6491\u4e4b\u95f4\u534f\u8c03\u591a\u6b65\u9aa4\u7684\u4e1a\u52a1\u5904\u7406\u6d41\u7a0b\u3002");
    }

    return utf8Literal(u8"\u627f\u62c5\u9879\u76ee\u4e2d\u67d0\u9879\u6838\u5fc3\u4e1a\u52a1\u80fd\u529b\uff0c\u53c2\u4e0e\u4e3b\u8981\u7684\u7528\u6237\u53ef\u89c1\u5de5\u4f5c\u6d41\u3002");
}

std::string buildCapabilitySummary(const CapabilityNode& node) {
    std::ostringstream output;
    output << node.responsibility << " Covers " << node.moduleCount << " module(s) and " << node.fileCount << " file(s)";
    output << ", spans stage " << node.stageStart;
    if (node.stageEnd != node.stageStart) {
        output << "-" << node.stageEnd;
    }
    if (node.flowParticipationCount > 0) {
        output << ", and appears in " << node.flowParticipationCount << " main flow(s)";
    }
    if (node.reachableFromEntry) {
        output << ". It is reachable from an entry capability";
    }
    if (!node.folderHint.empty()) {
        output << ". Folder scope: " << humanize(node.folderHint);
    }
    output << ". Main modules: " << joinStrings(node.moduleNames, 4) << ".";
    output << " Example files: " << joinStrings(node.exampleFiles, 4) << ".";
    output << " Key symbols: " << joinStrings(node.topSymbols, 5) << ".";
    if (!node.supportingRoles.empty()) {
        output << " Supporting roles inside this cluster: " << joinStrings(node.supportingRoles, 4) << ".";
    }
    if (node.collaboratorCount > 0) {
        output << " Direct collaborators: " << joinStrings(node.collaboratorNames, 4) << ".";
    }
    if (node.defaultVisible) {
        output << " It is shown in the default view.";
    } else {
        output << " It is hidden in the default view and starts collapsed.";
    }
    return output.str();
}

CapabilityEdgeKind classifyCapabilityEdgeKind(const CapabilityNode& fromNode, const CapabilityNode& toNode) {
    if (fromNode.kind == CapabilityNodeKind::Entry) {
        return CapabilityEdgeKind::Activates;
    }
    if (toNode.kind == CapabilityNodeKind::Infrastructure) {
        return CapabilityEdgeKind::UsesInfrastructure;
    }
    return CapabilityEdgeKind::Enables;
}

std::string buildCapabilityEdgeSummary(
    const CapabilityNode& fromNode,
    const CapabilityNode& toNode,
    const CapabilityEdgeKind kind,
    const std::size_t weight) {
    std::ostringstream output;
    switch (kind) {
    case CapabilityEdgeKind::Activates:
        output << fromNode.name << " activates " << toNode.name << " as one of the first visible workflow steps";
        break;
    case CapabilityEdgeKind::Enables:
        output << fromNode.name << " hands work to " << toNode.name << " to continue the main capability flow";
        break;
    case CapabilityEdgeKind::UsesInfrastructure:
        output << fromNode.name << " relies on " << toNode.name << " for shared support during execution";
        break;
    }
    output << " (aggregated weight=" << weight << ")";
    return output.str();
}

int capabilityKindOrder(const CapabilityNodeKind kind) {
    switch (kind) {
    case CapabilityNodeKind::Entry:
        return 0;
    case CapabilityNodeKind::Capability:
        return 1;
    case CapabilityNodeKind::Infrastructure:
        return 2;
    }
    return 3;
}

std::size_t computeVisualPriority(const CapabilityNode& node) {
    std::size_t score = node.visualPriority;
    switch (node.kind) {
    case CapabilityNodeKind::Entry:
        score += 1000;
        break;
    case CapabilityNodeKind::Capability:
        score += 500;
        break;
    case CapabilityNodeKind::Infrastructure:
        score += 250;
        break;
    }

    score += std::min<std::size_t>(node.aggregatedDependencyWeight * 4, 400);
    score += node.flowParticipationCount * 120;
    score += std::min<std::size_t>(node.moduleCount * 20, 120);
    score += std::min<std::size_t>(node.fileCount * 8, 80);
    score += node.reachableFromEntry ? 60 : 0;
    score += node.defaultPinned ? 150 : 0;
    if (!node.defaultVisible && node.kind != CapabilityNodeKind::Entry) {
        score = std::min<std::size_t>(score, 40);
    }
    return score;
}

std::string buildCapabilityFlowSummary(
    const CapabilityNode& entryNode,
    const CapabilityNode& destinationNode,
    const std::size_t stageSpan,
    const std::size_t weight,
    const std::size_t hopCount) {
    std::ostringstream output;
    output << "Starts at " << entryNode.name << ", crosses " << hopCount
           << " capability step(s), spans " << stageSpan << " stage hop(s), and reaches "
           << destinationNode.name << " with aggregate weight " << weight << ".";
    return output.str();
}

std::string buildLaneGroupSummary(const CapabilityNodeKind kind, const std::size_t count, const std::size_t hiddenCount) {
    std::ostringstream output;
    output << humanize(laneNameForKind(kind)) << " containing " << count << " node(s)";
    if (hiddenCount > 0) {
        output << ", including " << hiddenCount << " hidden-by-default node(s)";
    }
    output << ".";
    return output.str();
}

std::string buildRoleGroupSummary(const std::string& role, const std::size_t count, const std::size_t hiddenCount) {
    std::ostringstream output;
    output << "Role cluster '" << role << "' containing " << count << " node(s)";
    if (hiddenCount > 0) {
        output << ", with " << hiddenCount << " collapsed in the default view";
    }
    output << ".";
    return output.str();
}

std::string buildSupportGroupSummary(const std::size_t count) {
    std::ostringstream output;
    output << "Collapsed support cluster containing " << count
           << " lower-priority node(s) that are hidden in the default view to keep the first graph readable.";
    return output.str();
}

EvidencePackage buildNodeEvidencePackage(
    const CapabilityNode& node,
    const CapabilityAggregationMode aggregationMode,
    const CapabilityNodeBuildTrace& trace,
    const std::vector<std::string>& flowNames) {
    EvidencePackage evidence;
    evidence.sourceFiles = node.exampleFiles;
    evidence.symbols = node.topSymbols;
    evidence.modules = node.moduleNames;
    evidence.relationships = node.collaboratorNames;

    evidence.facts.push_back(
        "Capability kind: " + std::string(toString(node.kind)) +
        ", dominant role: " + node.dominantRole + ".");
    evidence.facts.push_back(
        "Aggregates " + std::to_string(node.moduleCount) + " module(s) and " +
        std::to_string(node.fileCount) + " file(s); stage span " +
        std::to_string(node.stageStart) +
        (node.stageEnd != node.stageStart ? "-" + std::to_string(node.stageEnd)
                                          : std::string()) +
        ".");
    if (!node.folderHint.empty()) {
        evidence.facts.push_back("Folder scope hint: " + humanize(node.folderHint) + ".");
    }
    if (node.reachableFromEntry) {
        evidence.facts.push_back("Reachable from an entry capability in the inferred workflow.");
    }
    if (node.flowParticipationCount > 0) {
        evidence.facts.push_back(
            "Participates in " + std::to_string(node.flowParticipationCount) +
            " capability flow(s).");
    }
    if (!flowNames.empty()) {
        evidence.relationships.insert(
            evidence.relationships.end(), flowNames.begin(), flowNames.end());
        evidence.facts.push_back(
            "Related flows: " + joinStrings(flowNames, 3) + ".");
    }

    evidence.rules.push_back(
        "Aggregation rule: " + std::string(toString(aggregationMode)) +
        " grouped the underlying overview nodes into this capability.");
    if (trace.usedFolderScope && !node.folderHint.empty()) {
        evidence.rules.push_back(
            "Folder scope rule: nodes under '" + node.folderHint +
            "' were kept together before role-based aggregation.");
    }
    if (node.kind == CapabilityNodeKind::Entry) {
        evidence.rules.push_back(
            "Kind rule: entry-point overview modules are surfaced as entry capabilities.");
    } else if (node.kind == CapabilityNodeKind::Infrastructure) {
        evidence.rules.push_back(
            "Kind rule: support-oriented roles such as storage, data, tooling, dependency, or leaf-only modules become infrastructure.");
    } else {
        evidence.rules.push_back(
            "Kind rule: non-entry, non-support modules remain business-facing capabilities.");
    }
    if (!trace.roleBucket.empty()) {
        evidence.rules.push_back(
            "Role bucketing rule: overview roles were normalized into the '" +
            trace.roleBucket + "' capability bucket.");
    }
    if (node.defaultPinned) {
        evidence.rules.push_back(
            "Visibility rule: this node starts pinned in the default scene.");
    } else if (node.defaultVisible) {
        evidence.rules.push_back(
            "Visibility rule: this node stays visible in the default scene because it ranked high enough for the first-read graph.");
    } else if (trace.hiddenByStrictPruning) {
        evidence.rules.push_back(
            "Visibility rule: this node was hidden by strict L2 pruning to keep the default graph bounded.");
    } else {
        evidence.rules.push_back(
            "Visibility rule: this node starts collapsed so the default graph stays readable.");
    }
    evidence.rules.insert(
        evidence.rules.end(), trace.matchedRules.begin(), trace.matchedRules.end());

    evidence.conclusions.push_back(node.responsibility);
    evidence.conclusions.push_back(node.summary);
    if (!node.riskLevel.empty()) {
        evidence.conclusions.push_back(
            "Risk hint: level=" + node.riskLevel +
            ", score=" + std::to_string(node.riskScore) + ".");
    }

    const std::size_t confidenceScore =
        computeNodeConfidenceScore(node, aggregationMode);
    evidence.confidenceLabel = confidenceLabelFromScore(confidenceScore);
    if (evidence.confidenceLabel == "high") {
        evidence.confidenceReason =
            "The node is backed by focused files, symbols, and relatively tight aggregation.";
    } else if (evidence.confidenceLabel == "medium") {
        evidence.confidenceReason =
            "The node has usable file/symbol evidence, but some aggregation or heuristics are still involved.";
    } else {
        evidence.confidenceReason =
            "The node depends on broad aggregation or sparse symbol evidence, so the conclusion should stay conservative.";
    }
    return evidence;
}

EvidencePackage buildEdgeEvidencePackage(
    const CapabilityEdge& edge,
    const CapabilityNode& fromNode,
    const CapabilityNode& toNode,
    const CapabilityEdgeAggregate& aggregate) {
    EvidencePackage evidence;
    evidence.sourceFiles = sortedVectorFromSet(aggregate.sourceFileSet, 8);
    evidence.symbols = sortedVectorFromSet(aggregate.symbolSet, 8);
    evidence.modules = sortedVectorFromSet(aggregate.moduleSet, 8);
    evidence.relationships = sortedVectorFromSet(aggregate.relationshipSet, 8);

    evidence.facts.push_back(
        "Connects '" + fromNode.name + "' -> '" + toNode.name + "' with aggregated weight " +
        std::to_string(edge.weight) + ".");
    evidence.facts.push_back(
        "Built from " + std::to_string(aggregate.supportingOverviewEdgeCount) +
        " supporting overview dependency edge(s).");
    evidence.facts.push_back(
        "Edge kind: " + std::string(toString(edge.kind)) +
        ", visible by default: " + std::string(edge.defaultVisible ? "yes" : "no") + ".");

    if (edge.kind == CapabilityEdgeKind::Activates) {
        evidence.rules.push_back(
            "Edge kind rule: any dependency leaving an entry capability becomes 'activates'.");
    } else if (edge.kind == CapabilityEdgeKind::UsesInfrastructure) {
        evidence.rules.push_back(
            "Edge kind rule: dependencies that land on infrastructure become 'uses_infrastructure'.");
    } else {
        evidence.rules.push_back(
            "Edge kind rule: business-to-business handoffs remain 'enables'.");
    }
    evidence.rules.push_back(
        "Aggregation rule: supporting overview edges between the same capability pair were merged into one scene edge.");
    if (!edge.defaultVisible) {
        evidence.rules.push_back(
            "Visibility rule: the edge is hidden when one or both endpoint nodes start collapsed.");
    } else {
        evidence.rules.push_back(
            "Visibility rule: the edge stays visible because both endpoint nodes are visible in the default scene.");
    }

    evidence.conclusions.push_back(edge.summary);
    evidence.conclusions.push_back(
        "This relationship links " + fromNode.name + " to " + toNode.name +
        " in the current capability view.");

    const std::size_t confidenceScore =
        computeEdgeConfidenceScore(edge, aggregate);
    evidence.confidenceLabel = confidenceLabelFromScore(confidenceScore);
    if (evidence.confidenceLabel == "high") {
        evidence.confidenceReason =
            "Multiple supporting dependency edges and concrete file/symbol traces back this relationship.";
    } else if (evidence.confidenceLabel == "medium") {
        evidence.confidenceReason =
            "The relationship is supported, but only by a limited number of aggregated module dependencies.";
    } else {
        evidence.confidenceReason =
            "This relationship currently rests on sparse dependency evidence and should be treated as directional guidance.";
    }
    return evidence;
}

}  // namespace

const char* toString(const CapabilityNodeKind kind) {
    switch (kind) {
    case CapabilityNodeKind::Entry:
        return "entry";
    case CapabilityNodeKind::Capability:
        return "capability";
    case CapabilityNodeKind::Infrastructure:
        return "infrastructure";
    }

    return "unknown";
}

const char* toString(const CapabilityEdgeKind kind) {
    switch (kind) {
    case CapabilityEdgeKind::Activates:
        return "activates";
    case CapabilityEdgeKind::Enables:
        return "enables";
    case CapabilityEdgeKind::UsesInfrastructure:
        return "uses_infrastructure";
    }

    return "unknown";
}

const char* toString(const CapabilityGroupKind kind) {
    switch (kind) {
    case CapabilityGroupKind::Lane:
        return "lane";
    case CapabilityGroupKind::RoleCluster:
        return "role_cluster";
    case CapabilityGroupKind::CollapsedSupportCluster:
        return "collapsed_support_cluster";
    }

    return "unknown";
}
void applyDependencyModuleCollapse(CapabilityGraph& graph) {
    std::vector<CapabilityNode*> depNodes;
    for (CapabilityNode& depNode : graph.nodes) {
        if (depNode.dominantRole == "dependency") {
            depNodes.push_back(&depNode);
        }
    }
    if (depNodes.empty()) return;
    std::sort(depNodes.begin(), depNodes.end(),
        [](const CapabilityNode* a, const CapabilityNode* b) {
            return a->fileCount > b->fileCount;
        });
    for (CapabilityNode* depNode : depNodes) {
        if (depNode->fileCount > 20) {
            depNode->defaultVisible = false;
            depNode->defaultCollapsed = true;
            depNode->responsibility =
                utf8Literal(u8"\u7b2c\u4e09\u65b9\u4f9d\u8d56\u5e93\uff08\u5171 ")
                + std::to_string(depNode->fileCount)
                + utf8Literal(u8" \u4e2a\u6587\u4ef6\uff09\uff0c\u9ed8\u8ba4\u6298\u53e0\u4ee5\u4fdd\u6301\u56fe\u9762\u6e05\u6670\u3002");
        }
    }
}

std::size_t applyConfiguredDependencyFolds(
    CapabilityGraph& graph,
    const ProjectAnalysisConfig& config) {
    if (!config.loaded || config.dependencyFoldRules.empty()) {
        return 0;
    }

    std::size_t foldedNodeCount = 0;
    for (CapabilityNode& node : graph.nodes) {
        for (const ProjectDependencyFoldRule& rule : config.dependencyFoldRules) {
            if (!capabilityMatchesConfigPrefix(node, rule.matchPrefix)) {
                continue;
            }

            if (rule.hideByDefault) {
                node.defaultVisible = false;
            }
            if (rule.collapse) {
                node.defaultCollapsed = true;
            }
            if (node.kind != CapabilityNodeKind::Entry) {
                node.kind = CapabilityNodeKind::Infrastructure;
            }
            ++foldedNodeCount;
            break;
        }
    }

    return foldedNodeCount;
}

CapabilityGraph buildCapabilityGraph(const AnalysisReport& report, const ArchitectureOverview& overview) {
    CapabilityGraph graph;
    if (overview.nodes.empty()) {
        graph.diagnostics.push_back("No overview nodes were available, so no capability graph could be derived.");
        return graph;
    }

    const ProjectAnalysisConfig projectConfig = loadProjectAnalysisConfig(report.rootPath);
    graph.diagnostics.insert(
        graph.diagnostics.end(),
        projectConfig.diagnostics.begin(),
        projectConfig.diagnostics.end());

    if (report.semanticAnalysisRequested && !report.semanticAnalysisEnabled) {
        graph.diagnostics.push_back("Capability graph is based on fallback analysis because semantic analysis was unavailable.");
    }

    std::unordered_map<std::string, std::size_t> folderCounts;
    for (const OverviewNode& node : overview.nodes) {
        ++folderCounts[node.folderKey];
    }

    const CapabilityAggregationMode aggregationMode = chooseAggregationMode(overview);
    graph.diagnostics.push_back(std::string("Capability aggregation mode: ") + toString(aggregationMode) + ".");
    if (aggregationMode == CapabilityAggregationMode::ModuleScoped) {
        graph.diagnostics.push_back("Capability graph preserves module-level detail to stay close to the project's C4 container/component structure.");
    }

    std::unordered_map<std::size_t, const OverviewNode*> overviewNodeIndex;
    overviewNodeIndex.reserve(overview.nodes.size());
    for (const OverviewNode& node : overview.nodes) {
        overviewNodeIndex.emplace(node.id, &node);
    }

    std::unordered_map<std::string, CapabilityAggregate> aggregates;
    std::vector<std::string> aggregateOrder;
    std::unordered_map<std::size_t, std::size_t> overviewNodeToCapabilityId;
    std::unordered_map<std::size_t, CapabilityNodeBuildTrace> nodeBuildTraces;
    std::size_t nextNodeId = 1;

    for (const OverviewNode& node : overview.nodes) {
        const CapabilityNodeKind capabilityKind = classifyCapabilityKind(node);
        const std::string roleBucket = bucketRole(node, capabilityKind);
        const bool useFolderScope = aggregationMode == CapabilityAggregationMode::FolderRoleScoped &&
                                    shouldUseFolderScope(node, capabilityKind, folderCounts);
        const std::string key = clusterKeyForNode(node, capabilityKind, roleBucket, aggregationMode, useFolderScope);

        auto [iterator, inserted] = aggregates.emplace(key, CapabilityAggregate{});
        CapabilityAggregate& aggregate = iterator->second;
        if (inserted) {
            aggregate.node.id = nextNodeId++;
            aggregate.node.kind = capabilityKind;
            aggregate.node.name = clusterNameForNode(node, capabilityKind, roleBucket, aggregationMode, useFolderScope);
            aggregate.node.dominantRole = roleBucket;
            aggregate.node.stageStart = node.stageIndex;
            aggregate.node.stageEnd = node.stageIndex;
            aggregate.node.folderHint = useFolderScope ? node.folderKey : (node.folderKey != node.name ? node.folderKey : std::string{});
            if (aggregate.node.summary.empty() && !node.summary.empty()) {
                aggregate.node.summary = node.summary;
            }
            aggregateOrder.push_back(key);

            CapabilityNodeBuildTrace trace;
            trace.roleBucket = roleBucket;
            trace.usedFolderScope = useFolderScope;
            nodeBuildTraces.emplace(aggregate.node.id, std::move(trace));
        }

        overviewNodeToCapabilityId[node.id] = aggregate.node.id;
        aggregate.node.moduleCount += 1;
        aggregate.node.fileCount += node.fileCount;
        aggregate.node.stageStart = std::min(aggregate.node.stageStart, node.stageIndex);
        aggregate.node.stageEnd = std::max(aggregate.node.stageEnd, node.stageIndex);
        aggregate.node.reachableFromEntry = aggregate.node.reachableFromEntry || node.reachableFromEntry;
        aggregate.node.incomingEdgeCount += node.incomingDependencyCount;
        aggregate.node.outgoingEdgeCount += node.outgoingDependencyCount;
        aggregate.node.aggregatedDependencyWeight += node.incomingDependencyCount + node.outgoingDependencyCount;
        aggregate.roleHistogram[node.role] += 1;
        aggregate.overviewNodeIdSet.emplace(node.id);

        if (aggregate.moduleNameSet.emplace(node.name).second) {
            aggregate.node.moduleNames.push_back(node.name);
        }
        if (aggregate.supportingRoleSet.emplace(node.role).second) {
            aggregate.node.supportingRoles.push_back(node.role);
        }
        for (const std::string& symbol : node.topSymbols) {
            if (aggregate.topSymbolSet.emplace(symbol).second) {
                aggregate.node.topSymbols.push_back(symbol);
            }
        }
        for (const std::string& filePath : node.filePaths) {
            if (aggregate.exampleFileSet.emplace(filePath).second) {
                aggregate.node.exampleFiles.push_back(filePath);
            }
        }
    }

    for (const std::string& key : aggregateOrder) {
        CapabilityAggregate& aggregate = aggregates[key];
        std::sort(aggregate.node.moduleNames.begin(), aggregate.node.moduleNames.end());
        std::sort(aggregate.node.supportingRoles.begin(), aggregate.node.supportingRoles.end());
        std::sort(aggregate.node.exampleFiles.begin(), aggregate.node.exampleFiles.end());
        aggregate.node.overviewNodeIds.assign(aggregate.overviewNodeIdSet.begin(), aggregate.overviewNodeIdSet.end());
        std::sort(aggregate.node.overviewNodeIds.begin(), aggregate.node.overviewNodeIds.end());
        if (aggregate.node.topSymbols.size() > 6) {
            aggregate.node.topSymbols.resize(6);
        }
        if (aggregate.node.exampleFiles.size() > 6) {
            aggregate.node.exampleFiles.resize(6);
        }
        aggregate.node.dominantRole = dominantRoleFromHistogram(aggregate.roleHistogram);
        graph.nodes.push_back(std::move(aggregate.node));
    }

    std::sort(graph.nodes.begin(), graph.nodes.end(), [](const CapabilityNode& left, const CapabilityNode& right) {
        if (left.kind != right.kind) {
            return capabilityKindOrder(left.kind) < capabilityKindOrder(right.kind);
        }
        return left.name < right.name;
    });

    std::unordered_map<std::string, CapabilityEdgeAggregate> edgeAggregates;
    std::unordered_map<std::size_t, CapabilityEdgeAggregate> edgeEvidenceAggregates;
    std::size_t nextEdgeId = 1;
    for (const OverviewEdge& edge : overview.edges) {
        const auto fromCapabilityIt = overviewNodeToCapabilityId.find(edge.fromId);
        const auto toCapabilityIt = overviewNodeToCapabilityId.find(edge.toId);
        if (fromCapabilityIt == overviewNodeToCapabilityId.end() || toCapabilityIt == overviewNodeToCapabilityId.end()) {
            continue;
        }
        if (fromCapabilityIt->second == toCapabilityIt->second) {
            continue;
        }

        const CapabilityNode* fromNode = findCapabilityNode(graph.nodes, fromCapabilityIt->second);
        const CapabilityNode* toNode = findCapabilityNode(graph.nodes, toCapabilityIt->second);
        if (fromNode == nullptr || toNode == nullptr) {
            continue;
        }

        const std::string edgeKey = std::to_string(fromNode->id) + ">" + std::to_string(toNode->id);
        CapabilityEdgeAggregate& aggregate = edgeAggregates[edgeKey];
        CapabilityEdge& aggregateEdge = aggregate.edge;
        if (aggregateEdge.id == 0) {
            aggregateEdge.id = nextEdgeId++;
            aggregateEdge.fromId = fromNode->id;
            aggregateEdge.toId = toNode->id;
            aggregateEdge.kind = classifyCapabilityEdgeKind(*fromNode, *toNode);
            aggregateEdge.weight = 0;
        }
        aggregateEdge.weight += edge.weight;
        ++aggregate.supportingOverviewEdgeCount;

        const OverviewNode* fromOverview = nullptr;
        const OverviewNode* toOverview = nullptr;
        if (const auto overviewIt = overviewNodeIndex.find(edge.fromId);
            overviewIt != overviewNodeIndex.end()) {
            fromOverview = overviewIt->second;
        }
        if (const auto overviewIt = overviewNodeIndex.find(edge.toId);
            overviewIt != overviewNodeIndex.end()) {
            toOverview = overviewIt->second;
        }

        if (fromOverview != nullptr) {
            aggregate.moduleSet.emplace(fromOverview->name);
            for (const std::string& filePath : fromOverview->filePaths) {
                aggregate.sourceFileSet.emplace(filePath);
            }
            for (const std::string& symbol : fromOverview->topSymbols) {
                aggregate.symbolSet.emplace(symbol);
            }
        }
        if (toOverview != nullptr) {
            aggregate.moduleSet.emplace(toOverview->name);
            for (const std::string& filePath : toOverview->filePaths) {
                aggregate.sourceFileSet.emplace(filePath);
            }
            for (const std::string& symbol : toOverview->topSymbols) {
                aggregate.symbolSet.emplace(symbol);
            }
        }
        if (fromOverview != nullptr && toOverview != nullptr) {
            aggregate.relationshipSet.emplace(fromOverview->name + " -> " + toOverview->name);
        }
    }

    for (auto& [key, aggregate] : edgeAggregates) {
        static_cast<void>(key);
        edgeEvidenceAggregates.emplace(aggregate.edge.id, aggregate);
        graph.edges.push_back(std::move(aggregate.edge));
    }

    std::sort(graph.edges.begin(), graph.edges.end(), [](const CapabilityEdge& left, const CapabilityEdge& right) {
        if (left.weight != right.weight) {
            return left.weight > right.weight;
        }
        if (left.fromId != right.fromId) {
            return left.fromId < right.fromId;
        }
        return left.toId < right.toId;
    });

    std::unordered_map<std::size_t, std::unordered_set<std::string>> collaboratorSets;
    for (const CapabilityEdge& edge : graph.edges) {
        const CapabilityNode* fromNode = findCapabilityNode(graph.nodes, edge.fromId);
        const CapabilityNode* toNode = findCapabilityNode(graph.nodes, edge.toId);
        if (fromNode == nullptr || toNode == nullptr) {
            continue;
        }
        collaboratorSets[fromNode->id].emplace(toNode->name);
        collaboratorSets[toNode->id].emplace(fromNode->name);
    }
    for (CapabilityNode& node : graph.nodes) {
        const auto collaboratorIt = collaboratorSets.find(node.id);
        if (collaboratorIt == collaboratorSets.end()) {
            continue;
        }
        node.collaboratorNames.assign(collaboratorIt->second.begin(), collaboratorIt->second.end());
        std::sort(node.collaboratorNames.begin(), node.collaboratorNames.end());
        node.collaboratorCount = node.collaboratorNames.size();
    }

    std::unordered_map<std::string, std::size_t> flowSignatureToIndex;
    std::size_t nextFlowId = 1;
    for (const OverviewFlow& overviewFlow : overview.flows) {
        std::vector<std::size_t> capabilityPath;
        for (const std::size_t overviewNodeId : overviewFlow.nodeIds) {
            const auto iterator = overviewNodeToCapabilityId.find(overviewNodeId);
            if (iterator == overviewNodeToCapabilityId.end()) {
                continue;
            }
            if (capabilityPath.empty() || capabilityPath.back() != iterator->second) {
                capabilityPath.push_back(iterator->second);
            }
        }
        if (capabilityPath.size() < 2) {
            continue;
        }

        std::string signature;
        for (const std::size_t capabilityNodeId : capabilityPath) {
            if (!signature.empty()) {
                signature += ">";
            }
            signature += std::to_string(capabilityNodeId);
        }

        const auto flowIt = flowSignatureToIndex.find(signature);
        if (flowIt == flowSignatureToIndex.end()) {
            CapabilityFlow flow;
            flow.id = nextFlowId++;
            flow.nodeIds = capabilityPath;
            flow.totalWeight = overviewFlow.totalWeight;
            graph.flows.push_back(std::move(flow));
            flowSignatureToIndex.emplace(signature, graph.flows.size() - 1);
        } else {
            graph.flows[flowIt->second].totalWeight += overviewFlow.totalWeight;
        }
    }

    for (CapabilityFlow& flow : graph.flows) {
        for (const std::size_t nodeId : flow.nodeIds) {
            if (CapabilityNode* node = findCapabilityNode(graph.nodes, nodeId); node != nullptr) {
                ++node->flowParticipationCount;
            }
        }

        const CapabilityNode* entryNode = findCapabilityNode(graph.nodes, flow.nodeIds.front());
        const CapabilityNode* destinationNode = findCapabilityNode(graph.nodes, flow.nodeIds.back());
        if (entryNode == nullptr || destinationNode == nullptr) {
            continue;
        }
        flow.name = entryNode->name + " -> " + destinationNode->name;
        flow.stageSpan = destinationNode->stageEnd >= entryNode->stageStart
                             ? destinationNode->stageEnd - entryNode->stageStart
                             : 0;
        flow.summary = buildCapabilityFlowSummary(
            *entryNode,
            *destinationNode,
            flow.stageSpan,
            flow.totalWeight,
            flow.nodeIds.size());
    }

    std::sort(graph.flows.begin(), graph.flows.end(), [](const CapabilityFlow& left, const CapabilityFlow& right) {
        if (left.totalWeight != right.totalWeight) {
            return left.totalWeight > right.totalWeight;
        }
        return left.id < right.id;
    });

    std::unordered_map<std::size_t, std::vector<std::string>> flowNamesByNode;
    for (const CapabilityFlow& flow : graph.flows) {
        for (const std::size_t nodeId : flow.nodeIds) {
            flowNamesByNode[nodeId].push_back(flow.name);
        }
    }

    for (CapabilityNode& node : graph.nodes) {
        node.defaultPinned = node.kind == CapabilityNodeKind::Entry;
        if (auto traceIt = nodeBuildTraces.find(node.id); traceIt != nodeBuildTraces.end()) {
            const std::string heuristicRule = applyVibeCoderHeuristics(node);
            if (!heuristicRule.empty()) {
                traceIt->second.matchedRules.push_back(heuristicRule);
            }
        } else {
            applyVibeCoderHeuristics(node);
        }
        node.visualPriority = computeVisualPriority(node);
    }

    std::unordered_map<std::size_t, bool> prePruningVisibility;
    for (const CapabilityNode& node : graph.nodes) {
        prePruningVisibility.emplace(node.id, node.defaultVisible);
    }
    applyDependencyModuleCollapse(graph);  // 鈽?鍏堟姌鍙犲ぇ浣撻噺涓夋柟搴?
    const std::size_t configuredFoldCount = applyConfiguredDependencyFolds(graph, projectConfig);
    if (configuredFoldCount > 0) {
        graph.diagnostics.push_back(
            "Project config folded " + std::to_string(configuredFoldCount) +
            " dependency/support node(s) by default.");
    }
    applyStrictL2Pruning(graph);
    for (CapabilityNode& node : graph.nodes) {
        const auto beforeIt = prePruningVisibility.find(node.id);
        if (beforeIt != prePruningVisibility.end() && beforeIt->second &&
            !node.defaultVisible) {
            nodeBuildTraces[node.id].hiddenByStrictPruning = true;
        }
    }

    std::unordered_set<std::size_t> visibleNodeIds;
    const bool showEverythingByDefault = graph.nodes.size() <= 18 ||
                                         (aggregationMode == CapabilityAggregationMode::ModuleScoped && graph.nodes.size() <= 24);
    if (showEverythingByDefault) {
        for (CapabilityNode& node : graph.nodes) {
            if (!node.defaultVisible && node.kind != CapabilityNodeKind::Entry) {
                continue;
            }
            node.defaultVisible = true;
            if (node.kind == CapabilityNodeKind::Entry) {
                node.defaultPinned = true;
            }
            visibleNodeIds.emplace(node.id);
        }
    } else {
        for (CapabilityNode& node : graph.nodes) {
            if (node.kind == CapabilityNodeKind::Entry) {
                node.defaultVisible = true;
                visibleNodeIds.emplace(node.id);
            }
        }

        std::vector<CapabilityNode*> capabilityNodes;
        for (CapabilityNode& node : graph.nodes) {
            if (node.kind == CapabilityNodeKind::Capability && node.defaultVisible) {
                capabilityNodes.push_back(&node);
            }
        }
        std::sort(capabilityNodes.begin(), capabilityNodes.end(), [](const CapabilityNode* left, const CapabilityNode* right) {
            if (left->visualPriority != right->visualPriority) {
                return left->visualPriority > right->visualPriority;
            }
            return left->name < right->name;
        });

        const std::size_t preferredCapabilityBudget = capabilityNodes.size() <= 6 ? capabilityNodes.size() : std::min<std::size_t>(10, capabilityNodes.size());
        for (std::size_t index = 0; index < preferredCapabilityBudget; ++index) {
            capabilityNodes[index]->defaultVisible = true;
            capabilityNodes[index]->defaultPinned = capabilityNodes[index]->defaultPinned || (index < 3);
            visibleNodeIds.emplace(capabilityNodes[index]->id);
        }

        for (const CapabilityFlow& flow : graph.flows) {
            if (flow.id > 4) {
                break;
            }
            for (const std::size_t nodeId : flow.nodeIds) {
                if (CapabilityNode* node = findCapabilityNode(graph.nodes, nodeId); node != nullptr) {
                    if (!node->defaultVisible && node->kind != CapabilityNodeKind::Entry) {
                        continue;
                    }
                    if (node->kind != CapabilityNodeKind::Infrastructure || node->flowParticipationCount > 1) {
                        node->defaultVisible = true;
                        visibleNodeIds.emplace(node->id);
                    }
                }
            }
        }

        std::unordered_map<std::size_t, std::size_t> infrastructureSupportScore;
        for (const CapabilityEdge& edge : graph.edges) {
            const CapabilityNode* fromNode = findCapabilityNode(graph.nodes, edge.fromId);
            const CapabilityNode* toNode = findCapabilityNode(graph.nodes, edge.toId);
            if (fromNode == nullptr || toNode == nullptr) {
                continue;
            }

            if (toNode->kind == CapabilityNodeKind::Infrastructure && visibleNodeIds.contains(fromNode->id)) {
                infrastructureSupportScore[toNode->id] += edge.weight;
            }
            if (fromNode->kind == CapabilityNodeKind::Infrastructure && visibleNodeIds.contains(toNode->id)) {
                infrastructureSupportScore[fromNode->id] += edge.weight;
            }
        }

        std::vector<CapabilityNode*> infrastructureNodes;
        for (CapabilityNode& node : graph.nodes) {
            if (node.kind == CapabilityNodeKind::Infrastructure && node.defaultVisible) {
                infrastructureNodes.push_back(&node);
            }
        }
        std::sort(infrastructureNodes.begin(), infrastructureNodes.end(), [&](const CapabilityNode* left, const CapabilityNode* right) {
            const std::size_t leftScore = infrastructureSupportScore[left->id] + left->visualPriority;
            const std::size_t rightScore = infrastructureSupportScore[right->id] + right->visualPriority;
            if (leftScore != rightScore) {
                return leftScore > rightScore;
            }
            return left->name < right->name;
        });

        const std::size_t infrastructureBudget = infrastructureNodes.size() <= 4 ? infrastructureNodes.size() : std::min<std::size_t>(6, infrastructureNodes.size());
        for (std::size_t index = 0; index < infrastructureBudget; ++index) {
            if (infrastructureSupportScore[infrastructureNodes[index]->id] > 0 || index == 0) {
                infrastructureNodes[index]->defaultVisible = true;
                visibleNodeIds.emplace(infrastructureNodes[index]->id);
            }
        }

        if (visibleNodeIds.empty() && !graph.nodes.empty()) {
            auto fallbackNodeIt = std::find_if(graph.nodes.begin(), graph.nodes.end(), [](const CapabilityNode& node) {
                return node.kind == CapabilityNodeKind::Entry || node.defaultVisible;
            });
            if (fallbackNodeIt == graph.nodes.end()) {
                fallbackNodeIt = graph.nodes.begin();
            }
            fallbackNodeIt->defaultVisible = true;
            fallbackNodeIt->defaultPinned = true;
            visibleNodeIds.emplace(fallbackNodeIt->id);
        }
    }

    for (CapabilityNode& node : graph.nodes) {
        node.defaultVisible = visibleNodeIds.contains(node.id);

        if (node.responsibility.empty() || node.summary.empty() ||
            node.summary.find(utf8Literal(u8" \u5305\u542b\u4e86 ")) == std::string::npos) {
            node.responsibility = buildRoleResponsibility(node);
            node.summary = buildCapabilitySummary(node);
        }

        // 纭繚濡傛灉琚惎鍙戝紡寮曟搸鍒ゅ畾锟?Utils (榛樿闅愯棌) 鐨勮瘽锛屽悓姝ヨ鐩栫姸锟?
        if (!node.defaultVisible) {
            visibleNodeIds.erase(node.id);
        }

        node.defaultCollapsed = !node.defaultVisible;
        CapabilityNodeBuildTrace& trace = nodeBuildTraces[node.id];
        const auto appendTraceRule = [&](std::string rule) {
            if (std::find(trace.matchedRules.begin(), trace.matchedRules.end(), rule) == trace.matchedRules.end()) {
                trace.matchedRules.push_back(std::move(rule));
            }
        };
        for (const ProjectRoleOverrideRule& rule : projectConfig.roleOverrides) {
            if (capabilityMatchesConfigPrefix(node, rule.matchPrefix)) {
                appendTraceRule(
                    "Project config role override: '" + rule.matchPrefix +
                    "' -> role '" + rule.role + "'.");
            }
        }
        for (const ProjectEntryOverrideRule& rule : projectConfig.entryOverrides) {
            if (capabilityMatchesConfigPrefix(node, rule.matchPrefix)) {
                appendTraceRule(
                    "Project config entry override: '" + rule.matchPrefix +
                    "' -> " + std::string(rule.entry ? "entry" : "non-entry") + ".");
            }
        }
        for (const ProjectDependencyFoldRule& rule : projectConfig.dependencyFoldRules) {
            if (capabilityMatchesConfigPrefix(node, rule.matchPrefix)) {
                appendTraceRule(
                    "Project config dependency fold: '" + rule.matchPrefix +
                    "' is collapsed in the default capability scene.");
            }
        }
        auto flowNames = flowNamesByNode[node.id];
        std::sort(flowNames.begin(), flowNames.end());
        flowNames.erase(std::unique(flowNames.begin(), flowNames.end()), flowNames.end());
        node.evidence = buildNodeEvidencePackage(
            node,
            aggregationMode,
            trace,
            flowNames);
    }
    std::size_t nextGroupId = 1;
    auto makeGroup = [&](const CapabilityGroupKind kind,
                         std::string name,
                         std::string summary,
                         std::vector<std::size_t> nodeIds,
                         const bool defaultExpanded,
                         const bool defaultVisible,
                         const std::size_t hiddenNodeCount) -> std::size_t {
        CapabilityGroup group;
        group.id = nextGroupId++;
        group.kind = kind;
        group.name = std::move(name);
        group.summary = std::move(summary);
        group.nodeIds = std::move(nodeIds);
        std::sort(group.nodeIds.begin(), group.nodeIds.end());
        group.defaultExpanded = defaultExpanded;
        group.defaultVisible = defaultVisible;
        group.hiddenNodeCount = hiddenNodeCount;
        graph.groups.push_back(std::move(group));
        return graph.groups.back().id;
    };

    for (const CapabilityNodeKind kind : {CapabilityNodeKind::Entry, CapabilityNodeKind::Capability, CapabilityNodeKind::Infrastructure}) {
        std::vector<std::size_t> laneNodeIds;
        std::size_t hiddenCount = 0;
        for (const CapabilityNode& node : graph.nodes) {
            if (node.kind != kind) {
                continue;
            }
            laneNodeIds.push_back(node.id);
            if (!node.defaultVisible) {
                ++hiddenCount;
            }
        }
        if (laneNodeIds.empty()) {
            continue;
        }

        const std::size_t groupId = makeGroup(
            CapabilityGroupKind::Lane,
            humanize(laneNameForKind(kind)),
            buildLaneGroupSummary(kind, laneNodeIds.size(), hiddenCount),
            laneNodeIds,
            kind != CapabilityNodeKind::Infrastructure || hiddenCount == 0,
            true,
            hiddenCount);

        for (const std::size_t nodeId : laneNodeIds) {
            if (CapabilityNode* node = findCapabilityNode(graph.nodes, nodeId); node != nullptr) {
                node->laneGroupId = groupId;
            }
        }
    }

    std::unordered_map<std::string, std::vector<std::size_t>> roleGroups;
    for (const CapabilityNode& node : graph.nodes) {
        if (node.kind == CapabilityNodeKind::Entry) {
            continue;
        }
        roleGroups[node.dominantRole].push_back(node.id);
    }
    for (auto& [role, nodeIds] : roleGroups) {
        std::size_t hiddenCount = 0;
        std::size_t visibleCount = 0;
        for (const std::size_t nodeId : nodeIds) {
            const CapabilityNode* node = findCapabilityNode(graph.nodes, nodeId);
            if (node == nullptr) {
                continue;
            }
            if (node->defaultVisible) {
                ++visibleCount;
            } else {
                ++hiddenCount;
            }
        }

        const std::size_t groupId = makeGroup(
            CapabilityGroupKind::RoleCluster,
            humanize(role),
            buildRoleGroupSummary(role, nodeIds.size(), hiddenCount),
            nodeIds,
            hiddenCount == 0 || visibleCount <= 3,
            visibleCount > 0,
            hiddenCount);

        for (const std::size_t nodeId : nodeIds) {
            if (CapabilityNode* node = findCapabilityNode(graph.nodes, nodeId); node != nullptr) {
                node->detailGroupId = groupId;
            }
        }
    }

    std::vector<std::size_t> hiddenNodeIds;
    for (const CapabilityNode& node : graph.nodes) {
        if (!node.defaultVisible) {
            hiddenNodeIds.push_back(node.id);
        }
    }
    if (!hiddenNodeIds.empty()) {
        const std::size_t supportGroupId = makeGroup(
            CapabilityGroupKind::CollapsedSupportCluster,
            "Deferred Support Nodes",
            buildSupportGroupSummary(hiddenNodeIds.size()),
            hiddenNodeIds,
            false,
            true,
            hiddenNodeIds.size());

        for (const std::size_t nodeId : hiddenNodeIds) {
            if (CapabilityNode* node = findCapabilityNode(graph.nodes, nodeId); node != nullptr) {
                node->detailGroupId = supportGroupId;
            }
        }
    }

    for (CapabilityEdge& edge : graph.edges) {
        const CapabilityNode* fromNode = findCapabilityNode(graph.nodes, edge.fromId);
        const CapabilityNode* toNode = findCapabilityNode(graph.nodes, edge.toId);
        if (fromNode == nullptr || toNode == nullptr) {
            continue;
        }
        edge.defaultVisible = fromNode->defaultVisible && toNode->defaultVisible;
        edge.summary = buildCapabilityEdgeSummary(*fromNode, *toNode, edge.kind, edge.weight);
        if (const auto aggregateIt = edgeEvidenceAggregates.find(edge.id);
            aggregateIt != edgeEvidenceAggregates.end()) {
            edge.evidence = buildEdgeEvidencePackage(
                edge, *fromNode, *toNode, aggregateIt->second);
        }
    }

    std::sort(graph.groups.begin(), graph.groups.end(), [](const CapabilityGroup& left, const CapabilityGroup& right) {
        if (left.kind != right.kind) {
            return static_cast<int>(left.kind) < static_cast<int>(right.kind);
        }
        return left.name < right.name;
    });

    const auto hasEntryNode = std::any_of(graph.nodes.begin(), graph.nodes.end(), [](const CapabilityNode& node) {
        return node.kind == CapabilityNodeKind::Entry;
    });
    const auto hasInfrastructureNode = std::any_of(graph.nodes.begin(), graph.nodes.end(), [](const CapabilityNode& node) {
        return node.kind == CapabilityNodeKind::Infrastructure;
    });

    if (!hasEntryNode) {
        graph.diagnostics.push_back("No explicit entry capability was detected; the graph relies on inferred roots only.");
    }
    if (!hasInfrastructureNode) {
        graph.diagnostics.push_back("No infrastructure cluster was derived; the project may be mostly workflow-oriented or the current role heuristics need refinement.");
    }
    if (graph.flows.empty() && !graph.nodes.empty()) {
        graph.diagnostics.push_back("No multi-step capability flow was derived from the current module dependency graph.");
    }
    if (hiddenNodeIds.size() > graph.nodes.size() / 2 && !graph.nodes.empty()) {
        graph.diagnostics.push_back("More than half of the capability nodes were hidden in the default view; consider tightening module aggregation heuristics for this project.");
    }

    return graph;
}

std::string formatCapabilityGraph(
    const CapabilityGraph& graph,
    const std::size_t maxNodes,
    const std::size_t maxEdges,
    const std::size_t maxDiagnostics) {
    std::ostringstream output;
    output << "[capability graph]\n";
    output << "capability nodes: " << graph.nodes.size() << "\n";
    output << "capability edges: " << graph.edges.size() << "\n";
    output << "capability flows: " << graph.flows.size() << "\n";
    output << "capability groups: " << graph.groups.size() << "\n";

    const std::size_t nodeLimit = std::min(maxNodes, graph.nodes.size());
    for (std::size_t index = 0; index < nodeLimit; ++index) {
        const CapabilityNode& node = graph.nodes[index];
        output << "- [" << toString(node.kind) << "] " << node.name
               << " role=" << node.dominantRole
               << " modules=" << node.moduleCount
               << " visible=" << (node.defaultVisible ? "yes" : "no")
               << " priority=" << node.visualPriority
               << " stages=" << node.stageStart;
        if (node.stageEnd != node.stageStart) {
            output << "-" << node.stageEnd;
        }
        output << "\n";
        output << "  responsibility: " << node.responsibility << "\n";
        output << "  summary: " << node.summary << "\n";
    }
    if (graph.nodes.size() > nodeLimit) {
        output << "... " << (graph.nodes.size() - nodeLimit) << " more capability nodes\n";
    }

    output << "\n[capability edges]\n";
    const std::size_t edgeLimit = std::min(maxEdges, graph.edges.size());
    if (edgeLimit == 0) {
        output << "- none\n";
    } else {
        std::unordered_map<std::size_t, const CapabilityNode*> nodeIndex;
        for (const CapabilityNode& node : graph.nodes) {
            nodeIndex.emplace(node.id, &node);
        }
        for (std::size_t index = 0; index < edgeLimit; ++index) {
            const CapabilityEdge& edge = graph.edges[index];
            const auto fromIt = nodeIndex.find(edge.fromId);
            const auto toIt = nodeIndex.find(edge.toId);
            if (fromIt == nodeIndex.end() || toIt == nodeIndex.end()) {
                continue;
            }
            output << "- " << toString(edge.kind) << ": "
                   << fromIt->second->name << " -> " << toIt->second->name
                   << " (weight=" << edge.weight
                   << ", visible=" << (edge.defaultVisible ? "yes" : "no") << ")\n";
            output << "  summary: " << edge.summary << "\n";
        }
        if (graph.edges.size() > edgeLimit) {
            output << "... " << (graph.edges.size() - edgeLimit) << " more capability edges\n";
        }
    }

    output << "\n[capability groups]\n";
    const std::size_t groupLimit = std::min<std::size_t>(16, graph.groups.size());
    if (groupLimit == 0) {
        output << "- none\n";
    } else {
        for (std::size_t index = 0; index < groupLimit; ++index) {
            const CapabilityGroup& group = graph.groups[index];
            output << "- [" << toString(group.kind) << "] " << group.name
                   << " nodes=" << group.nodeIds.size()
                   << " expanded=" << (group.defaultExpanded ? "yes" : "no")
                   << " hidden=" << group.hiddenNodeCount << "\n";
            output << "  summary: " << group.summary << "\n";
        }
        if (graph.groups.size() > groupLimit) {
            output << "... " << (graph.groups.size() - groupLimit) << " more capability groups\n";
        }
    }

    output << "\n[capability flows]\n";
    const std::size_t flowLimit = std::min<std::size_t>(10, graph.flows.size());
    if (flowLimit == 0) {
        output << "- none\n";
    } else {
        std::unordered_map<std::size_t, const CapabilityNode*> nodeIndex;
        for (const CapabilityNode& node : graph.nodes) {
            nodeIndex.emplace(node.id, &node);
        }
        for (std::size_t index = 0; index < flowLimit; ++index) {
            const CapabilityFlow& flow = graph.flows[index];
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
        if (graph.flows.size() > flowLimit) {
            output << "... " << (graph.flows.size() - flowLimit) << " more capability flows\n";
        }
    }

    output << "\n[capability diagnostics]\n";
    const std::size_t diagnosticLimit = std::min(maxDiagnostics, graph.diagnostics.size());
    if (diagnosticLimit == 0) {
        output << "- none\n";
    } else {
        for (std::size_t index = 0; index < diagnosticLimit; ++index) {
            output << "- " << graph.diagnostics[index] << "\n";
        }
        if (graph.diagnostics.size() > diagnosticLimit) {
            output << "... " << (graph.diagnostics.size() - diagnosticLimit) << " more diagnostics\n";
        }
    }

    return output.str();
}

}  // namespace savt::core
