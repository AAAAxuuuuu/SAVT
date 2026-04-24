#include "AnalyzerUtilities.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
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
           fileName == "qmldir" ||
           fileName == "pom.xml" || fileName == "mvnw" ||
           fileName == "build.gradle" || fileName == "build.gradle.kts" ||
           fileName == "settings.gradle" || fileName == "settings.gradle.kts" ||
           fileName == "gradlew" ||
           endsWith(fileName, ".pro") || endsWith(fileName, ".pri");
}

bool isBootstrapFileName(const std::string& fileName) {
    return fileName == "main.cpp" || fileName == "main.cc" || fileName == "main.cxx" ||
           fileName == "main.cp" || fileName == "main.c++" || fileName == "main.cppm" ||
           fileName == "main.ixx" || fileName == "main.mm" || fileName == "main.c" ||
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

bool pathContainsSegment(const std::string& normalizedLowerPath, const std::string_view segment) {
    const std::string token = "/" + std::string(segment) + "/";
    return normalizedLowerPath.find(token) != std::string::npos ||
           normalizedLowerPath.starts_with(std::string(segment) + "/") ||
           normalizedLowerPath.ends_with("/" + std::string(segment));
}

bool matchesConfiguredPathPrefix(const std::string& candidate, const std::string& prefix) {
    return savt::core::matchesProjectConfigPrefix(candidate, prefix);
}

bool isLikelyToolchainDirectory(const std::string& nameLower) {
    return nameLower == "llvm" ||
           nameLower == "clang" ||
           nameLower == "downloads" ||
           nameLower == "sdk" ||
           nameLower == "sdks" ||
           nameLower == "toolchain" ||
           nameLower == "toolchains" ||
           nameLower == ".qtc_clangd" ||
           nameLower.starts_with("clang+llvm-") ||
           nameLower.starts_with("llvm-") ||
           nameLower.starts_with("mingw") ||
           nameLower.starts_with("msvc") ||
           nameLower == "qt" ||
           nameLower.starts_with("qt-");
}

std::size_t pathSegmentCount(const std::filesystem::path& path) {
    std::size_t count = 0;
    for (const std::filesystem::path& segment : path) {
        const std::string value = segment.string();
        if (!value.empty() && value != ".") {
            ++count;
        }
    }
    return count;
}

bool shouldSkipConfiguredDirectory(
    const std::filesystem::path& rootPath,
    const std::filesystem::path& directoryPath,
    const savt::core::ProjectAnalysisConfig& config) {
    if (!config.loaded || config.ignoreDirectories.empty()) {
        return false;
    }

    std::error_code errorCode;
    const std::filesystem::path relativeDirectory =
        std::filesystem::relative(directoryPath, rootPath, errorCode);
    const std::string relativePath = errorCode
        ? normalizePath(directoryPath)
        : normalizePath(relativeDirectory);
    if (relativePath.empty() || relativePath == ".") {
        return false;
    }

    return std::any_of(
        config.ignoreDirectories.begin(),
        config.ignoreDirectories.end(),
        [&](const std::string& prefix) {
            return matchesConfiguredPathPrefix(relativePath, prefix);
        });
}

std::string applyConfiguredModuleMerge(
    const std::string& relativePath,
    std::string inferredModuleName,
    const savt::core::ProjectAnalysisConfig* config) {
    if (config == nullptr || !config->loaded) {
        return inferredModuleName;
    }

    for (const savt::core::ProjectModuleMergeRule& rule : config->moduleMerges) {
        if (matchesConfiguredPathPrefix(relativePath, rule.matchPrefix) ||
            matchesConfiguredPathPrefix(inferredModuleName, rule.matchPrefix)) {
            inferredModuleName = rule.targetModule;
        }
    }
    return inferredModuleName;
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

struct CMakeCompilationDatabaseAttempt {
    std::filesystem::path buildDirectory;
    std::string generatorName;
};

std::string configuredCMakeCommand() {
#ifdef SAVT_CMAKE_COMMAND
    const std::string configuredPath = SAVT_CMAKE_COMMAND;
    std::error_code errorCode;
    if (!configuredPath.empty() &&
        std::filesystem::exists(std::filesystem::path(configuredPath), errorCode)) {
        return configuredPath;
    }
#endif
    return "cmake";
}

std::string configuredCMakeMakeProgram() {
#ifdef SAVT_CMAKE_MAKE_PROGRAM
    const std::string configuredPath = SAVT_CMAKE_MAKE_PROGRAM;
    std::error_code errorCode;
    if (!configuredPath.empty() &&
        std::filesystem::exists(std::filesystem::path(configuredPath), errorCode)) {
        return configuredPath;
    }
#endif
    return {};
}

std::string environmentValue(const char* name) {
    const char* value = std::getenv(name);
    return value == nullptr ? std::string() : std::string(value);
}

std::string quoteShellArgument(const std::string& value) {
#ifdef _WIN32
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        if (ch == '"') {
            escaped += "\\\"";
        } else {
            escaped += ch;
        }
    }
    return "\"" + escaped + "\"";
#else
    std::string escaped = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            escaped += "'\\''";
        } else {
            escaped += ch;
        }
    }
    escaped += "'";
    return escaped;
#endif
}

std::string quoteShellPath(const std::filesystem::path& path) {
    return quoteShellArgument(path.lexically_normal().string());
}

std::filesystem::path savtCompileDatabaseRoot(const std::filesystem::path& rootPath) {
    return rootPath / ".savt";
}

std::vector<CMakeCompilationDatabaseAttempt> cmakeCompilationDatabaseAttempts(
    const std::filesystem::path& rootPath) {
    const std::filesystem::path savtRoot = savtCompileDatabaseRoot(rootPath);
    if (!environmentValue("CMAKE_GENERATOR").empty()) {
        return {{savtRoot / "compile-db", {}}};
    }

    return {
        {savtRoot / "compile-db-ninja", "Ninja"},
        {savtRoot / "compile-db", {}}
    };
}

bool appendExistingCompilationDatabase(
    CompilationDatabaseProbe& probe,
    const std::filesystem::path& candidate) {
    std::error_code errorCode;
    const std::filesystem::path normalizedCandidate = candidate.lexically_normal();
    if (std::filesystem::exists(normalizedCandidate, errorCode) &&
        std::filesystem::is_regular_file(normalizedCandidate, errorCode)) {
        probe.resolvedPath = normalizedCandidate;
        return true;
    }
    return false;
}

bool configureCMakeCompilationDatabase(
    const std::filesystem::path& rootPath,
    const CMakeCompilationDatabaseAttempt& attempt,
    std::vector<std::string>& diagnostics) {
    std::error_code errorCode;
    std::filesystem::create_directories(attempt.buildDirectory, errorCode);
    if (errorCode) {
        diagnostics.push_back(
            "Automatic compile_commands.json generation failed: could not create build directory " +
            normalizePath(attempt.buildDirectory) + ".");
        return false;
    }

    const std::filesystem::path logPath = attempt.buildDirectory / "cmake-configure.log";
    std::ostringstream command;
#ifdef _WIN32
    command << "call ";
#endif
    command << quoteShellArgument(configuredCMakeCommand())
            << " -S " << quoteShellPath(rootPath)
            << " -B " << quoteShellPath(attempt.buildDirectory);
    if (!attempt.generatorName.empty()) {
        command << " -G " << quoteShellArgument(attempt.generatorName);
        const std::string makeProgram = configuredCMakeMakeProgram();
        if (attempt.generatorName == "Ninja" && !makeProgram.empty()) {
            command << " -DCMAKE_MAKE_PROGRAM=" << quoteShellArgument(makeProgram);
        }
    }
    command << " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
            << " > " << quoteShellPath(logPath) << " 2>&1";

    diagnostics.push_back(
        "Automatic compile_commands.json generation: running CMake configure in " +
        normalizePath(attempt.buildDirectory) +
        (attempt.generatorName.empty() ? std::string(".") : " with generator " + attempt.generatorName + "."));

    const int exitCode = std::system(command.str().c_str());
    const std::filesystem::path generatedPath = attempt.buildDirectory / "compile_commands.json";
    if (exitCode == 0 && std::filesystem::exists(generatedPath, errorCode) &&
        std::filesystem::is_regular_file(generatedPath, errorCode)) {
        diagnostics.push_back(
            "Automatic compile_commands.json generation succeeded: " + normalizePath(generatedPath) + ".");
        return true;
    }

    if (exitCode != 0) {
        diagnostics.push_back(
            "Automatic compile_commands.json generation failed: CMake configure exited with code " +
            std::to_string(exitCode) + ". See " + normalizePath(logPath) + ".");
    } else {
        diagnostics.push_back(
            "Automatic compile_commands.json generation did not produce compile_commands.json. "
            "The selected CMake generator may not support CMAKE_EXPORT_COMPILE_COMMANDS. See " +
            normalizePath(logPath) + ".");
    }
    return false;
}

}  // namespace

bool hasSourceExtension(const std::filesystem::path& filePath) {
    static const std::unordered_set<std::string> extensions = {
        ".h", ".hh", ".hpp", ".hxx", ".h++",
        ".c", ".cc", ".cpp", ".cxx", ".cp", ".c++",
        ".ipp", ".inl", ".tpp", ".tcc", ".txx",
        ".ixx", ".cppm", ".ii", ".cu", ".cuh", ".mm"
    };

    return hasNormalizedExtension(filePath, extensions);
}

bool hasArchitectureRelevantExtension(const std::filesystem::path& filePath) {
    if (isProjectManifestFile(normalizedFileName(filePath))) {
        return true;
    }

    static const std::unordered_set<std::string> extensions = {
        ".h", ".hh", ".hpp", ".hxx", ".h++",
        ".c", ".cc", ".cpp", ".cxx", ".cp", ".c++",
        ".ipp", ".inl", ".tpp", ".tcc", ".txx",
        ".ixx", ".cppm", ".ii", ".cu", ".cuh", ".mm",
        ".qml", ".js", ".mjs", ".cjs", ".ts", ".tsx", ".mts", ".cts", ".html", ".htm", ".css",
        ".json", ".py", ".java",
        ".ui", ".qrc", ".rc", ".pro", ".pri", ".qbs", ".qmldir", ".cmake",
        ".s", ".asm", ".def"
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
    const std::string nameLower = lowerCopy(name);

    if (!options.includeThirdParty &&
        isDependencyLikeSegment(nameLower) &&
        pathSegmentCount(directoryPath) <= 1) {
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

    const std::string normalizedLowerPath =
        lowerCopy(directoryPath.lexically_normal().generic_string());
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

    if (nameLower == ".qtc_clangd" || nameLower == ".savt" || nameLower == ".cache" || nameLower == "cache") {
        return true;
    }

    const bool underToolsLikeRoot =
        pathContainsSegment(normalizedLowerPath, "tools") ||
        pathContainsSegment(normalizedLowerPath, "tool") ||
        pathContainsSegment(normalizedLowerPath, "toolchains") ||
        pathContainsSegment(normalizedLowerPath, "downloads");
    if (underToolsLikeRoot && isLikelyToolchainDirectory(nameLower)) {
        return true;
    }

    return name == ".git" || name == ".idea" || name == ".vscode";
}

std::vector<std::filesystem::path> collectSourceFiles(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options) {
    std::vector<std::filesystem::path> files;
    const savt::core::ProjectAnalysisConfig projectConfig =
        savt::core::loadProjectAnalysisConfig(rootPath);

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
            std::error_code relativeError;
            const std::filesystem::path relativeDirectory =
                std::filesystem::relative(entry.path(), rootPath, relativeError);
            const std::filesystem::path directoryForFiltering =
                relativeError ? entry.path().filename() : relativeDirectory;
            if (shouldSkipDirectory(directoryForFiltering, options) ||
                shouldSkipConfiguredDirectory(rootPath, entry.path(), projectConfig)) {
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

std::unordered_map<std::string, std::string> inferModuleNames(
    const std::vector<std::string>& relativePaths,
    const savt::core::ProjectAnalysisConfig* config) {
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
            topLevel.hasDirectBootstrapFile =
                topLevel.hasDirectBootstrapFile || isBootstrapFileName(fileName);
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

        moduleNames.emplace(relativePath, applyConfiguredModuleMerge(relativePath, std::move(moduleName), config));
    }

    return moduleNames;
}

std::string inferModuleName(
    const std::string& relativePath,
    const savt::core::ProjectAnalysisConfig* config) {
    const std::vector<std::string> segments = splitPathSegments(relativePath);
    if (segments.empty()) {
        return "(root)";
    }
    if (segments.size() == 1) {
        const std::string moduleName =
            isLikelyDirectorySegment(segments.front()) ? segments.front() : std::string("(root)");
        return applyConfiguredModuleMerge(relativePath, moduleName, config);
    }

    if (isDependencyLikeSegment(segments.front()) || isNumericOnlySegment(segments.front())) {
        return applyConfiguredModuleMerge(relativePath, segments.front(), config);
    }

    static const std::unordered_set<std::string> groupedRoots = {
        "apps", "src", "tests", "samples", "docs", "include"
    };

    if (const std::string javaModule = javaModuleName(segments, nullptr); !javaModule.empty()) {
        return applyConfiguredModuleMerge(relativePath, javaModule, config);
    }

    if (groupedRoots.contains(segments.front()) && segments.size() >= 2) {
        return applyConfiguredModuleMerge(relativePath, groupedRootModuleName(segments), config);
    }

    return applyConfiguredModuleMerge(relativePath, segments.front(), config);
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

#ifdef SAVT_CLANG_TOOLING_STATUS_CODE
    info.statusCode = SAVT_CLANG_TOOLING_STATUS_CODE;
#endif

#ifdef SAVT_CLANG_TOOLING_STATUS_MESSAGE
    info.statusMessage = SAVT_CLANG_TOOLING_STATUS_MESSAGE;
#endif

    if (info.available) {
        if (info.statusCode.empty()) {
            info.statusCode = "semantic_ready";
        }
        if (info.statusMessage.empty()) {
            info.statusMessage = "Clang/LibTooling semantic backend is available in this build.";
        }
        return info;
    }

    if (info.statusCode.empty()) {
        info.statusCode = "backend_unavailable";
    }
    if (info.statusMessage.empty()) {
        info.statusMessage =
            "This build does not include the Clang/LibTooling semantic backend. Reconfigure with "
            "SAVT_ENABLE_CLANG_TOOLING=ON to enable semantic analysis.";
    }
    return info;
}

CompilationDatabaseProbe probeCompilationDatabase(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options) {
    CompilationDatabaseProbe probe;

    std::vector<std::filesystem::path> candidates;
    const auto appendCandidate = [&candidates, &probe](const std::filesystem::path& candidate) {
        if (candidate.empty()) {
            return;
        }
        candidates.push_back(candidate);
        if (candidate.filename() != "compile_commands.json") {
            candidates.push_back(candidate / "compile_commands.json");
        }
    };

    if (!options.compilationDatabasePath.empty()) {
        probe.explicitPathProvided = true;
        appendCandidate(options.compilationDatabasePath);
    }
    appendCandidate(rootPath / "compile_commands.json");
    appendCandidate(rootPath / ".qtc_clangd" / "compile_commands.json");
    appendCandidate(rootPath / ".savt" / "compile-db-ninja" / "compile_commands.json");
    appendCandidate(rootPath / ".savt" / "compile-db" / "compile_commands.json");
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

        probe.searchedPaths.push_back(normalizedCandidate);

        if (appendExistingCompilationDatabase(probe, normalizedCandidate)) {
            probe.resolvedPath = normalizedCandidate;
            return probe;
        }
    }

    return probe;
}

CompilationDatabaseProbe prepareCompilationDatabase(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options) {
    CompilationDatabaseProbe probe = probeCompilationDatabase(rootPath, options);
    if (probe.resolvedPath.has_value() ||
        !options.autoGenerateCompilationDatabase ||
        !options.compilationDatabasePath.empty() ||
        isCancellationRequested(options)) {
        return probe;
    }

    const std::filesystem::path cmakeListsPath = rootPath / "CMakeLists.txt";
    std::error_code errorCode;
    if (!std::filesystem::exists(cmakeListsPath, errorCode) ||
        !std::filesystem::is_regular_file(cmakeListsPath, errorCode)) {
        probe.diagnostics.push_back(
            "Automatic compile_commands.json generation skipped: no CMakeLists.txt was found at the project root.");
        return probe;
    }

    probe.generationAttempted = true;
    for (const CMakeCompilationDatabaseAttempt& attempt : cmakeCompilationDatabaseAttempts(rootPath)) {
        if (isCancellationRequested(options)) {
            probe.diagnostics.push_back(
                "Automatic compile_commands.json generation canceled before CMake configure completed.");
            return probe;
        }

        probe.generationBuildDirectory = attempt.buildDirectory.lexically_normal();
        if (!configureCMakeCompilationDatabase(rootPath, attempt, probe.diagnostics)) {
            continue;
        }

        const std::filesystem::path generatedPath = attempt.buildDirectory / "compile_commands.json";
        if (appendExistingCompilationDatabase(probe, generatedPath)) {
            probe.generated = true;
            return probe;
        }
    }

    CompilationDatabaseProbe refreshedProbe = probeCompilationDatabase(rootPath, options);
    refreshedProbe.diagnostics = std::move(probe.diagnostics);
    refreshedProbe.generationAttempted = probe.generationAttempted;
    refreshedProbe.generated = probe.generated;
    refreshedProbe.generationBuildDirectory = std::move(probe.generationBuildDirectory);
    return refreshedProbe;
}

std::optional<std::filesystem::path> locateCompilationDatabase(
    const std::filesystem::path& rootPath,
    const AnalyzerOptions& options) {
    return probeCompilationDatabase(rootPath, options).resolvedPath;
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
        ext == ".hpp"|| ext == ".hxx" || ext == ".h++" ||
        ext == ".c"  || ext == ".cc"  ||
        ext == ".cpp"|| ext == ".cxx" || ext == ".cp" || ext == ".c++" ||
        ext == ".ipp"|| ext == ".inl" || ext == ".tpp" || ext == ".tcc" || ext == ".txx" ||
        ext == ".ixx"|| ext == ".cppm" || ext == ".ii" || ext == ".cu" || ext == ".cuh" ||
        ext == ".mm")                         return SourceLanguage::Cpp;
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
