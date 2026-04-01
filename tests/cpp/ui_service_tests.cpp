#include "savt/ui/AnalysisController.h"
#include "savt/ui/AstPreviewService.h"
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
    graph.nodes.push_back(analysisCapability);

    graph.groups.push_back({1, savt::core::CapabilityGroupKind::Lane, "Entry", "", {10}, true, true, 0});
    graph.groups.push_back({2, savt::core::CapabilityGroupKind::Lane, "Capability", "", {11}, true, true, 0});

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
    testIncrementalAnalysisPipelineReusesStableLayers();
    testIncrementalAnalysisPipelineInvalidatesChangedSourceFiles();
    std::cout << "[PASS] savt_ui_tests\n";
    return 0;
}
