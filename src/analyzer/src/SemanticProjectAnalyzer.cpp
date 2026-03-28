#include "SemanticProjectAnalyzer.h"

#include "AnalysisGraphBuilder.h"
#include "AnalyzerUtilities.h"

#include <array>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef SAVT_ENABLE_CLANG_TOOLING
#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/CXDiagnostic.h>
#include <clang-c/CXFile.h>
#include <clang-c/CXSourceLocation.h>
#include <clang-c/Index.h>
#endif

namespace savt::analyzer::detail {
namespace {

using savt::core::EdgeKind;
using savt::core::SymbolKind;

struct ProjectLocation {
    std::filesystem::path absolutePath;
    std::string relativePath;
    std::uint32_t line = 0;
};

#ifdef SAVT_ENABLE_CLANG_TOOLING

std::string lowerAsciiCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string cxStringToStdString(const CXString value) {
    const char* const raw = clang_getCString(value);
    std::string result = raw == nullptr ? std::string() : std::string(raw);
    clang_disposeString(value);
    return result;
}

bool isNullCursor(const CXCursor cursor) {
    return clang_equalCursors(cursor, clang_getNullCursor()) != 0;
}

bool isValidType(const CXType type) {
    return type.kind != CXType_Invalid;
}

const char* compilationDatabaseErrorToString(const CXCompilationDatabase_Error error) {
    switch (error) {
    case CXCompilationDatabase_NoError:
        return "no_error";
    case CXCompilationDatabase_CanNotLoadDatabase:
        return "cannot_load_database";
    }

    return "unknown";
}

std::string pathFromCxFile(const CXFile file) {
    if (file == nullptr) {
        return {};
    }

    std::string result = cxStringToStdString(clang_File_tryGetRealPathName(file));
    if (result.empty()) {
        result = cxStringToStdString(clang_getFileName(file));
    }
    if (result.empty()) {
        return {};
    }

    return normalizePath(std::filesystem::path(result));
}

std::optional<ProjectLocation> locationFromSourceLocation(
    const CXSourceLocation location,
    const AnalysisGraphBuilder& builder) {
    CXFile file = nullptr;
    unsigned line = 0;
    unsigned column = 0;
    unsigned offset = 0;
    clang_getExpansionLocation(location, &file, &line, &column, &offset);
    if (file == nullptr) {
        return std::nullopt;
    }

    const std::string normalizedPath = pathFromCxFile(file);
    if (normalizedPath.empty()) {
        return std::nullopt;
    }

    const std::filesystem::path absolutePath(normalizedPath);
    if (!builder.isProjectFilePath(absolutePath)) {
        return std::nullopt;
    }

    return ProjectLocation{
        absolutePath.lexically_normal(),
        relativizePath(builder.rootPath(), absolutePath),
        line > 0 ? static_cast<std::uint32_t>(line - 1) : 0u
    };
}

std::optional<ProjectLocation> locationFromCursor(const CXCursor cursor, const AnalysisGraphBuilder& builder) {
    return locationFromSourceLocation(clang_getCursorLocation(cursor), builder);
}

std::string spellingForCursor(const CXCursor cursor) {
    std::string spelling = cxStringToStdString(clang_getCursorSpelling(cursor));
    if (!spelling.empty()) {
        return spelling;
    }

    spelling = cxStringToStdString(clang_getCursorDisplayName(cursor));
    if (!spelling.empty()) {
        return spelling;
    }

    if (clang_getCursorKind(cursor) == CXCursor_Namespace) {
        return "(anonymous)";
    }

    return {};
}

std::optional<SymbolKind> toSymbolKind(const CXCursor cursor) {
    switch (clang_getCursorKind(cursor)) {
    case CXCursor_Namespace:
        return SymbolKind::Namespace;
    case CXCursor_ClassDecl:
    case CXCursor_ClassTemplate:
    case CXCursor_ClassTemplatePartialSpecialization:
        return SymbolKind::Class;
    case CXCursor_StructDecl:
        return SymbolKind::Struct;
    case CXCursor_EnumDecl:
        return SymbolKind::Enum;
    case CXCursor_EnumConstantDecl:
        return SymbolKind::Enumerator;
    case CXCursor_FieldDecl:
        return SymbolKind::Field;
    case CXCursor_FunctionDecl:
    case CXCursor_FunctionTemplate:
        return SymbolKind::Function;
    case CXCursor_CXXMethod:
    case CXCursor_Constructor:
    case CXCursor_Destructor:
    case CXCursor_ConversionFunction:
        return SymbolKind::Method;
    default:
        return std::nullopt;
    }
}

bool isTypeKind(const SymbolKind kind) {
    return kind == SymbolKind::Class || kind == SymbolKind::Struct || kind == SymbolKind::Enum;
}

bool isCallableKind(const SymbolKind kind) {
    return kind == SymbolKind::Function || kind == SymbolKind::Method;
}

std::string typeVisitKey(const CXType type) {
    return std::to_string(static_cast<int>(type.kind)) + ":" + cxStringToStdString(clang_getTypeSpelling(type));
}

CXCursor dereferenceEntityCursor(CXCursor cursor) {
    if (isNullCursor(cursor)) {
        return cursor;
    }

    if (clang_isReference(clang_getCursorKind(cursor)) != 0) {
        const CXCursor referenced = clang_getCursorReferenced(cursor);
        if (!isNullCursor(referenced)) {
            cursor = referenced;
        }
    }

    return cursor;
}

CXCursor normalizeTemplateEntityCursor(CXCursor cursor) {
    cursor = dereferenceEntityCursor(cursor);
    if (isNullCursor(cursor)) {
        return cursor;
    }

    const auto symbolKind = toSymbolKind(cursor);
    if (!symbolKind.has_value() || !isTypeKind(*symbolKind)) {
        return cursor;
    }

    const CXCursor specializedTemplate = clang_getSpecializedCursorTemplate(cursor);
    if (isNullCursor(specializedTemplate)) {
        return cursor;
    }

    const auto specializedKind = toSymbolKind(specializedTemplate);
    if (!specializedKind.has_value() || !isTypeKind(*specializedKind)) {
        return cursor;
    }

    return dereferenceEntityCursor(specializedTemplate);
}

CXCursor normalizeDeclarationCursor(CXCursor cursor) {
    cursor = normalizeTemplateEntityCursor(cursor);
    if (isNullCursor(cursor)) {
        return cursor;
    }

    const CXCursor canonical = clang_getCanonicalCursor(cursor);
    if (!isNullCursor(canonical)) {
        return canonical;
    }

    return cursor;
}

std::string qualifiedNameFromCursor(CXCursor cursor) {
    std::vector<std::string> parts;
    cursor = normalizeDeclarationCursor(cursor);
    while (!isNullCursor(cursor)) {
        const auto symbolKind = toSymbolKind(cursor);
        if (symbolKind.has_value()) {
            const std::string part = spellingForCursor(cursor);
            if (!part.empty()) {
                parts.push_back(part);
            }
        }

        const CXCursor parent = clang_getCursorSemanticParent(cursor);
        if (isNullCursor(parent) || clang_getCursorKind(parent) == CXCursor_TranslationUnit) {
            break;
        }
        cursor = normalizeDeclarationCursor(parent);
    }

    if (parts.empty()) {
        return {};
    }

    std::string qualifiedName;
    for (auto iterator = parts.rbegin(); iterator != parts.rend(); ++iterator) {
        if (!qualifiedName.empty()) {
            qualifiedName += "::";
        }
        qualifiedName += *iterator;
    }
    return qualifiedName;
}

std::string locationKey(const ProjectLocation& location) {
    return location.relativePath + ":" + std::to_string(location.line);
}

std::optional<ProjectLocation> preferredLocation(
    const std::vector<ProjectLocation>& preferred,
    const std::vector<ProjectLocation>& fallback) {
    auto lessLocation = [](const ProjectLocation& left, const ProjectLocation& right) {
        return std::tie(left.relativePath, left.line) < std::tie(right.relativePath, right.line);
    };

    if (!preferred.empty()) {
        return *std::min_element(preferred.begin(), preferred.end(), lessLocation);
    }
    if (!fallback.empty()) {
        return *std::min_element(fallback.begin(), fallback.end(), lessLocation);
    }
    return std::nullopt;
}

std::string identityFromCursor(const CXCursor cursor, const AnalysisGraphBuilder& builder) {
    const CXCursor identityCursor = normalizeDeclarationCursor(cursor);
    std::string identity = cxStringToStdString(clang_getCursorUSR(identityCursor));
    if (!identity.empty()) {
        return identity;
    }

    const auto location = locationFromCursor(identityCursor, builder);
    if (!location.has_value()) {
        return {};
    }

    std::ostringstream stream;
    stream << "declloc:"
           << static_cast<int>(clang_getCursorKind(identityCursor))
           << ":"
           << location->relativePath
           << ":"
           << location->line;

    const std::string qualifiedName = qualifiedNameFromCursor(identityCursor);
    if (!qualifiedName.empty()) {
        stream << ":" << qualifiedName;
    } else {
        const std::string displayName = spellingForCursor(identityCursor);
        if (!displayName.empty()) {
            stream << ":" << displayName;
        }
    }

    return stream.str();
}

struct SemanticSymbolRecord {
    SymbolKind kind = SymbolKind::File;
    std::string identityKey;
    std::string displayName;
    std::string qualifiedName;
    std::string parentIdentityKey;
    std::vector<ProjectLocation> declarationLocations;
    std::vector<ProjectLocation> definitionLocations;
    std::unordered_set<std::string> declarationLocationKeys;
    std::unordered_set<std::string> definitionLocationKeys;
    std::size_t nodeId = 0;
};

struct PendingSemanticEdge {
    std::string fromIdentity;
    std::string toIdentity;
    EdgeKind kind = EdgeKind::Contains;
    std::size_t supportCount = 0;
};

struct PendingSemanticEdgeKey {
    std::string fromIdentity;
    std::string toIdentity;
    EdgeKind kind = EdgeKind::Contains;

    bool operator<(const PendingSemanticEdgeKey& other) const {
        return std::tie(fromIdentity, toIdentity, kind) <
               std::tie(other.fromIdentity, other.toIdentity, other.kind);
    }
};

struct SemanticRegistry {
    AnalysisGraphBuilder* builder = nullptr;
    std::unordered_map<std::string, SemanticSymbolRecord> symbols;
    std::map<PendingSemanticEdgeKey, PendingSemanticEdge> pendingEdges;
};

void appendUniqueLocation(
    SemanticSymbolRecord& record,
    const ProjectLocation& location,
    const bool definition) {
    auto& locationKeys = definition ? record.definitionLocationKeys : record.declarationLocationKeys;
    auto& locations = definition ? record.definitionLocations : record.declarationLocations;
    if (locationKeys.emplace(locationKey(location)).second) {
        locations.push_back(location);
    }
}

std::optional<std::string> findRegisteredTypeIdentityForLocation(
    const std::string& qualifiedName,
    const ProjectLocation& location,
    const SemanticRegistry& registry) {
    if (qualifiedName.empty()) {
        return std::nullopt;
    }

    const std::string key = locationKey(location);
    for (const auto& [identity, record] : registry.symbols) {
        if (!isTypeKind(record.kind) || record.qualifiedName != qualifiedName) {
            continue;
        }

        if (record.declarationLocationKeys.contains(key) || record.definitionLocationKeys.contains(key)) {
            return identity;
        }
    }

    return std::nullopt;
}

std::optional<std::string> registerSymbolFromCursor(CXCursor cursor, SemanticRegistry& registry) {
    if (registry.builder == nullptr) {
        return std::nullopt;
    }

    cursor = dereferenceEntityCursor(cursor);
    if (isNullCursor(cursor)) {
        return std::nullopt;
    }

    const auto symbolKind = toSymbolKind(cursor);
    if (!symbolKind.has_value()) {
        return std::nullopt;
    }

    const auto location = locationFromCursor(cursor, *registry.builder);
    if (!location.has_value()) {
        return std::nullopt;
    }

    std::string displayName = spellingForCursor(cursor);
    std::string qualifiedName = qualifiedNameFromCursor(cursor);
    if (displayName.empty()) {
        displayName = qualifiedName;
    }
    if (qualifiedName.empty()) {
        qualifiedName = displayName;
    }

    std::string identityKey;
    if (isTypeKind(*symbolKind)) {
        if (const auto existingIdentity =
                findRegisteredTypeIdentityForLocation(qualifiedName, *location, registry);
            existingIdentity.has_value()) {
            identityKey = *existingIdentity;
        }
    }
    if (identityKey.empty()) {
        identityKey = identityFromCursor(cursor, *registry.builder);
    }
    if (identityKey.empty()) {
        return std::nullopt;
    }

    SemanticSymbolRecord& record = registry.symbols[identityKey];
    if (record.identityKey.empty()) {
        record.identityKey = identityKey;
        record.kind = *symbolKind;
    }

    if (record.displayName.empty() && !displayName.empty()) {
        record.displayName = std::move(displayName);
    }
    if (record.qualifiedName.empty() && !qualifiedName.empty()) {
        record.qualifiedName = std::move(qualifiedName);
    }

    appendUniqueLocation(record, *location, clang_isCursorDefinition(cursor) != 0);

    const CXCursor parent = clang_getCursorSemanticParent(cursor);
    if (!isNullCursor(parent) && clang_getCursorKind(parent) != CXCursor_TranslationUnit) {
        if (const auto parentIdentity = registerSymbolFromCursor(parent, registry); parentIdentity.has_value()) {
            if (record.parentIdentityKey.empty()) {
                record.parentIdentityKey = *parentIdentity;
            }
        }
    }

    return identityKey;
}

void appendPendingEdge(
    SemanticRegistry& registry,
    const std::string& fromIdentity,
    const std::string& toIdentity,
    const EdgeKind kind) {
    if (fromIdentity.empty() || toIdentity.empty() || fromIdentity == toIdentity) {
        return;
    }

    PendingSemanticEdgeKey key{fromIdentity, toIdentity, kind};
    PendingSemanticEdge& aggregated = registry.pendingEdges[key];
    if (aggregated.supportCount == 0) {
        aggregated.fromIdentity = fromIdentity;
        aggregated.toIdentity = toIdentity;
        aggregated.kind = kind;
    }
    ++aggregated.supportCount;
}

void maybeCollectDeclaredType(
    const CXType type,
    SemanticRegistry& registry,
    std::unordered_set<std::string>& dependencyIdentities) {
    if (!isValidType(type)) {
        return;
    }

    const CXCursor declaration = clang_getTypeDeclaration(type);
    if (isNullCursor(declaration)) {
        return;
    }

    const auto symbolKind = toSymbolKind(declaration);
    if (!symbolKind.has_value() || !isTypeKind(*symbolKind)) {
        return;
    }

    if (const auto identity = registerSymbolFromCursor(declaration, registry); identity.has_value()) {
        dependencyIdentities.emplace(*identity);
    }
}

void collectTypeDependencyNodes(
    const CXType type,
    SemanticRegistry& registry,
    std::unordered_set<std::string>& visitedTypes,
    std::unordered_set<std::string>& dependencyIdentities) {
    if (!isValidType(type)) {
        return;
    }

    const std::string key = typeVisitKey(type);
    if (!visitedTypes.emplace(key).second) {
        return;
    }

    maybeCollectDeclaredType(type, registry, dependencyIdentities);

    const CXType canonicalType = clang_getCanonicalType(type);
    if (isValidType(canonicalType)) {
        const std::string canonicalKey = typeVisitKey(canonicalType);
        if (canonicalKey != key) {
            collectTypeDependencyNodes(canonicalType, registry, visitedTypes, dependencyIdentities);
        }
    }

    const CXType pointeeType = clang_getPointeeType(type);
    if (isValidType(pointeeType)) {
        collectTypeDependencyNodes(pointeeType, registry, visitedTypes, dependencyIdentities);
    }

    const CXType elementType = clang_getElementType(type);
    if (isValidType(elementType)) {
        collectTypeDependencyNodes(elementType, registry, visitedTypes, dependencyIdentities);
    }

    const int templateArgumentCount = clang_Type_getNumTemplateArguments(type);
    if (templateArgumentCount >= 0) {
        for (int index = 0; index < templateArgumentCount; ++index) {
            const CXType templateArgumentType = clang_Type_getTemplateArgumentAsType(type, index);
            if (isValidType(templateArgumentType)) {
                collectTypeDependencyNodes(templateArgumentType, registry, visitedTypes, dependencyIdentities);
            }
        }
    }

    const int argumentTypeCount = clang_getNumArgTypes(type);
    if (argumentTypeCount >= 0) {
        for (int index = 0; index < argumentTypeCount; ++index) {
            const CXType argumentType = clang_getArgType(type, static_cast<unsigned>(index));
            if (isValidType(argumentType)) {
                collectTypeDependencyNodes(argumentType, registry, visitedTypes, dependencyIdentities);
            }
        }
    }
}

void addTypeUseEdgesFromCursor(const CXCursor cursor, const std::string& sourceIdentity, SemanticRegistry& registry) {
    const auto symbolKind = toSymbolKind(cursor);
    if (!symbolKind.has_value()) {
        return;
    }

    std::unordered_set<std::string> visitedTypes;
    std::unordered_set<std::string> dependencyIdentities;
    auto collect = [&](const CXType type) {
        collectTypeDependencyNodes(type, registry, visitedTypes, dependencyIdentities);
    };

    if (*symbolKind == SymbolKind::Field) {
        collect(clang_getCursorType(cursor));
    }

    if (isCallableKind(*symbolKind)) {
        const CXType callableType = clang_getCursorType(cursor);
        collect(clang_getResultType(callableType));

        const int argumentCount = clang_Cursor_getNumArguments(cursor);
        if (argumentCount >= 0) {
            for (int index = 0; index < argumentCount; ++index) {
                const CXCursor argument = clang_Cursor_getArgument(cursor, static_cast<unsigned>(index));
                if (!isNullCursor(argument)) {
                    collect(clang_getCursorType(argument));
                }
            }
        } else {
            const int argumentTypeCount = clang_getNumArgTypes(callableType);
            if (argumentTypeCount >= 0) {
                for (int index = 0; index < argumentTypeCount; ++index) {
                    collect(clang_getArgType(callableType, static_cast<unsigned>(index)));
                }
            }
        }
    }

    for (const std::string& dependencyIdentity : dependencyIdentities) {
        appendPendingEdge(registry, sourceIdentity, dependencyIdentity, EdgeKind::UsesType);
    }
}

void addOverrideEdgesFromCursor(const CXCursor cursor, const std::string& sourceIdentity, SemanticRegistry& registry) {
    if (clang_getCursorKind(cursor) != CXCursor_CXXMethod) {
        return;
    }

    CXCursor* overriddenCursors = nullptr;
    unsigned overriddenCount = 0;
    clang_getOverriddenCursors(cursor, &overriddenCursors, &overriddenCount);
    for (unsigned index = 0; index < overriddenCount; ++index) {
        if (const auto overriddenIdentity = registerSymbolFromCursor(overriddenCursors[index], registry);
            overriddenIdentity.has_value()) {
            appendPendingEdge(registry, sourceIdentity, *overriddenIdentity, EdgeKind::Overrides);
        }
    }
    if (overriddenCursors != nullptr) {
        clang_disposeOverriddenCursors(overriddenCursors);
    }
}

void addInheritanceEdgesFromCursor(const CXCursor cursor, const std::string& sourceIdentity, SemanticRegistry& registry) {
    const CXType recordType = clang_getCursorType(cursor);
    if (!isValidType(recordType)) {
        return;
    }

    struct BaseVisitorPayload {
        SemanticRegistry* registry = nullptr;
        const std::string* sourceIdentity = nullptr;
    };

    BaseVisitorPayload payload{&registry, &sourceIdentity};
    clang_visitCXXBaseClasses(
        recordType,
        [](const CXCursor baseCursor, const CXClientData clientData) {
            auto* payload = static_cast<BaseVisitorPayload*>(clientData);
            if (payload == nullptr || payload->registry == nullptr || payload->sourceIdentity == nullptr) {
                return CXVisit_Break;
            }

            CXCursor resolvedBaseCursor = dereferenceEntityCursor(clang_getCursorReferenced(baseCursor));
            if (isNullCursor(resolvedBaseCursor)) {
                resolvedBaseCursor = clang_getTypeDeclaration(clang_getCursorType(baseCursor));
            }

            if (const auto baseIdentity = registerSymbolFromCursor(resolvedBaseCursor, *payload->registry);
                baseIdentity.has_value()) {
                appendPendingEdge(*payload->registry, *payload->sourceIdentity, *baseIdentity, EdgeKind::Inherits);
            }

            return CXVisit_Continue;
        },
        &payload);
}

std::string extractQuotedToken(const std::string& text) {
    const std::size_t singleQuoteStart = text.find('\'');
    if (singleQuoteStart != std::string::npos) {
        const std::size_t singleQuoteEnd = text.find('\'', singleQuoteStart + 1);
        if (singleQuoteEnd != std::string::npos && singleQuoteEnd > singleQuoteStart + 1) {
            return text.substr(singleQuoteStart + 1, singleQuoteEnd - singleQuoteStart - 1);
        }
    }

    const std::size_t doubleQuoteStart = text.find('"');
    if (doubleQuoteStart != std::string::npos) {
        const std::size_t doubleQuoteEnd = text.find('"', doubleQuoteStart + 1);
        if (doubleQuoteEnd != std::string::npos && doubleQuoteEnd > doubleQuoteStart + 1) {
            return text.substr(doubleQuoteStart + 1, doubleQuoteEnd - doubleQuoteStart - 1);
        }
    }

    return {};
}

bool isLikelySystemHeaderName(const std::string& headerName) {
    const std::string lowered = lowerAsciiCopy(headerName);
    if (lowered.empty()) {
        return false;
    }

    if (lowered.starts_with("bits/") || lowered.starts_with("sys/") ||
        lowered.starts_with("ucrt/") || lowered.starts_with("c++/")) {
        return true;
    }

    static const std::unordered_set<std::string> knownHeaders = {
        "algorithm", "array", "atomic", "chrono", "deque", "exception", "filesystem", "functional",
        "future", "initializer_list", "iostream", "istream", "limits", "list", "map", "memory",
        "new", "optional", "ostream", "queue", "set", "span", "sstream", "stack", "stdexcept",
        "string", "string_view", "thread", "tuple", "type_traits", "unordered_map", "unordered_set",
        "utility", "variant", "vector",
        "cassert", "cctype", "cerrno", "cfloat", "cinttypes", "climits", "cmath", "cstddef", "cstdint",
        "cstdio", "cstdlib", "cstring",
        "assert.h", "ctype.h", "errno.h", "float.h", "inttypes.h", "limits.h", "math.h", "stddef.h",
        "stdint.h", "stdio.h", "stdlib.h", "string.h", "time.h", "wchar.h",
        "windows.h", "sal.h", "vcruntime.h", "corecrt.h"
    };

    return knownHeaders.contains(lowered);
}

bool diagnosticLooksLikeSystemHeaderFailure(const std::string& formattedDiagnostic) {
    const std::string lowered = lowerAsciiCopy(formattedDiagnostic);
    const bool fileNotFound =
        lowered.find("file not found") != std::string::npos ||
        lowered.find("no such file or directory") != std::string::npos ||
        lowered.find("cannot open include file") != std::string::npos;
    if (!fileNotFound) {
        return false;
    }

    return isLikelySystemHeaderName(extractQuotedToken(formattedDiagnostic));
}

bool argumentMatchesOption(const std::string& argument, const std::string_view option) {
    return argument == option ||
           (argument.size() > option.size() && argument.starts_with(option) && argument[option.size()] == '=');
}

bool argumentsContainOption(
    const std::vector<std::string>& arguments,
    const std::string_view option) {
    return std::any_of(arguments.begin(), arguments.end(), [&](const std::string& argument) {
        return argumentMatchesOption(argument, option);
    });
}

bool argumentsContainPathValue(
    const std::vector<std::string>& arguments,
    const std::string_view option,
    const std::filesystem::path& expectedPath) {
    const std::string normalizedExpected = normalizePath(expectedPath);
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string& argument = arguments[index];
        if (argument == option) {
            if (index + 1 < arguments.size() &&
                normalizePath(std::filesystem::path(arguments[index + 1])) == normalizedExpected) {
                return true;
            }
            continue;
        }

        const std::string prefix(option);
        if (argument.size() > prefix.size() && argument.starts_with(prefix) && argument[prefix.size()] == '=') {
            if (normalizePath(std::filesystem::path(argument.substr(prefix.size() + 1))) == normalizedExpected) {
                return true;
            }
        }
    }
    return false;
}

bool isCppLikeSourceFile(const std::filesystem::path& sourceFile) {
    const std::string extension = lowerAsciiCopy(sourceFile.extension().string());
    return extension == ".cc" || extension == ".cpp" || extension == ".cxx" || extension == ".mm" ||
           extension == ".hpp" || extension == ".hh" || extension == ".hxx";
}

bool commandDisablesSystemHeaderSearch(const std::vector<std::string>& arguments);

#if defined(__APPLE__)

std::optional<std::string> captureCommandOutput(const char* command) {
    std::array<char, 256> buffer{};
    std::string output;

    FILE* pipe = popen(command, "r");
    if (pipe == nullptr) {
        return std::nullopt;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

    if (pclose(pipe) != 0) {
        return std::nullopt;
    }

    while (!output.empty() && std::isspace(static_cast<unsigned char>(output.back())) != 0) {
        output.pop_back();
    }
    while (!output.empty() && std::isspace(static_cast<unsigned char>(output.front())) != 0) {
        output.erase(output.begin());
    }

    return output.empty() ? std::nullopt : std::optional<std::string>(std::move(output));
}

std::string shellQuoteForPosix(std::string_view value) {
    std::string quoted;
    quoted.reserve(value.size() + 2);
    quoted.push_back('\'');
    for (const char character : value) {
        if (character == '\'') {
            quoted += "'\\''";
        } else {
            quoted.push_back(character);
        }
    }
    quoted.push_back('\'');
    return quoted;
}

bool usesMacOsClangDriver(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        return false;
    }

    const std::string compiler = lowerAsciiCopy(std::filesystem::path(toUnixPath(arguments.front())).filename().string());
    return compiler == "clang" || compiler == "clang++" || compiler == "cc" || compiler == "c++";
}

std::optional<std::filesystem::path> macOsSdkRoot() {
    static const std::optional<std::filesystem::path> cached = []() -> std::optional<std::filesystem::path> {
        const auto output = captureCommandOutput("xcrun --show-sdk-path 2>/dev/null");
        if (!output.has_value()) {
            return std::nullopt;
        }

        std::error_code errorCode;
        const std::filesystem::path sdkPath = std::filesystem::path(*output).lexically_normal();
        if (!std::filesystem::exists(sdkPath, errorCode) || errorCode) {
            return std::nullopt;
        }

        return sdkPath;
    }();
    return cached;
}

std::optional<std::filesystem::path> compilerResourceDir(const std::string& compilerExecutable) {
    static std::unordered_map<std::string, std::optional<std::filesystem::path>> cache;

    const std::string cacheKey = compilerExecutable.empty() ? std::string("clang") : toUnixPath(compilerExecutable);
    if (const auto cached = cache.find(cacheKey); cached != cache.end()) {
        return cached->second;
    }

    const std::string executable = compilerExecutable.empty() ? std::string("clang") : compilerExecutable;
    const std::string command = shellQuoteForPosix(executable) + " -print-resource-dir 2>/dev/null";
    const auto output = captureCommandOutput(command.c_str());
    if (!output.has_value()) {
        cache.emplace(cacheKey, std::nullopt);
        return std::nullopt;
    }

    std::error_code errorCode;
    const std::filesystem::path resourceDir = std::filesystem::path(*output).lexically_normal();
    if (!std::filesystem::exists(resourceDir, errorCode) || errorCode) {
        cache.emplace(cacheKey, std::nullopt);
        return std::nullopt;
    }

    cache.emplace(cacheKey, resourceDir);
    return resourceDir;
}

struct MacOsSdkAdjustment {
    bool applied = false;
    bool addedSysroot = false;
    bool addedLibcxxHeaders = false;
    bool addedResourceDir = false;
};

MacOsSdkAdjustment applyMacOsSdkFallbackArguments(
    std::vector<std::string>& arguments,
    const std::filesystem::path& sourceFile) {
    MacOsSdkAdjustment adjustment;
    if (commandDisablesSystemHeaderSearch(arguments) || !usesMacOsClangDriver(arguments)) {
        return adjustment;
    }

    const auto sdkRoot = macOsSdkRoot();
    if (!sdkRoot.has_value()) {
        return adjustment;
    }

    if (!argumentsContainOption(arguments, "-isysroot") && !argumentsContainOption(arguments, "--sysroot")) {
        arguments.push_back("-isysroot");
        arguments.push_back(normalizePath(*sdkRoot));
        adjustment.applied = true;
        adjustment.addedSysroot = true;
    }

    const std::filesystem::path libcxxHeaders = *sdkRoot / "usr/include/c++/v1";
    if (isCppLikeSourceFile(sourceFile) && !argumentsContainPathValue(arguments, "-isystem", libcxxHeaders)) {
        std::error_code errorCode;
        if (std::filesystem::exists(libcxxHeaders, errorCode) && !errorCode) {
            arguments.push_back("-isystem");
            arguments.push_back(normalizePath(libcxxHeaders));
            adjustment.applied = true;
            adjustment.addedLibcxxHeaders = true;
        }
    }

    if (!argumentsContainOption(arguments, "-resource-dir")) {
        const auto resourceDir = compilerResourceDir(arguments.empty() ? std::string() : arguments.front());
        if (resourceDir.has_value()) {
            arguments.push_back("-resource-dir");
            arguments.push_back(normalizePath(*resourceDir));
            adjustment.applied = true;
            adjustment.addedResourceDir = true;
        }
    }

    return adjustment;
}

#endif

struct TranslationUnitDiagnosticSummary {
    bool hasSystemHeaderFailure = false;
    bool hasErrorOrFatal = false;
};

TranslationUnitDiagnosticSummary appendTranslationUnitDiagnostics(
    CXTranslationUnit translationUnit,
    savt::core::AnalysisReport& report) {
    TranslationUnitDiagnosticSummary summary;
    const unsigned diagnosticCount = clang_getNumDiagnostics(translationUnit);
    for (unsigned index = 0; index < diagnosticCount; ++index) {
        CXDiagnostic diagnostic = clang_getDiagnostic(translationUnit, index);
        const std::string formatted = cxStringToStdString(
            clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions()));
        report.diagnostics.push_back(formatted);

        const CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diagnostic);
        if (severity == CXDiagnostic_Error || severity == CXDiagnostic_Fatal) {
            summary.hasErrorOrFatal = true;
            summary.hasSystemHeaderFailure =
                summary.hasSystemHeaderFailure || diagnosticLooksLikeSystemHeaderFailure(formatted);
        }

        clang_disposeDiagnostic(diagnostic);
    }
    return summary;
}

struct InclusionVisitorContext {
    AnalysisGraphBuilder* builder = nullptr;
};

void inclusionVisitor(
    const CXFile includedFile,
    CXSourceLocation* inclusionStack,
    const unsigned includeLength,
    CXClientData clientData) {
    if (includeLength == 0 || clientData == nullptr) {
        return;
    }

    auto* context = static_cast<InclusionVisitorContext*>(clientData);
    if (context->builder == nullptr) {
        return;
    }

    const std::string includedPath = pathFromCxFile(includedFile);
    if (includedPath.empty()) {
        return;
    }

    const auto toFileId = context->builder->findFileIdByPath(std::filesystem::path(includedPath));
    if (!toFileId.has_value()) {
        return;
    }

    CXFile includingFile = nullptr;
    unsigned line = 0;
    unsigned column = 0;
    unsigned offset = 0;
    clang_getExpansionLocation(inclusionStack[0], &includingFile, &line, &column, &offset);
    const std::string includingPath = pathFromCxFile(includingFile);
    if (includingPath.empty()) {
        return;
    }

    const auto fromFileId = context->builder->findFileIdByPath(std::filesystem::path(includingPath));
    if (!fromFileId.has_value()) {
        return;
    }

    context->builder->addEdge(
        *fromFileId,
        *toFileId,
        EdgeKind::Includes,
        1,
        false,
        1,
        savt::core::FactSource::Semantic);
}

struct TraversalFrame {
    std::vector<std::string> typeStack;
    std::vector<std::string> callableStack;
};

void materializeSemanticRegistry(SemanticRegistry& registry) {
    if (registry.builder == nullptr) {
        return;
    }

    std::vector<std::string> orderedIdentities;
    orderedIdentities.reserve(registry.symbols.size());
    for (const auto& [identity, record] : registry.symbols) {
        if (preferredLocation(record.definitionLocations, record.declarationLocations).has_value()) {
            orderedIdentities.push_back(identity);
        }
    }

    std::sort(orderedIdentities.begin(), orderedIdentities.end(), [&](const std::string& left, const std::string& right) {
        const SemanticSymbolRecord& leftRecord = registry.symbols.at(left);
        const SemanticSymbolRecord& rightRecord = registry.symbols.at(right);
        const auto leftLocation = preferredLocation(leftRecord.definitionLocations, leftRecord.declarationLocations);
        const auto rightLocation = preferredLocation(rightRecord.definitionLocations, rightRecord.declarationLocations);
        const auto leftTuple = std::make_tuple(
            leftLocation.has_value() ? leftLocation->relativePath : std::string(),
            leftLocation.has_value() ? leftLocation->line : 0u,
            leftRecord.qualifiedName,
            leftRecord.displayName,
            static_cast<int>(leftRecord.kind),
            leftRecord.identityKey);
        const auto rightTuple = std::make_tuple(
            rightLocation.has_value() ? rightLocation->relativePath : std::string(),
            rightLocation.has_value() ? rightLocation->line : 0u,
            rightRecord.qualifiedName,
            rightRecord.displayName,
            static_cast<int>(rightRecord.kind),
            rightRecord.identityKey);
        return leftTuple < rightTuple;
    });

    for (const std::string& identity : orderedIdentities) {
        SemanticSymbolRecord& record = registry.symbols.at(identity);
        const auto primaryLocation = preferredLocation(record.definitionLocations, record.declarationLocations);
        if (!primaryLocation.has_value()) {
            continue;
        }

        std::string displayName = record.displayName.empty() ? record.qualifiedName : record.displayName;
        std::string qualifiedName = record.qualifiedName.empty() ? displayName : record.qualifiedName;
        if (displayName.empty() && qualifiedName.empty()) {
            continue;
        }

        record.nodeId = registry.builder->addOrMergeNode(
            record.kind,
            std::move(displayName),
            std::move(qualifiedName),
            primaryLocation->relativePath,
            primaryLocation->line,
            record.identityKey,
            savt::core::FactSource::Semantic);

        if (const auto fileId = registry.builder->findFileIdByRelativePath(primaryLocation->relativePath);
            fileId.has_value()) {
            registry.builder->bindNodeToFile(record.nodeId, *fileId);
        }
    }

    for (const std::string& identity : orderedIdentities) {
        const SemanticSymbolRecord& record = registry.symbols.at(identity);
        if (record.nodeId == 0) {
            continue;
        }

        if (!record.parentIdentityKey.empty()) {
            const auto parentIt = registry.symbols.find(record.parentIdentityKey);
            if (parentIt != registry.symbols.end() && parentIt->second.nodeId != 0) {
                registry.builder->addEdge(
                    parentIt->second.nodeId,
                    record.nodeId,
                    EdgeKind::Contains,
                    1,
                    false,
                    1,
                    savt::core::FactSource::Semantic);
                continue;
            }
        }

        const auto primaryLocation = preferredLocation(record.definitionLocations, record.declarationLocations);
        if (!primaryLocation.has_value()) {
            continue;
        }

        if (const auto fileId = registry.builder->findFileIdByRelativePath(primaryLocation->relativePath);
            fileId.has_value()) {
            registry.builder->addEdge(
                *fileId,
                record.nodeId,
                EdgeKind::Contains,
                1,
                false,
                1,
                savt::core::FactSource::Semantic);
        }
    }

    for (const auto& [edgeKey, edge] : registry.pendingEdges) {
        static_cast<void>(edgeKey);
        const auto fromIt = registry.symbols.find(edge.fromIdentity);
        const auto toIt = registry.symbols.find(edge.toIdentity);
        if (fromIt == registry.symbols.end() || toIt == registry.symbols.end()) {
            continue;
        }
        if (fromIt->second.nodeId == 0 || toIt->second.nodeId == 0) {
            continue;
        }

        registry.builder->addEdge(
            fromIt->second.nodeId,
            toIt->second.nodeId,
            edge.kind,
            1,
            false,
            edge.supportCount,
            savt::core::FactSource::Semantic);
    }
}

void traverseCursor(CXCursor cursor, SemanticRegistry& registry, TraversalFrame frame);

struct TraversalPayload {
    SemanticRegistry* registry = nullptr;
    TraversalFrame frame;
};

void traverseCursor(CXCursor cursor, SemanticRegistry& registry, TraversalFrame frame) {
    const CXCursorKind kind = clang_getCursorKind(cursor);
    if (kind != CXCursor_TranslationUnit) {
        const auto location = locationFromCursor(dereferenceEntityCursor(cursor), *registry.builder);
        if (!location.has_value() && clang_isDeclaration(kind) != 0) {
            return;
        }
    }

    if (const auto symbolKind = toSymbolKind(dereferenceEntityCursor(cursor)); symbolKind.has_value()) {
        if (const auto identity = registerSymbolFromCursor(cursor, registry); identity.has_value()) {
            if (*symbolKind == SymbolKind::Field || isCallableKind(*symbolKind)) {
                addTypeUseEdgesFromCursor(cursor, *identity, registry);
            }
            if (*symbolKind == SymbolKind::Method) {
                addOverrideEdgesFromCursor(cursor, *identity, registry);
            }
            if (isTypeKind(*symbolKind)) {
                addInheritanceEdgesFromCursor(cursor, *identity, registry);
                frame.typeStack.push_back(*identity);
            }
            if (isCallableKind(*symbolKind)) {
                frame.callableStack.push_back(*identity);
            }
        }
    }

    if (kind == CXCursor_CallExpr && !frame.callableStack.empty()) {
        const CXCursor referenced = clang_getCursorReferenced(cursor);
        if (const auto calleeIdentity = registerSymbolFromCursor(referenced, registry); calleeIdentity.has_value()) {
            appendPendingEdge(registry, frame.callableStack.back(), *calleeIdentity, EdgeKind::Calls);
        }
    }

    TraversalPayload payload{&registry, std::move(frame)};
    clang_visitChildren(
        cursor,
        [](const CXCursor child, const CXCursor parent, CXClientData clientData) {
            static_cast<void>(parent);
            auto* childPayload = static_cast<TraversalPayload*>(clientData);
            if (childPayload == nullptr || childPayload->registry == nullptr) {
                return CXChildVisit_Break;
            }

            traverseCursor(child, *childPayload->registry, childPayload->frame);
            return CXChildVisit_Continue;
        },
        &payload);
}

std::vector<std::string> collectCompileCommandArguments(const CXCompileCommand command) {
    std::vector<std::string> arguments;
    const unsigned argumentCount = clang_CompileCommand_getNumArgs(command);
    arguments.reserve(argumentCount);
    for (unsigned index = 0; index < argumentCount; ++index) {
        arguments.push_back(cxStringToStdString(clang_CompileCommand_getArg(command, index)));
    }
    return arguments;
}

void applyCompileCommandWorkingDirectory(
    const CXCompileCommand command,
    std::vector<std::string>& arguments) {
    const std::string commandDirectory = cxStringToStdString(clang_CompileCommand_getDirectory(command));
    if (commandDirectory.empty() || arguments.empty()) {
        return;
    }

    const bool alreadyConfigured = std::any_of(arguments.begin(), arguments.end(), [](const std::string& argument) {
        return argument == "-working-directory" || argument.starts_with("-working-directory=");
    });
    if (alreadyConfigured) {
        return;
    }

    arguments.insert(arguments.begin() + 1, "-working-directory=" + normalizePath(std::filesystem::path(commandDirectory)));
}

bool shouldDropCompileOnlyArgument(std::string_view argument) {
    return argument == "-c" ||
           argument == "-MD" ||
           argument == "-MMD" ||
           argument == "-MP" ||
           argument == "-MV" ||
           argument == "-Winvalid-command-line-argument";
}

bool argumentConsumesFollowingValue(std::string_view argument) {
    return argument == "-o" ||
           argument == "-MF" ||
           argument == "-MT" ||
           argument == "-MQ" ||
           argument == "-MJ" ||
           argument == "-fmodule-output" ||
           argument == "--serialize-diagnostics";
}

bool argumentEmbedsConsumedValue(std::string_view argument) {
    return (argument.size() > 2 && argument.starts_with("-o")) ||
           (argument.size() > 3 && argument.starts_with("-MF")) ||
           (argument.size() > 3 && argument.starts_with("-MT")) ||
           (argument.size() > 3 && argument.starts_with("-MQ")) ||
           (argument.size() > 3 && argument.starts_with("-MJ")) ||
           argument.starts_with("-fmodule-output=") ||
           argument.starts_with("--serialize-diagnostics=");
}

bool isLinkerArtifactArgument(const std::string& argument) {
    if (argument.empty() || argument.front() == '-') {
        return false;
    }

    const std::string extension = lowerAsciiCopy(std::filesystem::path(argument).extension().string());
    return extension == ".o" || extension == ".obj" || extension == ".a" || extension == ".lib";
}

void sanitizeCompileCommandArgumentsForSemanticParse(std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        return;
    }

    std::vector<std::string> sanitized;
    sanitized.reserve(arguments.size());
    sanitized.push_back(arguments.front());

    for (std::size_t index = 1; index < arguments.size(); ++index) {
        const std::string& argument = arguments[index];
        if (shouldDropCompileOnlyArgument(argument)) {
            continue;
        }

        if (argumentConsumesFollowingValue(argument)) {
            ++index;
            continue;
        }

        if (argumentEmbedsConsumedValue(argument)) {
            continue;
        }

        if (isLinkerArtifactArgument(argument)) {
            continue;
        }

        sanitized.push_back(argument);
    }

    arguments = std::move(sanitized);
}

bool commandDisablesSystemHeaderSearch(const std::vector<std::string>& arguments) {
    return std::find(arguments.begin(), arguments.end(), "-nostdinc") != arguments.end() ||
           std::find(arguments.begin(), arguments.end(), "-nostdinc++") != arguments.end();
}

bool sourceIncludesSystemHeader(const std::filesystem::path& sourceFile) {
    const auto sourceText = readAllText(sourceFile);
    return sourceText.has_value() && sourceText->find("#include <") != std::string::npos;
}

bool compileArgumentsContainSourceFile(
    const std::vector<std::string>& arguments,
    const std::filesystem::path& sourceFile) {
    const std::string normalizedSourcePath = normalizePath(sourceFile);
    const std::string fileName = sourceFile.filename().string();
    return std::any_of(arguments.begin(), arguments.end(), [&](const std::string& argument) {
        if (argument == fileName) {
            return true;
        }

        std::error_code errorCode;
        const std::filesystem::path candidatePath(argument);
        if (!candidatePath.empty()) {
            const std::filesystem::path weaklyNormalized = std::filesystem::weakly_canonical(candidatePath, errorCode);
            if (!errorCode && normalizePath(weaklyNormalized) == normalizedSourcePath) {
                return true;
            }
        }

        return normalizePath(std::filesystem::path(argument)) == normalizedSourcePath;
    });
}

#endif

}  // namespace

SemanticBackendResult analyzeSemanticProject(
    const std::filesystem::path& rootPath,
    const std::filesystem::path& compilationDatabasePath,
    const AnalyzerOptions& options) {
    SemanticBackendResult result;
    result.report.rootPath = normalizePath(rootPath);
    result.report.primaryEngine = "libclang-cindex";
    result.report.precisionLevel = "semantic";
    result.report.semanticAnalysisRequested = options.precision != AnalyzerPrecision::SyntaxOnly;
    result.report.semanticAnalysisEnabled = false;
    result.report.compilationDatabasePath = normalizePath(compilationDatabasePath);

#ifndef SAVT_ENABLE_CLANG_TOOLING
    result.report.diagnostics.push_back(
        "Semantic backend requested, but SAVT_ENABLE_CLANG_TOOLING is disabled at build time.");
    return result;
#else
    const std::filesystem::path normalizedRoot = rootPath.lexically_normal();
    AnalysisGraphBuilder builder(normalizedRoot, options);
    builder.report() = result.report;

    const std::vector<std::filesystem::path> sourceFiles = collectSourceFiles(normalizedRoot, options);
    builder.report().discoveredFiles = sourceFiles.size();
    builder.registerProjectFiles(sourceFiles);
    builder.registerModules();

    const std::filesystem::path buildDirectory = compilationDatabasePath.parent_path();
    CXCompilationDatabase_Error databaseError = CXCompilationDatabase_NoError;
    CXCompilationDatabase compilationDatabase = clang_CompilationDatabase_fromDirectory(
        buildDirectory.string().c_str(),
        &databaseError);
    if (compilationDatabase == nullptr || databaseError != CXCompilationDatabase_NoError) {
        std::ostringstream diagnostic;
        diagnostic << "Failed to load compilation database from " << normalizePath(buildDirectory)
                   << " (" << compilationDatabaseErrorToString(databaseError) << ")";
        builder.report().diagnostics.push_back(diagnostic.str());
        result.failureKind = SemanticFailureKind::CompilationDatabaseLoadFailed;
        result.failureMessage = "libclang could not load the compilation database.";
        result.report = builder.report();
        return result;
    }

    CXCompileCommands compileCommands = clang_CompilationDatabase_getAllCompileCommands(compilationDatabase);
    const unsigned commandCount = clang_CompileCommands_getSize(compileCommands);
    if (commandCount == 0) {
        builder.report().diagnostics.push_back("Compilation database is empty.");
        result.failureKind = SemanticFailureKind::CompilationDatabaseEmpty;
        result.failureMessage = "The compilation database was found but contains no compile commands.";
        clang_CompileCommands_dispose(compileCommands);
        clang_CompilationDatabase_dispose(compilationDatabase);
        result.report = builder.report();
        return result;
    }

    CXIndex index = clang_createIndex(0, 0);
    if (index == nullptr) {
        builder.report().diagnostics.push_back("Failed to create libclang index.");
        result.failureKind = SemanticFailureKind::LibclangIndexCreationFailed;
        result.failureMessage = "libclang failed to create a semantic analysis index.";
        clang_CompileCommands_dispose(compileCommands);
        clang_CompilationDatabase_dispose(compilationDatabase);
        result.report = builder.report();
        return result;
    }

    std::unordered_set<std::string> seenTranslationUnits;
    std::size_t eligibleTranslationUnits = 0;
    std::size_t systemHeaderFailureCount = 0;
    std::size_t parseFailureCount = 0;
    bool reportedMacOsSdkFallback = false;
    SemanticRegistry semanticRegistry{&builder};
    for (unsigned commandIndex = 0; commandIndex < commandCount; ++commandIndex) {
        const CXCompileCommand command = clang_CompileCommands_getCommand(compileCommands, commandIndex);
        const std::filesystem::path sourceFile = std::filesystem::path(
            cxStringToStdString(clang_CompileCommand_getFilename(command))).lexically_normal();
        if (sourceFile.empty() || !builder.isProjectFilePath(sourceFile) || !hasSourceExtension(sourceFile)) {
            continue;
        }

        const std::string normalizedSourcePath = normalizePath(sourceFile);
        if (!seenTranslationUnits.emplace(normalizedSourcePath).second) {
            continue;
        }
        ++eligibleTranslationUnits;

        std::vector<std::string> argumentStorage = collectCompileCommandArguments(command);
        if (argumentStorage.empty()) {
            builder.report().diagnostics.push_back("Compile command had no arguments: " + normalizedSourcePath);
            ++parseFailureCount;
            continue;
        }
        applyCompileCommandWorkingDirectory(command, argumentStorage);
        sanitizeCompileCommandArgumentsForSemanticParse(argumentStorage);
#if defined(__APPLE__)
        const MacOsSdkAdjustment macOsSdkAdjustment = applyMacOsSdkFallbackArguments(argumentStorage, sourceFile);
        if (macOsSdkAdjustment.applied && !reportedMacOsSdkFallback) {
            std::ostringstream diagnostic;
            diagnostic << "Applied macOS SDK fallback arguments for Apple Clang semantic analysis";
            if (macOsSdkAdjustment.addedSysroot) {
                diagnostic << " (-isysroot)";
            }
            if (macOsSdkAdjustment.addedLibcxxHeaders) {
                diagnostic << " (-isystem libc++ headers)";
            }
            if (macOsSdkAdjustment.addedResourceDir) {
                diagnostic << " (-resource-dir)";
            }
            diagnostic << ".";
            builder.report().diagnostics.push_back(diagnostic.str());
            reportedMacOsSdkFallback = true;
        }
#endif

        std::vector<const char*> argumentPointers;
        argumentPointers.reserve(argumentStorage.size());
        for (const std::string& argument : argumentStorage) {
            argumentPointers.push_back(argument.c_str());
        }

        CXTranslationUnit translationUnit = nullptr;
        const unsigned parseOptions = CXTranslationUnit_DetailedPreprocessingRecord | CXTranslationUnit_KeepGoing;
        const char* const translationUnitSource =
            compileArgumentsContainSourceFile(argumentStorage, sourceFile) ? nullptr : normalizedSourcePath.c_str();
        const CXErrorCode parseError = clang_parseTranslationUnit2FullArgv(
            index,
            translationUnitSource,
            argumentPointers.data(),
            static_cast<int>(argumentPointers.size()),
            nullptr,
            0,
            parseOptions,
            &translationUnit);
        if (parseError != CXError_Success || translationUnit == nullptr) {
            std::ostringstream diagnostic;
            diagnostic << "Failed to parse translation unit: " << normalizedSourcePath
                       << " (error=" << static_cast<int>(parseError) << ")";
            builder.report().diagnostics.push_back(diagnostic.str());
            if (commandDisablesSystemHeaderSearch(argumentStorage) && sourceIncludesSystemHeader(sourceFile)) {
                builder.report().diagnostics.push_back(
                    "Semantic analysis blocked by unresolved system headers for: " + normalizedSourcePath);
                ++systemHeaderFailureCount;
            } else {
                ++parseFailureCount;
            }
            continue;
        }

        const TranslationUnitDiagnosticSummary diagnosticSummary =
            appendTranslationUnitDiagnostics(translationUnit, builder.report());
        if (diagnosticSummary.hasSystemHeaderFailure) {
            builder.report().diagnostics.push_back(
                "Semantic analysis blocked by unresolved system headers for: " + normalizedSourcePath);
            ++systemHeaderFailureCount;
            clang_disposeTranslationUnit(translationUnit);
            continue;
        }

        ++builder.report().parsedFiles;

        InclusionVisitorContext inclusionContext{&builder};
        clang_getInclusions(translationUnit, inclusionVisitor, &inclusionContext);

        traverseCursor(clang_getTranslationUnitCursor(translationUnit), semanticRegistry, TraversalFrame{});
        clang_disposeTranslationUnit(translationUnit);
    }

    clang_disposeIndex(index);
    clang_CompileCommands_dispose(compileCommands);
    clang_CompilationDatabase_dispose(compilationDatabase);

    materializeSemanticRegistry(semanticRegistry);
    builder.aggregateModuleDependencies({
        EdgeKind::Includes,
        EdgeKind::Calls,
        EdgeKind::Inherits,
        EdgeKind::UsesType,
        EdgeKind::Overrides
    });
    builder.report().semanticAnalysisEnabled = builder.report().parsedFiles > 0;

    result.completed = builder.report().parsedFiles > 0;
    result.report = builder.report();
    if (!result.completed) {
        if (systemHeaderFailureCount > 0) {
            result.failureKind = SemanticFailureKind::SystemHeadersUnresolved;
            result.failureMessage =
                "compile_commands.json was found, but libclang could not resolve required system headers.";
        } else {
            result.failureKind = SemanticFailureKind::TranslationUnitParseFailed;
            result.failureMessage =
                eligibleTranslationUnits == 0
                    ? "No project C/C++ translation units were found in the compilation database."
                    : "The semantic backend could not parse any project translation units.";
        }

        if (parseFailureCount > 0 || systemHeaderFailureCount > 0) {
            std::ostringstream summary;
            summary << "Semantic backend parsed 0/" << eligibleTranslationUnits << " project translation units";
            if (systemHeaderFailureCount > 0) {
                summary << "; unresolved system-header failures: " << systemHeaderFailureCount;
            }
            if (parseFailureCount > 0) {
                summary << "; parse/startup failures: " << parseFailureCount;
            }
            summary << ".";
            result.report.diagnostics.push_back(summary.str());
        }
        result.report.diagnostics.push_back("Semantic backend did not successfully parse any translation units.");
    }
    return result;
#endif
}

}  // namespace savt::analyzer::detail
