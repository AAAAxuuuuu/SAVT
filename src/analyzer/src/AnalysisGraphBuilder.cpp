#include "AnalysisGraphBuilder.h"

#include "AnalyzerUtilities.h"

#include <algorithm>

namespace savt::analyzer::detail {
namespace {

bool endsWithPathSuffix(const std::string& path, const std::string& suffix) {
    if (suffix.empty()) {
        return false;
    }

    if (path == suffix) {
        return true;
    }

    if (path.size() <= suffix.size()) {
        return false;
    }

    return path.ends_with('/' + suffix);
}

savt::core::FactSource mergeFactSource(
    const savt::core::FactSource left,
    const savt::core::FactSource right) {
    return left == savt::core::FactSource::Semantic || right == savt::core::FactSource::Semantic
               ? savt::core::FactSource::Semantic
               : savt::core::FactSource::Inferred;
}

void appendUniqueNodeId(std::vector<std::size_t>& nodeIds, const std::size_t nodeId) {
    if (std::find(nodeIds.begin(), nodeIds.end(), nodeId) == nodeIds.end()) {
        nodeIds.push_back(nodeId);
    }
}

}  // namespace

AnalysisGraphBuilder::AnalysisGraphBuilder(std::filesystem::path rootPath, const AnalyzerOptions& options)
    : rootPath_(rootPath.lexically_normal()),
      options_(options) {
    report_.rootPath = normalizePath(rootPath_);
}

savt::core::AnalysisReport& AnalysisGraphBuilder::report() {
    return report_;
}

const savt::core::AnalysisReport& AnalysisGraphBuilder::report() const {
    return report_;
}

const std::filesystem::path& AnalysisGraphBuilder::rootPath() const {
    return rootPath_;
}

const AnalyzerOptions& AnalysisGraphBuilder::options() const {
    return options_;
}

const std::vector<FileRecord>& AnalysisGraphBuilder::files() const {
    return files_;
}

std::size_t AnalysisGraphBuilder::addOrMergeNode(
    const savt::core::SymbolKind kind,
    std::string displayName,
    std::string qualifiedName,
    const std::string& filePath,
    const std::uint32_t line,
    std::string identityKey,
    const savt::core::FactSource factSource) {
    if (displayName.empty()) {
        displayName = qualifiedName;
    }

    std::optional<std::size_t> existingId;
    if (!identityKey.empty()) {
        const auto identityIt = identityToNodeId_.find(identityKey);
        if (identityIt != identityToNodeId_.end()) {
            existingId = identityIt->second;
        }
    }

    if (!existingId.has_value() && !qualifiedName.empty()) {
        const auto qualifiedIt = qualifiedToNodeIds_.find(qualifiedName);
        if (qualifiedIt != qualifiedToNodeIds_.end() && qualifiedIt->second.size() == 1) {
            const std::size_t candidateId = qualifiedIt->second.front();
            const savt::core::SymbolNode* candidateNode = nodeById(candidateId);
            const bool compatibleIdentity =
                candidateNode != nullptr &&
                (identityKey.empty() || candidateNode->identityKey.empty() || candidateNode->identityKey == identityKey);
            const bool compatibleKind = candidateNode != nullptr && candidateNode->kind == kind;
            if (compatibleIdentity && compatibleKind) {
                existingId = candidateId;
            }
        }
    }

    if (existingId.has_value()) {
        if (savt::core::SymbolNode* existingNode = mutableNodeById(*existingId); existingNode != nullptr) {
            if (existingNode->displayName.empty()) {
                existingNode->displayName = displayName;
            }
            if (existingNode->qualifiedName.empty()) {
                existingNode->qualifiedName = qualifiedName;
            }
            if (existingNode->identityKey.empty()) {
                existingNode->identityKey = identityKey;
            }
            if (existingNode->filePath.empty() && !filePath.empty()) {
                existingNode->filePath = filePath;
            }
            if (existingNode->line == 0 && line != 0) {
                existingNode->line = line;
            }
            existingNode->factSource = mergeFactSource(existingNode->factSource, factSource);

            if (!existingNode->identityKey.empty()) {
                identityToNodeId_.emplace(existingNode->identityKey, existingNode->id);
            }
            if (!existingNode->qualifiedName.empty()) {
                appendUniqueNodeId(qualifiedToNodeIds_[existingNode->qualifiedName], existingNode->id);
            }

            if (kind != savt::core::SymbolKind::Module && !filePath.empty()) {
                if (const auto fileId = findFileIdByRelativePath(filePath); fileId.has_value()) {
                    nodeIdToFileId_[existingNode->id] = *fileId;
                }
            }
        }
        return *existingId;
    }

    const std::size_t nodeId = nextNodeId_++;
    report_.nodes.push_back(savt::core::SymbolNode{
        nodeId,
        kind,
        std::move(displayName),
        std::move(qualifiedName),
        std::move(identityKey),
        filePath,
        line,
        factSource
    });

    const savt::core::SymbolNode& stored = report_.nodes.back();
    if (!stored.identityKey.empty()) {
        identityToNodeId_.emplace(stored.identityKey, stored.id);
    }
    if (!stored.qualifiedName.empty()) {
        appendUniqueNodeId(qualifiedToNodeIds_[stored.qualifiedName], stored.id);
    }
    displayNameToNodeIds_[stored.displayName].push_back(stored.id);

    if (kind == savt::core::SymbolKind::File) {
        nodeIdToFileId_.emplace(nodeId, nodeId);
    } else if (kind != savt::core::SymbolKind::Module && !filePath.empty()) {
        if (const auto fileId = findFileIdByRelativePath(filePath); fileId.has_value()) {
            nodeIdToFileId_.emplace(nodeId, *fileId);
        }
    }

    return nodeId;
}

void AnalysisGraphBuilder::addEdge(
    const std::size_t fromId,
    const std::size_t toId,
    const savt::core::EdgeKind kind,
    const std::size_t weight,
    const bool accumulateWeight,
    const std::size_t supportCount,
    const savt::core::FactSource factSource) {
    if (fromId == 0 || toId == 0 || fromId == toId) {
        return;
    }

    auto duplicate = std::find_if(
        report_.edges.begin(),
        report_.edges.end(),
        [fromId, toId, kind](const savt::core::SymbolEdge& edge) {
            return edge.fromId == fromId && edge.toId == toId && edge.kind == kind;
        });

    if (duplicate != report_.edges.end()) {
        if (accumulateWeight) {
            duplicate->weight += weight;
            duplicate->supportCount += supportCount;
        } else {
            duplicate->supportCount = std::max(duplicate->supportCount, supportCount);
        }
        duplicate->factSource = mergeFactSource(duplicate->factSource, factSource);
        return;
    }

    report_.edges.push_back(savt::core::SymbolEdge{fromId, toId, kind, weight, supportCount, factSource});
}

void AnalysisGraphBuilder::registerProjectFiles(const std::vector<std::filesystem::path>& sourceFiles) {
    files_.reserve(sourceFiles.size());

    for (const std::filesystem::path& filePath : sourceFiles) {
        const std::string normalizedAbsolute = normalizePath(filePath);
        if (absolutePathToFileId_.contains(normalizedAbsolute)) {
            continue;
        }

        const std::string relativePath = relativizePath(rootPath_, filePath);
        const std::size_t nodeId = addOrMergeNode(
            savt::core::SymbolKind::File,
            filePath.filename().string(),
            relativePath,
            relativePath,
            0,
            "file:" + relativePath);

        files_.push_back(FileRecord{filePath.lexically_normal(), relativePath, nodeId});
        absolutePathToFileId_.emplace(normalizedAbsolute, nodeId);
        relativePathToFileId_.emplace(relativePath, nodeId);
        basenameToFileIds_[filePath.filename().string()].push_back(nodeId);
    }
}

void AnalysisGraphBuilder::registerModules() {
    std::unordered_map<std::string, std::size_t> moduleNameToId;
    std::vector<std::string> relativePaths;
    relativePaths.reserve(files_.size());
    for (const FileRecord& file : files_) {
        relativePaths.push_back(file.relativePath);
    }
    const std::unordered_map<std::string, std::string> inferredModuleNames = inferModuleNames(relativePaths);

    for (const FileRecord& file : files_) {
        const auto inferredIt = inferredModuleNames.find(file.relativePath);
        const std::string moduleName = inferredIt == inferredModuleNames.end()
                                           ? inferModuleName(file.relativePath)
                                           : inferredIt->second;
        auto moduleIt = moduleNameToId.find(moduleName);
        if (moduleIt == moduleNameToId.end()) {
            const std::size_t moduleId = addOrMergeNode(
                savt::core::SymbolKind::Module,
                moduleName,
                "module:" + moduleName,
                moduleName,
                0,
                "module:" + moduleName);
            moduleIt = moduleNameToId.emplace(moduleName, moduleId).first;
        }

        fileIdToModuleId_.emplace(file.nodeId, moduleIt->second);
        addEdge(moduleIt->second, file.nodeId, savt::core::EdgeKind::Contains);
    }
}

void AnalysisGraphBuilder::bindNodeToFile(const std::size_t nodeId, const std::size_t fileNodeId) {
    if (nodeId != 0 && fileNodeId != 0) {
        nodeIdToFileId_[nodeId] = fileNodeId;
    }
}

void AnalysisGraphBuilder::aggregateModuleDependencies(
    const std::initializer_list<savt::core::EdgeKind> edgeKinds) {
    const std::vector<savt::core::SymbolEdge> baseEdges = report_.edges;
    for (const savt::core::SymbolEdge& edge : baseEdges) {
        if (std::find(edgeKinds.begin(), edgeKinds.end(), edge.kind) == edgeKinds.end()) {
            continue;
        }

        const auto fromFileIt = nodeIdToFileId_.find(edge.fromId);
        const auto toFileIt = nodeIdToFileId_.find(edge.toId);
        if (fromFileIt == nodeIdToFileId_.end() || toFileIt == nodeIdToFileId_.end()) {
            continue;
        }

        const auto fromModuleIt = fileIdToModuleId_.find(fromFileIt->second);
        const auto toModuleIt = fileIdToModuleId_.find(toFileIt->second);
        if (fromModuleIt == fileIdToModuleId_.end() || toModuleIt == fileIdToModuleId_.end()) {
            continue;
        }

        if (fromModuleIt->second == toModuleIt->second) {
            continue;
        }

        addEdge(
            fromModuleIt->second,
            toModuleIt->second,
            savt::core::EdgeKind::DependsOn,
            edge.weight,
            true,
            edge.supportCount,
            savt::core::FactSource::Inferred);
    }
}

const savt::core::SymbolNode* AnalysisGraphBuilder::nodeById(const std::size_t nodeId) const {
    return savt::core::findNodeById(report_, nodeId);
}

std::optional<std::size_t> AnalysisGraphBuilder::findSymbolByQualifiedName(const std::string& qualifiedName) const {
    const auto iterator = qualifiedToNodeIds_.find(qualifiedName);
    if (iterator == qualifiedToNodeIds_.end() || iterator->second.size() != 1) {
        return std::nullopt;
    }
    return iterator->second.front();
}

std::vector<std::size_t> AnalysisGraphBuilder::findSymbolsByDisplayName(const std::string& displayName) const {
    const auto iterator = displayNameToNodeIds_.find(displayName);
    if (iterator == displayNameToNodeIds_.end()) {
        return {};
    }
    return iterator->second;
}

std::optional<std::size_t> AnalysisGraphBuilder::findFileIdByRelativePath(const std::string& relativePath) const {
    const auto iterator = relativePathToFileId_.find(relativePath);
    if (iterator == relativePathToFileId_.end()) {
        return std::nullopt;
    }
    return iterator->second;
}

std::optional<std::size_t> AnalysisGraphBuilder::findFileIdByPath(const std::filesystem::path& candidatePath) const {
    const std::string normalizedAbsolute = normalizePath(candidatePath);
    const auto absoluteIt = absolutePathToFileId_.find(normalizedAbsolute);
    if (absoluteIt != absolutePathToFileId_.end()) {
        return absoluteIt->second;
    }

    std::error_code errorCode;
    const std::filesystem::path relativePath = std::filesystem::relative(candidatePath, rootPath_, errorCode);
    if (!errorCode) {
        const std::string normalizedRelative = normalizePath(relativePath);
        const auto relativeIt = relativePathToFileId_.find(normalizedRelative);
        if (relativeIt != relativePathToFileId_.end()) {
            return relativeIt->second;
        }
    }

    return std::nullopt;
}

std::optional<std::size_t> AnalysisGraphBuilder::resolveIncludeTarget(
    const std::string& target,
    const std::filesystem::path& sourceFile,
    const bool system) const {
    const std::filesystem::path includePath(target);

    if (!system) {
        const std::filesystem::path localCandidate = sourceFile.parent_path() / includePath;
        if (const auto localMatch = findFileIdByPath(localCandidate); localMatch.has_value()) {
            return localMatch;
        }
    }

    const std::filesystem::path rootCandidate = rootPath_ / includePath;
    if (const auto rootMatch = findFileIdByPath(rootCandidate); rootMatch.has_value()) {
        return rootMatch;
    }

    const std::string normalizedTarget = normalizePath(includePath);
    const auto exactRelativeIt = relativePathToFileId_.find(normalizedTarget);
    if (exactRelativeIt != relativePathToFileId_.end()) {
        return exactRelativeIt->second;
    }

    std::vector<std::size_t> suffixMatches;
    for (const FileRecord& candidate : files_) {
        if (endsWithPathSuffix(candidate.relativePath, normalizedTarget)) {
            suffixMatches.push_back(candidate.nodeId);
        }
    }
    if (suffixMatches.size() == 1) {
        return suffixMatches.front();
    }

    const std::string basename = includePath.filename().string();
    const auto basenameIt = basenameToFileIds_.find(basename);
    if (basenameIt != basenameToFileIds_.end() && basenameIt->second.size() == 1) {
        return basenameIt->second.front();
    }

    return std::nullopt;
}

std::optional<std::size_t> AnalysisGraphBuilder::fileIdForNode(const std::size_t nodeId) const {
    const auto iterator = nodeIdToFileId_.find(nodeId);
    if (iterator == nodeIdToFileId_.end()) {
        return std::nullopt;
    }
    return iterator->second;
}

bool AnalysisGraphBuilder::isProjectFilePath(const std::filesystem::path& candidatePath) const {
    std::error_code errorCode;
    const std::filesystem::path relativePath = std::filesystem::relative(candidatePath, rootPath_, errorCode);
    if (errorCode || relativePath.empty()) {
        return false;
    }

    const std::string normalizedRelative = normalizePath(relativePath);
    if (normalizedRelative.starts_with("../") || normalizedRelative == "..") {
        return false;
    }

    std::filesystem::path segmentPath;
    for (auto iterator = relativePath.begin(); iterator != relativePath.end(); ++iterator) {
        const auto next = std::next(iterator);
        if (next == relativePath.end()) {
            break;
        }

        segmentPath /= *iterator;
        if (shouldSkipDirectory(segmentPath, options_)) {
            return false;
        }
    }

    return true;
}

savt::core::SymbolNode* AnalysisGraphBuilder::mutableNodeById(const std::size_t nodeId) {
    const auto iterator = std::find_if(report_.nodes.begin(), report_.nodes.end(), [nodeId](const savt::core::SymbolNode& node) {
        return node.id == nodeId;
    });
    return iterator == report_.nodes.end() ? nullptr : &(*iterator);
}

}  // namespace savt::analyzer::detail
