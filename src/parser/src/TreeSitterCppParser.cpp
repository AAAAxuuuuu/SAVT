#include "savt/parser/TreeSitterCppParser.h"

#include <fstream>
#include <iterator>
#include <sstream>

#include <tree_sitter/api.h>
#include <tree-sitter-cpp.h>

namespace savt::parser {
namespace {

savt::core::SourceRange toSourceRange(const TSNode& node) {
    const TSPoint startPoint = ts_node_start_point(node);
    const TSPoint endPoint = ts_node_end_point(node);

    return savt::core::SourceRange{
        ts_node_start_byte(node),
        ts_node_end_byte(node),
        startPoint.row,
        startPoint.column,
        endPoint.row,
        endPoint.column
    };
}

savt::core::SyntaxNode buildSyntaxNode(const TSNode& node) {
    savt::core::SyntaxNode syntaxNode;
    syntaxNode.type = ts_node_type(node);
    syntaxNode.range = toSourceRange(node);

    const std::uint32_t childCount = ts_node_child_count(node);
    syntaxNode.children.reserve(childCount);

    for (std::uint32_t index = 0; index < childCount; ++index) {
        syntaxNode.children.push_back(buildSyntaxNode(ts_node_child(node, index)));
    }

    return syntaxNode;
}

std::string readAllText(const std::filesystem::path& filePath) {
    std::ifstream input(filePath, std::ios::binary);
    if (!input.is_open()) {
        return {};
    }

    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

}  // namespace

ParseResult TreeSitterCppParser::parseFile(const std::filesystem::path& filePath) const {
    ParseResult result;
    result.sourceFile.path = filePath.string();
    result.sourceFile.content = readAllText(filePath);

    if (result.sourceFile.content.empty()) {
        result.errorMessage = "Unable to read source file: " + filePath.string();
        return result;
    }

    result = parseSource(result.sourceFile.content, result.sourceFile.path);
    result.sourceFile.path = filePath.string();
    return result;
}

ParseResult TreeSitterCppParser::parseSource(std::string_view source, std::string sourceName) const {
    ParseResult result;
    result.sourceFile.path = std::move(sourceName);
    result.sourceFile.content.assign(source.begin(), source.end());

    TSParser* parser = ts_parser_new();
    if (parser == nullptr) {
        result.errorMessage = "Failed to create tree-sitter parser.";
        return result;
    }

    if (!ts_parser_set_language(parser, tree_sitter_cpp())) {
        ts_parser_delete(parser);
        result.errorMessage = "Failed to bind tree-sitter C++ grammar.";
        return result;
    }

    TSTree* tree = ts_parser_parse_string(
        parser,
        nullptr,
        source.data(),
        static_cast<std::uint32_t>(source.size()));

    if (tree == nullptr) {
        ts_parser_delete(parser);
        result.errorMessage = "tree-sitter returned a null syntax tree.";
        return result;
    }

    const TSNode rootNode = ts_tree_root_node(tree);
    result.syntaxTree.language = "cpp";
    result.syntaxTree.root = buildSyntaxNode(rootNode);
    result.succeeded = true;

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    return result;
}

}  // namespace savt::parser

