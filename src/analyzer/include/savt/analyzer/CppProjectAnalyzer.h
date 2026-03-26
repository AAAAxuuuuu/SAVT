#pragma once

#include "savt/core/ArchitectureGraph.h"

#include <cstddef>
#include <filesystem>
#include <functional>

namespace savt::analyzer {

enum class AnalyzerPrecision {
    SyntaxOnly,
    SemanticPreferred,
    SemanticRequired
};

struct AnalyzerOptions {
    bool includeThirdParty = false;
    bool includeBuildDirectories = false;
    std::size_t maxFiles = 0;
    AnalyzerPrecision precision = AnalyzerPrecision::SemanticPreferred;
    std::filesystem::path compilationDatabasePath;
    std::function<bool()> cancellationRequested;
};

class CppProjectAnalyzer {
public:
    savt::core::AnalysisReport analyzeProject(
        const std::filesystem::path& rootPath,
        const AnalyzerOptions& options = {}) const;
};

}  // namespace savt::analyzer
