#pragma once

#include <QByteArray>
#include <QNetworkRequest>
#include <QString>
#include <QStringList>

namespace savt::ai {

struct DeepSeekConfig {
    QString provider = QStringLiteral("deepseek");
    QString apiKey;
    QString baseUrl = QStringLiteral("https://api.deepseek.com");
    QString endpointPath;
    QString model = QStringLiteral("deepseek-chat");
    int timeoutMs = 30000;
    bool enabled = false;

    bool isUsable() const;
    QString resolvedChatCompletionsUrl() const;
    QString providerLabel() const;
};

struct DeepSeekConfigLoadResult {
    DeepSeekConfig config;
    QString configPath;
    QString errorMessage;
    bool loadedFromFile = false;
    bool loadedFromBuiltInDefaults = false;

    bool hasConfig() const { return loadedFromFile || loadedFromBuiltInDefaults; }
};

struct ArchitectureAssistantRequest {
    QString projectName;
    QString projectRootPath;
    QString analyzerPrecision;
    QString analysisSummary;
    QString uiScope;
    QString learningStage;
    QString audience;
    QString explanationGoal;
    QString nodeName;
    QString nodeKind;
    QString nodeRole;
    QString nodeSummary;
    QString userTask;
    QStringList moduleNames;
    QStringList exampleFiles;
    QStringList topSymbols;
    QStringList collaboratorNames;
    QString filePath;
    QString fileLanguage;
    QString fileCategory;
    QString fileRoleHint;
    QString fileSummary;
    QString codeExcerpt;
    QStringList fileImports;
    QStringList fileDeclarations;
    QStringList fileSignals;
    QStringList fileReadingHints;
    QStringList contextClues;
    QStringList riskSignals;
    QStringList readingOrder;
    QStringList reportHighlights;
    QStringList diagnostics;
};

struct ArchitectureAssistantInsight {
    QString summary;
    QString plainSummary;
    QString responsibility;
    QString whyItMatters;
    QString uncertainty;
    QStringList collaborators;
    QStringList evidence;
    QStringList whereToStart;
    QStringList glossary;
    QStringList nextActions;

    bool isEmpty() const;
};

QString defaultDeepSeekConfigPath();
QString deepSeekConfigTemplatePath();
bool parseDeepSeekConfigJson(const QByteArray& jsonBytes, DeepSeekConfig* outConfig, QString* errorMessage = nullptr);
DeepSeekConfigLoadResult loadDeepSeekConfig(const QString& preferredPath = QString());

QString deepSeekSavtSystemPrompt();
QString deepSeekSavtUserPrompt(const ArchitectureAssistantRequest& request);
QNetworkRequest buildDeepSeekChatCompletionsRequest(const DeepSeekConfig& config);
QByteArray buildDeepSeekChatCompletionsPayload(
    const DeepSeekConfig& config,
    const ArchitectureAssistantRequest& request);
bool extractDeepSeekChatCompletionsText(
    const QByteArray& responseBytes,
    QString* outText,
    QString* errorMessage = nullptr);
bool parseArchitectureAssistantInsightJson(
    const QByteArray& jsonBytes,
    ArchitectureAssistantInsight* outInsight,
    QString* errorMessage = nullptr);
bool parseDeepSeekChatCompletionsResponse(
    const QByteArray& responseBytes,
    ArchitectureAssistantInsight* outInsight,
    QString* errorMessage = nullptr,
    QString* rawContent = nullptr);

}  // namespace savt::ai
