#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace savt::core {

struct ProjectModuleMergeRule {
    std::string matchPrefix;
    std::string targetModule;
};

struct ProjectRoleOverrideRule {
    std::string matchPrefix;
    std::string role;
};

struct ProjectEntryOverrideRule {
    std::string matchPrefix;
    bool entry = true;
};

struct ProjectDependencyFoldRule {
    std::string matchPrefix;
    bool collapse = true;
    bool hideByDefault = true;
};

struct ProjectAnalysisConfig {
    bool loaded = false;
    std::string configPath;
    std::vector<std::string> diagnostics;
    std::vector<std::string> ignoreDirectories;
    std::vector<ProjectModuleMergeRule> moduleMerges;
    std::vector<ProjectRoleOverrideRule> roleOverrides;
    std::vector<ProjectEntryOverrideRule> entryOverrides;
    std::vector<ProjectDependencyFoldRule> dependencyFoldRules;
};

ProjectAnalysisConfig loadProjectAnalysisConfig(const std::filesystem::path& rootPath);
bool matchesProjectConfigPrefix(std::string_view candidate, std::string_view prefix);
std::string normalizeProjectConfigToken(std::string value);

}  // namespace savt::core
