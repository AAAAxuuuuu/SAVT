#pragma once

#include "savt/core/ArchitectureGraph.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace savt::analyzer {

enum class AnalyzerPrecision {
    SyntaxOnly,
    SemanticPreferred,
    SemanticRequired
};

struct AnalyzerOptions {
    bool includeThirdParty = false;
    bool includeBuildDirectories = false;
    bool autoGenerateCompilationDatabase = true;
    std::size_t maxFiles = 0;
    AnalyzerPrecision precision = AnalyzerPrecision::SemanticPreferred;
    std::filesystem::path compilationDatabasePath;
    std::function<bool()> cancellationRequested;
    std::function<void(std::size_t completed, std::size_t total, const std::string& label)> progressReporter;
};

struct ProjectScanManifest {
    std::filesystem::path rootPath;
    std::vector<std::filesystem::path> sourceFiles;
    std::size_t discoveredFiles = 0;
    std::string metadataFingerprint;
    std::vector<std::string> diagnostics;
};

class CppProjectAnalyzer {
public:
    ProjectScanManifest buildScanManifest(
        const std::filesystem::path& rootPath,
        const AnalyzerOptions& options = {}) const;

    savt::core::AnalysisReport analyzeProject(
        const std::filesystem::path& rootPath,
        const AnalyzerOptions& options = {}) const;
};

}  // namespace savt::analyzer
