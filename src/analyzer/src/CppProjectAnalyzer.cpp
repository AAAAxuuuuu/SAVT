#include "savt/analyzer/CppProjectAnalyzer.h"

#include "AnalyzerUtilities.h"
#include "SemanticProjectAnalyzer.h"
#include "SyntaxProjectAnalyzer.h"

namespace savt::analyzer {
namespace {

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

savt::core::AnalysisReport CppProjectAnalyzer::analyzeProject(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options) const {
    const std::filesystem::path normalizedRoot = rootPath.lexically_normal();
    const bool semanticRequested = options.precision != AnalyzerPrecision::SyntaxOnly;
    const bool semanticRequired = options.precision == AnalyzerPrecision::SemanticRequired;
    const auto compilationDatabasePath = detail::locateCompilationDatabase(normalizedRoot, options);
    const auto backendInfo = detail::semanticBackendBuildInfo();

    std::vector<std::string> preflightDiagnostics;
    std::string semanticStatusCode = semanticRequested ? "semantic_ready" : "not_requested";
    std::string semanticStatusMessage = semanticRequested
        ? "Semantic analysis is ready to start."
        : "Semantic analysis was not requested for this run.";

    if (semanticRequested && !compilationDatabasePath.has_value()) {
        preflightDiagnostics.push_back(
            "Semantic analysis requested, but no compile_commands.json was found.");
        semanticStatusCode = "missing_compile_commands";
        semanticStatusMessage =
            "Semantic analysis is blocked because no compile_commands.json was found for the target project.";
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
