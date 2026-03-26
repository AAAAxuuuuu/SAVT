#pragma once

#include "savt/core/SourceFile.h"
#include "savt/core/SyntaxTree.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace savt::parser {

struct ParseResult {
    bool succeeded = false;
    std::string errorMessage;
    savt::core::SourceFile sourceFile;
    savt::core::SyntaxTree syntaxTree;
};

class TreeSitterCppParser {
public:
    ParseResult parseFile(const std::filesystem::path& filePath) const;
    ParseResult parseSource(std::string_view source, std::string sourceName = {}) const;
};

}  // namespace savt::parser

