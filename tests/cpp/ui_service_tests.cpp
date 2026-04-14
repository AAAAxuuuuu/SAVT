#include "savt/ui/AnalysisController.h"
#include "savt/ui/AiService.h"
#include "savt/ui/AstPreviewService.h"
#include "savt/ui/FileInsightService.h"
#include "savt/ui/IncrementalAnalysisPipeline.h"
#include "savt/ui/ReportService.h"

#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/layout/LayeredGraphLayout.h"

#include <QCoreApplication>
#include <QVariantMap>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

namespace {

void expect(const bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << "\n";
        std::exit(1);
    }
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

void testAstPreviewServiceBuildsItemsAndParsesCppPreview() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_ui_ast_preview");
    writeTextFile(tempRoot / "src" / "main.cpp", "int main() { return 0; }\n");
    writeTextFile(tempRoot / "include" / "app.h", "class App {};\n");
    writeTextFile(tempRoot / "tests" / "sample.cpp", "void test_only() {}\n");

    savt::core::AnalysisReport report;
    report.rootPath = tempRoot.generic_string();
    report.nodes = {
        {1, savt::core::SymbolKind::File, "main.cpp", "src/main.cpp", "file:src/main.cpp", "src/main.cpp", 0},
        {2, savt::core::SymbolKind::File, "app.h", "include/app.h", "file:include/app.h", "include/app.h", 0},
        {3, savt::core::SymbolKind::File, "sample.cpp", "tests/sample.cpp", "file:tests/sample.cpp", "tests/sample.cpp", 0}
    };

    const QVariantList items = savt::ui::AstPreviewService::buildAstFileItems(report);
    expect(items.size() == 3, "AST preview items should include every previewable C/C++ file node");

    const QString defaultPath = savt::ui::AstPreviewService::chooseDefaultAstFilePath(items);
    expect(defaultPath == QStringLiteral("src/main.cpp"),
           "default AST preview path should prioritize src/main.cpp ahead of headers and tests");

    const auto preview = savt::ui::AstPreviewService::buildPreview(
        QString::fromStdString(tempRoot.generic_string()),
        QStringLiteral("src/main.cpp"));
    expect(preview.summary.contains(QStringLiteral("AST 节点")),
           "AST preview summary should include parsed tree statistics");
    expect(!preview.text.trimmed().isEmpty(),
           "AST preview text should contain a formatted syntax tree");

    const auto emptyPreview = savt::ui::AstPreviewService::buildEmptyPreview();
    expect(emptyPreview.title == QStringLiteral("AST 预览"),
           "empty preview should expose the default AST title");

    std::error_code errorCode;
    fs::remove_all(tempRoot, errorCode);
}

void testReportServiceBuildsReadableSystemContextPayload() {
    savt::core::AnalysisReport report;
    report.rootPath = "/tmp/demo_workspace";
    report.parsedFiles = 6;
    report.semanticAnalysisRequested = true;
    report.semanticAnalysisEnabled = true;

    savt::core::ArchitectureOverview overview;
    savt::core::OverviewNode entryNode;
    entryNode.id = 1;
    entryNode.kind = savt::core::OverviewNodeKind::EntryPointModule;
    entryNode.name = "apps/desktop";
    entryNode.role = "presentation";
    entryNode.fileCount = 2;
    entryNode.sourceFileCount = 2;
    entryNode.qmlFileCount = 1;
    entryNode.stageIndex = 0;
    entryNode.stageName = "entry";
    entryNode.reachableFromEntry = true;
    overview.nodes.push_back(entryNode);

    savt::core::OverviewNode analysisNode;
    analysisNode.id = 2;
    analysisNode.kind = savt::core::OverviewNodeKind::Module;
    analysisNode.name = "src/analyzer";
    analysisNode.role = "analysis";
    analysisNode.fileCount = 3;
    analysisNode.sourceFileCount = 3;
    analysisNode.stageIndex = 1;
    analysisNode.stageName = "core";
    analysisNode.reachableFromEntry = true;
    overview.nodes.push_back(analysisNode);

    savt::core::CapabilityGraph graph;
    savt::core::CapabilityNode capabilityEntry;
    capabilityEntry.id = 10;
    capabilityEntry.kind = savt::core::CapabilityNodeKind::Entry;
    capabilityEntry.name = "Desktop Shell";
    capabilityEntry.dominantRole = "presentation";
    capabilityEntry.responsibility = "负责启动桌面工作台。";
    capabilityEntry.summary = "负责启动桌面工作台并调度后续流程。";
    capabilityEntry.moduleNames = {"apps/desktop"};
    capabilityEntry.exampleFiles = {"apps/desktop/src/main.cpp"};
    capabilityEntry.defaultVisible = true;
    capabilityEntry.defaultPinned = true;
    capabilityEntry.visualPriority = 200;
    graph.nodes.push_back(capabilityEntry);

    savt::core::CapabilityNode analysisCapability;
    analysisCapability.id = 11;
    analysisCapability.kind = savt::core::CapabilityNodeKind::Capability;
    analysisCapability.name = "Analyzer Pipeline";
    analysisCapability.dominantRole = "analysis";
    analysisCapability.responsibility = "组织扫描、解析和聚合。";
    analysisCapability.summary = "负责项目扫描、语法解析和架构聚合。";
    analysisCapability.moduleNames = {"src/analyzer"};
    analysisCapability.exampleFiles = {"src/analyzer/src/CppProjectAnalyzer.cpp"};
    analysisCapability.defaultVisible = true;
    analysisCapability.visualPriority = 180;
    analysisCapability.fileCount = 18;
    analysisCapability.incomingEdgeCount = 4;
    analysisCapability.outgoingEdgeCount = 3;
    analysisCapability.flowParticipationCount = 2;
    analysisCapability.riskScore = 6;
    analysisCapability.riskLevel = "high";
    graph.nodes.push_back(analysisCapability);

    graph.groups.push_back({1, savt::core::CapabilityGroupKind::Lane, "Entry", "", {10}, true, true, 0});
    graph.groups.push_back({2, savt::core::CapabilityGroupKind::Lane, "Capability", "", {11}, true, true, 0});
    graph.flows.push_back({1, "Primary path", "Desktop Shell -> Analyzer Pipeline", {10, 11}, 5, 2});

    savt::layout::LayoutResult layout;
    layout.width = 800.0;
    layout.height = 420.0;
    layout.layerCount = 2;

    const QString statusMessage =
        savt::ui::ReportService::buildStatusMessage(report, overview, graph, layout);
    expect(statusMessage.contains(QStringLiteral("已读取 6 个源码文件")),
           "status message should summarize parsed file counts");
    expect(statusMessage.contains(QStringLiteral("语义后端")),
           "status message should mention semantic backend state when enabled");

    const QVariantMap systemContext =
        savt::ui::ReportService::buildSystemContextData(
            report, overview, graph, QStringLiteral("/tmp/demo_workspace"));
    expect(systemContext.value(QStringLiteral("projectName")).toString() == QStringLiteral("demo_workspace"),
           "system context should derive the project name from the root path");
    expect(!systemContext.value(QStringLiteral("entrySummary")).toString().trimmed().isEmpty(),
           "system context should expose an entry summary");
    expect(!systemContext.value(QStringLiteral("containerNames")).toStringList().isEmpty(),
           "system context should publish core container names");
    expect(systemContext.value(QStringLiteral("mainFlowSummary")).toString().contains(QStringLiteral("Desktop Shell")),
           "system context should publish the primary collaboration path");
    expect(!systemContext.value(QStringLiteral("contextSections")).toList().isEmpty(),
           "system context should publish complete structured context sections");
    expect(!systemContext.value(QStringLiteral("hotspotSignals")).toList().isEmpty(),
           "system context should publish structural hotspot signals");

    const QVariantList cards =
        savt::ui::ReportService::buildSystemContextCards(report, overview, graph);
    expect(cards.size() >= 3, "system context cards should publish the expected summary cards");
}

void testAnalysisControllerStartsFromStableDefaultState() {
    savt::ui::AnalysisController controller;
    expect(!controller.projectRootPath().isEmpty(),
           "analysis controller should resolve a default project root path");
    expect(controller.statusMessage().contains(QStringLiteral("准备就绪")),
           "analysis controller should expose the ready status message on startup");
    expect(controller.analysisPhase() == QStringLiteral("等待开始"),
           "analysis controller should start in the waiting phase");
    expect(!controller.analyzing(),
           "analysis controller should not start in analyzing state");
    expect(controller.analysisProgress() == 0.0,
           "analysis controller should start with zero progress");
    expect(controller.capabilityNodeItems().isEmpty(),
           "analysis controller should start with an empty capability scene");
    expect(controller.componentSceneCatalog().isEmpty(),
           "analysis controller should start with an empty component catalog");
    expect(controller.astPreviewTitle() == QStringLiteral("AST 预览"),
           "analysis controller should initialize the AST preview through AstPreviewService");
}

void testAiServiceClassifiesCapabilityAndComponentScopes() {
    QVariantMap capabilityNode;
    capabilityNode.insert(QStringLiteral("name"), QStringLiteral("Analyzer Pipeline"));
    capabilityNode.insert(QStringLiteral("kind"), QStringLiteral("capability"));
    expect(savt::ui::AiService::classifyNodeScope(capabilityNode) == QStringLiteral("capability_map"),
           "capability scene nodes should be routed through the L2 AI scope");

    QVariantMap componentNode = capabilityNode;
    componentNode.insert(QStringLiteral("capabilityId"), 11);
    componentNode.insert(QStringLiteral("kind"), QStringLiteral("service"));
    expect(savt::ui::AiService::classifyNodeScope(componentNode) == QStringLiteral("component_node"),
           "component scene nodes should be routed through the L3 AI scope");

    QVariantMap fileNode = componentNode;
    fileNode.insert(QStringLiteral("name"), QStringLiteral("indexService.js"));
    fileNode.insert(QStringLiteral("fileCount"), 1);
    fileNode.insert(QStringLiteral("exampleFiles"), QVariantList{QStringLiteral("src/services/indexService.js")});
    expect(savt::ui::AiService::classifyNodeScope(fileNode) == QStringLiteral("file_node"),
           "single-file component nodes should be routed through the file AI scope");

    QVariantMap companionFileNode = componentNode;
    companionFileNode.insert(QStringLiteral("name"), QStringLiteral("TrafficBackend.cpp"));
    companionFileNode.insert(QStringLiteral("fileCount"), 2);
    companionFileNode.insert(QStringLiteral("exampleFiles"),
                             QVariantList{QStringLiteral("App/backend/facade/TrafficBackend.cpp"),
                                          QStringLiteral("App/backend/facade/TrafficBackend.h")});
    expect(savt::ui::AiService::classifyNodeScope(companionFileNode) == QStringLiteral("file_node"),
           "file-named component nodes should use the file AI scope even when paired with a companion header");

    QVariantMap rewrittenFileCluster = componentNode;
    rewrittenFileCluster.insert(QStringLiteral("name"), QStringLiteral("facade 入口协调"));
    rewrittenFileCluster.insert(QStringLiteral("fileCount"), 2);
    rewrittenFileCluster.insert(QStringLiteral("fileCluster"), true);
    rewrittenFileCluster.insert(QStringLiteral("fileBacked"), true);
    rewrittenFileCluster.insert(QStringLiteral("primaryFilePath"),
                                QStringLiteral("App/backend/facade/TrafficBackend.cpp"));
    rewrittenFileCluster.insert(QStringLiteral("exampleFiles"),
                                QVariantList{QStringLiteral("App/backend/facade/TrafficBackend.cpp"),
                                             QStringLiteral("App/backend/facade/TrafficBackend.h")});
    expect(savt::ui::AiService::classifyNodeScope(rewrittenFileCluster) == QStringLiteral("file_node"),
           "file-cluster metadata should route rewritten display labels through the file AI scope");

    QVariantMap folderNode = componentNode;
    folderNode.insert(QStringLiteral("name"), QStringLiteral("facade"));
    folderNode.insert(QStringLiteral("fileCount"), 1);
    folderNode.insert(QStringLiteral("exampleFiles"),
                      QVariantList{QStringLiteral("App/backend/facade/TrafficBackend.cpp")});
    expect(savt::ui::AiService::classifyNodeScope(folderNode) == QStringLiteral("component_node"),
           "folder-named component nodes should stay in the component AI scope even with one source file");
}

void testFileInsightServiceBuildsPortableSingleFileSummary() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_ui_file_insight");
    writeTextFile(tempRoot / "src" / "services" / "indexService.js",
                  "import fetchIndex from './fetchIndex.js';\n"
                  "const cache = new Map();\n\n"
                  "export function refreshIndex(query) {\n"
                  "  return fetchIndex(query).then(result => {\n"
                  "    cache.set(query, result);\n"
                  "    return result;\n"
                  "  });\n"
                  "}\n");
    writeTextFile(tempRoot / "App" / "backend" / "facade" / "TrafficBackend.cpp",
                  "#include \"TrafficBackend.h\"\n"
                  "#include <vector>\n"
                  "\n"
                  "namespace app {\n"
                  "int helperOnly() { return 1; }\n"
                  "}\n"
                  "\n"
                  "void TrafficBackend::parseSource(const std::string& source) {\n"
                  "  const auto parsed = source.size();\n"
                  "  dispatchTrafficUpdate(parsed);\n"
                  "}\n");
    writeTextFile(tempRoot / "App" / "backend" / "facade" / "TrafficBackend.h",
                  "class TrafficBackend {};\n");
    writeTextFile(tempRoot / "config" / "app.json",
                  "{\n"
                  "  \"name\": \"demo\",\n"
                  "  \"routes\": [\n"
                  "    {\"path\": \"/\", \"target\": \"Home\"}\n"
                  "  ],\n"
                  "  \"plugins\": [\"auth\"]\n"
                  "}\n");

    QVariantMap fileNode;
    fileNode.insert(QStringLiteral("name"), QStringLiteral("indexService.js"));
    fileNode.insert(QStringLiteral("role"), QStringLiteral("service"));
    fileNode.insert(QStringLiteral("fileCount"), 1);
    fileNode.insert(QStringLiteral("exampleFiles"),
                    QVariantList{QStringLiteral("src/services/indexService.js")});
    fileNode.insert(QStringLiteral("topSymbols"),
                    QVariantList{QStringLiteral("refreshIndex")});
    fileNode.insert(QStringLiteral("collaboratorNames"),
                    QVariantList{QStringLiteral("Search Store")});

    const QVariantMap detail = savt::ui::FileInsightService::buildDetail(
        QString::fromStdString(tempRoot.generic_string()), fileNode);
    expect(detail.value(QStringLiteral("available")).toBool(),
           "file insight should be available for an existing single file");
    expect(detail.value(QStringLiteral("languageLabel")).toString() == QStringLiteral("JavaScript"),
           "file insight should infer the language from the suffix");
    expect(detail.value(QStringLiteral("roleLabel")).toString().contains(QStringLiteral("服务")),
           "file insight should infer a portable role label");
    expect(!detail.value(QStringLiteral("importClues")).toList().isEmpty(),
           "file insight should expose import clues");
    expect(!detail.value(QStringLiteral("readingHints")).toList().isEmpty(),
           "file insight should expose reading hints");
    expect(detail.value(QStringLiteral("summary")).toString().contains(QStringLiteral("refreshIndex")),
           "file insight summary should use concrete declaration clues instead of only generic text");

    QVariantMap fileClusterNode;
    fileClusterNode.insert(QStringLiteral("name"), QStringLiteral("facade 入口协调"));
    fileClusterNode.insert(QStringLiteral("role"), QStringLiteral("service"));
    fileClusterNode.insert(QStringLiteral("fileCount"), 2);
    fileClusterNode.insert(QStringLiteral("fileCluster"), true);
    fileClusterNode.insert(QStringLiteral("fileBacked"), true);
    fileClusterNode.insert(QStringLiteral("primaryFilePath"),
                           QStringLiteral("App/backend/facade/TrafficBackend.cpp"));
    fileClusterNode.insert(QStringLiteral("exampleFiles"),
                           QVariantList{QStringLiteral("App/backend/facade/TrafficBackend.cpp"),
                                        QStringLiteral("App/backend/facade/TrafficBackend.h")});
    fileClusterNode.insert(QStringLiteral("topSymbols"),
                           QVariantList{QStringLiteral("parseSource")});

    const QVariantMap clusterDetail = savt::ui::FileInsightService::buildDetail(
        QString::fromStdString(tempRoot.generic_string()), fileClusterNode);
    expect(clusterDetail.value(QStringLiteral("available")).toBool(),
           "file insight should be available for a file cluster with an explicit primary file");
    expect(clusterDetail.value(QStringLiteral("fileName")).toString() == QStringLiteral("TrafficBackend.cpp"),
           "file insight should use the explicit primary file instead of the rewritten display label");
    expect(clusterDetail.value(QStringLiteral("languageLabel")).toString() == QStringLiteral("C++"),
           "file insight should prefer the C++ implementation file over its companion header");
    expect(clusterDetail.value(QStringLiteral("previewText")).toString().contains(QStringLiteral("parseSource")),
           "file insight preview should prefer symbol-backed implementation code");
    expect(!clusterDetail.value(QStringLiteral("previewText")).toString().contains(QStringLiteral("#include")),
           "file insight preview should not treat include lines as key implementation code");

    QVariantMap configNode;
    configNode.insert(QStringLiteral("name"), QStringLiteral("app.json"));
    configNode.insert(QStringLiteral("fileCount"), 1);
    configNode.insert(QStringLiteral("fileCluster"), true);
    configNode.insert(QStringLiteral("fileBacked"), true);
    configNode.insert(QStringLiteral("primaryFilePath"), QStringLiteral("config/app.json"));
    configNode.insert(QStringLiteral("exampleFiles"),
                      QVariantList{QStringLiteral("config/app.json")});

    const QVariantMap configDetail = savt::ui::FileInsightService::buildDetail(
        QString::fromStdString(tempRoot.generic_string()), configNode);
    expect(configDetail.value(QStringLiteral("previewText")).toString().contains(QStringLiteral("routes")),
           "file insight preview should use generic key/value structure for configuration files");
    expect(configDetail.value(QStringLiteral("previewText")).toString().contains(QStringLiteral("plugins")),
           "file insight preview should not require source-code-specific symbols to show useful configuration snippets");

    writeTextFile(tempRoot / "src" / "ai" / "ApiClient.cpp",
                  "#include \"ApiClient.h\"\n"
                  "#include <QString>\n"
                  "#include <vector>\n"
                  "\n"
                  "namespace {\n"
                  "void appendRelativePathCandidates(std::vector<QString>& candidates, const QString& path) {\n"
                  "  if (!path.isEmpty()) {\n"
                  "    candidates.push_back(path.trimmed());\n"
                  "  }\n"
                  "}\n"
                  "\n"
                  "bool builtInApiConfigEnabled() {\n"
                  "  return true;\n"
                  "}\n"
                  "}\n"
                  "\n"
                  "ApiRequest buildApiChatCompletionsRequest(const ApiConfig& config, const ApiPayload& payload) {\n"
                  "  ApiRequest request;\n"
                  "  request.endpoint = config.endpoint;\n"
                  "  request.body = payload.toJson();\n"
                  "  request.headers.insert(\"Content-Type\", \"application/json\");\n"
                  "  return request;\n"
                  "}\n"
                  "\n"
                  "ApiReply parseApiChatCompletionsReply(const QByteArray& bytes) {\n"
                  "  ApiReply reply;\n"
                  "  reply.payload = parseJson(bytes);\n"
                  "  if (reply.payload.isEmpty()) {\n"
                  "    reply.error = \"empty reply\";\n"
                  "  }\n"
                  "  return reply;\n"
                  "}\n");

    QVariantMap apiClientNode;
    apiClientNode.insert(QStringLiteral("name"), QStringLiteral("ApiClient.cpp"));
    apiClientNode.insert(QStringLiteral("fileCount"), 1);
    apiClientNode.insert(QStringLiteral("fileCluster"), true);
    apiClientNode.insert(QStringLiteral("fileBacked"), true);
    apiClientNode.insert(QStringLiteral("primaryFilePath"), QStringLiteral("src/ai/ApiClient.cpp"));
    apiClientNode.insert(QStringLiteral("exampleFiles"),
                         QVariantList{QStringLiteral("src/ai/ApiClient.cpp")});
    apiClientNode.insert(QStringLiteral("topSymbols"),
                         QVariantList{QStringLiteral("appendRelativePathCandidates"),
                                      QStringLiteral("builtInApiConfigEnabled"),
                                      QStringLiteral("buildApiChatCompletionsRequest"),
                                      QStringLiteral("parseApiChatCompletionsReply")});

    const QVariantMap apiClientDetail = savt::ui::FileInsightService::buildDetail(
        QString::fromStdString(tempRoot.generic_string()), apiClientNode);
    const QString apiClientPreview = apiClientDetail.value(QStringLiteral("previewText")).toString();
    expect(apiClientPreview.contains(QStringLiteral("buildApiChatCompletionsRequest")),
           "C++ preview should prefer file-identity-backed public implementation functions over private helpers");
    expect(apiClientPreview.contains(QStringLiteral("parseApiChatCompletionsReply")),
           "C++ preview should keep multiple central implementation anchors when they carry behavior");
    expect(!apiClientPreview.contains(QStringLiteral("bool builtInApiConfigEnabled")),
           "C++ preview should not spend a key-code slot on a private config switch helper");

    writeTextFile(tempRoot / "src" / "core" / "WorkflowGraph.cpp",
                  "#include \"WorkflowGraph.h\"\n"
                  "#include <unordered_map>\n"
                  "\n"
                  "namespace {\n"
                  "void assignStageGroups(WorkflowGraph& graph) {\n"
                  "  graph.groups.clear();\n"
                  "  for (const auto& node : graph.nodes) {\n"
                  "    graph.groups[node.stage].push_back(node.id);\n"
                  "  }\n"
                  "}\n"
                  "}\n"
                  "\n"
                  "WorkflowGraph buildWorkflowGraphForCapabilityImpl(const AnalysisReport& report,\n"
                  "                                                   const CapabilityId capabilityId) {\n"
                  "  WorkflowGraph graph;\n"
                  "  for (const auto& node : report.nodes) {\n"
                  "    if (node.capabilityId == capabilityId) {\n"
                  "      graph.nodes.push_back(toWorkflowNode(node));\n"
                  "    }\n"
                  "  }\n"
                  "  for (const auto& edge : report.edges) {\n"
                  "    if (graph.contains(edge.from) && graph.contains(edge.to)) {\n"
                  "      graph.edges.push_back(edge);\n"
                  "    }\n"
                  "  }\n"
                  "  return graph;\n"
                  "}\n"
                  "\n"
                  "WorkflowGraph buildWorkflowGraphForCapability(const AnalysisReport& report,\n"
                  "                                               const CapabilityId capabilityId) {\n"
                  "  WorkflowGraph graph = buildWorkflowGraphForCapabilityImpl(report, capabilityId);\n"
                  "  assignStageGroups(graph);\n"
                  "  return graph;\n"
                  "}\n");

    QVariantMap workflowGraphNode;
    workflowGraphNode.insert(QStringLiteral("name"), QStringLiteral("WorkflowGraph.cpp"));
    workflowGraphNode.insert(QStringLiteral("fileCount"), 1);
    workflowGraphNode.insert(QStringLiteral("fileCluster"), true);
    workflowGraphNode.insert(QStringLiteral("fileBacked"), true);
    workflowGraphNode.insert(QStringLiteral("primaryFilePath"), QStringLiteral("src/core/WorkflowGraph.cpp"));
    workflowGraphNode.insert(QStringLiteral("exampleFiles"),
                             QVariantList{QStringLiteral("src/core/WorkflowGraph.cpp")});
    workflowGraphNode.insert(QStringLiteral("topSymbols"),
                             QVariantList{QStringLiteral("assignStageGroups"),
                                          QStringLiteral("buildWorkflowGraphForCapabilityImpl"),
                                          QStringLiteral("buildWorkflowGraphForCapability")});

    const QVariantMap workflowGraphDetail = savt::ui::FileInsightService::buildDetail(
        QString::fromStdString(tempRoot.generic_string()), workflowGraphNode);
    const QString workflowGraphPreview =
        workflowGraphDetail.value(QStringLiteral("previewText")).toString();
    expect(workflowGraphPreview.contains(QStringLiteral("buildWorkflowGraphForCapabilityImpl")),
           "C++ preview should prefer graph-building implementation functions that match the file identity");
    expect(!workflowGraphPreview.contains(QStringLiteral("void assignStageGroups")),
           "C++ preview should avoid promoting a private grouping helper above central orchestration code");

    writeTextFile(tempRoot / "src" / "modules" / "TrafficMap.ixx",
                  "export module TrafficMap;\n"
                  "import <vector>;\n"
                  "\n"
                  "namespace detail {\n"
                  "int trimMapSeed(int value) { return value; }\n"
                  "}\n"
                  "\n"
                  "export TrafficMapModel buildTrafficMapModel(const std::vector<Road>& roads) {\n"
                  "  TrafficMapModel model;\n"
                  "  for (const auto& road : roads) {\n"
                  "    model.routes.push_back(toTrafficRoute(road));\n"
                  "  }\n"
                  "  return model;\n"
                  "}\n");

    QVariantMap moduleNode;
    moduleNode.insert(QStringLiteral("name"), QStringLiteral("TrafficMap.ixx"));
    moduleNode.insert(QStringLiteral("fileCount"), 1);
    moduleNode.insert(QStringLiteral("fileCluster"), true);
    moduleNode.insert(QStringLiteral("fileBacked"), true);
    moduleNode.insert(QStringLiteral("primaryFilePath"), QStringLiteral("src/modules/TrafficMap.ixx"));
    moduleNode.insert(QStringLiteral("exampleFiles"),
                      QVariantList{QStringLiteral("src/modules/TrafficMap.ixx")});
    moduleNode.insert(QStringLiteral("topSymbols"),
                      QVariantList{QStringLiteral("trimMapSeed"),
                                   QStringLiteral("buildTrafficMapModel")});

    const QVariantMap moduleDetail = savt::ui::FileInsightService::buildDetail(
        QString::fromStdString(tempRoot.generic_string()), moduleNode);
    const QString modulePreview = moduleDetail.value(QStringLiteral("previewText")).toString();
    expect(moduleDetail.value(QStringLiteral("languageLabel")).toString() == QStringLiteral("C++ Module"),
           "file insight should recognize mainstream C++ module interface files");
    expect(modulePreview.contains(QStringLiteral("buildTrafficMapModel")),
           "C++ module preview should use the same key-code extraction as cpp files");
    expect(!modulePreview.contains(QStringLiteral("trimMapSeed")),
           "C++ module preview should not promote a private helper above exported behavior");

    writeTextFile(tempRoot / "CMakeLists.txt",
                  "cmake_minimum_required(VERSION 3.25)\n"
                  "project(TrafficMap LANGUAGES CXX)\n"
                  "find_package(Qt6 REQUIRED COMPONENTS Quick)\n"
                  "qt_add_executable(traffic_map src/main.cpp src/modules/TrafficMap.ixx)\n"
                  "qt_add_qml_module(traffic_map URI TrafficMap QML_FILES qml/Main.qml)\n"
                  "target_link_libraries(traffic_map PRIVATE Qt6::Quick)\n");

    QVariantMap cmakeNode;
    cmakeNode.insert(QStringLiteral("name"), QStringLiteral("CMakeLists.txt"));
    cmakeNode.insert(QStringLiteral("fileCount"), 1);
    cmakeNode.insert(QStringLiteral("fileCluster"), true);
    cmakeNode.insert(QStringLiteral("fileBacked"), true);
    cmakeNode.insert(QStringLiteral("primaryFilePath"), QStringLiteral("CMakeLists.txt"));
    cmakeNode.insert(QStringLiteral("exampleFiles"),
                     QVariantList{QStringLiteral("CMakeLists.txt")});

    const QVariantMap cmakeDetail = savt::ui::FileInsightService::buildDetail(
        QString::fromStdString(tempRoot.generic_string()), cmakeNode);
    const QString cmakePreview = cmakeDetail.value(QStringLiteral("previewText")).toString();
    expect(cmakeDetail.value(QStringLiteral("languageLabel")).toString() == QStringLiteral("CMake"),
           "file insight should recognize CMake project files");
    expect(cmakePreview.contains(QStringLiteral("qt_add_executable")) &&
               cmakePreview.contains(QStringLiteral("target_link_libraries")),
           "CMake preview should prefer build target and linkage declarations as key lines");

    writeTextFile(tempRoot / "resources" / "app.qrc",
                  "<RCC>\n"
                  "  <qresource prefix=\"/\">\n"
                  "    <file>qml/Main.qml</file>\n"
                  "    <file>icons/map.svg</file>\n"
                  "  </qresource>\n"
                  "</RCC>\n");

    QVariantMap qrcNode;
    qrcNode.insert(QStringLiteral("name"), QStringLiteral("app.qrc"));
    qrcNode.insert(QStringLiteral("fileCount"), 1);
    qrcNode.insert(QStringLiteral("fileCluster"), true);
    qrcNode.insert(QStringLiteral("fileBacked"), true);
    qrcNode.insert(QStringLiteral("primaryFilePath"), QStringLiteral("resources/app.qrc"));
    qrcNode.insert(QStringLiteral("exampleFiles"),
                   QVariantList{QStringLiteral("resources/app.qrc")});

    const QVariantMap qrcDetail = savt::ui::FileInsightService::buildDetail(
        QString::fromStdString(tempRoot.generic_string()), qrcNode);
    const QString qrcPreview = qrcDetail.value(QStringLiteral("previewText")).toString();
    expect(qrcDetail.value(QStringLiteral("languageLabel")).toString() == QStringLiteral("Qt Resource"),
           "file insight should recognize Qt resource collection files");
    expect(qrcPreview.contains(QStringLiteral("<qresource")) &&
               qrcPreview.contains(QStringLiteral("qml/Main.qml")),
           "qrc preview should prefer resource prefixes and resource file entries");

    std::error_code errorCode;
    fs::remove_all(tempRoot, errorCode);
}

void testAiServiceParseReplyUsesScopeSpecificStatusMessages() {
    const QByteArray response = R"JSON({
        "choices": [
            {
                "message": {
                    "content": "{\"summary\":\"摘要\",\"plain_summary\":\"给新手看的摘要。\",\"responsibility\":\"解释当前焦点\",\"why_it_matters\":\"它会影响后续阅读顺序\",\"collaborators\":[\"A\"],\"evidence\":[\"证据 1\"],\"where_to_start\":[\"先看入口\"],\"next_actions\":[\"再核对依赖\"],\"uncertainty\":\"当前结论基于静态证据\"}"
                }
            }
        ]
    })JSON";

    const savt::ui::AiReplyState capabilityState =
        savt::ui::AiService::parseReply(response, QStringLiteral("capability_map"), false, {});
    expect(capabilityState.hasResult,
           "capability-scope AI reply should parse successfully");
    expect(capabilityState.statusMessage.contains(QStringLiteral("能力导览")),
           "capability-scope AI reply should use the L2 status message");
    expect(capabilityState.nextActions.size() == 2,
           "capability-scope AI reply should merge where-to-start and next actions");

    const savt::ui::AiReplyState reportState =
        savt::ui::AiService::parseReply(response, QStringLiteral("engineering_report"), false, {});
    expect(reportState.hasResult,
           "report-scope AI reply should parse successfully");
    expect(reportState.statusMessage.contains(QStringLiteral("报告导览")),
           "report-scope AI reply should use the L4 status message");

    const savt::ui::AiReplyState fileState =
        savt::ui::AiService::parseReply(response, QStringLiteral("file_node"), false, {});
    expect(fileState.hasResult,
           "file-scope AI reply should parse successfully");
    expect(fileState.statusMessage.contains(QStringLiteral("文件解读")),
           "file-scope AI reply should use the file status message");
}

void testIncrementalAnalysisPipelineReusesStableLayers() {
    namespace fs = std::filesystem;

    const fs::path tempRoot = makeTempDirectory("savt_ui_incremental_cache_hit");
    writeTextFile(tempRoot / "apps" / "desktop" / "main.cpp", "int main() { return helper(); }\n");
    writeTextFile(tempRoot / "src" / "helper.cpp", "int helper() { return 42; }\n");

    savt::ui::IncrementalAnalysisPipeline::clear();

    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;

    const auto coldArtifacts =
        savt::ui::IncrementalAnalysisPipeline::analyze(tempRoot, options);
    expect(!coldArtifacts.scanLayer.hit,
           "first incremental run should miss the scan cache");
    expect(!coldArtifacts.parseLayer.hit,
           "first incremental run should miss the parse cache");
    expect(!coldArtifacts.aggregateLayer.hit,
           "first incremental run should miss the aggregate cache");
    expect(!coldArtifacts.layoutLayer.hit,
           "first incremental run should miss the layout cache");

    const auto hotArtifacts =
        savt::ui::IncrementalAnalysisPipeline::analyze(tempRoot, options);
    expect(hotArtifacts.scanLayer.hit,
           "second incremental run should hit the scan cache");
    expect(hotArtifacts.parseLayer.hit,
           "second incremental run should hit the parse cache");
    expect(hotArtifacts.aggregateLayer.hit,
           "second incremental run should hit the aggregate cache");
    expect(hotArtifacts.layoutLayer.hit,
           "second incremental run should hit the layout cache");
    expect(hotArtifacts.report.parsedFiles == coldArtifacts.report.parsedFiles,
           "cached incremental run should preserve parsed file counts");

    std::error_code errorCode;
    fs::remove_all(tempRoot, errorCode);
}

void testIncrementalAnalysisPipelineInvalidatesChangedSourceFiles() {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;

    const fs::path tempRoot = makeTempDirectory("savt_ui_incremental_cache_invalidate");
    writeTextFile(tempRoot / "apps" / "desktop" / "main.cpp", "int main() { return helper(); }\n");
    writeTextFile(tempRoot / "src" / "helper.cpp", "int helper() { return 7; }\n");

    savt::ui::IncrementalAnalysisPipeline::clear();

    savt::analyzer::AnalyzerOptions options;
    options.precision = savt::analyzer::AnalyzerPrecision::SyntaxOnly;

    const auto baselineArtifacts =
        savt::ui::IncrementalAnalysisPipeline::analyze(tempRoot, options);
    expect(!baselineArtifacts.parseLayer.hit,
           "baseline incremental run should build a fresh parse cache entry");

    std::this_thread::sleep_for(20ms);
    writeTextFile(tempRoot / "src" / "helper.cpp", "int helper() { return helper_impl(); }\nint helper_impl() { return 9; }\n");

    const auto invalidatedArtifacts =
        savt::ui::IncrementalAnalysisPipeline::analyze(tempRoot, options);
    expect(!invalidatedArtifacts.scanLayer.hit,
           "changing a tracked source file should invalidate the scan cache");
    expect(!invalidatedArtifacts.parseLayer.hit,
           "changing a tracked source file should invalidate the parse cache");
    expect(!invalidatedArtifacts.aggregateLayer.hit,
           "changing a tracked source file should invalidate the aggregate cache");
    expect(!invalidatedArtifacts.layoutLayer.hit,
           "changing a tracked source file should invalidate the layout cache");
    expect(invalidatedArtifacts.report.nodes.size() >= baselineArtifacts.report.nodes.size(),
           "invalidated rerun should rebuild the updated symbol graph");

    std::error_code errorCode;
    fs::remove_all(tempRoot, errorCode);
}

}  // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    testAstPreviewServiceBuildsItemsAndParsesCppPreview();
    testReportServiceBuildsReadableSystemContextPayload();
    testAnalysisControllerStartsFromStableDefaultState();
    testAiServiceClassifiesCapabilityAndComponentScopes();
    testFileInsightServiceBuildsPortableSingleFileSummary();
    testAiServiceParseReplyUsesScopeSpecificStatusMessages();
    testIncrementalAnalysisPipelineReusesStableLayers();
    testIncrementalAnalysisPipelineInvalidatesChangedSourceFiles();
    std::cout << "[PASS] savt_ui_tests\n";
    return 0;
}
