#include "savt/core/ProjectAnalysisConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <sstream>

namespace savt::core {
namespace {

QString toQString(const std::filesystem::path& path) {
    return QString::fromStdString(path.generic_string());
}

std::string normalizeRuleToken(const QString& value) {
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    QString normalized = QDir::fromNativeSeparators(QDir::cleanPath(trimmed));
    while (normalized.startsWith(QStringLiteral("./"))) {
        normalized.remove(0, 2);
    }
    while (normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }
    return normalized.toStdString();
}

QJsonObject configBody(const QJsonObject& object) {
    const QJsonValue nested = object.value(QStringLiteral("analysis"));
    return nested.isObject() ? nested.toObject() : object;
}

std::vector<std::filesystem::path> candidateConfigPaths(const std::filesystem::path& rootPath) {
    return {
        rootPath / "config" / "savt.project.json",
        rootPath / "config" / "savt-project.json",
        rootPath / ".savt" / "project.json",
        rootPath / "savt.project.json"
    };
}

std::string joinCountSummary(const ProjectAnalysisConfig& config) {
    std::ostringstream output;
    output << "ignore=" << config.ignoreDirectories.size()
           << ", merge=" << config.moduleMerges.size()
           << ", role=" << config.roleOverrides.size()
           << ", entry=" << config.entryOverrides.size()
           << ", fold=" << config.dependencyFoldRules.size();
    return output.str();
}

void appendStringList(
    const QJsonObject& object,
    const QString& key,
    std::vector<std::string>& destination,
    std::vector<std::string>& diagnostics) {
    const QJsonValue value = object.value(key);
    if (value.isUndefined()) {
        return;
    }
    if (!value.isArray()) {
        diagnostics.push_back(
            "Project config key '" + key.toStdString() + "' must be an array of strings.");
        return;
    }

    for (const QJsonValue& itemValue : value.toArray()) {
        if (!itemValue.isString()) {
            diagnostics.push_back(
                "Project config key '" + key.toStdString() + "' ignored a non-string item.");
            continue;
        }
        const std::string token = normalizeRuleToken(itemValue.toString());
        if (!token.empty() &&
            std::find(destination.begin(), destination.end(), token) == destination.end()) {
            destination.push_back(token);
        }
    }
}

template <typename Rule>
void appendMatchRules(
    const QJsonObject& object,
    const QString& key,
    const std::function<std::optional<Rule>(const QJsonObject&, std::vector<std::string>&)>& parser,
    std::vector<Rule>& destination,
    std::vector<std::string>& diagnostics) {
    const QJsonValue value = object.value(key);
    if (value.isUndefined()) {
        return;
    }
    if (!value.isArray()) {
        diagnostics.push_back(
            "Project config key '" + key.toStdString() + "' must be an array.");
        return;
    }

    for (const QJsonValue& itemValue : value.toArray()) {
        if (!itemValue.isObject()) {
            diagnostics.push_back(
                "Project config key '" + key.toStdString() + "' ignored a non-object item.");
            continue;
        }
        if (auto rule = parser(itemValue.toObject(), diagnostics); rule.has_value()) {
            destination.push_back(std::move(*rule));
        }
    }
}

std::optional<ProjectModuleMergeRule> parseModuleMergeRule(
    const QJsonObject& object,
    std::vector<std::string>& diagnostics) {
    const std::string matchPrefix = normalizeRuleToken(object.value(QStringLiteral("match")).toString());
    const std::string targetModule = normalizeRuleToken(object.value(QStringLiteral("module")).toString());
    if (matchPrefix.empty() || targetModule.empty()) {
        diagnostics.push_back("Project config ignored a module merge rule without both 'match' and 'module'.");
        return std::nullopt;
    }
    return ProjectModuleMergeRule{matchPrefix, targetModule};
}

std::optional<ProjectRoleOverrideRule> parseRoleOverrideRule(
    const QJsonObject& object,
    std::vector<std::string>& diagnostics) {
    const std::string matchPrefix = normalizeRuleToken(object.value(QStringLiteral("match")).toString());
    const std::string role = normalizeRuleToken(object.value(QStringLiteral("role")).toString());
    if (matchPrefix.empty() || role.empty()) {
        diagnostics.push_back("Project config ignored a role override rule without both 'match' and 'role'.");
        return std::nullopt;
    }
    return ProjectRoleOverrideRule{matchPrefix, role};
}

std::optional<ProjectEntryOverrideRule> parseEntryOverrideRule(
    const QJsonObject& object,
    std::vector<std::string>& diagnostics) {
    const std::string matchPrefix = normalizeRuleToken(object.value(QStringLiteral("match")).toString());
    const QJsonValue entryValue = object.value(QStringLiteral("entry"));
    if (matchPrefix.empty() || !entryValue.isBool()) {
        diagnostics.push_back("Project config ignored an entry override rule without both 'match' and boolean 'entry'.");
        return std::nullopt;
    }
    return ProjectEntryOverrideRule{matchPrefix, entryValue.toBool()};
}

std::optional<ProjectDependencyFoldRule> parseDependencyFoldRule(
    const QJsonObject& object,
    std::vector<std::string>& diagnostics) {
    const std::string matchPrefix = normalizeRuleToken(object.value(QStringLiteral("match")).toString());
    if (matchPrefix.empty()) {
        diagnostics.push_back("Project config ignored a dependency fold rule without 'match'.");
        return std::nullopt;
    }

    ProjectDependencyFoldRule rule;
    rule.matchPrefix = matchPrefix;
    if (const QJsonValue collapseValue = object.value(QStringLiteral("collapse")); collapseValue.isBool()) {
        rule.collapse = collapseValue.toBool();
    }
    if (const QJsonValue hideValue = object.value(QStringLiteral("hideByDefault")); hideValue.isBool()) {
        rule.hideByDefault = hideValue.toBool();
    }
    return rule;
}

}  // namespace

std::string normalizeProjectConfigToken(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    while (value.starts_with("./")) {
        value.erase(0, 2);
    }
    while (!value.empty() && value.back() == '/') {
        value.pop_back();
    }
    return value;
}

bool matchesProjectConfigPrefix(std::string_view candidate, std::string_view prefix) {
    if (prefix.empty()) {
        return false;
    }

    const std::string normalizedCandidate = normalizeProjectConfigToken(std::string(candidate));
    const std::string normalizedPrefix = normalizeProjectConfigToken(std::string(prefix));
    if (normalizedCandidate.empty() || normalizedPrefix.empty()) {
        return false;
    }
    if (normalizedCandidate == normalizedPrefix) {
        return true;
    }
    return normalizedCandidate.size() > normalizedPrefix.size() &&
           normalizedCandidate.starts_with(normalizedPrefix) &&
           normalizedCandidate[normalizedPrefix.size()] == '/';
}

ProjectAnalysisConfig loadProjectAnalysisConfig(const std::filesystem::path& rootPath) {
    ProjectAnalysisConfig config;

    for (const std::filesystem::path& candidatePath : candidateConfigPaths(rootPath.lexically_normal())) {
        const QFileInfo fileInfo(toQString(candidatePath));
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            continue;
        }

        config.configPath = candidatePath.generic_string();

        QFile input(fileInfo.absoluteFilePath());
        if (!input.open(QIODevice::ReadOnly)) {
            config.diagnostics.push_back(
                "Project analysis config exists but could not be opened: " + config.configPath + ".");
            return config;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(input.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            config.diagnostics.push_back(
                "Project analysis config parse failed for " + config.configPath + ": " +
                parseError.errorString().toStdString() + ".");
            return config;
        }

        const QJsonObject rootObject = document.object();
        const QJsonObject body = configBody(rootObject);
        const QJsonValue versionValue = rootObject.value(QStringLiteral("version"));
        if (!versionValue.isUndefined() && (!versionValue.isDouble() || versionValue.toInt() != 1)) {
            config.diagnostics.push_back(
                "Project analysis config ignored unsupported version in " + config.configPath + ".");
            return config;
        }

        appendStringList(body, QStringLiteral("ignoreDirectories"), config.ignoreDirectories, config.diagnostics);
        appendMatchRules<ProjectModuleMergeRule>(
            body,
            QStringLiteral("moduleMerges"),
            parseModuleMergeRule,
            config.moduleMerges,
            config.diagnostics);
        appendMatchRules<ProjectRoleOverrideRule>(
            body,
            QStringLiteral("roleOverrides"),
            parseRoleOverrideRule,
            config.roleOverrides,
            config.diagnostics);
        appendMatchRules<ProjectEntryOverrideRule>(
            body,
            QStringLiteral("entryOverrides"),
            parseEntryOverrideRule,
            config.entryOverrides,
            config.diagnostics);
        appendMatchRules<ProjectDependencyFoldRule>(
            body,
            QStringLiteral("dependencyFoldRules"),
            parseDependencyFoldRule,
            config.dependencyFoldRules,
            config.diagnostics);

        config.loaded = true;
        config.diagnostics.insert(
            config.diagnostics.begin(),
            "Project analysis config loaded from " + config.configPath + " (" + joinCountSummary(config) + ").");
        return config;
    }

    return config;
}

}  // namespace savt::core
