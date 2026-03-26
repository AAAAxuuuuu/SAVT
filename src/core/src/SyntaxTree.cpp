#include "savt/core/SyntaxTree.h"

#include <algorithm>
#include <sstream>

namespace savt::core {
namespace {

void appendNode(
    std::ostringstream& output,
    const SyntaxNode& node,
    const std::size_t depth,
    const std::size_t maxDepth,
    const std::size_t maxChildrenPerNode) {
    const std::string indent(depth * 2, ' ');
    output << indent
           << node.type
           << " ["
           << (node.range.startLine + 1)
           << ":"
           << (node.range.startColumn + 1)
           << " -> "
           << (node.range.endLine + 1)
           << ":"
           << (node.range.endColumn + 1)
           << "]\n";

    if (depth >= maxDepth) {
        if (!node.children.empty()) {
            output << indent << "  ...\n";
        }
        return;
    }

    const std::size_t childLimit = std::min(node.children.size(), maxChildrenPerNode);
    for (std::size_t index = 0; index < childLimit; ++index) {
        appendNode(output, node.children[index], depth + 1, maxDepth, maxChildrenPerNode);
    }

    if (node.children.size() > childLimit) {
        output << indent << "  ... " << (node.children.size() - childLimit) << " more children\n";
    }
}

}  // namespace

std::string formatSyntaxTree(
    const SyntaxTree& tree,
    const std::size_t maxDepth,
    const std::size_t maxChildrenPerNode) {
    std::ostringstream output;
    output << "language: " << tree.language << "\n";
    appendNode(output, tree.root, 0, maxDepth, maxChildrenPerNode);
    return output.str();
}

}  // namespace savt::core

