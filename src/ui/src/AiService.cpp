#include "savt/ui/AiService.h"

#include "savt/core/CapabilityGraph.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <algorithm>

namespace savt::ui {

namespace {

QStringList toQStringList(const QVariant& value) {
    QStringList items;
    for (const QVariant& entry : value.toList()) {
        const QString cleaned = entry.toString().trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned))
            items.push_back(cleaned);
    }
    return items;
}

QStringList toQStringList(const std::vector<std::string>& values) {
    QStringList items;
    for (const std::string& value : values) {
        const QString cleaned = QString::fromStdString(value).trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned))
            items.push_back(cleaned);
    }
    return items;
}

QVariantList toVariantStringList(const QStringList& values) {
    QVariantList items;
    items.reserve(values.size());
    for (const QString& value : values) {
        const QString cleaned = value.trimmed();
        if (!cleaned.isEmpty())
            items.push_back(cleaned);
    }
    return items;
}

QString projectNameFromRootPath(const QString& rootPath) {
    const QFileInfo fileInfo(rootPath);
    return fileInfo.fileName().isEmpty() ? QStringLiteral("当前项目")
                                         : fileInfo.fileName();
}

QString inferAnalyzerPrecision(const QString& reportText) {
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

QStringList deduplicateQStringList(const QStringList& values) {
    QStringList items;
    for (const QString& value : values) {
        const QString cleaned = value.trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned))
            items.push_back(cleaned);
    }
    return items;
}

QString buildAiSetupMessage(const ai::DeepSeekConfigLoadResult& loadResult) {
    if (!loadResult.loadedFromFile)
        return QStringLiteral("AI 未就绪：%1").arg(loadResult.errorMessage);
    if (!loadResult.config.enabled) {
        return QStringLiteral("已找到 AI 配置，但当前未启用。把 %1 里的 enabled 改成 true 后即可使用。")
            .arg(QDir::toNativeSeparators(loadResult.configPath));
    }
    if (!loadResult.config.isUsable()) {
        return QStringLiteral(
            "AI 配置已加载，但还不能使用。请检查模型、地址、endpointPath 和 API Key 是否完整。");
    }
    return QStringLiteral("%1 已就绪，当前模型为 %2，请求地址为 %3。AI 只会基于当前项目或节点的证据包进行解读。")
        .arg(loadResult.config.providerLabel(),
             loadResult.config.model,
             loadResult.config.resolvedChatCompletionsUrl());
}

QString extractApiErrorMessage(const QByteArray& bytes) {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return {};
    const QJsonObject object = document.object();
    if (!object.contains(QStringLiteral("error")) ||
        !object.value(QStringLiteral("error")).isObject()) {
        return {};
    }
    return object.value(QStringLiteral("error"))
        .toObject()
        .value(QStringLiteral("message"))
        .toString()
        .trimmed();
}

ai::ArchitectureAssistantRequest buildNodeRequest(
    const AiRequestContext& context,
    const QVariantMap& nodeData) {
    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.nodeName = nodeData.value(QStringLiteral("name")).toString().trimmed();
    request.nodeKind = nodeData.value(QStringLiteral("kind")).toString().trimmed();
    request.nodeRole = nodeData.value(QStringLiteral("role")).toString().trimmed();
    request.nodeSummary = nodeData.value(QStringLiteral("summary")).toString().trimmed();
    if (request.nodeSummary.isEmpty()) {
        request.nodeSummary =
            nodeData.value(QStringLiteral("responsibility")).toString().trimmed();
    }
    request.userTask =
        context.userTask.trimmed().isEmpty()
            ? QStringLiteral(
                  "You are a senior multilingual software architect across C++, Java, Python, JavaScript, Go, and related ecosystems.\n"
                  "Analyze the selected module from the supplied static evidence only. Do not overfit to one keyword; for example, Node could mean Node.js, a graph node, a UI tree node, or a data structure.\n"
                  "Use module names, file extensions, import/include hints, core symbols, collaborators, and project context together before deciding the architecture role.\n"
                  "When evidence is weak or ambiguous, explain that explicitly in uncertainty and keep the conclusion conservative.\n"
                  "Focus on what the module does, who it collaborates with, what evidence supports the judgment, and what remains uncertain.")
            : context.userTask.trimmed();
    request.moduleNames = toQStringList(nodeData.value(QStringLiteral("moduleNames")));
    request.exampleFiles = toQStringList(nodeData.value(QStringLiteral("exampleFiles")));
    request.topSymbols = toQStringList(nodeData.value(QStringLiteral("topSymbols")));
    request.collaboratorNames =
        toQStringList(nodeData.value(QStringLiteral("collaboratorNames")));
    request.diagnostics = {QStringLiteral("analysis phase: %1")
                               .arg(context.analysisPhase.trimmed().isEmpty()
                                        ? QStringLiteral("unknown")
                                        : context.analysisPhase.trimmed())};
    if (!context.analysisReport.trimmed().isEmpty() &&
        context.analysisReport.contains(QStringLiteral("语法"),
                                        Qt::CaseInsensitive)) {
        request.diagnostics.push_back(
            QStringLiteral("analysis precision may be syntax-fallback"));
    }
    request.diagnostics.push_back(QStringLiteral(
        "language inference hint: inspect file suffixes, import/include evidence, and symbol naming before assigning a role"));
    return request;
}

ai::ArchitectureAssistantRequest buildProjectRequest(
    const AiRequestContext& context,
    const QVariantMap& systemContextData,
    const QVariantList& systemContextCards,
    const core::CapabilityGraph* capabilityGraph) {
    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.nodeName = systemContextData.value(QStringLiteral("title")).toString().trimmed();
    if (request.nodeName.isEmpty()) {
        request.nodeName =
            QStringLiteral("%1 / 系统上下文").arg(request.projectName);
    }
    request.nodeKind = QStringLiteral("system_context");
    request.nodeRole = QStringLiteral("project_context");

    QStringList summaryParts;
    summaryParts << systemContextData.value(QStringLiteral("headline")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("purposeSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("entrySummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("inputSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("outputSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("containerSummary")).toString().trimmed();
    for (const QVariant& value : systemContextCards) {
        const QVariantMap item = value.toMap();
        const QString name = item.value(QStringLiteral("name")).toString().trimmed();
        const QString summary = item.value(QStringLiteral("summary")).toString().trimmed();
        if (!name.isEmpty() && !summary.isEmpty())
            summaryParts << QStringLiteral("%1：%2").arg(name, summary);
    }
    request.nodeSummary = deduplicateQStringList(summaryParts).join(QStringLiteral(" "));
    request.userTask =
        context.userTask.trimmed().isEmpty()
            ? QStringLiteral("请像 C4 模型的 L1 系统上下文一样，总结这个项目：谁会使用它、它大致在解决什么问题、会接收哪些输入、主要部分是什么。语言要简体中文，适合非程序员一眼看懂。")
            : context.userTask.trimmed();
    request.moduleNames =
        toQStringList(systemContextData.value(QStringLiteral("containerNames")));
    request.collaboratorNames =
        toQStringList(systemContextData.value(QStringLiteral("containerNames")));
    request.diagnostics = {
        QStringLiteral("analysis phase: %1")
            .arg(context.analysisPhase.trimmed().isEmpty()
                     ? QStringLiteral("unknown")
                     : context.analysisPhase.trimmed()),
        QStringLiteral("ai scope: system_context"),
        QStringLiteral("technology summary: %1")
            .arg(systemContextData.value(QStringLiteral("technologySummary"))
                     .toString()
                     .trimmed())};
    if (capabilityGraph == nullptr)
        return request;

    for (const core::CapabilityNode& node : capabilityGraph->nodes) {
        for (const QString& path : toQStringList(node.exampleFiles)) {
            if (!request.exampleFiles.contains(path) &&
                request.exampleFiles.size() < 8) {
                request.exampleFiles.push_back(path);
            }
        }
        for (const QString& symbol : toQStringList(node.topSymbols)) {
            if (!request.topSymbols.contains(symbol) &&
                request.topSymbols.size() < 8) {
                request.topSymbols.push_back(symbol);
            }
        }
    }
    return request;
}

QString formattedPayloadForDebug(const QByteArray& payload) {
    QString formattedPayload = QString::fromUtf8(payload);
    const QJsonDocument payloadDocument = QJsonDocument::fromJson(payload);
    if (!payloadDocument.isNull()) {
        formattedPayload =
            QString::fromUtf8(payloadDocument.toJson(QJsonDocument::Indented));
    }
    return formattedPayload;
}

QString formattedResponseForDebug(const QByteArray& responseBytes) {
    QString rawResponseText = QString::fromUtf8(responseBytes);
    const QJsonDocument responseDocument = QJsonDocument::fromJson(responseBytes);
    if (!responseDocument.isNull()) {
        rawResponseText =
            QString::fromUtf8(responseDocument.toJson(QJsonDocument::Indented));
    }
    return rawResponseText;
}

}  // namespace

AiAvailabilityState AiService::inspectAvailability() {
    AiAvailabilityState state;
    state.configPath = ai::defaultDeepSeekConfigPath();
    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    state.available = loadResult.loadedFromFile && loadResult.config.isUsable();
    state.setupMessage = buildAiSetupMessage(loadResult);
    return state;
}

AiPreparedRequest AiService::prepareNodeRequest(
    const AiRequestContext& context,
    const QVariantMap& nodeData) {
    AiPreparedRequest prepared;
    prepared.scope = QStringLiteral("node");
    prepared.availability = inspectAvailability();

    prepared.targetName = nodeData.value(QStringLiteral("name")).toString().trimmed();
    if (prepared.targetName.isEmpty()) {
        prepared.failureStatusMessage =
            QStringLiteral("先在图里选中一个模块，再生成 AI 解读。");
        return prepared;
    }
    if (!prepared.availability.available) {
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    if (!loadResult.loadedFromFile || !loadResult.config.isUsable()) {
        prepared.availability.available = false;
        prepared.availability.setupMessage = buildAiSetupMessage(loadResult);
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    prepared.ready = true;
    prepared.pendingStatusMessage =
        QStringLiteral("正在向 AI 模型请求当前节点的解读...");
    prepared.assistantRequest = buildNodeRequest(context, nodeData);
    prepared.networkRequest =
        ai::buildDeepSeekChatCompletionsRequest(loadResult.config);
    prepared.payload = ai::buildDeepSeekChatCompletionsPayload(
        loadResult.config, prepared.assistantRequest);
    return prepared;
}

AiPreparedRequest AiService::prepareProjectRequest(
    const AiRequestContext& context,
    const QVariantMap& systemContextData,
    const QVariantList& systemContextCards,
    const core::CapabilityGraph* capabilityGraph) {
    AiPreparedRequest prepared;
    prepared.scope = QStringLiteral("system_context");
    prepared.availability = inspectAvailability();

    if (systemContextData.isEmpty() || capabilityGraph == nullptr ||
        capabilityGraph->nodes.empty()) {
        prepared.failureStatusMessage =
            QStringLiteral("先完成一次项目分析，再生成项目级系统上下文解读。");
        return prepared;
    }

    prepared.targetName =
        systemContextData.value(QStringLiteral("title")).toString().trimmed();
    if (prepared.targetName.isEmpty()) {
        prepared.targetName = QStringLiteral("%1 / 系统上下文")
                                  .arg(projectNameFromRootPath(context.projectRootPath));
    }
    if (!prepared.availability.available) {
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    if (!loadResult.loadedFromFile || !loadResult.config.isUsable()) {
        prepared.availability.available = false;
        prepared.availability.setupMessage = buildAiSetupMessage(loadResult);
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    prepared.ready = true;
    prepared.pendingStatusMessage =
        QStringLiteral("正在向 AI 模型请求项目级系统上下文解读...");
    prepared.assistantRequest = buildProjectRequest(
        context, systemContextData, systemContextCards, capabilityGraph);
    prepared.networkRequest =
        ai::buildDeepSeekChatCompletionsRequest(loadResult.config);
    prepared.payload = ai::buildDeepSeekChatCompletionsPayload(
        loadResult.config, prepared.assistantRequest);
    return prepared;
}

AiReplyState AiService::parseReply(
    const QByteArray& responseBytes,
    const QString& scope,
    const bool hasNetworkError,
    const QString& networkErrorString) {
    AiReplyState state;
    if (hasNetworkError) {
        state.statusMessage =
            QStringLiteral("AI 请求失败：%1").arg(networkErrorString);
        const QString apiMessage = extractApiErrorMessage(responseBytes);
        if (!apiMessage.isEmpty())
            state.statusMessage += QStringLiteral("\n%1").arg(apiMessage);
        return state;
    }

    ai::ArchitectureAssistantInsight insight;
    QString errorMessage;
    QString rawContent;
    if (!ai::parseDeepSeekChatCompletionsResponse(
            responseBytes, &insight, &errorMessage, &rawContent)) {
        state.statusMessage =
            QStringLiteral("AI 已返回结果，但当前内容无法被工具解析：%1")
                .arg(errorMessage);
        return state;
    }

    state.hasResult = true;
    state.summary = insight.summary;
    state.responsibility = insight.responsibility;
    state.uncertainty = insight.uncertainty;
    state.collaborators = toVariantStringList(insight.collaborators);
    state.evidence = toVariantStringList(insight.evidence);
    state.nextActions = toVariantStringList(insight.nextActions);
    state.statusMessage =
        scope == QStringLiteral("system_context")
            ? QStringLiteral("AI 已基于当前项目证据生成系统上下文解读。")
            : QStringLiteral("AI 已基于当前节点证据生成解读。");
    return state;
}

void AiService::logRequest(const AiPreparedRequest& request,
                           const QString& targetDebug) {
    qDebug().noquote() << "\n========== [AI Request] ==========";
    qDebug().noquote() << "scope:" << request.scope;
    qDebug().noquote() << "target:" << targetDebug;
    qDebug().noquote() << "[user prompt]\n"
                       << ai::deepSeekSavtUserPrompt(request.assistantRequest);
    qDebug().noquote() << "[json payload]\n"
                       << formattedPayloadForDebug(request.payload);
    qDebug().noquote() << "==================================\n";
}

void AiService::logResponse(const QByteArray& responseBytes,
                            const QString& scope) {
    qDebug().noquote() << "\n========== [AI Response] ==========";
    qDebug().noquote() << "scope:"
                       << (scope.isEmpty() ? QStringLiteral("unknown") : scope);
    qDebug().noquote() << "[raw response]\n"
                       << formattedResponseForDebug(responseBytes);
    qDebug().noquote() << "===================================\n";
}

}  // namespace savt::ui
