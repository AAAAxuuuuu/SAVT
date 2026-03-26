#include "savt/core/ArchitectureGraph.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace savt::core {
namespace {

std::size_t countNodesByKind(const AnalysisReport& report, const SymbolKind kind) {
    return static_cast<std::size_t>(
        std::count_if(report.nodes.begin(), report.nodes.end(), [kind](const SymbolNode& node) {
            return node.kind == kind;
        }));
}

std::size_t countEdgesByKind(const AnalysisReport& report, const EdgeKind kind) {
    return static_cast<std::size_t>(
        std::count_if(report.edges.begin(), report.edges.end(), [kind](const SymbolEdge& edge) {
            return edge.kind == kind;
        }));
}

std::size_t countNodesByFactSource(const AnalysisReport& report, const FactSource source) {
    return static_cast<std::size_t>(
        std::count_if(report.nodes.begin(), report.nodes.end(), [source](const SymbolNode& node) {
            return node.factSource == source;
        }));
}

std::size_t countEdgesByFactSource(const AnalysisReport& report, const FactSource source) {
    return static_cast<std::size_t>(
        std::count_if(report.edges.begin(), report.edges.end(), [source](const SymbolEdge& edge) {
            return edge.factSource == source;
        }));
}

std::string escapeJson(const std::string& value) {
    std::ostringstream output;
    for (const char c : value) {
        switch (c) {
        case '\\': output << "\\\\"; break;
        case '"': output << "\\\""; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default: output << c; break;
        }
    }
    return output.str();
}

std::string escapeDot(const std::string& value) {
    std::ostringstream output;
    for (const char c : value) {
        switch (c) {
        case '\\': output << "\\\\"; break;
        case '"': output << "\\\""; break;
        case '\n': output << "\\n"; break;
        default: output << c; break;
        }
    }
    return output.str();
}

bool shouldIncludeNodeInDot(const SymbolNode& node, const bool modulesOnly) {
    return !modulesOnly || node.kind == SymbolKind::Module;
}

bool shouldIncludeEdgeInDot(const SymbolEdge& edge, const bool modulesOnly, const AnalysisReport& report) {
    if (!modulesOnly) {
        return true;
    }

    if (edge.kind != EdgeKind::DependsOn) {
        return false;
    }

    const SymbolNode* from = findNodeById(report, edge.fromId);
    const SymbolNode* to = findNodeById(report, edge.toId);
    return from != nullptr && to != nullptr &&
           from->kind == SymbolKind::Module && to->kind == SymbolKind::Module;
}

}  // namespace

const char* toString(const SymbolKind kind) {
    switch (kind) {
    case SymbolKind::Module:
        return "module";
    case SymbolKind::File:
        return "file";
    case SymbolKind::Namespace:
        return "namespace";
    case SymbolKind::Class:
        return "class";
    case SymbolKind::Struct:
        return "struct";
    case SymbolKind::Enum:
        return "enum";
    case SymbolKind::Enumerator:
        return "enumerator";
    case SymbolKind::Field:
        return "field";
    case SymbolKind::Function:
        return "function";
    case SymbolKind::Method:
        return "method";
    }

    return "unknown";
}

const char* toString(const EdgeKind kind) {
    switch (kind) {
    case EdgeKind::Contains:
        return "contains";
    case EdgeKind::Calls:
        return "calls";
    case EdgeKind::Includes:
        return "includes";
    case EdgeKind::Inherits:
        return "inherits";
    case EdgeKind::UsesType:
        return "uses_type";
    case EdgeKind::Overrides:
        return "overrides";
    case EdgeKind::DependsOn:
        return "depends_on";
    }

    return "unknown";
}

const char* toString(const FactSource source) {
    switch (source) {
    case FactSource::Inferred:
        return "inferred";
    case FactSource::Semantic:
        return "semantic";
    }

    return "unknown";
}

const SymbolNode* findNodeById(const AnalysisReport& report, const std::size_t nodeId) {
    const auto iterator = std::find_if(report.nodes.begin(), report.nodes.end(), [nodeId](const SymbolNode& node) {
        return node.id == nodeId;
    });
    return iterator == report.nodes.end() ? nullptr : &(*iterator);
}

std::string formatAnalysisReport(
    const AnalysisReport& report,
    const std::size_t maxNodes,
    const std::size_t maxEdges,
    const std::size_t maxDiagnostics) {
    std::ostringstream output;

    output << "root: " << report.rootPath << "\n";
    output << "engine: " << (report.primaryEngine.empty() ? "unknown" : report.primaryEngine) << "\n";
    output << "precision: " << (report.precisionLevel.empty() ? "unknown" : report.precisionLevel) << "\n";
    output << "semantic status: "
           << (report.semanticStatusCode.empty() ? "unknown" : report.semanticStatusCode) << "\n";
    if (!report.semanticStatusMessage.empty()) {
        output << "semantic detail: " << report.semanticStatusMessage << "\n";
    }
    output << "semantic analysis: " << (report.semanticAnalysisEnabled ? "enabled" : "disabled") << "\n";
    output << "semantic requested: " << (report.semanticAnalysisRequested ? "yes" : "no") << "\n";
    output << "compilation database: "
           << (report.compilationDatabasePath.empty() ? "not found" : report.compilationDatabasePath) << "\n";
    output << "files: " << report.parsedFiles << " parsed / " << report.discoveredFiles << " discovered\n";
    output << "nodes: " << report.nodes.size() << "\n";
    output << "  modules: " << countNodesByKind(report, SymbolKind::Module) << "\n";
    output << "  files: " << countNodesByKind(report, SymbolKind::File) << "\n";
    output << "  namespaces: " << countNodesByKind(report, SymbolKind::Namespace) << "\n";
    output << "  classes: " << countNodesByKind(report, SymbolKind::Class) << "\n";
    output << "  structs: " << countNodesByKind(report, SymbolKind::Struct) << "\n";
    output << "  enums: " << countNodesByKind(report, SymbolKind::Enum) << "\n";
    output << "  enumerators: " << countNodesByKind(report, SymbolKind::Enumerator) << "\n";
    output << "  fields: " << countNodesByKind(report, SymbolKind::Field) << "\n";
    output << "  functions: " << countNodesByKind(report, SymbolKind::Function) << "\n";
    output << "  methods: " << countNodesByKind(report, SymbolKind::Method) << "\n";
    output << "  inferred facts: " << countNodesByFactSource(report, FactSource::Inferred) << "\n";
    output << "  semantic facts: " << countNodesByFactSource(report, FactSource::Semantic) << "\n";
    output << "edges: " << report.edges.size() << "\n";
    output << "  contains: " << countEdgesByKind(report, EdgeKind::Contains) << "\n";
    output << "  calls: " << countEdgesByKind(report, EdgeKind::Calls) << "\n";
    output << "  includes: " << countEdgesByKind(report, EdgeKind::Includes) << "\n";
    output << "  inherits: " << countEdgesByKind(report, EdgeKind::Inherits) << "\n";
    output << "  uses_type: " << countEdgesByKind(report, EdgeKind::UsesType) << "\n";
    output << "  overrides: " << countEdgesByKind(report, EdgeKind::Overrides) << "\n";
    output << "  depends_on: " << countEdgesByKind(report, EdgeKind::DependsOn) << "\n";
    output << "  inferred facts: " << countEdgesByFactSource(report, FactSource::Inferred) << "\n";
    output << "  semantic facts: " << countEdgesByFactSource(report, FactSource::Semantic) << "\n";

    output << "\n[node sample]\n";
    const std::size_t nodeLimit = std::min(maxNodes, report.nodes.size());
    for (std::size_t index = 0; index < nodeLimit; ++index) {
        const SymbolNode& node = report.nodes[index];
        output << "- [" << toString(node.factSource) << "] [" << toString(node.kind) << "] " << node.qualifiedName
               << " @ " << node.filePath << ":" << (node.line + 1) << "\n";
    }
    if (report.nodes.size() > nodeLimit) {
        output << "... " << (report.nodes.size() - nodeLimit) << " more nodes\n";
    }

    std::unordered_map<std::size_t, const SymbolNode*> nodeIndex;
    nodeIndex.reserve(report.nodes.size());
    for (const SymbolNode& node : report.nodes) {
        nodeIndex.emplace(node.id, &node);
    }

    output << "\n[edge sample]\n";
    const std::size_t edgeLimit = std::min(maxEdges, report.edges.size());
    for (std::size_t index = 0; index < edgeLimit; ++index) {
        const SymbolEdge& edge = report.edges[index];
        const auto fromIt = nodeIndex.find(edge.fromId);
        const auto toIt = nodeIndex.find(edge.toId);
        if (fromIt == nodeIndex.end() || toIt == nodeIndex.end()) {
            continue;
        }

        output << "- [" << toString(edge.factSource) << "] " << toString(edge.kind) << ": "
               << fromIt->second->qualifiedName << " -> " << toIt->second->qualifiedName;
        if (edge.weight > 1) {
            output << " (weight=" << edge.weight << ")";
        }
        if (edge.supportCount > 1) {
            output << " (support=" << edge.supportCount << ")";
        }
        output << "\n";
    }
    if (report.edges.size() > edgeLimit) {
        output << "... " << (report.edges.size() - edgeLimit) << " more edges\n";
    }

    output << "\n[module dependency sample]\n";
    std::size_t shownDependencies = 0;
    for (const SymbolEdge& edge : report.edges) {
        if (edge.kind != EdgeKind::DependsOn) {
            continue;
        }

        const auto fromIt = nodeIndex.find(edge.fromId);
        const auto toIt = nodeIndex.find(edge.toId);
        if (fromIt == nodeIndex.end() || toIt == nodeIndex.end()) {
            continue;
        }

        output << "- [" << toString(edge.factSource) << "] " << fromIt->second->displayName
               << " -> " << toIt->second->displayName
               << " (weight=" << edge.weight << ")";
        if (edge.supportCount > 1) {
            output << " (support=" << edge.supportCount << ")";
        }
        output << "\n";
        ++shownDependencies;
        if (shownDependencies >= std::min<std::size_t>(20, maxEdges)) {
            break;
        }
    }
    if (shownDependencies == 0) {
        output << "- none\n";
    }

    output << "\n[diagnostics]\n";
    const std::size_t diagnosticLimit = std::min(maxDiagnostics, report.diagnostics.size());
    if (diagnosticLimit == 0) {
        output << "- none\n";
    } else {
        for (std::size_t index = 0; index < diagnosticLimit; ++index) {
            output << "- " << report.diagnostics[index] << "\n";
        }
        if (report.diagnostics.size() > diagnosticLimit) {
            output << "... " << (report.diagnostics.size() - diagnosticLimit) << " more diagnostics\n";
        }
    }

    return output.str();
}

std::string serializeAnalysisReportToJson(const AnalysisReport& report, const bool pretty) {
    const std::string indent = pretty ? "  " : "";
    const std::string newline = pretty ? "\n" : "";

    std::ostringstream output;
    output << "{" << newline;
    output << indent << "\"rootPath\": \"" << escapeJson(report.rootPath) << "\"," << newline;
    output << indent << "\"engine\": \"" << escapeJson(report.primaryEngine) << "\"," << newline;
    output << indent << "\"precisionLevel\": \"" << escapeJson(report.precisionLevel) << "\"," << newline;
    output << indent << "\"semanticStatusCode\": \"" << escapeJson(report.semanticStatusCode) << "\"," << newline;
    output << indent << "\"semanticStatusMessage\": \"" << escapeJson(report.semanticStatusMessage) << "\"," << newline;
    output << indent << "\"compilationDatabasePath\": \"" << escapeJson(report.compilationDatabasePath) << "\"," << newline;
    output << indent << "\"semanticAnalysisRequested\": " << (report.semanticAnalysisRequested ? "true" : "false") << "," << newline;
    output << indent << "\"semanticAnalysisEnabled\": " << (report.semanticAnalysisEnabled ? "true" : "false") << "," << newline;
    output << indent << "\"summary\": {" << newline;
    output << indent << indent << "\"discoveredFiles\": " << report.discoveredFiles << "," << newline;
    output << indent << indent << "\"parsedFiles\": " << report.parsedFiles << "," << newline;
    output << indent << indent << "\"nodeCount\": " << report.nodes.size() << "," << newline;
    output << indent << indent << "\"edgeCount\": " << report.edges.size() << "," << newline;
    output << indent << indent << "\"inferredNodeCount\": " << countNodesByFactSource(report, FactSource::Inferred) << "," << newline;
    output << indent << indent << "\"semanticNodeCount\": " << countNodesByFactSource(report, FactSource::Semantic) << "," << newline;
    output << indent << indent << "\"inferredEdgeCount\": " << countEdgesByFactSource(report, FactSource::Inferred) << "," << newline;
    output << indent << indent << "\"semanticEdgeCount\": " << countEdgesByFactSource(report, FactSource::Semantic) << newline;
    output << indent << "}," << newline;

    output << indent << "\"nodes\": [" << newline;
    for (std::size_t index = 0; index < report.nodes.size(); ++index) {
        const SymbolNode& node = report.nodes[index];
        output << indent << indent << "{";
        output << "\"id\": " << node.id << ", ";
        output << "\"kind\": \"" << toString(node.kind) << "\", ";
        output << "\"displayName\": \"" << escapeJson(node.displayName) << "\", ";
        output << "\"qualifiedName\": \"" << escapeJson(node.qualifiedName) << "\", ";
        output << "\"identityKey\": \"" << escapeJson(node.identityKey) << "\", ";
        output << "\"filePath\": \"" << escapeJson(node.filePath) << "\", ";
        output << "\"line\": " << node.line << ", ";
        output << "\"factSource\": \"" << toString(node.factSource) << "\"";
        output << "}";
        if (index + 1 < report.nodes.size()) {
            output << ",";
        }
        output << newline;
    }
    output << indent << "]," << newline;

    output << indent << "\"edges\": [" << newline;
    for (std::size_t index = 0; index < report.edges.size(); ++index) {
        const SymbolEdge& edge = report.edges[index];
        output << indent << indent << "{";
        output << "\"fromId\": " << edge.fromId << ", ";
        output << "\"toId\": " << edge.toId << ", ";
        output << "\"kind\": \"" << toString(edge.kind) << "\", ";
        output << "\"weight\": " << edge.weight << ", ";
        output << "\"supportCount\": " << edge.supportCount << ", ";
        output << "\"factSource\": \"" << toString(edge.factSource) << "\"";
        output << "}";
        if (index + 1 < report.edges.size()) {
            output << ",";
        }
        output << newline;
    }
    output << indent << "]," << newline;

    output << indent << "\"diagnostics\": [" << newline;
    for (std::size_t index = 0; index < report.diagnostics.size(); ++index) {
        output << indent << indent << "\"" << escapeJson(report.diagnostics[index]) << "\"";
        if (index + 1 < report.diagnostics.size()) {
            output << ",";
        }
        output << newline;
    }
    output << indent << "]" << newline;
    output << "}";
    return output.str();
}

std::string formatAnalysisReportAsDot(const AnalysisReport& report, const bool modulesOnly) {
    std::ostringstream output;
    output << "digraph SAVT {\n";
    output << "  rankdir=LR;\n";
    output << "  graph [fontname=\"Cascadia Mono\"];\n";
    output << "  node [shape=box, fontname=\"Cascadia Mono\"];\n";
    output << "  edge [fontname=\"Cascadia Mono\"];\n";

    for (const SymbolNode& node : report.nodes) {
        if (!shouldIncludeNodeInDot(node, modulesOnly)) {
            continue;
        }

        output << "  n" << node.id << " [label=\""
               << escapeDot(std::string(toString(node.kind)) + "\n" + node.displayName)
               << "\"];\n";
    }

    for (const SymbolEdge& edge : report.edges) {
        if (!shouldIncludeEdgeInDot(edge, modulesOnly, report)) {
            continue;
        }

        output << "  n" << edge.fromId << " -> n" << edge.toId
               << " [label=\"" << toString(edge.kind);
        if (edge.weight > 1) {
            output << " (" << edge.weight << ")";
        }
        output << "\"];\n";
    }

    output << "}\n";
    return output.str();
}

}  // namespace savt::core
