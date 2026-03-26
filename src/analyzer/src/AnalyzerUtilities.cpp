#include "AnalyzerUtilities.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <unordered_set>

namespace savt::analyzer::detail {
namespace {

std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool endsWith(const std::string& value, const std::string_view suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix.data(), suffix.size()) == 0;
}

bool hasNormalizedExtension(
    const std::filesystem::path& filePath,
    const std::unordered_set<std::string>& extensions) {
    return extensions.contains(lowerCopy(filePath.extension().string()));
}

std::string normalizedFileName(const std::filesystem::path& filePath) {
    return lowerCopy(filePath.filename().string());
}

bool isProjectManifestFile(const std::string& fileName) {
    return fileName == "cmakelists.txt" || fileName == "package.json" ||
           fileName == "pom.xml" || fileName == "mvnw" ||
           fileName == "build.gradle" || fileName == "build.gradle.kts" ||
           fileName == "settings.gradle" || fileName == "settings.gradle.kts" ||
           fileName == "gradlew" ||
           endsWith(fileName, ".pro") || endsWith(fileName, ".pri");
}

bool isBootstrapFileName(const std::string& fileName) {
    return fileName == "main.cpp" || fileName == "main.cc" || fileName == "main.cxx" || fileName == "main.c" ||
           fileName == "main.js" || fileName == "main.mjs" || fileName == "main.cjs" ||
           fileName == "index.js" || fileName == "index.mjs" || fileName == "index.cjs" ||
           fileName == "app.js" || fileName == "app.mjs" || fileName == "app.cjs" ||
           fileName == "server.js" || fileName == "server.mjs" || fileName == "server.cjs" ||
           isProjectManifestFile(fileName) || endsWith(fileName, ".qrc") || endsWith(fileName, ".rc");
}

bool isLikelyDirectorySegment(const std::string_view segment) {
    return segment.find('.') == std::string_view::npos;
}

bool isGenericBridgeSegment(std::string segment) {
    segment = lowerCopy(std::move(segment));

    static const std::unordered_set<std::string> genericSegments = {
        "src", "source", "sources", "include", "includes", "inc", "impl", "implementation", "lib", "libs", "code"
    };

    return genericSegments.contains(segment);
}

bool isDependencyLikeSegment(const std::string_view segment) {
    const std::string lowered = lowerCopy(std::string(segment));
    static const std::unordered_set<std::string> dependencySegments = {
        "third_party", "thirdparty", "3rdparty", "external", "vendor", "vendors", "deps", "dependency", "dependencies"
    };

    if (dependencySegments.contains(lowered)) {
        return true;
    }

    return lowered.find("qxlsx") != std::string::npos || lowered.find("vendor") != std::string::npos ||
           lowered.find("dependency") != std::string::npos;
}

bool isNumericOnlySegment(const std::string_view segment) {
    return !segment.empty() && std::all_of(segment.begin(), segment.end(), [](unsigned char c) {
        return std::isdigit(c) != 0;
    });
}

std::string componentStemForFile(const std::filesystem::path& relativePath) {
    const std::string fileName = normalizedFileName(relativePath);
    if (isBootstrapFileName(fileName) || fileName.empty()) {
        return {};
    }

    std::string stem = relativePath.stem().string();
    if (stem.empty()) {
        return {};
    }

    return lowerCopy(std::move(stem));
}

std::string joinSegments(const std::vector<std::string>& segments, const std::size_t count) {
    if (segments.empty() || count == 0) {
        return "(root)";
    }

    std::string value;
    for (std::size_t index = 0; index < count && index < segments.size(); ++index) {
        if (!value.empty()) {
            value += '/';
        }
        value += segments[index];
    }
    return value.empty() ? std::string("(root)") : value;
}

std::string groupedRootModuleName(const std::vector<std::string>& segments) {
    if (segments.empty()) {
        return "(root)";
    }
    if (segments.size() == 1) {
        return segments.front();
    }
    if (isLikelyDirectorySegment(segments[1])) {
        return segments[0] + "/" + segments[1];
    }

    const std::string stem = std::filesystem::path(segments[1]).stem().string();
    return stem.empty() ? segments[0] : segments[0] + "/" + stem;
}

bool isJavaSourceLayout(const std::vector<std::string>& segments) {
    return segments.size() >= 4 &&
           segments[0] == "src" &&
           (segments[1] == "main" || segments[1] == "test") &&
           (segments[2] == "java" || segments[2] == "kotlin");
}

std::string javaSourceSetPrefix(const std::vector<std::string>& segments) {
    if (!isJavaSourceLayout(segments)) {
        return {};
    }
    return segments[0] + "/" + segments[1];
}

std::vector<std::string> javaPackageSegments(const std::vector<std::string>& segments) {
    if (!isJavaSourceLayout(segments) || segments.size() <= 4) {
        return {};
    }

    return std::vector<std::string>(segments.begin() + 3, segments.end() - 1);
}

void narrowCommonPrefix(std::vector<std::string>& prefix, const std::vector<std::string>& candidate) {
    const std::size_t limit = std::min(prefix.size(), candidate.size());
    std::size_t index = 0;
    while (index < limit && prefix[index] == candidate[index]) {
        ++index;
    }
    prefix.resize(index);
}

struct JavaSourceSetStats {
    bool initialized = false;
    std::vector<std::string> commonPackagePrefix;
};

std::string javaModuleName(
    const std::vector<std::string>& segments,
    const JavaSourceSetStats* stats) {
    if (!isJavaSourceLayout(segments)) {
        return {};
    }

    const std::string modulePrefix = javaSourceSetPrefix(segments);
    const std::vector<std::string> packageSegments = javaPackageSegments(segments);
    const std::string stem = lowerCopy(std::filesystem::path(segments.back()).stem().string());

    if (endsWith(stem, "application") || stem == "main" || stem == "bootstrap" || stem == "startup") {
        return modulePrefix + "/bootstrap";
    }

    if (!packageSegments.empty()) {
        const std::string lastPackage = lowerCopy(packageSegments.back());
        if (lastPackage != "com" && lastPackage != "org" && lastPackage != "net" &&
            lastPackage != "io" && lastPackage != "cn" && lastPackage != "dev" &&
            lastPackage != "app") {
            return modulePrefix + "/" + lastPackage;
        }
    }

    if (stats != nullptr && !stats->commonPackagePrefix.empty() &&
        packageSegments.size() > stats->commonPackagePrefix.size()) {
        return modulePrefix + "/" + lowerCopy(packageSegments[stats->commonPackagePrefix.size()]);
    }

    if (stem.empty()) {
        return modulePrefix;
    }
    return modulePrefix + "/" + stem;
}

struct TopLevelStats {
    std::size_t fileCount = 0;
    std::unordered_set<std::string> secondLevelDirs;
    std::size_t directFileCount = 0;
    std::unordered_set<std::string> directComponentStems;
    bool dependencyLike = false;
    bool numericScratch = false;
    bool hasDirectBootstrapFile = false;
    bool hasDirectManifest = false;
};

struct PrefixStats {
    std::size_t fileCount = 0;
    std::unordered_set<std::string> thirdLevelDirs;
};

}  // namespace

bool hasSourceExtension(const std::filesystem::path& filePath) {
    static const std::unordered_set<std::string> extensions = {
        ".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx"
    };

    return hasNormalizedExtension(filePath, extensions);
}

bool hasArchitectureRelevantExtension(const std::filesystem::path& filePath) {
    if (isProjectManifestFile(normalizedFileName(filePath))) {
        return true;
    }

    static const std::unordered_set<std::string> extensions = {
        ".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx",
        ".qml", ".js", ".mjs", ".cjs", ".ts", ".tsx", ".mts", ".cts", ".html", ".htm", ".css",
        ".json", ".py", ".java",
        ".ui", ".qrc", ".rc", ".pro", ".pri"
    };

    return hasNormalizedExtension(filePath, extensions);
}

bool shouldSkipGeneratedFile(const std::filesystem::path& filePath) {
    std::string fileName = filePath.filename().string();
    std::transform(fileName.begin(), fileName.end(), fileName.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (fileName == "package-lock.json" || fileName == "yarn.lock" ||
        fileName == "pnpm-lock.yaml" || fileName == "bun.lockb") {
        return true;
    }

    // 原有：Qt 生成文件
    if (fileName.starts_with("moc_") || fileName.starts_with("qrc_") || fileName.starts_with("ui_")) {
        return true;
    }
    if (fileName.find("qmltyperegistrations") != std::string::npos ||
        fileName.find("qmlcache_loader") != std::string::npos) {
        return true;
    }

    // ★ 新增：CMake 编译器探测文件
    if (fileName.starts_with("cmake") ||
        fileName.find("compilerid") != std::string::npos ||
        fileName.find("compilerId") != std::string::npos ||
        fileName.find("featuretest") != std::string::npos ||
        fileName.find("feature_tests") != std::string::npos) {
        return true;
    }

    // ★ 新增：常见构建产物
    if (fileName.ends_with(".pb.h") || fileName.ends_with(".pb.cc") ||
        fileName.ends_with(".generated.h") ||
        fileName.ends_with(".designer.cs") ||
        fileName.ends_with("_p.h")) {
        return true;
    }

    // ★ 新增：Python 字节码
    if (fileName.ends_with(".pyc") || fileName.ends_with(".pyo")) {
        return true;
    }

    return false;
}

bool shouldSkipDirectory(const std::filesystem::path& directoryPath, const AnalyzerOptions& options) {
    const std::string name = directoryPath.filename().string();

    if (!options.includeThirdParty && name == "third_party") {
        return true;
    }

    if (!options.includeBuildDirectories && (name == "build" || name == ".qt")) {
        return true;
    }

    // ★ 新增：构建系统生成的中间目录
    static const std::unordered_set<std::string> buildDirs = {
        "cmake-build-debug", "cmake-build-release",
        "cmake-build-relwithdebinfo", "cmake-build-minsizerel",
        "CMakeFiles", "_deps", "autogen",
        ".cmake", "Testing", "CTestTestfile",
        "x64", "x86", "Debug", "Release",
        "RelWithDebInfo", "MinSizeRel",
        "__pycache__", ".mypy_cache", "node_modules",
        ".m2", ".mvn", "target", ".gradle",
        "dist", "out", ".next", ".nuxt"
    };

    const std::string nameLower = lowerCopy(name);
    if (buildDirs.contains(name) || buildDirs.contains(nameLower)) {
        return true;
    }

    // qtcreator-* 或 msvc* 等构建变体目录
    if (nameLower.starts_with("cmake-build-") ||
        nameLower.starts_with("qtcreator-") ||
        nameLower.starts_with("msvc") ||
        nameLower.starts_with(".qt")) {
        return true;
    }

    return name == ".git" || name == ".idea" || name == ".vscode";
}

std::vector<std::filesystem::path> collectSourceFiles(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options) {
    std::vector<std::filesystem::path> files;

    std::error_code errorCode;
    std::filesystem::recursive_directory_iterator iterator(
        rootPath,
        std::filesystem::directory_options::skip_permission_denied,
        errorCode);

    for (auto end = std::filesystem::recursive_directory_iterator(); iterator != end; iterator.increment(errorCode)) {
        if (isCancellationRequested(options)) {
            break;
        }

        if (errorCode) {
            errorCode.clear();
            continue;
        }

        const std::filesystem::directory_entry& entry = *iterator;
        if (entry.is_directory(errorCode)) {
            if (shouldSkipDirectory(entry.path(), options)) {
                iterator.disable_recursion_pending();
            }
            continue;
        }

        if (entry.is_regular_file(errorCode) &&
            hasArchitectureRelevantExtension(entry.path()) &&
            !shouldSkipGeneratedFile(entry.path())) {
            files.push_back(entry.path());
            if (options.maxFiles != 0 && files.size() >= options.maxFiles) {
                break;
            }
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

std::optional<std::string> readAllText(const std::filesystem::path& filePath) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input.is_open()) {
        return std::nullopt;
    }

    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::string toUnixPath(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    return value;
}

std::string normalizePath(const std::filesystem::path& value) {
    return toUnixPath(value.lexically_normal().string());
}

std::string relativizePath(const std::filesystem::path& rootPath, const std::filesystem::path& filePath) {
    std::error_code errorCode;
    const std::filesystem::path relativePath = std::filesystem::relative(filePath, rootPath, errorCode);
    if (errorCode) {
        return normalizePath(filePath);
    }

    return normalizePath(relativePath);
}

std::vector<std::string> splitPathSegments(std::string_view path) {
    std::vector<std::string> segments;
    std::size_t start = 0;
    while (start < path.size()) {
        const std::size_t separator = path.find('/', start);
        const std::size_t length = separator == std::string_view::npos ? path.size() - start : separator - start;
        if (length > 0) {
            segments.emplace_back(path.substr(start, length));
        }
        if (separator == std::string_view::npos) {
            break;
        }
        start = separator + 1;
    }
    return segments;
}

std::unordered_map<std::string, std::string> inferModuleNames(const std::vector<std::string>& relativePaths) {
    std::unordered_map<std::string, std::string> moduleNames;
    std::unordered_map<std::string, TopLevelStats> topLevelStats;
    std::unordered_map<std::string, PrefixStats> prefixStats;
    std::unordered_map<std::string, JavaSourceSetStats> javaSourceSetStats;

    for (const std::string& relativePath : relativePaths) {
        const std::vector<std::string> segments = splitPathSegments(relativePath);
        if (segments.empty()) {
            continue;
        }

        TopLevelStats& topLevel = topLevelStats[segments.front()];
        ++topLevel.fileCount;
        topLevel.dependencyLike = topLevel.dependencyLike || isDependencyLikeSegment(segments.front());
        topLevel.numericScratch = topLevel.numericScratch || isNumericOnlySegment(segments.front());

        if (segments.size() == 2) {
            ++topLevel.directFileCount;
            const std::filesystem::path relativeFile(relativePath);
            const std::string fileName = normalizedFileName(relativeFile);
            topLevel.hasDirectBootstrapFile = topLevel.hasDirectBootstrapFile ||
                                             fileName == "main.cpp" || fileName == "main.cc" ||
                                             fileName == "main.cxx" || fileName == "main.c";
            topLevel.hasDirectManifest = topLevel.hasDirectManifest || isProjectManifestFile(fileName);
            if (const std::string stem = componentStemForFile(relativeFile); !stem.empty()) {
                topLevel.directComponentStems.emplace(stem);
            }
        }

        if (segments.size() >= 3 && isLikelyDirectorySegment(segments[1])) {
            topLevel.secondLevelDirs.emplace(segments[1]);

            PrefixStats& prefix = prefixStats[segments[0] + "/" + segments[1]];
            ++prefix.fileCount;
            if (segments.size() >= 4 && isLikelyDirectorySegment(segments[2])) {
                prefix.thirdLevelDirs.emplace(segments[2]);
            }
        }

        if (isJavaSourceLayout(segments)) {
            JavaSourceSetStats& stats = javaSourceSetStats[javaSourceSetPrefix(segments)];
            const std::vector<std::string> packageSegments = javaPackageSegments(segments);
            if (!stats.initialized) {
                stats.initialized = true;
                stats.commonPackagePrefix = packageSegments;
            } else {
                narrowCommonPrefix(stats.commonPackagePrefix, packageSegments);
            }
        }
    }

    static const std::unordered_set<std::string> groupedRoots = {
        "apps", "src", "tests", "samples", "docs", "include"
    };

    for (const std::string& relativePath : relativePaths) {
        const std::vector<std::string> segments = splitPathSegments(relativePath);
        if (segments.empty()) {
            moduleNames.emplace(relativePath, "(root)");
            continue;
        }

        if (segments.size() == 1) {
            moduleNames.emplace(relativePath, isLikelyDirectorySegment(segments.front()) ? segments.front() : std::string("(root)"));
            continue;
        }

        const std::string& topLevelName = segments.front();
        const TopLevelStats& topLevel = topLevelStats[topLevelName];
        const std::filesystem::path relativeFile(relativePath);
        const std::string fileName = normalizedFileName(relativeFile);
        std::string moduleName;

        if (topLevel.dependencyLike) {
            moduleName = topLevelName;
        }

        const bool shouldSplitFlatTopLevel = !topLevel.dependencyLike && !topLevel.numericScratch &&
                                             segments.size() == 2 &&
                                             topLevel.directFileCount >= 8 &&
                                             topLevel.directComponentStems.size() >= 5 &&
                                             (topLevel.hasDirectBootstrapFile || topLevel.hasDirectManifest);
        if (moduleName.empty() && shouldSplitFlatTopLevel) {
            if (isBootstrapFileName(fileName)) {
                moduleName = topLevelName;
            } else if (const std::string componentStem = componentStemForFile(relativeFile); !componentStem.empty()) {
                moduleName = topLevelName + "/" + componentStem;
            }
        }

        if (moduleName.empty() && isJavaSourceLayout(segments)) {
            const auto javaStatsIt = javaSourceSetStats.find(javaSourceSetPrefix(segments));
            moduleName = javaModuleName(
                segments,
                javaStatsIt == javaSourceSetStats.end() ? nullptr : &javaStatsIt->second);
        }

        if (moduleName.empty() && groupedRoots.contains(topLevelName)) {
            moduleName = groupedRootModuleName(segments);
        }

        if (moduleName.empty() && segments.size() >= 4 && isLikelyDirectorySegment(segments[1])) {
            const std::string prefixKey = segments[0] + "/" + segments[1];
            const auto prefixIt = prefixStats.find(prefixKey);
            if (prefixIt != prefixStats.end() &&
                prefixIt->second.fileCount >= 6 &&
                prefixIt->second.thirdLevelDirs.size() >= 2 &&
                isLikelyDirectorySegment(segments[2])) {
                moduleName = joinSegments(segments, 3);
            }
        }

        if (moduleName.empty() && segments.size() >= 3 && isLikelyDirectorySegment(segments[1]) &&
            topLevel.fileCount >= 4 && topLevel.secondLevelDirs.size() >= 2) {
            moduleName = joinSegments(segments, 2);
        }

        if (moduleName.empty() && segments.size() >= 4 && isGenericBridgeSegment(segments[1]) && isLikelyDirectorySegment(segments[2])) {
            moduleName = segments[0] + "/" + segments[2];
        }

        if (moduleName.empty()) {
            moduleName = topLevelName;
        }

        moduleNames.emplace(relativePath, std::move(moduleName));
    }

    return moduleNames;
}

std::string inferModuleName(const std::string& relativePath) {
    const std::vector<std::string> segments = splitPathSegments(relativePath);
    if (segments.empty()) {
        return "(root)";
    }
    if (segments.size() == 1) {
        return isLikelyDirectorySegment(segments.front()) ? segments.front() : std::string("(root)");
    }

    if (isDependencyLikeSegment(segments.front()) || isNumericOnlySegment(segments.front())) {
        return segments.front();
    }

    static const std::unordered_set<std::string> groupedRoots = {
        "apps", "src", "tests", "samples", "docs", "include"
    };

    if (const std::string javaModule = javaModuleName(segments, nullptr); !javaModule.empty()) {
        return javaModule;
    }

    if (groupedRoots.contains(segments.front()) && segments.size() >= 2) {
        return groupedRootModuleName(segments);
    }

    return segments.front();
}

const char* toString(const AnalyzerPrecision precision) {
    switch (precision) {
    case AnalyzerPrecision::SyntaxOnly:
        return "syntax_only";
    case AnalyzerPrecision::SemanticPreferred:
        return "semantic_preferred";
    case AnalyzerPrecision::SemanticRequired:
        return "semantic_required";
    }

    return "unknown";
}

bool isCancellationRequested(const AnalyzerOptions& options) {
    return options.cancellationRequested && options.cancellationRequested();
}

bool isSemanticBackendBuilt() {
#ifdef SAVT_ENABLE_CLANG_TOOLING
    return true;
#else
    return false;
#endif
}

SemanticBackendBuildInfo semanticBackendBuildInfo() {
    SemanticBackendBuildInfo info;

#ifdef SAVT_CLANG_TOOLING_REQUESTED
    info.requested = true;
#endif

#ifdef SAVT_ENABLE_CLANG_TOOLING
    info.available = true;
#endif

#ifdef SAVT_CONFIGURED_LLVM_ROOT
    info.configuredLlvmRoot = SAVT_CONFIGURED_LLVM_ROOT;
#endif

    if (info.available) {
        info.statusCode = "semantic_ready";
        info.statusMessage = "Clang/LibTooling semantic backend is available in this build.";
        return info;
    }

#ifdef SAVT_CLANG_TOOLING_MISSING
    info.statusCode = "backend_unavailable";
    if (!info.configuredLlvmRoot.empty()) {
        info.statusMessage =
            "SAVT_ENABLE_CLANG_TOOLING was requested, but LLVM/Clang headers or libraries were not found under " +
            info.configuredLlvmRoot + ".";
    } else {
        info.statusMessage =
            "SAVT_ENABLE_CLANG_TOOLING was requested, but LLVM/Clang headers or libraries were not found.";
    }
    return info;
#endif

    info.statusCode = "backend_unavailable";
    info.statusMessage =
        "This build does not include the Clang/LibTooling semantic backend. Reconfigure with "
        "SAVT_ENABLE_CLANG_TOOLING=ON to enable semantic analysis.";
    return info;
}

std::optional<std::filesystem::path> locateCompilationDatabase(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options) {
    std::vector<std::filesystem::path> candidates;
    const auto appendCandidate = [&candidates](const std::filesystem::path& candidate) {
        if (candidate.empty()) {
            return;
        }
        candidates.push_back(candidate);
        if (candidate.filename() != "compile_commands.json") {
            candidates.push_back(candidate / "compile_commands.json");
        }
    };

    appendCandidate(options.compilationDatabasePath);
    appendCandidate(rootPath / "compile_commands.json");
    appendCandidate(rootPath / ".qtc_clangd" / "compile_commands.json");
    appendCandidate(rootPath / "build" / "compile_commands.json");
    appendCandidate(rootPath / "build" / ".qtc_clangd" / "compile_commands.json");

    const std::filesystem::path buildRoot = rootPath / "build";
    std::error_code errorCode;
    if (std::filesystem::exists(buildRoot, errorCode) && std::filesystem::is_directory(buildRoot, errorCode)) {
        for (const auto& entry : std::filesystem::directory_iterator(buildRoot, errorCode)) {
            if (errorCode) {
                break;
            }
            if (entry.is_directory(errorCode)) {
                appendCandidate(entry.path() / "compile_commands.json");
                appendCandidate(entry.path() / ".qtc_clangd" / "compile_commands.json");
            }
        }
    }

    std::unordered_set<std::string> seenCandidates;
    for (const std::filesystem::path& candidate : candidates) {
        if (candidate.empty()) {
            continue;
        }

        const std::filesystem::path normalizedCandidate = candidate.lexically_normal();
        if (!seenCandidates.emplace(normalizePath(normalizedCandidate)).second) {
            continue;
        }

        if (std::filesystem::exists(normalizedCandidate, errorCode) && std::filesystem::is_regular_file(normalizedCandidate, errorCode)) {
            return normalizedCandidate;
        }
    }

    return std::nullopt;
}


SourceLanguage detectLanguage(const std::filesystem::path& filePath) {
    const std::string ext = lowerCopy(filePath.extension().string());
    if (ext == ".py")                         return SourceLanguage::Python;
    if (ext == ".java")                       return SourceLanguage::Java;
    if (ext == ".ts" || ext == ".tsx" || ext == ".mts" || ext == ".cts")
                                              return SourceLanguage::TypeScript;
    if (ext == ".js" || ext == ".mjs" || ext == ".cjs")
                                              return SourceLanguage::JavaScript;
    if (ext == ".h"  || ext == ".hh"  ||
        ext == ".hpp"|| ext == ".hxx" ||
        ext == ".c"  || ext == ".cc"  ||
        ext == ".cpp"|| ext == ".cxx")        return SourceLanguage::Cpp;
    return SourceLanguage::Unknown;
}

bool isHeuristicAnalyzableFile(const std::filesystem::path& filePath) {
    const SourceLanguage lang = detectLanguage(filePath);
    return lang == SourceLanguage::Python
        || lang == SourceLanguage::Java
        || lang == SourceLanguage::JavaScript
        || lang == SourceLanguage::TypeScript;
}

}  // namespace savt::analyzer::detail

