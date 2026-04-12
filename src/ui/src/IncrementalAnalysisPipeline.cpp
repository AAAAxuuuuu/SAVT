#include "savt/ui/IncrementalAnalysisPipeline.h"

#include "savt/core/ArchitectureAggregation.h"
#include "savt/core/ComponentGraph.h"
#include "savt/core/ProjectAnalysisConfig.h"

#include <QDebug>
#include <QElapsedTimer>

#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string_view>
#include <system_error>
#include <unordered_map>

namespace savt::ui {
namespace {

constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ull;
constexpr std::uint64_t kFnvPrime = 1099511628211ull;
constexpr std::string_view kIncrementalAnalysisCacheVersion = "phase12-v1";

struct CachedScanEntry {
    savt::analyzer::ProjectScanManifest manifest;
    std::string contentFingerprint;
};

struct CachedAggregateEntry {
    savt::core::ArchitectureAggregation aggregation;
};

struct CachedLayoutEntry {
    savt::layout::CapabilitySceneLayoutResult capabilitySceneLayout;
    savt::layout::LayoutResult moduleLayout;
    std::unordered_map<std::size_t, savt::core::ComponentGraph> componentGraphs;
    std::unordered_map<std::size_t, savt::layout::ComponentSceneLayoutResult> componentLayouts;
};

auto& cacheMutex() {
    static std::mutex mutex;
    return mutex;
}

auto& scanCache() {
    static std::unordered_map<std::string, CachedScanEntry> cache;
    return cache;
}

auto& parseCache() {
    static std::unordered_map<std::string, savt::core::AnalysisReport> cache;
    return cache;
}

auto& aggregateCache() {
    static std::unordered_map<std::string, CachedAggregateEntry> cache;
    return cache;
}

auto& layoutCache() {
    static std::unordered_map<std::string, CachedLayoutEntry> cache;
    return cache;
}

void hashBytes(std::uint64_t& state, const void* data, const std::size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t index = 0; index < size; ++index) {
        state ^= static_cast<std::uint64_t>(bytes[index]);
        state *= kFnvPrime;
    }
}

void hashString(std::uint64_t& state, const std::string_view value) {
    hashBytes(state, value.data(), value.size());
}

std::string hashToHex(const std::uint64_t value) {
    std::ostringstream output;
    output << std::hex << std::setfill('0') << std::setw(16) << value;
    return output.str();
}

std::string normalizePath(const std::filesystem::path& path) {
    return path.lexically_normal().generic_string();
}

bool isCancellationRequested(const savt::analyzer::AnalyzerOptions& options) {
    return options.cancellationRequested && options.cancellationRequested();
}

void publishProgress(
    const IncrementalProgressCallback& progress,
    const int value,
    const std::string& label) {
    if (progress) {
        progress(value, label);
    }
}

std::string buildAnalyzerOptionsSignature(const savt::analyzer::AnalyzerOptions& options) {
    std::ostringstream output;
    output << "third_party=" << (options.includeThirdParty ? 1 : 0)
           << ";build_dirs=" << (options.includeBuildDirectories ? 1 : 0)
           << ";max_files=" << options.maxFiles
           << ";precision=" << static_cast<int>(options.precision)
           << ";tooling=" << SAVT_CLANG_TOOLING_STATUS_CODE;
    if (!options.compilationDatabasePath.empty()) {
        output << ";compdb=" << normalizePath(options.compilationDatabasePath);
    }
    return output.str();
}

std::optional<std::filesystem::path> locateCompilationDatabase(
    const std::filesystem::path& rootPath,
    const savt::analyzer::AnalyzerOptions& options) {
    if (!options.compilationDatabasePath.empty()) {
        std::error_code errorCode;
        const std::filesystem::path explicitPath = options.compilationDatabasePath.lexically_normal();
        if (std::filesystem::exists(explicitPath, errorCode)) {
            return explicitPath;
        }
        return std::nullopt;
    }

    const std::array<std::filesystem::path, 4> candidates = {
        rootPath / "compile_commands.json",
        rootPath / ".qtc_clangd" / "compile_commands.json",
        rootPath / "build" / "compile_commands.json",
        rootPath / "build" / ".qtc_clangd" / "compile_commands.json"
    };
    for (const std::filesystem::path& candidate : candidates) {
        std::error_code errorCode;
        if (std::filesystem::exists(candidate, errorCode)) {
            return candidate.lexically_normal();
        }
    }
    return std::nullopt;
}

std::string hashFileContents(const std::filesystem::path& filePath) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input) {
        return "unreadable";
    }

    std::uint64_t state = kFnvOffsetBasis;
    std::array<char, 8192> buffer{};
    while (input.good()) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = input.gcount();
        if (count <= 0) {
            break;
        }
        hashBytes(state, buffer.data(), static_cast<std::size_t>(count));
    }
    return hashToHex(state);
}

std::string buildProjectConfigSignature(const std::filesystem::path& rootPath) {
    const savt::core::ProjectAnalysisConfig config =
        savt::core::loadProjectAnalysisConfig(rootPath);
    std::ostringstream output;
    output << "loaded=" << (config.loaded ? 1 : 0);
    if (!config.configPath.empty()) {
        output << ";path=" << config.configPath
               << ";hash=" << hashFileContents(std::filesystem::path(config.configPath));
    } else {
        output << ";path=none";
    }
    return output.str();
}

std::string buildCompilationDatabaseSignature(
    const std::filesystem::path& rootPath,
    const savt::analyzer::AnalyzerOptions& options) {
    if (options.precision == savt::analyzer::AnalyzerPrecision::SyntaxOnly &&
        options.compilationDatabasePath.empty()) {
        return "not_requested";
    }

    const auto compilationDatabasePath = locateCompilationDatabase(rootPath, options);
    if (!compilationDatabasePath.has_value()) {
        return "missing";
    }

    return normalizePath(*compilationDatabasePath) + ":" + hashFileContents(*compilationDatabasePath);
}

std::string buildCacheKey(
    const std::filesystem::path& rootPath,
    const std::initializer_list<std::string_view> parts,
    const std::initializer_list<std::string> ownedParts = {}) {
    std::uint64_t state = kFnvOffsetBasis;
    hashString(state, normalizePath(rootPath));
    hashString(state, std::string(kIncrementalAnalysisCacheVersion));
    for (const std::string_view part : parts) {
        hashString(state, part);
    }
    for (const std::string& part : ownedParts) {
        hashString(state, part);
    }
    return hashToHex(state);
}

CachedScanEntry buildScanEntry(
    const savt::analyzer::ProjectScanManifest& manifest) {
    CachedScanEntry entry;
    entry.manifest = manifest;
    // Reuse the scan metadata fingerprint so the first analysis pass does not
    // reread every source file only to seed the incremental cache.
    entry.contentFingerprint = manifest.metadataFingerprint;
    return entry;
}

void appendCacheDiagnostics(
    savt::core::AnalysisReport& report,
    const std::vector<std::string>& cacheDiagnostics) {
    report.diagnostics.insert(
        report.diagnostics.end(),
        cacheDiagnostics.begin(),
        cacheDiagnostics.end());
}

void logPhaseTiming(
    const char* phaseName,
    const qint64 elapsedMs,
    const bool cacheHit) {
    qInfo().noquote()
        << QStringLiteral("[analysis][phase] %1 elapsed_ms=%2 cache_hit=%3")
               .arg(QString::fromUtf8(phaseName),
                    QString::number(elapsedMs),
                    cacheHit ? QStringLiteral("true") : QStringLiteral("false"));
}

}  // namespace

AnalysisArtifacts IncrementalAnalysisPipeline::analyze(
    const std::filesystem::path& rootPath,
    const savt::analyzer::AnalyzerOptions& options,
    IncrementalProgressCallback progress) {
    const std::filesystem::path normalizedRoot = rootPath.lexically_normal();
    const std::string optionsSignature = buildAnalyzerOptionsSignature(options);
    const std::string projectConfigSignature = buildProjectConfigSignature(normalizedRoot);
    const std::string compilationDatabaseSignature =
        buildCompilationDatabaseSignature(normalizedRoot, options);

    AnalysisArtifacts result;
    savt::analyzer::CppProjectAnalyzer analyzer;
    QElapsedTimer scanTimer;
    scanTimer.start();

    publishProgress(progress, 5, "扫描项目目录...");
    const savt::analyzer::ProjectScanManifest manifest =
        analyzer.buildScanManifest(normalizedRoot, options);
    result.scanLayer.key = buildCacheKey(
        normalizedRoot,
        {"scan"},
        {optionsSignature, projectConfigSignature, manifest.metadataFingerprint});
    result.cacheDiagnostics.insert(
        result.cacheDiagnostics.end(),
        manifest.diagnostics.begin(),
        manifest.diagnostics.end());
    if (isCancellationRequested(options)) {
        result.canceled = true;
        result.canceledPhase = "扫描项目目录";
        return result;
    }

    publishProgress(progress, 15, "计算缓存指纹...");
    CachedScanEntry scanEntry;
    {
        std::lock_guard<std::mutex> lock(cacheMutex());
        const auto cached = scanCache().find(result.scanLayer.key);
        if (cached != scanCache().end() &&
            cached->second.manifest.metadataFingerprint == manifest.metadataFingerprint) {
            scanEntry = cached->second;
            result.scanLayer.hit = true;
        }
    }

    if (!result.scanLayer.hit) {
        scanEntry = buildScanEntry(manifest);
        std::lock_guard<std::mutex> lock(cacheMutex());
        scanCache()[result.scanLayer.key] = scanEntry;
    }

    result.cacheDiagnostics.push_back(
        result.scanLayer.hit
            ? "Incremental cache scan hit: reused unchanged source manifest and metadata fingerprint."
            : "Incremental cache scan miss: refreshed source manifest and metadata fingerprint.");
    logPhaseTiming("scan", scanTimer.elapsed(), result.scanLayer.hit);

    result.parseLayer.key = buildCacheKey(
        normalizedRoot,
        {"parse"},
        {
            optionsSignature,
            projectConfigSignature,
            compilationDatabaseSignature,
            scanEntry.contentFingerprint
        });
    QElapsedTimer parseTimer;
    parseTimer.start();

    publishProgress(
        progress,
        25,
        result.parseLayer.hit ? "命中解析缓存..." : "解析源码并构建关系图...");
    {
        std::lock_guard<std::mutex> lock(cacheMutex());
        const auto cached = parseCache().find(result.parseLayer.key);
        if (cached != parseCache().end()) {
            result.report = cached->second;
            result.parseLayer.hit = true;
        }
    }

    if (!result.parseLayer.hit) {
        result.report = analyzer.analyzeProject(normalizedRoot, options);
        std::lock_guard<std::mutex> lock(cacheMutex());
        parseCache()[result.parseLayer.key] = result.report;
    }
    result.cacheDiagnostics.push_back(
        result.parseLayer.hit
            ? "Incremental cache parse hit: reused cached analysis report."
            : "Incremental cache parse miss: rebuilt analysis report from source.");
    logPhaseTiming("parse", parseTimer.elapsed(), result.parseLayer.hit);
    if (isCancellationRequested(options)) {
        result.canceled = true;
        result.canceledPhase = "解析源码并构建关系图";
        return result;
    }

    result.aggregateLayer.key = buildCacheKey(
        normalizedRoot,
        {"aggregate"},
        {result.parseLayer.key});
    QElapsedTimer aggregateTimer;
    aggregateTimer.start();
    publishProgress(
        progress,
        75,
        result.aggregateLayer.hit ? "命中聚合缓存..." : "提炼架构总览...");
    {
        std::lock_guard<std::mutex> lock(cacheMutex());
        const auto cached = aggregateCache().find(result.aggregateLayer.key);
        if (cached != aggregateCache().end()) {
            result.overview = cached->second.aggregation.overview;
            result.capabilityGraph = cached->second.aggregation.capabilityGraph;
            result.ruleReport = cached->second.aggregation.ruleReport;
            result.aggregateLayer.hit = true;
        }
    }

    if (!result.aggregateLayer.hit) {
        const savt::core::ArchitectureAggregation aggregation =
            savt::core::buildArchitectureAggregation(result.report);
        result.overview = aggregation.overview;
        result.capabilityGraph = aggregation.capabilityGraph;
        result.ruleReport = aggregation.ruleReport;
        std::lock_guard<std::mutex> lock(cacheMutex());
        aggregateCache()[result.aggregateLayer.key] = CachedAggregateEntry{aggregation};
    }
    result.cacheDiagnostics.push_back(
        result.aggregateLayer.hit
            ? "Incremental cache aggregate hit: reused overview, capability graph, and architecture rules."
            : "Incremental cache aggregate miss: rebuilt overview, capability graph, and architecture rules.");
    logPhaseTiming("aggregate", aggregateTimer.elapsed(), result.aggregateLayer.hit);
    if (isCancellationRequested(options)) {
        result.canceled = true;
        result.canceledPhase = "提炼架构总览";
        return result;
    }

    result.layoutLayer.key = buildCacheKey(
        normalizedRoot,
        {"layout"},
        {result.aggregateLayer.key});
    QElapsedTimer layoutTimer;
    layoutTimer.start();
    publishProgress(
        progress,
        92,
        result.layoutLayer.hit ? "命中布局缓存..." : "计算模块布局...");
    {
        std::lock_guard<std::mutex> lock(cacheMutex());
        const auto cached = layoutCache().find(result.layoutLayer.key);
        if (cached != layoutCache().end()) {
            result.capabilitySceneLayout = cached->second.capabilitySceneLayout;
            result.moduleLayout = cached->second.moduleLayout;
            result.componentGraphs = cached->second.componentGraphs;
            result.componentLayouts = cached->second.componentLayouts;
            result.layoutLayer.hit = true;
        }
    }

    if (!result.layoutLayer.hit) {
        savt::layout::LayeredGraphLayout layoutEngine;
        result.capabilitySceneLayout = layoutEngine.layoutCapabilityScene(result.capabilityGraph);
        result.moduleLayout = layoutEngine.layoutModules(result.report);
        publishProgress(progress, 94, "生成 L3 组件视图...");
        for (const savt::core::CapabilityNode& capabilityNode : result.capabilityGraph.nodes) {
            if (isCancellationRequested(options)) {
                result.canceled = true;
                result.canceledPhase = "生成 L3 组件视图";
                return result;
            }

            auto componentGraph = savt::core::buildComponentGraphForCapability(
                result.report,
                result.overview,
                result.capabilityGraph,
                capabilityNode.id);
            auto componentLayout = layoutEngine.layoutComponentScene(componentGraph);
            result.componentGraphs.emplace(capabilityNode.id, std::move(componentGraph));
            result.componentLayouts.emplace(capabilityNode.id, std::move(componentLayout));
        }
        std::lock_guard<std::mutex> lock(cacheMutex());
        layoutCache()[result.layoutLayer.key] = CachedLayoutEntry{
            result.capabilitySceneLayout,
            result.moduleLayout,
            result.componentGraphs,
            result.componentLayouts};
    }
    result.cacheDiagnostics.push_back(
        result.layoutLayer.hit
            ? "Incremental cache layout hit: reused capability and component scene layouts."
            : "Incremental cache layout miss: recomputed capability and component scene layouts.");
    logPhaseTiming("layout", layoutTimer.elapsed(), result.layoutLayer.hit);

    appendCacheDiagnostics(result.report, result.cacheDiagnostics);
    return result;
}

void IncrementalAnalysisPipeline::clear() {
    std::lock_guard<std::mutex> lock(cacheMutex());
    scanCache().clear();
    parseCache().clear();
    aggregateCache().clear();
    layoutCache().clear();
}

std::string IncrementalAnalysisPipeline::cacheVersion() {
    return std::string(kIncrementalAnalysisCacheVersion);
}

}  // namespace savt::ui
