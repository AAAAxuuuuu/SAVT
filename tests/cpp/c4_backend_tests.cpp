#include "AnalyzerUtilities.h"
#include "AnalysisGraphBuilder.h"

#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/layout/LayeredGraphLayout.h"
#include "savt/ui/SceneMapper.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void expect(const bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << "\n";
        std::exit(1);
    }
}

bool containsText(const std::vector<std::string>& values, const std::string_view expected) {
    return std::find(values.begin(), values.end(), expected) != values.end();
}

const savt::core::OverviewNode* findOverviewNode(
    const savt::core::ArchitectureOverview& overview,
    const std::string_view name) {
    const auto iterator = std::find_if(overview.nodes.begin(), overview.nodes.end(), [&](const savt::core::OverviewNode& node) {
        return node.name == name;
    });
    return iterator == overview.nodes.end() ? nullptr : &(*iterator);
}

const savt::core::CapabilityNode* findCapabilityByModule(
    const savt::core::CapabilityGraph& graph,
    const std::string_view moduleName) {
    const auto iterator = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&](const savt::core::CapabilityNode& node) {
        return containsText(node.moduleNames, std::string(moduleName));
    });
    return iterator == graph.nodes.end() ? nullptr : &(*iterator);
}

const savt::core::SymbolNode* findReportModule(
    const savt::core::AnalysisReport& report,
    const std::string_view displayName) {
    const auto iterator = std::find_if(report.nodes.begin(), report.nodes.end(), [&](const savt::core::SymbolNode& node) {
        return node.kind == savt::core::SymbolKind::Module && node.displayName == displayName;
    });
    return iterator == report.nodes.end() ? nullptr : &(*iterator);
}

std::vector<const savt::core::SymbolNode*> findNodesByQualifiedName(
    const savt::core::AnalysisReport& report,
    const std::string_view qualifiedName) {
    std::vector<const savt::core::SymbolNode*> matches;
    for (const savt::core::SymbolNode& node : report.nodes) {
        if (node.qualifiedName == qualifiedName) {
            matches.push_back(&node);
        }
    }
    return matches;
}

const savt::core::SymbolNode* findSingleNodeByQualifiedName(
    const savt::core::AnalysisReport& report,
    const std::string_view qualifiedName) {
    const auto matches = findNodesByQualifiedName(report, qualifiedName);
    return matches.size() == 1 ? matches.front() : nullptr;
}

bool hasSymbolEdge(
    const savt::core::AnalysisReport& report,
    const std::size_t fromId,
    const std::size_t toId,
    const savt::core::EdgeKind kind) {
    return std::any_of(report.edges.begin(), report.edges.end(), [&](const savt::core::SymbolEdge& edge) {
        return edge.kind == kind && edge.fromId == fromId && edge.toId == toId;
    });
}

std::size_t countSymbolEdges(
    const savt::core::AnalysisReport& report,
    const std::size_t fromId,
    const std::size_t toId,
    const savt::core::EdgeKind kind) {
    return static_cast<std::size_t>(std::count_if(
        report.edges.begin(),
        report.edges.end(),
        [&](const savt::core::SymbolEdge& edge) {
            return edge.kind == kind && edge.fromId == fromId && edge.toId == toId;
        }));
}

bool hasModuleDependency(
    const savt::core::AnalysisReport& report,
    const std::string_view fromName,
    const std::string_view toName) {
    const auto* fromNode = findReportModule(report, fromName);
    const auto* toNode = findReportModule(report, toName);
    if (fromNode == nullptr || toNode == nullptr) {
        return false;
    }

    return std::any_of(report.edges.begin(), report.edges.end(), [&](const savt::core::SymbolEdge& edge) {
        return edge.kind == savt::core::EdgeKind::DependsOn &&
               edge.fromId == fromNode->id &&
               edge.toId == toNode->id;
    });
}

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

std::string jsonPath(const std::filesystem::path& filePath) {
    return filePath.generic_string();
}

std::string firstDiagnosticOrNone(const savt::core::AnalysisReport& report) {
    return report.diagnostics.empty() ? std::string("<none>") : report.diagnostics.front();
}

bool isBuildTimeSemanticBackendUnavailableCode(const std::string& statusCode) {
    return statusCode == "backend_unavailable" ||
           statusCode == "llvm_not_found" ||
           statusCode == "llvm_headers_missing" ||
           statusCode == "libclang_not_found";
}

std::string symbolNameById(const savt::core::AnalysisReport& report, const std::size_t nodeId) {
    const auto iterator = std::find_if(report.nodes.begin(), report.nodes.end(), [&](const savt::core::SymbolNode& node) {
        return node.id == nodeId;
    });
    if (iterator == report.nodes.end()) {
        return "<missing>";
    }
    return iterator->qualifiedName.empty() ? iterator->displayName : iterator->qualifiedName;
}

std::string describeEdgesOfKind(const savt::core::AnalysisReport& report, const savt::core::EdgeKind kind) {
    std::string description;
    for (const savt::core::SymbolEdge& edge : report.edges) {
        if (edge.kind != kind) {
            continue;
        }
        if (!description.empty()) {
            description += "; ";
        }
        description += symbolNameById(report, edge.fromId) + " -> " + symbolNameById(report, edge.toId);
    }
    return description.empty() ? std::string("<none>") : description;
}

const savt::core::SymbolEdge* findSingleEdge(
    const savt::core::AnalysisReport& report,
    const std::size_t fromId,
    const std::size_t toId,
    const savt::core::EdgeKind kind) {
    const savt::core::SymbolEdge* match = nullptr;
    for (const savt::core::SymbolEdge& edge : report.edges) {
        if (edge.kind != kind || edge.fromId != fromId || edge.toId != toId) {
            continue;
        }
        if (match != nullptr) {
            return nullptr;
        }
        match = &edge;
    }
    return match;
}

std::string factSourceName(const savt::core::FactSource source) {
    return savt::core::toString(source);
}

void testAdaptiveModuleInference() {
    const std::vector<std::string> paths = {
        "App/main.cpp",
        "App/backend/algorithm/AlgorithmLibrary.cpp",
        "App/backend/algorithm/AlgorithmLibrary.h",
        "App/backend/editor/NetworkEditor.cpp",
        "App/backend/editor/NetworkEditor.h",
        "App/backend/graph/GraphManager.cpp",
        "App/backend/graph/GraphManager.h",
        "App/bootstrap/EngineSetup.cpp",
        "TrafficMap/MapShell.qml",
        "TrafficMapContent/Screen01.ui.qml",
        "Resources/data/amap.json",
        "tools/build_core_graph.py",
        "mainSystem/CMakeLists.txt",
        "mainSystem/main.cpp",
        "mainSystem/mainwindow.cpp",
        "mainSystem/mainwindow.h",
        "mainSystem/LoginDlg.cpp",
        "mainSystem/LoginDlg.h",
        "mainSystem/trainmanagement.cpp",
        "mainSystem/trainmanagement.h",
        "mainSystem/stationmanagement.cpp",
        "mainSystem/stationmanagement.h",
        "mainSystem/prediction.cpp",
        "mainSystem/prediction.h",
        "src/main/java/com/example/schedule/ScheduleApplication.java",
        "src/main/java/com/example/schedule/controller/ScheduleController.java",
        "src/main/java/com/example/schedule/service/LoginService.java",
        "QXlsx-1.5.0/QXlsx/source/xlsxdocument.cpp",
        "QXlsx-1.5.0/QXlsx/header/xlsxdocument.h",
        "QXlsx-1.5.0/HelloWorld/main.cpp",
        "QXlsx-1.5.0/Pump/main.cpp"
    };

    const auto modules = savt::analyzer::detail::inferModuleNames(paths);
    expect(modules.at("App/backend/algorithm/AlgorithmLibrary.cpp") == "App/backend/algorithm",
           "algorithm files should be split into a dedicated backend module");
    expect(modules.at("App/backend/editor/NetworkEditor.cpp") == "App/backend/editor",
           "editor files should be split into a dedicated backend module");
    expect(modules.at("App/bootstrap/EngineSetup.cpp") == "App/bootstrap",
           "bootstrap files should keep a focused module boundary");
    expect(modules.at("TrafficMap/MapShell.qml") == "TrafficMap",
           "qml root folder should remain a top-level module");
    expect(modules.at("tools/build_core_graph.py") == "tools",
           "tooling scripts should surface as a tooling module");
    expect(modules.at("mainSystem/main.cpp") == "mainSystem",
           "flat application roots should keep their bootstrap files inside the primary app module");
    expect(modules.at("mainSystem/mainwindow.cpp") == "mainSystem/mainwindow",
           "flat application roots should split direct component files into component-level modules");
    expect(modules.at("mainSystem/LoginDlg.cpp") == "mainSystem/logindlg",
           "flat application roots should preserve dialog-level component modules");
    expect(modules.at("QXlsx-1.5.0/HelloWorld/main.cpp") == "QXlsx-1.5.0",
           "third-party sample apps should collapse into a single dependency module");
    expect(modules.at("QXlsx-1.5.0/QXlsx/source/xlsxdocument.cpp") == "QXlsx-1.5.0",
           "third-party library sources should stay inside the dependency boundary");
    expect(modules.at("src/main/java/com/example/schedule/ScheduleApplication.java") == "src/main/bootstrap",
           "spring boot application entry should be grouped into a bootstrap module, got " +
               modules.at("src/main/java/com/example/schedule/ScheduleApplication.java"));
    expect(modules.at("src/main/java/com/example/schedule/controller/ScheduleController.java") == "src/main/controller",
           "java controllers should be grouped by business package instead of collapsing into src/main");
    expect(modules.at("src/main/java/com/example/schedule/service/LoginService.java") == "src/main/service",
           "java services should be grouped by business package instead of collapsing into src/main");
}

void testDependencyAndPrimaryEntryClassification() {
    using savt::core::AnalysisReport;
    using savt::core::EdgeKind;
    using savt::core::SymbolKind;

    AnalysisReport report;
    report.rootPath = "synthetic";
    report.discoveredFiles = 8;
    report.parsedFiles = 6;

    report.nodes = {
        {1, SymbolKind::Module, "mainSystem", "module:mainSystem", "module:mainSystem", "mainSystem", 0},
        {2, SymbolKind::Module, "mainSystem/mainwindow", "module:mainSystem/mainwindow", "module:mainSystem/mainwindow", "mainSystem/mainwindow", 0},
        {3, SymbolKind::Module, "QXlsx-1.5.0", "module:QXlsx-1.5.0", "module:QXlsx-1.5.0", "QXlsx-1.5.0", 0},
        {10, SymbolKind::File, "main.cpp", "mainSystem/main.cpp", "file:mainSystem/main.cpp", "mainSystem/main.cpp", 0},
        {11, SymbolKind::File, "mainwindow.cpp", "mainSystem/mainwindow.cpp", "file:mainSystem/mainwindow.cpp", "mainSystem/mainwindow.cpp", 0},
        {12, SymbolKind::File, "xlsxdocument.cpp", "QXlsx-1.5.0/QXlsx/source/xlsxdocument.cpp", "file:QXlsx-1.5.0/QXlsx/source/xlsxdocument.cpp", "QXlsx-1.5.0/QXlsx/source/xlsxdocument.cpp", 0},
        {13, SymbolKind::File, "sample_main.cpp", "QXlsx-1.5.0/HelloWorld/main.cpp", "file:QXlsx-1.5.0/HelloWorld/main.cpp", "QXlsx-1.5.0/HelloWorld/main.cpp", 0},
        {20, SymbolKind::Function, "main", "main", "symbol:main", "mainSystem/main.cpp", 1},
        {21, SymbolKind::Class, "MainWindow", "MainWindow", "symbol:MainWindow", "mainSystem/mainwindow.cpp", 1},
        {22, SymbolKind::Class, "Document", "Document", "symbol:Document", "QXlsx-1.5.0/QXlsx/source/xlsxdocument.cpp", 1},
        {23, SymbolKind::Function, "main", "main", "symbol:sample_main", "QXlsx-1.5.0/HelloWorld/main.cpp", 1}
    };

    report.edges = {
        {1, 10, EdgeKind::Contains, 1},
        {2, 11, EdgeKind::Contains, 1},
        {3, 12, EdgeKind::Contains, 1},
        {3, 13, EdgeKind::Contains, 1},
        {1, 2, EdgeKind::DependsOn, 3},
        {1, 3, EdgeKind::DependsOn, 2},
        {2, 3, EdgeKind::DependsOn, 1}
    };

    const auto overview = savt::core::buildArchitectureOverview(report);
    const auto capabilityGraph = savt::core::buildCapabilityGraph(report, overview);

    const auto* appEntry = findOverviewNode(overview, "mainSystem");
    expect(appEntry != nullptr, "overview should retain the mainSystem entry module");
    expect(appEntry->kind == savt::core::OverviewNodeKind::EntryPointModule,
           "mainSystem should remain the primary entry instead of being displaced by dependencies");

    const auto* mainWindowNode = findOverviewNode(overview, "mainSystem/MainWindow");
    expect(mainWindowNode != nullptr, "flat app modules should be renamed from top symbols into readable component names");
    expect(mainWindowNode->role == "presentation",
           "mainwindow component should be recognized as presentation/UI logic");

    const auto* dependencyNode = findOverviewNode(overview, "QXlsx-1.5.0");
    expect(dependencyNode != nullptr, "overview should keep the vendored dependency visible as one collapsed node");
    expect(dependencyNode->role == "dependency",
           "vendored libraries should be classified as dependency rather than business logic");
    expect(dependencyNode->kind != savt::core::OverviewNodeKind::EntryPointModule,
           "dependency samples must not become entry modules");

    const auto* dependencyCapability = findCapabilityByModule(capabilityGraph, "QXlsx-1.5.0");
    expect(dependencyCapability != nullptr, "capability graph should preserve the dependency node");
    expect(dependencyCapability->kind == savt::core::CapabilityNodeKind::Infrastructure,
           "dependency nodes should surface as infrastructure/support");
}

void testCrossArtifactOverviewAndCapabilityGraph() {
    using savt::core::AnalysisReport;
    using savt::core::EdgeKind;
    using savt::core::SymbolKind;
    using savt::core::SymbolNode;
    using savt::core::SymbolEdge;

    AnalysisReport report;
    report.rootPath = "synthetic";
    report.primaryEngine = "tree-sitter";
    report.precisionLevel = "syntax";
    report.semanticAnalysisRequested = true;
    report.semanticAnalysisEnabled = false;
    report.discoveredFiles = 7;
    report.parsedFiles = 4;

    report.nodes = {
        {1, SymbolKind::Module, "App", "module:App", "module:App", "App", 0},
        {2, SymbolKind::Module, "App/backend/facade", "module:App/backend/facade", "module:App/backend/facade", "App/backend/facade", 0},
        {3, SymbolKind::Module, "App/backend/algorithm", "module:App/backend/algorithm", "module:App/backend/algorithm", "App/backend/algorithm", 0},
        {4, SymbolKind::Module, "TrafficMap", "module:TrafficMap", "module:TrafficMap", "TrafficMap", 0},
        {5, SymbolKind::Module, "Resources", "module:Resources", "module:Resources", "Resources", 0},
        {6, SymbolKind::Module, "tools", "module:tools", "module:tools", "tools", 0},
        {10, SymbolKind::File, "main.cpp", "App/main.cpp", "file:App/main.cpp", "App/main.cpp", 0},
        {11, SymbolKind::File, "TrafficBackend.cpp", "App/backend/facade/TrafficBackend.cpp", "file:App/backend/facade/TrafficBackend.cpp", "App/backend/facade/TrafficBackend.cpp", 0},
        {12, SymbolKind::File, "AlgorithmLibrary.cpp", "App/backend/algorithm/AlgorithmLibrary.cpp", "file:App/backend/algorithm/AlgorithmLibrary.cpp", "App/backend/algorithm/AlgorithmLibrary.cpp", 0},
        {13, SymbolKind::File, "MapShell.qml", "TrafficMap/MapShell.qml", "file:TrafficMap/MapShell.qml", "TrafficMap/MapShell.qml", 0},
        {14, SymbolKind::File, "amap.json", "Resources/amap.json", "file:Resources/amap.json", "Resources/amap.json", 0},
        {15, SymbolKind::File, "build_core_graph.py", "tools/build_core_graph.py", "file:tools/build_core_graph.py", "tools/build_core_graph.py", 0},
        {20, SymbolKind::Function, "main", "main", "symbol:main", "App/main.cpp", 1},
        {21, SymbolKind::Class, "TrafficBackend", "TrafficBackend", "symbol:TrafficBackend", "App/backend/facade/TrafficBackend.cpp", 1},
        {22, SymbolKind::Class, "AlgorithmLibrary", "AlgorithmLibrary", "symbol:AlgorithmLibrary", "App/backend/algorithm/AlgorithmLibrary.cpp", 1}
    };

    report.edges = {
        {1, 10, EdgeKind::Contains, 1},
        {2, 11, EdgeKind::Contains, 1},
        {3, 12, EdgeKind::Contains, 1},
        {4, 13, EdgeKind::Contains, 1},
        {5, 14, EdgeKind::Contains, 1},
        {6, 15, EdgeKind::Contains, 1},
        {1, 2, EdgeKind::DependsOn, 2},
        {2, 3, EdgeKind::DependsOn, 3},
        {4, 2, EdgeKind::DependsOn, 2},
        {2, 5, EdgeKind::DependsOn, 1},
        {6, 5, EdgeKind::DependsOn, 1}
    };

    const auto overview = savt::core::buildArchitectureOverview(report);
    const auto capabilityGraph = savt::core::buildCapabilityGraph(report, overview);

    const auto* qmlNode = findOverviewNode(overview, "TrafficMap");
    expect(qmlNode != nullptr, "overview should contain the TrafficMap module");
    expect(qmlNode->role == "presentation", "qml-heavy module should be classified as presentation");

    const auto* toolingNode = findOverviewNode(overview, "tools");
    expect(toolingNode != nullptr, "overview should contain the tools module");
    expect(toolingNode->role == "tooling", "python module should be classified as tooling");

    const auto* resourceNode = findOverviewNode(overview, "Resources");
    expect(resourceNode != nullptr, "overview should contain the resources module");
    expect(resourceNode->role == "data" || resourceNode->role == "storage",
           "resource module should be classified as data/storage");

    expect(std::any_of(overview.diagnostics.begin(), overview.diagnostics.end(), [](const std::string& diagnostic) {
               return diagnostic.find("Cross-artifact inventory") != std::string::npos;
           }),
           "overview diagnostics should mention cross-artifact inventory");

    expect(capabilityGraph.nodes.size() >= overview.nodes.size(),
           "module-scoped capability graph should preserve most module detail");
    expect(std::any_of(capabilityGraph.diagnostics.begin(), capabilityGraph.diagnostics.end(), [](const std::string& diagnostic) {
               return diagnostic.find("module_scoped") != std::string::npos;
           }),
           "capability diagnostics should expose the aggregation mode");

    const auto* facadeNode = findCapabilityByModule(capabilityGraph, "App/backend/facade");
    expect(facadeNode != nullptr, "capability graph should keep the facade module visible as its own node");
    expect(facadeNode->collaboratorCount > 0, "facade node should keep direct collaborators");
    expect(!facadeNode->exampleFiles.empty(), "facade node should expose example files for traceability");

    const auto* toolsCapability = findCapabilityByModule(capabilityGraph, "tools");
    expect(toolsCapability != nullptr, "tooling capability should be preserved");
    expect(toolsCapability->kind == savt::core::CapabilityNodeKind::Infrastructure,
           "tooling module should surface as infrastructure/support in the capability graph");

    savt::layout::LayeredGraphLayout layoutEngine;
    const auto sceneLayout = layoutEngine.layoutCapabilityScene(capabilityGraph);
    expect(sceneLayout.width > 0.0 && sceneLayout.height > 0.0,
           "capability scene layout should expose a non-empty canvas");
    expect(sceneLayout.nodes.size() == capabilityGraph.nodes.size(),
           "capability scene layout should position every visible capability node");
    expect(sceneLayout.edges.size() == capabilityGraph.edges.size(),
           "capability scene layout should route every visible capability edge");
    expect(sceneLayout.groups.size() == capabilityGraph.groups.size(),
           "capability scene layout should preserve every capability group");
    expect(std::all_of(sceneLayout.nodes.begin(), sceneLayout.nodes.end(), [](const savt::layout::CapabilitySceneNodeLayout& node) {
               return node.width > 0.0 && node.height > 0.0;
           }),
           "capability scene layout should assign positive bounds to every positioned node");
    expect(std::all_of(sceneLayout.edges.begin(), sceneLayout.edges.end(), [](const savt::layout::CapabilitySceneEdgeLayout& edge) {
               return edge.routePoints.size() >= 2;
           }),
           "capability scene layout should emit route points for every visible edge");
    expect(std::all_of(sceneLayout.groups.begin(), sceneLayout.groups.end(), [](const savt::layout::CapabilitySceneGroupLayout& group) {
               return !group.hasBounds || (group.width > 0.0 && group.height > 0.0);
           }),
           "capability scene layout should assign positive bounds to every visible group");
}

void testCapabilitySceneMapperPublishesSingleScenePayload() {
    savt::core::CapabilityGraph graph;

    savt::core::CapabilityNode entryNode;
    entryNode.id = 1;
    entryNode.kind = savt::core::CapabilityNodeKind::Entry;
    entryNode.name = "CLI";
    entryNode.defaultPinned = true;
    entryNode.visualPriority = 5;
    entryNode.laneGroupId = 100;
    graph.nodes.push_back(entryNode);

    savt::core::CapabilityNode capabilityNode;
    capabilityNode.id = 2;
    capabilityNode.kind = savt::core::CapabilityNodeKind::Capability;
    capabilityNode.name = "Analyzer";
    capabilityNode.collaboratorCount = 4;
    capabilityNode.laneGroupId = 101;
    graph.nodes.push_back(capabilityNode);

    savt::core::CapabilityEdge edge;
    edge.id = 10;
    edge.fromId = 1;
    edge.toId = 2;
    edge.kind = savt::core::CapabilityEdgeKind::Activates;
    edge.summary = "CLI kicks off analysis";
    graph.edges.push_back(edge);

    savt::core::CapabilityGroup entryGroup;
    entryGroup.id = 100;
    entryGroup.kind = savt::core::CapabilityGroupKind::Lane;
    entryGroup.name = "Entry lane";
    entryGroup.nodeIds = {1};
    graph.groups.push_back(entryGroup);

    savt::core::CapabilityGroup capabilityGroup;
    capabilityGroup.id = 101;
    capabilityGroup.kind = savt::core::CapabilityGroupKind::Lane;
    capabilityGroup.name = "Capability lane";
    capabilityGroup.nodeIds = {2};
    graph.groups.push_back(capabilityGroup);

    savt::layout::LayeredGraphLayout layoutEngine;
    const auto layout = layoutEngine.layoutCapabilityScene(graph);
    const auto sceneData = savt::ui::SceneMapper::buildCapabilitySceneData(graph, layout);
    const QVariantMap sceneMap = savt::ui::SceneMapper::toVariantMap(sceneData);

    const QVariantMap bounds = sceneMap.value("bounds").toMap();
    expect(sceneMap.value("nodes").toList().size() == 2,
           "scene map should publish all capability nodes in a single payload");
    expect(sceneMap.value("edges").toList().size() == 1,
           "scene map should publish all capability edges in a single payload");
    expect(sceneMap.value("groups").toList().size() == 2,
           "scene map should publish all capability groups in a single payload");
    expect(bounds.value("width").toDouble() == sceneData.sceneWidth,
           "scene payload bounds should reuse scene width from the layout result");
    expect(bounds.value("height").toDouble() == sceneData.sceneHeight,
           "scene payload bounds should reuse scene height from the layout result");

    const QVariantList nodeItems = sceneMap.value("nodes").toList();
    const QVariantMap firstNode = nodeItems.front().toMap();
    expect(firstNode.contains("layoutBounds"),
           "scene payload nodes should expose layout bounds from the layout layer");

    const QVariantList edgeItems = sceneMap.value("edges").toList();
    const QVariantMap firstEdge = edgeItems.front().toMap();
    expect(firstEdge.value("routePointCount").toULongLong() >= 2,
           "scene payload edges should expose routed path points from the layout layer");
}

void testAnalysisReportSerializesFactSources() {
    using savt::core::AnalysisReport;
    using savt::core::EdgeKind;
    using savt::core::FactSource;
    using savt::core::SymbolKind;

    AnalysisReport report;
    report.rootPath = "synthetic";
    report.primaryEngine = "libclang-cindex";
    report.precisionLevel = "semantic";
    report.semanticStatusCode = "semantic_ready";
    report.semanticAnalysisRequested = true;
    report.semanticAnalysisEnabled = true;
    report.nodes = {
        {1, SymbolKind::Function, "compute", "demo::compute", "usr:compute", "demo.cpp", 2, FactSource::Semantic},
        {2, SymbolKind::Function, "fallback", "demo::fallback", "usr:fallback", "demo.cpp", 8, FactSource::Inferred}
    };
    report.edges = {
        {1, 2, EdgeKind::Calls, 1, 3, FactSource::Semantic},
        {2, 1, EdgeKind::DependsOn, 2, 1, FactSource::Inferred}
    };

    const std::string text = savt::core::formatAnalysisReport(report, 10, 10, 10);
    const std::string json = savt::core::serializeAnalysisReportToJson(report, true);

    expect(text.find("[semantic] [function] demo::compute") != std::string::npos,
           "text report should label semantic nodes");
    expect(text.find("[semantic] calls: demo::compute -> demo::fallback (support=3)") != std::string::npos,
           "text report should expose semantic edge factSource and support count");
    expect(text.find("[inferred] fallback -> compute (weight=2)") != std::string::npos,
           "module dependency sample should retain inferred fact labeling");
    expect(json.find("\"factSource\": \"semantic\"") != std::string::npos,
           "json report should serialize semantic factSource");
    expect(json.find("\"factSource\": \"inferred\"") != std::string::npos,
           "json report should serialize inferred factSource");
    expect(json.find("\"supportCount\": 3") != std::string::npos,
           "json report should serialize edge support counts");
    expect(json.find("\"semanticNodeCount\": 1") != std::string::npos,
           "json report summary should expose semantic node counts");
}

void testAnalysisGraphBuilderPreservesDistinctSemanticOverloads() {
    using savt::analyzer::AnalyzerOptions;
    using savt::analyzer::detail::AnalysisGraphBuilder;
    using savt::core::FactSource;
    using savt::core::SymbolKind;

    AnalysisGraphBuilder builder(std::filesystem::current_path(), AnalyzerOptions{});

    const std::size_t syntaxDeclId = builder.addOrMergeNode(
        SymbolKind::Function,
        "helper",
        "demo::helper",
        "demo.cpp",
        1);
    const std::size_t syntaxDefId = builder.addOrMergeNode(
        SymbolKind::Function,
        "helper",
        "demo::helper",
        "demo.cpp",
        12);
    expect(syntaxDeclId == syntaxDefId,
           "syntax-level duplicates should still merge by qualifiedName when the symbol is otherwise unambiguous");
    expect(builder.findSymbolByQualifiedName("demo::helper").has_value() &&
               *builder.findSymbolByQualifiedName("demo::helper") == syntaxDeclId,
           "unique qualified names should remain directly resolvable");

    const std::size_t intOverloadId = builder.addOrMergeNode(
        SymbolKind::Function,
        "compute",
        "demo::compute",
        "demo.cpp",
        20,
        "usr:demo::compute#int",
        FactSource::Semantic);
    const std::size_t doubleOverloadId = builder.addOrMergeNode(
        SymbolKind::Function,
        "compute",
        "demo::compute",
        "demo.cpp",
        28,
        "usr:demo::compute#double",
        FactSource::Semantic);
    expect(intOverloadId != doubleOverloadId,
           "semantic overloads with different identities must not merge just because they share a qualifiedName");
    expect(findNodesByQualifiedName(builder.report(), "demo::compute").size() == 2,
           "distinct semantic overloads should remain as two logical nodes in the report");
    expect(builder.findSymbolsByQualifiedName("demo::compute").size() == 2,
           "builder qualified-name lookups should retain all overload candidates for downstream resolvers");
    expect(!builder.findSymbolByQualifiedName("demo::compute").has_value(),
           "ambiguous qualified names should no longer resolve to an arbitrary node");

    const std::size_t repeatIntOverloadId = builder.addOrMergeNode(
        SymbolKind::Function,
        "compute",
        "demo::compute",
        "demo.cpp",
        33,
        "usr:demo::compute#int",
        FactSource::Semantic);
    expect(repeatIntOverloadId == intOverloadId,
           "semantic nodes should still merge by identity when the same overload is seen again");

    const auto computeNodes = findNodesByQualifiedName(builder.report(), "demo::compute");
    expect(std::all_of(computeNodes.begin(), computeNodes.end(), [](const savt::core::SymbolNode* node) {
               return node != nullptr && node->factSource == savt::core::FactSource::Semantic;
           }),
           "preserved overload nodes should retain semantic fact labeling");
}

void testNodeBackendRelativeImportsAndRoleClassification() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = fs::temp_directory_path() /
                              ("savt_js_backend_test_" + std::to_string(std::rand()));
    std::error_code errorCode;
    fs::remove_all(tempRoot, errorCode);
    fs::create_directories(tempRoot, errorCode);

    writeTextFile(tempRoot / "package.json", R"JSON({
  "name": "schedule-service",
  "main": "src/server.js"
})JSON");
    writeTextFile(tempRoot / "src/server.js", R"JS(
const express = require('express');
const scheduleRoutes = require('./routes/schedule');

function bootstrapServer() {
  return express();
}

module.exports = { bootstrapServer, scheduleRoutes };
)JS");
    writeTextFile(tempRoot / "src/routes/schedule.js", R"JS(
const controller = require('../controllers/scheduleController');

function registerScheduleRoutes(app) {
  return controller.getSchedule(app);
}

module.exports = { registerScheduleRoutes };
)JS");
    writeTextFile(tempRoot / "src/controllers/scheduleController.js", R"JS(
const scheduleService = require('../services/scheduleService');

async function getSchedule() {
  return scheduleService.loadSchedule();
}

module.exports = { getSchedule };
)JS");
    writeTextFile(tempRoot / "src/services/scheduleService.js", R"JS(
const cacheStore = require('../stores/cacheStore');

const loadSchedule = async () => cacheStore.readSchedule();

module.exports = { loadSchedule };
)JS");
    writeTextFile(tempRoot / "src/stores/cacheStore.js", R"JS(
async function readSchedule() {
  return [];
}

module.exports = { readSchedule };
)JS");

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;
    const auto report = analyzer.analyzeProject(tempRoot, options);

    expect(report.parsedFiles >= 5,
           "js backend files should be parsed by the heuristic analyzer");
    expect(hasModuleDependency(report, "src/server", "src/routes"),
           "relative require() should create a dependency from server to routes");
    expect(hasModuleDependency(report, "src/routes", "src/controllers"),
           "relative require() should create a dependency from routes to controllers");
    expect(hasModuleDependency(report, "src/controllers", "src/services"),
           "relative require() should create a dependency from controllers to services");
    expect(hasModuleDependency(report, "src/services", "src/stores"),
           "relative require() should create a dependency from services to stores");

    const auto overview = savt::core::buildArchitectureOverview(report);

    const auto* entryNode = findOverviewNode(overview, "src/server");
    expect(entryNode != nullptr, "node backend overview should include the server entry module");
    expect(entryNode->kind == savt::core::OverviewNodeKind::EntryPointModule,
           "server.js based module should be recognized as an entry point");
    expect(entryNode->role == "interaction",
           "server/router style js modules should be classified as backend interaction rather than web assets");

    const auto* serviceNode = findOverviewNode(overview, "src/services");
    expect(serviceNode != nullptr, "node backend overview should include the services module");
    expect(serviceNode->role == "analysis",
           "service modules in js backends should be classified as analysis/business logic");

    const auto* storeNode = findOverviewNode(overview, "src/stores");
    expect(storeNode != nullptr, "node backend overview should include the stores module");
    expect(storeNode->role == "storage",
           "store/cache modules in js backends should be classified as storage");

    fs::remove_all(tempRoot, errorCode);
}

void testSpringBootJavaImportsAndRoleClassification() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = fs::temp_directory_path() /
                              ("savt_java_backend_test_" + std::to_string(std::rand()));
    std::error_code errorCode;
    fs::remove_all(tempRoot, errorCode);
    fs::create_directories(tempRoot, errorCode);

    writeTextFile(tempRoot / "pom.xml", R"XML(
<project>
  <modelVersion>4.0.0</modelVersion>
  <groupId>com.example</groupId>
  <artifactId>schedule</artifactId>
</project>
)XML");
    writeTextFile(tempRoot / "src/main/java/com/example/schedule/ScheduleApplication.java", R"JAVA(
package com.example.schedule;

import org.springframework.boot.autoconfigure.SpringBootApplication;

@SpringBootApplication
public class ScheduleApplication {}
)JAVA");
    writeTextFile(tempRoot / "src/main/java/com/example/schedule/controller/ScheduleController.java", R"JAVA(
package com.example.schedule.controller;

import com.example.schedule.service.GradeService;
import com.example.schedule.service.LoginService;
import org.springframework.web.bind.annotation.RestController;

@RestController
public class ScheduleController {
  private final LoginService loginService;
  private final GradeService gradeService;
}
)JAVA");
    writeTextFile(tempRoot / "src/main/java/com/example/schedule/service/LoginService.java", R"JAVA(
package com.example.schedule.service;

import com.example.schedule.model.Course;
import org.springframework.stereotype.Service;

@Service
public class LoginService {
  private Course latestCourse;
}
)JAVA");
    writeTextFile(tempRoot / "src/main/java/com/example/schedule/service/GradeService.java", R"JAVA(
package com.example.schedule.service;

import com.example.schedule.model.Course;
import org.springframework.stereotype.Service;

@Service
public class GradeService {
  private Course latestCourse;
}
)JAVA");
    writeTextFile(tempRoot / "src/main/java/com/example/schedule/model/Course.java", R"JAVA(
package com.example.schedule.model;

public class Course {}
)JAVA");

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;
    const auto report = analyzer.analyzeProject(tempRoot, options);

    expect(report.parsedFiles >= 5,
           "spring boot java files should be parsed by the heuristic analyzer");
    expect(hasModuleDependency(report, "src/main/controller", "src/main/service"),
           "java controller imports should create a dependency toward the service module");
    expect(hasModuleDependency(report, "src/main/service", "src/main/model"),
           "java service imports should create a dependency toward the model module");

    const auto overview = savt::core::buildArchitectureOverview(report);

    const auto* bootstrapNode = findOverviewNode(overview, "src/main/bootstrap");
    expect(bootstrapNode != nullptr, "spring boot overview should include the bootstrap module");
    expect(bootstrapNode->kind == savt::core::OverviewNodeKind::EntryPointModule,
           "spring boot application module should be recognized as the entry point");

    const auto* controllerNode = findOverviewNode(overview, "src/main/controller");
    expect(controllerNode != nullptr, "spring boot overview should include the controller module");
    expect(controllerNode->role == "interaction",
           "java controller modules should be classified as interaction");

    const auto* serviceNode = findOverviewNode(overview, "src/main/service");
    expect(serviceNode != nullptr, "spring boot overview should include the service module");
    expect(serviceNode->role == "analysis",
           "java service modules should be classified as analysis/business logic");

    const auto* modelNode = findOverviewNode(overview, "src/main/model");
    expect(modelNode != nullptr, "spring boot overview should include the model module");
    expect(modelNode->role == "core_model",
           "java model modules should be classified as core domain models");

    fs::remove_all(tempRoot, errorCode);
}

void testSemanticRequiredBlocksWhenCompileCommandsAreMissing() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_semantic_missing_cdb");
    std::error_code errorCode;
    writeTextFile(tempRoot / "src/main.cpp", R"CPP(
int main() { return 0; }
)CPP");

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SemanticRequired;
    const auto report = analyzer.analyzeProject(tempRoot, options);

    expect(report.precisionLevel == "blocked_semantic_required",
           "semantic-required mode should block when compile_commands.json is missing");
    expect(report.semanticStatusCode == "missing_compile_commands",
           "missing compilation database should be exposed as the primary semantic status");
    expect(report.semanticStatusMessage.find("compile_commands.json") != std::string::npos,
           "semantic status message should explain that compile_commands.json is missing");
    expect(std::any_of(report.diagnostics.begin(), report.diagnostics.end(), [](const std::string& diagnostic) {
               return diagnostic.find("compile_commands.json") != std::string::npos;
           }),
           "diagnostics should mention the missing compilation database");
    expect(std::any_of(report.diagnostics.begin(), report.diagnostics.end(), [](const std::string& diagnostic) {
               return diagnostic.find("Compilation database search paths:") != std::string::npos;
           }),
           "diagnostics should record the compilation database search paths");

    fs::remove_all(tempRoot, errorCode);
}

void testSemanticPreferredExplainsBackendUnavailability() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_semantic_backend_state");
    std::error_code errorCode;
    writeTextFile(tempRoot / "main.cpp", R"CPP(
int helper() { return 7; }
int main() { return helper(); }
)CPP");
    writeTextFile(tempRoot / "compile_commands.json",
                  std::string("[\n")
                      + "  {\n"
                      + "    \"directory\": \"" + jsonPath(tempRoot) + "\",\n"
                      + "    \"file\": \"" + jsonPath(tempRoot / "main.cpp") + "\",\n"
                      + "    \"command\": \"clang++ -std=c++20 -c main.cpp\"\n"
                      + "  }\n"
                      + "]\n");

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SemanticPreferred;
    const auto report = analyzer.analyzeProject(tempRoot, options);

#ifdef SAVT_ENABLE_CLANG_TOOLING
    expect(report.semanticStatusCode == "semantic_ready" ||
               report.semanticStatusCode == "translation_unit_parse_failed" ||
               report.semanticStatusCode == "system_headers_unresolved",
           "semantic-capable builds should expose a runtime semantic status");
#else
    expect(report.precisionLevel == "syntax_fallback",
           "semantic-preferred mode should fall back to syntax when the backend is unavailable");
    expect(isBuildTimeSemanticBackendUnavailableCode(report.semanticStatusCode),
           "build-time backend failures should expose a specific semantic backend status, got " +
               report.semanticStatusCode);
    expect(report.semanticStatusMessage.find("semantic backend") != std::string::npos ||
               report.semanticStatusMessage.find("LLVM/Clang") != std::string::npos ||
               report.semanticStatusMessage.find("clang-c/Index.h") != std::string::npos ||
               report.semanticStatusMessage.find("libclang") != std::string::npos,
           "semantic status should explain why the backend is unavailable");
#endif

    fs::remove_all(tempRoot, errorCode);
}

void testSyntaxOnlyAnalysisMarksFactsAsInferred() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_syntax_fact_source");
    std::error_code errorCode;
    writeTextFile(tempRoot / "main.cpp", R"CPP(
int helper() { return 7; }

int main() {
  return helper();
}
)CPP");

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;
    const auto report = analyzer.analyzeProject(tempRoot, options);

    const auto* helper = findSingleNodeByQualifiedName(report, "helper");
    const auto* mainFn = findSingleNodeByQualifiedName(report, "main");
    expect(helper != nullptr && mainFn != nullptr,
           "syntax-only fixture should materialize helper and main symbols");
    expect(helper->factSource == savt::core::FactSource::Inferred,
           "syntax-only nodes should be marked as inferred, got " + factSourceName(helper->factSource));
    expect(mainFn->factSource == savt::core::FactSource::Inferred,
           "syntax-only functions should remain inferred, got " + factSourceName(mainFn->factSource));

    const auto* callEdge = findSingleEdge(report, mainFn->id, helper->id, savt::core::EdgeKind::Calls);
    expect(callEdge != nullptr,
           "syntax-only fixture should resolve the helper call edge");
    expect(callEdge->factSource == savt::core::FactSource::Inferred,
           "syntax-only call edges should be marked as inferred, got " + factSourceName(callEdge->factSource));
    expect(callEdge->supportCount == 1,
           "syntax-only call edges should keep the default support count of 1");

    const std::string json = savt::core::serializeAnalysisReportToJson(report, true);
    expect(json.find("\"factSource\": \"inferred\"") != std::string::npos,
           "syntax-only report json should expose inferred factSource values");

    fs::remove_all(tempRoot, errorCode);
}

void testCollectSourceFilesSkipsToolchainDirectoriesUnderTools() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_skip_toolchain_dirs");
    writeTextFile(tempRoot / "src" / "main.cpp", "int main() { return 0; }\n");
    writeTextFile(tempRoot / "tools" / "llvm" / "include" / "clang" / "AST.h",
                  "class ToolchainOnlyHeader {};\n");
    writeTextFile(tempRoot / "tools" / "downloads" / "manifest.json", "{ \"name\": \"llvm\" }\n");
    writeTextFile(tempRoot / ".qtc_clangd" / "index" / "cache.h", "class CacheOnlyHeader {};\n");

    const auto files =
        savt::analyzer::detail::collectSourceFiles(tempRoot, savt::analyzer::AnalyzerOptions{});

    std::vector<std::string> relativePaths;
    relativePaths.reserve(files.size());
    for (const fs::path& filePath : files) {
        relativePaths.push_back(savt::analyzer::detail::relativizePath(tempRoot, filePath));
    }

    expect(containsText(relativePaths, "src/main.cpp"),
           "real project sources should still be collected");
    expect(!containsText(relativePaths, "tools/llvm/include/clang/AST.h"),
           "toolchain headers under tools/llvm should be skipped");
    expect(!containsText(relativePaths, "tools/downloads/manifest.json"),
           "downloaded tool payloads under tools/downloads should be skipped");
    expect(!containsText(relativePaths, ".qtc_clangd/index/cache.h"),
           "Qt Creator clangd cache directories should be skipped");

    std::error_code errorCode;
    fs::remove_all(tempRoot, errorCode);
}

void testLargeMixedOverviewAvoidsModuleScopedCapabilityExplosion() {
    savt::core::AnalysisReport report;
    savt::core::ArchitectureOverview overview;

    for (std::size_t index = 0; index < 36; ++index) {
        savt::core::OverviewNode node;
        node.id = index + 1;
        node.kind = savt::core::OverviewNodeKind::Module;
        node.name = "workspace/module_" + std::to_string(index + 1);
        node.folderKey = "workspace";
        node.summary = "synthetic large mixed workspace module";
        node.fileCount = 1;
        node.sourceFileCount = 1;
        node.role = (index % 3 == 0) ? "presentation" : ((index % 3 == 1) ? "analysis" : "storage");
        if (index < 3) {
            node.qmlFileCount = 1;
        }
        overview.nodes.push_back(std::move(node));
    }

    const auto graph = savt::core::buildCapabilityGraph(report, overview);
    expect(!containsText(graph.diagnostics, "Capability aggregation mode: module_scoped."),
           "large mixed-language workspaces should no longer force module-scoped capability graphs");
    expect(graph.nodes.size() < overview.nodes.size(),
           "large mixed-language workspaces should aggregate capability nodes to keep the scene bounded");
}

void testSyntaxAnalysisPreservesDistinctOverloads() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_syntax_overloads");
    std::error_code errorCode;
    writeTextFile(tempRoot / "main.cpp", R"CPP(
int helper(int value) {
  return value + 1;
}

double helper(double value) {
  return value + 0.5;
}

int main() {
  return helper(1);
}
)CPP");

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;
    const auto report = analyzer.analyzeProject(tempRoot, options);

    const auto helperNodes = findNodesByQualifiedName(report, "helper");
    const auto* mainFn = findSingleNodeByQualifiedName(report, "main");
    expect(helperNodes.size() == 2,
           "syntax analysis should preserve both helper overloads instead of merging them by qualifiedName");
    expect(mainFn != nullptr,
           "syntax overload fixture should still materialize the caller function");
    expect(std::all_of(helperNodes.begin(), helperNodes.end(), [](const savt::core::SymbolNode* node) {
               return node != nullptr && !node->identityKey.empty();
           }),
           "syntax overloads should materialize a synthetic identity key for disambiguation");
    expect(std::none_of(helperNodes.begin(), helperNodes.end(), [&](const savt::core::SymbolNode* node) {
               return node != nullptr &&
                      hasSymbolEdge(report, mainFn->id, node->id, savt::core::EdgeKind::Calls);
           }),
           "ambiguous syntax overloads should not emit a Calls edge, got " +
               describeEdgesOfKind(report, savt::core::EdgeKind::Calls));
    expect(std::any_of(report.diagnostics.begin(), report.diagnostics.end(), [](const std::string& diagnostic) {
               return diagnostic.find("ambiguous call target: helper") != std::string::npos;
           }),
           "ambiguous syntax overloads should emit an explicit ambiguity diagnostic, got " +
               firstDiagnosticOrNone(report));

    fs::remove_all(tempRoot, errorCode);
}

void testSyntaxAnalysisPrefersSameModuleCallTarget() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_syntax_same_module_call");
    std::error_code errorCode;
    writeTextFile(tempRoot / "app" / "main.cpp", R"CPP(
int main() {
  return helper();
}
)CPP");
    writeTextFile(tempRoot / "app" / "helper.cpp", R"CPP(
int helper() {
  return 1;
}
)CPP");
    writeTextFile(tempRoot / "shared" / "helper.cpp", R"CPP(
int helper() {
  return 2;
}
)CPP");

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;
    const auto report = analyzer.analyzeProject(tempRoot, options);

    const auto helperNodes = findNodesByQualifiedName(report, "helper");
    const auto* mainFn = findSingleNodeByQualifiedName(report, "main");
    expect(helperNodes.size() == 2,
           "same-module fixture should preserve both helper candidates, got " +
               std::to_string(helperNodes.size()));
    expect(mainFn != nullptr,
           "same-module fixture should materialize the main caller");

    const auto preferredHelper = std::find_if(helperNodes.begin(), helperNodes.end(), [](const savt::core::SymbolNode* node) {
        return node != nullptr && node->filePath == "app/helper.cpp";
    });
    const auto fallbackHelper = std::find_if(helperNodes.begin(), helperNodes.end(), [](const savt::core::SymbolNode* node) {
        return node != nullptr && node->filePath == "shared/helper.cpp";
    });
    expect(preferredHelper != helperNodes.end() && fallbackHelper != helperNodes.end(),
           "same-module fixture should expose both app/shared helper candidates");
    expect(hasSymbolEdge(report, mainFn->id, (*preferredHelper)->id, savt::core::EdgeKind::Calls),
           "syntax resolver should prefer the helper from the caller's inferred module");
    expect(!hasSymbolEdge(report, mainFn->id, (*fallbackHelper)->id, savt::core::EdgeKind::Calls),
           "syntax resolver should not link the lower-scoring cross-module helper candidate");
    expect(std::none_of(report.diagnostics.begin(), report.diagnostics.end(), [](const std::string& diagnostic) {
               return diagnostic.find("ambiguous call target: helper") != std::string::npos;
           }),
           "same-module resolution should not fall back to ambiguity diagnostics");

    fs::remove_all(tempRoot, errorCode);
}

#ifdef SAVT_ENABLE_CLANG_TOOLING
void testSemanticRequiredAppleClangCommandsResolveSystemHeaders() {
#if defined(__APPLE__)
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_semantic_macos_sdk_fallback");
    std::error_code errorCode;
    const fs::path sourceFile = tempRoot / "main.cpp";
    writeTextFile(sourceFile, R"CPP(
#include <vector>

int main() {
  std::vector<int> values{1, 2, 3};
  return static_cast<int>(values.size());
}
)CPP");

    writeTextFile(tempRoot / "compile_commands.json",
                  std::string("[\n")
                      + "  {\n"
                      + "    \"directory\": \"" + jsonPath(tempRoot) + "\",\n"
                      + "    \"file\": \"" + jsonPath(sourceFile) + "\",\n"
                      + "    \"arguments\": [\n"
                      + "      \"/usr/bin/clang++\",\n"
                      + "      \"-std=c++20\",\n"
                      + "      \"-c\",\n"
                      + "      \"" + sourceFile.filename().string() + "\"\n"
                      + "    ]\n"
                      + "  }\n"
                      + "]\n");

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SemanticRequired;
    const auto report = analyzer.analyzeProject(tempRoot, options);

    expect(report.precisionLevel == "semantic",
           "macOS AppleClang compile commands without explicit sysroot should still parse semantically, got precision=" +
               report.precisionLevel + ", status=" + report.semanticStatusCode);
    expect(report.semanticStatusCode == "semantic_ready",
           "macOS SDK fallback should avoid unresolved system headers, got status=" + report.semanticStatusCode +
               ", message=" + report.semanticStatusMessage);
    expect(report.parsedFiles >= 1,
           "semantic analysis should parse at least one translation unit after applying the macOS SDK fallback");
    expect(std::any_of(report.diagnostics.begin(), report.diagnostics.end(), [](const std::string& diagnostic) {
               return diagnostic.find("Applied macOS SDK fallback arguments") != std::string::npos;
           }),
           "diagnostics should record when macOS SDK fallback arguments were applied");
    expect(std::none_of(report.diagnostics.begin(), report.diagnostics.end(), [](const std::string& diagnostic) {
               return diagnostic.find("argument unused during compilation: '-c'") != std::string::npos;
           }),
           "semantic parsing should strip compile-only flags such as -c before invoking libclang");
    expect(std::none_of(report.diagnostics.begin(), report.diagnostics.end(), [](const std::string& diagnostic) {
               return diagnostic.find("'linker' input unused") != std::string::npos;
           }),
           "semantic parsing should strip output-object linker inputs before invoking libclang");

    fs::remove_all(tempRoot, errorCode);
#endif
}

void testSemanticRequiredReportsSystemHeaderResolutionFailures() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_semantic_system_headers");
    std::error_code errorCode;
    const fs::path sourceFile = tempRoot / "main.cpp";
    writeTextFile(sourceFile, R"CPP(
#include <vector>

int main() {
  std::vector<int> values{1, 2, 3};
  return static_cast<int>(values.size());
}
)CPP");

    const fs::path clangExecutable =
        fs::current_path() / "tools/llvm/clang+llvm-21.1.7-x86_64-pc-windows-msvc/bin/clang++.exe";
    writeTextFile(tempRoot / "compile_commands.json",
                  std::string("[\n")
                      + "  {\n"
                      + "    \"directory\": \"" + jsonPath(tempRoot) + "\",\n"
                      + "    \"file\": \"" + jsonPath(sourceFile) + "\",\n"
                      + "    \"arguments\": [\n"
                      + "      \"" + jsonPath(clangExecutable) + "\",\n"
                      + "      \"-std=c++20\",\n"
                      + "      \"-nostdinc\",\n"
                      + "      \"-nostdinc++\",\n"
                      + "      \"-c\",\n"
                      + "      \"" + sourceFile.filename().string() + "\"\n"
                      + "    ]\n"
                      + "  }\n"
                      + "]\n");

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SemanticRequired;
    const auto report = analyzer.analyzeProject(tempRoot, options);

    expect(report.precisionLevel == "blocked_semantic_required",
           "semantic-required mode should block when system headers cannot be resolved");
    expect(report.semanticStatusCode == "system_headers_unresolved",
           "system header failures should be classified separately from generic parse failures, got code=" +
               report.semanticStatusCode + ", message=" + report.semanticStatusMessage);
    expect(report.semanticStatusMessage.find("system headers") != std::string::npos,
           "semantic status message should explain that system headers are unresolved");
    expect(std::any_of(report.diagnostics.begin(), report.diagnostics.end(), [](const std::string& diagnostic) {
               return diagnostic.find("system headers") != std::string::npos ||
                      diagnostic.find("file not found") != std::string::npos;
           }),
           "diagnostics should retain the underlying system-header parse failures");

    fs::remove_all(tempRoot, errorCode);
}

void testSemanticAnalysisMergesDeclarationsDefinitionsAndCrossTuEdges() {
    namespace fs = std::filesystem;

    const fs::path tempRoot =
        fs::current_path() / "build" / ("savt_semantic_cross_tu_" + std::to_string(std::rand()));
    std::error_code errorCode;
    fs::remove_all(tempRoot, errorCode);
    fs::create_directories(tempRoot, errorCode);
    const fs::path headerFile = tempRoot / "model.h";
    const fs::path baseFile = tempRoot / "base.cpp";
    const fs::path derivedFile = tempRoot / "derived.cpp";
    const fs::path clangExecutable =
        fs::current_path() / "tools/llvm/clang+llvm-21.1.7-x86_64-pc-windows-msvc/bin/clang++.exe";

    writeTextFile(headerFile, R"CPP(
struct Base {
  virtual int run();
};

struct Derived : Base {
  int run() override;
};

template <typename T>
struct Box {};

int callBase(Base& base);
Box<int>* makeBox();
)CPP");
    writeTextFile(baseFile, R"CPP(
#include "model.h"

int Base::run() {
  return 1;
}

int callBase(Base& base) {
  return base.run();
}
)CPP");
    writeTextFile(derivedFile, R"CPP(
#include "model.h"

int Derived::run() {
  return Base::run();
}

Box<int>* makeBox() {
  return nullptr;
}
)CPP");

    writeTextFile(tempRoot / "compile_commands.json",
                  std::string("[\n")
                      + "  {\n"
                      + "    \"directory\": \"" + jsonPath(tempRoot) + "\",\n"
                      + "    \"file\": \"" + jsonPath(baseFile) + "\",\n"
                      + "    \"command\": \"" + jsonPath(clangExecutable)
                      + " -std=c++20 -I" + jsonPath(tempRoot)
                      + " -fsyntax-only " + jsonPath(baseFile) + "\"\n"
                      + "  },\n"
                      + "  {\n"
                      + "    \"directory\": \"" + jsonPath(tempRoot) + "\",\n"
                      + "    \"file\": \"" + jsonPath(derivedFile) + "\",\n"
                      + "    \"command\": \"" + jsonPath(clangExecutable)
                      + " -std=c++20 -I" + jsonPath(tempRoot)
                      + " -fsyntax-only " + jsonPath(derivedFile) + "\"\n"
                      + "  }\n"
                      + "]\n");

    savt::analyzer::CppProjectAnalyzer analyzer;
    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SemanticRequired;
    const auto report = analyzer.analyzeProject(tempRoot, options);

    expect(report.precisionLevel == "semantic",
           "semantic-required mode should stay on the semantic backend when cross-TU parsing succeeds, got precision=" +
               report.precisionLevel + ", code=" + report.semanticStatusCode + ", message=" + report.semanticStatusMessage +
               ", first diagnostic=" + firstDiagnosticOrNone(report));
    expect(report.semanticAnalysisEnabled,
           "semantic cross-TU fixture should complete semantic analysis successfully");
    expect(report.parsedFiles == 2,
           "semantic cross-TU fixture should parse both translation units");
    expect(findNodesByQualifiedName(report, "Base").size() == 1,
           "Base should be registered exactly once across repeated header traversals");
    expect(findNodesByQualifiedName(report, "Derived").size() == 1,
           "Derived should be registered exactly once across repeated header traversals");
    expect(findNodesByQualifiedName(report, "Base::run").size() == 1,
           "Base::run declaration and definition should merge into one semantic symbol");
    expect(findNodesByQualifiedName(report, "Derived::run").size() == 1,
           "Derived::run declaration and definition should merge into one semantic symbol");
    expect(findNodesByQualifiedName(report, "Box").size() == 1,
           "template symbols should keep a single logical identity across translation units");

    const auto* base = findSingleNodeByQualifiedName(report, "Base");
    const auto* derived = findSingleNodeByQualifiedName(report, "Derived");
    const auto* baseRun = findSingleNodeByQualifiedName(report, "Base::run");
    const auto* callBase = findSingleNodeByQualifiedName(report, "callBase");
    const auto* derivedRun = findSingleNodeByQualifiedName(report, "Derived::run");
    const auto* makeBox = findSingleNodeByQualifiedName(report, "makeBox");
    const auto* box = findSingleNodeByQualifiedName(report, "Box");
    expect(base != nullptr && derived != nullptr && baseRun != nullptr && callBase != nullptr &&
               derivedRun != nullptr && makeBox != nullptr && box != nullptr,
           "semantic cross-TU fixture should materialize the expected callable, type, and template symbols");
    expect(base->factSource == savt::core::FactSource::Semantic &&
               derived->factSource == savt::core::FactSource::Semantic &&
               baseRun->factSource == savt::core::FactSource::Semantic &&
               derivedRun->factSource == savt::core::FactSource::Semantic &&
               callBase->factSource == savt::core::FactSource::Semantic &&
               makeBox->factSource == savt::core::FactSource::Semantic &&
               box->factSource == savt::core::FactSource::Semantic,
           "semantic cross-TU symbols should all be marked as semantic facts");
    expect(hasSymbolEdge(report, derived->id, base->id, savt::core::EdgeKind::Inherits),
           "inheritance relationships should resolve against the merged semantic type identity, got " +
               describeEdgesOfKind(report, savt::core::EdgeKind::Inherits));
    expect(countSymbolEdges(report, derived->id, base->id, savt::core::EdgeKind::Inherits) == 1,
           "cross-TU header traversal should still materialize only one logical Inherits edge");
    expect(hasSymbolEdge(report, derivedRun->id, baseRun->id, savt::core::EdgeKind::Overrides),
           "override relationships should be preserved after cross-TU semantic merging");
    expect(countSymbolEdges(report, derivedRun->id, baseRun->id, savt::core::EdgeKind::Overrides) == 1,
           "override relationships should deduplicate into one logical edge across repeated semantic hits");
    expect(hasSymbolEdge(report, derivedRun->id, baseRun->id, savt::core::EdgeKind::Calls),
           "direct base calls should resolve against the merged semantic identity");
    expect(countSymbolEdges(report, derivedRun->id, baseRun->id, savt::core::EdgeKind::Calls) == 1,
           "cross-TU semantic traversal should keep only one logical direct call edge");
    expect(hasSymbolEdge(report, callBase->id, base->id, savt::core::EdgeKind::UsesType),
           "callBase should keep a semantic UsesType edge toward its Base parameter type");
    expect(countSymbolEdges(report, callBase->id, base->id, savt::core::EdgeKind::UsesType) == 1,
           "declaration and definition should fold repeated parameter type hits into one UsesType edge");
    expect(hasSymbolEdge(report, makeBox->id, box->id, savt::core::EdgeKind::UsesType),
           "template return types should register a stable UsesType edge to the merged template symbol");
    expect(countSymbolEdges(report, makeBox->id, box->id, savt::core::EdgeKind::UsesType) == 1,
           "template UsesType edges should remain single logical edges across header and source traversal");

    const auto* inheritsEdge = findSingleEdge(report, derived->id, base->id, savt::core::EdgeKind::Inherits);
    const auto* overrideEdge = findSingleEdge(report, derivedRun->id, baseRun->id, savt::core::EdgeKind::Overrides);
    const auto* callEdge = findSingleEdge(report, derivedRun->id, baseRun->id, savt::core::EdgeKind::Calls);
    const auto* typeEdge = findSingleEdge(report, callBase->id, base->id, savt::core::EdgeKind::UsesType);
    expect(inheritsEdge != nullptr && overrideEdge != nullptr && callEdge != nullptr && typeEdge != nullptr,
           "semantic cross-TU fixture should preserve unique semantic edges for inheritance, override, call, and type use");
    expect(inheritsEdge->factSource == savt::core::FactSource::Semantic &&
               overrideEdge->factSource == savt::core::FactSource::Semantic &&
               callEdge->factSource == savt::core::FactSource::Semantic &&
               typeEdge->factSource == savt::core::FactSource::Semantic,
           "semantic relationship edges should be marked as semantic facts");
    expect(inheritsEdge->supportCount >= 2,
           "cross-TU inheritance edges should accumulate repeated semantic hits into supportCount");

    const std::string reportText = savt::core::formatAnalysisReport(report, 80, 120, 40);
    const std::string reportJson = savt::core::serializeAnalysisReportToJson(report, true);
    expect(reportText.find("[semantic] inherits: Derived -> Base") != std::string::npos,
           "text report should expose semantic relationship labels");
    expect(reportJson.find("\"factSource\": \"semantic\"") != std::string::npos,
           "semantic report json should serialize semantic factSource values");
    expect(reportJson.find("\"supportCount\": ") != std::string::npos,
           "semantic report json should serialize edge support counts");

    fs::remove_all(tempRoot, errorCode);
}
#endif

}  // namespace

int main() {
    testAdaptiveModuleInference();
    testDependencyAndPrimaryEntryClassification();
    testCrossArtifactOverviewAndCapabilityGraph();
    testCapabilitySceneMapperPublishesSingleScenePayload();
    testAnalysisReportSerializesFactSources();
    testAnalysisGraphBuilderPreservesDistinctSemanticOverloads();
    testNodeBackendRelativeImportsAndRoleClassification();
    testSpringBootJavaImportsAndRoleClassification();
    testSemanticRequiredBlocksWhenCompileCommandsAreMissing();
    testSemanticPreferredExplainsBackendUnavailability();
    testSyntaxOnlyAnalysisMarksFactsAsInferred();
    testCollectSourceFilesSkipsToolchainDirectoriesUnderTools();
    testLargeMixedOverviewAvoidsModuleScopedCapabilityExplosion();
    testSyntaxAnalysisPreservesDistinctOverloads();
    testSyntaxAnalysisPrefersSameModuleCallTarget();
#ifdef SAVT_ENABLE_CLANG_TOOLING
    testSemanticRequiredAppleClangCommandsResolveSystemHeaders();
    testSemanticRequiredReportsSystemHeaderResolutionFailures();
    testSemanticAnalysisMergesDeclarationsDefinitionsAndCrossTuEdges();
#endif
    std::cout << "[PASS] savt_backend_tests\n";
    return 0;
}
