#include "savt/ui/AstPreviewService.h"

#include "savt/core/SyntaxTree.h"
#include "savt/parser/TreeSitterCppParser.h"

#include <QDir>
#include <QFileInfo>
#include <QVariantMap>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <vector>

namespace savt::ui {

namespace {

std::filesystem::path toFilesystemPath(const QString& path) {
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toStdString());
#endif
}

std::size_t countSyntaxNodes(const core::SyntaxNode& node) {
    std::size_t count = 1;
    for (const core::SyntaxNode& child : node.children) {
        count += countSyntaxNodes(child);
    }
    return count;
}

std::size_t computeSyntaxDepth(const core::SyntaxNode& node, const std::size_t currentDepth = 1) {
    std::size_t maxDepth = currentDepth;
    for (const core::SyntaxNode& child : node.children) {
        maxDepth = std::max(maxDepth, computeSyntaxDepth(child, currentDepth + 1));
    }
    return maxDepth;
}

bool isAstPreviewablePath(const QString& relativePath) {
    const QString suffix = QFileInfo(relativePath).suffix().toLower();
    return suffix == QStringLiteral("cpp") || suffix == QStringLiteral("cc") ||
           suffix == QStringLiteral("cxx") || suffix == QStringLiteral("cp") ||
           suffix == QStringLiteral("c++") || suffix == QStringLiteral("c") ||
           suffix == QStringLiteral("ixx") || suffix == QStringLiteral("cppm") ||
           suffix == QStringLiteral("cu") || suffix == QStringLiteral("mm") ||
           suffix == QStringLiteral("hpp") || suffix == QStringLiteral("hh") ||
           suffix == QStringLiteral("hxx") || suffix == QStringLiteral("h++") ||
           suffix == QStringLiteral("h") || suffix == QStringLiteral("ipp") ||
           suffix == QStringLiteral("inl") || suffix == QStringLiteral("tpp") ||
           suffix == QStringLiteral("tcc") || suffix == QStringLiteral("txx") ||
           suffix == QStringLiteral("ii") || suffix == QStringLiteral("cuh");
}

int astPreviewScore(const QString& relativePath) {
    int score = 0;
    if (relativePath.startsWith(QStringLiteral("src/"))) {
        score += 40;
    }
    if (relativePath.startsWith(QStringLiteral("apps/"))) {
        score += 35;
    }
    if (relativePath == QStringLiteral("main.cpp") || relativePath.endsWith(QStringLiteral("/main.cpp")) ||
        relativePath.endsWith(QStringLiteral("/main.ixx")) || relativePath.endsWith(QStringLiteral("/main.cppm"))) {
        score += 30;
    }
    if (relativePath.endsWith(QStringLiteral(".cpp")) || relativePath.endsWith(QStringLiteral(".ixx")) ||
        relativePath.endsWith(QStringLiteral(".cppm"))) {
        score += 10;
    }
    if (relativePath.endsWith(QStringLiteral(".hpp")) || relativePath.endsWith(QStringLiteral(".h")) ||
        relativePath.endsWith(QStringLiteral(".ipp")) || relativePath.endsWith(QStringLiteral(".tpp"))) {
        score += 5;
    }
    if (relativePath.startsWith(QStringLiteral("tests/")) || relativePath.contains(QStringLiteral("/tests/"))) {
        score -= 25;
    }
    if (relativePath.startsWith(QStringLiteral("samples/")) ||
        relativePath.contains(QStringLiteral("/samples/"))) {
        score -= 10;
    }
    return score;
}

}  // namespace

QVariantList AstPreviewService::buildAstFileItems(const core::AnalysisReport& report) {
    std::vector<const core::SymbolNode*> fileNodes;
    for (const core::SymbolNode& node : report.nodes) {
        if (node.kind == core::SymbolKind::File && !node.filePath.empty()) {
            fileNodes.push_back(&node);
        }
    }

    std::sort(fileNodes.begin(), fileNodes.end(), [](const core::SymbolNode* left, const core::SymbolNode* right) {
        return left->filePath < right->filePath;
    });

    QVariantList items;
    for (const core::SymbolNode* node : fileNodes) {
        const QString relativePath = QString::fromStdString(node->filePath);
        if (!isAstPreviewablePath(relativePath)) {
            continue;
        }

        const QString name = QString::fromStdString(
            node->displayName.empty() ? node->qualifiedName : node->displayName);
        QVariantMap item;
        item.insert(QStringLiteral("name"), name);
        item.insert(QStringLiteral("path"), relativePath);
        item.insert(
            QStringLiteral("label"),
            name == relativePath ? relativePath : QStringLiteral("%1 | %2").arg(name, relativePath));
        items.push_back(item);
    }
    return items;
}

QString AstPreviewService::chooseDefaultAstFilePath(const QVariantList& items) {
    QString bestPath;
    int bestScore = std::numeric_limits<int>::min();
    for (const QVariant& value : items) {
        const QVariantMap item = value.toMap();
        const QString path = item.value(QStringLiteral("path")).toString();
        if (path.isEmpty()) {
            continue;
        }

        const int score = astPreviewScore(path);
        if (bestPath.isEmpty() || score > bestScore || (score == bestScore && path < bestPath)) {
            bestPath = path;
            bestScore = score;
        }
    }
    return bestPath;
}

AstPreviewData AstPreviewService::buildEmptyPreview() {
    AstPreviewData preview;
    preview.title = QStringLiteral("AST 预览");
    preview.summary = QStringLiteral("完成项目分析后，会在这里展示所选源码文件的 Tree-sitter AST。");
    preview.text = QStringLiteral(
        "当前还没有可预览的源代码文件。\n\n先执行一次项目分析，然后从文件下拉框里选择想查看的 C++ "
        "源文件。\n界面会展示该文件的语法树层级、节点范围和基础统计信息。");
    return preview;
}

AstPreviewData AstPreviewService::buildPreview(const QString& projectRootPath, const QString& relativePath) {
    if (relativePath.trimmed().isEmpty()) {
        return buildEmptyPreview();
    }

    AstPreviewData preview;
    preview.title = relativePath;

    const QString absolutePath = QDir(projectRootPath).filePath(relativePath);
    const QFileInfo fileInfo(absolutePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        preview.summary = QStringLiteral("无法读取所选源代码文件。");
        preview.text =
            QStringLiteral("文件不存在或不可访问：\n%1").arg(QDir::toNativeSeparators(absolutePath));
        return preview;
    }

    parser::TreeSitterCppParser parser;
    const auto parseResult = parser.parseFile(toFilesystemPath(fileInfo.absoluteFilePath()));
    if (!parseResult.succeeded) {
        preview.summary = QStringLiteral("Tree-sitter 解析失败。");
        preview.text = QString::fromStdString(
            parseResult.errorMessage.empty() ? std::string("Unknown tree-sitter parsing error.")
                                             : parseResult.errorMessage);
        return preview;
    }

    const auto nodeCount = static_cast<qulonglong>(countSyntaxNodes(parseResult.syntaxTree.root));
    const auto depth = static_cast<qulonglong>(computeSyntaxDepth(parseResult.syntaxTree.root));
    const auto rootChildren = static_cast<qulonglong>(parseResult.syntaxTree.root.children.size());
    const auto byteSize = static_cast<qulonglong>(parseResult.sourceFile.content.size());
    preview.summary = QStringLiteral(
                          "语言：C++ | AST 节点：%1 | 最大深度：%2 | 根子节点：%3 | 文件大小：%4 B | "
                          "预览限制：深度 6 / 每层 20 个子节点")
                          .arg(nodeCount)
                          .arg(depth)
                          .arg(rootChildren)
                          .arg(byteSize);
    preview.text = QString::fromStdString(core::formatSyntaxTree(parseResult.syntaxTree, 6, 20));
    return preview;
}

}  // namespace savt::ui
