#include "savt/ui/AiService.h"

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

QStringList mergeQStringLists(const QStringList& first,
                              const QStringList& second) {
    QStringList items = deduplicateQStringList(first);
    for (const QString& value : second) {
        const QString cleaned = value.trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned))
            items.push_back(cleaned);
    }
    return items;
}

QStringList extractMarkdownHighlights(const QString& text,
                                      const int maxItems,
                                      const int maxLineLength = 160) {
    QStringList items;
    const QStringList rawLines = text.split(QLatin1Char('\n'));
    for (QString line : rawLines) {
        line = line.trimmed();
        while (line.startsWith(QLatin1Char('#'))) {
            line.remove(0, 1);
            line = line.trimmed();
        }
        if (line.isEmpty() || line == QStringLiteral("---"))
            continue;
        if (line.size() > maxLineLength)
            line = line.left(maxLineLength - 3) + QStringLiteral("...");
        if (!items.contains(line))
            items.push_back(line);
        if (items.size() >= maxItems)
            break;
    }
    return items;
}

QStringList collectNodeFieldValues(const QVariantList& nodeItems,
                                   const QString& fieldName,
                                   const int maxItems) {
    QStringList items;
    for (const QVariant& value : nodeItems) {
        const QVariantMap node = value.toMap();
        const QVariant fieldValue = node.value(fieldName);
        QStringList candidateValues;
        if (fieldValue.metaType().id() == QMetaType::QString) {
            const QString cleaned = fieldValue.toString().trimmed();
            if (!cleaned.isEmpty())
                candidateValues.push_back(cleaned);
        } else {
            candidateValues = toQStringList(fieldValue);
        }

        for (const QString& candidate : candidateValues) {
            if (!candidate.isEmpty() && !items.contains(candidate))
                items.push_back(candidate);
            if (items.size() >= maxItems)
                return items;
        }
    }
    return items;
}

QString buildAiSetupMessage(const ai::DeepSeekConfigLoadResult& loadResult) {
    if (!loadResult.hasConfig())
        return QStringLiteral("AI 未就绪：%1").arg(loadResult.errorMessage);
    if (loadResult.loadedFromBuiltInDefaults && !loadResult.config.enabled) {
        return QStringLiteral("已加载编译期 AI 配置，但当前未启用。请重新配置 CMake 后再构建。");
    }
    if (!loadResult.config.enabled) {
        return QStringLiteral("已找到 AI 配置，但当前未启用。把 %1 里的 enabled 改成 true 后即可使用。")
            .arg(QDir::toNativeSeparators(loadResult.configPath));
    }
    if (!loadResult.config.isUsable()) {
        return QStringLiteral(
            "AI 配置已加载，但还不能使用。请检查模型、地址、endpointPath 和 API Key 是否完整。");
    }
    if (loadResult.loadedFromBuiltInDefaults) {
        return QStringLiteral("%1 已就绪，当前模型为 %2，请求地址为 %3。当前使用的是编译期 AI 配置。")
            .arg(loadResult.config.providerLabel(),
                 loadResult.config.model,
                 loadResult.config.resolvedChatCompletionsUrl());
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

QString publicScopeLabel(const QString& scope) {
    if (scope == QStringLiteral("system_context")) {
        return QStringLiteral("项目导览");
    }
    if (scope == QStringLiteral("engineering_report")) {
        return QStringLiteral("深入阅读建议");
    }
    if (scope == QStringLiteral("capability_map") ||
        scope == QStringLiteral("component_node")) {
        return QStringLiteral("模块解读");
    }
    return QStringLiteral("模块解读");
}

ai::ArchitectureAssistantRequest buildCapabilityRequest(
    const AiRequestContext& context,
    const QVariantMap& nodeData) {
    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.uiScope = QStringLiteral("l2_capability_map");
    request.learningStage = QStringLiteral("L2");
    request.audience = QStringLiteral("beginner");
    request.explanationGoal = QStringLiteral(
        "Help a beginner understand what this capability owns, how it connects to nearby capabilities, and when to drill into L3.");
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
                  "Explain the selected capability to a beginner who is new to this repository. "
                  "Use plain language to describe what responsibility this capability owns, "
                  "which other capabilities it depends on or supports, what risks or signals are worth noticing, "
                  "and when the reader should continue into L3. Use only the supplied evidence package.")
            : context.userTask.trimmed();
    request.moduleNames = toQStringList(nodeData.value(QStringLiteral("moduleNames")));
    request.exampleFiles = toQStringList(nodeData.value(QStringLiteral("exampleFiles")));
    request.topSymbols = toQStringList(nodeData.value(QStringLiteral("topSymbols")));
    request.collaboratorNames =
        toQStringList(nodeData.value(QStringLiteral("collaboratorNames")));

    const QVariantMap evidence = nodeData.value(QStringLiteral("evidence")).toMap();
    request.diagnostics = {
        QStringLiteral("analysis phase: %1")
            .arg(context.analysisPhase.trimmed().isEmpty()
                     ? QStringLiteral("unknown")
                     : context.analysisPhase.trimmed()),
        QStringLiteral("ai scope: capability_map"),
        QStringLiteral("audience: beginner"),
        QStringLiteral("learning stage: L2"),
        QStringLiteral("risk level: %1")
            .arg(nodeData.value(QStringLiteral("riskLevel")).toString().trimmed()),
        QStringLiteral("incoming edges: %1")
            .arg(nodeData.value(QStringLiteral("incomingEdgeCount")).toULongLong()),
        QStringLiteral("outgoing edges: %1")
            .arg(nodeData.value(QStringLiteral("outgoingEdgeCount")).toULongLong())};
    const QStringList supportingRoles =
        toQStringList(nodeData.value(QStringLiteral("supportingRoles")));
    if (!supportingRoles.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("supporting roles: %1").arg(supportingRoles.join(QStringLiteral(", "))));
    }
    const QStringList technologyTags =
        toQStringList(nodeData.value(QStringLiteral("technologyTags")));
    if (!technologyTags.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("technology tags: %1").arg(technologyTags.join(QStringLiteral(", "))));
    }
    const QString confidenceLabel =
        evidence.value(QStringLiteral("confidenceLabel")).toString().trimmed();
    if (!confidenceLabel.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("evidence confidence: %1").arg(confidenceLabel));
    }
    const QString confidenceReason =
        evidence.value(QStringLiteral("confidenceReason")).toString().trimmed();
    if (!confidenceReason.isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("confidence reason: %1").arg(confidenceReason));
    }
    return request;
}

ai::ArchitectureAssistantRequest buildComponentRequest(
    const AiRequestContext& context,
    const QVariantMap& nodeData) {
    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.uiScope = QStringLiteral("l3_component_guide");
    request.learningStage = QStringLiteral("L3");
    request.audience = QStringLiteral("beginner");
    request.explanationGoal = QStringLiteral(
        "Help a beginner understand what this component does, why it matters, and which files to read first.");
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
                  "Explain the selected component to a beginner who is new to this repository. "
                  "Start with a plain-language explanation of what it does, then explain why it matters, "
                  "which files are the best starting points, and which nearby components it works with. "
                  "If you must use architecture jargon, explain it briefly. Use only the supplied evidence package.")
            : context.userTask.trimmed();
    request.moduleNames = toQStringList(nodeData.value(QStringLiteral("moduleNames")));
    request.exampleFiles = toQStringList(nodeData.value(QStringLiteral("exampleFiles")));
    request.topSymbols = toQStringList(nodeData.value(QStringLiteral("topSymbols")));
    request.collaboratorNames =
        toQStringList(nodeData.value(QStringLiteral("collaboratorNames")));
    request.diagnostics = {QStringLiteral("analysis phase: %1")
                               .arg(context.analysisPhase.trimmed().isEmpty()
                                        ? QStringLiteral("unknown")
                                        : context.analysisPhase.trimmed()),
                           QStringLiteral("ai scope: component_node"),
                           QStringLiteral("audience: beginner"),
                           QStringLiteral("learning stage: L3")};
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
    const QVariantList& capabilityNodeItems) {
    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.uiScope = QStringLiteral("l1_project_guide");
    request.learningStage = QStringLiteral("L1");
    request.audience = QStringLiteral("beginner");
    request.explanationGoal = QStringLiteral(
        "Help a beginner understand what the project is, who it serves, its main parts, and where to start reading.");
    request.nodeName = systemContextData.value(QStringLiteral("title")).toString().trimmed();
    if (request.nodeName.isEmpty()) {
        request.nodeName =
            QStringLiteral("%1 / 项目导览").arg(request.projectName);
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
            ? QStringLiteral(
                  "Explain the whole project to a beginner who is new to code repositories. "
                  "Use plain language to describe what the project is for, who uses it, its main parts, "
                  "and where to start reading first. If architecture jargon appears, explain it briefly. "
                  "Use only the supplied evidence package.")
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
        QStringLiteral("audience: beginner"),
        QStringLiteral("learning stage: L1"),
        QStringLiteral("technology summary: %1")
            .arg(systemContextData.value(QStringLiteral("technologySummary"))
                     .toString()
                     .trimmed())};
    for (const QVariant& value : capabilityNodeItems) {
        const QVariantMap node = value.toMap();
        for (const QString& path :
             toQStringList(node.value(QStringLiteral("exampleFiles")))) {
            if (!request.exampleFiles.contains(path) &&
                request.exampleFiles.size() < 8) {
                request.exampleFiles.push_back(path);
            }
        }
        for (const QString& symbol :
             toQStringList(node.value(QStringLiteral("topSymbols")))) {
            if (!request.topSymbols.contains(symbol) &&
                request.topSymbols.size() < 8) {
                request.topSymbols.push_back(symbol);
            }
        }
    }
    return request;
}

ai::ArchitectureAssistantRequest buildReportRequest(
    const AiRequestContext& context,
    const QVariantMap& systemContextData,
    const QVariantList& systemContextCards,
    const QVariantList& capabilityNodeItems) {
    ai::ArchitectureAssistantRequest request;
    request.projectName = projectNameFromRootPath(context.projectRootPath);
    request.projectRootPath = QDir::toNativeSeparators(context.projectRootPath);
    request.analyzerPrecision = inferAnalyzerPrecision(context.analysisReport);
    request.analysisSummary = context.statusMessage.trimmed();
    request.uiScope = QStringLiteral("l4_engineering_report");
    request.learningStage = QStringLiteral("L4");
    request.audience = QStringLiteral("beginner");
    request.explanationGoal = QStringLiteral(
        "Help a beginner connect the current findings to concrete next steps, important modules, and the best reading order.");
    request.nodeName = QStringLiteral("%1 / 深入阅读建议").arg(request.projectName);
    request.nodeKind = QStringLiteral("engineering_report");
    request.nodeRole = QStringLiteral("report_context");

    QStringList summaryParts;
    summaryParts << systemContextData.value(QStringLiteral("headline")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("purposeSummary")).toString().trimmed()
                 << systemContextData.value(QStringLiteral("containerSummary")).toString().trimmed();
    for (const QVariant& value : systemContextCards) {
        const QVariantMap item = value.toMap();
        const QString name = item.value(QStringLiteral("name")).toString().trimmed();
        const QString summary = item.value(QStringLiteral("summary")).toString().trimmed();
        if (!name.isEmpty() && !summary.isEmpty())
            summaryParts << QStringLiteral("%1：%2").arg(name, summary);
    }
    const QStringList reportHighlights =
        extractMarkdownHighlights(context.analysisReport, 6);
    for (const QString& highlight : reportHighlights)
        summaryParts.push_back(highlight);
    if (!context.astPreviewSummary.trimmed().isEmpty()) {
        summaryParts.push_back(
            QStringLiteral("当前 AST 聚焦：%1").arg(context.astPreviewSummary.trimmed()));
    }
    request.nodeSummary = deduplicateQStringList(summaryParts).join(QStringLiteral(" "));
    request.userTask =
        context.userTask.trimmed().isEmpty()
            ? QStringLiteral(
                  "Explain the current engineering report to a beginner. "
                  "Summarize the most important conclusions in plain language, explain what they mean for actual code changes, "
                  "point out the best files or modules to inspect next, and keep the answer grounded in the supplied evidence package.")
            : context.userTask.trimmed();

    request.moduleNames = mergeQStringLists(
        toQStringList(systemContextData.value(QStringLiteral("containerNames"))),
        collectNodeFieldValues(capabilityNodeItems, QStringLiteral("moduleNames"), 10));
    request.collaboratorNames = collectNodeFieldValues(
        capabilityNodeItems, QStringLiteral("name"), 10);
    request.topSymbols = collectNodeFieldValues(
        capabilityNodeItems, QStringLiteral("topSymbols"), 12);
    if (!context.selectedAstFilePath.trimmed().isEmpty()) {
        request.exampleFiles.push_back(context.selectedAstFilePath.trimmed());
    }
    request.exampleFiles = mergeQStringLists(
        request.exampleFiles,
        collectNodeFieldValues(capabilityNodeItems, QStringLiteral("exampleFiles"), 12));

    request.diagnostics = {
        QStringLiteral("analysis phase: %1")
            .arg(context.analysisPhase.trimmed().isEmpty()
                     ? QStringLiteral("unknown")
                     : context.analysisPhase.trimmed()),
        QStringLiteral("ai scope: engineering_report"),
        QStringLiteral("audience: beginner"),
        QStringLiteral("learning stage: L4"),
        QStringLiteral("technology summary: %1")
            .arg(systemContextData.value(QStringLiteral("technologySummary"))
                     .toString()
                     .trimmed())};
    if (!context.selectedAstFilePath.trimmed().isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("current ast file: %1").arg(context.selectedAstFilePath.trimmed()));
    }
    if (!context.astPreviewTitle.trimmed().isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("ast preview title: %1").arg(context.astPreviewTitle.trimmed()));
    }
    if (!context.astPreviewSummary.trimmed().isEmpty()) {
        request.diagnostics.push_back(
            QStringLiteral("ast preview summary: %1").arg(context.astPreviewSummary.trimmed()));
    }
    for (const QString& highlight : reportHighlights) {
        request.diagnostics.push_back(
            QStringLiteral("report highlight: %1").arg(highlight));
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
    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    state.configPath = loadResult.configPath.trimmed().isEmpty()
        ? ai::defaultDeepSeekConfigPath()
        : loadResult.configPath;
    state.available = loadResult.hasConfig() && loadResult.config.isUsable();
    state.setupMessage = buildAiSetupMessage(loadResult);
    return state;
}

QString AiService::classifyNodeScope(const QVariantMap& nodeData) {
    if (nodeData.isEmpty())
        return QStringLiteral("node");
    return nodeData.contains(QStringLiteral("capabilityId"))
               ? QStringLiteral("component_node")
               : QStringLiteral("capability_map");
}

AiPreparedRequest AiService::prepareNodeRequest(
    const AiRequestContext& context,
    const QVariantMap& nodeData) {
    AiPreparedRequest prepared;
    prepared.scope = classifyNodeScope(nodeData);
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
    if (!loadResult.hasConfig() || !loadResult.config.isUsable()) {
        prepared.availability.available = false;
        prepared.availability.setupMessage = buildAiSetupMessage(loadResult);
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    prepared.ready = true;
    prepared.pendingStatusMessage =
        prepared.scope == QStringLiteral("capability_map")
            ? QStringLiteral("正在生成当前模块的解读...")
            : QStringLiteral("正在生成当前模块的解读...");
    prepared.assistantRequest =
        prepared.scope == QStringLiteral("capability_map")
            ? buildCapabilityRequest(context, nodeData)
            : buildComponentRequest(context, nodeData);
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
    const QVariantList& capabilityNodeItems) {
    AiPreparedRequest prepared;
    prepared.scope = QStringLiteral("system_context");
    prepared.availability = inspectAvailability();

    if (systemContextData.isEmpty() || capabilityNodeItems.isEmpty()) {
        prepared.failureStatusMessage =
            QStringLiteral("先完成一次项目分析，再生成项目导览。");
        return prepared;
    }

    prepared.targetName =
        systemContextData.value(QStringLiteral("title")).toString().trimmed();
    if (prepared.targetName.isEmpty()) {
        prepared.targetName = QStringLiteral("%1 / 项目导览")
                                  .arg(projectNameFromRootPath(context.projectRootPath));
    }
    if (!prepared.availability.available) {
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    if (!loadResult.hasConfig() || !loadResult.config.isUsable()) {
        prepared.availability.available = false;
        prepared.availability.setupMessage = buildAiSetupMessage(loadResult);
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    prepared.ready = true;
    prepared.pendingStatusMessage =
        QStringLiteral("正在生成项目导览...");
    prepared.assistantRequest = buildProjectRequest(
        context, systemContextData, systemContextCards, capabilityNodeItems);
    prepared.networkRequest =
        ai::buildDeepSeekChatCompletionsRequest(loadResult.config);
    prepared.payload = ai::buildDeepSeekChatCompletionsPayload(
        loadResult.config, prepared.assistantRequest);
    return prepared;
}

AiPreparedRequest AiService::prepareReportRequest(
    const AiRequestContext& context,
    const QVariantMap& systemContextData,
    const QVariantList& systemContextCards,
    const QVariantList& capabilityNodeItems) {
    AiPreparedRequest prepared;
    prepared.scope = QStringLiteral("engineering_report");
    prepared.availability = inspectAvailability();

    if (context.analysisReport.trimmed().isEmpty()) {
        prepared.failureStatusMessage =
            QStringLiteral("先完成一次项目分析，再生成深入阅读建议。");
        return prepared;
    }

    prepared.targetName = QStringLiteral("%1 / 深入阅读建议")
                              .arg(projectNameFromRootPath(context.projectRootPath));
    if (!prepared.availability.available) {
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    const ai::DeepSeekConfigLoadResult loadResult = ai::loadDeepSeekConfig();
    if (!loadResult.hasConfig() || !loadResult.config.isUsable()) {
        prepared.availability.available = false;
        prepared.availability.setupMessage = buildAiSetupMessage(loadResult);
        prepared.failureStatusMessage = prepared.availability.setupMessage;
        return prepared;
    }

    prepared.ready = true;
    prepared.pendingStatusMessage =
        QStringLiteral("正在生成深入阅读建议...");
    prepared.assistantRequest = buildReportRequest(
        context, systemContextData, systemContextCards, capabilityNodeItems);
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

    const QString effectiveSummary =
        !insight.plainSummary.trimmed().isEmpty() ? insight.plainSummary
                                                  : insight.summary;
    QStringList responsibilitySections;
    if (!insight.whyItMatters.trimmed().isEmpty()) {
        responsibilitySections.push_back(
            QStringLiteral("为什么重要：%1").arg(insight.whyItMatters.trimmed()));
    }
    if (!insight.responsibility.trimmed().isEmpty()) {
        responsibilitySections.push_back(
            QStringLiteral("具体职责：%1")
                .arg(insight.responsibility.trimmed()));
    }

    state.hasResult = true;
    state.summary = effectiveSummary;
    state.responsibility = responsibilitySections.join(QStringLiteral("\n\n"));
    state.uncertainty = insight.uncertainty;
    state.collaborators = toVariantStringList(insight.collaborators);
    state.evidence = toVariantStringList(
        mergeQStringLists(insight.evidence, insight.glossary));
    state.nextActions = toVariantStringList(
        mergeQStringLists(insight.whereToStart, insight.nextActions));
    state.statusMessage = QStringLiteral("AI 已基于当前证据生成%1。")
                              .arg(publicScopeLabel(scope));
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
