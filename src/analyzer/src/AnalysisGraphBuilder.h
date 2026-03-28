#pragma once

#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ArchitectureGraph.h"

#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace savt::analyzer::detail {

struct FileRecord {
    std::filesystem::path absolutePath;
    std::string relativePath;
    std::size_t nodeId = 0;
};

class AnalysisGraphBuilder {
public:
    AnalysisGraphBuilder(std::filesystem::path rootPath, const AnalyzerOptions& options);

    savt::core::AnalysisReport& report();
    const savt::core::AnalysisReport& report() const;

    const std::filesystem::path& rootPath() const;
    const AnalyzerOptions& options() const;
    const std::vector<FileRecord>& files() const;

    std::size_t addOrMergeNode(
        savt::core::SymbolKind kind,
        std::string displayName,
        std::string qualifiedName,
        const std::string& filePath,
        std::uint32_t line,
        std::string identityKey = {},
        savt::core::FactSource factSource = savt::core::FactSource::Inferred);

    void addEdge(
        std::size_t fromId,
        std::size_t toId,
        savt::core::EdgeKind kind,
        std::size_t weight = 1,
        bool accumulateWeight = false,
        std::size_t supportCount = 1,
        savt::core::FactSource factSource = savt::core::FactSource::Inferred);

    void registerProjectFiles(const std::vector<std::filesystem::path>& sourceFiles);
    void registerModules();
    void bindNodeToFile(std::size_t nodeId, std::size_t fileNodeId);
    void aggregateModuleDependencies(std::initializer_list<savt::core::EdgeKind> edgeKinds);

    const savt::core::SymbolNode* nodeById(std::size_t nodeId) const;

    std::optional<std::size_t> findSymbolByQualifiedName(const std::string& qualifiedName) const;
    std::vector<std::size_t> findSymbolsByDisplayName(const std::string& displayName) const;
    std::optional<std::size_t> findFileIdByRelativePath(const std::string& relativePath) const;
    std::optional<std::size_t> findFileIdByPath(const std::filesystem::path& candidatePath) const;
    std::optional<std::size_t> resolveIncludeTarget(
        const std::string& target,
        const std::filesystem::path& sourceFile,
        bool system) const;
    std::optional<std::size_t> fileIdForNode(std::size_t nodeId) const;
    bool isProjectFilePath(const std::filesystem::path& candidatePath) const;

private:
    savt::core::SymbolNode* mutableNodeById(std::size_t nodeId);

    std::filesystem::path rootPath_;
    AnalyzerOptions options_;
    savt::core::AnalysisReport report_;
    std::size_t nextNodeId_ = 1;
    std::vector<FileRecord> files_;
    std::unordered_map<std::string, std::size_t> identityToNodeId_;
    std::unordered_map<std::string, std::vector<std::size_t>> qualifiedToNodeIds_;
    std::unordered_map<std::string, std::vector<std::size_t>> displayNameToNodeIds_;
    std::unordered_map<std::string, std::size_t> absolutePathToFileId_;
    std::unordered_map<std::string, std::size_t> relativePathToFileId_;
    std::unordered_map<std::string, std::vector<std::size_t>> basenameToFileIds_;
    std::unordered_map<std::size_t, std::size_t> nodeIdToFileId_;
    std::unordered_map<std::size_t, std::size_t> fileIdToModuleId_;
};

}  // namespace savt::analyzer::detail
