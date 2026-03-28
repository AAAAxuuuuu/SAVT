#include "savt/ui/AnalysisController.h"

#include "savt/ai/DeepSeekClient.h"
#include "savt/analyzer/CppProjectAnalyzer.h"
#include "savt/core/ArchitectureGraph.h"
#include "savt/core/ArchitectureOverview.h"
#include "savt/core/CapabilityGraph.h"
#include "savt/core/SyntaxTree.h"
#include "savt/layout/LayeredGraphLayout.h"
#include "savt/parser/TreeSitterCppParser.h"
#include "savt/ui/AiService.h"
#include "savt/ui/AstPreviewService.h"
#include "savt/ui/AnalysisOrchestrator.h"
#include "savt/ui/ReportService.h"
#include "savt/ui/SceneMapper.h"
#include "savt/ui/AnalysisTextFormatter.h"

#include <QtGui/QClipboard>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtGui/QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPromise>
#include <QRectF>
#include <QVariantMap>

#include <QtConcurrent/qtconcurrentrun.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

namespace savt::ui {

namespace {

struct AstPreviewData {
  QString title;
  QString summary;
  QString text;
};

std::filesystem::path toFilesystemPath(const QString &path) {
#ifdef _WIN32
  return std::filesystem::path(path.toStdWString());
#else
  return std::filesystem::path(path.toStdString());
#endif
}

QVariantList toVariantStringList(const std::vector<std::string> &values) {
  QVariantList items;
  items.reserve(static_cast<qsizetype>(values.size()));
  for (const std::string &value : values) {
    items.push_back(QString::fromStdString(value));
  }
  return items;
}

QVariantList toVariantStringList(const QStringList &values) {
  QVariantList items;
  items.reserve(values.size());
  for (const QString &value : values) {
    const QString cleaned = value.trimmed();
    if (!cleaned.isEmpty()) {
      items.push_back(cleaned);
    }
  }
  return items;
}

QStringList toQStringList(const QVariant &value) {
  QStringList items;
  for (const QVariant &entry : value.toList()) {
    const QString cleaned = entry.toString().trimmed();
    if (!cleaned.isEmpty() && !items.contains(cleaned)) {
      items.push_back(cleaned);
    }
  }
  return items;
}

QStringList toQStringList(const std::vector<std::string> &values) {
  QStringList items;
  for (const std::string &value : values) {
    const QString cleaned = QString::fromStdString(value).trimmed();
    if (!cleaned.isEmpty() && !items.contains(cleaned)) {
      items.push_back(cleaned);
    }
  }
  return items;
}

std::size_t countSyntaxNodes(const core::SyntaxNode &node) {
  std::size_t count = 1;
  for (const core::SyntaxNode &child : node.children) {
    count += countSyntaxNodes(child);
  }
  return count;
}

std::size_t computeSyntaxDepth(const core::SyntaxNode &node,
                               const std::size_t currentDepth = 1) {
  std::size_t maxDepth = currentDepth;
  for (const core::SyntaxNode &child : node.children) {
    maxDepth = std::max(maxDepth, computeSyntaxDepth(child, currentDepth + 1));
  }
  return maxDepth;
}

bool isAstPreviewablePath(const QString &relativePath) {
  const QString lowerPath = relativePath.toLower();
  return lowerPath.endsWith(QStringLiteral(".cpp")) ||
         lowerPath.endsWith(QStringLiteral(".cc")) ||
         lowerPath.endsWith(QStringLiteral(".cxx")) ||
         lowerPath.endsWith(QStringLiteral(".c")) ||
         lowerPath.endsWith(QStringLiteral(".hpp")) ||
         lowerPath.endsWith(QStringLiteral(".hh")) ||
         lowerPath.endsWith(QStringLiteral(".hxx")) ||
         lowerPath.endsWith(QStringLiteral(".h"));
}

int astPreviewScore(const QString &relativePath) {
  int score = 0;
  if (relativePath.startsWith(QStringLiteral("src/")))
    score += 40;
  if (relativePath.startsWith(QStringLiteral("apps/")))
    score += 35;
  if (relativePath == QStringLiteral("main.cpp") ||
      relativePath.endsWith(QStringLiteral("/main.cpp")))
    score += 30;
  if (relativePath.endsWith(QStringLiteral(".cpp")))
    score += 10;
  if (relativePath.endsWith(QStringLiteral(".hpp")) ||
      relativePath.endsWith(QStringLiteral(".h")))
    score += 5;
  if (relativePath.startsWith(QStringLiteral("tests/")) ||
      relativePath.contains(QStringLiteral("/tests/")))
    score -= 25;
  if (relativePath.startsWith(QStringLiteral("samples/")) ||
      relativePath.contains(QStringLiteral("/samples/")))
    score -= 10;
  return score;
}

QVariantList buildAstFileItems(const core::AnalysisReport &report) {
  std::vector<const core::SymbolNode *> fileNodes;
  for (const core::SymbolNode &node : report.nodes) {
    if (node.kind == core::SymbolKind::File && !node.filePath.empty()) {
      fileNodes.push_back(&node);
    }
  }
  std::sort(fileNodes.begin(), fileNodes.end(),
            [](const core::SymbolNode *left, const core::SymbolNode *right) {
              return left->filePath < right->filePath;
            });

  QVariantList items;
  for (const core::SymbolNode *node : fileNodes) {
    const QString relativePath = QString::fromStdString(node->filePath);
    if (!isAstPreviewablePath(relativePath)) {
      continue;
    }
    const QString name = QString::fromStdString(
        node->displayName.empty() ? node->qualifiedName : node->displayName);
    QVariantMap item;
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("path"), relativePath);
    item.insert(QStringLiteral("label"),
                name == relativePath
                    ? relativePath
                    : QStringLiteral("%1 | %2").arg(name, relativePath));
    items.push_back(item);
  }
  return items;
}

QString chooseDefaultAstFilePath(const QVariantList &items) {
  QString bestPath;
  int bestScore = std::numeric_limits<int>::min();
  for (const QVariant &value : items) {
    const QVariantMap item = value.toMap();
    const QString path = item.value(QStringLiteral("path")).toString();
    if (path.isEmpty())
      continue;
    const int score = astPreviewScore(path);
    if (bestPath.isEmpty() || score > bestScore ||
        (score == bestScore && path < bestPath)) {
      bestPath = path;
      bestScore = score;
    }
  }
  return bestPath;
}

AstPreviewData buildAstPreview(const QString &projectRootPath,
                               const QString &relativePath) {
  AstPreviewData preview;
  preview.title =
      relativePath.isEmpty() ? QStringLiteral("AST 预览") : relativePath;
  if (relativePath.trimmed().isEmpty()) {
    preview.summary = QStringLiteral(
        "完成项目分析后，会在这里展示所选源码文件的 Tree-sitter AST。");
    preview.text = QStringLiteral(
        "当前还没有可预览的源码文件。\n\n先执行一次项目分析，然后从文件下拉框里"
        "选择想查看的 C++ "
        "源文件。\n界面会展示该文件的语法树层级、节点范围和基础统计信息。");
    return preview;
  }

  const QString absolutePath = QDir(projectRootPath).filePath(relativePath);
  const QFileInfo fileInfo(absolutePath);
  if (!fileInfo.exists() || !fileInfo.isFile()) {
    preview.summary = QStringLiteral("无法读取所选源码文件。");
    preview.text = QStringLiteral("文件不存在或不可访问：\n%1")
                       .arg(QDir::toNativeSeparators(absolutePath));
    return preview;
  }

  parser::TreeSitterCppParser parser;
  const auto parseResult =
      parser.parseFile(toFilesystemPath(fileInfo.absoluteFilePath()));
  if (!parseResult.succeeded) {
    preview.summary = QStringLiteral("Tree-sitter 解析失败。");
    preview.text = QString::fromStdString(
        parseResult.errorMessage.empty()
            ? std::string("Unknown tree-sitter parsing error.")
            : parseResult.errorMessage);
    return preview;
  }

  const auto nodeCount =
      static_cast<qulonglong>(countSyntaxNodes(parseResult.syntaxTree.root));
  const auto depth =
      static_cast<qulonglong>(computeSyntaxDepth(parseResult.syntaxTree.root));
  const auto rootChildren =
      static_cast<qulonglong>(parseResult.syntaxTree.root.children.size());
  const auto byteSize =
      static_cast<qulonglong>(parseResult.sourceFile.content.size());
  preview.summary =
      QStringLiteral("语言：C++ | AST 节点：%1 | 最大深度：%2 | 根子节点：%3 | "
                     "文件大小：%4 B | 预览限制：深度 6 / 每层 20 个子节点")
          .arg(nodeCount)
          .arg(depth)
          .arg(rootChildren)
          .arg(byteSize);
  preview.text = QString::fromStdString(
      core::formatSyntaxTree(parseResult.syntaxTree, 6, 20));
  return preview;
}

std::optional<std::filesystem::path>
locateCompilationDatabase(const QString &projectRootPath) {
  const std::filesystem::path rootPath(projectRootPath.toStdString());
  if (rootPath.empty())
    return std::nullopt;

  const std::array<std::filesystem::path, 4> directCandidates = {
      rootPath / "compile_commands.json",
      rootPath / ".qtc_clangd" / "compile_commands.json",
      rootPath / "build" / "compile_commands.json",
      rootPath / "build" / ".qtc_clangd" / "compile_commands.json"};
  for (const std::filesystem::path &candidate : directCandidates) {
    std::error_code errorCode;
    if (std::filesystem::exists(candidate, errorCode)) {
      return candidate.lexically_normal();
    }
  }
  return std::nullopt;
}

int laneIndexForNode(const core::CapabilityNode &node) {
  switch (node.kind) {
  case core::CapabilityNodeKind::Entry:
    return 0;
  case core::CapabilityNodeKind::Capability:
    return 1;
  case core::CapabilityNodeKind::Infrastructure:
    return 2;
  }
  return 1;
}

CapabilitySceneData
buildCapabilitySceneData(const core::CapabilityGraph &graph) {
  layout::LayeredGraphLayout layoutEngine;
  const auto sceneLayout = layoutEngine.layoutCapabilityScene(graph);
  return SceneMapper::buildCapabilitySceneData(graph, sceneLayout);
}

QString projectNameFromRootPath(const QString &rootPath) {
  const QFileInfo fileInfo(rootPath);
  return fileInfo.fileName().isEmpty() ? QStringLiteral("当前项目")
                                       : fileInfo.fileName();
}

QString inferAnalyzerPrecision(const QString &reportText) {
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

QStringList deduplicateQStringList(const QStringList &values) {
  QStringList items;
  for (const QString &value : values) {
    const QString cleaned = value.trimmed();
    if (!cleaned.isEmpty() && !items.contains(cleaned)) {
      items.push_back(cleaned);
    }
  }
  return items;
}

QString joinPreview(const QStringList &values, const int limit,
                    const QString &emptyText = QString()) {
  const QStringList items = deduplicateQStringList(values);
  if (items.isEmpty())
    return emptyText;
  const int count = std::min(limit, static_cast<int>(items.size()));
  QString preview = items.mid(0, count).join(QStringLiteral("、"));
  if (items.size() > count)
    preview += QStringLiteral(" 等另外 %1 项").arg(items.size() - count);
  return preview;
}

bool textContainsAnyToken(const QString &source, const QStringList &tokens) {
  const QString lowered = source.toLower();
  for (const QString &token : tokens) {
    const QString cleaned = token.trimmed().toLower();
    if (!cleaned.isEmpty() && lowered.contains(cleaned))
      return true;
  }
  return false;
}

QString capabilityEvidenceBlob(const core::CapabilityGraph &graph) {
  QStringList parts;
  for (const core::CapabilityNode &node : graph.nodes) {
    parts.push_back(QString::fromStdString(node.name));
    parts.push_back(QString::fromStdString(node.dominantRole));
    parts.push_back(QString::fromStdString(node.responsibility));
    parts.push_back(QString::fromStdString(node.summary));
    parts.push_back(QString::fromStdString(node.folderHint));
    parts.append(toQStringList(node.moduleNames));
    parts.append(toQStringList(node.exampleFiles));
    parts.append(toQStringList(node.topSymbols));
    parts.append(toQStringList(node.collaboratorNames));
  }
  return parts.join(QLatin1Char(' ')).toLower();
}

bool graphContainsRole(const core::CapabilityGraph &graph,
                       const QStringList &roles) {
  for (const core::CapabilityNode &node : graph.nodes) {
    if (roles.contains(QString::fromStdString(node.dominantRole)))
      return true;
  }
  return false;
}

const core::CapabilityNode *
primaryEntryNode(const core::CapabilityGraph &graph) {
  const core::CapabilityNode *bestNode = nullptr;
  for (const core::CapabilityNode &node : graph.nodes) {
    if (node.kind != core::CapabilityNodeKind::Entry)
      continue;
    if (!bestNode || (node.defaultPinned && !bestNode->defaultPinned) ||
        node.visualPriority > bestNode->visualPriority) {
      bestNode = &node;
    }
  }
  return bestNode;
}

struct OverviewFileTotals {
  std::size_t sourceFiles = 0;
  std::size_t qmlFiles = 0;
  std::size_t webFiles = 0;
  std::size_t scriptFiles = 0;
  std::size_t dataFiles = 0;
};

OverviewFileTotals
summarizeOverviewFiles(const core::ArchitectureOverview &overview) {
  OverviewFileTotals totals;
  for (const core::OverviewNode &node : overview.nodes) {
    totals.sourceFiles += node.sourceFileCount;
    totals.qmlFiles += node.qmlFileCount;
    totals.webFiles += node.webFileCount;
    totals.scriptFiles += node.scriptFileCount;
    totals.dataFiles += node.dataFileCount;
  }
  return totals;
}

QStringList topCapabilityNames(const core::CapabilityGraph &graph,
                               const int limit) {
  std::vector<const core::CapabilityNode *> nodes;
  for (const core::CapabilityNode &node : graph.nodes) {
    if (node.defaultVisible)
      nodes.push_back(&node);
  }
  std::sort(
      nodes.begin(), nodes.end(),
      [](const core::CapabilityNode *left, const core::CapabilityNode *right) {
        if (left->defaultPinned != right->defaultPinned)
          return left->defaultPinned;
        if (left->visualPriority != right->visualPriority)
          return left->visualPriority > right->visualPriority;
        if (left->fileCount != right->fileCount)
          return left->fileCount > right->fileCount;
        return left->name < right->name;
      });
  QStringList names;
  for (const core::CapabilityNode *node : nodes) {
    if (static_cast<int>(names.size()) >= limit)
      break;
    const QString name = QString::fromStdString(node->name).trimmed();
    if (!name.isEmpty() && !names.contains(name))
      names.push_back(name);
  }
  return names;
}

QStringList inferAudienceHints(const core::CapabilityGraph &graph) {
  const QString evidence = capabilityEvidenceBlob(graph);
  QStringList hints;
  if (textContainsAnyToken(
          evidence, {QStringLiteral("admin"), QStringLiteral("editor"),
                     QStringLiteral("manage"), QStringLiteral("management"),
                     QStringLiteral("config")})) {
    hints.push_back(QStringLiteral("管理员或配置维护人员"));
  }
  if (textContainsAnyToken(
          evidence, {QStringLiteral("qml"), QStringLiteral("ui"),
                     QStringLiteral("screen"), QStringLiteral("window"),
                     QStringLiteral("page"), QStringLiteral("dialog"),
                     QStringLiteral("map"), QStringLiteral("webview")})) {
    hints.push_back(QStringLiteral("终端用户或业务操作人员"));
  }
  if (textContainsAnyToken(
          evidence, {QStringLiteral("tool"), QStringLiteral("build"),
                     QStringLiteral("script"), QStringLiteral("generator"),
                     QStringLiteral("cmake")})) {
    hints.push_back(QStringLiteral("开发者或构建人员"));
  }
  if (hints.isEmpty()) {
    hints.push_back(graphContainsRole(graph, {QStringLiteral("presentation"),
                                              QStringLiteral("experience"),
                                              QStringLiteral("web")})
                        ? QStringLiteral("终端用户或业务操作人员")
                        : QStringLiteral("开发者或维护人员"));
  }
  return deduplicateQStringList(hints);
}

QString inferProjectKindSummary(const core::AnalysisReport &,
                                const core::ArchitectureOverview &overview,
                                const core::CapabilityGraph &graph) {
  const auto totals = summarizeOverviewFiles(overview);
  const bool hasPresentation = graphContainsRole(
      graph, {QStringLiteral("presentation"), QStringLiteral("experience"),
              QStringLiteral("web")});
  const bool hasBackend = graphContainsRole(
      graph, {QStringLiteral("interaction"), QStringLiteral("analysis"),
              QStringLiteral("workflow")});
  const bool hasStorage = graphContainsRole(
      graph, {QStringLiteral("storage"), QStringLiteral("data"),
              QStringLiteral("foundation")});
  if (totals.qmlFiles > 0 && totals.sourceFiles > 0 &&
      (totals.webFiles > 0 || hasPresentation))
    return QStringLiteral("一个带图形界面、后端处理和资源数据的 Qt/QML 应用");
  if (totals.qmlFiles > 0 && totals.sourceFiles > 0)
    return QStringLiteral("一个以 QML 界面配合 C++ 后端的桌面应用");
  if (totals.webFiles > 0 && totals.sourceFiles > 0)
    return QStringLiteral("一个同时包含本地后端和网页前端的混合应用");
  if (totals.sourceFiles > 0 && hasBackend && hasStorage)
    return QStringLiteral("一个包含核心处理与数据支撑层的本地应用工程");
  if (totals.sourceFiles > 0 && totals.scriptFiles > 0)
    return QStringLiteral("一个以 C/C++ 为主、脚本辅助处理的工程");
  if (totals.sourceFiles > 0)
    return QStringLiteral("一个以 C/C++ 为主的工程");
  if (totals.scriptFiles > 0)
    return QStringLiteral("一个以脚本和资源处理为主的工程");
  return QStringLiteral("一个仍需要更多证据来确认形态的项目");
}

QString inferInputSummary(const core::ArchitectureOverview &overview,
                          const core::CapabilityGraph &graph) {
  const auto totals = summarizeOverviewFiles(overview);
  const QString evidence = capabilityEvidenceBlob(graph);
  QStringList inputs;
  if (graphContainsRole(graph,
                        {QStringLiteral("presentation"),
                         QStringLiteral("experience"), QStringLiteral("web")}))
    inputs.push_back(QStringLiteral("界面上的用户操作与页面交互"));
  if (totals.dataFiles > 0)
    inputs.push_back(QStringLiteral("配置、资源或 JSON/数据文件"));
  if (totals.scriptFiles > 0)
    inputs.push_back(QStringLiteral("脚本预处理生成的辅助数据"));
  if (textContainsAnyToken(evidence,
                           {QStringLiteral("api"), QStringLiteral("http"),
                            QStringLiteral("network"), QStringLiteral("amap"),
                            QStringLiteral("webservice")}))
    inputs.push_back(QStringLiteral("外部服务或第三方接口返回的数据"));
  if (graphContainsRole(graph, {QStringLiteral("dependency")}))
    inputs.push_back(QStringLiteral("第三方库和外部组件提供的能力"));
  return QStringLiteral("当前看起来，它主要会读取%1。")
      .arg(joinPreview(inputs, 4, QStringLiteral("项目内部传递的调用与状态")));
}

QString inferOutputSummary(const core::ArchitectureOverview &overview,
                           const core::CapabilityGraph &graph) {
  const auto totals = summarizeOverviewFiles(overview);
  QStringList outputs;
  if (graphContainsRole(graph, {QStringLiteral("analysis"),
                                QStringLiteral("interaction"),
                                QStringLiteral("workflow")}))
    outputs.push_back(QStringLiteral("核心处理结果、算法结果或结构化业务数据"));
  if (graphContainsRole(graph,
                        {QStringLiteral("presentation"),
                         QStringLiteral("experience"), QStringLiteral("web")}))
    outputs.push_back(QStringLiteral("界面上的展示结果、页面反馈或可视化状态"));
  if (graphContainsRole(graph,
                        {QStringLiteral("storage"), QStringLiteral("data"),
                         QStringLiteral("foundation")}) ||
      totals.dataFiles > 0)
    outputs.push_back(QStringLiteral("保存后的配置、资源或持久化数据"));
  if (graphContainsRole(graph, {QStringLiteral("tooling")}))
    outputs.push_back(QStringLiteral("构建、生成或预处理阶段的辅助产物"));
  return QStringLiteral("从结构上看，它最后会驱动或产出%1。")
      .arg(joinPreview(outputs, 4,
                       QStringLiteral("模块之间继续向下传递的处理结果")));
}

QString inferEntrySummary(const core::ArchitectureOverview &overview,
                          const core::CapabilityGraph &graph) {
  if (const core::CapabilityNode *entryNode = primaryEntryNode(graph);
      entryNode != nullptr) {
    return QStringLiteral("当前识别到最像主入口的是“%"
                          "1”，它会先发起流程，再把工作交给后续模块。")
        .arg(QString::fromStdString(entryNode->name));
  }
  if (!overview.flows.empty() && !overview.flows.front().summary.empty()) {
    return QStringLiteral(
               "还没有检测到特别稳定的显式入口，但现有依赖图显示主线大致是：%1")
        .arg(QString::fromStdString(overview.flows.front().summary));
  }
  return QStringLiteral("当前还没有识别到稳定的显式入口，这通常意味着入口文件、"
                        "QML 根组件或启动链路还需要继续细化。");
}

QString overviewTechnologySummary(const core::ArchitectureOverview &overview) {
  const auto totals = summarizeOverviewFiles(overview);
  QStringList items;
  if (totals.sourceFiles > 0)
    items.push_back(QStringLiteral("%1 个 C/C++ 文件")
                        .arg(static_cast<qulonglong>(totals.sourceFiles)));
  if (totals.qmlFiles > 0)
    items.push_back(QStringLiteral("%1 个 QML 文件")
                        .arg(static_cast<qulonglong>(totals.qmlFiles)));
  if (totals.webFiles > 0)
    items.push_back(QStringLiteral("%1 个 Web 文件")
                        .arg(static_cast<qulonglong>(totals.webFiles)));
  if (totals.scriptFiles > 0)
    items.push_back(QStringLiteral("%1 个脚本文件")
                        .arg(static_cast<qulonglong>(totals.scriptFiles)));
  if (totals.dataFiles > 0)
    items.push_back(QStringLiteral("%1 个数据/配置文件")
                        .arg(static_cast<qulonglong>(totals.dataFiles)));
  return joinPreview(items, 5,
                     QStringLiteral("当前还没有足够的文件类型证据。"));
}

struct SystemContextSignals {
  int coreModuleCount = 0;
  bool hasUI = false;
  bool hasDatabase = false;
  bool hasNetwork = false;
  QStringList topModules;
};

QString capabilityNodeEvidence(const core::CapabilityNode &node) {
  QStringList parts;
  parts << QString::fromStdString(node.name)
        << QString::fromStdString(node.dominantRole)
        << QString::fromStdString(node.responsibility)
        << QString::fromStdString(node.summary)
        << QString::fromStdString(node.folderHint);
  parts.append(toQStringList(node.moduleNames));
  parts.append(toQStringList(node.topSymbols));
  parts.append(toQStringList(node.exampleFiles));
  return parts.join(QLatin1Char(' ')).toLower();
}

std::vector<const core::CapabilityNode *>
visibleNodesForSummary(const core::CapabilityGraph &graph) {
  std::vector<const core::CapabilityNode *> nodes;
  for (const core::CapabilityNode &node : graph.nodes) {
    if (node.defaultVisible)
      nodes.push_back(&node);
  }
  std::sort(
      nodes.begin(), nodes.end(),
      [](const core::CapabilityNode *left, const core::CapabilityNode *right) {
        if (left->defaultPinned != right->defaultPinned)
          return left->defaultPinned;
        if (left->visualPriority != right->visualPriority)
          return left->visualPriority > right->visualPriority;
        if (left->fileCount != right->fileCount)
          return left->fileCount > right->fileCount;
        return left->name < right->name;
      });
  return nodes;
}

SystemContextSignals summarizeSystemContext(const core::CapabilityGraph &graph) {
  SystemContextSignals contextSignals;
  const auto visibleNodes = visibleNodesForSummary(graph);
  contextSignals.coreModuleCount = static_cast<int>(visibleNodes.size());

  QStringList topModules;
  for (const core::CapabilityNode *node : visibleNodes) {
    const QString evidence = capabilityNodeEvidence(*node);
    if (evidence.contains(QStringLiteral("ui")) ||
        evidence.contains(QStringLiteral("qml")) ||
        evidence.contains(QStringLiteral("view")) ||
        evidence.contains(QStringLiteral("window")) ||
        evidence.contains(QStringLiteral("widget")) ||
        evidence.contains(QStringLiteral("\u754c\u9762"))) {
      contextSignals.hasUI = true;
    }
    if (evidence.contains(QStringLiteral("db")) ||
        evidence.contains(QStringLiteral("database")) ||
        evidence.contains(QStringLiteral("sql")) ||
        evidence.contains(QStringLiteral("repo")) ||
        evidence.contains(QStringLiteral("storage")) ||
        evidence.contains(QStringLiteral("\u5b58\u50a8"))) {
      contextSignals.hasDatabase = true;
    }
    if (evidence.contains(QStringLiteral("net")) ||
        evidence.contains(QStringLiteral("network")) ||
        evidence.contains(QStringLiteral("http")) ||
        evidence.contains(QStringLiteral("api")) ||
        evidence.contains(QStringLiteral("client")) ||
        evidence.contains(QStringLiteral("socket")) ||
        evidence.contains(QStringLiteral("\u7f51\u7edc"))) {
      contextSignals.hasNetwork = true;
    }

    if (node->kind == core::CapabilityNodeKind::Entry)
      continue;

    const QString name = QString::fromStdString(node->name).trimmed();
    if (!name.isEmpty() && !topModules.contains(name)) {
      topModules.push_back(name);
      if (topModules.size() >= 3)
        break;
    }
  }

  if (topModules.isEmpty()) {
    for (const core::CapabilityNode *node : visibleNodes) {
      const QString name = QString::fromStdString(node->name).trimmed();
      if (!name.isEmpty() && !topModules.contains(name)) {
        topModules.push_back(name);
        if (topModules.size() >= 3)
          break;
      }
    }
  }

  contextSignals.topModules = topModules;
  return contextSignals;
}

QString inlineMarkdownCodeList(const QStringList &values, const int limit) {
  QStringList items = deduplicateQStringList(values);
  const bool truncated = items.size() > limit;
  if (truncated)
    items = items.mid(0, limit);
  for (QString &value : items) {
    value = QStringLiteral("`%1`").arg(value);
  }
  QString output = items.join(QStringLiteral(", "));
  if (truncated && !output.isEmpty())
    output += QStringLiteral(" \u7b49");
  return output;
}

QString capabilityEdgeRelationLabel(const core::CapabilityEdgeKind kind) {
  switch (kind) {
  case core::CapabilityEdgeKind::Activates:
    return QStringLiteral("\u8c03\u7528/\u63a8\u8fdb");
  case core::CapabilityEdgeKind::Enables:
    return QStringLiteral("\u534f\u4f5c/\u652f\u6491");
  case core::CapabilityEdgeKind::UsesInfrastructure:
    return QStringLiteral("\u4f9d\u8d56\u57fa\u7840\u80fd\u529b");
  }
  return QStringLiteral("\u5173\u8054");
}

QString safeMermaidLabel(QString label) {
  label.replace(QStringLiteral("\r"), QString());
  label.replace(QStringLiteral("\n"), QStringLiteral(" "));
  label.replace(QLatin1Char('"'), QLatin1Char('\''));
  label.replace(QLatin1Char('['), QLatin1Char('('));
  label.replace(QLatin1Char(']'), QLatin1Char(')'));
  return label.trimmed();
}

QVariantMap findCapabilityNodeItem(const QVariantList &nodeItems,
                                   const qulonglong nodeId) {
  for (const QVariant &entry : nodeItems) {
    const QVariantMap node = entry.toMap();
    if (node.value(QStringLiteral("id")).toULongLong() == nodeId)
      return node;
  }
  return {};
}

QString buildL4MarkdownReportFromNodeItems(const QVariantList &capabilityNodeItems) {
  QList<QVariantMap> nodes;
  for (const QVariant &item : capabilityNodeItems) {
    const QVariantMap node = item.toMap();
    if (node.value(QStringLiteral("defaultVisible"), true).toBool())
      nodes.append(node);
  }
  if (nodes.isEmpty()) {
    return QStringLiteral("# 暂无分析结果\n请选择项目目录并点击“开始分析”。");
  }

  int totalFiles = 0;
  int totalModules = 0;
  QStringList entryNames;
  QStringList infraNames;
  for (const QVariantMap &node : nodes) {
    totalFiles += node.value(QStringLiteral("fileCount")).toInt();
    totalModules += node.value(QStringLiteral("moduleCount")).toInt();
    const QString kind = node.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("entry")) {
      entryNames << node.value(QStringLiteral("name")).toString();
    } else if (kind == QStringLiteral("infrastructure")) {
      infraNames << node.value(QStringLiteral("name")).toString();
    }
  }

  std::sort(nodes.begin(), nodes.end(),
            [](const QVariantMap &left, const QVariantMap &right) {
              return left.value(QStringLiteral("priority")).toULongLong() >
                     right.value(QStringLiteral("priority")).toULongLong();
            });

  QString report;
  report.reserve(4096);
  report += QStringLiteral("# 📐 项目架构全景报告\n\n");
  report += QStringLiteral(
      "> 由 SAVT 静态分析引擎自动生成 · C4 模型 L4 视图\n\n---\n\n");
  report += QStringLiteral(
      "## 📊 项目概览\n\n| 指标 | 数值 |\n|------|------|\n");
  report +=
      QStringLiteral("| 识别模块数 | **%1 个** |\n").arg(nodes.size());
  report +=
      QStringLiteral("| 涉及源码文件 | **%1 个** |\n").arg(totalFiles);
  report +=
      QStringLiteral("| 子模块总数 | **%1 个** |\n").arg(totalModules);
  if (!entryNames.isEmpty()) {
    report += QStringLiteral("| 发起入口 | %1 |\n")
                  .arg(entryNames.join(QStringLiteral(", ")));
  }
  if (!infraNames.isEmpty()) {
    report += QStringLiteral("| 后台支撑 | %1 |\n")
                  .arg(infraNames.join(QStringLiteral(", ")));
  }
  report += QStringLiteral("\n---\n\n");

  report += QStringLiteral("## 🗂️ 模块详情\n\n");
  for (const QVariantMap &node : nodes) {
    const QString name = node.value(QStringLiteral("name")).toString();
    const QString role = node.value(QStringLiteral("role")).toString();
    const QString responsibility =
        node.value(QStringLiteral("responsibility")).toString();
    const QString summary = node.value(QStringLiteral("summary")).toString();
    const QString folderHint =
        node.value(QStringLiteral("folderHint")).toString();
    const int inEdge =
        node.value(QStringLiteral("incomingEdgeCount")).toInt();
    const int outEdge =
        node.value(QStringLiteral("outgoingEdgeCount")).toInt();
    const int fileCount = node.value(QStringLiteral("fileCount")).toInt();
    const QStringList topSymbols =
        toQStringList(node.value(QStringLiteral("topSymbols")));
    const QStringList exampleFiles =
        toQStringList(node.value(QStringLiteral("exampleFiles")));
    const QStringList collaborators =
        toQStringList(node.value(QStringLiteral("collaboratorNames")));

    report += QStringLiteral("### %1 %2\n\n").arg(role, name);
    if (!responsibility.isEmpty()) {
      report += QStringLiteral("**职责：** %1\n\n").arg(responsibility);
    }
    if (!summary.isEmpty() && summary != responsibility) {
      report += QStringLiteral("%1\n\n").arg(summary);
    }

    report += QStringLiteral("- 📁 源文件：**%1 个**").arg(fileCount);
    if (!folderHint.isEmpty()) {
      report += QStringLiteral("（`%1`）").arg(folderHint);
    }
    report += QStringLiteral(
                  "\n- 🔗 被依赖：**%1 次** / 依赖他人：**%2 次**\n")
                  .arg(inEdge)
                  .arg(outEdge);
    if (!collaborators.isEmpty()) {
      report += QStringLiteral("- 🤝 协作模块：%1\n")
                    .arg(collaborators.join(QStringLiteral("、")));
    }

    if (!topSymbols.isEmpty()) {
      report += QStringLiteral("\n**核心类 / 符号（AST 提取）：**\n");
      const int limit = qMin(topSymbols.size(), 8);
      for (int index = 0; index < limit; ++index) {
        report += QStringLiteral("  - `%1`\n").arg(topSymbols[index]);
      }
      if (topSymbols.size() > 8) {
        report +=
            QStringLiteral("  - *...共 %1 个符号*\n").arg(topSymbols.size());
      }
    }
    if (!exampleFiles.isEmpty()) {
      report += QStringLiteral("\n**代表文件：**\n");
      const int limit = qMin(exampleFiles.size(), 5);
      for (int index = 0; index < limit; ++index) {
        report += QStringLiteral("  - `%1`\n").arg(exampleFiles[index]);
      }
    }
    report += QStringLiteral("\n---\n\n");
  }

  return report;
}

QString buildL1SystemContextFromNodeItems(const QVariantList &capabilityNodeItems) {
  QString report = QStringLiteral("# 🌐 系统上下文 (L1)\n\n");
  QStringList entries;
  QStringList capabilities;
  QStringList infrastructures;

  for (const QVariant &item : capabilityNodeItems) {
    const QVariantMap node = item.toMap();
    if (!node.value(QStringLiteral("defaultVisible"), true).toBool())
      continue;

    const QString line =
        QStringLiteral("- **%1** (%2)：%3\n")
            .arg(node.value(QStringLiteral("name")).toString(),
                 node.value(QStringLiteral("role")).toString(),
                 node.value(QStringLiteral("responsibility")).toString());
    const QString kind = node.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("entry")) {
      entries << line;
    } else if (kind == QStringLiteral("infrastructure")) {
      infrastructures << line;
    } else {
      capabilities << line;
    }
  }

  if (!entries.isEmpty()) {
    report += QStringLiteral("## 🚪 发起入口\n") + entries.join(QString()) +
              QStringLiteral("\n");
  }
  if (!capabilities.isEmpty()) {
    report += QStringLiteral("## ⚙️ 核心能力\n") +
              capabilities.join(QString()) + QStringLiteral("\n");
  }
  if (!infrastructures.isEmpty()) {
    report += QStringLiteral("## 🏗️ 后台支撑\n") +
              infrastructures.join(QString()) + QStringLiteral("\n");
  }

  return report;
}

QVariantList buildSystemContextCards(const core::AnalysisReport &report,
                                     const core::ArchitectureOverview &overview,
                                     const core::CapabilityGraph &graph) {
  const auto contextSignals = summarizeSystemContext(graph);
  const QString inputSummary =
      contextSignals.hasNetwork
          ? QStringLiteral("\u4e3b\u8981\u901a\u8fc7\u7f51\u7edc API\u3001Socket \u6216\u5916\u90e8\u914d\u7f6e\u6587\u4ef6\u63a5\u6536\u8f93\u5165\u3002")
          : QStringLiteral("\u4e3b\u8981\u901a\u8fc7\u672c\u5730\u7cfb\u7edf\u63a5\u53e3\u3001\u7528\u6237\u64cd\u4f5c\u6216\u672c\u5730\u6587\u4ef6\u52a0\u8f7d\u63a5\u6536\u8f93\u5165\u3002");
  const QString outputSummary =
      contextSignals.hasUI
          ? QStringLiteral("\u901a\u8fc7\u56fe\u5f62\u7528\u6237\u754c\u9762 (GUI) \u5411\u7528\u6237\u8fdb\u884c\u53ef\u89c6\u5316\u5448\u73b0\u4e0e\u4ea4\u4e92\u3002")
          : QStringLiteral("\u4e3b\u8981\u5c06\u5904\u7406\u7ed3\u679c\u8f93\u51fa\u5230\u63a7\u5236\u53f0\u3001\u65e5\u5fd7\u6216\u672c\u5730\u5b58\u50a8\u4e2d\u3002");

  QVariantList cards;
  cards.push_back(QVariantMap{
      {QStringLiteral("name"), QStringLiteral("\u9879\u76ee\u5f62\u6001")},
      {QStringLiteral("summary"),
       QStringLiteral("\u5f53\u524d\u9879\u76ee\u5df2\u88ab\u5f15\u64ce\u89e3\u6790\u4e3a %1 \u4e2a\u6838\u5fc3\u6a21\u5757\uff0c\u6574\u4f53\u66f4\u50cf%2\u3002")
           .arg(contextSignals.coreModuleCount)
           .arg(inferProjectKindSummary(report, overview, graph))}});
  cards.push_back(QVariantMap{
      {QStringLiteral("name"), QStringLiteral("\u8f93\u5165 / \u8f93\u51fa")},
      {QStringLiteral("summary"),
       QStringLiteral("%1 %2").arg(inputSummary, outputSummary)}});
  cards.push_back(QVariantMap{
      {QStringLiteral("name"), QStringLiteral("\u5efa\u8bae\u5148\u770b")},
      {QStringLiteral("summary"),
       contextSignals.topModules.isEmpty()
           ? inferEntrySummary(overview, graph)
           : QStringLiteral("\u5efa\u8bae\u4f18\u5148\u67e5\u770b\u300c%1\u300d\u3002")
                 .arg(contextSignals.topModules.join(QStringLiteral("\u300d\u3001\u300c")))}});
  return cards;
}

QVariantMap buildSystemContextData(const core::AnalysisReport &report,
                                   const core::ArchitectureOverview &overview,
                                   const core::CapabilityGraph &graph,
                                   const QString &projectRootPath) {
  Q_UNUSED(report);
  const QString projectName = projectNameFromRootPath(projectRootPath);
  const auto contextSignals = summarizeSystemContext(graph);
  const auto totals = summarizeOverviewFiles(overview);

  QStringList technologyClues;
  technologyClues.push_back(QStringLiteral("\u5e95\u5c42\u91c7\u7528 C++ \u7f16\u5199"));
  if (contextSignals.hasUI || totals.qmlFiles > 0)
    technologyClues.push_back(QStringLiteral("\u5305\u542b Qt/QML \u7b49\u524d\u7aef\u754c\u9762\u5c42"));
  if (contextSignals.hasDatabase)
    technologyClues.push_back(QStringLiteral("\u6d89\u53ca\u6570\u636e\u6301\u4e45\u5316\u6216\u672c\u5730\u5b58\u50a8"));
  if (contextSignals.hasNetwork)
    technologyClues.push_back(QStringLiteral("\u5305\u542b\u7f51\u7edc\u901a\u4fe1\u6216\u5916\u90e8\u63a5\u53e3\u96c6\u6210"));
  if (totals.scriptFiles > 0)
    technologyClues.push_back(QStringLiteral("\u5e26\u6709\u811a\u672c\u548c\u8f85\u52a9\u751f\u6210\u6d41\u7a0b"));
  const QString technologySummary = technologyClues.join(QStringLiteral("\uff0c")) +
                                    QStringLiteral("\u3002");

  QVariantMap data;
  data.insert(QStringLiteral("projectName"), projectName);
  data.insert(QStringLiteral("title"),
              QStringLiteral("L1 \u7cfb\u7edf\u4e0a\u4e0b\u6587 (System Context)"));
  data.insert(QStringLiteral("headline"),
              QStringLiteral("\u5f53\u524d\u9879\u76ee\u5df2\u88ab\u5f15\u64ce\u89e3\u6790\u4e3a %1 \u4e2a\u6838\u5fc3\u6a21\u5757\u3002")
                  .arg(contextSignals.coreModuleCount));
  data.insert(QStringLiteral("audienceSummary"),
              joinPreview(inferAudienceHints(graph), 3,
                          QStringLiteral("\u5f00\u53d1\u8005\u3001\u7ef4\u62a4\u8005\u6216\u9700\u8981\u5feb\u901f\u7406\u89e3\u7cfb\u7edf\u7684\u4eba")));
  data.insert(QStringLiteral("purposeSummary"),
              contextSignals.topModules.isEmpty()
                  ? QStringLiteral("\u5f53\u524d\u8fd8\u5728\u7ee7\u7eed\u6536\u655b\u9879\u76ee\u7684\u6838\u5fc3\u4e1a\u52a1\u8fb9\u754c\u3002")
                  : QStringLiteral("\u6700\u503c\u5f97\u4f18\u5148\u7406\u89e3\u7684\u6a21\u5757\u5305\u62ec\uff1a\u300c%1\u300d\u3002")
                        .arg(contextSignals.topModules.join(QStringLiteral("\u300d\u3001\u300c"))));
  data.insert(QStringLiteral("entrySummary"), inferEntrySummary(overview, graph));
  data.insert(QStringLiteral("inputSummary"),
              contextSignals.hasNetwork
                  ? QStringLiteral("\u4e3b\u8981\u901a\u8fc7\u7f51\u7edc API\u3001Socket \u6216\u5916\u90e8\u914d\u7f6e\u6587\u4ef6\u63a5\u6536\u8f93\u5165\u3002")
                  : QStringLiteral("\u4e3b\u8981\u901a\u8fc7\u672c\u5730\u7cfb\u7edf\u63a5\u53e3\u3001\u7528\u6237\u64cd\u4f5c\u6216\u672c\u5730\u6587\u4ef6\u52a0\u8f7d\u63a5\u6536\u8f93\u5165\u3002"));
  data.insert(QStringLiteral("outputSummary"),
              contextSignals.hasUI
                  ? QStringLiteral("\u901a\u8fc7\u56fe\u5f62\u7528\u6237\u754c\u9762 (GUI) \u5411\u7528\u6237\u8fdb\u884c\u53ef\u89c6\u5316\u5448\u73b0\u4e0e\u4ea4\u4e92\u3002")
                  : QStringLiteral("\u4e3b\u8981\u5c06\u5904\u7406\u7ed3\u679c\u8f93\u51fa\u5230\u63a7\u5236\u53f0\u3001\u65e5\u5fd7\u6216\u672c\u5730\u5b58\u50a8\u4e2d\u3002"));
  data.insert(QStringLiteral("technologySummary"), technologySummary);
  data.insert(QStringLiteral("containerSummary"),
              contextSignals.topModules.isEmpty()
                  ? QStringLiteral("\u5f53\u524d\u8fd8\u6ca1\u6709\u63d0\u70bc\u51fa\u7a33\u5b9a\u7684\u6838\u5fc3\u6a21\u5757\uff0c\u5efa\u8bae\u5148\u4ece\u5165\u53e3\u548c\u4e3b\u6d41\u7a0b\u5f00\u59cb\u67e5\u770b\u3002")
                  : QStringLiteral("\u6700\u6838\u5fc3\u7684\u90e8\u5206\u5305\u62ec\uff1a\u300c%1\u300d\u7b49\u3002\u5efa\u8bae\u4f18\u5148\u67e5\u770b\u8fd9\u51e0\u5757\u3002")
                        .arg(contextSignals.topModules.join(QStringLiteral("\u300d\u3001\u300c"))));
  data.insert(QStringLiteral("containerNames"), contextSignals.topModules);
  return data;
}

QString buildStatusMessage(const core::AnalysisReport &report,
                           const core::ArchitectureOverview &overview,
                           const core::CapabilityGraph &capabilityGraph,
                           const layout::LayoutResult &layoutResult) {
  Q_UNUSED(layoutResult);
  const auto visibleNodeCount = static_cast<qulonglong>(std::count_if(
      capabilityGraph.nodes.begin(), capabilityGraph.nodes.end(),
      [](const core::CapabilityNode &node) { return node.defaultVisible; }));
  const auto visibleEdgeCount = static_cast<qulonglong>(std::count_if(
      capabilityGraph.edges.begin(), capabilityGraph.edges.end(),
      [](const core::CapabilityEdge &edge) { return edge.defaultVisible; }));
  const QString semanticStatusMessage =
      QString::fromStdString(report.semanticStatusMessage).trimmed();
  if (report.semanticAnalysisRequested && !report.semanticAnalysisEnabled &&
      report.parsedFiles == 0) {
    return semanticStatusMessage.isEmpty()
               ? QStringLiteral("未能进入语义分析。请检查项目目录、编译数据库或当前分析环境后再试一次。")
               : QStringLiteral("未能进入语义分析：%1")
                     .arg(semanticStatusMessage);
  }
  if (report.parsedFiles > 0) {
    QString message =
        QStringLiteral("已读取 %1 个源码文件，并整理出 %2 个默认可见模块、%3 条主要关系和 %4 个分组。")
            .arg(static_cast<qulonglong>(report.parsedFiles))
            .arg(visibleNodeCount)
            .arg(visibleEdgeCount)
            .arg(static_cast<qulonglong>(capabilityGraph.groups.size()));
    if (report.semanticAnalysisRequested && !report.semanticAnalysisEnabled) {
      message += semanticStatusMessage.isEmpty()
                     ? QStringLiteral(" 当前未进入语义后端，结果已回退到语法级分析。")
                     : QStringLiteral(" 当前未进入语义后端，结果已回退到语法级分析。原因：%1")
                           .arg(semanticStatusMessage);
    } else if (report.semanticAnalysisEnabled) {
      message += QStringLiteral(" 当前已进入 Clang/LibTooling 语义后端。");
    }
    if (overview.nodes.empty()) {
      message += QStringLiteral(" 当前还没有提炼出稳定的高层模块，建议优先查看入口和主流程。");
    }
    return message;
  }
  if (report.semanticAnalysisRequested && !report.semanticAnalysisEnabled &&
      report.parsedFiles == 0) {
    return QStringLiteral("这次分析没有成功读到可用源码。请检查项目目录、编译数"
                          "据库或当前分析环境后再试一次。");
  }
  if (report.parsedFiles > 0) {
    QString message =
        QStringLiteral(
            "已经读完 %1 个源码文件，并整理出 %2 个关键部分、%3 条主要关系和 "
            "%4 个分组。先看左侧 C4 容器图，再点任意卡片查看说明。")
            .arg(static_cast<qulonglong>(report.parsedFiles))
            .arg(visibleNodeCount)
            .arg(visibleEdgeCount)
            .arg(static_cast<qulonglong>(capabilityGraph.groups.size()));
    if (report.semanticAnalysisRequested && !report.semanticAnalysisEnabled)
      message += QStringLiteral(" 当前使用语法级分析，适合快速理解结构。详细诊"
                                "断可在“程序员入口”查看。");
    if (overview.nodes.empty())
      message += QStringLiteral(" 当前还没有提炼出稳定的高层模块，请优先参考右"
                                "侧的项目说明与源码入口。");
    return message;
  }
  return QStringLiteral(
      "还没有分析到源码文件。请确认项目目录正确，然后重新开始分析。");
}

QString buildAnalysisReportText(const core::AnalysisReport &report,
                                const core::ArchitectureOverview &overview,
                                const core::CapabilityGraph &capabilityGraph,
                                const layout::LayoutResult &layoutResult) {
  Q_UNUSED(layoutResult);
  const QString projectName =
      projectNameFromRootPath(QString::fromStdString(report.rootPath));
  const auto contextSignals = summarizeSystemContext(capabilityGraph);
  const auto visibleNodes = visibleNodesForSummary(capabilityGraph);

  std::unordered_map<std::size_t, QString> nodeLabels;
  for (const core::CapabilityNode *node : visibleNodes) {
    nodeLabels.emplace(node->id,
                       safeMermaidLabel(QString::fromStdString(node->name)));
  }

  std::vector<const core::CapabilityEdge *> visibleEdges;
  for (const core::CapabilityEdge &edge : capabilityGraph.edges) {
    if (!edge.defaultVisible)
      continue;
    if (!nodeLabels.contains(edge.fromId) || !nodeLabels.contains(edge.toId))
      continue;
    visibleEdges.push_back(&edge);
  }
  std::sort(visibleEdges.begin(), visibleEdges.end(),
            [](const core::CapabilityEdge *left,
               const core::CapabilityEdge *right) {
              if (left->weight != right->weight)
                return left->weight > right->weight;
              return left->id < right->id;
            });

  QString reportText;
  reportText += QStringLiteral("# \U0001F4E6 %1 \u67b6\u6784\u5206\u6790\u62a5\u544a (C4 Model)\n\n")
                    .arg(projectName);
  reportText += QStringLiteral(
      "> \U0001F4A1 \u7531 SAVT \u5f15\u64ce\u57fa\u4e8e AST \u9759\u6001\u8bed\u4e49\u5206\u6790\u79d2\u7ea7\u751f\u6210\uff0c\u65e0\u9700\u6d88\u8017 AI API \u989d\u5ea6\u3002\n\n");
  reportText += QStringLiteral("## 1. \u7cfb\u7edf\u6982\u89c8\n\n");
  reportText += QStringLiteral("- \u5df2\u89e3\u6790 `%1` \u4e2a\u6e90\u7801\u6587\u4ef6\uff0c\u8bc6\u522b\u51fa `%2` \u4e2a\u9ed8\u8ba4\u5c55\u793a\u6a21\u5757\u3002\n")
                    .arg(static_cast<qulonglong>(report.parsedFiles))
                    .arg(contextSignals.coreModuleCount);
  reportText += QStringLiteral("- \u5f53\u524d\u9879\u76ee\u5f62\u6001\uff1a%1\u3002\n")
                    .arg(inferProjectKindSummary(report, overview, capabilityGraph));
  reportText += QStringLiteral("- \u5efa\u8bae\u4f18\u5148\u67e5\u770b\uff1a%1\u3002\n\n")
                    .arg(contextSignals.topModules.isEmpty()
                             ? QStringLiteral("\u5165\u53e3\u6a21\u5757\u548c\u4e3b\u6d41\u7a0b")
                             : contextSignals.topModules.join(QStringLiteral("\u3001")));

  reportText += QStringLiteral("## 2. \u6838\u5fc3\u7ec4\u4ef6 (Components)\n\n");
  if (visibleNodes.empty()) {
    reportText += QStringLiteral("- \u5f53\u524d\u8fd8\u6ca1\u6709\u53ef\u5c55\u793a\u7684\u6838\u5fc3\u6a21\u5757\uff0c\u8bf7\u5148\u5b8c\u6210\u4e00\u6b21\u9879\u76ee\u5206\u6790\u3002\n\n");
  } else {
    for (const core::CapabilityNode *node : visibleNodes) {
      const QString nodeName = QString::fromStdString(node->name).trimmed();
      const QString role = QString::fromStdString(node->dominantRole).trimmed();
      const QString responsibility =
          QString::fromStdString(node->responsibility).trimmed();
      const QString summary = QString::fromStdString(node->summary).trimmed();
      const QString files =
          inlineMarkdownCodeList(toQStringList(node->exampleFiles), 3);
      const QString symbols =
          inlineMarkdownCodeList(toQStringList(node->topSymbols), 5);

      reportText += QStringLiteral("- **%1**\n")
                        .arg(nodeName.isEmpty()
                                 ? QStringLiteral("\u672a\u547d\u540d\u6a21\u5757")
                                 : nodeName);
      if (!role.isEmpty())
        reportText += QStringLiteral("  - \U0001F9ED **\u89d2\u8272**: %1\n").arg(role);
      reportText += QStringLiteral("  - \U0001F3AF **\u804c\u8d23**: %1\n")
                        .arg(responsibility.isEmpty()
                                 ? (summary.isEmpty()
                                        ? QStringLiteral("\u5f53\u524d\u8fd8\u6ca1\u6709\u63d0\u70bc\u51fa\u7a33\u5b9a\u804c\u8d23\u3002")
                                        : summary)
                                 : responsibility);
      if (!summary.isEmpty() && summary != responsibility)
        reportText += QStringLiteral("  - \U0001F4DD **\u6458\u8981**: %1\n").arg(summary);
      if (!files.isEmpty())
        reportText += QStringLiteral("  - \U0001F4C1 **\u76f8\u5173\u6587\u4ef6**: %1\n").arg(files);
      if (!symbols.isEmpty())
        reportText += QStringLiteral("  - \U0001F9F1 **\u6838\u5fc3\u7c7b/\u7ed3\u6784**: %1\n").arg(symbols);
      reportText += QStringLiteral("\n");
    }
  }

  reportText += QStringLiteral("## 3. \u6838\u5fc3\u5173\u7cfb\u6d41\u8f6c (Relationships)\n\n");
  reportText += QStringLiteral("```mermaid\n");
  reportText += QStringLiteral("graph TD;\n");
  if (visibleEdges.empty()) {
    reportText += QStringLiteral("    A[\u6682\u65e0\u53ef\u5c55\u793a\u5173\u7cfb] --> B[\u8bf7\u5148\u5b8c\u6210\u5206\u6790];\n");
  } else {
    const int limit = std::min<int>(15, static_cast<int>(visibleEdges.size()));
    for (int index = 0; index < limit; ++index) {
      const core::CapabilityEdge *edge = visibleEdges[index];
      reportText += QStringLiteral("    N%1[%2] -->|%3| N%4[%5];\n")
                        .arg(static_cast<qulonglong>(edge->fromId))
                        .arg(nodeLabels.at(edge->fromId))
                        .arg(capabilityEdgeRelationLabel(edge->kind))
                        .arg(static_cast<qulonglong>(edge->toId))
                        .arg(nodeLabels.at(edge->toId));
    }
  }
  reportText += QStringLiteral("```\n\n");

  reportText += QStringLiteral("## 4. \u4f7f\u7528\u5efa\u8bae\n\n");
  reportText += QStringLiteral(
      "- \u4f60\u53ef\u4ee5\u76f4\u63a5\u590d\u5236\u8fd9\u4efd\u62a5\u544a\uff0c\u7c98\u8d34\u7ed9 Cursor\u3001Claude \u6216 DeepSeek \u4f5c\u4e3a\u9879\u76ee\u4e0a\u4e0b\u6587\u3002\n");
  reportText += QStringLiteral(
      "- \u5982\u679c\u53ea\u60f3\u4fee\u6539\u67d0\u4e2a\u6a21\u5757\uff0c\u5148\u5728\u53f3\u4fa7\u9009\u62e9\u6a21\u5757\uff0c\u518d\u7528\u201c\u6838\u5fc3\u4ee3\u7801\u63d0\u53d6\u5668\u201d\u590d\u5236\u66f4\u7cbe\u51c6\u7684\u6587\u4ef6\u4e0e\u7b26\u53f7\u5217\u8868\u3002\n\n");
  reportText += QStringLiteral(
      "---\n\u63d0\u793a\uff1a\u8fd9\u4efd\u62a5\u544a\u5df2\u7ecf\u6309 AI \u63d0\u793a\u8bcd\u573a\u666f\u6574\u7406\u8fc7\u7ed3\u6784\uff0c\u9002\u5408\u76f4\u63a5\u4f5c\u4e3a\u5927\u6a21\u578b\u7684\u9996\u8f6e\u4e0a\u4e0b\u6587\u3002\n");
  return reportText;
}

void populatePendingAnalysisResult(
    QPromise<void> &promise, const QString &cleanedPath,
    const core::AnalysisReport &report, const core::ArchitectureOverview &overview,
    const core::CapabilityGraph &capabilityGraph,
    const layout::CapabilitySceneLayoutResult &capabilitySceneLayout,
    const layout::LayoutResult &layoutResult, PendingAnalysisResult &result) {
  promise.setProgressValueAndText(96, QStringLiteral("鏁寸悊 AST 棰勮..."));
  if (promise.isCanceled())
    return;

  result.astFileItems = AstPreviewService::buildAstFileItems(report);
  result.selectedAstFilePath =
      AstPreviewService::chooseDefaultAstFilePath(result.astFileItems);
  const auto astPreview =
      AstPreviewService::buildPreview(cleanedPath, result.selectedAstFilePath);
  result.astPreviewTitle = astPreview.title;
  result.astPreviewSummary = astPreview.summary;
  result.astPreviewText = astPreview.text;
  if (promise.isCanceled())
    return;

  promise.setProgressValueAndText(98, QStringLiteral("鏁寸悊鍙鍖栨暟鎹?.."));
  const auto sceneData = SceneMapper::buildCapabilitySceneData(capabilityGraph, capabilitySceneLayout);
  if (promise.isCanceled())
    return;

  result.statusMessage =
      ReportService::buildStatusMessage(report, overview, capabilityGraph,
                                        layoutResult);
  result.nodeItems = sceneData.nodeItems;
  result.edgeItems = sceneData.edgeItems;
  result.groupItems = sceneData.groupItems;
  result.sceneWidth = sceneData.sceneWidth;
  result.sceneHeight = sceneData.sceneHeight;
  result.analysisReport = formatCapabilityReportMarkdown(report, capabilityGraph);
  result.systemContextReport = formatSystemContextReportMarkdown(capabilityGraph);
  result.systemContextData = ReportService::buildSystemContextData(
      report, overview, capabilityGraph, cleanedPath);
  result.systemContextCards = ReportService::buildSystemContextCards(
      report, overview, capabilityGraph);
}

void clearSceneData(PendingAnalysisResult &result) {
  result.astFileItems.clear();
  result.selectedAstFilePath.clear();
  result.astPreviewTitle = QStringLiteral("AST 预览");
  result.astPreviewSummary = QStringLiteral(
      "完成项目分析后，会在这里展示所选源码文件的 Tree-sitter AST。");
  result.astPreviewText = QStringLiteral(
      "当前还没有可预览的源码文件。\n\n先执行一次项目分析，然后从文件下拉框里选"
      "择想查看的 C++ "
      "源文件。\n界面会展示该文件的语法树层级、节点范围和基础统计信息。");
  result.nodeItems.clear();
  result.edgeItems.clear();
  result.groupItems.clear();
  result.sceneWidth = 0.0;
  result.sceneHeight = 0.0;
  result.systemContextData.clear();
  result.systemContextCards.clear();
}

bool shouldAbortAnalysis(QPromise<void> &promise, PendingAnalysisResult &result,
                         const QString &phaseLabel = QString()) {
  if (!promise.isCanceled())
    return false;
  result.canceled = true;
  result.statusMessage =
      phaseLabel.isEmpty()
          ? QStringLiteral("已停止当前分析。你可以重新开始。")
          : QStringLiteral("已在“%1”阶段停止当前分析。你可以重新开始。")
                .arg(phaseLabel);
  result.analysisReport = QStringLiteral(
      "本次分析已手动停止。\n\n为了避免把未完成的中间结果展示成最终结论，当前界"
      "面不会保留这次分析的半成品。\n你可以直接再次开始分析。");
  clearSceneData(result);
  return true;
}

void runAnalysisTask(QPromise<void> &promise, const QString &cleanedPath,
                     const std::shared_ptr<PendingAnalysisResult> &output) {
  PendingAnalysisResult result;
  clearSceneData(result);
  try {
    promise.setProgressRange(0, 100);
    promise.setProgressValueAndText(5, QStringLiteral("扫描项目目录..."));
    if (shouldAbortAnalysis(promise, result, QStringLiteral("扫描项目目录"))) {
      if (output)
        *output = std::move(result);
      return;
    }

    analyzer::AnalyzerOptions options;
    options.precision = analyzer::AnalyzerPrecision::SemanticPreferred;
    options.cancellationRequested = [&promise]() {
      return promise.isCanceled();
    };

    promise.setProgressValueAndText(15, QStringLiteral("检测编译数据库..."));
    if (const auto compilationDatabasePath =
            locateCompilationDatabase(cleanedPath);
        compilationDatabasePath.has_value()) {
      options.compilationDatabasePath = *compilationDatabasePath;
    }
    if (shouldAbortAnalysis(promise, result,
                            QStringLiteral("检测编译数据库"))) {
      if (output)
        *output = std::move(result);
      return;
    }

    promise.setProgressValueAndText(25,
                                    QStringLiteral("解析源码并构建关系图..."));
    analyzer::CppProjectAnalyzer analyzer;
    const auto report =
        analyzer.analyzeProject(cleanedPath.toStdString(), options);
    if (shouldAbortAnalysis(promise, result,
                            QStringLiteral("解析源码并构建关系图"))) {
      if (output)
        *output = std::move(result);
      return;
    }

    promise.setProgressValueAndText(75, QStringLiteral("提炼架构总览..."));
    const auto overview = core::buildArchitectureOverview(report);
    if (shouldAbortAnalysis(promise, result, QStringLiteral("提炼架构总览"))) {
      if (output)
        *output = std::move(result);
      return;
    }

    promise.setProgressValueAndText(85, QStringLiteral("生成能力视图..."));
    const auto capabilityGraph = core::buildCapabilityGraph(report, overview);
    if (shouldAbortAnalysis(promise, result, QStringLiteral("生成能力视图"))) {
      if (output)
        *output = std::move(result);
      return;
    }

    promise.setProgressValueAndText(92, QStringLiteral("计算模块布局..."));
    layout::LayeredGraphLayout layoutEngine;
    const auto layoutResult = layoutEngine.layoutModules(report);
    if (shouldAbortAnalysis(promise, result, QStringLiteral("计算模块布局"))) {
      if (output)
        *output = std::move(result);
      return;
    }

    promise.setProgressValueAndText(96, QStringLiteral("整理 AST 预览..."));
    result.astFileItems = buildAstFileItems(report);
    result.selectedAstFilePath = chooseDefaultAstFilePath(result.astFileItems);
    const auto astPreview =
        buildAstPreview(cleanedPath, result.selectedAstFilePath);
    result.astPreviewTitle = astPreview.title;
    result.astPreviewSummary = astPreview.summary;
    result.astPreviewText = astPreview.text;
    if (shouldAbortAnalysis(promise, result, QStringLiteral("整理 AST 预览"))) {
      if (output)
        *output = std::move(result);
      return;
    }

    promise.setProgressValueAndText(98, QStringLiteral("整理可视化数据..."));
    const auto sceneData = buildCapabilitySceneData(capabilityGraph);
    if (shouldAbortAnalysis(promise, result,
                            QStringLiteral("整理可视化数据"))) {
      if (output)
        *output = std::move(result);
      return;
    }

    result.statusMessage = ReportService::buildStatusMessage(
        report, overview, capabilityGraph, layoutResult);
    result.nodeItems = sceneData.nodeItems;
    result.edgeItems = sceneData.edgeItems;
    result.groupItems = sceneData.groupItems;
    result.sceneWidth = sceneData.sceneWidth;
    result.sceneHeight = sceneData.sceneHeight;
    result.analysisReport = formatCapabilityReportMarkdown(report, capabilityGraph);
    result.systemContextReport =
        formatSystemContextReportMarkdown(capabilityGraph);
    result.systemContextData = ReportService::buildSystemContextData(
        report, overview, capabilityGraph, cleanedPath);
    result.systemContextCards = ReportService::buildSystemContextCards(
        report, overview, capabilityGraph);
    promise.setProgressValueAndText(
        100, QStringLiteral("分析完成，正在准备界面..."));
  } catch (const std::exception &exception) {
    result.statusMessage =
        QStringLiteral("分析失败：%1").arg(QString::fromUtf8(exception.what()));
    result.analysisReport =
        QStringLiteral("分析过程中发生异常。\n\n错误信息：%"
                       "1\n\n请检查项目路径、源码文件和当前构建环境是否可用。")
            .arg(QString::fromUtf8(exception.what()));
    clearSceneData(result);
  } catch (...) {
    result.statusMessage = QStringLiteral("分析失败：发生未知异常。");
    result.analysisReport =
        QStringLiteral("分析过程中发生未知异常。\n\n请检查项目路径、源码文件和"
                       "当前构建环境是否可用。");
    clearSceneData(result);
  }
  if (output)
    *output = std::move(result);
}

QString buildAiSetupMessage(const ai::DeepSeekConfigLoadResult &loadResult) {
  if (!loadResult.loadedFromFile)
    return QStringLiteral("AI 未就绪：%1").arg(loadResult.errorMessage);
  if (!loadResult.config.enabled)
    return QStringLiteral("已找到 AI 配置，但当前未启用。把 %1 里的 enabled "
                          "改成 true 后即可使用。")
        .arg(QDir::toNativeSeparators(loadResult.configPath));
  if (!loadResult.config.isUsable())
    return QStringLiteral(
        "AI 配置已加载，但还不能使用。请检查模型、地址、endpointPath 和 API "
        "Key 是否完整。");
  return QStringLiteral("%1 已就绪，当前模型为 %2，请求地址为 %3。AI "
                        "只会基于当前项目或节点的证据包进行解读。")
      .arg(loadResult.config.providerLabel(), loadResult.config.model,
           loadResult.config.resolvedChatCompletionsUrl());
}

QString extractApiErrorMessage(const QByteArray &bytes) {
  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject())
    return {};
  const QJsonObject object = document.object();
  if (!object.contains(QStringLiteral("error")) ||
      !object.value(QStringLiteral("error")).isObject())
    return {};
  return object.value(QStringLiteral("error"))
      .toObject()
      .value(QStringLiteral("message"))
      .toString()
      .trimmed();
}

ai::ArchitectureAssistantRequest
buildAiRequest(const QString &projectRootPath, const QString &analysisReport,
               const QString &statusMessage, const QString &analysisPhase,
               const QVariantMap &nodeData, const QString &userTask) {
  ai::ArchitectureAssistantRequest request;
  request.projectName = projectNameFromRootPath(projectRootPath);
  request.projectRootPath = QDir::toNativeSeparators(projectRootPath);
  request.analyzerPrecision = inferAnalyzerPrecision(analysisReport);
  request.analysisSummary = statusMessage.trimmed();
  request.nodeName =
      nodeData.value(QStringLiteral("name")).toString().trimmed();
  request.nodeKind =
      nodeData.value(QStringLiteral("kind")).toString().trimmed();
  request.nodeRole =
      nodeData.value(QStringLiteral("role")).toString().trimmed();
  request.nodeSummary =
      nodeData.value(QStringLiteral("summary")).toString().trimmed();
  if (request.nodeSummary.isEmpty())
    request.nodeSummary =
        nodeData.value(QStringLiteral("responsibility")).toString().trimmed();
  request.userTask =
      userTask.trimmed().isEmpty()
          ? QStringLiteral(
                "You are a senior multilingual software architect across C++, Java, Python, JavaScript, Go, and related ecosystems.\n"
                "Analyze the selected module from the supplied static evidence only. Do not overfit to one keyword; for example, Node could mean Node.js, a graph node, a UI tree node, or a data structure.\n"
                "Use module names, file extensions, import/include hints, core symbols, collaborators, and project context together before deciding the architecture role.\n"
                "When evidence is weak or ambiguous, explain that explicitly in uncertainty and keep the conclusion conservative.\n"
                "Focus on what the module does, who it collaborates with, what evidence supports the judgment, and what remains uncertain.")
          : userTask.trimmed();
  request.moduleNames =
      toQStringList(nodeData.value(QStringLiteral("moduleNames")));
  request.exampleFiles =
      toQStringList(nodeData.value(QStringLiteral("exampleFiles")));
  request.topSymbols =
      toQStringList(nodeData.value(QStringLiteral("topSymbols")));
  request.collaboratorNames =
      toQStringList(nodeData.value(QStringLiteral("collaboratorNames")));
  request.diagnostics = {QStringLiteral("analysis phase: %1")
                             .arg(analysisPhase.trimmed().isEmpty()
                                      ? QStringLiteral("unknown")
                                      : analysisPhase.trimmed())};
  if (!analysisReport.trimmed().isEmpty() &&
      analysisReport.contains(QStringLiteral("语法"),
                              Qt::CaseInsensitive))
    request.diagnostics.push_back(
        QStringLiteral("analysis precision may be syntax-fallback"));
  request.diagnostics.push_back(QStringLiteral(
      "language inference hint: inspect file suffixes, import/include evidence, and symbol naming before assigning a role"));
  return request;
}

ai::ArchitectureAssistantRequest buildProjectAiRequest(
    const QString &projectRootPath, const QString &analysisReport,
    const QString &statusMessage, const QString &analysisPhase,
    const QVariantMap &systemContextData,
    const QVariantList &systemContextCards,
    const QVariantList &capabilityNodeItems, const QString &userTask) {
  ai::ArchitectureAssistantRequest request;
  request.projectName = projectNameFromRootPath(projectRootPath);
  request.projectRootPath = QDir::toNativeSeparators(projectRootPath);
  request.analyzerPrecision = inferAnalyzerPrecision(analysisReport);
  request.analysisSummary = statusMessage.trimmed();
  request.nodeName =
      systemContextData.value(QStringLiteral("title")).toString().trimmed();
  if (request.nodeName.isEmpty())
    request.nodeName =
        QStringLiteral("%1 / 系统上下文").arg(request.projectName);
  request.nodeKind = QStringLiteral("system_context");
  request.nodeRole = QStringLiteral("project_context");
  QStringList summaryParts;
  summaryParts << systemContextData.value(QStringLiteral("headline"))
                      .toString()
                      .trimmed()
               << systemContextData.value(QStringLiteral("purposeSummary"))
                      .toString()
                      .trimmed()
               << systemContextData.value(QStringLiteral("entrySummary"))
                      .toString()
                      .trimmed()
               << systemContextData.value(QStringLiteral("inputSummary"))
                      .toString()
                      .trimmed()
               << systemContextData.value(QStringLiteral("outputSummary"))
                      .toString()
                      .trimmed()
               << systemContextData.value(QStringLiteral("containerSummary"))
                      .toString()
                      .trimmed();
  for (const QVariant &value : systemContextCards) {
    const QVariantMap item = value.toMap();
    const QString name =
        item.value(QStringLiteral("name")).toString().trimmed();
    const QString summary =
        item.value(QStringLiteral("summary")).toString().trimmed();
    if (!name.isEmpty() && !summary.isEmpty())
      summaryParts << QStringLiteral("%1：%2").arg(name, summary);
  }
  request.nodeSummary =
      deduplicateQStringList(summaryParts).join(QStringLiteral(" "));
  request.userTask =
      userTask.trimmed().isEmpty()
          ? QStringLiteral("请像 C4 模型的 L1 "
                           "系统上下文一样，总结这个项目：谁会使用它、它大致在"
                           "解决什么问题、会接收哪些输入、主要部分是什么。语言"
                           "要简体中文、适合非程序员一眼看懂。")
          : userTask.trimmed();
  request.moduleNames =
      toQStringList(systemContextData.value(QStringLiteral("containerNames")));
  request.collaboratorNames =
      toQStringList(systemContextData.value(QStringLiteral("containerNames")));
  request.diagnostics = {
      QStringLiteral("analysis phase: %1")
          .arg(analysisPhase.trimmed().isEmpty() ? QStringLiteral("unknown")
                                                 : analysisPhase.trimmed()),
      QStringLiteral("ai scope: system_context"),
      QStringLiteral("technology summary: %1")
          .arg(systemContextData.value(QStringLiteral("technologySummary"))
                   .toString()
                   .trimmed())};
  for (const QVariant &value : capabilityNodeItems) {
    const QVariantMap node = value.toMap();
    for (const QString &path :
         toQStringList(node.value(QStringLiteral("exampleFiles")))) {
      if (!request.exampleFiles.contains(path) &&
          request.exampleFiles.size() < 8)
        request.exampleFiles.push_back(path);
    }
    for (const QString &symbol :
         toQStringList(node.value(QStringLiteral("topSymbols")))) {
      if (!request.topSymbols.contains(symbol) && request.topSymbols.size() < 8)
        request.topSymbols.push_back(symbol);
    }
  }
  return request;
}

} // namespace

AnalysisController::AnalysisController(QObject *parent)
    : QObject(parent), m_aiNetworkManager(new QNetworkAccessManager(this)),
      m_analysisWatcher(new QFutureWatcher<void>(this)) {
  setProjectRootPathInternal(AnalysisOrchestrator::defaultProjectRootPath(),
                             false);
  setStatusMessage(QStringLiteral("准备就绪。选择一个项目目录后，就可以先看项目"
                                  "分工，再按需进入程序员视图。"));
  setAnalysisReport(QStringLiteral(
      "程序员入口的详细分析报告会显示在这里。\n\n完成项目分析后，你可以在右侧切"
      "换到“程序员入口”，查看完整报告和 AST 预览。"));
  setAnalysisPhase(QStringLiteral("等待开始"));
  clearVisualizationState();
  refreshAiAvailability();

  connect(m_analysisWatcher, &QFutureWatcher<void>::progressValueChanged, this,
          [this](const int progressValue) {
            setAnalysisProgress(std::clamp(
                static_cast<double>(progressValue) / 100.0, 0.0, 1.0));
          });
  connect(m_analysisWatcher, &QFutureWatcher<void>::progressTextChanged, this,
          [this](const QString &progressText) {
            if (progressText.isEmpty() || m_stopRequested)
              return;
            setAnalysisPhase(progressText);
            setStatusMessage(progressText);
          });
  connect(m_analysisWatcher, &QFutureWatcher<void>::finished, this,
          [this]() { finishAnalysis(); });
}

QString AnalysisController::projectRootPath() const {
  return m_projectRootPath;
}
void AnalysisController::setProjectRootPath(const QString &value) {
  setProjectRootPathInternal(QDir::cleanPath(value.trimmed()), true);
}
QString AnalysisController::statusMessage() const { return m_statusMessage; }
QString AnalysisController::analysisReport() const { return m_analysisReport; }
QVariantList AnalysisController::astFileItems() const { return m_astFileItems; }
QString AnalysisController::selectedAstFilePath() const {
  return m_selectedAstFilePath;
}
QString AnalysisController::astPreviewTitle() const {
  return m_astPreviewTitle;
}
QString AnalysisController::astPreviewSummary() const {
  return m_astPreviewSummary;
}
QString AnalysisController::astPreviewText() const { return m_astPreviewText; }
QVariantList AnalysisController::capabilityNodeItems() const {
  return m_capabilityNodeItems;
}
QVariantList AnalysisController::capabilityEdgeItems() const {
  return m_capabilityEdgeItems;
}
QVariantList AnalysisController::capabilityGroupItems() const {
  return m_capabilityGroupItems;
}
double AnalysisController::capabilitySceneWidth() const {
  return m_capabilitySceneWidth;
}
double AnalysisController::capabilitySceneHeight() const {
  return m_capabilitySceneHeight;
}
bool AnalysisController::analyzing() const { return m_analyzing; }
bool AnalysisController::stopRequested() const { return m_stopRequested; }
double AnalysisController::analysisProgress() const {
  return m_analysisProgress;
}
QString AnalysisController::analysisPhase() const { return m_analysisPhase; }
QVariantMap AnalysisController::systemContextData() const {
  return m_systemContextData;
}
QVariantList AnalysisController::systemContextCards() const {
  return m_systemContextCards;
}
bool AnalysisController::aiAvailable() const { return m_aiAvailable; }
QString AnalysisController::aiConfigPath() const { return m_aiConfigPath; }
QString AnalysisController::aiSetupMessage() const { return m_aiSetupMessage; }
bool AnalysisController::aiBusy() const { return m_aiBusy; }
bool AnalysisController::aiHasResult() const { return m_aiHasResult; }
QString AnalysisController::aiNodeName() const { return m_aiNodeName; }
QString AnalysisController::aiSummary() const { return m_aiSummary; }
QString AnalysisController::aiResponsibility() const {
  return m_aiResponsibility;
}
QString AnalysisController::aiUncertainty() const { return m_aiUncertainty; }
QVariantList AnalysisController::aiCollaborators() const {
  return m_aiCollaborators;
}
QVariantList AnalysisController::aiEvidence() const { return m_aiEvidence; }
QVariantList AnalysisController::aiNextActions() const {
  return m_aiNextActions;
}
QString AnalysisController::aiStatusMessage() const {
  return m_aiStatusMessage;
}
QString AnalysisController::aiScope() const { return m_aiScope; }

void AnalysisController::setSelectedAstFilePath(const QString &value) {
  const QString cleanedValue = QDir::fromNativeSeparators(value.trimmed());
  if (m_selectedAstFilePath == cleanedValue)
    return;
  setSelectedAstFilePathInternal(cleanedValue, true);
  refreshAstPreview();
}

void AnalysisController::analyzeCurrentProject() {
  analyzeProject(m_projectRootPath);
}

void AnalysisController::stopAnalysis() {
  if (!m_analyzing || m_stopRequested)
    return;
  setStopRequested(true);
  setAnalysisPhase(QStringLiteral("正在停止..."));
  setStatusMessage(
      QStringLiteral("已发出停止请求，会在当前阶段结束后安全退出。"));
  if (m_analysisWatcher != nullptr)
    m_analysisWatcher->cancel();
}

void AnalysisController::analyzeProject(const QString &projectRootPath) {
  const QString cleanedPath = QDir::cleanPath(projectRootPath.trimmed());
  if (cleanedPath.isEmpty()) {
    setStatusMessage(QStringLiteral("请先选择一个项目目录。"));
    return;
  }
  const QFileInfo info(cleanedPath);
  if (!info.exists() || !info.isDir()) {
    setProjectRootPathInternal(cleanedPath, true);
    clearVisualizationState();
    setStatusMessage(QStringLiteral("项目目录不存在或不可访问：%1")
                         .arg(QDir::toNativeSeparators(cleanedPath)));
    setAnalysisReport(
        QStringLiteral(
            "无法开始分析，因为项目目录不存在或不可访问。\n\n路径：%1")
            .arg(QDir::toNativeSeparators(cleanedPath)));
    setAnalysisPhase(QStringLiteral("等待开始"));
    return;
  }
  if (m_analyzing) {
    setStatusMessage(
        QStringLiteral("当前已经有分析在进行，请先等待完成或点击停止。"));
    return;
  }
  setProjectRootPathInternal(cleanedPath, true);
  beginAnalysis(cleanedPath);
}

void AnalysisController::analyzeProjectUrl(const QUrl &projectRootUrl) {
  if (projectRootUrl.isLocalFile())
    analyzeProject(projectRootUrl.toLocalFile());
  else
    analyzeProject(projectRootUrl.toString());
}

void AnalysisController::copyCodeContextToClipboard(const qulonglong nodeId) {
  QVariantMap targetNode;
  bool found = false;
  for (const QVariant &item : m_capabilityNodeItems) {
    QVariantMap node = item.toMap();
    if (node.value(QStringLiteral("id")).toULongLong() == nodeId) {
      targetNode = node;
      found = true;
      break;
    }
  }
  if (!found)
    return;

  const QString name = targetNode.value(QStringLiteral("name")).toString();
  const QString responsibility =
      targetNode.value(QStringLiteral("responsibility")).toString();
  const QString kind = targetNode.value(QStringLiteral("kind")).toString();
  const QStringList exampleFiles =
      toQStringList(targetNode.value(QStringLiteral("exampleFiles")));
  const QStringList topSymbols =
      toQStringList(targetNode.value(QStringLiteral("topSymbols")));
  const QStringList collaborators =
      toQStringList(targetNode.value(QStringLiteral("collaboratorNames")));

  const QString kindLabel =
      (kind == QStringLiteral("entry"))
          ? QStringLiteral("\u53d1\u8d77\u5165\u53e3\uff08Entry\uff09")
          : (kind == QStringLiteral("infrastructure"))
                ? QStringLiteral("\u540e\u53f0\u652f\u6491\uff08Infrastructure\uff09")
                : QStringLiteral("\u6838\u5fc3\u80fd\u529b\uff08Capability\uff09");

  QString prompt = QStringLiteral(
      "\u6211\u73b0\u5728\u9700\u8981\u4fee\u6539\u3010%1\u3011\u6a21\u5757\uff08\u67b6\u6784\u89d2\u8272\uff1a%2\uff09\u7684\u4ee3\u7801\u3002"
      "\u4ee5\u4e0b\u662f\u7531\u9759\u6001\u5206\u6790\u5f15\u64ce\u63d0\u53d6\u7684\u67b6\u6784\u4e0a\u4e0b\u6587\uff0c\u8bf7\u5148\u9605\u8bfb\u7406\u89e3\uff0c\u7a0d\u540e\u6211\u4f1a\u7ed9\u51fa\u5177\u4f53\u6307\u4ee4\u3002\n\n")
      .arg(name, kindLabel);

  prompt += QStringLiteral("### \U0001F3AF \u6a21\u5757\u6838\u5fc3\u804c\u8d23\n%1\n\n").arg(responsibility);

  if (!exampleFiles.isEmpty()) {
    prompt += QStringLiteral("### \U0001F4C1 \u6838\u5fc3\u5173\u8054\u6587\u4ef6\n");
    for (const QString &f : exampleFiles)
      prompt += QStringLiteral("- `%1`\n").arg(f);
    prompt += '\n';
  }
  if (!topSymbols.isEmpty()) {
    prompt += QStringLiteral("### \U0001F9F1 \u6838\u5fc3\u7c7b\u4e0e\u5173\u952e\u7b26\u53f7\uff08AST \u63d0\u53d6\uff09\n");
    for (const QString &s : topSymbols)
      prompt += QStringLiteral("- `%1`\n").arg(s);
    prompt += '\n';
  }
  if (!collaborators.isEmpty()) {
    prompt += QStringLiteral("### \U0001F91D \u76f4\u63a5\u534f\u4f5c\u6a21\u5757\n");
    for (const QString &c : collaborators)
      prompt += QStringLiteral("- `%1`\n").arg(c);
    prompt += '\n';
  }
  prompt += QStringLiteral("---\n*\u8bf7\u56de\u590d\"\u5df2\u4e86\u89e3\u4e0a\u4e0b\u6587\"\u540e\u518d\u7b49\u5f85\u6211\u7684\u5177\u4f53\u6307\u4ee4\u3002*");

  if (QClipboard *clipboard = QGuiApplication::clipboard()) {
    clipboard->setText(prompt);
    setStatusMessage(QStringLiteral("\u2705 \u5df2\u590d\u5236\u300c%1\u300d\u7684\u4e0a\u4e0b\u6587 Prompt\uff0c\u7c98\u8d34\u8fdb Cursor / Claude \u5373\u53ef").arg(name));
  }
}

void AnalysisController::copyTextToClipboard(const QString &text) {
  if (text.trimmed().isEmpty())
    return;
  if (QClipboard *clipboard = QGuiApplication::clipboard(); clipboard != nullptr)
    clipboard->setText(text);
}

void AnalysisController::refreshAiAvailability() {
  applyAiAvailability(AiService::inspectAvailability());
  if (!m_aiBusy && !m_aiHasResult)
    setAiStatusMessage(m_aiSetupMessage);
}

void AnalysisController::clearAiExplanation() {
  cancelActiveAiReply();
  resetAiState(true);
}

void AnalysisController::requestAiExplanation(const QVariantMap &nodeData,
                                              const QString &userTask) {
  const AiPreparedRequest prepared = AiService::prepareNodeRequest(
      AiRequestContext{m_projectRootPath, m_analysisReport, m_statusMessage,
                       m_analysisPhase, userTask},
      nodeData);
  applyAiAvailability(prepared.availability);
  if (!prepared.ready) {
    setAiStatusMessage(prepared.failureStatusMessage);
    return;
  }
  cancelActiveAiReply();
  resetAiState(false);
  setAiScope(prepared.scope);
  setAiNodeName(prepared.targetName);
  setAiBusy(true);
  setAiStatusMessage(prepared.pendingStatusMessage);
  AiService::logRequest(
      prepared,
      QStringLiteral("%1 (%2)")
          .arg(nodeData.value(QStringLiteral("id")).toULongLong())
          .arg(prepared.targetName));
  m_aiReply =
      m_aiNetworkManager->post(prepared.networkRequest, prepared.payload);
  connect(m_aiReply, &QNetworkReply::finished, this,
          [this, reply = m_aiReply]() { finishAiReply(reply); });
}

void AnalysisController::requestProjectAiExplanation(const QString &userTask) {
  const AiPreparedRequest prepared = AiService::prepareProjectRequest(
      AiRequestContext{m_projectRootPath, m_analysisReport, m_statusMessage,
                       m_analysisPhase, userTask},
      m_systemContextData, m_systemContextCards, m_capabilityNodeItems);
  applyAiAvailability(prepared.availability);
  if (!prepared.ready) {
    setAiStatusMessage(prepared.failureStatusMessage);
    return;
  }
  cancelActiveAiReply();
  resetAiState(false);
  setAiScope(prepared.scope);
  setAiNodeName(prepared.targetName);
  setAiBusy(true);
  setAiStatusMessage(prepared.pendingStatusMessage);
  AiService::logRequest(prepared, prepared.targetName);
  m_aiReply =
      m_aiNetworkManager->post(prepared.networkRequest, prepared.payload);
  connect(m_aiReply, &QNetworkReply::finished, this,
          [this, reply = m_aiReply]() { finishAiReply(reply); });
}

void AnalysisController::beginAnalysis(const QString &cleanedPath) {
  clearVisualizationState();
  setAnalyzing(true);
  setStopRequested(false);
  setAnalysisProgress(0.0);
  setAnalysisPhase(QStringLiteral("准备分析..."));
  setStatusMessage(QStringLiteral("准备分析..."));
  setAnalysisReport(QStringLiteral("正在分析项目：%"
                                   "1\n\n系统会先整理项目的主要分工，再为程序员"
                                   "准备详细分析报告和 AST 预览。")
                        .arg(QDir::toNativeSeparators(cleanedPath)));
  m_pendingAnalysisResult = std::make_shared<PendingAnalysisResult>();
  const auto resultAssembler = populatePendingAnalysisResult;
  m_analysisWatcher->setFuture(QtConcurrent::run(
      [cleanedPath, pendingAnalysisResult = m_pendingAnalysisResult,
       resultAssembler](QPromise<void> &promise) {
        AnalysisOrchestrator::run(promise, cleanedPath, resultAssembler,
                                  pendingAnalysisResult);
      }));
}

void AnalysisController::finishAnalysis() {
  const bool canceled =
      m_stopRequested || (m_analysisWatcher != nullptr &&
                          m_analysisWatcher->future().isCanceled());
  setAnalyzing(false);
  const auto result = m_pendingAnalysisResult;
  m_pendingAnalysisResult.reset();
  if (!result) {
    setStopRequested(false);
    setAnalysisProgress(0.0);
    setAnalysisPhase(canceled ? QStringLiteral("已停止")
                              : QStringLiteral("分析失败"));
    setStatusMessage(canceled
                         ? QStringLiteral("当前分析已停止。")
                         : QStringLiteral("分析任务结束，但没有拿到结果。"));
    clearVisualizationState();
    return;
  }
  setStopRequested(false);
  if (canceled || result->canceled) {
    setAnalysisProgress(0.0);
    setAnalysisPhase(QStringLiteral("已停止"));
  } else {
    setAnalysisProgress(1.0);
    setAnalysisPhase(QStringLiteral("分析完成"));
  }
  setStatusMessage(result->statusMessage.isEmpty()
                       ? (canceled ? QStringLiteral("当前分析已停止。")
                                   : QStringLiteral("分析完成。"))
                       : result->statusMessage);
  setAnalysisReport(result->analysisReport.isEmpty()
                        ? QStringLiteral("没有可显示的分析报告。")
                        : result->analysisReport);
  setAstFileItems(result->astFileItems);
  setSelectedAstFilePathInternal(result->selectedAstFilePath, true);
  setAstPreviewTitle(result->astPreviewTitle);
  setAstPreviewSummary(result->astPreviewSummary);
  setAstPreviewText(result->astPreviewText);
  setCapabilityNodeItems(result->nodeItems);
  setCapabilityEdgeItems(result->edgeItems);
  if (m_systemContextReport != result->systemContextReport) {
    m_systemContextReport = result->systemContextReport;
    emit systemContextReportChanged();
  }
  setCapabilityGroupItems(result->groupItems);
  setCapabilitySceneWidth(result->sceneWidth);
  setCapabilitySceneHeight(result->sceneHeight);
  setSystemContextData(result->systemContextData);
  setSystemContextCards(result->systemContextCards);
}

void AnalysisController::clearVisualizationState() {
  clearAiExplanation();
  setAstFileItems({});
  setSelectedAstFilePathInternal({}, true);
  setAstPreviewTitle(QStringLiteral("AST 预览"));
  setAstPreviewSummary(QStringLiteral(
      "完成项目分析后，会在这里展示所选源码文件的 Tree-sitter AST。"));
  setAstPreviewText(QStringLiteral(
      "当前还没有可预览的源码文件。\n\n先执行一次项目分析，然后从文件下拉框里选"
      "择想查看的 C++ "
      "源文件。\n界面会展示该文件的语法树层级、节点范围和基础统计信息。"));
  const auto preview = AstPreviewService::buildEmptyPreview();
  setAstPreviewTitle(preview.title);
  setAstPreviewSummary(preview.summary);
  setAstPreviewText(preview.text);
  setCapabilityNodeItems({});
  setCapabilityEdgeItems({});
  setCapabilityGroupItems({});
  setCapabilitySceneWidth(0.0);
  setCapabilitySceneHeight(0.0);
  setSystemContextData({});
  setSystemContextCards({});
  if (!m_systemContextReport.isEmpty()) {
    m_systemContextReport.clear();
    emit systemContextReportChanged();
  }
}

void AnalysisController::setProjectRootPathInternal(QString value,
                                                    const bool emitSignal) {
  if (value.isEmpty())
    value = AnalysisOrchestrator::defaultProjectRootPath();
  if (m_projectRootPath == value)
    return;
  m_projectRootPath = std::move(value);
  if (emitSignal)
    emit projectRootPathChanged();
}

void AnalysisController::setSelectedAstFilePathInternal(QString value,
                                                        const bool emitSignal) {
  value = QDir::fromNativeSeparators(value.trimmed());
  if (m_selectedAstFilePath == value)
    return;
  m_selectedAstFilePath = std::move(value);
  if (emitSignal)
    emit selectedAstFilePathChanged();
}

void AnalysisController::setStatusMessage(QString value) {
  if (m_statusMessage == value)
    return;
  m_statusMessage = std::move(value);
  emit statusMessageChanged();
}
void AnalysisController::setAnalysisReport(QString value) {
  if (m_analysisReport == value)
    return;
  m_analysisReport = std::move(value);
  emit analysisReportChanged();
}
void AnalysisController::setAstFileItems(QVariantList value) {
  if (m_astFileItems == value)
    return;
  m_astFileItems = std::move(value);
  emit astFileItemsChanged();
}
void AnalysisController::setAstPreviewTitle(QString value) {
  if (m_astPreviewTitle == value)
    return;
  m_astPreviewTitle = std::move(value);
  emit astPreviewTitleChanged();
}
void AnalysisController::setAstPreviewSummary(QString value) {
  if (m_astPreviewSummary == value)
    return;
  m_astPreviewSummary = std::move(value);
  emit astPreviewSummaryChanged();
}
void AnalysisController::setAstPreviewText(QString value) {
  if (m_astPreviewText == value)
    return;
  m_astPreviewText = std::move(value);
  emit astPreviewTextChanged();
}
void AnalysisController::setCapabilityNodeItems(QVariantList value) {
  if (m_capabilityNodeItems == value)
    return;
  m_capabilityNodeItems = std::move(value);
  emit capabilityNodeItemsChanged();
}
void AnalysisController::setCapabilityEdgeItems(QVariantList value) {
  if (m_capabilityEdgeItems == value)
    return;
  m_capabilityEdgeItems = std::move(value);
  emit capabilityEdgeItemsChanged();
}
void AnalysisController::setCapabilityGroupItems(QVariantList value) {
  if (m_capabilityGroupItems == value)
    return;
  m_capabilityGroupItems = std::move(value);
  emit capabilityGroupItemsChanged();
}
void AnalysisController::setCapabilitySceneWidth(const double value) {
  if (m_capabilitySceneWidth == value)
    return;
  m_capabilitySceneWidth = value;
  emit capabilitySceneWidthChanged();
}
void AnalysisController::setCapabilitySceneHeight(const double value) {
  if (m_capabilitySceneHeight == value)
    return;
  m_capabilitySceneHeight = value;
  emit capabilitySceneHeightChanged();
}
void AnalysisController::setAnalyzing(const bool value) {
  if (m_analyzing == value)
    return;
  m_analyzing = value;
  emit analyzingChanged();
}
void AnalysisController::setStopRequested(const bool value) {
  if (m_stopRequested == value)
    return;
  m_stopRequested = value;
  emit stopRequestedChanged();
}
void AnalysisController::setAnalysisProgress(const double value) {
  if (qFuzzyCompare(m_analysisProgress, value))
    return;
  m_analysisProgress = value;
  emit analysisProgressChanged();
}
void AnalysisController::setAnalysisPhase(QString value) {
  if (m_analysisPhase == value)
    return;
  m_analysisPhase = std::move(value);
  emit analysisPhaseChanged();
}
void AnalysisController::setSystemContextData(QVariantMap value) {
  if (m_systemContextData == value)
    return;
  m_systemContextData = std::move(value);
  emit systemContextDataChanged();
}
void AnalysisController::setSystemContextCards(QVariantList value) {
  if (m_systemContextCards == value)
    return;
  m_systemContextCards = std::move(value);
  emit systemContextCardsChanged();
}

void AnalysisController::refreshAstPreview() {
  const auto preview =
      AstPreviewService::buildPreview(m_projectRootPath, m_selectedAstFilePath);
  setAstPreviewTitle(preview.title);
  setAstPreviewSummary(preview.summary);
  setAstPreviewText(preview.text);
}
void AnalysisController::applyAiAvailability(const AiAvailabilityState &state) {
  setAiConfigPath(state.configPath);
  setAiAvailable(state.available);
  setAiSetupMessage(state.setupMessage);
}
void AnalysisController::applyAiReplyState(const AiReplyState &state) {
  setAiHasResult(state.hasResult);
  setAiStatusMessage(state.statusMessage);
  if (!state.hasResult)
    return;
  setAiSummary(state.summary);
  setAiResponsibility(state.responsibility);
  setAiUncertainty(state.uncertainty);
  setAiCollaborators(state.collaborators);
  setAiEvidence(state.evidence);
  setAiNextActions(state.nextActions);
}
void AnalysisController::cancelActiveAiReply() {
  if (m_aiReply == nullptr)
    return;
  m_aiReply->disconnect(this);
  m_aiReply->abort();
  m_aiReply->deleteLater();
  m_aiReply = nullptr;
}

void AnalysisController::setAiAvailable(const bool value) {
  if (m_aiAvailable == value)
    return;
  m_aiAvailable = value;
  emit aiAvailableChanged();
}
void AnalysisController::setAiConfigPath(QString value) {
  if (m_aiConfigPath == value)
    return;
  m_aiConfigPath = std::move(value);
  emit aiConfigPathChanged();
}
void AnalysisController::setAiSetupMessage(QString value) {
  if (m_aiSetupMessage == value)
    return;
  m_aiSetupMessage = std::move(value);
  emit aiSetupMessageChanged();
}
void AnalysisController::setAiBusy(const bool value) {
  if (m_aiBusy == value)
    return;
  m_aiBusy = value;
  emit aiBusyChanged();
}
void AnalysisController::setAiHasResult(const bool value) {
  if (m_aiHasResult == value)
    return;
  m_aiHasResult = value;
  emit aiHasResultChanged();
}
void AnalysisController::setAiNodeName(QString value) {
  if (m_aiNodeName == value)
    return;
  m_aiNodeName = std::move(value);
  emit aiNodeNameChanged();
}
void AnalysisController::setAiSummary(QString value) {
  if (m_aiSummary == value)
    return;
  m_aiSummary = std::move(value);
  emit aiSummaryChanged();
}
void AnalysisController::setAiResponsibility(QString value) {
  if (m_aiResponsibility == value)
    return;
  m_aiResponsibility = std::move(value);
  emit aiResponsibilityChanged();
}
void AnalysisController::setAiUncertainty(QString value) {
  if (m_aiUncertainty == value)
    return;
  m_aiUncertainty = std::move(value);
  emit aiUncertaintyChanged();
}
void AnalysisController::setAiCollaborators(QVariantList value) {
  if (m_aiCollaborators == value)
    return;
  m_aiCollaborators = std::move(value);
  emit aiCollaboratorsChanged();
}
void AnalysisController::setAiEvidence(QVariantList value) {
  if (m_aiEvidence == value)
    return;
  m_aiEvidence = std::move(value);
  emit aiEvidenceChanged();
}
void AnalysisController::setAiNextActions(QVariantList value) {
  if (m_aiNextActions == value)
    return;
  m_aiNextActions = std::move(value);
  emit aiNextActionsChanged();
}
void AnalysisController::setAiStatusMessage(QString value) {
  if (m_aiStatusMessage == value)
    return;
  m_aiStatusMessage = std::move(value);
  emit aiStatusMessageChanged();
}
void AnalysisController::setAiScope(QString value) {
  if (m_aiScope == value)
    return;
  m_aiScope = std::move(value);
  emit aiScopeChanged();
}

void AnalysisController::resetAiState(const bool keepSetupMessage) {
  setAiBusy(false);
  setAiHasResult(false);
  setAiNodeName({});
  setAiSummary({});
  setAiResponsibility({});
  setAiUncertainty({});
  setAiCollaborators({});
  setAiEvidence({});
  setAiNextActions({});
  setAiScope({});
  setAiStatusMessage(keepSetupMessage ? m_aiSetupMessage : QString());
}

QString AnalysisController::buildL4MarkdownReport() const {
  return buildL4MarkdownReportFromNodeItems(m_capabilityNodeItems);
}

QString AnalysisController::buildL1SystemContext() const {
  return buildL1SystemContextFromNodeItems(m_capabilityNodeItems);
}
void AnalysisController::finishAiReply(QNetworkReply *reply) {
  if (reply == nullptr)
    return;
  const bool isActiveReply = (reply == m_aiReply);
  if (isActiveReply)
    m_aiReply = nullptr;
  const QByteArray responseBytes = reply->readAll();
  const auto cleanup = [reply]() { reply->deleteLater(); };
  if (!isActiveReply) {
    cleanup();
    return;
  }
  setAiBusy(false);
  AiService::logResponse(responseBytes, m_aiScope);
  applyAiReplyState(AiService::parseReply(
      responseBytes, m_aiScope, reply->error() != QNetworkReply::NoError,
      reply->errorString()));
  cleanup();
}

} // namespace savt::ui
