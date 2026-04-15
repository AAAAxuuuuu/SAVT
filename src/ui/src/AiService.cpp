#include "savt/ui/AiService.h"
#include "savt/ui/FileInsightService.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <algorithm>

namespace savt::ui {

namespace {

QStringList toQStringList(const QVariant& value) {
    QStringList items;
    for (const QVariant& entry : value.toList()) {
        const QString cleaned = entry.toString().trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned))
            items.push_back(cleaned);
    }
    return items;
}

QVariantList toVariantStringList(const QStringList& values) {
    QVariantList items;
    items.reserve(values.size());
    for (const QString& value : values) {
        const QString cleaned = value.trimmed();
        if (!cleaned.isEmpty())
            items.push_back(cleaned);
    }
    return items;
}

QString projectNameFromRootPath(const QString& rootPath) {
    const QFileInfo fileInfo(rootPath);
    return fileInfo.fileName().isEmpty() ? QStringLiteral("当前项目")
                                         : fileInfo.fileName();
}

QString inferAnalyzerPrecision(const QString& reportText) {
    if (reportText.contains(QStringLiteral("semantic-required"),
                            Qt::CaseInsensitive) ||
        reportText.contains(QStringLiteral("语义优先"), Qt::CaseInsensitive)) {
        return QStringLiteral("semantic_preferred");
    }
    if (reportText.contains(QStringLiteral("semantic"), Qt::CaseInsensitive) ||
        reportText.contains(QStringLiteral("语义分析"), Qt::CaseInsensitive)) {
        return QStringLiteral("semantic_available");
    }
    return QStringLiteral("syntax_fallback");
}

QStringList deduplicateQStringList(const QStringList& values) {
    QStringList items;
    for (const QString& value : values) {
        const QString cleaned = value.trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned))
            items.push_back(cleaned);
    }
    return items;
}

QStringList mergeQStringLists(const QStringList& first,
                              const QStringList& second) {
    QStringList items = deduplicateQStringList(first);
    for (const QString& value : second) {
        const QString cleaned = value.trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned))
            items.push_back(cleaned);
    }
    return items;
}

QStringList extractMarkdownHighlights(const QString& text,
                                      const int maxItems,
                                      const int maxLineLength = 160) {
    QStringList items;
    const QStringList rawLines = text.split(QLatin1Char('\n'));
    for (QString line : rawLines) {
        line = line.trimmed();
        while (line.startsWith(QLatin1Char('#'))) {
            line.remove(0, 1);
            line = line.trimmed();
        }
        if (line.isEmpty() || line == QStringLiteral("---"))
            continue;
        if (line.size() > maxLineLength)
            line = line.left(maxLineLength - 3) + QStringLiteral("...");
        if (!items.contains(line))
            items.push_back(line);
        if (items.size() >= maxItems)
            break;
    }
    return items;
}

QString trimTrailingWhitespace(QString text) {
    while (!text.isEmpty()) {
        const QChar last = text.back();
        if (last != QLatin1Char(' ') && last != QLatin1Char('\t')) {
            break;
        }
        text.chop(1);
    }
    return text;
}

QString normalizedRepositoryPreview(QString text,
                                    const int maxLines,
                                    const int maxChars,
                                    const bool preserveIndentation) {
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    const QStringList rawLines = text.split(QLatin1Char('\n'));
    QStringList lines;
    lines.reserve(std::min(static_cast<int>(rawLines.size()), maxLines));

    int blankRun = 0;
    for (const QString& rawLine : rawLines) {
        QString line = preserveIndentation ? trimTrailingWhitespace(rawLine)
                                           : rawLine.simplified();
        if (line.isEmpty()) {
            if (lines.isEmpty() || blankRun >= 1) {
                continue;
            }
            ++blankRun;
            lines.push_back(QString());
        } else {
            blankRun = 0;
            lines.push_back(line);
        }

        if (lines.size() >= maxLines) {
            break;
        }
    }

    QString preview = lines.join(QStringLiteral("\n")).trimmed();
    if (preview.isEmpty()) {
        return {};
    }

    if (preview.size() > maxChars) {
        preview = preview.left(maxChars).trimmed();
    }
    return preview;
}

QString readRepositoryFilePreview(const QString& absolutePath,
                                  const int maxChars,
                                  const int maxLines,
                                  const bool preserveIndentation) {
    QFile file(absolutePath);
    if (!file.exists() ||
        !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    const qint64 maxBytes = std::max(4096, maxChars * 6);
    const QByteArray bytes = file.read(maxBytes);
    const bool truncated = !file.atEnd();
    QString preview = normalizedRepositoryPreview(
        QString::fromUtf8(bytes.constData(), bytes.size()),
        maxLines,
        maxChars,
        preserveIndentation);
    if (preview.isEmpty()) {
        return {};
    }

    if (truncated) {
        preview += preserveIndentation
                       ? QStringLiteral("\n...[truncated]")
                       : QStringLiteral(" ...[truncated]");
    }
    return preview;
}

QString lowerFileName(const QString& path) {
    return QFileInfo(path).fileName().trimmed().toLower();
}

QString lowerSuffix(const QString& path) {
    return QFileInfo(path).suffix().trimmed().toLower();
}

bool isRepositoryReadmeFile(const QString& path) {
    const QString fileName = lowerFileName(path);
    return fileName == QStringLiteral("readme") ||
           fileName.startsWith(QStringLiteral("readme."));
}

bool isRepositoryDocFile(const QString& path) {
    const QString fileName = lowerFileName(path);
    const QString suffix = lowerSuffix(path);
    if (suffix != QStringLiteral("md") &&
        suffix != QStringLiteral("markdown") &&
        suffix != QStringLiteral("txt") &&
        suffix != QStringLiteral("rst") &&
        suffix != QStringLiteral("adoc")) {
        return false;
    }

    if (isRepositoryReadmeFile(path)) {
        return true;
    }

    return fileName.contains(QStringLiteral("overview")) ||
           fileName.contains(QStringLiteral("architecture")) ||
           fileName.contains(QStringLiteral("introduction")) ||
           fileName.contains(QStringLiteral("intro")) ||
           fileName.contains(QStringLiteral("quickstart")) ||
           fileName.contains(QStringLiteral("getting-started")) ||
           fileName.contains(QStringLiteral("guide"));
}

int repositoryDocumentPriority(const QString& path) {
    const QString fileName = lowerFileName(path);
    if (isRepositoryReadmeFile(fileName)) {
        return 300;
    }
    if (fileName.contains(QStringLiteral("overview"))) {
        return 220;
    }
    if (fileName.contains(QStringLiteral("architecture"))) {
        return 210;
    }
    if (fileName.contains(QStringLiteral("introduction")) ||
        fileName.contains(QStringLiteral("intro"))) {
        return 200;
    }
    if (fileName.contains(QStringLiteral("quickstart")) ||
        fileName.contains(QStringLiteral("getting-started"))) {
        return 190;
    }
    if (fileName.contains(QStringLiteral("guide"))) {
        return 180;
    }
    return 100;
}

bool isRepositoryManifestFile(const QString& path) {
    const QString fileName = lowerFileName(path);
    const QString suffix = lowerSuffix(path);
    static const QStringList exactFileNames = {
        QStringLiteral("package.json"),
        QStringLiteral("pyproject.toml"),
        QStringLiteral("cargo.toml"),
        QStringLiteral("go.mod"),
        QStringLiteral("pom.xml"),
        QStringLiteral("build.gradle"),
        QStringLiteral("build.gradle.kts"),
        QStringLiteral("settings.gradle"),
        QStringLiteral("settings.gradle.kts"),
        QStringLiteral("cmakelists.txt"),
        QStringLiteral("cmakepresets.json"),
        QStringLiteral("requirements.txt"),
        QStringLiteral("setup.py"),
        QStringLiteral("makefile"),
        QStringLiteral("meson.build"),
        QStringLiteral("composer.json"),
        QStringLiteral("pubspec.yaml"),
        QStringLiteral("pubspec.yml"),
        QStringLiteral("gemfile"),
        QStringLiteral("mix.exs")
    };
    if (exactFileNames.contains(fileName)) {
        return true;
    }
    return suffix == QStringLiteral("pro") ||
           suffix == QStringLiteral("pri") ||
           suffix == QStringLiteral("qbs") ||
           suffix == QStringLiteral("sln");
}

int repositoryManifestPriority(const QString& path) {
    const QString fileName = lowerFileName(path);
    if (fileName == QStringLiteral("package.json")) {
        return 240;
    }
    if (fileName == QStringLiteral("pyproject.toml")) {
        return 235;
    }
    if (fileName == QStringLiteral("cargo.toml")) {
        return 230;
    }
    if (fileName == QStringLiteral("go.mod")) {
        return 225;
    }
    if (fileName == QStringLiteral("pom.xml")) {
        return 220;
    }
    if (fileName == QStringLiteral("build.gradle.kts") ||
        fileName == QStringLiteral("build.gradle")) {
        return 210;
    }
    if (fileName == QStringLiteral("cmakelists.txt")) {
        return 205;
    }
    if (fileName == QStringLiteral("cmakepresets.json")) {
        return 200;
    }
    if (fileName == QStringLiteral("requirements.txt") ||
        fileName == QStringLiteral("setup.py")) {
        return 195;
    }
    if (fileName == QStringLiteral("makefile") ||
        fileName == QStringLiteral("meson.build")) {
        return 190;
    }
    return 150;
}

bool isRepositorySnippetFile(const QString& path) {
    const QString fileName = lowerFileName(path);
    const QString suffix = lowerSuffix(path);
    if (fileName == QStringLiteral("cmakelists.txt")) {
        return true;
    }

    static const QStringList snippetSuffixes = {
        QStringLiteral("c"),   QStringLiteral("cc"),  QStringLiteral("cpp"),
        QStringLiteral("cxx"), QStringLiteral("h"),   QStringLiteral("hh"),
        QStringLiteral("hpp"), QStringLiteral("hxx"), QStringLiteral("ixx"),
        QStringLiteral("cppm"), QStringLiteral("qml"), QStringLiteral("js"),
        QStringLiteral("mjs"), QStringLiteral("cjs"), QStringLiteral("ts"),
        QStringLiteral("tsx"), QStringLiteral("py"),  QStringLiteral("java"),
        QStringLiteral("kt"),  QStringLiteral("go"),  QStringLiteral("rs"),
        QStringLiteral("cs"),  QStringLiteral("swift")
    };
    return snippetSuffixes.contains(suffix);
}

int repositorySnippetPriority(const QString& path) {
    const QString normalizedPath =
        QDir::fromNativeSeparators(path).trimmed().toLower();
    const QString fileName = QFileInfo(normalizedPath).fileName();
    const QString suffix = QFileInfo(normalizedPath).suffix().trimmed().toLower();
    int score = 0;

    if (fileName == QStringLiteral("main.cpp") ||
        fileName == QStringLiteral("main.cc") ||
        fileName == QStringLiteral("main.cxx") ||
        fileName == QStringLiteral("main.c") ||
        fileName == QStringLiteral("main.py") ||
        fileName == QStringLiteral("main.go") ||
        fileName == QStringLiteral("main.rs") ||
        fileName == QStringLiteral("main.qml") ||
        fileName == QStringLiteral("program.cs")) {
        score += 240;
    } else if (fileName == QStringLiteral("app.py") ||
               fileName == QStringLiteral("server.js") ||
               fileName == QStringLiteral("server.ts") ||
               fileName == QStringLiteral("index.js") ||
               fileName == QStringLiteral("index.ts") ||
               fileName == QStringLiteral("index.tsx") ||
               fileName == QStringLiteral("app.qml")) {
        score += 210;
    }

    if (normalizedPath.contains(QStringLiteral("/apps/"))) {
        score += 60;
    }
    if (normalizedPath.contains(QStringLiteral("/src/"))) {
        score += 45;
    }
    if (normalizedPath.contains(QStringLiteral("/app/")) ||
        normalizedPath.contains(QStringLiteral("/cmd/")) ||
        normalizedPath.contains(QStringLiteral("/server/")) ||
        normalizedPath.contains(QStringLiteral("/client/"))) {
        score += 35;
    }

    if (suffix == QStringLiteral("qml")) {
        score += 20;
    } else if (suffix == QStringLiteral("cpp") ||
               suffix == QStringLiteral("cc") ||
               suffix == QStringLiteral("cxx") ||
               suffix == QStringLiteral("py") ||
               suffix == QStringLiteral("ts") ||
               suffix == QStringLiteral("tsx") ||
               suffix == QStringLiteral("js")) {
        score += 12;
    } else if (suffix == QStringLiteral("h") ||
               suffix == QStringLiteral("hh") ||
               suffix == QStringLiteral("hpp") ||
               suffix == QStringLiteral("hxx")) {
        score -= 8;
    }

    if (normalizedPath.contains(QStringLiteral("/tests/")) ||
        normalizedPath.contains(QStringLiteral("/test/")) ||
        normalizedPath.contains(QStringLiteral("/spec/")) ||
        normalizedPath.contains(QStringLiteral("/example/")) ||
        normalizedPath.contains(QStringLiteral("/examples/"))) {
        score -= 80;
    }
    if (normalizedPath.contains(QStringLiteral("/third_party/")) ||
        normalizedPath.contains(QStringLiteral("/vendor/")) ||
        normalizedPath.contains(QStringLiteral("/external/"))) {
        score -= 120;
    }

    return score;
}

QString formatRepositoryContextEntry(const QString& kind,
                                     const QString& relativePath,
                                     const QString& previewText) {
    return QStringLiteral("%1 | %2\n%3")
        .arg(kind.trimmed(), QDir::fromNativeSeparators(relativePath.trimmed()), previewText.trimmed());
}

void appendRepositoryContextEntry(QStringList* entries,
                                  QStringList* seenPaths,
                                  const QString& rootPath,
                                  const QString& filePath,
                                  const QString& kind,
                                  const int maxChars,
                                  const int maxLines,
                                  const bool preserveIndentation) {
    if (!entries || !seenPaths) {
        return;
    }

    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return;
    }

    const QString absolutePath = fileInfo.absoluteFilePath();
    const QString relativePath =
        QDir::fromNativeSeparators(QDir(rootPath).relativeFilePath(absolutePath));
    if (relativePath.isEmpty() || relativePath.startsWith(QStringLiteral("../"))) {
        return;
    }
    if (seenPaths->contains(relativePath)) {
        return;
    }

    const QString preview = readRepositoryFilePreview(
        absolutePath, maxChars, maxLines, preserveIndentation);
    if (preview.isEmpty()) {
        return;
    }

    entries->push_back(formatRepositoryContextEntry(kind, relativePath, preview));
    seenPaths->push_back(relativePath);
}

QStringList collectProjectOverviewDocuments(const QString& projectRootPath) {
    QStringList entries;
    QStringList seenPaths;
    const QDir rootDir(projectRootPath);
    if (!rootDir.exists()) {
        return entries;
    }

    QFileInfoList rootFiles =
        rootDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    std::stable_sort(rootFiles.begin(), rootFiles.end(), [](const QFileInfo& left,
                                                            const QFileInfo& right) {
        return repositoryDocumentPriority(left.fileName()) >
               repositoryDocumentPriority(right.fileName());
    });

    int readmeCount = 0;
    for (const QFileInfo& fileInfo : rootFiles) {
        if (!isRepositoryReadmeFile(fileInfo.fileName())) {
            continue;
        }
        appendRepositoryContextEntry(
            &entries,
            &seenPaths,
            projectRootPath,
            fileInfo.absoluteFilePath(),
            QStringLiteral("README 摘录"),
            1400,
            42,
            false);
        ++readmeCount;
        if (readmeCount >= 1) {
            break;
        }
    }

    const QDir docsDir(rootDir.filePath(QStringLiteral("docs")));
    if (docsDir.exists()) {
        QFileInfoList docsFiles =
            docsDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        std::stable_sort(docsFiles.begin(), docsFiles.end(), [](const QFileInfo& left,
                                                                const QFileInfo& right) {
            return repositoryDocumentPriority(left.fileName()) >
                   repositoryDocumentPriority(right.fileName());
        });

        int docsCount = 0;
        for (const QFileInfo& fileInfo : docsFiles) {
            if (!isRepositoryDocFile(fileInfo.fileName())) {
                continue;
            }
            appendRepositoryContextEntry(
                &entries,
                &seenPaths,
                projectRootPath,
                fileInfo.absoluteFilePath(),
                QStringLiteral("项目文档"),
                900,
                28,
                false);
            ++docsCount;
            if (docsCount >= 1) {
                break;
            }
        }
    }

    std::stable_sort(rootFiles.begin(), rootFiles.end(), [](const QFileInfo& left,
                                                            const QFileInfo& right) {
        return repositoryManifestPriority(left.fileName()) >
               repositoryManifestPriority(right.fileName());
    });

    int manifestCount = 0;
    for (const QFileInfo& fileInfo : rootFiles) {
        if (!isRepositoryManifestFile(fileInfo.fileName())) {
            continue;
        }
        appendRepositoryContextEntry(
            &entries,
            &seenPaths,
            projectRootPath,
            fileInfo.absoluteFilePath(),
            QStringLiteral("工程清单"),
            700,
            28,
            true);
        ++manifestCount;
        if (manifestCount >= 2) {
            break;
        }
    }

    return entries;
}

QStringList fallbackProjectOverviewSnippetPaths(const QString& projectRootPath) {
    QStringList candidates;
    const QDir rootDir(projectRootPath);
    const QStringList directories = {
        QString(),
        QStringLiteral("src"),
        QStringLiteral("app"),
        QStringLiteral("apps"),
        QStringLiteral("cmd"),
        QStringLiteral("server"),
        QStringLiteral("client")
    };
    const QStringList fileNames = {
        QStringLiteral("main.cpp"),
        QStringLiteral("main.cc"),
        QStringLiteral("main.c"),
        QStringLiteral("main.py"),
        QStringLiteral("main.go"),
        QStringLiteral("main.rs"),
        QStringLiteral("main.qml"),
        QStringLiteral("app.qml"),
        QStringLiteral("index.js"),
        QStringLiteral("index.ts"),
        QStringLiteral("index.tsx"),
        QStringLiteral("server.js"),
        QStringLiteral("server.ts"),
        QStringLiteral("Program.cs")
    };

    for (const QString& directoryName : directories) {
        const QDir directory(directoryName.isEmpty()
                                 ? rootDir
                                 : QDir(rootDir.filePath(directoryName)));
        if (!directory.exists()) {
            continue;
        }

        for (const QString& fileName : fileNames) {
            const QString absolutePath = directory.filePath(fileName);
            if (QFileInfo::exists(absolutePath)) {
                candidates.push_back(
                    QDir::fromNativeSeparators(rootDir.relativeFilePath(absolutePath)));
            }
        }
    }

    return deduplicateQStringList(candidates);
}

QStringList collectProjectOverviewSnippets(const QString& projectRootPath,
                                           const QStringList& candidatePaths) {
    QStringList entries;
    QStringList seenPaths;
    const QDir rootDir(projectRootPath);
    if (!rootDir.exists()) {
        return entries;
    }

    QStringList prioritizedPaths = deduplicateQStringList(candidatePaths);
    std::stable_sort(prioritizedPaths.begin(), prioritizedPaths.end(), [](const QString& left,
                                                                          const QString& right) {
        return repositorySnippetPriority(left) > repositorySnippetPriority(right);
    });
    prioritizedPaths = mergeQStringLists(
        prioritizedPaths, fallbackProjectOverviewSnippetPaths(projectRootPath));

    for (const QString& candidatePath : prioritizedPaths) {
        const QString cleanedPath =
            QDir::fromNativeSeparators(candidatePath).trimmed();
        if (cleanedPath.isEmpty() || !isRepositorySnippetFile(cleanedPath)) {
            continue;
        }

        const QFileInfo candidateInfo(cleanedPath);
        const QString absolutePath = candidateInfo.isAbsolute()
                                         ? candidateInfo.absoluteFilePath()
                                         : rootDir.filePath(cleanedPath);
        appendRepositoryContextEntry(
            &entries,
            &seenPaths,
            projectRootPath,
            absolutePath,
            QStringLiteral("关键源码"),
            900,
            34,
            true);
        if (entries.size() >= 2) {
            break;
        }
    }

    return entries;
}

QStringList collectNodeFieldValues(const QVariantList& nodeItems,
                                   const QString& fieldName,
                                   const int maxItems) {
    QStringList items;
    for (const QVariant& value : nodeItems) {
        const QVariantMap node = value.toMap();
        const QVariant fieldValue = node.value(fieldName);
        QStringList candidateValues;
        if (fieldValue.metaType().id() == QMetaType::QString) {
            const QString cleaned = fieldValue.toString().trimmed();
            if (!cleaned.isEmpty())
                candidateValues.push_back(cleaned);
        } else {
            candidateValues = toQStringList(fieldValue);
        }

        for (const QString& candidate : candidateValues) {
            if (!candidate.isEmpty() && !items.contains(candidate))
                items.push_back(candidate);
            if (items.size() >= maxItems)
                return items;
        }
    }
    return items;
}

QStringList collectStructuredPreview(const QVariantList& items,
                                     const QString& titleField,
                                     const QString& bodyField,
                                     const int maxItems) {
    QStringList previewItems;
    for (const QVariant& value : items) {
        const QVariantMap item = value.toMap();
        const QString title = item.value(titleField).toString().trimmed();
        const QString body = item.value(bodyField).toString().trimmed();

        QString preview;
        if (!title.isEmpty() && !body.isEmpty()) {
            preview = QStringLiteral("%1：%2").arg(title, body);
        } else if (!title.isEmpty()) {
            preview = title;
        } else {
            preview = body;
        }

        if (preview.isEmpty() || previewItems.contains(preview))
            continue;
        previewItems.push_back(preview);
        if (previewItems.size() >= maxItems)
            break;
    }
    return previewItems;
}

QString buildAiSetupMessage(const ai::DeepSeekConfigLoadResult& loadResult) {
    if (!loadResult.hasConfig())
        return QStringLiteral("AI 未就绪：%1").arg(loadResult.errorMessage);
    if (loadResult.loadedFromBuiltInDefaults && !loadResult.config.enabled) {
        return QStringLiteral("已加载编译期 AI 配置，但当前未启用。请重新配置 CMake 后再构建。");
    }
    if (!loadResult.config.enabled) {
        return QStringLiteral("已找到 AI 配置，但当前未启用。把 %1 里的 enabled 改成 true 后即可使用。")
            .arg(QDir::toNativeSeparators(loadResult.configPath));
    }
    if (!loadResult.config.isUsable()) {
        return QStringLiteral(
            "AI 配置已加载，但还不能使用。请检查模型、地址、endpointPath 和 API Key 是否完整。");
    }
    if (loadResult.loadedFromBuiltInDefaults) {
        return QStringLiteral("%1 已就绪，当前模型为 %2，请求地址为 %3。当前使用的是编译期 AI 配置。")
            .arg(loadResult.config.providerLabel(),
                 loadResult.config.model,
                 loadResult.config.resolvedChatCompletionsUrl());
    }
    return QStringLiteral("%1 已就绪，当前模型为 %2，请求地址为 %3。AI 只会基于当前项目或节点的证据包进行解读。")
        .arg(loadResult.config.providerLabel(),
             loadResult.config.model,
             loadResult.config.resolvedChatCompletionsUrl());
}

QString extractApiErrorMessage(const QByteArray& bytes) {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return {};
    const QJsonObject object = document.object();
    if (!object.contains(QStringLiteral("error")) ||
        !object.value(QStringLiteral("error")).isObject()) {
        return {};
    }
    return object.value(QStringLiteral("error"))
        .toObject()
        .value(QStringLiteral("message"))
        .toString()
        .trimmed();
}

QString publicScopeLabel(const QString& scope) {
    if (scope == QStringLiteral("project_overview")) {
        return QStringLiteral("项目总览");
    }
    if (scope == QStringLiteral("system_context")) {
        return QStringLiteral("项目导览");
    }
    if (scope == QStringLiteral("engineering_report")) {
        return QStringLiteral("报告导览");
    }
    if (scope == QStringLiteral("file_node")) {
        return QStringLiteral("文件解读");
    }
    if (scope == QStringLiteral("capability_map")) {
        return QStringLiteral("能力导览");
    }
    if (scope == QStringLiteral("component_node")) {
        return QStringLiteral("模块解读");
    }
    return QStringLiteral("模块解读");
}

ai::ArchitectureAssistantRequest buildCapabilityRequest(
    const AiRequestContext& context,
    const QVariantMap& nodeData) {
    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.uiScope = QStringLiteral("l2_capability_map");
    request.learningStage = QStringLiteral("L2");
    request.audience = QStringLiteral("beginner");
    request.explanationGoal = QStringLiteral(
        "Help a beginner understand what this capability owns, how it connects to nearby capabilities, and when to drill into L3.");
    request.nodeName = nodeData.value(QStringLiteral("name")).toString().trimmed();
    request.nodeKind = nodeData.value(QStringLiteral("kind")).toString().trimmed();
    request.nodeRole = nodeData.value(QStringLiteral("role")).toString().trimmed();
    request.nodeSummary = nodeData.value(QStringLiteral("summary")).toString().trimmed();
    if (request.nodeSummary.isEmpty()) {
        request.nodeSummary =
            nodeData.value(QStringLiteral("responsibility")).toString().trimmed();
    }
    request.userTask =
        context.userTask.trimmed().isEmpty()
            ? QStringLiteral(
                  "Explain the selected capability to a beginner who is new to this repository. "
                  "Use plain language to describe what responsibility this capability owns, "
                  "which other capabilities it depends on or supports, what risks or signals are worth noticing, "
                  "and when the reader should continue into L3. Use only the supplied evidence package.")
            : context.userTask.trimmed();
    request.moduleNames = toQStringList(nodeData.value(QStringLiteral("moduleNames")));
    request.exampleFiles = toQStringList(nodeData.value(QStringLiteral("exampleFiles")));
    request.topSymbols = toQStringList(nodeData.value(QStringLiteral("topSymbols")));
    request.collaboratorNames =
        toQStringList(nodeData.value(QStringLiteral("collaboratorNames")));

    const QVariantMap evidence = nodeData.value(QStringLiteral("evidence")).toMap();
    request.diagnostics = {
        QStringLiteral("analysis phase: %1")
            .arg(context.analysisPhase.trimmed().isEmpty()
                     ? QStringLiteral("unknown")
                     : context.analysisPhase.trimmed()),
        QStringLiteral("ai scope: capability_map"),
        QStringLiteral("audience: beginner"),
        QStringLiteral("learning stage: L2"),
        QStringLiteral("risk level: %1")
            .arg(nodeData.value(QStringLiteral("riskLevel")).toString().trimmed()),
        QStringLiteral("incoming edges: %1")
            .arg(nodeData.value(QStringLiteral("incomingEdgeCount")).toULongLong()),
        QStringLiteral("outgoing edges: %1")
            .arg(nodeData.value(QStringLiteral("outgoingEdgeCount")).toULongLong())};
    const QStringList supportingRoles =
        toQStringList(nodeData.value(QStringLiteral("supportingRoles")));
    if (!supportingRoles.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("supporting roles: %1").arg(supportingRoles.join(QStringLiteral(", "))));
    }
    const QStringList technologyTags =
        toQStringList(nodeData.value(QStringLiteral("technologyTags")));
    if (!technologyTags.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("technology tags: %1").arg(technologyTags.join(QStringLiteral(", "))));
    }
    const QString confidenceLabel =
        evidence.value(QStringLiteral("confidenceLabel")).toString().trimmed();
    if (!confidenceLabel.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("evidence confidence: %1").arg(confidenceLabel));
    }
    const QString confidenceReason =
        evidence.value(QStringLiteral("confidenceReason")).toString().trimmed();
    if (!confidenceReason.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("confidence reason: %1").arg(confidenceReason));
    }
    return request;
}

ai::ArchitectureAssistantRequest buildComponentRequest(
    const AiRequestContext& context,
    const QVariantMap& nodeData) {
    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.uiScope = QStringLiteral("l3_component_guide");
    request.learningStage = QStringLiteral("L3");
    request.audience = QStringLiteral("beginner");
    request.explanationGoal = QStringLiteral(
        "Help a beginner understand what this component does, why it matters, and which files to read first.");
    request.nodeName = nodeData.value(QStringLiteral("name")).toString().trimmed();
    request.nodeKind = nodeData.value(QStringLiteral("kind")).toString().trimmed();
    request.nodeRole = nodeData.value(QStringLiteral("role")).toString().trimmed();
    request.nodeSummary = nodeData.value(QStringLiteral("summary")).toString().trimmed();
    if (request.nodeSummary.isEmpty()) {
        request.nodeSummary =
            nodeData.value(QStringLiteral("responsibility")).toString().trimmed();
    }
    request.userTask =
        context.userTask.trimmed().isEmpty()
            ? QStringLiteral(
                  "Explain the selected component to a beginner who is new to this repository. "
                  "Start with a plain-language explanation of what it does, then explain why it matters, "
                  "which files are the best starting points, and which nearby components it works with. "
                  "If you must use architecture jargon, explain it briefly. Use only the supplied evidence package.")
            : context.userTask.trimmed();
    request.moduleNames = toQStringList(nodeData.value(QStringLiteral("moduleNames")));
    request.exampleFiles = toQStringList(nodeData.value(QStringLiteral("exampleFiles")));
    request.topSymbols = toQStringList(nodeData.value(QStringLiteral("topSymbols")));
    request.collaboratorNames =
        toQStringList(nodeData.value(QStringLiteral("collaboratorNames")));
    request.diagnostics = {QStringLiteral("analysis phase: %1")
                               .arg(context.analysisPhase.trimmed().isEmpty()
                                        ? QStringLiteral("unknown")
                                        : context.analysisPhase.trimmed()),
                           QStringLiteral("ai scope: component_node"),
                           QStringLiteral("audience: beginner"),
                           QStringLiteral("learning stage: L3")};
    if (!context.analysisReport.trimmed().isEmpty() &&
        context.analysisReport.contains(QStringLiteral("语法"),
                                        Qt::CaseInsensitive)) {
        request.diagnostics.push_back(
            QStringLiteral("analysis precision may be syntax-fallback"));
    }
    request.diagnostics.push_back(QStringLiteral(
        "language inference hint: inspect file suffixes, import/include evidence, and symbol naming before assigning a role"));
    return request;
}

ai::ArchitectureAssistantRequest buildFileRequest(
    const AiRequestContext& context,
    const QVariantMap& nodeData) {
    const QVariantMap fileDetail =
        FileInsightService::buildDetail(context.projectRootPath, nodeData);

    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.uiScope = QStringLiteral("l3_file_guide");
    request.learningStage = QStringLiteral("L3");
    request.audience = QStringLiteral("beginner");
    request.explanationGoal = QStringLiteral(
        "Help a beginner understand what this concrete file does, which structural clues support that judgment, and which parts should be read first.");
    request.nodeName = fileDetail.value(QStringLiteral("fileName")).toString().trimmed();
    if (request.nodeName.isEmpty()) {
        request.nodeName = nodeData.value(QStringLiteral("name")).toString().trimmed();
    }
    request.nodeKind = QStringLiteral("file");
    request.nodeRole = fileDetail.value(QStringLiteral("roleLabel")).toString().trimmed();
    request.nodeSummary = fileDetail.value(QStringLiteral("summary")).toString().trimmed();
    if (request.nodeSummary.isEmpty()) {
        request.nodeSummary = nodeData.value(QStringLiteral("summary")).toString().trimmed();
    }
    request.userTask =
        context.userTask.trimmed().isEmpty()
            ? QStringLiteral(
                  "Explain the selected file to a beginner who is new to this repository. "
                  "Describe what this file is responsible for, which local code clues support that conclusion, "
                  "what it appears to depend on, which declarations or sections are worth reading first, "
                  "and what nearby files or components should be checked next. Use only the supplied evidence package.")
            : context.userTask.trimmed();

    request.moduleNames = toQStringList(nodeData.value(QStringLiteral("moduleNames")));
    request.exampleFiles = mergeQStringLists(
        {fileDetail.value(QStringLiteral("filePath")).toString().trimmed()},
        toQStringList(nodeData.value(QStringLiteral("exampleFiles"))));
    request.topSymbols = mergeQStringLists(
        toQStringList(nodeData.value(QStringLiteral("topSymbols"))),
        toQStringList(fileDetail.value(QStringLiteral("declarationClues"))));
    request.collaboratorNames =
        toQStringList(nodeData.value(QStringLiteral("collaboratorNames")));

    request.filePath = fileDetail.value(QStringLiteral("filePath")).toString().trimmed();
    request.fileLanguage = fileDetail.value(QStringLiteral("languageLabel")).toString().trimmed();
    request.fileCategory = fileDetail.value(QStringLiteral("categoryLabel")).toString().trimmed();
    request.fileRoleHint = fileDetail.value(QStringLiteral("roleLabel")).toString().trimmed();
    request.fileSummary = fileDetail.value(QStringLiteral("summary")).toString().trimmed();
    request.codeExcerpt = fileDetail.value(QStringLiteral("previewText")).toString().trimmed();
    request.fileImports = toQStringList(fileDetail.value(QStringLiteral("importClues")));
    request.fileDeclarations =
        toQStringList(fileDetail.value(QStringLiteral("declarationClues")));
    request.fileSignals = toQStringList(fileDetail.value(QStringLiteral("behaviorSignals")));
    request.fileReadingHints =
        toQStringList(fileDetail.value(QStringLiteral("readingHints")));
    request.contextClues = mergeQStringLists(
        {request.fileSummary,
         QStringLiteral("language: %1").arg(request.fileLanguage),
         QStringLiteral("category: %1").arg(request.fileCategory),
         QStringLiteral("role hint: %1").arg(request.fileRoleHint)},
        request.fileReadingHints);
    request.diagnostics = {
        QStringLiteral("analysis phase: %1")
            .arg(context.analysisPhase.trimmed().isEmpty()
                     ? QStringLiteral("unknown")
                     : context.analysisPhase.trimmed()),
        QStringLiteral("ai scope: file_node"),
        QStringLiteral("audience: beginner"),
        QStringLiteral("learning stage: L3"),
        QStringLiteral("file path: %1").arg(request.filePath),
        QStringLiteral("file language: %1").arg(request.fileLanguage),
        QStringLiteral("file category: %1").arg(request.fileCategory),
        QStringLiteral("file role hint: %1").arg(request.fileRoleHint),
        QStringLiteral("line count: %1")
            .arg(fileDetail.value(QStringLiteral("lineCount")).toULongLong()),
        QStringLiteral("non-empty lines: %1")
            .arg(fileDetail.value(QStringLiteral("nonEmptyLineCount")).toULongLong())};
    for (const QString& item : request.fileImports) {
        request.diagnostics.push_back(
            QStringLiteral("import clue: %1").arg(item));
    }
    for (const QString& item : request.fileDeclarations) {
        request.diagnostics.push_back(
            QStringLiteral("declaration clue: %1").arg(item));
    }
    for (const QString& item : request.fileSignals) {
        request.diagnostics.push_back(
            QStringLiteral("behavior signal: %1").arg(item));
    }
    for (const QString& item : request.fileReadingHints) {
        request.diagnostics.push_back(
            QStringLiteral("reading hint: %1").arg(item));
    }
    return request;
}

ai::ArchitectureAssistantRequest buildProjectRequest(
    const AiRequestContext& context,
    const QVariantMap& systemContextData,
    const QVariantList& systemContextCards,
    const QVariantList& capabilityNodeItems) {
    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.uiScope = QStringLiteral("l1_project_guide");
    request.learningStage = QStringLiteral("L1");
    request.audience = QStringLiteral("beginner");
    request.explanationGoal = QStringLiteral(
        "Help a beginner understand what the project is, who it serves, its main parts, and where to start reading.");
    request.nodeName = systemContextData.value(QStringLiteral("title")).toString().trimmed();
    if (request.nodeName.isEmpty()) {
        request.nodeName =
            QStringLiteral("%1 / 项目导览").arg(request.projectName);
    }
    request.nodeKind = QStringLiteral("system_context");
    request.nodeRole = QStringLiteral("project_context");

    QStringList summaryParts;
    summaryParts << systemContextData.value(QStringLiteral("headline")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("purposeSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("entrySummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("mainFlowSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("inputSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("outputSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("containerSummary")).toString().trimmed();
    const QStringList hotspotSignals = collectStructuredPreview(
        systemContextData.value(QStringLiteral("hotspotSignals")).toList(),
        QStringLiteral("title"),
        QStringLiteral("summary"),
        4);
    for (const QVariant& value : systemContextCards) {
        const QVariantMap item = value.toMap();
        const QString name = item.value(QStringLiteral("name")).toString().trimmed();
        const QString summary = item.value(QStringLiteral("summary")).toString().trimmed();
        if (!name.isEmpty() && !summary.isEmpty())
            summaryParts << QStringLiteral("%1：%2").arg(name, summary);
    }
    for (const QString& hotspot : hotspotSignals)
        summaryParts.push_back(QStringLiteral("结构热点：%1").arg(hotspot));
    request.nodeSummary = deduplicateQStringList(summaryParts).join(QStringLiteral(" "));
    request.userTask =
        context.userTask.trimmed().isEmpty()
            ? QStringLiteral(
                  "Explain the whole project to a beginner who is new to code repositories. "
                  "Use plain language to describe what the project is for, who uses it, its main parts, "
                  "and where to start reading first. If architecture jargon appears, explain it briefly. "
                  "Use only the supplied evidence package.")
            : context.userTask.trimmed();
    request.moduleNames =
        toQStringList(systemContextData.value(QStringLiteral("containerNames")));
    request.collaboratorNames =
        toQStringList(systemContextData.value(QStringLiteral("containerNames")));
    request.diagnostics = {
        QStringLiteral("analysis phase: %1")
            .arg(context.analysisPhase.trimmed().isEmpty()
                     ? QStringLiteral("unknown")
                     : context.analysisPhase.trimmed()),
        QStringLiteral("ai scope: system_context"),
        QStringLiteral("audience: beginner"),
        QStringLiteral("learning stage: L1"),
        QStringLiteral("technology summary: %1")
            .arg(systemContextData.value(QStringLiteral("technologySummary"))
                     .toString()
                     .trimmed())};
    const QString mainFlowSummary =
        systemContextData.value(QStringLiteral("mainFlowSummary")).toString().trimmed();
    if (!mainFlowSummary.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("main flow: %1").arg(mainFlowSummary));
    }
    for (const QString& hotspot : hotspotSignals) {
        request.diagnostics.push_back(
            QStringLiteral("hotspot signal: %1").arg(hotspot));
    }
    for (const QVariant& value : capabilityNodeItems) {
        const QVariantMap node = value.toMap();
        for (const QString& path :
             toQStringList(node.value(QStringLiteral("exampleFiles")))) {
            if (!request.exampleFiles.contains(path) &&
                request.exampleFiles.size() < 8) {
                request.exampleFiles.push_back(path);
            }
        }
        for (const QString& symbol :
             toQStringList(node.value(QStringLiteral("topSymbols")))) {
            if (!request.topSymbols.contains(symbol) &&
                request.topSymbols.size() < 8) {
                request.topSymbols.push_back(symbol);
            }
        }
    }
    return request;
}

ai::ArchitectureAssistantRequest buildProjectOverviewRequest(
    const AiRequestContext& context,
    const QVariantMap& systemContextData,
    const QVariantList& systemContextCards,
    const QVariantList& capabilityNodeItems) {
    ai::ArchitectureAssistantRequest request = buildProjectRequest(
        context, systemContextData, systemContextCards, capabilityNodeItems);
    request.uiScope = QStringLiteral("l1_project_overview");
    request.explanationGoal = QStringLiteral(
        "Read the supplied repository context and explain in plain Chinese what this repository actually does.");
    request.nodeName = QStringLiteral("%1 / 项目总览").arg(request.projectName);

    const QString projectKindSummary =
        systemContextData.value(QStringLiteral("projectKindSummary")).toString().trimmed();
    const QString purposeSummary =
        systemContextData.value(QStringLiteral("purposeSummary")).toString().trimmed();
    const QString entrySummary =
        systemContextData.value(QStringLiteral("entrySummary")).toString().trimmed();
    const QString mainFlowSummary =
        systemContextData.value(QStringLiteral("mainFlowSummary")).toString().trimmed();
    const QString inputSummary =
        systemContextData.value(QStringLiteral("inputSummary")).toString().trimmed();
    const QString outputSummary =
        systemContextData.value(QStringLiteral("outputSummary")).toString().trimmed();
    const QString technologySummary =
        systemContextData.value(QStringLiteral("technologySummary")).toString().trimmed();
    const QString containerSummary =
        systemContextData.value(QStringLiteral("containerSummary")).toString().trimmed();

    request.nodeSummary = deduplicateQStringList(
                              {projectKindSummary,
                               purposeSummary,
                               entrySummary,
                               mainFlowSummary,
                               inputSummary,
                               outputSummary,
                               technologySummary})
                              .join(QStringLiteral(" "));
    request.contextClues = deduplicateQStringList(
        {projectKindSummary,
         entrySummary,
         mainFlowSummary,
         inputSummary,
         outputSummary,
         technologySummary,
         containerSummary});
    request.moduleNames = request.moduleNames.mid(0, 3);
    request.collaboratorNames = request.collaboratorNames.mid(0, 3);
    request.exampleFiles = request.exampleFiles.mid(0, 3);
    request.topSymbols = request.topSymbols.mid(0, 4);
    request.repositoryDocuments =
        collectProjectOverviewDocuments(context.projectRootPath);
    request.repositorySnippets =
        collectProjectOverviewSnippets(context.projectRootPath, request.exampleFiles);
    request.userTask =
        context.userTask.trimmed().isEmpty()
            ? QStringLiteral(
                  "先读 README、项目文档、工程清单和关键源码摘录，再直接说明这个仓库是做什么的、给谁用、核心模块、入口主路径和输入输出。"
                  " README 不清楚时，再按工程结构和代码谨慎归纳。"
                  " 结果写进 plain_summary，控制在 120 到 200 个中文字符。"
                  " 避免套话、空泛称赞和页面说明式表述。")
            : context.userTask.trimmed();
    request.diagnostics = {
        QStringLiteral("analysis phase: %1")
            .arg(context.analysisPhase.trimmed().isEmpty()
                     ? QStringLiteral("unknown")
                     : context.analysisPhase.trimmed()),
        QStringLiteral("ai scope: project_overview"),
        QStringLiteral("audience: beginner"),
        QStringLiteral("learning stage: L1")
    };
    if (!mainFlowSummary.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("main flow: %1").arg(mainFlowSummary));
    }
    request.diagnostics.push_back(
        request.repositoryDocuments.isEmpty()
            ? QStringLiteral("repository documents: none found")
            : QStringLiteral("repository documents: %1 excerpt(s)")
                  .arg(request.repositoryDocuments.size()));
    request.diagnostics.push_back(
        request.repositorySnippets.isEmpty()
            ? QStringLiteral("repository snippets: none found")
            : QStringLiteral("repository snippets: %1 excerpt(s)")
                  .arg(request.repositorySnippets.size()));
    request.diagnostics.push_back(
        request.repositoryDocuments.join(QStringLiteral("\n")).contains(
            QStringLiteral("README"),
            Qt::CaseInsensitive)
            ? QStringLiteral("readme status: detected")
            : QStringLiteral("readme status: missing or unreadable; infer from code"));
    return request;
}

ai::ArchitectureAssistantRequest buildReportRequest(
    const AiRequestContext& context,
    const QVariantMap& systemContextData,
    const QVariantList& systemContextCards,
    const QVariantList& capabilityNodeItems) {
    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.uiScope = QStringLiteral("l4_engineering_report");
    request.learningStage = QStringLiteral("L4");
    request.audience = QStringLiteral("beginner");
    request.explanationGoal = QStringLiteral(
        "Help a beginner understand the real system architecture, the current risk picture, the best reading order, and the next concrete inspection steps.");
    request.nodeName = request.projectName;
    request.nodeKind = QStringLiteral("project_diagnosis");
    request.nodeRole = QStringLiteral("architecture_context");

    const QStringList contextClues = deduplicateQStringList(
        {systemContextData.value(QStringLiteral("projectKindSummary")).toString().trimmed(),
         systemContextData.value(QStringLiteral("purposeSummary")).toString().trimmed(),
         systemContextData.value(QStringLiteral("entrySummary")).toString().trimmed(),
         systemContextData.value(QStringLiteral("mainFlowSummary")).toString().trimmed(),
         systemContextData.value(QStringLiteral("inputSummary")).toString().trimmed(),
         systemContextData.value(QStringLiteral("outputSummary")).toString().trimmed(),
         systemContextData.value(QStringLiteral("technologySummary")).toString().trimmed(),
         systemContextData.value(QStringLiteral("containerSummary")).toString().trimmed()});
    const QStringList riskSignals = collectStructuredPreview(
        systemContextData.value(QStringLiteral("riskSignals")).toList(),
        QStringLiteral("title"),
        QStringLiteral("summary"),
        5);
    const QStringList readingOrder = collectStructuredPreview(
        systemContextData.value(QStringLiteral("readingOrder")).toList(),
        QStringLiteral("title"),
        QStringLiteral("body"),
        4);
    const QStringList hotspotSignals = collectStructuredPreview(
        systemContextData.value(QStringLiteral("hotspotSignals")).toList(),
        QStringLiteral("title"),
        QStringLiteral("summary"),
        4);

    QStringList summaryParts;
    summaryParts << systemContextData.value(QStringLiteral("headline")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("purposeSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("projectKindSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("entrySummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("mainFlowSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("inputSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("outputSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("technologySummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("containerSummary")).toString().trimmed();
    for (const QVariant& value : systemContextCards) {
        const QVariantMap item = value.toMap();
        const QString name = item.value(QStringLiteral("name")).toString().trimmed();
        const QString summary = item.value(QStringLiteral("summary")).toString().trimmed();
        if (!name.isEmpty() && !summary.isEmpty())
            summaryParts << QStringLiteral("%1：%2").arg(name, summary);
    }
    const QStringList reportHighlights =
        extractMarkdownHighlights(context.analysisReport, 10);
    for (const QString& highlight : reportHighlights)
        summaryParts.push_back(highlight);
    for (const QString& risk : riskSignals)
        summaryParts.push_back(QStringLiteral("风险线索：%1").arg(risk));
    for (const QString& hotspot : hotspotSignals)
        summaryParts.push_back(QStringLiteral("结构热点：%1").arg(hotspot));
    for (const QString& step : readingOrder)
        summaryParts.push_back(QStringLiteral("阅读顺序：%1").arg(step));
    if (!context.astPreviewSummary.trimmed().isEmpty()) {
        summaryParts.push_back(
            QStringLiteral("当前 AST 聚焦：%1").arg(context.astPreviewSummary.trimmed()));
    }
    request.nodeSummary = deduplicateQStringList(summaryParts).join(QStringLiteral(" "));
    request.contextClues = contextClues;
    request.riskSignals = riskSignals;
    request.readingOrder = readingOrder;
    request.reportHighlights = reportHighlights;
    request.userTask =
        context.userTask.trimmed().isEmpty()
            ? QStringLiteral(
                  "Explain the actual architecture of the current system to a beginner, not the value of the report page itself. "
                  "Use plain Chinese to cover the overall diagnosis, the main modules, the main risk sources, the best reading order, "
                  "the modules or files worth inspecting first, and the concrete next actions a reader should take. "
                  "Do not write meta commentary such as 'this report is important' or 'this report shows'. "
                  "Instead, directly describe the system, its responsibilities, its risks, and what the reader should inspect next. "
                  "Give enough detail for a report page, roughly 200-300 Chinese characters of useful substance across the answer, "
                  "while keeping every claim grounded in the supplied evidence package.")
            : context.userTask.trimmed();

    request.moduleNames = mergeQStringLists(
        toQStringList(systemContextData.value(QStringLiteral("containerNames"))),
        collectNodeFieldValues(capabilityNodeItems, QStringLiteral("moduleNames"), 10));
    request.collaboratorNames = collectNodeFieldValues(
        capabilityNodeItems, QStringLiteral("name"), 10);
    request.topSymbols = collectNodeFieldValues(
        capabilityNodeItems, QStringLiteral("topSymbols"), 12);
    if (!context.selectedAstFilePath.trimmed().isEmpty()) {
        request.exampleFiles.push_back(context.selectedAstFilePath.trimmed());
    }
    request.exampleFiles = mergeQStringLists(
        request.exampleFiles,
        collectNodeFieldValues(capabilityNodeItems, QStringLiteral("exampleFiles"), 12));

    request.diagnostics = {
        QStringLiteral("analysis phase: %1")
            .arg(context.analysisPhase.trimmed().isEmpty()
                     ? QStringLiteral("unknown")
                     : context.analysisPhase.trimmed()),
        QStringLiteral("ai scope: engineering_report"),
        QStringLiteral("audience: beginner"),
        QStringLiteral("learning stage: L4"),
        QStringLiteral("project kind: %1")
            .arg(systemContextData.value(QStringLiteral("projectKindSummary"))
                     .toString()
                     .trimmed()),
        QStringLiteral("entry summary: %1")
            .arg(systemContextData.value(QStringLiteral("entrySummary"))
                     .toString()
                     .trimmed()),
        QStringLiteral("input summary: %1")
            .arg(systemContextData.value(QStringLiteral("inputSummary"))
                     .toString()
                     .trimmed()),
        QStringLiteral("output summary: %1")
            .arg(systemContextData.value(QStringLiteral("outputSummary"))
                     .toString()
                     .trimmed()),
        QStringLiteral("technology summary: %1")
            .arg(systemContextData.value(QStringLiteral("technologySummary"))
                     .toString()
                     .trimmed())};
    if (!context.selectedAstFilePath.trimmed().isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("current ast file: %1").arg(context.selectedAstFilePath.trimmed()));
    }
    if (!context.astPreviewTitle.trimmed().isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("ast preview title: %1").arg(context.astPreviewTitle.trimmed()));
    }
    if (!context.astPreviewSummary.trimmed().isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("ast preview summary: %1").arg(context.astPreviewSummary.trimmed()));
    }
    const QString mainFlowSummary =
        systemContextData.value(QStringLiteral("mainFlowSummary")).toString().trimmed();
    if (!mainFlowSummary.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("main flow: %1").arg(mainFlowSummary));
    }
    for (const QString& risk : riskSignals) {
        request.diagnostics.push_back(
            QStringLiteral("risk signal: %1").arg(risk));
    }
    for (const QString& hotspot : hotspotSignals) {
        request.diagnostics.push_back(
            QStringLiteral("hotspot signal: %1").arg(hotspot));
    }
    for (const QString& step : readingOrder) {
        request.diagnostics.push_back(
            QStringLiteral("reading order: %1").arg(step));
    }
    for (const QString& highlight : reportHighlights) {
        request.diagnostics.push_back(
            QStringLiteral("report highlight: %1").arg(highlight));
    }
    return request;
}

QString formattedPayloadForDebug(const QByteArray& payload) {
    QString formattedPayload = QString::fromUtf8(payload);
    const QJsonDocument payloadDocument = QJsonDocument::fromJson(payload);
    if (!payloadDocument.isNull()) {
        formattedPayload =
            QString::fromUtf8(payloadDocument.toJson(QJsonDocument::Indented));
    }
    return formattedPayload;
}

QString formattedResponseForDebug(const QByteArray& responseBytes) {
    QString rawResponseText = QString::fromUtf8(responseBytes);
    const QJsonDocument responseDocument = QJsonDocument::fromJson(responseBytes);
    if (!responseDocument.isNull()) {
        rawResponseText =
            QString::fromUtf8(responseDocument.toJson(QJsonDocument::Indented));
    }
    return rawResponseText;
}

}  // namespace

AiAvailabilityState AiService::inspectAvailability() {
    AiAvailabilityState state;
    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    state.configPath = loadResult.configPath.trimmed().isEmpty()
        ? ai::defaultDeepSeekConfigPath()
        : loadResult.configPath;
    state.available = loadResult.hasConfig() && loadResult.config.isUsable();
    state.setupMessage = buildAiSetupMessage(loadResult);
    return state;
}

QString AiService::classifyNodeScope(const QVariantMap& nodeData) {
    if (nodeData.isEmpty())
        return QStringLiteral("node");
    if (nodeData.contains(QStringLiteral("capabilityId")) &&
        FileInsightService::isSingleFileNode(nodeData)) {
        return QStringLiteral("file_node");
    }
    return nodeData.contains(QStringLiteral("capabilityId"))
               ? QStringLiteral("component_node")
               : QStringLiteral("capability_map");
}

AiPreparedRequest AiService::prepareNodeRequest(
    const AiRequestContext& context,
    const QVariantMap& nodeData) {
    AiPreparedRequest prepared;
    prepared.scope = classifyNodeScope(nodeData);
    prepared.availability = inspectAvailability();

    prepared.targetName = nodeData.value(QStringLiteral("name")).toString().trimmed();
    if (prepared.targetName.isEmpty()) {
        prepared.failureStatusMessage =
            QStringLiteral("先在图里选中一个模块，再生成 AI 解读。");
        return prepared;
    }
    if (!prepared.availability.available) {
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    if (!loadResult.hasConfig() || !loadResult.config.isUsable()) {
        prepared.availability.available = false;
        prepared.availability.setupMessage = buildAiSetupMessage(loadResult);
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    prepared.ready = true;
    prepared.pendingStatusMessage =
        prepared.scope == QStringLiteral("capability_map")
            ? QStringLiteral("正在生成当前模块的解读...")
            : (prepared.scope == QStringLiteral("file_node")
                   ? QStringLiteral("正在生成当前文件的解读...")
                   : QStringLiteral("正在生成当前模块的解读..."));
    if (prepared.scope == QStringLiteral("capability_map")) {
        prepared.assistantRequest = buildCapabilityRequest(context, nodeData);
    } else if (prepared.scope == QStringLiteral("file_node")) {
        prepared.assistantRequest = buildFileRequest(context, nodeData);
    } else {
        prepared.assistantRequest = buildComponentRequest(context, nodeData);
    }
    prepared.networkRequest =
        ai::buildDeepSeekChatCompletionsRequest(loadResult.config);
    prepared.payload = ai::buildDeepSeekChatCompletionsPayload(
        loadResult.config, prepared.assistantRequest);
    return prepared;
}

AiPreparedRequest AiService::prepareProjectRequest(
    const AiRequestContext& context,
    const QVariantMap& systemContextData,
    const QVariantList& systemContextCards,
    const QVariantList& capabilityNodeItems) {
    AiPreparedRequest prepared;
    prepared.scope = QStringLiteral("system_context");
    prepared.availability = inspectAvailability();

    if (systemContextData.isEmpty() || capabilityNodeItems.isEmpty()) {
        prepared.failureStatusMessage =
            QStringLiteral("先完成一次项目分析，再生成项目导览。");
        return prepared;
    }

    prepared.targetName =
        systemContextData.value(QStringLiteral("title")).toString().trimmed();
    if (prepared.targetName.isEmpty()) {
        prepared.targetName = QStringLiteral("%1 / 项目导览")
                                  .arg(projectNameFromRootPath(context.projectRootPath));
    }
    if (!prepared.availability.available) {
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    if (!loadResult.hasConfig() || !loadResult.config.isUsable()) {
        prepared.availability.available = false;
        prepared.availability.setupMessage = buildAiSetupMessage(loadResult);
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    prepared.ready = true;
    prepared.pendingStatusMessage =
        QStringLiteral("正在生成项目导览...");
    prepared.assistantRequest = buildProjectRequest(
        context, systemContextData, systemContextCards, capabilityNodeItems);
    prepared.networkRequest =
        ai::buildDeepSeekChatCompletionsRequest(loadResult.config);
    prepared.payload = ai::buildDeepSeekChatCompletionsPayload(
        loadResult.config, prepared.assistantRequest);
    return prepared;
}

AiPreparedRequest AiService::prepareProjectOverviewRequest(
    const AiRequestContext& context,
    const QVariantMap& systemContextData,
    const QVariantList& systemContextCards,
    const QVariantList& capabilityNodeItems) {
    AiPreparedRequest prepared;
    prepared.scope = QStringLiteral("project_overview");
    prepared.availability = inspectAvailability();

    if (context.projectRootPath.trimmed().isEmpty()) {
        prepared.failureStatusMessage =
            QStringLiteral("先绑定项目目录，再生成项目总览。");
        return prepared;
    }

    prepared.targetName = QStringLiteral("%1 / 项目总览")
                              .arg(projectNameFromRootPath(context.projectRootPath));
    if (!prepared.availability.available) {
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    if (!loadResult.hasConfig() || !loadResult.config.isUsable()) {
        prepared.availability.available = false;
        prepared.availability.setupMessage = buildAiSetupMessage(loadResult);
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    prepared.ready = true;
    prepared.pendingStatusMessage =
        QStringLiteral("正在生成项目总览...");
    prepared.assistantRequest = buildProjectOverviewRequest(
        context, systemContextData, systemContextCards, capabilityNodeItems);
    prepared.networkRequest =
        ai::buildDeepSeekChatCompletionsRequest(loadResult.config);
    prepared.payload = ai::buildDeepSeekChatCompletionsPayload(
        loadResult.config, prepared.assistantRequest);
    return prepared;
}

AiPreparedRequest AiService::prepareReportRequest(
    const AiRequestContext& context,
    const QVariantMap& systemContextData,
    const QVariantList& systemContextCards,
    const QVariantList& capabilityNodeItems) {
    AiPreparedRequest prepared;
    prepared.scope = QStringLiteral("engineering_report");
    prepared.availability = inspectAvailability();

    if (context.analysisReport.trimmed().isEmpty()) {
        prepared.failureStatusMessage =
            QStringLiteral("先完成一次项目分析，再生成深入阅读建议。");
        return prepared;
    }

    prepared.targetName = QStringLiteral("%1 / 深入阅读建议")
                              .arg(projectNameFromRootPath(context.projectRootPath));
    if (!prepared.availability.available) {
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    if (!loadResult.hasConfig() || !loadResult.config.isUsable()) {
        prepared.availability.available = false;
        prepared.availability.setupMessage = buildAiSetupMessage(loadResult);
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    prepared.ready = true;
    prepared.pendingStatusMessage =
        QStringLiteral("正在生成深入阅读建议...");
    prepared.assistantRequest = buildReportRequest(
        context, systemContextData, systemContextCards, capabilityNodeItems);
    prepared.networkRequest =
        ai::buildDeepSeekChatCompletionsRequest(loadResult.config);
    prepared.payload = ai::buildDeepSeekChatCompletionsPayload(
        loadResult.config, prepared.assistantRequest);
    return prepared;
}

AiReplyState AiService::parseReply(
    const QByteArray& responseBytes,
    const QString& scope,
    const bool hasNetworkError,
    const QString& networkErrorString) {
    AiReplyState state;
    if (hasNetworkError) {
        state.statusMessage =
            QStringLiteral("AI 请求失败：%1").arg(networkErrorString);
        const QString apiMessage = extractApiErrorMessage(responseBytes);
        if (!apiMessage.isEmpty())
            state.statusMessage += QStringLiteral("\n%1").arg(apiMessage);
        return state;
    }

    ai::ArchitectureAssistantInsight insight;
    QString errorMessage;
    QString rawContent;
    if (!ai::parseDeepSeekChatCompletionsResponse(
            responseBytes, &insight, &errorMessage, &rawContent)) {
        state.statusMessage =
            QStringLiteral("AI 已返回结果，但当前内容无法被工具解析：%1")
                .arg(errorMessage);
        return state;
    }

    const QString effectiveSummary =
        !insight.plainSummary.trimmed().isEmpty() ? insight.plainSummary
                                                  : insight.summary;
    QStringList responsibilitySections;
    if (!insight.whyItMatters.trimmed().isEmpty()) {
        responsibilitySections.push_back(
            QStringLiteral("为什么重要：%1").arg(insight.whyItMatters.trimmed()));
    }
    if (!insight.responsibility.trimmed().isEmpty()) {
        responsibilitySections.push_back(
            QStringLiteral("具体职责：%1")
                .arg(insight.responsibility.trimmed()));
    }

    state.hasResult = true;
    state.summary = effectiveSummary;
    state.responsibility = responsibilitySections.join(QStringLiteral("\n\n"));
    state.uncertainty = insight.uncertainty;
    state.collaborators = toVariantStringList(insight.collaborators);
    state.evidence = toVariantStringList(
        mergeQStringLists(insight.evidence, insight.glossary));
    state.nextActions = toVariantStringList(
        mergeQStringLists(insight.whereToStart, insight.nextActions));
    state.statusMessage =
        scope == QStringLiteral("project_overview")
            ? QStringLiteral("AI 已基于 README、工程清单和关键源码生成项目总览。")
            : QStringLiteral("AI 已基于当前证据生成%1。")
                  .arg(publicScopeLabel(scope));
    return state;
}

void AiService::logRequest(const AiPreparedRequest& request,
                           const QString& targetDebug) {
    qDebug().noquote() << "\n========== [AI Request] ==========";
    qDebug().noquote() << "scope:" << request.scope;
    qDebug().noquote() << "target:" << targetDebug;
    qDebug().noquote() << "[user prompt]\n"
                       << ai::deepSeekSavtUserPrompt(request.assistantRequest);
    qDebug().noquote() << "[json payload]\n"
                       << formattedPayloadForDebug(request.payload);
    qDebug().noquote() << "==================================\n";
}

void AiService::logResponse(const QByteArray& responseBytes,
                            const QString& scope) {
    qDebug().noquote() << "\n========== [AI Response] ==========";
    qDebug().noquote() << "scope:"
                       << (scope.isEmpty() ? QStringLiteral("unknown") : scope);
    qDebug().noquote() << "[raw response]\n"
                       << formattedResponseForDebug(responseBytes);
    qDebug().noquote() << "===================================\n";
}

}  // namespace savt::ui
