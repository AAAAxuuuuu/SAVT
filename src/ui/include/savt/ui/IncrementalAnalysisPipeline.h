#pragma once

#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ArchitectureAggregation.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/core/ComponentGraph.h"
#include "savt/layout/LayeredGraphLayout.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace savt::ui {

struct IncrementalCacheLayerState {
    bool hit = false;
    std::string key;
};

struct AnalysisArtifacts {
    savt::core::AnalysisReport report;
    savt::core::ArchitectureOverview overview;
    savt::core::CapabilityGraph capabilityGraph;
    savt::core::ArchitectureRuleReport ruleReport;
    savt::layout::CapabilitySceneLayoutResult capabilitySceneLayout;
    savt::layout::LayoutResult moduleLayout;
    std::unordered_map<std::size_t, savt::core::ComponentGraph> componentGraphs;
    std::unordered_map<std::size_t, savt::layout::ComponentSceneLayoutResult> componentLayouts;
    IncrementalCacheLayerState scanLayer;
    IncrementalCacheLayerState parseLayer;
    IncrementalCacheLayerState aggregateLayer;
    IncrementalCacheLayerState layoutLayer;
    std::vector<std::string> cacheDiagnostics;
    bool canceled = false;
    std::string canceledPhase;
};

using IncrementalAnalysisArtifacts = AnalysisArtifacts;

using IncrementalProgressCallback =
    std::function<void(int value, const std::string& label)>;

class IncrementalAnalysisPipeline {
public:
    static AnalysisArtifacts analyze(
        const std::filesystem::path& rootPath,
        const savt::analyzer::AnalyzerOptions& options,
        IncrementalProgressCallback progress = {});

    static void clear();
    static std::string cacheVersion();
};

}  // namespace savt::ui
