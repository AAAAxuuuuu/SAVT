#pragma once

#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ProjectAnalysisConfig.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace savt::analyzer::detail {

struct SemanticBackendBuildInfo {
    bool requested = false;
    bool available = false;
    std::string configuredLlvmRoot;
    std::string statusCode;
    std::string statusMessage;
};

struct CompilationDatabaseProbe {
    std::optional<std::filesystem::path> resolvedPath;
    std::vector<std::filesystem::path> searchedPaths;
    bool explicitPathProvided = false;
};

bool hasSourceExtension(const std::filesystem::path& filePath);
bool hasArchitectureRelevantExtension(const std::filesystem::path& filePath);
bool shouldSkipDirectory(const std::filesystem::path& directoryPath, const AnalyzerOptions& options);

std::vector<std::filesystem::path> collectSourceFiles(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options);

std::optional<std::string> readAllText(const std::filesystem::path& filePath);
std::string toUnixPath(std::string value);
std::string normalizePath(const std::filesystem::path& value);
std::string relativizePath(const std::filesystem::path& rootPath, const std::filesystem::path& filePath);
std::vector<std::string> splitPathSegments(std::string_view path);
std::unordered_map<std::string, std::string> inferModuleNames(
    const std::vector<std::string>& relativePaths,
    const savt::core::ProjectAnalysisConfig* config = nullptr);
std::string inferModuleName(
    const std::string& relativePath,
    const savt::core::ProjectAnalysisConfig* config = nullptr);
const char* toString(AnalyzerPrecision precision);
bool isCancellationRequested(const AnalyzerOptions& options);
bool isSemanticBackendBuilt();
SemanticBackendBuildInfo semanticBackendBuildInfo();
CompilationDatabaseProbe probeCompilationDatabase(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options);

std::optional<std::filesystem::path> locateCompilationDatabase(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options);

// Source language classification used by heuristic analyzers.
enum class SourceLanguage {
    Cpp,
    Python,
    Java,
    JavaScript,
    TypeScript,
    Unknown
};

SourceLanguage detectLanguage(const std::filesystem::path& filePath);
bool isHeuristicAnalyzableFile(const std::filesystem::path& filePath);

}  // namespace savt::analyzer::detail
