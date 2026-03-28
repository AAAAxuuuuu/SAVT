#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/layout/LayeredGraphLayout.h"
#include "savt/ui/AnalysisTextFormatter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

struct SnapshotCase {
    std::string name;
    savt::analyzer::AnalyzerPrecision precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;
    enum class CompilationDatabaseMode {
        None,
        SingleTranslationUnit,
        CrossTranslationUnits,
        NoSystemHeadersSingleTranslationUnit
    };
    CompilationDatabaseMode compilationDatabaseMode = CompilationDatabaseMode::None;
};

struct SnapshotArtifacts {
    std::string reportJson;
    std::string reportDot;
    std::string overviewText;
    std::string capabilityText;
    std::string layoutText;
    std::string capabilitySceneLayoutText;
    std::string analysisReportMarkdown;
    std::string systemContextMarkdown;
    std::string precisionSummary;
};

#define SAVT_SNAPSHOT_STRINGIFY_DETAIL(value) #value
#define SAVT_SNAPSHOT_STRINGIFY(value) SAVT_SNAPSHOT_STRINGIFY_DETAIL(value)

std::string normalizePath(const fs::path& path) {
    return path.lexically_normal().generic_string();
}

void replaceAll(std::string& text, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return;
    }

    std::size_t cursor = 0;
    while ((cursor = text.find(from, cursor)) != std::string::npos) {
        text.replace(cursor, from.size(), to);
        cursor += to.size();
    }
}

void writeTextFile(const fs::path& filePath, const std::string& text) {
    fs::create_directories(filePath.parent_path());
    std::ofstream output(filePath, std::ios::binary);
    output << text;
}

std::string readTextFile(const fs::path& filePath) {
    std::ifstream input(filePath, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

fs::path testsSourceDir() {
    return fs::path(SAVT_SNAPSHOT_STRINGIFY(SAVT_SNAPSHOT_TESTS_SOURCE_DIR)).lexically_normal();
}

fs::path snapshotArtifactDir() {
    return fs::path(SAVT_SNAPSHOT_STRINGIFY(SAVT_SNAPSHOT_ARTIFACT_DIR)).lexically_normal();
}

fs::path fixtureSourceDir(const SnapshotCase& snapshotCase) {
    return testsSourceDir() / "fixtures" / "precision" / snapshotCase.name;
}

fs::path goldenDir(const SnapshotCase& snapshotCase) {
    return testsSourceDir() / "golden" / "precision" / snapshotCase.name;
}

fs::path runtimeDir(const SnapshotCase& snapshotCase) {
    return snapshotArtifactDir() / "runtime" / snapshotCase.name;
}

fs::path actualOutputDir(const SnapshotCase& snapshotCase) {
    return snapshotArtifactDir() / "actual" / snapshotCase.name;
}

std::string defaultCompilerCommand() {
#ifdef SAVT_SNAPSHOT_LLVM_BIN_DIR
    return normalizePath(fs::path(SAVT_SNAPSHOT_STRINGIFY(SAVT_SNAPSHOT_LLVM_BIN_DIR)) / "clang++.exe");
#else
    return "clang++";
#endif
}

std::string normalizeSnapshotText(std::string text, const fs::path& runtimeRoot) {
    const std::string runtimeRootPath = normalizePath(runtimeRoot);
    replaceAll(text, runtimeRootPath, "<fixture_root>");
    replaceAll(text, runtimeRoot.lexically_normal().string(), "<fixture_root>");
    replaceAll(text, runtimeRoot.string(), "<fixture_root>");
    replaceAll(text, (runtimeRoot / "compile_commands.json").lexically_normal().string(), "<fixture_root>/compile_commands.json");
    replaceAll(text, normalizePath(runtimeRoot / "compile_commands.json"), "<fixture_root>/compile_commands.json");
#ifdef SAVT_SNAPSHOT_LLVM_BIN_DIR
    const fs::path clangPath = fs::path(SAVT_SNAPSHOT_STRINGIFY(SAVT_SNAPSHOT_LLVM_BIN_DIR)) / "clang++.exe";
    replaceAll(text, normalizePath(clangPath), "<llvm_bin>/clang++.exe");
#endif
    return text;
}

void recreateDirectory(const fs::path& directory) {
    std::error_code errorCode;
    fs::remove_all(directory, errorCode);
    fs::create_directories(directory, errorCode);
    if (errorCode) {
        throw std::runtime_error("Failed to create directory: " + normalizePath(directory));
    }
}

void copyFixtureTree(const fs::path& sourceRoot, const fs::path& destinationRoot) {
    recreateDirectory(destinationRoot);
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(sourceRoot)) {
        const fs::path relativePath = fs::relative(entry.path(), sourceRoot);
        const fs::path targetPath = destinationRoot / relativePath;
        std::error_code errorCode;
        if (entry.is_directory()) {
            fs::create_directories(targetPath, errorCode);
        } else if (entry.is_regular_file()) {
            fs::create_directories(targetPath.parent_path(), errorCode);
            if (!errorCode) {
                fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing, errorCode);
            }
        }
        if (errorCode) {
            throw std::runtime_error(
                "Failed to copy fixture entry " + normalizePath(entry.path()) +
                " to " + normalizePath(targetPath));
        }
    }
}

void writeCompileCommands(
    const fs::path& runtimeRoot,
    const std::vector<std::string>& translationUnits,
    const std::vector<std::string>& extraArguments,
    const std::string& compilerCommand) {
    std::ostringstream output;
    output << "[\n";
    for (std::size_t index = 0; index < translationUnits.size(); ++index) {
        const fs::path translationUnit = runtimeRoot / translationUnits[index];
        output << "  {\n";
        output << "    \"directory\": \"" << normalizePath(runtimeRoot) << "\",\n";
        output << "    \"file\": \"" << normalizePath(translationUnit) << "\",\n";
        output << "    \"arguments\": [\n";
        output << "      \"" << compilerCommand << "\"";
        if (!extraArguments.empty() || true) {
            output << ",\n";
        } else {
            output << "\n";
        }
        for (std::size_t argumentIndex = 0; argumentIndex < extraArguments.size(); ++argumentIndex) {
            output << "      \"" << extraArguments[argumentIndex] << "\",\n";
        }
        output << "      \"" << normalizePath(translationUnit) << "\"\n";
        output << "    ]\n";
        output << "  }";
        if (index + 1 < translationUnits.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "]\n";

    writeTextFile(runtimeRoot / "compile_commands.json", output.str());
}

SnapshotArtifacts buildArtifacts(const fs::path& runtimeRoot, const SnapshotCase& snapshotCase) {
    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = snapshotCase.precision;

    const savt::core::AnalysisReport report = analyzer.analyzeProject(runtimeRoot, options);
    const savt::core::ArchitectureOverview overview = savt::core::buildArchitectureOverview(report);
    const savt::core::CapabilityGraph capability = savt::core::buildCapabilityGraph(report, overview);
    savt::layout::LayeredGraphLayout layoutEngine;
    const savt::layout::CapabilitySceneLayoutResult capabilitySceneLayout = layoutEngine.layoutCapabilityScene(capability);
    const savt::layout::LayoutResult layout = layoutEngine.layoutModules(report);

    SnapshotArtifacts artifacts;
    artifacts.reportJson = normalizeSnapshotText(savt::core::serializeAnalysisReportToJson(report, true), runtimeRoot);
    artifacts.reportDot = normalizeSnapshotText(savt::core::formatAnalysisReportAsDot(report, false), runtimeRoot);
    artifacts.overviewText = normalizeSnapshotText(savt::core::formatArchitectureOverview(overview), runtimeRoot);
    artifacts.capabilityText = normalizeSnapshotText(savt::core::formatCapabilityGraph(capability), runtimeRoot);
    artifacts.layoutText = normalizeSnapshotText(savt::layout::formatLayoutResult(layout, report), runtimeRoot);
    artifacts.capabilitySceneLayoutText = normalizeSnapshotText(savt::layout::formatCapabilitySceneLayoutResult(capabilitySceneLayout), runtimeRoot);
    artifacts.analysisReportMarkdown = normalizeSnapshotText(
        savt::ui::formatCapabilityReportMarkdown(report, capability).toStdString(),
        runtimeRoot);
    artifacts.systemContextMarkdown = normalizeSnapshotText(
        savt::ui::formatSystemContextReportMarkdown(capability).toStdString(),
        runtimeRoot);
    artifacts.precisionSummary = normalizeSnapshotText(
        savt::ui::formatPrecisionSummary(report, overview, capability).toStdString(),
        runtimeRoot);
    return artifacts;
}

std::map<std::string, std::string> artifactFileMap(const SnapshotArtifacts& artifacts) {
    return {
        {"analysis_report.json", artifacts.reportJson},
        {"analysis_report.dot", artifacts.reportDot},
        {"analysis_report.md", artifacts.analysisReportMarkdown},
        {"overview.txt", artifacts.overviewText},
        {"capability.txt", artifacts.capabilityText},
        {"layout.txt", artifacts.layoutText},
        {"capability_scene_layout.txt", artifacts.capabilitySceneLayoutText},
        {"system_context.md", artifacts.systemContextMarkdown},
        {"precision_summary.txt", artifacts.precisionSummary},
    };
}

std::vector<std::string> expectedArtifactFileNames() {
    return {
        "analysis_report.json",
        "analysis_report.dot",
        "analysis_report.md",
        "overview.txt",
        "capability.txt",
        "layout.txt",
        "capability_scene_layout.txt",
        "system_context.md",
        "precision_summary.txt",
    };
}

std::set<std::string> directoryNames(const fs::path& root) {
    std::set<std::string> names;
    if (!fs::exists(root)) {
        return names;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(root)) {
        if (entry.is_directory()) {
            names.insert(entry.path().filename().string());
        }
    }
    return names;
}

bool hasRegularFiles(const fs::path& root) {
    if (!fs::exists(root)) {
        return false;
    }

    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file()) {
            return true;
        }
    }
    return false;
}

void validateSnapshotInventory(
    const std::vector<SnapshotCase>& activeCases,
    const std::vector<SnapshotCase>& knownCases,
    const bool updateGoldens,
    std::vector<std::string>& failures) {
    std::set<std::string> caseNames;
    for (const SnapshotCase& snapshotCase : knownCases) {
        if (!caseNames.insert(snapshotCase.name).second) {
            failures.push_back("Duplicate snapshot case registration: " + snapshotCase.name);
        }
    }
    std::set<std::string> activeCaseNames;
    for (const SnapshotCase& snapshotCase : activeCases) {
        activeCaseNames.insert(snapshotCase.name);
    }

    const fs::path fixturesRoot = testsSourceDir() / "fixtures" / "precision";
    const fs::path goldenRoot = testsSourceDir() / "golden" / "precision";
    const std::set<std::string> fixtureNames = directoryNames(fixturesRoot);
    const std::set<std::string> goldenNames = directoryNames(goldenRoot);

    for (const std::string& fixtureName : fixtureNames) {
        if (!caseNames.contains(fixtureName)) {
            failures.push_back("Unregistered fixture directory: " + fixtureName);
        }
    }
    for (const std::string& goldenName : goldenNames) {
        if (!caseNames.contains(goldenName)) {
            failures.push_back("Unregistered golden directory: " + goldenName);
        }
    }

    for (const SnapshotCase& snapshotCase : knownCases) {
        const fs::path sourceRoot = fixtureSourceDir(snapshotCase);
        const fs::path caseGoldenRoot = goldenDir(snapshotCase);
        if (!fs::exists(sourceRoot)) {
            failures.push_back("Missing fixture source directory: " + normalizePath(sourceRoot));
            continue;
        }
        if (!hasRegularFiles(sourceRoot)) {
            failures.push_back("Fixture source directory has no files: " + normalizePath(sourceRoot));
        }
        if (!fs::exists(caseGoldenRoot)) {
            if (updateGoldens && activeCaseNames.contains(snapshotCase.name)) {
                continue;
            }
            failures.push_back("Missing golden directory: " + normalizePath(caseGoldenRoot));
            continue;
        }
        for (const std::string& artifactFileName : expectedArtifactFileNames()) {
            if (!fs::exists(caseGoldenRoot / artifactFileName)) {
                if (updateGoldens && activeCaseNames.contains(snapshotCase.name)) {
                    continue;
                }
                failures.push_back(
                    "Missing golden artifact for " + snapshotCase.name + ": " +
                    normalizePath(caseGoldenRoot / artifactFileName));
            }
        }
    }
}

bool compareOrUpdateArtifacts(
    const SnapshotCase& snapshotCase,
    const SnapshotArtifacts& artifacts,
    const bool updateGoldens,
    std::vector<std::string>& failures) {
    bool success = true;
    const fs::path caseGoldenDir = goldenDir(snapshotCase);
    const fs::path caseActualDir = actualOutputDir(snapshotCase);
    recreateDirectory(caseActualDir);

    for (const auto& [fileName, contents] : artifactFileMap(artifacts)) {
        const fs::path goldenFile = caseGoldenDir / fileName;
        const fs::path actualFile = caseActualDir / fileName;
        writeTextFile(actualFile, contents);

        if (updateGoldens) {
            writeTextFile(goldenFile, contents);
            continue;
        }

        if (!fs::exists(goldenFile)) {
            failures.push_back(
                "Missing golden for " + snapshotCase.name + "/" + fileName +
                " (expected " + normalizePath(goldenFile) + ")");
            success = false;
            continue;
        }

        const std::string expected = readTextFile(goldenFile);
        if (expected != contents) {
            failures.push_back(
                "Snapshot mismatch for " + snapshotCase.name + "/" + fileName +
                ". Actual output written to " + normalizePath(actualFile));
            success = false;
        }
    }

    return success;
}

std::vector<SnapshotCase> allSnapshotCases() {
    return {
        {"cpp_syntax_only", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"cpp_vendored_dependency_app", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"js_backend", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"spring_boot_java", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"qml_mixed_project", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"python_tooling_data_project", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"cpp_semantic_cross_tu", savt::analyzer::AnalyzerPrecision::SemanticRequired, SnapshotCase::CompilationDatabaseMode::CrossTranslationUnits},
        {"semantic_required_missing_compile_commands", savt::analyzer::AnalyzerPrecision::SemanticRequired, SnapshotCase::CompilationDatabaseMode::None},
        {"semantic_required_system_headers_unresolved", savt::analyzer::AnalyzerPrecision::SemanticRequired, SnapshotCase::CompilationDatabaseMode::NoSystemHeadersSingleTranslationUnit},
        {"semantic_preferred_llvm_not_found", savt::analyzer::AnalyzerPrecision::SemanticPreferred, SnapshotCase::CompilationDatabaseMode::SingleTranslationUnit},
        {"semantic_preferred_backend_unavailable", savt::analyzer::AnalyzerPrecision::SemanticPreferred, SnapshotCase::CompilationDatabaseMode::SingleTranslationUnit},
    };
}

std::vector<SnapshotCase> snapshotCases() {
    std::vector<SnapshotCase> cases = {
        {"cpp_syntax_only", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"cpp_vendored_dependency_app", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"js_backend", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"spring_boot_java", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"qml_mixed_project", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
        {"python_tooling_data_project", savt::analyzer::AnalyzerPrecision::SyntaxOnly, SnapshotCase::CompilationDatabaseMode::None},
    };
#ifdef SAVT_ENABLE_CLANG_TOOLING
    cases.push_back({
        "cpp_semantic_cross_tu",
        savt::analyzer::AnalyzerPrecision::SemanticRequired,
        SnapshotCase::CompilationDatabaseMode::CrossTranslationUnits
    });
    cases.push_back({
        "semantic_required_missing_compile_commands",
        savt::analyzer::AnalyzerPrecision::SemanticRequired,
        SnapshotCase::CompilationDatabaseMode::None
    });
    cases.push_back({
        "semantic_required_system_headers_unresolved",
        savt::analyzer::AnalyzerPrecision::SemanticRequired,
        SnapshotCase::CompilationDatabaseMode::NoSystemHeadersSingleTranslationUnit
    });
#elif defined(SAVT_CLANG_TOOLING_REQUESTED)
    cases.push_back({
        "semantic_preferred_llvm_not_found",
        savt::analyzer::AnalyzerPrecision::SemanticPreferred,
        SnapshotCase::CompilationDatabaseMode::SingleTranslationUnit
    });
#else
    cases.push_back({
        "semantic_preferred_backend_unavailable",
        savt::analyzer::AnalyzerPrecision::SemanticPreferred,
        SnapshotCase::CompilationDatabaseMode::SingleTranslationUnit
    });
#endif
    return cases;
}

void prepareFixtureRuntime(const SnapshotCase& snapshotCase, const fs::path& runtimeRoot) {
    const fs::path sourceRoot = fixtureSourceDir(snapshotCase);
    if (!fs::exists(sourceRoot)) {
        throw std::runtime_error("Missing fixture source directory: " + normalizePath(sourceRoot));
    }

    copyFixtureTree(sourceRoot, runtimeRoot);

    switch (snapshotCase.compilationDatabaseMode) {
    case SnapshotCase::CompilationDatabaseMode::None:
        break;
    case SnapshotCase::CompilationDatabaseMode::SingleTranslationUnit:
        writeCompileCommands(
            runtimeRoot,
            {"main.cpp"},
            {
                "-std=c++20",
                "-I" + normalizePath(runtimeRoot),
                "-fsyntax-only",
            },
            defaultCompilerCommand());
        break;
    case SnapshotCase::CompilationDatabaseMode::CrossTranslationUnits:
#ifdef SAVT_ENABLE_CLANG_TOOLING
        writeCompileCommands(
            runtimeRoot,
            {"base.cpp", "derived.cpp"},
            {
                "-std=c++20",
                "-I" + normalizePath(runtimeRoot),
                "-fsyntax-only",
            },
            defaultCompilerCommand());
#endif
        break;
    case SnapshotCase::CompilationDatabaseMode::NoSystemHeadersSingleTranslationUnit:
#ifdef SAVT_ENABLE_CLANG_TOOLING
        writeCompileCommands(
            runtimeRoot,
            {"main.cpp"},
            {
                "-std=c++20",
                "-nostdinc",
                "-nostdinc++",
                "-fsyntax-only",
            },
            defaultCompilerCommand());
#endif
        break;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const bool updateGoldens = argc > 1 && std::string(argv[1]) == "--update";
        fs::create_directories(snapshotArtifactDir());

        const std::vector<SnapshotCase> cases = snapshotCases();
        const std::vector<SnapshotCase> knownCases = allSnapshotCases();
        std::vector<std::string> failures;
        validateSnapshotInventory(cases, knownCases, updateGoldens, failures);
        for (const SnapshotCase& snapshotCase : cases) {
            const fs::path caseRuntimeDir = runtimeDir(snapshotCase);
            prepareFixtureRuntime(snapshotCase, caseRuntimeDir);
            const SnapshotArtifacts artifacts = buildArtifacts(caseRuntimeDir, snapshotCase);
            const bool caseOk = compareOrUpdateArtifacts(snapshotCase, artifacts, updateGoldens, failures);
            std::cout << (caseOk ? "[PASS] " : "[FAIL] ") << snapshotCase.name << "\n";
        }

        if (!failures.empty()) {
            for (const std::string& failure : failures) {
                std::cerr << "[FAIL] " << failure << "\n";
            }
            return 1;
        }

        std::cout << (updateGoldens ? "[PASS] savt_snapshot_tests (updated)" : "[PASS] savt_snapshot_tests") << "\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "[FAIL] savt_snapshot_tests: " << exception.what() << "\n";
        return 1;
    }
}
