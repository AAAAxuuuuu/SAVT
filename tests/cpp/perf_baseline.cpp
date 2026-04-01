#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/ui/IncrementalAnalysisPipeline.h"

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
    double coldTotalMs = 0.0;
    double hotTotalMs = 0.0;
    bool hotScanHit = false;
    bool hotParseHit = false;
    bool hotAggregateHit = false;
    bool hotLayoutHit = false;
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
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;

    PerfResult result;
    result.label = label;

    savt::ui::IncrementalAnalysisPipeline::clear();

    savt::ui::IncrementalAnalysisArtifacts coldArtifacts;
    savt::ui::IncrementalAnalysisArtifacts hotArtifacts;
    result.coldTotalMs = measureMs([&] {
        coldArtifacts = savt::ui::IncrementalAnalysisPipeline::analyze(rootPath, options);
    });
    result.hotTotalMs = measureMs([&] {
        hotArtifacts = savt::ui::IncrementalAnalysisPipeline::analyze(rootPath, options);
    });

    result.discoveredFiles = coldArtifacts.report.discoveredFiles;
    result.parsedFiles = coldArtifacts.report.parsedFiles;
    result.overviewNodes = coldArtifacts.overview.nodes.size();
    result.capabilityNodes = coldArtifacts.capabilityGraph.nodes.size();
    result.capabilityEdges = coldArtifacts.capabilityGraph.edges.size();
    result.hotScanHit = hotArtifacts.scanLayer.hit;
    result.hotParseHit = hotArtifacts.parseLayer.hit;
    result.hotAggregateHit = hotArtifacts.aggregateLayer.hit;
    result.hotLayoutHit = hotArtifacts.layoutLayer.hit;
    result.peakMemoryMb = peakMemoryMb();
    return result;
}

void printHumanReadable(const std::vector<PerfResult>& results) {
    std::cout << "label,discovered,parsed,overview_nodes,capability_nodes,capability_edges,"
                 "cold_total_ms,hot_total_ms,hot_scan_hit,hot_parse_hit,hot_aggregate_hit,hot_layout_hit,peak_memory_mb\n";
    for (const PerfResult& result : results) {
        std::cout << result.label << ','
                  << result.discoveredFiles << ','
                  << result.parsedFiles << ','
                  << result.overviewNodes << ','
                  << result.capabilityNodes << ','
                  << result.capabilityEdges << ','
                  << std::fixed << std::setprecision(2)
                  << result.coldTotalMs << ','
                  << result.hotTotalMs << ','
                  << (result.hotScanHit ? 1 : 0) << ','
                  << (result.hotParseHit ? 1 : 0) << ','
                  << (result.hotAggregateHit ? 1 : 0) << ','
                  << (result.hotLayoutHit ? 1 : 0) << ','
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
