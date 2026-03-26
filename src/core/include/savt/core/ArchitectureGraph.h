#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace savt::core {

enum class SymbolKind {
    Module,
    File,
    Namespace,
    Class,
    Struct,
    Enum,
    Enumerator,
    Field,
    Function,
    Method
};

enum class EdgeKind {
    Contains,
    Calls,
    Includes,
    Inherits,
    UsesType,
    Overrides,
    DependsOn
};

enum class FactSource {
    Inferred,
    Semantic
};

struct SymbolNode {
    std::size_t id = 0;
    SymbolKind kind = SymbolKind::File;
    std::string displayName;
    std::string qualifiedName;
    std::string identityKey;
    std::string filePath;
    std::uint32_t line = 0;
    FactSource factSource = FactSource::Inferred;
};

struct SymbolEdge {
    std::size_t fromId = 0;
    std::size_t toId = 0;
    EdgeKind kind = EdgeKind::Contains;
    std::size_t weight = 1;
    std::size_t supportCount = 1;
    FactSource factSource = FactSource::Inferred;
};

struct AnalysisReport {
    std::string rootPath;
    std::string primaryEngine;
    std::string precisionLevel;
    std::string semanticStatusCode;
    std::string semanticStatusMessage;
    std::string compilationDatabasePath;
    bool semanticAnalysisRequested = false;
    bool semanticAnalysisEnabled = false;
    std::size_t discoveredFiles = 0;
    std::size_t parsedFiles = 0;
    std::vector<SymbolNode> nodes;
    std::vector<SymbolEdge> edges;
    std::vector<std::string> diagnostics;
};

const char* toString(SymbolKind kind);
const char* toString(EdgeKind kind);
const char* toString(FactSource source);
const SymbolNode* findNodeById(const AnalysisReport& report, std::size_t nodeId);

std::string formatAnalysisReport(
    const AnalysisReport& report,
    std::size_t maxNodes = 80,
    std::size_t maxEdges = 120,
    std::size_t maxDiagnostics = 40);

std::string serializeAnalysisReportToJson(const AnalysisReport& report, bool pretty = true);
std::string formatAnalysisReportAsDot(const AnalysisReport& report, bool modulesOnly = false);

}  // namespace savt::core
