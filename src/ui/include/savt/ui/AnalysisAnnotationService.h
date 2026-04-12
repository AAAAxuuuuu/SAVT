#pragma once

#include "savt/core/ArchitectureGraph.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/core/ComponentGraph.h"

#include <QtGlobal>
#include <QString>

#include <cstddef>
#include <unordered_map>

namespace savt::ui {

struct AnalysisAnnotationStats {
    bool configured = false;
    bool applied = false;
    std::size_t capabilityNodeCount = 0;
    std::size_t componentNodeCount = 0;
    std::size_t requestCount = 0;
    std::size_t cacheHitCount = 0;
    std::size_t skippedComponentGraphCount = 0;
    qint64 totalElapsedMs = 0;
    QString model;
    QString statusMessage;
};

class AnalysisAnnotationService {
public:
    static AnalysisAnnotationStats annotateCapabilityGraph(
        const QString& projectRootPath,
        const savt::core::AnalysisReport& report,
        savt::core::CapabilityGraph& capabilityGraph);
    static AnalysisAnnotationStats annotateComponentGraph(
        const QString& projectRootPath,
        const savt::core::AnalysisReport& report,
        savt::core::ComponentGraph& componentGraph);
    static AnalysisAnnotationStats annotate(
        const QString& projectRootPath,
        const savt::core::AnalysisReport& report,
        savt::core::CapabilityGraph& capabilityGraph,
        std::unordered_map<std::size_t, savt::core::ComponentGraph>& componentGraphs);
};

}  // namespace savt::ui
