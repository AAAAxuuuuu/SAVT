#include "SyntaxProjectAnalyzer.h"

#include "AnalysisGraphBuilder.h"
#include "AnalyzerUtilities.h"

#include <tree_sitter/api.h>
#include <tree-sitter-cpp.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace savt::analyzer::detail {
namespace {

using savt::core::EdgeKind;
using savt::core::SymbolKind;

struct ScopeInfo {
    std::size_t nodeId = 0;
    SymbolKind kind = SymbolKind::Namespace;
    std::string displayName;
    std::string qualifiedName;
};

struct PendingCall {
    std::size_t callerId = 0;
    std::string calleeName;
    std::string namespacePrefix;
    std::string typePrefix;
    std::string callerQualifiedName;
    std::string filePath;
    std::uint32_t line = 0;
};

struct IncludeDirective {
    std::string target;
    std::uint32_t line = 0;
    bool system = false;
};

std::string nodeText(const std::string_view source, const TSNode node) {
    if (ts_node_is_null(node)) {
        return {};
    }

    const std::uint32_t start = ts_node_start_byte(node);
    const std::uint32_t end = ts_node_end_byte(node);
    if (end <= start || end > source.size()) {
        return {};
    }

    return std::string(source.substr(start, end - start));
}

std::string compactText(std::string text) {
    text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }), text.end());
    return text;
}

std::string basenameFromQualified(const std::string& name) {
    const std::size_t position = name.rfind("::");
    if (position == std::string::npos) {
        return name;
    }
    return name.substr(position + 2);
}

std::string namespacePrefix(const std::vector<ScopeInfo>& scopes) {
    std::string prefix;
    for (const ScopeInfo& scope : scopes) {
        if (scope.kind != SymbolKind::Namespace) {
            continue;
        }

        if (!prefix.empty()) {
            prefix += "::";
        }
        prefix += scope.displayName;
    }
    return prefix;
}

std::optional<ScopeInfo> currentTypeScope(const std::vector<ScopeInfo>& scopes) {
    for (auto iterator = scopes.rbegin(); iterator != scopes.rend(); ++iterator) {
        if (iterator->kind == SymbolKind::Class || iterator->kind == SymbolKind::Struct) {
            return *iterator;
        }
    }
    return std::nullopt;
}

std::string qualifyName(
    const std::string& rawName,
    const std::string& namespaceScope,
    const std::optional<ScopeInfo>& typeScope) {
    if (rawName.empty()) {
        return {};
    }

    if (rawName.starts_with("::")) {
        return rawName.substr(2);
    }

    if (rawName.find("::") != std::string::npos) {
        if (!namespaceScope.empty() && !rawName.starts_with(namespaceScope + "::")) {
            return namespaceScope + "::" + rawName;
        }
        return rawName;
    }

    if (typeScope.has_value()) {
        return typeScope->qualifiedName + "::" + rawName;
    }

    if (!namespaceScope.empty()) {
        return namespaceScope + "::" + rawName;
    }

    return rawName;
}

TSNode fieldNode(const TSNode node, const char* name) {
    return ts_node_child_by_field_name(node, name, std::strlen(name));
}

std::string extractEntityName(const TSNode node, const std::string_view source) {
    if (ts_node_is_null(node)) {
        return {};
    }

    const std::string type = ts_node_type(node);
    if (type == "identifier" ||
        type == "field_identifier" ||
        type == "type_identifier" ||
        type == "namespace_identifier" ||
        type == "destructor_name" ||
        type == "operator_name" ||
        type == "qualified_identifier" ||
        type == "scoped_identifier" ||
        type == "nested_namespace_specifier" ||
        type == "template_function" ||
        type == "template_method" ||
        type == "template_type") {
        return compactText(nodeText(source, node));
    }

    const TSNode nameNode = fieldNode(node, "name");
    if (!ts_node_is_null(nameNode)) {
        const std::string name = extractEntityName(nameNode, source);
        if (!name.empty()) {
            return name;
        }
    }

    const TSNode declaratorNode = fieldNode(node, "declarator");
    if (!ts_node_is_null(declaratorNode)) {
        const std::string name = extractEntityName(declaratorNode, source);
        if (!name.empty()) {
            return name;
        }
    }

    for (std::int32_t index = static_cast<std::int32_t>(ts_node_named_child_count(node)) - 1; index >= 0; --index) {
        const TSNode child = ts_node_named_child(node, static_cast<std::uint32_t>(index));
        const std::string name = extractEntityName(child, source);
        if (!name.empty()) {
            return name;
        }
    }

    return {};
}

std::string extractCallTarget(const TSNode node, const std::string_view source) {
    if (ts_node_is_null(node)) {
        return {};
    }

    const std::string type = ts_node_type(node);
    if (type == "field_expression") {
        const TSNode field = fieldNode(node, "field");
        if (!ts_node_is_null(field)) {
            return extractEntityName(field, source);
        }
    }

    if (type == "call_expression") {
        return extractCallTarget(fieldNode(node, "function"), source);
    }

    return extractEntityName(node, source);
}

std::string_view trimLeft(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1);
    }
    return text;
}

std::vector<IncludeDirective> extractIncludeDirectives(const std::string_view source) {
    std::vector<IncludeDirective> directives;
    std::size_t offset = 0;
    std::uint32_t lineNumber = 0;

    while (offset <= source.size()) {
        const std::size_t nextNewline = source.find('\n', offset);
        std::string_view line = nextNewline == std::string_view::npos
                                    ? source.substr(offset)
                                    : source.substr(offset, nextNewline - offset);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }

        std::string_view trimmed = trimLeft(line);
        if (trimmed.starts_with('#')) {
            trimmed.remove_prefix(1);
            trimmed = trimLeft(trimmed);
            if (trimmed.starts_with("include")) {
                trimmed.remove_prefix(std::strlen("include"));
                trimmed = trimLeft(trimmed);
                if (!trimmed.empty() && (trimmed.front() == '"' || trimmed.front() == '<')) {
                    const bool system = trimmed.front() == '<';
                    const char closing = system ? '>' : '"';
                    trimmed.remove_prefix(1);
                    const std::size_t closingIndex = trimmed.find(closing);
                    if (closingIndex != std::string_view::npos) {
                        directives.push_back(IncludeDirective{
                            std::string(trimmed.substr(0, closingIndex)),
                            lineNumber,
                            system
                        });
                    }
                }
            }
        }

        if (nextNewline == std::string_view::npos) {
            break;
        }

        offset = nextNewline + 1;
        ++lineNumber;
    }

    return directives;
}

void analyzeIncludes(
    const std::string_view source,
    const FileRecord& file,
    AnalysisGraphBuilder& builder) {
    for (const IncludeDirective& include : extractIncludeDirectives(source)) {
        const auto resolved = builder.resolveIncludeTarget(include.target, file.absolutePath, include.system);
        if (resolved.has_value()) {
            builder.addEdge(file.nodeId, *resolved, EdgeKind::Includes);
            continue;
        }

        if (!include.system) {
            std::ostringstream diagnostic;
            diagnostic << file.relativePath << ":" << (include.line + 1)
                       << " unresolved include: " << include.target;
            builder.report().diagnostics.push_back(diagnostic.str());
        }
    }
}

void collectCalls(
    const TSNode node,
    const std::string_view source,
    const std::string& filePath,
    const std::size_t callerId,
    const std::string& callerQualifiedName,
    const std::string& namespaceScope,
    const std::optional<ScopeInfo>& typeScope,
    std::vector<PendingCall>& pendingCalls) {
    if (ts_node_is_null(node)) {
        return;
    }

    if (std::string_view(ts_node_type(node)) == "call_expression") {
        const TSNode functionNode = fieldNode(node, "function");
        const std::string calleeName = extractCallTarget(functionNode, source);
        if (!calleeName.empty()) {
            pendingCalls.push_back(PendingCall{
                callerId,
                calleeName,
                namespaceScope,
                typeScope.has_value() ? typeScope->qualifiedName : std::string(),
                callerQualifiedName,
                filePath,
                ts_node_start_point(node).row
            });
        }
    }

    const std::uint32_t childCount = ts_node_named_child_count(node);
    for (std::uint32_t index = 0; index < childCount; ++index) {
        collectCalls(
            ts_node_named_child(node, index),
            source,
            filePath,
            callerId,
            callerQualifiedName,
            namespaceScope,
            typeScope,
            pendingCalls);
    }
}

void analyzeNode(
    const TSNode node,
    const std::string_view source,
    const std::string& relativePath,
    const std::size_t fileNodeId,
    std::vector<ScopeInfo>& scopes,
    std::vector<PendingCall>& pendingCalls,
    AnalysisGraphBuilder& builder);

void analyzeChildren(
    const TSNode node,
    const std::string_view source,
    const std::string& relativePath,
    const std::size_t fileNodeId,
    std::vector<ScopeInfo>& scopes,
    std::vector<PendingCall>& pendingCalls,
    AnalysisGraphBuilder& builder) {
    if (ts_node_is_null(node)) {
        return;
    }

    const std::uint32_t childCount = ts_node_named_child_count(node);
    for (std::uint32_t index = 0; index < childCount; ++index) {
        analyzeNode(ts_node_named_child(node, index), source, relativePath, fileNodeId, scopes, pendingCalls, builder);
    }
}

void analyzeNode(
    const TSNode node,
    const std::string_view source,
    const std::string& relativePath,
    const std::size_t fileNodeId,
    std::vector<ScopeInfo>& scopes,
    std::vector<PendingCall>& pendingCalls,
    AnalysisGraphBuilder& builder) {
    if (ts_node_is_null(node)) {
        return;
    }

    const std::string type = ts_node_type(node);
    const std::size_t parentNodeId = scopes.empty() ? fileNodeId : scopes.back().nodeId;

    if (type == "namespace_definition") {
        const std::string rawName = extractEntityName(fieldNode(node, "name"), source);
        if (rawName.empty()) {
            analyzeChildren(node, source, relativePath, fileNodeId, scopes, pendingCalls, builder);
            return;
        }

        const std::string qualifiedName = qualifyName(rawName, namespacePrefix(scopes), std::nullopt);
        const std::size_t nodeId = builder.addOrMergeNode(
            SymbolKind::Namespace,
            basenameFromQualified(rawName),
            qualifiedName,
            relativePath,
            ts_node_start_point(node).row);
        builder.addEdge(parentNodeId, nodeId, EdgeKind::Contains);

        scopes.push_back(ScopeInfo{nodeId, SymbolKind::Namespace, basenameFromQualified(rawName), qualifiedName});
        const TSNode body = fieldNode(node, "body");
        analyzeChildren(body, source, relativePath, fileNodeId, scopes, pendingCalls, builder);
        scopes.pop_back();
        return;
    }

    if (type == "class_specifier" || type == "struct_specifier") {
        const std::string rawName = extractEntityName(fieldNode(node, "name"), source);
        if (rawName.empty()) {
            analyzeChildren(node, source, relativePath, fileNodeId, scopes, pendingCalls, builder);
            return;
        }

        const SymbolKind kind = type == "struct_specifier" ? SymbolKind::Struct : SymbolKind::Class;
        const std::string qualifiedName = qualifyName(rawName, namespacePrefix(scopes), currentTypeScope(scopes));
        const std::size_t nodeId = builder.addOrMergeNode(
            kind,
            basenameFromQualified(rawName),
            qualifiedName,
            relativePath,
            ts_node_start_point(node).row);
        builder.addEdge(parentNodeId, nodeId, EdgeKind::Contains);

        scopes.push_back(ScopeInfo{nodeId, kind, basenameFromQualified(rawName), qualifiedName});
        const TSNode body = fieldNode(node, "body");
        analyzeChildren(body, source, relativePath, fileNodeId, scopes, pendingCalls, builder);
        scopes.pop_back();
        return;
    }

    if (type == "function_definition") {
        const std::string rawName = extractEntityName(fieldNode(node, "declarator"), source);
        if (rawName.empty()) {
            analyzeChildren(node, source, relativePath, fileNodeId, scopes, pendingCalls, builder);
            return;
        }

        const std::string currentNamespace = namespacePrefix(scopes);
        const auto typeScope = currentTypeScope(scopes);
        const bool isMethod = typeScope.has_value() || rawName.find("::") != std::string::npos;
        const std::string qualifiedName = qualifyName(rawName, currentNamespace, typeScope);
        const std::size_t nodeId = builder.addOrMergeNode(
            isMethod ? SymbolKind::Method : SymbolKind::Function,
            basenameFromQualified(rawName),
            qualifiedName,
            relativePath,
            ts_node_start_point(node).row);
        builder.addEdge(parentNodeId, nodeId, EdgeKind::Contains);

        const TSNode body = fieldNode(node, "body");
        collectCalls(body, source, relativePath, nodeId, qualifiedName, currentNamespace, typeScope, pendingCalls);
        return;
    }

    analyzeChildren(node, source, relativePath, fileNodeId, scopes, pendingCalls, builder);
}

std::optional<std::size_t> resolveCall(const PendingCall& call, const AnalysisGraphBuilder& builder) {
    std::string calleeName = call.calleeName;
    if (calleeName.starts_with("::")) {
        calleeName = calleeName.substr(2);
    }

    if (const auto exactMatch = builder.findSymbolByQualifiedName(calleeName); exactMatch.has_value()) {
        return exactMatch;
    }

    if (!call.typePrefix.empty()) {
        if (const auto exactMatch = builder.findSymbolByQualifiedName(call.typePrefix + "::" + calleeName); exactMatch.has_value()) {
            return exactMatch;
        }
    }

    if (!call.namespacePrefix.empty()) {
        if (const auto exactMatch = builder.findSymbolByQualifiedName(call.namespacePrefix + "::" + calleeName); exactMatch.has_value()) {
            return exactMatch;
        }
    }

    const std::string simpleName = basenameFromQualified(calleeName);
    const std::vector<std::size_t> candidates = builder.findSymbolsByDisplayName(simpleName);
    if (candidates.empty()) {
        return std::nullopt;
    }

    std::vector<std::size_t> namespaceScopedMatches;
    if (!call.namespacePrefix.empty()) {
        for (const std::size_t candidateId : candidates) {
            const savt::core::SymbolNode* node = builder.nodeById(candidateId);
            if (node != nullptr && node->qualifiedName.starts_with(call.namespacePrefix + "::")) {
                namespaceScopedMatches.push_back(candidateId);
            }
        }
    }

    if (namespaceScopedMatches.size() == 1) {
        return namespaceScopedMatches.front();
    }

    if (candidates.size() == 1) {
        return candidates.front();
    }

    return std::nullopt;
}

void resolvePendingCalls(
    const std::vector<PendingCall>& pendingCalls,
    AnalysisGraphBuilder& builder) {
    for (const PendingCall& call : pendingCalls) {
        const auto resolved = resolveCall(call, builder);
        if (resolved.has_value()) {
            builder.addEdge(call.callerId, *resolved, EdgeKind::Calls);
        } else {
            std::ostringstream diagnostic;
            diagnostic << call.filePath << ":" << (call.line + 1)
                       << " unresolved call target: " << call.calleeName
                       << " from " << call.callerQualifiedName;
            builder.report().diagnostics.push_back(diagnostic.str());
        }
    }
}


// ============================================================
// 多语言启发式分析（Python / Java / JS / TS）
// ============================================================

struct HeuristicImport {
    std::string moduleName;
    std::string rawSpecifier;
    bool relative = false;
    std::uint32_t line = 0;
};

struct HeuristicSymbol {
    std::string name;
    savt::core::SymbolKind kind = savt::core::SymbolKind::Class;
    std::uint32_t line = 0;
};

// 从一行文本里提取 Python import
std::optional<HeuristicImport> extractPythonImport(const std::string_view line, const std::uint32_t lineNo) {
    // "import X" 或 "import X.Y.Z"
    if (line.starts_with("import ")) {
        std::string mod(line.substr(7));
        // 取第一个空格/逗号之前的部分
        const auto end = mod.find_first_of(" ,\r\n");
        if (end != std::string::npos) mod = mod.substr(0, end);
        // 取顶层包名 (X.Y → X)
        const auto dot = mod.find('.');
        if (dot != std::string::npos) mod = mod.substr(0, dot);
        if (!mod.empty()) return HeuristicImport{mod, mod, false, lineNo};
    }
    // "from X import Y"
    if (line.starts_with("from ")) {
        const auto importPos = line.find(" import ");
        if (importPos != std::string_view::npos) {
            std::string mod(line.substr(5, importPos - 5));
            // 去掉相对导入的点
            const auto firstNonDot = mod.find_first_not_of('.');
            if (firstNonDot != std::string::npos) mod = mod.substr(firstNonDot);
            const auto dot = mod.find('.');
            if (dot != std::string::npos) mod = mod.substr(0, dot);
            if (!mod.empty()) return HeuristicImport{mod, mod, false, lineNo};
        }
    }
    return std::nullopt;
}

// 从一行文本里提取 Java import
std::optional<HeuristicImport> extractJavaImport(const std::string_view line, const std::uint32_t lineNo) {
    if (line.starts_with("import ")) {
        std::string mod(line.substr(7));
        if (mod.starts_with("static ")) {
            mod = mod.substr(7);
        }
        const auto semi = mod.find(';');
        if (semi != std::string::npos) mod = mod.substr(0, semi);
        while (!mod.empty() && std::isspace(static_cast<unsigned char>(mod.front())) != 0) {
            mod.erase(mod.begin());
        }
        while (!mod.empty() && std::isspace(static_cast<unsigned char>(mod.back())) != 0) {
            mod.pop_back();
        }
        if (!mod.empty()) {
            const std::size_t dot = mod.rfind('.');
            const std::string importedName = dot == std::string::npos ? mod : mod.substr(dot + 1);
            return HeuristicImport{importedName, mod, false, lineNo};
        }
    }
    return std::nullopt;
}

bool isRelativeScriptSpecifier(const std::string_view specifier) {
    return specifier.starts_with("./") || specifier.starts_with("../");
}

std::string packageRootFromSpecifier(std::string specifier) {
    if (specifier.empty()) {
        return {};
    }

    if (specifier.front() == '@') {
        const std::size_t firstSlash = specifier.find('/');
        if (firstSlash == std::string::npos) {
            return specifier;
        }
        const std::size_t secondSlash = specifier.find('/', firstSlash + 1);
        return secondSlash == std::string::npos ? specifier : specifier.substr(0, secondSlash);
    }

    const std::size_t slash = specifier.find('/');
    return slash == std::string::npos ? specifier : specifier.substr(0, slash);
}

bool pathEndsWithSuffix(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::vector<std::size_t> resolveJavaImportTargets(
    std::string specifier,
    const AnalysisGraphBuilder& builder) {
    std::vector<std::size_t> matches;
    if (specifier.empty()) {
        return matches;
    }

    const bool wildcardImport = pathEndsWithSuffix(specifier, ".*");
    if (wildcardImport) {
        specifier.resize(specifier.size() - 2);
    }
    if (specifier.empty()) {
        return matches;
    }

    auto appendMatchesForSuffix = [&](const std::string& suffix) {
        for (const FileRecord& candidate : builder.files()) {
            const std::string relativePath = normalizePath(candidate.relativePath);
            if (pathEndsWithSuffix(relativePath, suffix) &&
                std::find(matches.begin(), matches.end(), candidate.nodeId) == matches.end()) {
                matches.push_back(candidate.nodeId);
            }
        }
    };

    std::replace(specifier.begin(), specifier.end(), '.', '/');

    if (wildcardImport) {
        const std::string packageSuffix = "/" + specifier;
        for (const FileRecord& candidate : builder.files()) {
            const std::string parentPath = normalizePath(candidate.absolutePath.parent_path());
            if (pathEndsWithSuffix(parentPath, packageSuffix) &&
                std::find(matches.begin(), matches.end(), candidate.nodeId) == matches.end()) {
                matches.push_back(candidate.nodeId);
            }
        }
        return matches;
    }

    std::string fileSuffix = "/" + specifier + ".java";
    appendMatchesForSuffix(fileSuffix);
    if (!matches.empty()) {
        return matches;
    }

    const std::size_t lastSeparator = specifier.rfind('/');
    if (lastSeparator != std::string::npos) {
        specifier.resize(lastSeparator);
        fileSuffix = "/" + specifier + ".java";
        appendMatchesForSuffix(fileSuffix);
    }

    return matches;
}

std::optional<std::size_t> resolveRelativeScriptImport(
    const FileRecord& file,
    const std::string& specifier,
    const AnalysisGraphBuilder& builder) {
    if (!isRelativeScriptSpecifier(specifier)) {
        return std::nullopt;
    }

    const std::filesystem::path baseCandidate =
        (file.absolutePath.parent_path() / std::filesystem::path(specifier)).lexically_normal();

    const std::array<const char*, 8> extensions = {
        "", ".js", ".mjs", ".cjs", ".ts", ".tsx", ".mts", ".cts"
    };
    for (const char* extension : extensions) {
        const std::filesystem::path candidate =
            std::strlen(extension) == 0
                ? baseCandidate
                : std::filesystem::path(baseCandidate.string() + extension);
        if (const auto fileId = builder.findFileIdByPath(candidate); fileId.has_value()) {
            return fileId;
        }
    }

    const std::array<const char*, 7> indexExtensions = {
        "index.js", "index.mjs", "index.cjs", "index.ts", "index.tsx", "index.mts", "index.cts"
    };
    for (const char* fileName : indexExtensions) {
        if (const auto fileId = builder.findFileIdByPath(baseCandidate / fileName); fileId.has_value()) {
            return fileId;
        }
    }

    return std::nullopt;
}

// 从一行文本里提取 JS/TS import
std::optional<HeuristicImport> extractJsImport(const std::string_view line, const std::uint32_t lineNo) {
    // import ... from 'X' 或 import ... from "X"
    const auto fromPos = line.rfind("from ");
    if (fromPos != std::string_view::npos) {
        std::string_view rest = line.substr(fromPos + 5);
        while (!rest.empty() && (rest.front() == '\'' || rest.front() == '"' || rest.front() == ' '))
            rest.remove_prefix(1);
        const auto endQ = rest.find_first_of("'\"");
        std::string mod(endQ != std::string_view::npos ? rest.substr(0, endQ) : rest);
        if (!mod.empty()) {
            return HeuristicImport{
                packageRootFromSpecifier(mod),
                mod,
                isRelativeScriptSpecifier(mod),
                lineNo
            };
        }
    }
    // require('X')
    const auto reqPos = line.find("require(");
    if (reqPos != std::string_view::npos) {
        std::string_view rest = line.substr(reqPos + 8);
        while (!rest.empty() && (rest.front() == '\'' || rest.front() == '"'))
            rest.remove_prefix(1);
        const auto endQ = rest.find_first_of("'\")");
        std::string mod(endQ != std::string_view::npos ? rest.substr(0, endQ) : rest);
        if (!mod.empty()) {
            return HeuristicImport{
                packageRootFromSpecifier(mod),
                mod,
                isRelativeScriptSpecifier(mod),
                lineNo
            };
        }
    }
    return std::nullopt;
}

std::optional<HeuristicSymbol> extractJsAssignedFunction(
    const std::string_view line,
    const std::uint32_t lineNo) {
    if (line.find("=>") == std::string_view::npos) {
        return std::nullopt;
    }

    static constexpr std::array<std::string_view, 4> prefixes = {
        "const ", "let ", "var ", "export const "
    };
    for (const std::string_view prefix : prefixes) {
        if (!line.starts_with(prefix)) {
            continue;
        }

        std::string name(line.substr(prefix.size()));
        const std::size_t eq = name.find('=');
        if (eq == std::string::npos) {
            return std::nullopt;
        }

        name = std::string(std::string_view(name).substr(0, eq));
        while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back())) != 0) {
            name.pop_back();
        }
        if (!name.empty()) {
            return HeuristicSymbol{name, savt::core::SymbolKind::Function, lineNo};
        }
    }

    return std::nullopt;
}

// 从一行文本里提取类/函数定义
std::optional<HeuristicSymbol> extractHeuristicSymbol(
    const std::string_view line,
    const SourceLanguage lang,
    const std::uint32_t lineNo) {

    // Python: "class Foo:" 或 "def foo("
    if (lang == SourceLanguage::Python) {
        if (line.starts_with("class ")) {
            std::string name(line.substr(6));
            const auto end = name.find_first_of("(:. \t\r\n");
            if (end != std::string::npos) name = name.substr(0, end);
            if (!name.empty()) return HeuristicSymbol{name, savt::core::SymbolKind::Class, lineNo};
        }
        if (line.starts_with("def ")) {
            std::string name(line.substr(4));
            const auto end = name.find('(');
            if (end != std::string::npos) name = name.substr(0, end);
            if (!name.empty()) return HeuristicSymbol{name, savt::core::SymbolKind::Function, lineNo};
        }
    }

    // Java: "public/private/protected class Foo" 或 "public Foo("
    if (lang == SourceLanguage::Java) {
        auto trimmed = line;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front())))
            trimmed.remove_prefix(1);
        const auto classPos = trimmed.find("class ");
        if (classPos != std::string_view::npos) {
            std::string name(trimmed.substr(classPos + 6));
            const auto end = name.find_first_of(" {<\r\n");
            if (end != std::string::npos) name = name.substr(0, end);
            if (!name.empty()) return HeuristicSymbol{name, savt::core::SymbolKind::Class, lineNo};
        }
    }

    // JS/TS: "class Foo" 或 "function foo(" 或 "export function foo("
    if (lang == SourceLanguage::JavaScript || lang == SourceLanguage::TypeScript) {
        const auto classPos = line.find("class ");
        if (classPos != std::string_view::npos) {
            std::string name(line.substr(classPos + 6));
            const auto end = name.find_first_of(" {<\r\n");
            if (end != std::string::npos) name = name.substr(0, end);
            if (!name.empty()) return HeuristicSymbol{name, savt::core::SymbolKind::Class, lineNo};
        }
        const auto funcPos = line.find("function ");
        if (funcPos != std::string_view::npos) {
            std::string name(line.substr(funcPos + 9));
            const auto end = name.find('(');
            if (end != std::string::npos) name = name.substr(0, end);
            if (!name.empty() && name.front() != '(')
                return HeuristicSymbol{name, savt::core::SymbolKind::Function, lineNo};
        }
        if (const auto assigned = extractJsAssignedFunction(line, lineNo); assigned.has_value()) {
            return assigned;
        }
    }

    return std::nullopt;
}

void analyzeHeuristicFile(
    const FileRecord& file,
    const SourceLanguage lang,
    AnalysisGraphBuilder& builder) {

    const auto source = readAllText(file.absolutePath);
    if (!source.has_value()) {
        builder.report().diagnostics.push_back("Unable to read: " + file.relativePath);
        return;
    }

    // 逐行扫描
    std::uint32_t lineNo = 0;
    std::size_t offset = 0;
    while (offset <= source->size()) {
        const std::size_t nextNewline = source->find('\n', offset);
        const std::size_t length = nextNewline == std::string::npos
                                       ? source->size() - offset
                                       : nextNewline - offset;
        std::string_view line = std::string_view(*source).substr(offset, length);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        // 去掉行首空格（用于符号检测）
        std::string_view trimmed = line;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front())))
            trimmed.remove_prefix(1);

        // 提取 import
        std::optional<HeuristicImport> imp;
        switch (lang) {
        case SourceLanguage::Python:     imp = extractPythonImport(trimmed, lineNo); break;
        case SourceLanguage::Java:       imp = extractJavaImport(trimmed, lineNo);   break;
        case SourceLanguage::JavaScript:
        case SourceLanguage::TypeScript: imp = extractJsImport(trimmed, lineNo);     break;
        default: break;
        }

        if (imp.has_value()) {
            bool resolvedImport = false;

            if (lang == SourceLanguage::Java) {
                const auto javaTargets = resolveJavaImportTargets(imp->rawSpecifier, builder);
                for (const std::size_t targetId : javaTargets) {
                    builder.addEdge(file.nodeId, targetId, savt::core::EdgeKind::Includes);
                    resolvedImport = true;
                }
            }

            if (!resolvedImport &&
                (lang == SourceLanguage::JavaScript || lang == SourceLanguage::TypeScript) &&
                imp->relative) {
                if (const auto targetId = resolveRelativeScriptImport(file, imp->rawSpecifier, builder);
                    targetId.has_value()) {
                    builder.addEdge(file.nodeId, *targetId, savt::core::EdgeKind::Includes);
                    resolvedImport = true;
                }
            }

            if (!resolvedImport) {
                const auto targetIds = builder.findSymbolsByDisplayName(imp->moduleName);
                for (const std::size_t targetId : targetIds) {
                    builder.addEdge(file.nodeId, targetId, savt::core::EdgeKind::Includes);
                    resolvedImport = true;
                }
            }

            if (!resolvedImport && imp->relative) {
                std::ostringstream diagnostic;
                diagnostic << file.relativePath << ":" << (imp->line + 1)
                           << " unresolved local import: " << imp->rawSpecifier;
                builder.report().diagnostics.push_back(diagnostic.str());
            }
        }

        // 提取类/函数符号
        const auto sym = extractHeuristicSymbol(trimmed, lang, lineNo);
        if (sym.has_value()) {
            const std::size_t symId = builder.addOrMergeNode(
                sym->kind,
                sym->name,
                sym->name,
                file.relativePath,
                sym->line);
            builder.addEdge(file.nodeId, symId, savt::core::EdgeKind::Contains);
        }

        if (nextNewline == std::string::npos) break;
        offset = nextNewline + 1;
        ++lineNo;
    }

    ++builder.report().parsedFiles;
}

}  // namespace

savt::core::AnalysisReport analyzeSyntaxProject(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options) {
    const std::filesystem::path normalizedRoot = rootPath.lexically_normal();
    AnalysisGraphBuilder builder(normalizedRoot, options);
    builder.report().primaryEngine = "tree-sitter";
    builder.report().precisionLevel = "syntax";
    builder.report().semanticAnalysisRequested = options.precision != AnalyzerPrecision::SyntaxOnly;
    builder.report().semanticAnalysisEnabled = false;

    const std::vector<std::filesystem::path> sourceFiles = collectSourceFiles(normalizedRoot, options);
    builder.report().discoveredFiles = sourceFiles.size();
    builder.registerProjectFiles(sourceFiles);
    builder.registerModules();

    if (isCancellationRequested(options)) {
        return builder.report();
    }

    TSParser* parser = ts_parser_new();
    if (parser == nullptr) {
        builder.report().diagnostics.push_back("Failed to create tree-sitter parser.");
        return builder.report();
    }

    if (!ts_parser_set_language(parser, tree_sitter_cpp())) {
        ts_parser_delete(parser);
        builder.report().diagnostics.push_back("Failed to bind tree-sitter C++ grammar.");
        return builder.report();
    }

    std::vector<PendingCall> pendingCalls;
    for (const FileRecord& file : builder.files()) {
        if (isCancellationRequested(options)) {
            break;
        }

        if (!hasSourceExtension(file.absolutePath)) {
            continue;
        }

        const auto source = readAllText(file.absolutePath);
        if (!source.has_value()) {
            builder.report().diagnostics.push_back("Unable to read source file: " + file.relativePath);
            continue;
        }

        analyzeIncludes(*source, file, builder);

        TSTree* tree = ts_parser_parse_string(
            parser,
            nullptr,
            source->data(),
            static_cast<std::uint32_t>(source->size()));

        if (tree == nullptr) {
            builder.report().diagnostics.push_back("Parser returned null tree for: " + file.relativePath);
            continue;
        }

        ++builder.report().parsedFiles;
        std::vector<ScopeInfo> scopes;
        analyzeNode(ts_tree_root_node(tree), *source, file.relativePath, file.nodeId, scopes, pendingCalls, builder);
        ts_tree_delete(tree);
    }

    ts_parser_delete(parser);

    // ★ 多语言启发式分析通道
    for (const FileRecord& file : builder.files()) {
        if (isCancellationRequested(options)) break;
        if (!isHeuristicAnalyzableFile(file.absolutePath)) continue;
        const SourceLanguage lang = detectLanguage(file.absolutePath);
        analyzeHeuristicFile(file, lang, builder);
    }

    if (isCancellationRequested(options)) {
        return builder.report();
    }

    resolvePendingCalls(pendingCalls, builder);
    builder.aggregateModuleDependencies({EdgeKind::Includes, EdgeKind::Calls, EdgeKind::Inherits, EdgeKind::UsesType, EdgeKind::Overrides});
    return builder.report();
}

}  // namespace savt::analyzer::detail




