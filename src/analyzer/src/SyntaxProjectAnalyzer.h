#pragma once

#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ArchitectureGraph.h"

#include <filesystem>

namespace savt::analyzer::detail {

savt::core::AnalysisReport analyzeSyntaxProject(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options);

}  // namespace savt::analyzer::detail
