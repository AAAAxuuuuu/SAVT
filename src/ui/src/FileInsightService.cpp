#include "savt/ui/FileInsightService.h"

#include "savt/core/SyntaxTree.h"
#include "savt/parser/TreeSitterCppParser.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace savt::ui {

namespace {

QStringList toQStringList(const QVariant& value) {
    QStringList items;
    for (const QVariant& entry : value.toList()) {
        const QString cleaned = entry.toString().trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned)) {
            items.push_back(cleaned);
        }
    }
    return items;
}

QVariantList toVariantStringList(const QStringList& values) {
    QVariantList items;
    items.reserve(values.size());
    for (const QString& value : values) {
        const QString cleaned = value.trimmed();
        if (!cleaned.isEmpty()) {
            items.push_back(cleaned);
        }
    }
    return items;
}

QString lowerPath(const QString& path) {
    return QDir::fromNativeSeparators(path).trimmed().toLower();
}

QString lowerSuffix(const QString& path) {
    return QFileInfo(path).suffix().trimmed().toLower();
}

QString lowerFileName(const QString& path) {
    return QFileInfo(path).fileName().trimmed().toLower();
}

bool isCppImplementationSuffix(const QString& suffix) {
    return suffix == QStringLiteral("c") ||
           suffix == QStringLiteral("cc") ||
           suffix == QStringLiteral("cpp") ||
           suffix == QStringLiteral("cxx") ||
           suffix == QStringLiteral("cp") ||
           suffix == QStringLiteral("c++") ||
           suffix == QStringLiteral("ixx") ||
           suffix == QStringLiteral("cppm") ||
           suffix == QStringLiteral("cu") ||
           suffix == QStringLiteral("mm");
}

bool isCppHeaderOrInlineSuffix(const QString& suffix) {
    return suffix == QStringLiteral("h") ||
           suffix == QStringLiteral("hh") ||
           suffix == QStringLiteral("hpp") ||
           suffix == QStringLiteral("hxx") ||
           suffix == QStringLiteral("h++") ||
           suffix == QStringLiteral("ipp") ||
           suffix == QStringLiteral("inl") ||
           suffix == QStringLiteral("tpp") ||
           suffix == QStringLiteral("tcc") ||
           suffix == QStringLiteral("txx") ||
           suffix == QStringLiteral("ii") ||
           suffix == QStringLiteral("cuh");
}

bool isCppFamilySuffix(const QString& suffix) {
    return isCppImplementationSuffix(suffix) || isCppHeaderOrInlineSuffix(suffix);
}

bool isQtOrCppProjectSuffix(const QString& suffix) {
    return suffix == QStringLiteral("qrc") ||
           suffix == QStringLiteral("ui") ||
           suffix == QStringLiteral("rc") ||
           suffix == QStringLiteral("pro") ||
           suffix == QStringLiteral("pri") ||
           suffix == QStringLiteral("qbs") ||
           suffix == QStringLiteral("cmake") ||
           suffix == QStringLiteral("def");
}

bool isSupportedSpecialFileName(const QString& fileName) {
    return fileName == QStringLiteral("cmakelists.txt") ||
           fileName == QStringLiteral("qmldir");
}

bool isSupportedFileSuffix(const QString& suffix) {
    static const QStringList suffixes = {
        QStringLiteral("c"),     QStringLiteral("cc"),   QStringLiteral("cpp"),
        QStringLiteral("cxx"),   QStringLiteral("cp"),   QStringLiteral("c++"),
        QStringLiteral("ixx"),   QStringLiteral("cppm"), QStringLiteral("cu"),
        QStringLiteral("mm"),    QStringLiteral("h"),    QStringLiteral("hh"),
        QStringLiteral("hpp"),   QStringLiteral("hxx"),  QStringLiteral("h++"),
        QStringLiteral("ipp"),   QStringLiteral("inl"),  QStringLiteral("tpp"),
        QStringLiteral("tcc"),   QStringLiteral("txx"),  QStringLiteral("ii"),
        QStringLiteral("cuh"),   QStringLiteral("qml"),
        QStringLiteral("js"),    QStringLiteral("mjs"),  QStringLiteral("cjs"),
        QStringLiteral("ts"),    QStringLiteral("tsx"),  QStringLiteral("mts"),
        QStringLiteral("cts"),   QStringLiteral("py"),   QStringLiteral("java"),
        QStringLiteral("kt"),    QStringLiteral("kts"),  QStringLiteral("go"),
        QStringLiteral("rs"),    QStringLiteral("json"), QStringLiteral("yaml"),
        QStringLiteral("yml"),   QStringLiteral("toml"), QStringLiteral("xml"),
        QStringLiteral("html"),  QStringLiteral("htm"),  QStringLiteral("css"),
        QStringLiteral("scss"),  QStringLiteral("md"),   QStringLiteral("qrc"),
        QStringLiteral("ui"),    QStringLiteral("rc"),   QStringLiteral("pro"),
        QStringLiteral("pri"),   QStringLiteral("qbs"),  QStringLiteral("cmake"),
        QStringLiteral("s"),     QStringLiteral("asm"),  QStringLiteral("def")
    };
    return suffixes.contains(suffix);
}

bool looksLikeSourceFileName(const QString& value) {
    return isSupportedFileSuffix(lowerSuffix(value)) ||
           isSupportedSpecialFileName(lowerFileName(value));
}

QStringList sourceFileCandidates(const QVariantMap& nodeData) {
    QStringList candidates;
    const QString primaryPath =
        nodeData.value(QStringLiteral("primaryFilePath")).toString().trimmed();
    if (!primaryPath.isEmpty()) {
        candidates.push_back(primaryPath);
    }

    for (const QString& filePath : toQStringList(nodeData.value(QStringLiteral("exampleFiles")))) {
        if (!candidates.contains(filePath)) {
            candidates.push_back(filePath);
        }
    }

    const QVariantMap evidence = nodeData.value(QStringLiteral("evidence")).toMap();
    const QStringList evidenceFiles =
        toQStringList(evidence.value(QStringLiteral("sourceFiles")));
    for (const QString& filePath : evidenceFiles) {
        if (!candidates.contains(filePath)) {
            candidates.push_back(filePath);
        }
    }

    const QStringList directSourceFiles =
        toQStringList(nodeData.value(QStringLiteral("sourceFiles")));
    for (const QString& filePath : directSourceFiles) {
        if (!candidates.contains(filePath)) {
            candidates.push_back(filePath);
        }
    }

    return candidates;
}

QString selectPrimaryFilePath(const QVariantMap& nodeData) {
    const QString explicitPath =
        nodeData.value(QStringLiteral("primaryFilePath")).toString().trimmed();
    if (!explicitPath.isEmpty()) {
        return explicitPath;
    }

    const QStringList candidates = sourceFileCandidates(nodeData);
    if (candidates.isEmpty()) {
        return {};
    }

    const QString nodeName = nodeData.value(QStringLiteral("name")).toString().trimmed();
    const QString nodeFileName = QFileInfo(nodeName).fileName().toLower();
    if (looksLikeSourceFileName(nodeName) && !nodeFileName.isEmpty()) {
        for (const QString& candidate : candidates) {
            const QString candidateFileName = QFileInfo(candidate).fileName().toLower();
            if (candidateFileName == nodeFileName) {
                return candidate;
            }
        }
    }

    return candidates.size() == 1 ? candidates.constFirst() : QString();
}

QString joinedPreview(const QStringList& values,
                      const int maxItems,
                      const QString& separator = QStringLiteral("、")) {
    if (values.isEmpty()) {
        return {};
    }
    QStringList limited = values;
    if (limited.size() > maxItems) {
        limited = limited.mid(0, maxItems);
    }
    return limited.join(separator);
}

bool isGenericSummary(const QString& text) {
    const QString cleaned = text.trimmed();
    if (cleaned.isEmpty()) {
        return true;
    }

    static const QStringList genericTokens = {
        QStringLiteral("当前能力域"),
        QStringLiteral("当前能力块"),
        QStringLiteral("内部实现职责"),
        QStringLiteral("共享支撑能力"),
        QStringLiteral("入口动作"),
        QStringLiteral("支撑性"),
        QStringLiteral("当前模块"),
        QStringLiteral("当前对象")
    };
    for (const QString& token : genericTokens) {
        if (cleaned.contains(token)) {
            return true;
        }
    }
    return false;
}

QString detectLanguageLabel(const QString& filePath) {
    const QString suffix = lowerSuffix(filePath);
    const QString fileName = lowerFileName(filePath);
    if (fileName == QStringLiteral("cmakelists.txt") || suffix == QStringLiteral("cmake")) {
        return QStringLiteral("CMake");
    }
    if (fileName == QStringLiteral("qmldir")) {
        return QStringLiteral("QML Module");
    }
    if (suffix == QStringLiteral("ixx") || suffix == QStringLiteral("cppm")) {
        return QStringLiteral("C++ Module");
    }
    if (suffix == QStringLiteral("cu")) {
        return QStringLiteral("CUDA C++");
    }
    if (suffix == QStringLiteral("mm")) {
        return QStringLiteral("Objective-C++");
    }
    if (isCppImplementationSuffix(suffix)) {
        return QStringLiteral("C++");
    }
    if (suffix == QStringLiteral("cuh")) {
        return QStringLiteral("CUDA Header");
    }
    if (isCppHeaderOrInlineSuffix(suffix)) {
        return QStringLiteral("C/C++ Header");
    }
    if (suffix == QStringLiteral("qml")) {
        return QStringLiteral("QML");
    }
    if (suffix == QStringLiteral("qrc")) {
        return QStringLiteral("Qt Resource");
    }
    if (suffix == QStringLiteral("ui")) {
        return QStringLiteral("Qt Designer UI");
    }
    if (suffix == QStringLiteral("pro") || suffix == QStringLiteral("pri")) {
        return QStringLiteral("qmake");
    }
    if (suffix == QStringLiteral("qbs")) {
        return QStringLiteral("Qbs");
    }
    if (suffix == QStringLiteral("rc")) {
        return QStringLiteral("Resource Script");
    }
    if (suffix == QStringLiteral("s") || suffix == QStringLiteral("asm")) {
        return QStringLiteral("Assembly");
    }
    if (suffix == QStringLiteral("def")) {
        return QStringLiteral("Module Definition");
    }
    if (suffix == QStringLiteral("js") || suffix == QStringLiteral("mjs") ||
        suffix == QStringLiteral("cjs")) {
        return QStringLiteral("JavaScript");
    }
    if (suffix == QStringLiteral("ts") || suffix == QStringLiteral("tsx") ||
        suffix == QStringLiteral("mts") || suffix == QStringLiteral("cts")) {
        return QStringLiteral("TypeScript");
    }
    if (suffix == QStringLiteral("py")) {
        return QStringLiteral("Python");
    }
    if (suffix == QStringLiteral("java")) {
        return QStringLiteral("Java");
    }
    if (suffix == QStringLiteral("kt") || suffix == QStringLiteral("kts")) {
        return QStringLiteral("Kotlin");
    }
    if (suffix == QStringLiteral("go")) {
        return QStringLiteral("Go");
    }
    if (suffix == QStringLiteral("rs")) {
        return QStringLiteral("Rust");
    }
    if (suffix == QStringLiteral("json")) {
        return QStringLiteral("JSON");
    }
    if (suffix == QStringLiteral("yaml") || suffix == QStringLiteral("yml")) {
        return QStringLiteral("YAML");
    }
    if (suffix == QStringLiteral("toml")) {
        return QStringLiteral("TOML");
    }
    if (suffix == QStringLiteral("xml")) {
        return QStringLiteral("XML");
    }
    if (suffix == QStringLiteral("html") || suffix == QStringLiteral("htm")) {
        return QStringLiteral("HTML");
    }
    if (suffix == QStringLiteral("css") || suffix == QStringLiteral("scss")) {
        return QStringLiteral("CSS");
    }
    if (suffix == QStringLiteral("md")) {
        return QStringLiteral("Markdown");
    }
    return suffix.isEmpty() ? QStringLiteral("Unknown") : suffix.toUpper();
}

QString detectCategoryLabel(const QString& filePath) {
    const QString suffix = lowerSuffix(filePath);
    const QString lowered = lowerPath(filePath);
    const QString fileName = lowerFileName(filePath);
    if (lowered.contains(QStringLiteral("/test")) ||
        lowered.contains(QStringLiteral("/spec")) ||
        lowered.contains(QStringLiteral("/mock")) ||
        lowered.contains(QStringLiteral("/fixture"))) {
        return QStringLiteral("测试/样例");
    }
    if (fileName == QStringLiteral("cmakelists.txt") ||
        fileName == QStringLiteral("qmldir") ||
        isQtOrCppProjectSuffix(suffix)) {
        return QStringLiteral("工程/资源配置");
    }
    if (suffix == QStringLiteral("json") || suffix == QStringLiteral("yaml") ||
        suffix == QStringLiteral("yml") || suffix == QStringLiteral("toml") ||
        suffix == QStringLiteral("ini") || suffix == QStringLiteral("xml")) {
        return QStringLiteral("配置/数据");
    }
    if (suffix == QStringLiteral("qml") || suffix == QStringLiteral("html") ||
        suffix == QStringLiteral("htm") || suffix == QStringLiteral("css") ||
        suffix == QStringLiteral("scss")) {
        return QStringLiteral("界面/样式");
    }
    if (suffix == QStringLiteral("md")) {
        return QStringLiteral("文档");
    }
    if (lowered.contains(QStringLiteral("/script")) ||
        lowered.contains(QStringLiteral("/tool")) ||
        lowered.contains(QStringLiteral("/build")) ||
        lowered.contains(QStringLiteral("/ci"))) {
        return QStringLiteral("脚本/工具");
    }
    return QStringLiteral("源码文件");
}

bool looksLikeImportLine(const QString& trimmedLine) {
    const QString lowered = trimmedLine.toLower();
    return lowered.startsWith(QStringLiteral("#include")) ||
           lowered.startsWith(QStringLiteral("import ")) ||
           lowered.startsWith(QStringLiteral("from ")) ||
           lowered.contains(QStringLiteral(" require(")) ||
           (lowered.startsWith(QStringLiteral("const ")) &&
            lowered.contains(QStringLiteral("require("))) ||
           (lowered.startsWith(QStringLiteral("let ")) &&
            lowered.contains(QStringLiteral("require("))) ||
           (lowered.startsWith(QStringLiteral("var ")) &&
            lowered.contains(QStringLiteral("require("))) ||
           lowered.startsWith(QStringLiteral("@import")) ||
           lowered.startsWith(QStringLiteral("<script "));
}

QStringList collectImportClues(const QStringList& lines, const int maxItems) {
    QStringList items;
    for (const QString& rawLine : lines) {
        const QString trimmed = rawLine.trimmed();
        if (trimmed.isEmpty() || !looksLikeImportLine(trimmed)) {
            continue;
        }
        if (!items.contains(trimmed)) {
            items.push_back(trimmed);
        }
        if (items.size() >= maxItems) {
            break;
        }
    }
    return items;
}

bool looksLikeDeclarationLine(const QString& trimmedLine) {
    const QString lowered = trimmedLine.toLower();
    if (lowered.startsWith(QStringLiteral("class ")) ||
        lowered.startsWith(QStringLiteral("struct ")) ||
        lowered.startsWith(QStringLiteral("enum ")) ||
        lowered.startsWith(QStringLiteral("namespace ")) ||
        lowered.startsWith(QStringLiteral("interface ")) ||
        lowered.startsWith(QStringLiteral("type ")) ||
        lowered.startsWith(QStringLiteral("def ")) ||
        lowered.startsWith(QStringLiteral("func ")) ||
        lowered.startsWith(QStringLiteral("function ")) ||
        lowered.startsWith(QStringLiteral("export function ")) ||
        lowered.startsWith(QStringLiteral("export default ")) ||
        lowered.startsWith(QStringLiteral("property ")) ||
        lowered.startsWith(QStringLiteral("signal "))) {
        return true;
    }

    if ((lowered.startsWith(QStringLiteral("const ")) ||
         lowered.startsWith(QStringLiteral("let ")) ||
         lowered.startsWith(QStringLiteral("var ")) ||
         lowered.startsWith(QStringLiteral("readonly property "))) &&
        lowered.contains(QStringLiteral("="))) {
        return true;
    }

    if (!trimmedLine.contains(QLatin1Char('(')) ||
        !trimmedLine.contains(QLatin1Char(')'))) {
        return false;
    }

    static const QStringList controlTokens = {
        QStringLiteral("if "), QStringLiteral("for "), QStringLiteral("while "),
        QStringLiteral("switch "), QStringLiteral("catch "), QStringLiteral("return "),
        QStringLiteral("else "), QStringLiteral("case ")
    };
    for (const QString& token : controlTokens) {
        if (lowered.startsWith(token)) {
            return false;
        }
    }

    return trimmedLine.contains(QLatin1Char('{')) ||
           trimmedLine.endsWith(QLatin1Char(';'));
}

QStringList collectDeclarationClues(const QVariantMap& nodeData,
                                    const QStringList& lines,
                                    const int maxItems) {
    QStringList items = toQStringList(nodeData.value(QStringLiteral("topSymbols")));
    if (items.size() >= maxItems) {
        items = items.mid(0, maxItems);
        return items;
    }

    for (const QString& rawLine : lines) {
        const QString trimmed = rawLine.trimmed();
        if (trimmed.isEmpty() || !looksLikeDeclarationLine(trimmed)) {
            continue;
        }
        if (!items.contains(trimmed)) {
            items.push_back(trimmed);
        }
        if (items.size() >= maxItems) {
            break;
        }
    }
    return items;
}

QStringList collectBehaviorSignals(const QString& normalizedText,
                                   const QString& normalizedPath,
                                   const QString& categoryLabel) {
    QStringList items;
    auto appendSignal = [&items](const QString& signal) {
        if (!signal.isEmpty() && !items.contains(signal)) {
            items.push_back(signal);
        }
    };

    if (normalizedText.contains(QStringLiteral("fetch(")) ||
        normalizedText.contains(QStringLiteral("axios")) ||
        normalizedText.contains(QStringLiteral("qnetworkaccessmanager")) ||
        normalizedText.contains(QStringLiteral("http"))) {
        appendSignal(QStringLiteral("包含外部请求或数据拉取线索"));
    }
    if (normalizedText.contains(QStringLiteral("render")) ||
        normalizedText.contains(QStringLiteral("draw")) ||
        normalizedText.contains(QStringLiteral("map")) ||
        normalizedText.contains(QStringLiteral("layer")) ||
        normalizedText.contains(QStringLiteral("chart"))) {
        appendSignal(QStringLiteral("包含界面绘制或可视化处理线索"));
    }
    if (normalizedText.contains(QStringLiteral("click")) ||
        normalizedText.contains(QStringLiteral("onclick")) ||
        normalizedText.contains(QStringLiteral("signal")) ||
        normalizedText.contains(QStringLiteral("emit")) ||
        normalizedText.contains(QStringLiteral("connect("))) {
        appendSignal(QStringLiteral("包含事件触发或交互响应线索"));
    }
    if (normalizedText.contains(QStringLiteral("json")) ||
        normalizedText.contains(QStringLiteral("parse")) ||
        normalizedText.contains(QStringLiteral("serialize")) ||
        normalizedText.contains(QStringLiteral("decode")) ||
        normalizedText.contains(QStringLiteral("encode"))) {
        appendSignal(QStringLiteral("包含数据转换或序列化线索"));
    }
    if (normalizedText.contains(QStringLiteral("cache")) ||
        normalizedText.contains(QStringLiteral("store")) ||
        normalizedText.contains(QStringLiteral("state")) ||
        normalizedText.contains(QStringLiteral("model"))) {
        appendSignal(QStringLiteral("包含状态、缓存或模型组织线索"));
    }
    if (normalizedPath.contains(QStringLiteral("/config")) ||
        categoryLabel == QStringLiteral("配置/数据")) {
        appendSignal(QStringLiteral("更像提供配置或静态数据，而不是承接复杂流程"));
    }
    return items;
}

QString inferRoleLabel(const QString& filePath,
                       const QString& categoryLabel,
                       const QString& nodeRole,
                       const QStringList& importClues,
                       const QStringList& declarationClues,
                       const QStringList& behaviorSignals) {
    const QString lowered = lowerPath(filePath);
    const QString roleLowered = nodeRole.trimmed().toLower();

    if (categoryLabel == QStringLiteral("测试/样例")) {
        return QStringLiteral("测试/验证");
    }
    if (categoryLabel == QStringLiteral("配置/数据")) {
        return QStringLiteral("配置/数据承载");
    }
    if (lowered.contains(QStringLiteral("/main.")) ||
        lowered.endsWith(QStringLiteral("/main.cpp")) ||
        lowered.endsWith(QStringLiteral("/main.js")) ||
        lowered.contains(QStringLiteral("bootstrap")) ||
        lowered.contains(QStringLiteral("startup")) ||
        lowered.contains(QStringLiteral("/app.")) ||
        lowered.contains(QStringLiteral("/server."))) {
        return QStringLiteral("入口/装配");
    }
    if (lowered.contains(QStringLiteral("/route")) ||
        lowered.contains(QStringLiteral("/controller")) ||
        lowered.contains(QStringLiteral("/handler")) ||
        lowered.contains(QStringLiteral("/event"))) {
        return QStringLiteral("请求/事件分发");
    }
    if (lowered.contains(QStringLiteral("/view")) ||
        lowered.contains(QStringLiteral("/page")) ||
        lowered.contains(QStringLiteral("/ui")) ||
        lowered.contains(QStringLiteral("/widget")) ||
        lowered.contains(QStringLiteral("/component")) ||
        categoryLabel == QStringLiteral("界面/样式")) {
        return QStringLiteral("界面/交互");
    }
    if (lowered.contains(QStringLiteral("/service")) ||
        lowered.contains(QStringLiteral("/facade")) ||
        lowered.contains(QStringLiteral("/manager")) ||
        lowered.contains(QStringLiteral("/store")) ||
        roleLowered == QStringLiteral("service")) {
        return QStringLiteral("服务/编排");
    }
    if (lowered.contains(QStringLiteral("/model")) ||
        lowered.contains(QStringLiteral("/entity")) ||
        lowered.contains(QStringLiteral("/schema")) ||
        lowered.contains(QStringLiteral("/type")) ||
        roleLowered == QStringLiteral("core_model")) {
        return QStringLiteral("数据模型");
    }
    if (lowered.contains(QStringLiteral("/util")) ||
        lowered.contains(QStringLiteral("/helper")) ||
        lowered.contains(QStringLiteral("/common")) ||
        lowered.contains(QStringLiteral("/tool")) ||
        roleLowered == QStringLiteral("tooling")) {
        return QStringLiteral("工具/支撑");
    }
    if (behaviorSignals.contains(QStringLiteral("包含界面绘制或可视化处理线索"))) {
        return QStringLiteral("界面/可视化");
    }
    if (behaviorSignals.contains(QStringLiteral("包含外部请求或数据拉取线索"))) {
        return QStringLiteral("数据获取/编排");
    }
    if (!importClues.isEmpty() && declarationClues.isEmpty()) {
        return QStringLiteral("装配/转接");
    }
    return QStringLiteral("具体实现");
}

QStringList buildReadingHints(const QStringList& importClues,
                              const QStringList& declarationClues,
                              const QStringList& behaviorSignals,
                              const QStringList& collaborators) {
    QStringList items;
    if (!importClues.isEmpty()) {
        items.push_back(
            QStringLiteral("先看文件头部的导入/依赖，确认它需要和谁协作：%1。")
                .arg(joinedPreview(importClues, 2, QStringLiteral("；"))));
    }
    if (!declarationClues.isEmpty()) {
        items.push_back(
            QStringLiteral("再看这些声明或主符号，基本能定位文件的主入口：%1。")
                .arg(joinedPreview(declarationClues, 3)));
    }
    if (!behaviorSignals.isEmpty()) {
        items.push_back(
            QStringLiteral("阅读时重点核对这些行为线索是否真的成立：%1。")
                .arg(joinedPreview(behaviorSignals, 3)));
    }
    if (!collaborators.isEmpty()) {
        items.push_back(
            QStringLiteral("最后回到组件关系，确认它和 %1 的衔接边界。")
                .arg(joinedPreview(collaborators, 3)));
    }
    if (items.isEmpty()) {
        items.push_back(QStringLiteral("先从文件顶部开始读，优先找导入、初始化和对外暴露的入口。"));
    }
    return items;
}

QString buildSummary(const QString& fileName,
                     const QString& filePath,
                     const QString& folderPath,
                     const QString& languageLabel,
                     const QString& roleLabel,
                     const QString& fallbackSummary,
                     const QStringList& importClues,
                     const QStringList& declarationClues,
                     const QStringList& behaviorSignals,
                     const QStringList& collaborators) {
    QStringList parts;
    parts.push_back(
        QStringLiteral("`%1` 是 `%2` 下的一个 %3 文件，当前更像承担「%4」这一类职责。")
            .arg(fileName, filePath, languageLabel, roleLabel));

    if (!fallbackSummary.trimmed().isEmpty() && !isGenericSummary(fallbackSummary)) {
        parts.push_back(fallbackSummary.trimmed());
    } else if (!behaviorSignals.isEmpty()) {
        parts.push_back(
            QStringLiteral("从静态线索看，它主要围绕 %1 展开。")
                .arg(joinedPreview(behaviorSignals, 2)));
    } else if (!folderPath.trimmed().isEmpty()) {
        parts.push_back(
            QStringLiteral("它位于 `%1` 目录下，通常表示这部分实现会和同目录职责保持一致。")
                .arg(folderPath));
    }

    if (!declarationClues.isEmpty()) {
        parts.push_back(
            QStringLiteral("当前最值得先看的声明/符号有：%1。")
                .arg(joinedPreview(declarationClues, 3)));
    }
    if (!importClues.isEmpty()) {
        parts.push_back(
            QStringLiteral("它直接暴露出的依赖线索包括：%1。")
                .arg(joinedPreview(importClues, 2, QStringLiteral("；"))));
    }
    if (!collaborators.isEmpty()) {
        parts.push_back(
            QStringLiteral("在当前组件图里，它还需要和 %1 这些邻接节点一起看，才能判断边界是否合理。")
                .arg(joinedPreview(collaborators, 3)));
    }

    return parts.join(QStringLiteral(" "));
}

QString normalizedSymbolHint(QString value) {
    value = value.trimmed();
    const int parenIndex = value.indexOf(QLatin1Char('('));
    if (parenIndex >= 0) {
        value = value.left(parenIndex).trimmed();
    }
    const int scopeIndex = value.lastIndexOf(QStringLiteral("::"));
    if (scopeIndex >= 0) {
        value = value.mid(scopeIndex + 2).trimmed();
    }
    const QStringList parts = value.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (!parts.isEmpty()) {
        value = parts.constLast();
    }

    while (!value.isEmpty() &&
           !value.front().isLetterOrNumber() &&
           value.front() != QLatin1Char('_')) {
        value.remove(0, 1);
    }
    while (!value.isEmpty() &&
           !value.back().isLetterOrNumber() &&
           value.back() != QLatin1Char('_')) {
        value.chop(1);
    }
    return value.size() >= 3 ? value.toLower() : QString();
}

QStringList previewSymbolHints(const QVariantMap& nodeData) {
    QStringList hints;
    auto addHint = [&hints](const QString& value) {
        const QString normalized = normalizedSymbolHint(value);
        if (!normalized.isEmpty() && !hints.contains(normalized)) {
            hints.push_back(normalized);
        }
    };

    for (const QString& symbol : toQStringList(nodeData.value(QStringLiteral("topSymbols")))) {
        addHint(symbol);
    }
    const QVariantMap evidence = nodeData.value(QStringLiteral("evidence")).toMap();
    for (const QString& symbol : toQStringList(evidence.value(QStringLiteral("symbols")))) {
        addHint(symbol);
    }
    return hints;
}

QString compactIdentifier(QString value) {
    QString compact;
    compact.reserve(value.size());
    for (const QChar character : value) {
        if (character.isLetterOrNumber() || character == QLatin1Char('_')) {
            compact.push_back(character.toLower());
        }
    }
    return compact;
}

QStringList splitIdentifierTokens(QString value) {
    value.replace(QRegularExpression(QStringLiteral("([a-z0-9])([A-Z])")),
                  QStringLiteral("\\1 \\2"));
    value.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_]+")),
                  QStringLiteral(" "));
    QStringList tokens;
    for (const QString& token : value.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        const QString cleaned = token.trimmed().toLower();
        if (cleaned.size() >= 3 && !tokens.contains(cleaned)) {
            tokens.push_back(cleaned);
        }
    }
    return tokens;
}

QString previewPrimaryFilePath(const QVariantMap& nodeData) {
    const QString directPath = nodeData.value(QStringLiteral("filePath")).toString().trimmed();
    if (!directPath.isEmpty()) {
        return directPath;
    }
    const QString primaryPath =
        nodeData.value(QStringLiteral("primaryFilePath")).toString().trimmed();
    if (!primaryPath.isEmpty()) {
        return primaryPath;
    }
    const QStringList candidates = sourceFileCandidates(nodeData);
    return candidates.isEmpty() ? QString() : candidates.constFirst();
}

QStringList fileIdentityHints(const QVariantMap& nodeData) {
    QStringList hints;
    const QString fileStem = QFileInfo(previewPrimaryFilePath(nodeData)).completeBaseName();
    const QString compactStem = compactIdentifier(fileStem);
    if (compactStem.size() >= 4) {
        hints.push_back(compactStem);
    }
    for (const QString& token : splitIdentifierTokens(fileStem)) {
        if (!hints.contains(token)) {
            hints.push_back(token);
        }
    }
    return hints;
}

bool isTriviaPreviewLine(const QString& trimmedLine) {
    const QString lowered = trimmedLine.toLower();
    if (trimmedLine.isEmpty()) {
        return true;
    }
    if (trimmedLine == QLatin1String("{") ||
        trimmedLine == QLatin1String("}") ||
        trimmedLine == QLatin1String("};")) {
        return true;
    }
    return lowered.startsWith(QStringLiteral("#include")) ||
           lowered.startsWith(QStringLiteral("import ")) ||
           lowered.startsWith(QStringLiteral("export module ")) ||
           lowered.startsWith(QStringLiteral("module;")) ||
           lowered.startsWith(QStringLiteral("from ")) ||
           lowered.startsWith(QStringLiteral("@import")) ||
           lowered.startsWith(QStringLiteral("using namespace ")) ||
           (lowered.startsWith(QStringLiteral("namespace ")) &&
            trimmedLine.endsWith(QLatin1Char('{')));
}

bool startsWithAnyToken(const QString& loweredLine, const QStringList& tokens) {
    for (const QString& token : tokens) {
        if (loweredLine.startsWith(token)) {
            return true;
        }
    }
    return false;
}

bool containsAnyToken(const QString& loweredLine, const QStringList& tokens) {
    for (const QString& token : tokens) {
        if (loweredLine.contains(token)) {
            return true;
        }
    }
    return false;
}

bool looksLikeImplementationAnchor(const QString& trimmedLine) {
    const QString lowered = trimmedLine.toLower();
    if (startsWithAnyToken(lowered, {
            QStringLiteral("class "),
            QStringLiteral("struct "),
            QStringLiteral("enum "),
            QStringLiteral("interface "),
            QStringLiteral("def "),
            QStringLiteral("async def "),
            QStringLiteral("func "),
            QStringLiteral("function "),
            QStringLiteral("async function "),
            QStringLiteral("export function "),
            QStringLiteral("export async function "),
            QStringLiteral("export default "),
            QStringLiteral("public "),
            QStringLiteral("private "),
            QStringLiteral("protected "),
            QStringLiteral("override "),
            QStringLiteral("signal "),
            QStringLiteral("property "),
            QStringLiteral("component ")
        })) {
        return true;
    }
    if (trimmedLine.contains(QStringLiteral("::")) &&
        trimmedLine.contains(QLatin1Char('(')) &&
        trimmedLine.contains(QLatin1Char(')'))) {
        return true;
    }
    if (trimmedLine.contains(QLatin1Char('(')) &&
        trimmedLine.contains(QLatin1Char(')')) &&
        trimmedLine.contains(QLatin1Char('{')) &&
        !lowered.startsWith(QStringLiteral("if ")) &&
        !lowered.startsWith(QStringLiteral("for ")) &&
        !lowered.startsWith(QStringLiteral("while ")) &&
        !lowered.startsWith(QStringLiteral("switch ")) &&
        !lowered.startsWith(QStringLiteral("return ")) &&
        !lowered.startsWith(QStringLiteral("const ")) &&
        !lowered.startsWith(QStringLiteral("auto "))) {
        return true;
    }
    return false;
}

QString declarationNameFromLine(const QString& trimmedLine) {
    QString value = trimmedLine;
    const int parenIndex = value.indexOf(QLatin1Char('('));
    if (parenIndex >= 0) {
        value = value.left(parenIndex).trimmed();
    } else {
        const int equalsIndex = value.indexOf(QLatin1Char('='));
        if (equalsIndex >= 0) {
            value = value.left(equalsIndex).trimmed();
        }
    }

    const int scopeIndex = value.lastIndexOf(QStringLiteral("::"));
    if (scopeIndex >= 0) {
        value = value.mid(scopeIndex + 2).trimmed();
    }

    value.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_]+")),
                  QStringLiteral(" "));
    const QStringList parts = value.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    return parts.isEmpty() ? QString() : parts.constLast();
}

bool isHelperLikeDeclarationName(const QString& declarationName) {
    const QString lowered = declarationName.toLower();
    return lowered.contains(QStringLiteral("helper")) ||
           lowered.contains(QStringLiteral("candidate")) ||
           lowered.contains(QStringLiteral("normalize")) ||
           lowered.contains(QStringLiteral("deduplicate")) ||
           lowered.contains(QStringLiteral("trim")) ||
           lowered.contains(QStringLiteral("trimmed")) ||
           lowered.contains(QStringLiteral("display")) ||
           lowered.contains(QStringLiteral("label")) ||
           lowered.contains(QStringLiteral("enabled")) ||
           lowered.contains(QStringLiteral("isempty")) ||
           lowered.contains(QStringLiteral("tojson")) ||
           lowered.contains(QStringLiteral("tostring")) ||
           lowered.contains(QStringLiteral("looks")) ||
           lowered.contains(QStringLiteral("fallback"));
}

bool lineMatchesFileIdentity(const QString& loweredLine, const QStringList& identityHints) {
    if (identityHints.isEmpty()) {
        return false;
    }

    const QString compactLine = compactIdentifier(loweredLine);
    int tokenMatches = 0;
    for (const QString& hint : identityHints) {
        if (hint.size() >= 6 && compactLine.contains(hint)) {
            return true;
        }
        if (hint.size() >= 3 && compactLine.contains(hint)) {
            ++tokenMatches;
        }
    }
    return tokenMatches >= 2;
}

int callDensityScore(const QString& trimmedLine);

QSet<int> anonymousNamespaceLineIndexes(const QStringList& lines) {
    QSet<int> indexes;
    bool inAnonymousNamespace = false;
    int braceDepth = 0;
    for (int index = 0; index < lines.size(); ++index) {
        const QString line = lines.at(index);
        const QString trimmed = line.trimmed();
        const QString lowered = trimmed.toLower();
        if (!inAnonymousNamespace &&
            (trimmed == QStringLiteral("namespace {") ||
             lowered.startsWith(QStringLiteral("namespace detail")) ||
             lowered.startsWith(QStringLiteral("namespace internal")) ||
             lowered.startsWith(QStringLiteral("namespace impl")) ||
             lowered.startsWith(QStringLiteral("namespace helper")) ||
             lowered.contains(QStringLiteral("::detail")) ||
             lowered.contains(QStringLiteral("::internal")) ||
             lowered.contains(QStringLiteral("::impl")))) {
            inAnonymousNamespace = true;
            braceDepth = 0;
        }

        if (inAnonymousNamespace) {
            indexes.insert(index);
        }

        if (inAnonymousNamespace) {
            for (const QChar character : line) {
                if (character == QLatin1Char('{')) {
                    ++braceDepth;
                } else if (character == QLatin1Char('}')) {
                    --braceDepth;
                }
            }
            if (braceDepth <= 0) {
                inAnonymousNamespace = false;
            }
        }
    }
    return indexes;
}

int surroundingBlockComplexityScore(const QStringList& lines, const int lineIndex) {
    int score = 0;
    int meaningfulLines = 0;
    const int end = std::min(static_cast<int>(lines.size()) - 1, lineIndex + 16);
    for (int index = lineIndex; index <= end; ++index) {
        const QString trimmed = lines.at(index).trimmed();
        if (trimmed.isEmpty() || isTriviaPreviewLine(trimmed)) {
            continue;
        }
        ++meaningfulLines;
        score += std::min(2 + callDensityScore(trimmed), 14);
        const QString lowered = trimmed.toLower();
        if (startsWithAnyToken(lowered, {
                QStringLiteral("if "),
                QStringLiteral("for "),
                QStringLiteral("while "),
                QStringLiteral("switch "),
                QStringLiteral("case "),
                QStringLiteral("try "),
                QStringLiteral("catch ")
            })) {
            score += 6;
        }
        if (lowered.contains(QStringLiteral(".insert")) ||
            lowered.contains(QStringLiteral(".push")) ||
            lowered.contains(QStringLiteral(".emplace")) ||
            lowered.contains(QStringLiteral("return "))) {
            score += 5;
        }
    }
    if (meaningfulLines <= 3) {
        score -= 18;
    }
    return std::clamp(score, -24, 70);
}

int callDensityScore(const QString& trimmedLine) {
    int score = 0;
    int callLikeCount = 0;
    for (int index = 0; index < trimmedLine.size(); ++index) {
        if (trimmedLine.at(index) == QLatin1Char('(')) {
            ++callLikeCount;
        }
    }
    if (callLikeCount > 0) {
        score += std::min(callLikeCount, 3) * 8;
    }
    return score;
}

int universalStructureSignalScore(const QString& trimmedLine) {
    const QString lowered = trimmedLine.toLower();
    int score = callDensityScore(trimmedLine);

    if (startsWithAnyToken(lowered, {
            QStringLiteral("return "),
            QStringLiteral("yield "),
            QStringLiteral("throw "),
            QStringLiteral("emit "),
            QStringLiteral("await "),
            QStringLiteral("co_return ")
        }) ||
        containsAnyToken(lowered, {
            QStringLiteral(" return "),
            QStringLiteral(" yield "),
            QStringLiteral(" throw "),
            QStringLiteral(" emit "),
            QStringLiteral(" await "),
            QStringLiteral("=>")
        })) {
        score += 12;
    }

    if (startsWithAnyToken(lowered, {
            QStringLiteral("if "),
            QStringLiteral("else if "),
            QStringLiteral("for "),
            QStringLiteral("while "),
            QStringLiteral("switch "),
            QStringLiteral("case "),
            QStringLiteral("try "),
            QStringLiteral("catch "),
            QStringLiteral("when "),
            QStringLiteral("match ")
        })) {
        score += 8;
    }

    if (containsAnyToken(lowered, {
            QStringLiteral("assert"),
            QStringLiteral("expect("),
            QStringLiteral("require("),
            QStringLiteral("check("),
            QStringLiteral("verify("),
            QStringLiteral("describe("),
            QStringLiteral("it("),
            QStringLiteral("test(")
        })) {
        score += 24;
    }

    if (containsAnyToken(lowered, {
            QStringLiteral(" signal "),
            QStringLiteral(" on"),
            QStringLiteral("property "),
            QStringLiteral("binding"),
            QStringLiteral("onclick"),
            QStringLiteral("onclicked"),
            QStringLiteral("slot")
        })) {
        score += 18;
    }

    const bool keyValueLike =
        trimmedLine.contains(QLatin1Char(':')) ||
        trimmedLine.contains(QLatin1Char('=')) ||
        lowered.contains(QStringLiteral("=>")) ||
        lowered.contains(QStringLiteral(":="));
    if (keyValueLike && trimmedLine.size() <= 180) {
        score += 14;
    }

    if (containsAnyToken(lowered, {
            QStringLiteral("export "),
            QStringLiteral("module.exports"),
            QStringLiteral("public "),
            QStringLiteral("@"),
            QStringLiteral("q_invokable"),
            QStringLiteral("override"),
            QStringLiteral("virtual ")
        })) {
        score += 16;
    }

    return score;
}

int previewLineScore(const QString& trimmedLine,
                     const QStringList& symbolHints,
                     const QStringList& identityHints,
                     const bool inPrivateScope,
                     const int blockComplexityScore,
                     const int lineIndex) {
    if (isTriviaPreviewLine(trimmedLine)) {
        return -1000;
    }

    const QString lowered = trimmedLine.toLower();
    int score = 0;
    const QString declarationName = declarationNameFromLine(trimmedLine);
    const bool matchesFileIdentity =
        !declarationName.isEmpty()
            ? lineMatchesFileIdentity(declarationName.toLower(), identityHints)
            : lineMatchesFileIdentity(lowered, identityHints);
    if (inPrivateScope && isHelperLikeDeclarationName(declarationName) && !matchesFileIdentity) {
        return -1000;
    }
    for (const QString& hint : symbolHints) {
        if (lowered.contains(hint)) {
            score += 65;
        }
    }
    if (matchesFileIdentity) {
        score += 55;
    }
    if (looksLikeImplementationAnchor(trimmedLine)) {
        score += 45;
    }
    if (lowered.startsWith(QStringLiteral("return ")) ||
        lowered.contains(QStringLiteral(" return "))) {
        score += 10;
    }
    if (trimmedLine.contains(QLatin1Char('{'))) {
        score += 5;
    }
    score += universalStructureSignalScore(trimmedLine);
    score += blockComplexityScore;
    if (trimmedLine.contains(QStringLiteral("::"))) {
        score += 12;
    }
    if (!inPrivateScope && looksLikeImplementationAnchor(trimmedLine)) {
        score += 24;
    }
    if (lowered.startsWith(QStringLiteral("export "))) {
        score += 42;
    }
    if (isHelperLikeDeclarationName(declarationName)) {
        score -= blockComplexityScore < 25 ? 60 : 35;
    }
    if (inPrivateScope && !matchesFileIdentity) {
        score -= 140;
    }
    if (lineIndex < 12) {
        score -= 6;
    }
    return score;
}

struct PreviewCandidate {
    int lineIndex = 0;
    int endLineIndex = 0;
    int score = 0;
};

bool isCppLikeFilePath(const QString& filePath) {
    return isCppFamilySuffix(lowerSuffix(filePath));
}

bool isCMakeLikeFilePath(const QString& filePath) {
    const QString fileName = lowerFileName(filePath);
    const QString suffix = lowerSuffix(filePath);
    return fileName == QStringLiteral("cmakelists.txt") ||
           suffix == QStringLiteral("cmake");
}

bool isQtProjectLikeFilePath(const QString& filePath) {
    const QString suffix = lowerSuffix(filePath);
    const QString fileName = lowerFileName(filePath);
    return suffix == QStringLiteral("pro") ||
           suffix == QStringLiteral("pri") ||
           suffix == QStringLiteral("qbs") ||
           suffix == QStringLiteral("qrc") ||
           suffix == QStringLiteral("ui") ||
           suffix == QStringLiteral("rc") ||
           fileName == QStringLiteral("qmldir");
}

int projectFileLineScore(const QString& filePath,
                         const QString& trimmedLine,
                         const int lineIndex) {
    if (trimmedLine.isEmpty()) {
        return -1000;
    }

    const QString loweredLine = trimmedLine.toLower();
    if (loweredLine.startsWith(QStringLiteral("//")) ||
        loweredLine.startsWith(QStringLiteral("# ")) ||
        loweredLine.startsWith(QStringLiteral("<!--"))) {
        return -20;
    }

    int score = 0;
    const QString suffix = lowerSuffix(filePath);
    const QString fileName = lowerFileName(filePath);
    if (isCMakeLikeFilePath(filePath)) {
        if (startsWithAnyToken(loweredLine, {
                QStringLiteral("project("),
                QStringLiteral("cmake_minimum_required("),
                QStringLiteral("find_package("),
                QStringLiteral("add_executable("),
                QStringLiteral("add_library("),
                QStringLiteral("qt_add_executable("),
                QStringLiteral("qt_add_qml_module("),
                QStringLiteral("target_sources("),
                QStringLiteral("target_link_libraries("),
                QStringLiteral("target_include_directories("),
                QStringLiteral("add_subdirectory("),
                QStringLiteral("set(cmake_cxx_standard")
            })) {
            score += 120;
        }
        if (loweredLine.contains(QStringLiteral("qt6::")) ||
            loweredLine.contains(QStringLiteral("qt_add_")) ||
            loweredLine.contains(QStringLiteral("tree-sitter")) ||
            loweredLine.contains(QStringLiteral("libclang"))) {
            score += 28;
        }
    } else if (suffix == QStringLiteral("pro") || suffix == QStringLiteral("pri")) {
        if (startsWithAnyToken(loweredLine, {
                QStringLiteral("qt +="),
                QStringLiteral("config +="),
                QStringLiteral("template ="),
                QStringLiteral("target ="),
                QStringLiteral("sources +="),
                QStringLiteral("headers +="),
                QStringLiteral("resources +="),
                QStringLiteral("forms +="),
                QStringLiteral("include(")
            })) {
            score += 112;
        }
    } else if (suffix == QStringLiteral("qbs")) {
        if (containsAnyToken(loweredLine, {
                QStringLiteral("cppapplication"),
                QStringLiteral("product {"),
                QStringLiteral("depends {"),
                QStringLiteral("files:"),
                QStringLiteral("cpp.cxxlanguageversion"),
                QStringLiteral("qtc.coreplugin")
            })) {
            score += 110;
        }
    } else if (suffix == QStringLiteral("qrc")) {
        if (loweredLine.startsWith(QStringLiteral("<qresource"))) {
            score += 120;
        }
        if (loweredLine.startsWith(QStringLiteral("<file"))) {
            score += 98;
            if (loweredLine.contains(QStringLiteral(".qml")) ||
                loweredLine.contains(QStringLiteral(".png")) ||
                loweredLine.contains(QStringLiteral(".svg"))) {
                score += 20;
            }
        }
    } else if (suffix == QStringLiteral("ui")) {
        if (startsWithAnyToken(loweredLine, {
                QStringLiteral("<class>"),
                QStringLiteral("<widget "),
                QStringLiteral("<layout "),
                QStringLiteral("<connection>"),
                QStringLiteral("<slot>"),
                QStringLiteral("<property name=")
            })) {
            score += 102;
        }
        if (loweredLine.contains(QStringLiteral("qmainwindow")) ||
            loweredLine.contains(QStringLiteral("qdialog")) ||
            loweredLine.contains(QStringLiteral("qwidget"))) {
            score += 24;
        }
    } else if (fileName == QStringLiteral("qmldir")) {
        if (startsWithAnyToken(loweredLine, {
                QStringLiteral("module "),
                QStringLiteral("plugin "),
                QStringLiteral("singleton "),
                QStringLiteral("internal "),
                QStringLiteral("depends ")
            })) {
            score += 115;
        } else if (trimmedLine.contains(QLatin1Char(' ')) &&
                   (loweredLine.endsWith(QStringLiteral(".qml")) ||
                    loweredLine.contains(QStringLiteral(" .")))) {
            score += 92;
        }
    } else if (suffix == QStringLiteral("rc")) {
        if (containsAnyToken(loweredLine, {
                QStringLiteral("versioninfo"),
                QStringLiteral("manifest"),
                QStringLiteral(" icon "),
                QStringLiteral(" dialog"),
                QStringLiteral(" menu")
            })) {
            score += 96;
        }
    } else if (suffix == QStringLiteral("s") || suffix == QStringLiteral("asm")) {
        if (loweredLine.endsWith(QLatin1Char(':')) ||
            startsWithAnyToken(loweredLine, {
                QStringLiteral(".globl "),
                QStringLiteral(".global "),
                QStringLiteral(".section "),
                QStringLiteral("global "),
                QStringLiteral("extern ")
            }) ||
            loweredLine.contains(QStringLiteral(" proc"))) {
            score += 92;
        }
    } else if (suffix == QStringLiteral("def")) {
        if (startsWithAnyToken(loweredLine, {
                QStringLiteral("library "),
                QStringLiteral("exports"),
                QStringLiteral("description ")
            }) ||
            (!trimmedLine.startsWith(QLatin1Char(';')) &&
             trimmedLine.contains(QLatin1Char('@')))) {
            score += 92;
        }
    }

    if (score > 0 && lineIndex < 40) {
        score += 12;
    }
    return score;
}

QString combinedDeclarationText(const QStringList& lines,
                                const int startLine,
                                const int endLine,
                                const int maxLines = 6) {
    QStringList parts;
    const int safeEnd = std::min({endLine, static_cast<int>(lines.size()) - 1, startLine + maxLines - 1});
    for (int index = startLine; index <= safeEnd; ++index) {
        const QString trimmed = lines.at(index).trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        parts.push_back(trimmed);
        if (trimmed.contains(QLatin1Char('{')) || trimmed.endsWith(QLatin1Char(';'))) {
            break;
        }
    }
    return parts.join(QLatin1Char(' '));
}

int rangeComplexityScore(const QStringList& lines, const int startLine, const int endLine) {
    int score = 0;
    int meaningfulLines = 0;
    int controlLines = 0;
    int mutationLines = 0;
    const int lastLineIndex = std::max(0, static_cast<int>(lines.size()) - 1);
    const int safeStart = std::clamp(startLine, 0, lastLineIndex);
    const int safeEnd = std::clamp(endLine, safeStart, lastLineIndex);
    for (int index = safeStart; index <= safeEnd && index <= safeStart + 120; ++index) {
        const QString trimmed = lines.at(index).trimmed();
        if (trimmed.isEmpty() || isTriviaPreviewLine(trimmed)) {
            continue;
        }

        ++meaningfulLines;
        const QString lowered = trimmed.toLower();
        score += std::min(callDensityScore(trimmed), 12);
        if (startsWithAnyToken(lowered, {
                QStringLiteral("if "),
                QStringLiteral("else if "),
                QStringLiteral("for "),
                QStringLiteral("while "),
                QStringLiteral("switch "),
                QStringLiteral("case "),
                QStringLiteral("try "),
                QStringLiteral("catch ")
            })) {
            ++controlLines;
            score += 7;
        }
        if (lowered.contains(QStringLiteral("return ")) ||
            lowered.contains(QStringLiteral("emit ")) ||
            lowered.contains(QStringLiteral("connect(")) ||
            lowered.contains(QStringLiteral(".insert")) ||
            lowered.contains(QStringLiteral(".push")) ||
            lowered.contains(QStringLiteral(".emplace")) ||
            lowered.contains(QStringLiteral(".append")) ||
            lowered.contains(QStringLiteral("std::"))) {
            ++mutationLines;
            score += 6;
        }
    }

    if (meaningfulLines <= 2) {
        score -= 32;
    } else if (meaningfulLines <= 5) {
        score -= 12;
    } else if (meaningfulLines <= 40) {
        score += meaningfulLines;
    } else {
        score += 40;
    }
    score += std::min(controlLines * 4 + mutationLines * 3, 42);
    return std::clamp(score, -48, 120);
}

bool isCppSyntaxPreviewNode(const core::SyntaxNode& node) {
    return node.type == "function_definition" ||
           node.type == "class_specifier" ||
           node.type == "struct_specifier" ||
           node.type == "enum_specifier";
}

int cppSyntaxBaseScore(const core::SyntaxNode& node) {
    if (node.type == "function_definition") {
        return 130;
    }
    if (node.type == "class_specifier" || node.type == "struct_specifier") {
        return 88;
    }
    if (node.type == "enum_specifier") {
        return 46;
    }
    return 0;
}

void collectCppSyntaxCandidates(const core::SyntaxNode& node,
                                const QStringList& lines,
                                const QStringList& symbolHints,
                                const QStringList& identityHints,
                                const QSet<int>& privateScopeIndexes,
                                std::vector<PreviewCandidate>& candidates) {
    const int startLine = static_cast<int>(node.range.startLine);
    const int endLine = static_cast<int>(node.range.endLine);
    if (isCppSyntaxPreviewNode(node) &&
        startLine >= 0 &&
        startLine < lines.size()) {
        const QString signature = combinedDeclarationText(lines, startLine, endLine);
        if (!signature.isEmpty() && !isTriviaPreviewLine(signature)) {
            const QString loweredSignature = signature.toLower();
            const QString declarationName = declarationNameFromLine(signature);
            const bool inPrivateScope = privateScopeIndexes.contains(startLine);
            const bool matchesFileIdentity =
                !declarationName.isEmpty()
                    ? lineMatchesFileIdentity(declarationName.toLower(), identityHints)
                    : lineMatchesFileIdentity(loweredSignature, identityHints);
            const int bodyScore = rangeComplexityScore(lines, startLine, std::min(endLine, startLine + 160));
            int score = cppSyntaxBaseScore(node) +
                        previewLineScore(signature,
                                         symbolHints,
                                         identityHints,
                                         inPrivateScope,
                                         bodyScore,
                                         startLine);

            if (matchesFileIdentity) {
                score += 52;
            }
            if (node.type == "function_definition" && bodyScore > 35) {
                score += 26;
            }
            if (node.type != "function_definition" && node.children.size() <= 2) {
                score -= 18;
            }
            if (isHelperLikeDeclarationName(declarationName) && !matchesFileIdentity) {
                score -= 54;
            }
            if (loweredSignature.startsWith(QStringLiteral("export "))) {
                score += 56;
            }
            if (inPrivateScope && !matchesFileIdentity) {
                score -= 82;
            }

            if (score > 0) {
                candidates.push_back({
                    startLine,
                    std::min(endLine, static_cast<int>(lines.size()) - 1),
                    score
                });
            }
        }
    }

    for (const core::SyntaxNode& child : node.children) {
        collectCppSyntaxCandidates(child, lines, symbolHints, identityHints, privateScopeIndexes, candidates);
    }
}

std::vector<PreviewCandidate> buildCppSyntaxCandidates(const QString& filePath,
                                                       const QByteArray& sourceBytes,
                                                       const QStringList& lines,
                                                       const QStringList& symbolHints,
                                                       const QStringList& identityHints,
                                                       const QSet<int>& privateScopeIndexes) {
    std::vector<PreviewCandidate> candidates;
    if (!isCppLikeFilePath(filePath) || sourceBytes.isEmpty()) {
        return candidates;
    }

    parser::TreeSitterCppParser parser;
    const QByteArray sourceNameBytes = filePath.toUtf8();
    const std::string sourceName(sourceNameBytes.constData(),
                                 static_cast<std::size_t>(sourceNameBytes.size()));
    const std::string_view sourceView(sourceBytes.constData(),
                                      static_cast<std::size_t>(sourceBytes.size()));
    const parser::ParseResult parseResult = parser.parseSource(sourceView, sourceName);
    if (!parseResult.succeeded) {
        return candidates;
    }

    collectCppSyntaxCandidates(parseResult.syntaxTree.root,
                               lines,
                               symbolHints,
                               identityHints,
                               privateScopeIndexes,
                               candidates);
    return candidates;
}

QString formattedPreviewLine(const int lineIndex, QString line) {
    line.replace(QLatin1Char('\r'), QLatin1Char(' '));
    return QStringLiteral("%1 | %2")
        .arg(lineIndex + 1, 3, 10, QLatin1Char(' '))
        .arg(line.left(160));
}

QString buildFallbackPreviewText(const QStringList& lines) {
    QStringList preview;
    for (int index = 0; index < lines.size(); ++index) {
        const QString trimmed = lines.at(index).trimmed();
        if (trimmed.isEmpty() || isTriviaPreviewLine(trimmed)) {
            continue;
        }
        preview.push_back(formattedPreviewLine(index, lines.at(index)));
        if (preview.size() >= 14) {
            break;
        }
    }

    if (preview.isEmpty()) {
        return QStringLiteral("当前文件没有抽取到稳定的文本片段。");
    }
    return preview.join(QLatin1Char('\n'));
}

QString buildPreviewText(const QVariantMap& nodeData,
                         const QStringList& lines,
                         const QByteArray& sourceBytes,
                         const QString& filePath) {
    const QStringList symbolHints = previewSymbolHints(nodeData);
    const QStringList identityHints = fileIdentityHints(nodeData);
    const QSet<int> privateScopeIndexes = anonymousNamespaceLineIndexes(lines);
    std::vector<PreviewCandidate> candidates =
        buildCppSyntaxCandidates(filePath,
                                 sourceBytes,
                                 lines,
                                 symbolHints,
                                 identityHints,
                                 privateScopeIndexes);
    candidates.reserve(static_cast<std::size_t>(lines.size()));
    for (int index = 0; index < lines.size(); ++index) {
        const QString trimmed = lines.at(index).trimmed();
        const int score = previewLineScore(trimmed,
                                           symbolHints,
                                           identityHints,
                                           privateScopeIndexes.contains(index),
                                           surroundingBlockComplexityScore(lines, index),
                                           index) +
                          projectFileLineScore(filePath, trimmed, index);
        if (score > 0) {
            candidates.push_back({index, index, score});
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const PreviewCandidate& left,
                                                       const PreviewCandidate& right) {
        if (left.score != right.score) {
            return left.score > right.score;
        }
        return left.lineIndex < right.lineIndex;
    });

    std::vector<int> anchors;
    const std::size_t maxAnchors =
        isCppLikeFilePath(filePath) ? 2U
                                    : ((isCMakeLikeFilePath(filePath) ||
                                        isQtProjectLikeFilePath(filePath)) ? 4U : 3U);
    for (const PreviewCandidate& candidate : candidates) {
        const bool overlapsExistingAnchor = std::any_of(
            anchors.begin(), anchors.end(), [&](const int anchor) {
                return std::abs(anchor - candidate.lineIndex) <= 6;
            });
        if (overlapsExistingAnchor) {
            continue;
        }
        anchors.push_back(candidate.lineIndex);
        if (anchors.size() >= maxAnchors) {
            break;
        }
    }

    if (anchors.empty()) {
        return buildFallbackPreviewText(lines);
    }

    QSet<int> selectedLineIndexes;
    const bool projectLikeFile = isCMakeLikeFilePath(filePath) || isQtProjectLikeFilePath(filePath);
    for (const int anchor : anchors) {
        const int start = std::max(0, anchor - (projectLikeFile ? 2 : 1));
        const int end = std::min(static_cast<int>(lines.size()) - 1, anchor + 5);
        for (int index = start; index <= end; ++index) {
            const QString trimmed = lines.at(index).trimmed();
            if (trimmed.isEmpty() || isTriviaPreviewLine(trimmed)) {
                continue;
            }
            selectedLineIndexes.insert(index);
        }
    }

    std::vector<int> sortedLineIndexes;
    sortedLineIndexes.reserve(static_cast<std::size_t>(selectedLineIndexes.size()));
    for (const int lineIndex : selectedLineIndexes) {
        sortedLineIndexes.push_back(lineIndex);
    }
    std::sort(sortedLineIndexes.begin(), sortedLineIndexes.end());
    if (sortedLineIndexes.size() > 24) {
        sortedLineIndexes.resize(24);
    }

    QStringList preview;
    int previousLineIndex = -1;
    for (const int lineIndex : sortedLineIndexes) {
        if (previousLineIndex >= 0 && lineIndex > previousLineIndex + 1) {
            preview.push_back(QStringLiteral("... | ..."));
        }
        preview.push_back(formattedPreviewLine(lineIndex, lines.at(lineIndex)));
        previousLineIndex = lineIndex;
    }

    return preview.isEmpty() ? buildFallbackPreviewText(lines)
                             : preview.join(QLatin1Char('\n'));
}

}  // namespace

bool FileInsightService::isSingleFileNode(const QVariantMap& nodeData) {
    if (nodeData.isEmpty()) {
        return false;
    }

    const QString primaryPath = selectPrimaryFilePath(nodeData);
    if (primaryPath.isEmpty()) {
        return false;
    }

    if (!nodeData.value(QStringLiteral("primaryFilePath")).toString().trimmed().isEmpty() ||
        nodeData.value(QStringLiteral("fileCluster")).toBool() ||
        nodeData.value(QStringLiteral("fileBacked")).toBool()) {
        return true;
    }

    const QString nodeName = nodeData.value(QStringLiteral("name")).toString().trimmed();
    if (looksLikeSourceFileName(nodeName)) {
        return true;
    }

    const quint64 fileCount =
        nodeData.value(QStringLiteral("fileCount")).toULongLong();
    const QStringList candidates = sourceFileCandidates(nodeData);
    return fileCount == 1 && candidates.size() == 1 &&
           QFileInfo(candidates.constFirst()).fileName().compare(
               nodeName, Qt::CaseInsensitive) == 0;
}

QString FileInsightService::primaryFilePath(const QVariantMap& nodeData) {
    return selectPrimaryFilePath(nodeData);
}

QVariantMap FileInsightService::buildDetail(
    const QString& projectRootPath,
    const QVariantMap& nodeData) {
    QVariantMap detail;
    detail.insert(QStringLiteral("available"), false);
    detail.insert(QStringLiteral("singleFile"), isSingleFileNode(nodeData));
    detail.insert(QStringLiteral("importClues"), QVariantList{});
    detail.insert(QStringLiteral("declarationClues"), QVariantList{});
    detail.insert(QStringLiteral("behaviorSignals"), QVariantList{});
    detail.insert(QStringLiteral("readingHints"), QVariantList{});
    detail.insert(QStringLiteral("previewText"), QString());

    if (!isSingleFileNode(nodeData)) {
        return detail;
    }

    const QString filePath = primaryFilePath(nodeData);
    const QString absolutePath = QDir(projectRootPath).filePath(filePath);
    const QFileInfo fileInfo(absolutePath);
    const QString languageLabel = detectLanguageLabel(filePath);
    const QString categoryLabel = detectCategoryLabel(filePath);
    const QStringList collaborators =
        toQStringList(nodeData.value(QStringLiteral("collaboratorNames")));
    const QString fallbackSummary =
        nodeData.value(QStringLiteral("responsibility")).toString().trimmed().isEmpty()
            ? nodeData.value(QStringLiteral("summary")).toString().trimmed()
            : nodeData.value(QStringLiteral("responsibility")).toString().trimmed();

    detail.insert(QStringLiteral("filePath"), filePath);
    detail.insert(QStringLiteral("absolutePath"), QDir::toNativeSeparators(absolutePath));
    detail.insert(QStringLiteral("fileName"), QFileInfo(filePath).fileName());
    detail.insert(QStringLiteral("folderPath"), QFileInfo(filePath).path());
    detail.insert(QStringLiteral("languageLabel"), languageLabel);
    detail.insert(QStringLiteral("categoryLabel"), categoryLabel);

    if (!fileInfo.exists() || !fileInfo.isFile()) {
        const QString roleLabel = inferRoleLabel(
            filePath, categoryLabel,
            nodeData.value(QStringLiteral("role")).toString(),
            {}, {}, {});
        detail.insert(QStringLiteral("roleLabel"), roleLabel);
        detail.insert(QStringLiteral("summary"),
                      buildSummary(QFileInfo(filePath).fileName(),
                                   filePath,
                                   QFileInfo(filePath).path(),
                                   languageLabel,
                                   roleLabel,
                                   fallbackSummary,
                                   {}, {}, {}, collaborators));
        detail.insert(QStringLiteral("readingHints"),
                      toVariantStringList(buildReadingHints({}, {}, {}, collaborators)));
        return detail;
    }

    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return detail;
    }

    const QByteArray bytes = file.readAll();
    const QString text = QString::fromUtf8(bytes);
    const QStringList lines = text.split(QLatin1Char('\n'));
    int nonEmptyLineCount = 0;
    for (const QString& line : lines) {
        if (!line.trimmed().isEmpty()) {
            ++nonEmptyLineCount;
        }
    }

    const QStringList importClues = collectImportClues(lines, 6);
    const QStringList declarationClues = collectDeclarationClues(nodeData, lines, 8);
    const QStringList behaviorSignals = collectBehaviorSignals(
        text.toLower(),
        lowerPath(filePath),
        categoryLabel);
    const QString roleLabel = inferRoleLabel(
        filePath,
        categoryLabel,
        nodeData.value(QStringLiteral("role")).toString(),
        importClues,
        declarationClues,
        behaviorSignals);
    const QStringList readingHints = buildReadingHints(
        importClues, declarationClues, behaviorSignals, collaborators);

    detail.insert(QStringLiteral("available"), true);
    detail.insert(QStringLiteral("roleLabel"), roleLabel);
    detail.insert(QStringLiteral("sizeBytes"), static_cast<qulonglong>(bytes.size()));
    detail.insert(QStringLiteral("lineCount"), static_cast<qulonglong>(lines.size()));
    detail.insert(QStringLiteral("nonEmptyLineCount"),
                  static_cast<qulonglong>(nonEmptyLineCount));
    detail.insert(QStringLiteral("importClues"), toVariantStringList(importClues));
    detail.insert(QStringLiteral("declarationClues"),
                  toVariantStringList(declarationClues));
    detail.insert(QStringLiteral("behaviorSignals"),
                  toVariantStringList(behaviorSignals));
    detail.insert(QStringLiteral("readingHints"), toVariantStringList(readingHints));
    detail.insert(QStringLiteral("previewText"), buildPreviewText(nodeData, lines, bytes, filePath));
    detail.insert(QStringLiteral("summary"),
                  buildSummary(QFileInfo(filePath).fileName(),
                               filePath,
                               QFileInfo(filePath).path(),
                               languageLabel,
                               roleLabel,
                               fallbackSummary,
                               importClues,
                               declarationClues,
                               behaviorSignals,
                               collaborators));
    return detail;
}

}  // namespace savt::ui
