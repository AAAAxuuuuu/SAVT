#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/layout/LayeredGraphLayout.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

namespace {

using Clock = std::chrono::steady_clock;

struct PerfResult {
    std::string label;
    std::size_t discoveredFiles = 0;
    std::size_t parsedFiles = 0;
    std::size_t overviewNodes = 0;
    std::size_t capabilityNodes = 0;
    std::size_t capabilityEdges = 0;
    double analyzeMs = 0.0;
    double overviewMs = 0.0;
    double capabilityMs = 0.0;
    double layoutMs = 0.0;
    double totalMs = 0.0;
    double peakMemoryMb = 0.0;
};

void writeTextFile(const std::filesystem::path& filePath, const std::string& text) {
    std::filesystem::create_directories(filePath.parent_path());
    std::ofstream output(filePath, std::ios::binary);
    output << text;
}

std::filesystem::path makeTempDirectory(const std::string& prefix) {
    namespace fs = std::filesystem;
    const fs::path tempRoot = fs::temp_directory_path() /
                              (prefix + "_" + std::to_string(std::rand()));
    std::error_code errorCode;
    fs::remove_all(tempRoot, errorCode);
    fs::create_directories(tempRoot, errorCode);
    return tempRoot;
}

double peakMemoryMb() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)) == 0) {
        return 0.0;
    }
    return static_cast<double>(counters.PeakWorkingSetSize) / (1024.0 * 1024.0);
#else
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0.0;
    }
#if defined(__APPLE__)
    return static_cast<double>(usage.ru_maxrss) / (1024.0 * 1024.0);
#else
    return static_cast<double>(usage.ru_maxrss) / 1024.0;
#endif
#endif
}

template <typename Fn>
double measureMs(Fn&& fn) {
    const auto start = Clock::now();
    fn();
    const auto end = Clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

std::filesystem::path createSyntheticLargeWorkspace(const std::size_t totalFiles) {
    const std::filesystem::path tempRoot = makeTempDirectory("savt_perf_large_workspace");
    const std::size_t moduleCount = totalFiles >= 10000 ? 20 : 8;
    const std::size_t filesPerModule =
        std::max<std::size_t>(1, (totalFiles + moduleCount - 1) / moduleCount);

    writeTextFile(tempRoot / "apps" / "desktop" / "main.cpp", "int main() { return 0; }\n");
    std::size_t fileIndex = 0;
    for (std::size_t moduleIndex = 0; moduleIndex < moduleCount; ++moduleIndex) {
        for (std::size_t localIndex = 0; localIndex < filesPerModule && fileIndex < totalFiles; ++localIndex, ++fileIndex) {
            const std::filesystem::path filePath =
                tempRoot / "src" / ("module_" + std::to_string(moduleIndex)) /
                ("component_" + std::to_string(localIndex) + ".cpp");
            writeTextFile(
                filePath,
                "namespace module_" + std::to_string(moduleIndex) + " {\n"
                "int component_" + std::to_string(localIndex) + "() { return " +
                std::to_string(static_cast<int>(moduleIndex + localIndex)) + "; }\n}\n");
        }
    }
    return tempRoot;
}

PerfResult measureProject(const std::string& label, const std::filesystem::path& rootPath) {
    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;

    PerfResult result;
    result.label = label;

    savt::core::AnalysisReport report;
    savt::core::ArchitectureOverview overview;
    savt::core::CapabilityGraph capabilityGraph;
    savt::layout::CapabilitySceneLayoutResult layoutResult;
    savt::layout::LayeredGraphLayout layoutEngine;

    const auto totalStart = Clock::now();
    result.analyzeMs = measureMs([&] { report = analyzer.analyzeProject(rootPath, options); });
    result.overviewMs = measureMs([&] { overview = savt::core::buildArchitectureOverview(report); });
    result.capabilityMs = measureMs([&] { capabilityGraph = savt::core::buildCapabilityGraph(report, overview); });
    result.layoutMs = measureMs([&] { layoutResult = layoutEngine.layoutCapabilityScene(capabilityGraph); });
    const auto totalEnd = Clock::now();

    result.totalMs = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();
    result.discoveredFiles = report.discoveredFiles;
    result.parsedFiles = report.parsedFiles;
    result.overviewNodes = overview.nodes.size();
    result.capabilityNodes = capabilityGraph.nodes.size();
    result.capabilityEdges = capabilityGraph.edges.size();
    result.peakMemoryMb = peakMemoryMb();
    static_cast<void>(layoutResult);
    return result;
}

void printHumanReadable(const std::vector<PerfResult>& results) {
    std::cout << "label,discovered,parsed,overview_nodes,capability_nodes,capability_edges,"
                 "analyze_ms,overview_ms,capability_ms,layout_ms,total_ms,peak_memory_mb\n";
    for (const PerfResult& result : results) {
        std::cout << result.label << ','
                  << result.discoveredFiles << ','
                  << result.parsedFiles << ','
                  << result.overviewNodes << ','
                  << result.capabilityNodes << ','
                  << result.capabilityEdges << ','
                  << std::fixed << std::setprecision(2)
                  << result.analyzeMs << ','
                  << result.overviewMs << ','
                  << result.capabilityMs << ','
                  << result.layoutMs << ','
                  << result.totalMs << ','
                  << result.peakMemoryMb << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    bool fullMode = false;
    for (int index = 1; index < argc; ++index) {
        if (std::string_view(argv[index]) == "--full") {
            fullMode = true;
        }
    }

    std::vector<PerfResult> results;
    results.push_back(measureProject(
        "cpp_vendored_dependency_app",
        std::filesystem::path(SAVT_PERF_SOURCE_ROOT) / "tests/fixtures/precision/cpp_vendored_dependency_app"));
    results.push_back(measureProject(
        "qml_dashboard_workspace",
        std::filesystem::path(SAVT_PERF_SOURCE_ROOT) / "tests/fixtures/precision/qml_dashboard_workspace"));

    const std::size_t syntheticFiles = fullMode ? 10000 : 2000;
    const std::filesystem::path syntheticRoot = createSyntheticLargeWorkspace(syntheticFiles);
    results.push_back(measureProject(
        fullMode ? "synthetic_large_10000" : "synthetic_quick_2000",
        syntheticRoot));

    printHumanReadable(results);

    std::error_code errorCode;
    std::filesystem::remove_all(syntheticRoot, errorCode);
    return 0;
}
