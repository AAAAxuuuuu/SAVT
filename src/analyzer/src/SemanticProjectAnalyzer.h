#pragma once

#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ArchitectureGraph.h"

#include <filesystem>

namespace savt::analyzer::detail {

enum class SemanticFailureKind {
    None,
    CompilationDatabaseLoadFailed,
    CompilationDatabaseEmpty,
    LibclangIndexCreationFailed,
    SystemHeadersUnresolved,
    TranslationUnitParseFailed
};

struct SemanticBackendResult {
    savt::core::AnalysisReport report;
    bool completed = false;
    SemanticFailureKind failureKind = SemanticFailureKind::None;
    std::string failureMessage;
};

SemanticBackendResult analyzeSemanticProject(
    const std::filesystem::path& rootPath,
    const std::filesystem::path& compilationDatabasePath,
    const AnalyzerOptions& options);

}  // namespace savt::analyzer::detail
