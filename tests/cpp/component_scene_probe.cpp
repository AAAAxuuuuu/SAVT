#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/layout/LayeredGraphLayout.h"
#include "savt/reconstruction/ArchitectureReconstruction.h"
#include "savt/ui/SceneMapper.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantList>
#include <QVariantMap>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace fs = std::filesystem;

struct ProbeArgs {
    fs::path rootPath;
    std::string capabilityFilter;
    fs::path outputPath;
};

std::string normalizePath(const fs::path& path) {
    return path.lexically_normal().generic_string();
}

std::string toUtf8(const QString& value) {
    const QByteArray bytes = value.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

std::string joinStrings(const std::vector<std::string>& values, const std::string_view separator) {
    std::string joined;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            joined += separator;
        }
        joined += values[index];
    }
    return joined;
}

bool containsCaseSensitive(const std::string& haystack, const std::string& needle) {
    return !needle.empty() && haystack.find(needle) != std::string::npos;
}

bool capabilityMatchesFilter(
    const savt::core::CapabilityNode& capability,
    const std::string& filter) {
    if (filter.empty()) {
        return false;
    }

    if (containsCaseSensitive(capability.name, filter) ||
        containsCaseSensitive(capability.summary, filter) ||
        containsCaseSensitive(capability.folderHint, filter)) {
        return true;
    }

    for (const std::string& moduleName : capability.moduleNames) {
        if (containsCaseSensitive(moduleName, filter)) {
            return true;
        }
    }
    return false;
}

std::optional<std::size_t> autoSelectCapabilityId(
    const savt::core::CapabilityGraph& capabilityGraph) {
    for (const savt::core::CapabilityNode& capability : capabilityGraph.nodes) {
        bool hasCore = false;
        bool hasUi = false;
        bool hasLayout = false;
        bool hasAnalyzer = false;
        bool hasParser = false;
        for (const std::string& moduleName : capability.moduleNames) {
            hasCore = hasCore || moduleName.find("src/core") != std::string::npos;
            hasUi = hasUi || moduleName.find("src/ui") != std::string::npos;
            hasLayout = hasLayout || moduleName.find("src/layout") != std::string::npos;
            hasAnalyzer = hasAnalyzer || moduleName.find("src/analyzer") != std::string::npos;
            hasParser = hasParser || moduleName.find("src/parser") != std::string::npos;
        }
        if (hasCore && hasUi && hasLayout && hasAnalyzer && hasParser) {
            return capability.id;
        }
    }
    return std::nullopt;
}

ProbeArgs parseArgs(int argc, char** argv) {
    ProbeArgs args;
    args.rootPath = fs::path(SAVT_PROBE_SOURCE_ROOT);
    args.outputPath = args.rootPath / "tmp" / "component-scene-probe.json";

    if (argc >= 2 && argv[1] != nullptr && std::string_view(argv[1]).size() > 0) {
        args.rootPath = fs::path(argv[1]);
    }
    if (argc >= 3 && argv[2] != nullptr) {
        args.capabilityFilter = argv[2];
    }
    if (argc >= 4 && argv[3] != nullptr && std::string_view(argv[3]).size() > 0) {
        args.outputPath = fs::path(argv[3]);
    }
    return args;
}

void writeJsonFile(const fs::path& filePath, const QVariantMap& payload) {
    fs::create_directories(filePath.parent_path());
    std::ofstream output(filePath, std::ios::binary);
    const QJsonDocument document = QJsonDocument::fromVariant(payload);
    const QByteArray bytes = document.toJson(QJsonDocument::Indented);
    output.write(bytes.constData(), bytes.size());
}

QVariantMap capabilitySummaryMap(const savt::core::CapabilityNode& capability) {
    QVariantList moduleNames;
    for (const std::string& moduleName : capability.moduleNames) {
        moduleNames.push_back(QString::fromStdString(moduleName));
    }

    QVariantMap entry;
    entry.insert("id", static_cast<qulonglong>(capability.id));
    entry.insert("name", QString::fromStdString(capability.name));
    entry.insert("summary", QString::fromStdString(capability.summary));
    entry.insert("folderHint", QString::fromStdString(capability.folderHint));
    entry.insert("moduleCount", static_cast<qulonglong>(capability.moduleCount));
    entry.insert("fileCount", static_cast<qulonglong>(capability.fileCount));
    entry.insert("modules", moduleNames);
    return entry;
}

void printCapabilityList(const savt::core::CapabilityGraph& capabilityGraph) {
    std::cout << "Capabilities:\n";
    for (const savt::core::CapabilityNode& capability : capabilityGraph.nodes) {
        std::cout << "  [" << capability.id << "] " << capability.name
                  << " | modules=" << capability.moduleCount
                  << " | files=" << capability.fileCount
                  << " | moduleNames=" << joinStrings(capability.moduleNames, ", ")
                  << "\n";
    }
}

void printComponentSceneSummary(const QVariantMap& sceneMap) {
    const QVariantList nodes = sceneMap.value("nodes").toList();
    const QVariantList edges = sceneMap.value("edges").toList();
    std::cout << "Component scene nodes=" << nodes.size()
              << " edges=" << edges.size() << "\n";

    for (const QVariant& nodeValue : nodes) {
        const QVariantMap node = nodeValue.toMap();
        std::cout << "  node[" << node.value("id").toULongLong() << "] "
                  << toUtf8(node.value("name").toString())
                  << " | files=" << node.value("fileCount").toULongLong()
                  << " | x=" << node.value("x").toDouble()
                  << " | y=" << node.value("y").toDouble()
                  << "\n";
    }

    for (const QVariant& edgeValue : edges) {
        const QVariantMap edge = edgeValue.toMap();
        std::cout << "  edge[" << edge.value("id").toULongLong() << "] "
                  << edge.value("fromId").toULongLong()
                  << " -> " << edge.value("toId").toULongLong()
                  << " | kind=" << toUtf8(edge.value("kind").toString())
                  << "\n";
    }
}

void printCapabilitySceneSummary(const QVariantMap& sceneMap) {
    const QVariantList nodes = sceneMap.value("nodes").toList();
    const QVariantList edges = sceneMap.value("edges").toList();
    std::cout << "Capability scene nodes=" << nodes.size()
              << " edges=" << edges.size() << "\n";

    for (const QVariant& nodeValue : nodes) {
        const QVariantMap node = nodeValue.toMap();
        std::cout << "  capabilityNode[" << node.value("id").toULongLong() << "] "
                  << toUtf8(node.value("name").toString())
                  << " | x=" << node.value("x").toDouble()
                  << " | y=" << node.value("y").toDouble()
                  << "\n";
    }

    for (const QVariant& edgeValue : edges) {
        const QVariantMap edge = edgeValue.toMap();
        std::cout << "  capabilityEdge[" << edge.value("id").toULongLong() << "] "
                  << edge.value("fromId").toULongLong()
                  << " -> " << edge.value("toId").toULongLong()
                  << " | kind=" << toUtf8(edge.value("kind").toString())
                  << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    const ProbeArgs args = parseArgs(argc, argv);

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;

    const savt::core::AnalysisReport report = analyzer.analyzeProject(args.rootPath, options);
    const savt::core::ArchitectureOverview overview = savt::core::buildArchitectureOverview(report);
    const savt::core::CapabilityGraph capabilityGraph = savt::core::buildCapabilityGraph(report, overview);
    savt::layout::LayeredGraphLayout layoutEngine;
    const savt::layout::CapabilitySceneLayoutResult capabilityLayout =
        layoutEngine.layoutCapabilityScene(capabilityGraph);
    const savt::ui::CapabilitySceneData capabilityScene =
        savt::ui::SceneMapper::buildCapabilitySceneData(capabilityGraph, capabilityLayout);
    const QVariantMap capabilitySceneMap = savt::ui::SceneMapper::toVariantMap(capabilityScene);

    printCapabilityList(capabilityGraph);
    printCapabilitySceneSummary(capabilitySceneMap);

    std::optional<std::size_t> selectedCapabilityId;
    for (const savt::core::CapabilityNode& capability : capabilityGraph.nodes) {
        if (capabilityMatchesFilter(capability, args.capabilityFilter)) {
            selectedCapabilityId = capability.id;
            break;
        }
    }
    if (!selectedCapabilityId.has_value()) {
        selectedCapabilityId = autoSelectCapabilityId(capabilityGraph);
    }
    if (!selectedCapabilityId.has_value() && !capabilityGraph.nodes.empty()) {
        selectedCapabilityId = capabilityGraph.nodes.front().id;
    }

    QVariantList capabilityList;
    for (const savt::core::CapabilityNode& capability : capabilityGraph.nodes) {
        capabilityList.push_back(capabilitySummaryMap(capability));
    }

    QVariantMap payload;
    payload.insert("rootPath", QString::fromStdString(normalizePath(args.rootPath)));
    payload.insert("capabilities", capabilityList);
    payload.insert("capabilityScene", capabilitySceneMap);
    payload.insert("selectedCapabilityId",
                   selectedCapabilityId.has_value()
                       ? QVariant::fromValue(static_cast<qulonglong>(selectedCapabilityId.value()))
                       : QVariant());

    if (selectedCapabilityId.has_value()) {
        const savt::reconstruction::CapabilityDrilldown drilldown =
            savt::reconstruction::buildCapabilityDrilldown(
                report,
                overview,
                capabilityGraph,
                selectedCapabilityId.value());
        const savt::ui::ComponentSceneData scene =
            savt::ui::SceneMapper::buildComponentSceneData(drilldown.graph, drilldown.layout);
        const QVariantMap sceneMap = savt::ui::SceneMapper::toVariantMap(scene);
        payload.insert("componentScene", sceneMap);
        printComponentSceneSummary(sceneMap);
    }

    writeJsonFile(args.outputPath, payload);
    std::cout << "Wrote probe output to " << normalizePath(args.outputPath) << "\n";
    return 0;
}
