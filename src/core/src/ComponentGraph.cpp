#include "savt/core/ComponentGraph.h"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace savt::core {
namespace {

using EvidencePackage = CapabilityNode::EvidencePackage;

void applyVibeCoderHeuristics(ComponentNode& node);

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

std::vector<std::string> deduplicateSorted(std::vector<std::string> values, const std::size_t limit = 0) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    if (limit > 0 && values.size() > limit) {
        values.resize(limit);
    }
    return values;
}

std::string collapseSpaces(std::string value) {
    std::string output;
    output.reserve(value.size());
    bool previousWasSpace = true;
    for (const unsigned char character : value) {
        if (std::isspace(character)) {
            if (!previousWasSpace) {
                output.push_back(' ');
            }
            previousWasSpace = true;
            continue;
        }
        output.push_back(static_cast<char>(character));
        previousWasSpace = false;
    }
    if (!output.empty() && output.back() == ' ') {
        output.pop_back();
    }
    return output;
}

std::string humanizeIdentifier(std::string value) {
    std::string spaced;
    spaced.reserve(value.size() * 2);
    char previousCharacter = '\0';
    for (const char character : value) {
        if (character == '/' || character == '\\' || character == '_' ||
            character == '-' || character == '.') {
            if (!spaced.empty() && spaced.back() != ' ') {
                spaced.push_back(' ');
            }
            previousCharacter = ' ';
            continue;
        }

        const bool currentIsUpper =
            std::isupper(static_cast<unsigned char>(character)) != 0;
        const bool previousIsLowerOrDigit =
            std::islower(static_cast<unsigned char>(previousCharacter)) != 0 ||
            std::isdigit(static_cast<unsigned char>(previousCharacter)) != 0;
        if (!spaced.empty() && currentIsUpper && previousIsLowerOrDigit &&
            spaced.back() != ' ') {
            spaced.push_back(' ');
        }

        spaced.push_back(character);
        previousCharacter = character;
    }

    return collapseSpaces(spaced);
}

std::string lastPathSegment(const std::string& path) {
    const std::size_t separator = path.find_last_of("/\\");
    return separator == std::string::npos ? path : path.substr(separator + 1);
}

ComponentNodeKind classifyComponentKind(const OverviewNode& node) {
    if (node.kind == OverviewNodeKind::EntryPointModule) {
        return ComponentNodeKind::Entry;
    }
    if (node.role == "dependency" || node.role == "storage" || node.role == "data" ||
        node.role == "tooling" || node.role == "core_model" ||
        node.role == "type_centric") {
        return ComponentNodeKind::Support;
    }
    return ComponentNodeKind::Component;
}

ComponentEdgeKind classifyComponentEdgeKind(
    const ComponentNode& fromNode,
    const ComponentNode& toNode) {
    if (fromNode.kind == ComponentNodeKind::Entry) {
        return ComponentEdgeKind::Activates;
    }
    if (toNode.kind == ComponentNodeKind::Support) {
        return ComponentEdgeKind::UsesSupport;
    }
    return ComponentEdgeKind::Coordinates;
}

std::string componentResponsibility(const OverviewNode& node, const ComponentNodeKind kind) {
    if (!node.summary.empty()) {
        return node.summary;
    }
    if (kind == ComponentNodeKind::Entry) {
        return "负责发起当前能力域内部的主流程。";
    }
    if (kind == ComponentNodeKind::Support) {
        return "为当前能力域提供支撑性的数据、工具或基础设施能力。";
    }
    return "承担当前能力域中的具体业务组件职责。";
}

std::size_t componentVisualPriority(const ComponentNode& node) {
    std::size_t score = node.incomingEdgeCount + node.outgoingEdgeCount;
    score += node.fileCount * 3;
    score += node.kind == ComponentNodeKind::Entry ? 50 : 0;
    score += node.kind == ComponentNodeKind::Support ? 5 : 15;
    score += node.reachableFromEntry ? 12 : 0;
    return score;
}

std::string confidenceLabel(const std::size_t score) {
    if (score >= 70) {
        return "high";
    }
    if (score >= 40) {
        return "medium";
    }
    return "low";
}

EvidencePackage buildNodeEvidence(
    const ComponentNode& node,
    const ComponentGraph& graph) {
    EvidencePackage evidence;
    evidence.sourceFiles = node.exampleFiles;
    evidence.symbols = node.topSymbols;
    evidence.modules = node.moduleNames;
    evidence.relationships = node.collaboratorNames;

    evidence.facts.push_back(
        "Belongs to capability '" + graph.capabilityName + "' and maps to " +
        std::to_string(node.overviewNodeIds.size()) + " overview node(s).");
    evidence.facts.push_back(
        "Stage " + std::to_string(node.stageIndex) + " (" + node.stageName + "), files=" +
        std::to_string(node.fileCount) + ", incoming=" +
        std::to_string(node.incomingEdgeCount) + ", outgoing=" +
        std::to_string(node.outgoingEdgeCount) + ".");
    if (!node.primaryFilePath.empty()) {
        evidence.facts.push_back("Primary file path: " + node.primaryFilePath + ".");
    }
    if (!node.folderHint.empty()) {
        evidence.facts.push_back("Folder scope: " + node.folderHint + ".");
    }
    if (node.reachableFromEntry) {
        evidence.facts.push_back("Reachable from the capability's internal entry path.");
    }
    if (node.cycleParticipant) {
        evidence.facts.push_back("Participates in an internal dependency cycle.");
    }

    evidence.rules.push_back(
        "Membership rule: only overview modules aggregated into the selected capability are allowed into this L3 graph.");
    evidence.rules.push_back(
        "Stage rule: the component inherits its stage index from the overview dependency ordering.");
    if (node.fileCluster) {
        evidence.rules.push_back(
            "File cluster rule: files sharing the same path stem are treated as one file-level component and expose a primary file path.");
    }
    if (node.kind == ComponentNodeKind::Entry) {
        evidence.rules.push_back(
            "Kind rule: entry-point overview modules become entry components.");
    } else if (node.kind == ComponentNodeKind::Support) {
        evidence.rules.push_back(
            "Kind rule: support-oriented roles such as storage, data, tooling, dependency, or type-centric modules become support components.");
    } else {
        evidence.rules.push_back(
            "Kind rule: remaining overview modules stay as business-facing components.");
    }

    evidence.conclusions.push_back(node.responsibility);
    evidence.conclusions.push_back(node.summary);

    std::size_t confidenceScore = 0;
    confidenceScore += std::min<std::size_t>(node.fileCount * 10, 30);
    confidenceScore += std::min<std::size_t>(node.topSymbols.size() * 8, 24);
    confidenceScore += std::min<std::size_t>(node.collaboratorNames.size() * 5, 20);
    confidenceScore += node.kind == ComponentNodeKind::Entry ? 15 : 0;
    evidence.confidenceLabel = confidenceLabel(confidenceScore);
    if (evidence.confidenceLabel == "high") {
        evidence.confidenceReason =
            "This component is backed by focused files, symbols, and explicit internal dependencies.";
    } else if (evidence.confidenceLabel == "medium") {
        evidence.confidenceReason =
            "This component has enough local file and symbol evidence to be useful, but some inference is still involved.";
    } else {
        evidence.confidenceReason =
            "This component currently relies on sparse internal evidence and should be treated as a conservative structural hint.";
    }
    return evidence;
}

EvidencePackage buildEdgeEvidence(
    const ComponentEdge& edge,
    const ComponentNode& fromNode,
    const ComponentNode& toNode,
    const std::vector<std::string>& sourceFiles,
    const std::vector<std::string>& symbols,
    const std::vector<std::string>& modules) {
    EvidencePackage evidence;
    evidence.sourceFiles = sourceFiles;
    evidence.symbols = symbols;
    evidence.modules = modules;
    evidence.relationships = {fromNode.name + " -> " + toNode.name};

    evidence.facts.push_back(
        "Internal dependency weight " + std::to_string(edge.weight) +
        " from '" + fromNode.name + "' to '" + toNode.name + "'.");
    evidence.facts.push_back(
        "Both endpoints belong to the selected capability, so the edge remains visible inside the L3 graph.");

    evidence.rules.push_back(
        "Membership rule: only internal overview dependencies whose two endpoints stay inside the selected capability are shown in L3.");
    if (edge.kind == ComponentEdgeKind::Activates) {
        evidence.rules.push_back(
            "Edge kind rule: dependencies leaving an entry component become 'activates'.");
    } else if (edge.kind == ComponentEdgeKind::UsesSupport) {
        evidence.rules.push_back(
            "Edge kind rule: dependencies landing on support components become 'uses_support'.");
    } else {
        evidence.rules.push_back(
            "Edge kind rule: business-to-business handoffs remain 'coordinates'.");
    }

    evidence.conclusions.push_back(edge.summary);
    evidence.confidenceLabel = confidenceLabel(
        std::min<std::size_t>(
            20 + edge.weight * 15 + sourceFiles.size() * 3 + symbols.size() * 3, 100));
    if (evidence.confidenceLabel == "high") {
        evidence.confidenceReason =
            "This relationship is backed by multiple internal dependency signals and concrete file/symbol traces.";
    } else if (evidence.confidenceLabel == "medium") {
        evidence.confidenceReason =
            "This relationship is supported by internal dependency edges, but the trace set is still modest.";
    } else {
        evidence.confidenceReason =
            "This relationship is inferred from a small number of internal dependency signals.";
    }
    return evidence;
}

const CapabilityNode* findCapabilityNodeById(
    const CapabilityGraph& graph,
    const std::size_t capabilityId) {
    const auto iterator = std::find_if(
        graph.nodes.begin(), graph.nodes.end(), [&](const CapabilityNode& node) {
            return node.id == capabilityId;
        });
    return iterator == graph.nodes.end() ? nullptr : &(*iterator);
}

const OverviewNode* findOverviewNodeById(
    const ArchitectureOverview& overview,
    const std::size_t nodeId) {
    const auto iterator = std::find_if(
        overview.nodes.begin(), overview.nodes.end(), [&](const OverviewNode& node) {
            return node.id == nodeId;
        });
    return iterator == overview.nodes.end() ? nullptr : &(*iterator);
}

std::string edgeSummary(
    const ComponentNode& fromNode,
    const ComponentNode& toNode,
    const ComponentEdgeKind kind,
    const std::size_t weight) {
    std::ostringstream output;
    switch (kind) {
    case ComponentEdgeKind::Activates:
        output << fromNode.name << " activates " << toNode.name;
        break;
    case ComponentEdgeKind::Coordinates:
        output << fromNode.name << " coordinates with " << toNode.name;
        break;
    case ComponentEdgeKind::UsesSupport:
        output << fromNode.name << " uses support from " << toNode.name;
        break;
    }
    output << " (weight=" << weight << ")";
    return output.str();
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
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

bool isCppImplementationExtension(const std::string& extension) {
    return extension == ".cpp" || extension == ".cc" || extension == ".cxx" ||
           extension == ".cp" || extension == ".c++" || extension == ".cppm" ||
           extension == ".ixx" || extension == ".cu" || extension == ".mm" ||
           extension == ".c";
}

bool isCppHeaderOrInlineExtension(const std::string& extension) {
    return extension == ".h" || extension == ".hh" || extension == ".hpp" ||
           extension == ".hxx" || extension == ".h++" ||
           extension == ".ipp" || extension == ".inl" || extension == ".tpp" ||
           extension == ".tcc" || extension == ".txx" || extension == ".ii" ||
           extension == ".cuh";
}

bool isCppFamilyExtension(const std::string& extension) {
    return isCppImplementationExtension(extension) || isCppHeaderOrInlineExtension(extension);
}

int primaryFilePathRank(const std::string& path) {
    const std::string extension = lowerFileExtension(path);
    if (isCppImplementationExtension(extension)) {
        return 0;
    }
    if (extension == ".qml" || extension == ".js" || extension == ".mjs" ||
        extension == ".cjs" || extension == ".ts" || extension == ".tsx" ||
        extension == ".mts" || extension == ".cts" || extension == ".py" ||
        extension == ".java" || extension == ".kt" || extension == ".kts" ||
        extension == ".go" || extension == ".rs") {
        return 1;
    }
    if (extension == ".html" || extension == ".htm") {
        return 2;
    }
    if (isCppHeaderOrInlineExtension(extension)) {
        return 3;
    }
    if (extension == ".css" || extension == ".scss") {
        return 4;
    }
    if (extension == ".json" || extension == ".yaml" || extension == ".yml" ||
        extension == ".toml" || extension == ".xml") {
        return 5;
    }
    if (extension == ".md") {
        return 6;
    }
    return 10;
}

std::string selectPrimaryFilePath(std::vector<std::string> paths) {
    paths = deduplicateSorted(std::move(paths));
    if (paths.empty()) {
        return {};
    }

    std::sort(paths.begin(), paths.end(), [](const std::string& left, const std::string& right) {
        const int leftRank = primaryFilePathRank(left);
        const int rightRank = primaryFilePathRank(right);
        if (leftRank != rightRank) {
            return leftRank < rightRank;
        }
        return left < right;
    });
    return paths.front();
}

std::string filePathWithoutExtension(const std::string& path) {
    const std::size_t separator = path.find_last_of('.');
    return separator == std::string::npos ? path : path.substr(0, separator);
}

std::string fileStem(const std::string& path) {
    const std::size_t separator = path.find_last_of("/\\");
    const std::string fileName =
        separator == std::string::npos ? path : path.substr(separator + 1);
    const std::size_t extensionSeparator = fileName.find_last_of('.');
    return extensionSeparator == std::string::npos
               ? fileName
               : fileName.substr(0, extensionSeparator);
}

std::string parentFolderName(const std::string& path) {
    const std::size_t separator = path.find_last_of("/\\");
    if (separator == std::string::npos) {
        return {};
    }
    const std::string parentPath = path.substr(0, separator);
    const std::size_t parentSeparator = parentPath.find_last_of("/\\");
    return parentSeparator == std::string::npos
               ? parentPath
               : parentPath.substr(parentSeparator + 1);
}

std::vector<std::string> splitPathSegments(const std::string& path) {
    std::vector<std::string> segments;
    std::string current;
    current.reserve(path.size());
    for (const char character : path) {
        if (character == '/' || character == '\\') {
            if (!current.empty()) {
                segments.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(character);
    }
    if (!current.empty()) {
        segments.push_back(current);
    }
    return segments;
}

std::string joinedPathTail(
    const std::vector<std::string>& segments,
    const std::size_t count) {
    if (segments.empty() || count == 0) {
        return {};
    }

    const std::size_t start =
        segments.size() > count ? segments.size() - count : 0;
    std::ostringstream output;
    for (std::size_t index = start; index < segments.size(); ++index) {
        if (index > start) {
            output << " / ";
        }
        output << humanizeIdentifier(segments[index]);
    }
    return collapseSpaces(output.str());
}

bool looksLikeMojibakeText(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    static const std::initializer_list<const char*> tokens = {
        "�", "锟", "鑱", "鏂", "鍙", "璁", "浠", "銆", "锛", "鏈", "娴"};
    const std::string lowered = toLower(value);
    return std::any_of(tokens.begin(), tokens.end(), [&](const char* token) {
        return lowered.find(token) != std::string::npos;
    });
}

std::string cleanDisplayContext(const std::string& value) {
    const std::string collapsed = collapseSpaces(value);
    if (collapsed.empty()) {
        return {};
    }
    if (looksLikeMojibakeText(collapsed)) {
        return {};
    }
    return collapsed;
}

std::string cleanStageName(const std::string& value) {
    const std::string cleaned = cleanDisplayContext(value);
    if (cleaned.empty()) {
        return {};
    }

    const std::string lowered = toLower(cleaned);
    if (lowered == "entry" || lowered == "core" || lowered == "leaf" ||
        lowered == "support" || lowered == "internal") {
        return lowered;
    }
    return cleaned;
}

std::string preferredComponentScope(const ComponentNode& node) {
    for (const std::string& filePath : node.exampleFiles) {
        const std::vector<std::string> segments = splitPathSegments(filePath);
        if (segments.size() > 1) {
            const std::string scope = cleanDisplayContext(joinedPathTail(segments, 2));
            if (!scope.empty()) {
                return scope;
            }
        }
    }

    if (!node.folderHint.empty()) {
        const std::vector<std::string> segments = splitPathSegments(node.folderHint);
        const std::string scope = cleanDisplayContext(joinedPathTail(segments, 2));
        if (!scope.empty()) {
            return scope;
        }
    }

    for (const std::string& moduleName : node.moduleNames) {
        const std::string scope =
            cleanDisplayContext(humanizeIdentifier(moduleName));
        if (!scope.empty()) {
            return scope;
        }
    }

    return {};
}

std::vector<std::string> componentNameCandidates(const ComponentNode& node) {
    std::vector<std::string> candidates;
    std::unordered_set<std::string> seen;

    auto addCandidate = [&](const std::string& candidate) {
        const std::string normalized = cleanDisplayContext(candidate);
        if (normalized.empty()) {
            return;
        }
        const std::string lowered = toLower(normalized);
        if (!seen.insert(lowered).second) {
            return;
        }
        candidates.push_back(normalized);
    };

    addCandidate(node.name);

    if (!node.scopeLabel.empty()) {
        addCandidate(node.scopeLabel + " · " + node.name);
    }

    if (!node.folderHint.empty()) {
        const std::vector<std::string> segments = splitPathSegments(node.folderHint);
        addCandidate(joinedPathTail(segments, 1) + " · " + node.name);
        addCandidate(joinedPathTail(segments, 2) + " · " + node.name);
        addCandidate(joinedPathTail(segments, 3) + " · " + node.name);
    }

    for (const std::string& moduleName : node.moduleNames) {
        const std::string moduleScope = collapseSpaces(humanizeIdentifier(moduleName));
        if (!moduleScope.empty()) {
            addCandidate(moduleScope + " · " + node.name);
        }
    }

    if (!node.stageName.empty()) {
        addCandidate(collapseSpaces(humanizeIdentifier(node.stageName)) + " · " + node.name);
    }

    for (const std::string& filePath : node.exampleFiles) {
        const std::vector<std::string> segments = splitPathSegments(filePath);
        if (segments.size() <= 1) {
            continue;
        }
        addCandidate(joinedPathTail(segments, 2) + " · " + node.name);
        addCandidate(joinedPathTail(segments, 3) + " · " + node.name);
    }

    return candidates;
}

std::string componentIdentityKey(const ComponentNode& node) {
    std::ostringstream output;
    output << static_cast<int>(node.kind) << "|"
           << collapseSpaces(toLower(node.scopeLabel)) << "|"
           << collapseSpaces(toLower(node.stageName)) << "|";
    for (const std::string& moduleName : node.moduleNames) {
        output << collapseSpaces(toLower(moduleName)) << ",";
    }
    output << "|";
    const std::size_t fileLimit = std::min<std::size_t>(node.exampleFiles.size(), 3);
    for (std::size_t index = 0; index < fileLimit; ++index) {
        output << collapseSpaces(toLower(node.exampleFiles[index])) << ",";
    }
    return output.str();
}

void resolveComponentNameConflicts(ComponentGraph& graph) {
    std::unordered_map<std::string, std::vector<ComponentNode*>> nodesByBaseName;
    for (ComponentNode& node : graph.nodes) {
        nodesByBaseName[toLower(collapseSpaces(node.name))].push_back(&node);
    }

    for (auto& [baseKey, group] : nodesByBaseName) {
        static_cast<void>(baseKey);
        if (group.size() <= 1) {
            continue;
        }

        std::unordered_map<ComponentNode*, std::vector<std::string>> candidatesByNode;
        std::unordered_map<std::string, std::size_t> candidateUsage;
        for (ComponentNode* node : group) {
            std::vector<std::string> candidates = componentNameCandidates(*node);
            for (const std::string& candidate : candidates) {
                candidateUsage[toLower(candidate)] += 1;
            }
            candidatesByNode.emplace(node, std::move(candidates));
        }

        std::vector<ComponentNode*> unresolved;
        unresolved.reserve(group.size());
        for (ComponentNode* node : group) {
            unresolved.push_back(node);
            const auto candidateIt = candidatesByNode.find(node);
            if (candidateIt == candidatesByNode.end()) {
                continue;
            }
            for (const std::string& candidate : candidateIt->second) {
                if (candidateUsage[toLower(candidate)] == 1) {
                    node->name = candidate;
                    unresolved.pop_back();
                    break;
                }
            }
        }

        std::sort(unresolved.begin(), unresolved.end(), [](const ComponentNode* left, const ComponentNode* right) {
            return componentIdentityKey(*left) < componentIdentityKey(*right);
        });

        for (std::size_t index = 0; index < unresolved.size(); ++index) {
            ComponentNode* node = unresolved[index];
            const auto candidateIt = candidatesByNode.find(node);
            std::string fallbackBase = node->name;
            if (candidateIt != candidatesByNode.end() && !candidateIt->second.empty()) {
                fallbackBase = candidateIt->second.back();
            }
            node->name = fallbackBase + " #" + std::to_string(index + 1);
        }
    }
}

bool containsAnyToken(
    const std::string& text,
    const std::initializer_list<const char*> tokens) {
    const std::string lowered = toLower(text);
    return std::any_of(tokens.begin(), tokens.end(), [&](const char* token) {
        return lowered.find(token) != std::string::npos;
    });
}

bool isEntryLikeFilePath(const std::string& path) {
    return containsAnyToken(
        path,
        {"/main.cpp", "/main.cc", "/main.cxx", "/main.cp", "/main.c++",
         "/main.cppm", "/main.ixx", "/main.mm", "/main.c", "/main.qml", "/app.qml",
         "/mainwindow.cpp", "/mainwindow.qml"});
}

bool isEntryLikeSymbol(const SymbolNode& node) {
    const std::string lowered = toLower(node.displayName);
    return lowered == "main" || lowered == "qmlmain" || lowered == "winmain" ||
           lowered == "wwinmain" || lowered == "mainwindow";
}

bool looksSupportLike(const std::string& evidenceText) {
    return containsAnyToken(
        evidenceText,
        {"storage", "store", "repo", "repository", "cache", "config", "setting",
         "schema", "model", "type", "util", "helper", "shared", "adapter",
         "tool", "snapshot"});
}

void applyVibeCoderHeuristics(ComponentNode& node) {
    auto fileStemFromPath = [](const std::string& path) {
        std::string fileName = lastPathSegment(path);
        const std::size_t extensionSeparator = fileName.find_last_of('.');
        if (extensionSeparator != std::string::npos) {
            fileName = fileName.substr(0, extensionSeparator);
        }
        return fileName;
    };

    auto firstContext = [&]() {
        if (node.fileCluster && !node.primaryFilePath.empty()) {
            return humanizeIdentifier(fileStemFromPath(node.primaryFilePath));
        }
        if (!node.folderHint.empty()) {
            return humanizeIdentifier(node.folderHint);
        }
        if (!node.exampleFiles.empty()) {
            return humanizeIdentifier(fileStemFromPath(node.exampleFiles.front()));
        }
        return humanizeIdentifier(node.name);
    };

    std::string evidence = node.name + " " + node.role + " " + node.stageName;
    for (const std::string& file : node.exampleFiles) {
        evidence += " " + file;
    }
    for (const std::string& symbol : node.topSymbols) {
        evidence += " " + symbol;
    }
    evidence = toLower(evidence);

    const std::string context = collapseSpaces(firstContext());
    const bool summaryLooksMechanical =
        node.summary.empty() ||
        node.summary.find("代表文件") != std::string::npos ||
        node.summary.find("+ more") != std::string::npos ||
        node.summary.find('/') != std::string::npos ||
        node.summary.find('\\') != std::string::npos ||
        node.responsibility.find("围绕") != std::string::npos;

    auto replaceNodeText = [&](std::string label,
                               std::string responsibility,
                               const bool mentionFiles = true) {
        label = collapseSpaces(std::move(label));
        responsibility = collapseSpaces(std::move(responsibility));
        if (!label.empty()) {
            node.name = label;
        }
        if (!responsibility.empty()) {
            node.responsibility = responsibility;
        }

        std::ostringstream summary;
        summary << node.responsibility;
        if (mentionFiles && !node.exampleFiles.empty()) {
            summary << " 可以先从 " << joinStrings(node.exampleFiles, 2) << " 看起。";
        } else if (node.kind == ComponentNodeKind::Entry) {
            summary << " 这通常是继续理解该能力块时最适合先看的位置。";
        }
        node.summary = summary.str();
    };

    if (node.kind == ComponentNodeKind::Entry ||
        containsAnyToken(evidence, {"main", "app", "shell", "window", "entry"})) {
        replaceNodeText(
            context.empty() ? "入口协调" : context + " 入口协调",
            "负责接住当前能力块的入口动作，把用户或系统发起的请求交给后续模块继续处理。它更像流程起点，不负责把所有细节都做完。",
            false);
        return;
    }

    if (containsAnyToken(evidence, {"qml", "ui", "view", "page", "window", "widget"})) {
        replaceNodeText(
            context.empty() ? "界面交互" : context + " 界面",
            "负责展示界面、接收用户操作，并把这些操作转换成后续处理需要的信息。它主要解决人机交互，不负责底层解析和数据计算。");
        return;
    }

    if (containsAnyToken(evidence, {"parser", "tree-sitter", "treesitter", "ast", "analyzer", "parse"})) {
        replaceNodeText(
            context.empty() ? "结构解析" : context + " 解析与结构提取",
            "负责把源码或输入内容整理成结构化结果，供后续分析、映射或展示继续使用。它产出的是可继续加工的中间结构，而不是最终界面结果。");
        return;
    }

    if (containsAnyToken(evidence, {"graph", "node", "edge", "layout", "scene"})) {
        replaceNodeText(
            context.empty() ? "图结构整理" : context + " 图结构整理",
            "负责组织节点、连线或布局信息，让后续页面和分析流程能够读取统一的结构数据。它更像结构搬运和整理层，不直接决定业务结论。");
        return;
    }

    if (containsAnyToken(evidence, {"report", "markdown", "export", "summary"})) {
        replaceNodeText(
            context.empty() ? "结果输出" : context + " 结果输出",
            "负责把分析结果整理成报告、说明文字或可复制的输出内容，帮助用户更快阅读和继续提问。它主要负责表达结果，而不是生成底层分析事实。");
        return;
    }

    if (containsAnyToken(evidence, {"ai", "deepseek", "prompt", "model", "assistant"})) {
        replaceNodeText(
            context.empty() ? "AI 解释接入" : context + " AI 解释接入",
            "负责把已有分析结果交给大模型生成解释、总结或下一步建议。它依赖前面的结构化证据，本身不负责底层源码解析。");
        return;
    }

    if (node.kind == ComponentNodeKind::Support || looksSupportLike(evidence)) {
        replaceNodeText(
            context.empty() ? "共享支撑" : context + " 共享支撑",
            "负责提供配置、缓存、通用工具或共享数据支撑，帮助主流程更稳定地工作。它通常服务于别的模块，而不是直接面向最终操作。");
        return;
    }

    if (containsAnyToken(evidence, {"test", "tests", "spec", "benchmark"})) {
        replaceNodeText(
            context.empty() ? "验证与回归" : context + " 验证与回归",
            "负责验证关键流程是否正常、结果是否稳定，帮助团队发现回归和行为变化。它主要承担校验职责，而不是业务主流程。");
        return;
    }

    if (!summaryLooksMechanical) {
        return;
    }

    replaceNodeText(
        context.empty() ? "内部处理单元" : context + " 内部处理单元",
        "负责承担该能力块中的一段内部处理工作，把上游信息继续整理、计算或转交给下游模块。它是能力块内部的执行拼图之一。");
}

std::string resolveRawNodeFilePath(const SymbolNode& node) {
    if (node.kind == SymbolKind::File) {
        return node.qualifiedName;
    }
    return node.filePath;
}

std::pair<std::size_t, std::string> componentStageForKind(
    const ComponentNodeKind kind,
    const OverviewNode& fallbackOverview) {
    switch (kind) {
    case ComponentNodeKind::Entry:
        return {0, "entry"};
    case ComponentNodeKind::Support:
        return {2, "support"};
    case ComponentNodeKind::Component:
        break;
    }

    const std::string fallbackStageName = cleanStageName(fallbackOverview.stageName);
    if (!fallbackStageName.empty()) {
        return {1, fallbackStageName};
    }
    return {1, "internal"};
}

void assignStageGroups(ComponentGraph& graph) {
    graph.groups.clear();
    std::unordered_map<std::string, std::size_t> stageGroupIds;
    std::size_t nextGroupId = 1;

    for (const ComponentNode& node : graph.nodes) {
        const std::string stageKey =
            std::to_string(node.stageIndex) + ":" + node.stageName;
        if (!stageGroupIds.contains(stageKey)) {
            ComponentGroup group;
            group.id = nextGroupId++;
            group.kind = ComponentGroupKind::Stage;
            group.name = node.stageName.empty()
                             ? ("Stage " + std::to_string(node.stageIndex))
                             : node.stageName;
            group.summary =
                "Internal components grouped by the same dependency stage inside the selected capability.";
            group.stageIndex = node.stageIndex;
            graph.groups.push_back(std::move(group));
            stageGroupIds.emplace(stageKey, graph.groups.back().id);
        }
    }

    for (ComponentNode& node : graph.nodes) {
        const std::string stageKey =
            std::to_string(node.stageIndex) + ":" + node.stageName;
        node.stageGroupId = stageGroupIds[stageKey];
        auto groupIt = std::find_if(
            graph.groups.begin(), graph.groups.end(), [&](const ComponentGroup& group) {
                return group.id == node.stageGroupId;
            });
        if (groupIt != graph.groups.end()) {
            groupIt->nodeIds.push_back(node.id);
        }
    }
}

void finalizeComponentGraph(
    ComponentGraph& graph,
    std::unordered_map<std::size_t, std::vector<std::string>> collaboratorSets) {
    for (ComponentNode& node : graph.nodes) {
        auto collaborators = collaboratorSets[node.id];
        collaborators = deduplicateSorted(std::move(collaborators), 8);
        node.collaboratorNames = std::move(collaborators);
        node.stageName = cleanStageName(node.stageName);
        applyVibeCoderHeuristics(node);
        node.scopeLabel = preferredComponentScope(node);
        if (node.scopeLabel.empty()) {
            node.scopeLabel = collapseSpaces(humanizeIdentifier(node.stageName));
        }
        if (collapseSpaces(toLower(node.scopeLabel)) == collapseSpaces(toLower(node.name))) {
            node.scopeLabel.clear();
        }
    }

    resolveComponentNameConflicts(graph);

    for (ComponentNode& node : graph.nodes) {
        node.visualPriority = componentVisualPriority(node);
        node.evidence = buildNodeEvidence(node, graph);
    }

    std::sort(graph.edges.begin(), graph.edges.end(), [](const ComponentEdge& left, const ComponentEdge& right) {
        if (left.weight != right.weight) {
            return left.weight > right.weight;
        }
        if (left.fromId != right.fromId) {
            return left.fromId < right.fromId;
        }
        return left.toId < right.toId;
    });

    std::sort(graph.nodes.begin(), graph.nodes.end(), [](const ComponentNode& left, const ComponentNode& right) {
        if (left.stageIndex != right.stageIndex) {
            return left.stageIndex < right.stageIndex;
        }
        if (left.defaultPinned != right.defaultPinned) {
            return left.defaultPinned;
        }
        if (left.visualPriority != right.visualPriority) {
            return left.visualPriority > right.visualPriority;
        }
        return left.name < right.name;
    });

    std::sort(graph.groups.begin(), graph.groups.end(), [](const ComponentGroup& left, const ComponentGroup& right) {
        if (left.stageIndex != right.stageIndex) {
            return left.stageIndex < right.stageIndex;
        }
        return left.name < right.name;
    });

    if (graph.nodes.empty()) {
        graph.diagnostics.push_back("No internal overview modules were available for the selected capability.");
    } else if (graph.nodes.size() == 1) {
        graph.diagnostics.push_back("Selected capability currently maps to a single internal component.");
    }
    if (graph.edges.empty() && graph.nodes.size() > 1) {
        graph.diagnostics.push_back("Internal components were derived, but no internal handoff edges were found.");
    }
}

ComponentGraph buildOverviewBackedComponentGraph(
    const ArchitectureOverview& overview,
    const CapabilityNode& capabilityNode) {
    ComponentGraph graph;
    graph.capabilityId = capabilityNode.id;
    graph.capabilityName = capabilityNode.name;
    graph.capabilitySummary = capabilityNode.summary;

    std::unordered_set<std::size_t> selectedOverviewIds(
        capabilityNode.overviewNodeIds.begin(), capabilityNode.overviewNodeIds.end());
    std::unordered_map<std::size_t, std::size_t> overviewToComponentId;
    std::unordered_map<std::size_t, std::vector<std::string>> collaboratorSets;

    std::size_t nextNodeId = 1;
    for (const std::size_t overviewNodeId : capabilityNode.overviewNodeIds) {
        const OverviewNode* overviewNode = findOverviewNodeById(overview, overviewNodeId);
        if (overviewNode == nullptr) {
            continue;
        }

        ComponentNode node;
        node.id = nextNodeId++;
        node.capabilityId = capabilityNode.id;
        node.kind = classifyComponentKind(*overviewNode);
        node.name = overviewNode->name;
        node.role = overviewNode->role;
        node.stageIndex = overviewNode->stageIndex;
        node.stageName = overviewNode->stageName;
        node.folderHint = overviewNode->folderKey;
        node.overviewNodeIds = {overviewNode->id};
        node.moduleNames = {overviewNode->name};
        node.exampleFiles = deduplicateSorted(overviewNode->filePaths, 8);
        node.topSymbols = overviewNode->topSymbols;
        node.fileCount = overviewNode->fileCount;
        node.sourceFileCount = overviewNode->sourceFileCount;
        node.qmlFileCount = overviewNode->qmlFileCount;
        node.webFileCount = overviewNode->webFileCount;
        node.scriptFileCount = overviewNode->scriptFileCount;
        node.dataFileCount = overviewNode->dataFileCount;
        node.incomingEdgeCount = overviewNode->incomingDependencyCount;
        node.outgoingEdgeCount = overviewNode->outgoingDependencyCount;
        node.reachableFromEntry = overviewNode->reachableFromEntry;
        node.cycleParticipant = overviewNode->cycleParticipant;
        node.defaultPinned = node.kind == ComponentNodeKind::Entry;
        node.summary = overviewNode->summary;
        node.responsibility = componentResponsibility(*overviewNode, node.kind);

        overviewToComponentId.emplace(overviewNode->id, node.id);
        graph.nodes.push_back(std::move(node));
    }

    assignStageGroups(graph);

    struct EdgeAggregate {
        ComponentEdge edge;
        std::unordered_set<std::string> sourceFileSet;
        std::unordered_set<std::string> symbolSet;
        std::unordered_set<std::string> moduleSet;
    };

    std::unordered_map<std::string, EdgeAggregate> edgeAggregates;
    std::size_t nextEdgeId = 1;
    for (const OverviewEdge& overviewEdge : overview.edges) {
        if (!selectedOverviewIds.contains(overviewEdge.fromId) ||
            !selectedOverviewIds.contains(overviewEdge.toId)) {
            continue;
        }

        const auto fromIt = overviewToComponentId.find(overviewEdge.fromId);
        const auto toIt = overviewToComponentId.find(overviewEdge.toId);
        if (fromIt == overviewToComponentId.end() || toIt == overviewToComponentId.end() ||
            fromIt->second == toIt->second) {
            continue;
        }

        const auto fromNodeIt = std::find_if(
            graph.nodes.begin(), graph.nodes.end(), [&](const ComponentNode& node) {
                return node.id == fromIt->second;
            });
        const auto toNodeIt = std::find_if(
            graph.nodes.begin(), graph.nodes.end(), [&](const ComponentNode& node) {
                return node.id == toIt->second;
            });
        if (fromNodeIt == graph.nodes.end() || toNodeIt == graph.nodes.end()) {
            continue;
        }

        const std::string edgeKey =
            std::to_string(fromIt->second) + ">" + std::to_string(toIt->second);
        EdgeAggregate& aggregate = edgeAggregates[edgeKey];
        if (aggregate.edge.id == 0) {
            aggregate.edge.id = nextEdgeId++;
            aggregate.edge.fromId = fromIt->second;
            aggregate.edge.toId = toIt->second;
            aggregate.edge.kind = classifyComponentEdgeKind(*fromNodeIt, *toNodeIt);
            aggregate.edge.weight = 0;
        }
        aggregate.edge.weight += overviewEdge.weight;

        const OverviewNode* fromOverview = findOverviewNodeById(overview, overviewEdge.fromId);
        const OverviewNode* toOverview = findOverviewNodeById(overview, overviewEdge.toId);
        if (fromOverview != nullptr) {
            aggregate.moduleSet.emplace(fromOverview->name);
            for (const std::string& file : fromOverview->filePaths) {
                aggregate.sourceFileSet.emplace(file);
            }
            for (const std::string& symbol : fromOverview->topSymbols) {
                aggregate.symbolSet.emplace(symbol);
            }
        }
        if (toOverview != nullptr) {
            aggregate.moduleSet.emplace(toOverview->name);
            for (const std::string& file : toOverview->filePaths) {
                aggregate.sourceFileSet.emplace(file);
            }
            for (const std::string& symbol : toOverview->topSymbols) {
                aggregate.symbolSet.emplace(symbol);
            }
        }
    }

    for (auto& [key, aggregate] : edgeAggregates) {
        static_cast<void>(key);
        auto fromNodeIt = std::find_if(
            graph.nodes.begin(), graph.nodes.end(), [&](const ComponentNode& node) {
                return node.id == aggregate.edge.fromId;
            });
        auto toNodeIt = std::find_if(
            graph.nodes.begin(), graph.nodes.end(), [&](const ComponentNode& node) {
                return node.id == aggregate.edge.toId;
            });
        if (fromNodeIt == graph.nodes.end() || toNodeIt == graph.nodes.end()) {
            continue;
        }

        aggregate.edge.summary =
            edgeSummary(*fromNodeIt, *toNodeIt, aggregate.edge.kind, aggregate.edge.weight);
        aggregate.edge.evidence = buildEdgeEvidence(
            aggregate.edge,
            *fromNodeIt,
            *toNodeIt,
            deduplicateSorted(std::vector<std::string>(
                aggregate.sourceFileSet.begin(), aggregate.sourceFileSet.end()), 8),
            deduplicateSorted(std::vector<std::string>(
                aggregate.symbolSet.begin(), aggregate.symbolSet.end()), 8),
            deduplicateSorted(std::vector<std::string>(
                aggregate.moduleSet.begin(), aggregate.moduleSet.end()), 8));

        fromNodeIt->outgoingEdgeCount += aggregate.edge.weight;
        toNodeIt->incomingEdgeCount += aggregate.edge.weight;
        collaboratorSets[aggregate.edge.fromId].push_back(toNodeIt->name);
        collaboratorSets[aggregate.edge.toId].push_back(fromNodeIt->name);
        graph.edges.push_back(std::move(aggregate.edge));
    }

    finalizeComponentGraph(graph, std::move(collaboratorSets));
    return graph;
}

ComponentGraph buildSingleOverviewFallbackComponentGraph(
    const AnalysisReport& report,
    const OverviewNode& overviewNode,
    const CapabilityNode& capabilityNode) {
    struct FileClusterAggregate {
        std::size_t id = 0;
        std::string key;
        std::string name;
        std::string folderHint;
        std::vector<std::size_t> rawNodeIds;
        std::unordered_set<std::string> filePathSet;
        std::unordered_set<std::string> symbolSet;
        std::unordered_set<std::string> moduleSet;
        std::size_t fileCount = 0;
        std::size_t sourceFileCount = 0;
        std::size_t qmlFileCount = 0;
        std::size_t webFileCount = 0;
        std::size_t scriptFileCount = 0;
        std::size_t dataFileCount = 0;
        bool entrySignal = false;
        bool supportSignal = false;
    };

    ComponentGraph graph;
    graph.capabilityId = capabilityNode.id;
    graph.capabilityName = capabilityNode.name;
    graph.capabilitySummary = capabilityNode.summary;

    std::unordered_set<std::size_t> rawNodeIdSet(
        overviewNode.rawNodeIds.begin(), overviewNode.rawNodeIds.end());
    if (rawNodeIdSet.empty()) {
        graph.diagnostics.push_back("Single-module fallback could not run because raw node ids were not available.");
        return graph;
    }

    std::unordered_map<std::size_t, const SymbolNode*> reportNodeIndex;
    reportNodeIndex.reserve(report.nodes.size());
    for (const SymbolNode& node : report.nodes) {
        reportNodeIndex.emplace(node.id, &node);
    }

    std::unordered_map<std::string, FileClusterAggregate> clusters;
    std::unordered_map<std::size_t, std::size_t> rawNodeToComponentId;
    std::size_t nextNodeId = 1;

    for (const std::size_t rawNodeId : overviewNode.rawNodeIds) {
        const auto nodeIt = reportNodeIndex.find(rawNodeId);
        if (nodeIt == reportNodeIndex.end()) {
            continue;
        }
        const SymbolNode& rawNode = *nodeIt->second;
        const std::string filePath = resolveRawNodeFilePath(rawNode);
        if (filePath.empty()) {
            continue;
        }

        const std::string clusterKey = filePathWithoutExtension(filePath);
        auto [clusterIt, inserted] = clusters.emplace(clusterKey, FileClusterAggregate{});
        FileClusterAggregate& cluster = clusterIt->second;
        if (inserted) {
            cluster.id = nextNodeId++;
            cluster.key = clusterKey;
            cluster.name = fileStem(filePath);
            cluster.folderHint = parentFolderName(filePath);
        }

        cluster.rawNodeIds.push_back(rawNodeId);
        rawNodeToComponentId.emplace(rawNodeId, cluster.id);

        if (cluster.filePathSet.emplace(filePath).second) {
            ++cluster.fileCount;
            const std::string extension = lowerFileExtension(filePath);
            if (extension == ".qml") {
                ++cluster.qmlFileCount;
            } else if (extension == ".html" || extension == ".htm" || extension == ".css") {
                ++cluster.webFileCount;
            } else if (extension == ".py" || extension == ".js" || extension == ".mjs" ||
                       extension == ".cjs" || extension == ".ts" || extension == ".tsx" ||
                       extension == ".mts" || extension == ".cts") {
                ++cluster.scriptFileCount;
            } else if (extension == ".json") {
                ++cluster.dataFileCount;
            } else if (isCppFamilyExtension(extension)) {
                ++cluster.sourceFileCount;
            }
        }

        if (rawNode.kind != SymbolKind::File && rawNode.kind != SymbolKind::Module &&
            !rawNode.displayName.empty()) {
            cluster.symbolSet.emplace(rawNode.displayName);
        }
        cluster.moduleSet.emplace(overviewNode.name);
        cluster.entrySignal = cluster.entrySignal || isEntryLikeFilePath(filePath) ||
                              isEntryLikeSymbol(rawNode);
        cluster.supportSignal = cluster.supportSignal ||
                                looksSupportLike(filePath + " " + rawNode.displayName + " " +
                                                 rawNode.qualifiedName);
    }

    if (clusters.size() <= 1) {
        graph.diagnostics.push_back("Single-module fallback did not find enough file clusters to expand this capability.");
        return graph;
    }

    std::unordered_map<std::string, std::size_t> clusterNameCounts;
    for (const auto& [key, cluster] : clusters) {
        static_cast<void>(key);
        clusterNameCounts[cluster.name] += 1;
    }

    for (auto& [key, cluster] : clusters) {
        static_cast<void>(key);
        if (clusterNameCounts[cluster.name] > 1 && !cluster.folderHint.empty()) {
            cluster.name = cluster.folderHint + "/" + cluster.name;
        }

        ComponentNode node;
        node.id = cluster.id;
        node.capabilityId = capabilityNode.id;
        node.kind = cluster.entrySignal
                        ? ComponentNodeKind::Entry
                        : (cluster.supportSignal ? ComponentNodeKind::Support
                                                 : ComponentNodeKind::Component);
        node.name = cluster.name;
        node.role = node.kind == ComponentNodeKind::Support ? "support" : overviewNode.role;
        const auto [stageIndex, stageName] =
            componentStageForKind(node.kind, overviewNode);
        node.stageIndex = stageIndex;
        node.stageName = stageName;
        node.folderHint = cluster.folderHint;
        node.overviewNodeIds = {overviewNode.id};
        node.moduleNames = deduplicateSorted(
            std::vector<std::string>(cluster.moduleSet.begin(), cluster.moduleSet.end()), 4);
        node.exampleFiles = deduplicateSorted(
            std::vector<std::string>(cluster.filePathSet.begin(), cluster.filePathSet.end()), 8);
        node.primaryFilePath = selectPrimaryFilePath(
            std::vector<std::string>(cluster.filePathSet.begin(), cluster.filePathSet.end()));
        node.fileCluster = !node.primaryFilePath.empty();
        node.topSymbols = deduplicateSorted(
            std::vector<std::string>(cluster.symbolSet.begin(), cluster.symbolSet.end()), 8);
        node.fileCount = cluster.fileCount;
        node.sourceFileCount = cluster.sourceFileCount;
        node.qmlFileCount = cluster.qmlFileCount;
        node.webFileCount = cluster.webFileCount;
        node.scriptFileCount = cluster.scriptFileCount;
        node.dataFileCount = cluster.dataFileCount;
        node.reachableFromEntry = overviewNode.reachableFromEntry || cluster.entrySignal;
        node.cycleParticipant = overviewNode.cycleParticipant;
        node.defaultPinned = node.kind == ComponentNodeKind::Entry;
        if (node.kind == ComponentNodeKind::Entry) {
            node.responsibility = "负责发起当前能力域内部的入口流程。";
        } else if (node.kind == ComponentNodeKind::Support) {
            node.responsibility = "为当前能力域提供共享支撑能力。";
        } else if (!node.topSymbols.empty()) {
            node.responsibility = "围绕 " + joinStrings(node.topSymbols, 3) + " 承担内部实现职责。";
        } else {
            node.responsibility = "承担当前能力域中的一部分内部实现职责。";
        }
        node.summary =
            node.responsibility + " 代表文件: " + joinStrings(node.exampleFiles, 3) + ".";
        graph.nodes.push_back(std::move(node));
    }

    assignStageGroups(graph);

    struct EdgeAggregate {
        ComponentEdge edge;
        std::unordered_set<std::string> sourceFileSet;
        std::unordered_set<std::string> symbolSet;
        std::unordered_set<std::string> moduleSet;
    };

    std::unordered_map<std::string, EdgeAggregate> edgeAggregates;
    std::unordered_map<std::size_t, std::vector<std::string>> collaboratorSets;
    std::size_t nextEdgeId = 1;
    for (const SymbolEdge& reportEdge : report.edges) {
        if (reportEdge.kind == EdgeKind::Contains) {
            continue;
        }

        const auto fromIt = rawNodeToComponentId.find(reportEdge.fromId);
        const auto toIt = rawNodeToComponentId.find(reportEdge.toId);
        if (fromIt == rawNodeToComponentId.end() || toIt == rawNodeToComponentId.end() ||
            fromIt->second == toIt->second) {
            continue;
        }

        auto fromNodeIt = std::find_if(
            graph.nodes.begin(), graph.nodes.end(), [&](const ComponentNode& node) {
                return node.id == fromIt->second;
            });
        auto toNodeIt = std::find_if(
            graph.nodes.begin(), graph.nodes.end(), [&](const ComponentNode& node) {
                return node.id == toIt->second;
            });
        if (fromNodeIt == graph.nodes.end() || toNodeIt == graph.nodes.end()) {
            continue;
        }

        const std::string edgeKey =
            std::to_string(fromIt->second) + ">" + std::to_string(toIt->second);
        EdgeAggregate& aggregate = edgeAggregates[edgeKey];
        if (aggregate.edge.id == 0) {
            aggregate.edge.id = nextEdgeId++;
            aggregate.edge.fromId = fromIt->second;
            aggregate.edge.toId = toIt->second;
            aggregate.edge.kind = classifyComponentEdgeKind(*fromNodeIt, *toNodeIt);
            aggregate.edge.weight = 0;
        }
        aggregate.edge.weight += std::max<std::size_t>(1, reportEdge.weight);

        if (const auto fromRawNodeIt = reportNodeIndex.find(reportEdge.fromId);
            fromRawNodeIt != reportNodeIndex.end()) {
            const SymbolNode& rawNode = *fromRawNodeIt->second;
            if (!resolveRawNodeFilePath(rawNode).empty()) {
                aggregate.sourceFileSet.emplace(resolveRawNodeFilePath(rawNode));
            }
            if (!rawNode.displayName.empty()) {
                aggregate.symbolSet.emplace(rawNode.displayName);
            }
        }
        if (const auto toRawNodeIt = reportNodeIndex.find(reportEdge.toId);
            toRawNodeIt != reportNodeIndex.end()) {
            const SymbolNode& rawNode = *toRawNodeIt->second;
            if (!resolveRawNodeFilePath(rawNode).empty()) {
                aggregate.sourceFileSet.emplace(resolveRawNodeFilePath(rawNode));
            }
            if (!rawNode.displayName.empty()) {
                aggregate.symbolSet.emplace(rawNode.displayName);
            }
        }
        aggregate.moduleSet.emplace(overviewNode.name);
    }

    for (auto& [key, aggregate] : edgeAggregates) {
        static_cast<void>(key);
        auto fromNodeIt = std::find_if(
            graph.nodes.begin(), graph.nodes.end(), [&](const ComponentNode& node) {
                return node.id == aggregate.edge.fromId;
            });
        auto toNodeIt = std::find_if(
            graph.nodes.begin(), graph.nodes.end(), [&](const ComponentNode& node) {
                return node.id == aggregate.edge.toId;
            });
        if (fromNodeIt == graph.nodes.end() || toNodeIt == graph.nodes.end()) {
            continue;
        }

        aggregate.edge.summary =
            edgeSummary(*fromNodeIt, *toNodeIt, aggregate.edge.kind, aggregate.edge.weight);
        aggregate.edge.evidence = buildEdgeEvidence(
            aggregate.edge,
            *fromNodeIt,
            *toNodeIt,
            deduplicateSorted(std::vector<std::string>(
                aggregate.sourceFileSet.begin(), aggregate.sourceFileSet.end()), 8),
            deduplicateSorted(std::vector<std::string>(
                aggregate.symbolSet.begin(), aggregate.symbolSet.end()), 8),
            deduplicateSorted(std::vector<std::string>(
                aggregate.moduleSet.begin(), aggregate.moduleSet.end()), 8));

        fromNodeIt->outgoingEdgeCount += aggregate.edge.weight;
        toNodeIt->incomingEdgeCount += aggregate.edge.weight;
        collaboratorSets[aggregate.edge.fromId].push_back(toNodeIt->name);
        collaboratorSets[aggregate.edge.toId].push_back(fromNodeIt->name);
        graph.edges.push_back(std::move(aggregate.edge));
    }

    graph.diagnostics.push_back(
        "Expanded a single-module capability into file-level component clusters.");
    finalizeComponentGraph(graph, std::move(collaboratorSets));
    return graph;
}

ComponentGraph buildComponentGraphForCapabilityImpl(
    const AnalysisReport* report,
    const ArchitectureOverview& overview,
    const CapabilityGraph& capabilityGraph,
    const std::size_t capabilityId) {
    ComponentGraph graph;
    graph.capabilityId = capabilityId;

    const CapabilityNode* capabilityNode = findCapabilityNodeById(capabilityGraph, capabilityId);
    if (capabilityNode == nullptr) {
        graph.diagnostics.push_back("Selected capability was not found in the capability graph.");
        return graph;
    }

    graph.capabilityName = capabilityNode->name;
    graph.capabilitySummary = capabilityNode->summary;
    if (capabilityNode->overviewNodeIds.empty()) {
        graph.diagnostics.push_back("Selected capability does not expose overview node ids, so no internal component graph can be derived.");
        return graph;
    }

    if (report != nullptr && capabilityNode->overviewNodeIds.size() == 1) {
        const OverviewNode* singleOverview =
            findOverviewNodeById(overview, capabilityNode->overviewNodeIds.front());
        if (singleOverview != nullptr) {
            ComponentGraph fallbackGraph = buildSingleOverviewFallbackComponentGraph(
                *report, *singleOverview, *capabilityNode);
            if (fallbackGraph.nodes.size() > 1 || fallbackGraph.edges.size() > 0) {
                return fallbackGraph;
            }
        }
    }

    return buildOverviewBackedComponentGraph(overview, *capabilityNode);
}

}  // namespace

const char* toString(const ComponentNodeKind kind) {
    switch (kind) {
    case ComponentNodeKind::Entry:
        return "entry_component";
    case ComponentNodeKind::Component:
        return "component";
    case ComponentNodeKind::Support:
        return "support_component";
    }
    return "unknown";
}

const char* toString(const ComponentEdgeKind kind) {
    switch (kind) {
    case ComponentEdgeKind::Activates:
        return "activates";
    case ComponentEdgeKind::Coordinates:
        return "coordinates";
    case ComponentEdgeKind::UsesSupport:
        return "uses_support";
    }
    return "unknown";
}

const char* toString(const ComponentGroupKind kind) {
    switch (kind) {
    case ComponentGroupKind::Stage:
        return "stage";
    }
    return "unknown";
}

ComponentGraph buildComponentGraphForCapability(
    const ArchitectureOverview& overview,
    const CapabilityGraph& capabilityGraph,
    const std::size_t capabilityId) {
    return buildComponentGraphForCapabilityImpl(
        nullptr, overview, capabilityGraph, capabilityId);
}

ComponentGraph buildComponentGraphForCapability(
    const AnalysisReport& report,
    const ArchitectureOverview& overview,
    const CapabilityGraph& capabilityGraph,
    const std::size_t capabilityId) {
    return buildComponentGraphForCapabilityImpl(
        &report, overview, capabilityGraph, capabilityId);
}

}  // namespace savt::core
