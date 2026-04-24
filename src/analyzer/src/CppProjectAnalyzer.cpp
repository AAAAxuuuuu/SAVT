#include "savt/analyzer/CppProjectAnalyzer.h"

#include "AnalyzerUtilities.h"
#include "SemanticProjectAnalyzer.h"
#include "SyntaxProjectAnalyzer.h"
#include "savt/core/ProjectAnalysisConfig.h"

#include <charconv>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace savt::analyzer {
namespace {

constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ull;
constexpr std::uint64_t kFnvPrime = 1099511628211ull;

void hashBytes(std::uint64_t& state, const void* data, const std::size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t index = 0; index < size; ++index) {
        state ^= static_cast<std::uint64_t>(bytes[index]);
        state *= kFnvPrime;
    }
}

void hashString(std::uint64_t& state, const std::string_view value) {
    hashBytes(state, value.data(), value.size());
}

std::string hashToHex(const std::uint64_t value) {
    std::ostringstream output;
    output << std::hex << std::setfill('0') << std::setw(16) << value;
    return output.str();
}

std::string joinPathList(const std::vector<std::filesystem::path>& paths, const std::size_t maxItems = 8) {
    std::string text;
    const std::size_t itemCount = std::min(paths.size(), maxItems);
    for (std::size_t index = 0; index < itemCount; ++index) {
        if (!text.empty()) {
            text += "; ";
        }
        text += detail::normalizePath(paths[index]);
    }
    if (paths.size() > itemCount) {
        text += "; ...";
    }
    return text;
}

void setSemanticStatus(
    savt::core::AnalysisReport& report,
    std::string code,
    std::string message) {
    report.semanticStatusCode = std::move(code);
    report.semanticStatusMessage = std::move(message);
}

std::pair<std::string, std::string> semanticFailureStatus(const detail::SemanticBackendResult& result) {
    switch (result.failureKind) {
    case detail::SemanticFailureKind::None:
        return {"semantic_ready", "Clang/LibTooling semantic backend completed successfully."};
    case detail::SemanticFailureKind::CompilationDatabaseLoadFailed:
        return {"compilation_database_load_failed",
                result.failureMessage.empty()
                    ? "libclang could not load the compilation database."
                    : result.failureMessage};
    case detail::SemanticFailureKind::CompilationDatabaseEmpty:
        return {"compilation_database_empty",
                result.failureMessage.empty()
                    ? "The compilation database was found but contains no compile commands."
                    : result.failureMessage};
    case detail::SemanticFailureKind::LibclangIndexCreationFailed:
        return {"libclang_index_creation_failed",
                result.failureMessage.empty()
                    ? "libclang failed to create a semantic analysis index."
                    : result.failureMessage};
    case detail::SemanticFailureKind::SystemHeadersUnresolved:
        return {"system_headers_unresolved",
                result.failureMessage.empty()
                    ? "compile_commands.json was found, but libclang could not resolve required system headers."
                    : result.failureMessage};
    case detail::SemanticFailureKind::TranslationUnitParseFailed:
        return {"translation_unit_parse_failed",
                result.failureMessage.empty()
                    ? "The semantic backend could not parse any project translation units."
                    : result.failureMessage};
    }

    return {"semantic_startup_failed", "The semantic backend did not reach a usable state."};
}

savt::core::AnalysisReport makeBlockedReport(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options,
    const std::optional<std::filesystem::path>& compilationDatabasePath,
    std::vector<std::string> diagnostics,
    std::string semanticStatusCode,
    std::string semanticStatusMessage) {
    savt::core::AnalysisReport report;
    report.rootPath = detail::normalizePath(rootPath);
    report.primaryEngine = "none";
    report.precisionLevel = "blocked_semantic_required";
    report.semanticAnalysisRequested = options.precision != AnalyzerPrecision::SyntaxOnly;
    report.semanticAnalysisEnabled = false;
    setSemanticStatus(report, std::move(semanticStatusCode), std::move(semanticStatusMessage));
    if (compilationDatabasePath.has_value()) {
        report.compilationDatabasePath = detail::normalizePath(*compilationDatabasePath);
    }
    report.diagnostics = std::move(diagnostics);
    report.diagnostics.push_back(std::string("Analyzer precision mode: ") + detail::toString(options.precision));
    return report;
}

void finalizeSyntaxReport(
    savt::core::AnalysisReport& report,
    const AnalyzerOptions& options,
    const std::optional<std::filesystem::path>& compilationDatabasePath,
    const std::vector<std::string>& preflightDiagnostics,
    const std::string& semanticStatusCode,
    const std::string& semanticStatusMessage,
    const bool usedFallback) {
    report.semanticAnalysisRequested = options.precision != AnalyzerPrecision::SyntaxOnly;
    report.semanticAnalysisEnabled = false;
    setSemanticStatus(report, semanticStatusCode, semanticStatusMessage);
    if (compilationDatabasePath.has_value()) {
        report.compilationDatabasePath = detail::normalizePath(*compilationDatabasePath);
    }
    if (usedFallback) {
        report.precisionLevel = "syntax_fallback";
    }
    report.diagnostics.insert(report.diagnostics.begin(), preflightDiagnostics.begin(), preflightDiagnostics.end());
    report.diagnostics.push_back(std::string("Analyzer precision mode: ") + detail::toString(options.precision));
}

void finalizeSemanticReport(
    savt::core::AnalysisReport& report,
    const AnalyzerOptions& options,
    const std::filesystem::path& compilationDatabasePath,
    const std::vector<std::string>& preflightDiagnostics,
    const std::string& semanticStatusCode,
    const std::string& semanticStatusMessage,
    const bool completed,
    const bool semanticRequired) {
    report.semanticAnalysisRequested = options.precision != AnalyzerPrecision::SyntaxOnly;
    report.compilationDatabasePath = detail::normalizePath(compilationDatabasePath);
    report.semanticAnalysisEnabled = completed;
    report.primaryEngine = "libclang-cindex";
    report.precisionLevel = completed ? "semantic" : (semanticRequired ? "blocked_semantic_required" : "semantic_startup_failed");
    setSemanticStatus(report, semanticStatusCode, semanticStatusMessage);
    report.diagnostics.insert(report.diagnostics.begin(), preflightDiagnostics.begin(), preflightDiagnostics.end());
    report.diagnostics.push_back(std::string("Analyzer precision mode: ") + detail::toString(options.precision));
}

}  // namespace

ProjectScanManifest CppProjectAnalyzer::buildScanManifest(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options) const {
    const std::filesystem::path normalizedRoot = rootPath.lexically_normal();
    ProjectScanManifest manifest;
    manifest.rootPath = normalizedRoot;
    manifest.sourceFiles = detail::collectSourceFiles(normalizedRoot, options);
    manifest.discoveredFiles = manifest.sourceFiles.size();

    std::uint64_t fingerprint = kFnvOffsetBasis;
    hashString(fingerprint, detail::normalizePath(normalizedRoot));
    for (const std::filesystem::path& filePath : manifest.sourceFiles) {
        if (detail::isCancellationRequested(options)) {
            manifest.diagnostics.push_back("Project scan canceled before metadata fingerprint completed.");
            break;
        }

        const std::string relativePath = detail::relativizePath(normalizedRoot, filePath);
        hashString(fingerprint, relativePath);

        std::error_code errorCode;
        const auto fileSize = std::filesystem::file_size(filePath, errorCode);
        if (errorCode) {
            manifest.diagnostics.push_back(
                "Project scan could not read file size for: " + detail::normalizePath(filePath));
            hashString(fingerprint, "size:error");
        } else {
            hashBytes(fingerprint, &fileSize, sizeof(fileSize));
        }

        errorCode.clear();
        const auto lastWriteTime = std::filesystem::last_write_time(filePath, errorCode);
        if (errorCode) {
            manifest.diagnostics.push_back(
                "Project scan could not read timestamp for: " + detail::normalizePath(filePath));
            hashString(fingerprint, "mtime:error");
        } else {
            const auto tickCount = lastWriteTime.time_since_epoch().count();
            hashBytes(fingerprint, &tickCount, sizeof(tickCount));
        }
    }

    manifest.metadataFingerprint = hashToHex(fingerprint);
    return manifest;
}

savt::core::AnalysisReport CppProjectAnalyzer::analyzeProject(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options) const {
    const std::filesystem::path normalizedRoot = rootPath.lexically_normal();
    const bool semanticRequested = options.precision != AnalyzerPrecision::SyntaxOnly;
    const bool semanticRequired = options.precision == AnalyzerPrecision::SemanticRequired;
    const auto backendInfo = detail::semanticBackendBuildInfo();
    const auto compilationDatabaseProbe =
        semanticRequested
            ? detail::prepareCompilationDatabase(normalizedRoot, options)
            : detail::probeCompilationDatabase(normalizedRoot, options);
    const auto& compilationDatabasePath = compilationDatabaseProbe.resolvedPath;
    const auto projectConfig = savt::core::loadProjectAnalysisConfig(normalizedRoot);

    std::vector<std::string> preflightDiagnostics;
    preflightDiagnostics.insert(
        preflightDiagnostics.end(),
        projectConfig.diagnostics.begin(),
        projectConfig.diagnostics.end());
    preflightDiagnostics.insert(
        preflightDiagnostics.end(),
        compilationDatabaseProbe.diagnostics.begin(),
        compilationDatabaseProbe.diagnostics.end());
    std::string semanticStatusCode = semanticRequested ? "semantic_ready" : "not_requested";
    std::string semanticStatusMessage = semanticRequested
        ? "Semantic analysis is ready to start."
        : "Semantic analysis was not requested for this run.";

    if (semanticRequested && !compilationDatabasePath.has_value()) {
        preflightDiagnostics.push_back(
            "Semantic analysis requested, but no compile_commands.json was found.");
        if (compilationDatabaseProbe.explicitPathProvided && !options.compilationDatabasePath.empty()) {
            preflightDiagnostics.push_back(
                "Requested compilation database path was not found: " +
                detail::normalizePath(options.compilationDatabasePath));
        }
        if (!compilationDatabaseProbe.searchedPaths.empty()) {
            preflightDiagnostics.push_back(
                "Compilation database search paths: " + joinPathList(compilationDatabaseProbe.searchedPaths));
        }
        semanticStatusCode = "missing_compile_commands";
        semanticStatusMessage = compilationDatabaseProbe.generationAttempted
            ? "Semantic analysis is blocked because no compile_commands.json was found or generated for the target project."
            : "Semantic analysis is blocked because no compile_commands.json was found for the target project.";
    }
    if (semanticRequested && !backendInfo.available) {
        preflightDiagnostics.push_back(backendInfo.statusMessage);
        if (compilationDatabasePath.has_value()) {
            semanticStatusCode = backendInfo.statusCode;
            semanticStatusMessage = backendInfo.statusMessage;
        } else {
            semanticStatusMessage += " " + backendInfo.statusMessage;
        }
    }

    if (semanticRequested && compilationDatabasePath.has_value() && backendInfo.available) {
        detail::SemanticBackendResult semanticResult =
            detail::analyzeSemanticProject(normalizedRoot, *compilationDatabasePath, options);
        const auto [runtimeStatusCode, runtimeStatusMessage] = semanticFailureStatus(semanticResult);
        finalizeSemanticReport(
            semanticResult.report,
            options,
            *compilationDatabasePath,
            preflightDiagnostics,
            runtimeStatusCode,
            runtimeStatusMessage,
            semanticResult.completed,
            semanticRequired);

        if (semanticResult.completed || semanticRequired) {
            return semanticResult.report;
        }

        savt::core::AnalysisReport syntaxReport = detail::analyzeSyntaxProject(normalizedRoot, options);
        finalizeSyntaxReport(
            syntaxReport,
            options,
            compilationDatabasePath,
            semanticResult.report.diagnostics,
            runtimeStatusCode,
            runtimeStatusMessage,
            true);
        syntaxReport.diagnostics.push_back(
            "Fell back to tree-sitter syntax analysis after semantic backend startup failure.");
        return syntaxReport;
    }

    if (semanticRequired) {
        return makeBlockedReport(
            normalizedRoot,
            options,
            compilationDatabasePath,
            std::move(preflightDiagnostics),
            std::move(semanticStatusCode),
            std::move(semanticStatusMessage));
    }

    savt::core::AnalysisReport syntaxReport = detail::analyzeSyntaxProject(normalizedRoot, options);
    finalizeSyntaxReport(
        syntaxReport,
        options,
        compilationDatabasePath,
        preflightDiagnostics,
        semanticStatusCode,
        semanticStatusMessage,
        semanticRequested);
    return syntaxReport;
}

}  // namespace savt::analyzer
