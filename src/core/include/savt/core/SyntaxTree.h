#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace savt::core {

struct SourceRange {
    std::uint32_t startByte = 0;
    std::uint32_t endByte = 0;
    std::uint32_t startLine = 0;
    std::uint32_t startColumn = 0;
    std::uint32_t endLine = 0;
    std::uint32_t endColumn = 0;
};

struct SyntaxNode {
    std::string type;
    SourceRange range;
    std::vector<SyntaxNode> children;
};

struct SyntaxTree {
    std::string language;
    SyntaxNode root;
};

std::string formatSyntaxTree(
    const SyntaxTree& tree,
    std::size_t maxDepth = 4,
    std::size_t maxChildrenPerNode = 24);

}  // namespace savt::core

