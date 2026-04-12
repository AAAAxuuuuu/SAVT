#include "savt/ui/AnalysisAnnotationService.h"

#include "savt/ai/DeepSeekClient.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace savt::ui {
namespace {

struct NodeAnnotation {
    std::size_t id = 0;
    QString label;
    QString summary;
    QString responsibility;
    QString confidence;
    QString confidenceReason;
};

struct BatchAnnotationResult {
    QString graphSummary;
    std::vector<NodeAnnotation> annotations;
    bool cacheHit = false;
    qint64 elapsedMs = 0;
    qsizetype promptBytes = 0;
    qsizetype responseBytes = 0;
};

struct CachedBatchAnnotationResponse {
    QString graphSummary;
    std::vector<NodeAnnotation> annotations;
};

auto& annotationCacheMutex() {
    static std::mutex mutex;
    return mutex;
}

auto& annotationCache() {
    static std::unordered_map<std::string, CachedBatchAnnotationResponse> cache;
    return cache;
}

bool parseBatchAnnotations(
    const QString& rawText,
    std::vector<NodeAnnotation>* outAnnotations,
    QString* graphSummary,
    QString* errorMessage);

QString projectNameFromRootPath(const QString& rootPath) {
    const QFileInfo fileInfo(rootPath);
    return fileInfo.fileName().isEmpty() ? QStringLiteral("Current project")
                                         : fileInfo.fileName();
}

QString trimAndClamp(const QString& value, const int maxLength = 180) {
    QString cleaned = value.trimmed();
    if (cleaned.size() > maxLength) {
        cleaned = cleaned.left(maxLength - 3) + QStringLiteral("...");
    }
    return cleaned;
}

QStringList limitStringList(
    const std::vector<std::string>& values,
    const int maxItems,
    const int maxLength = 140) {
    QStringList items;
    for (const std::string& value : values) {
        const QString cleaned = trimAndClamp(QString::fromStdString(value), maxLength);
        if (!cleaned.isEmpty() && !items.contains(cleaned)) {
            items.push_back(cleaned);
        }
        if (items.size() >= maxItems) {
            break;
        }
    }
    return items;
}

QJsonArray toJsonArray(const QStringList& values) {
    QJsonArray array;
    for (const QString& value : values) {
        const QString cleaned = value.trimmed();
        if (!cleaned.isEmpty()) {
            array.push_back(cleaned);
        }
    }
    return array;
}

QJsonObject buildEvidenceObject(const savt::core::CapabilityNode::EvidencePackage& evidence) {
    QJsonObject object;
    object.insert(QStringLiteral("facts"), toJsonArray(limitStringList(evidence.facts, 4)));
    object.insert(QStringLiteral("rules"), toJsonArray(limitStringList(evidence.rules, 3)));
    object.insert(QStringLiteral("conclusions"), toJsonArray(limitStringList(evidence.conclusions, 3)));
    object.insert(QStringLiteral("sourceFiles"), toJsonArray(limitStringList(evidence.sourceFiles, 4)));
    object.insert(QStringLiteral("symbols"), toJsonArray(limitStringList(evidence.symbols, 4)));
    object.insert(QStringLiteral("modules"), toJsonArray(limitStringList(evidence.modules, 4)));
    object.insert(QStringLiteral("relationships"), toJsonArray(limitStringList(evidence.relationships, 4)));
    object.insert(QStringLiteral("confidenceLabel"), QString::fromStdString(evidence.confidenceLabel));
    object.insert(QStringLiteral("confidenceReason"), trimAndClamp(QString::fromStdString(evidence.confidenceReason), 200));
    return object;
}

QJsonObject buildCapabilityNodeObject(const savt::core::CapabilityNode& node) {
    QJsonObject object;
    object.insert(QStringLiteral("id"), static_cast<qint64>(node.id));
    object.insert(QStringLiteral("name"), trimAndClamp(QString::fromStdString(node.name)));
    object.insert(QStringLiteral("kind"), QString::fromStdString(savt::core::toString(node.kind)));
    object.insert(QStringLiteral("role"), trimAndClamp(QString::fromStdString(node.dominantRole), 120));
    object.insert(QStringLiteral("responsibility"), trimAndClamp(QString::fromStdString(node.responsibility), 220));
    object.insert(QStringLiteral("summary"), trimAndClamp(QString::fromStdString(node.summary), 260));
    object.insert(QStringLiteral("moduleNames"), toJsonArray(limitStringList(node.moduleNames, 4)));
    object.insert(QStringLiteral("exampleFiles"), toJsonArray(limitStringList(node.exampleFiles, 4)));
    object.insert(QStringLiteral("topSymbols"), toJsonArray(limitStringList(node.topSymbols, 4)));
    object.insert(QStringLiteral("collaborators"), toJsonArray(limitStringList(node.collaboratorNames, 4)));
    object.insert(QStringLiteral("technologyTags"), toJsonArray(limitStringList(node.technologyTags, 4)));
    object.insert(QStringLiteral("evidence"), buildEvidenceObject(node.evidence));
    return object;
}

QJsonObject buildComponentNodeObject(const savt::core::ComponentNode& node) {
    QJsonObject object;
    object.insert(QStringLiteral("id"), static_cast<qint64>(node.id));
    object.insert(QStringLiteral("name"), trimAndClamp(QString::fromStdString(node.name)));
    object.insert(QStringLiteral("kind"), QString::fromStdString(savt::core::toString(node.kind)));
    object.insert(QStringLiteral("role"), trimAndClamp(QString::fromStdString(node.role), 120));
    object.insert(QStringLiteral("responsibility"), trimAndClamp(QString::fromStdString(node.responsibility), 220));
    object.insert(QStringLiteral("summary"), trimAndClamp(QString::fromStdString(node.summary), 260));
    object.insert(QStringLiteral("moduleNames"), toJsonArray(limitStringList(node.moduleNames, 4)));
    object.insert(QStringLiteral("exampleFiles"), toJsonArray(limitStringList(node.exampleFiles, 4)));
    object.insert(QStringLiteral("topSymbols"), toJsonArray(limitStringList(node.topSymbols, 4)));
    object.insert(QStringLiteral("collaborators"), toJsonArray(limitStringList(node.collaboratorNames, 4)));
    object.insert(QStringLiteral("stageName"), trimAndClamp(QString::fromStdString(node.stageName), 80));
    object.insert(QStringLiteral("evidence"), buildEvidenceObject(node.evidence));
    return object;
}

QString buildReportDigest(const savt::core::AnalysisReport& report) {
    return QStringLiteral("precision=%1; discovered=%2; parsed=%3; semantic=%4")
        .arg(QString::fromStdString(report.precisionLevel).trimmed(),
             QString::number(static_cast<qulonglong>(report.discoveredFiles)),
             QString::number(static_cast<qulonglong>(report.parsedFiles)),
             QString::fromStdString(report.semanticStatusMessage).trimmed());
}

QString compactPreview(QString text, const int maxLength = 180) {
    text = text.trimmed();
    text.replace(QLatin1Char('\r'), QLatin1Char(' '));
    text.replace(QLatin1Char('\n'), QLatin1Char(' '));
    while (text.contains(QStringLiteral("  "))) {
        text.replace(QStringLiteral("  "), QStringLiteral(" "));
    }
    if (text.size() > maxLength) {
        text = text.left(maxLength - 3) + QStringLiteral("...");
    }
    return text;
}

bool startupAnnotationEnabled() {
    const QByteArray rawValue = qgetenv("SAVT_ENABLE_STARTUP_AI_ANNOTATION").trimmed().toLower();
    return rawValue == "1" || rawValue == "true" || rawValue == "yes" || rawValue == "on";
}

std::string buildCacheKey(
    const savt::ai::DeepSeekConfig& config,
    const QByteArray& payload) {
    QByteArray bytes = config.resolvedChatCompletionsUrl().toUtf8();
    bytes += '\n';
    bytes += payload;
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex().toStdString();
}

void logAnnotationStageStart(
    const QString& scope,
    const QString& target,
    const QString& model,
    const int nodeCount,
    const qsizetype promptBytes) {
    qInfo().noquote()
        << QStringLiteral("[AI annotation][start] scope=%1 target=%2 model=%3 nodes=%4 prompt_bytes=%5")
               .arg(scope,
                    target.isEmpty() ? QStringLiteral("unknown") : target,
                    model,
                    QString::number(nodeCount),
                    QString::number(promptBytes));
}

void logAnnotationStageCacheHit(
    const QString& scope,
    const QString& target,
    const int nodeCount,
    const std::size_t annotationCount,
    const qsizetype promptBytes) {
    qInfo().noquote()
        << QStringLiteral("[AI annotation][cache-hit] scope=%1 target=%2 nodes=%3 annotations=%4 prompt_bytes=%5")
               .arg(scope,
                    target.isEmpty() ? QStringLiteral("unknown") : target,
                    QString::number(nodeCount),
                    QString::number(static_cast<qulonglong>(annotationCount)),
                    QString::number(promptBytes));
}

void logAnnotationStageSuccess(
    const QString& scope,
    const QString& target,
    const int nodeCount,
    const BatchAnnotationResult& result) {
    qInfo().noquote()
        << QStringLiteral("[AI annotation][done] scope=%1 target=%2 nodes=%3 annotations=%4 elapsed_ms=%5 response_bytes=%6 preview=%7")
               .arg(scope,
                    target.isEmpty() ? QStringLiteral("unknown") : target,
                    QString::number(nodeCount),
                    QString::number(static_cast<qulonglong>(result.annotations.size())),
                    QString::number(result.elapsedMs),
                    QString::number(result.responseBytes),
                    compactPreview(result.graphSummary));
}

void logAnnotationStageFailure(
    const QString& scope,
    const QString& target,
    const int nodeCount,
    const qint64 elapsedMs,
    const QString& errorMessage) {
    qWarning().noquote()
        << QStringLiteral("[AI annotation][failed] scope=%1 target=%2 nodes=%3 elapsed_ms=%4 error=%5")
               .arg(scope,
                    target.isEmpty() ? QStringLiteral("unknown") : target,
                    QString::number(nodeCount),
                    QString::number(elapsedMs),
                    compactPreview(errorMessage, 260));
}

QString analysisSystemPrompt() {
    QString prompt = savt::ai::deepSeekSavtSystemPrompt();
    prompt += QStringLiteral(
        " You are also responsible for rewriting node labels and summaries for a desktop architecture reading tool aimed at beginner vibecoders."
        " Prefer natural, concrete Chinese labels over raw file paths, class names, symbol dumps, or mechanical formulas."
        " Avoid writing labels like '围绕X' or descriptions that merely list identifiers."
        " If architecture jargon is necessary, explain it in everyday language.");
    return prompt;
}

QByteArray buildBatchPayload(
    const savt::ai::DeepSeekConfig& config,
    const QString& userPrompt) {
    QJsonArray messages;
    messages.push_back(QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                                   {QStringLiteral("content"), analysisSystemPrompt()}});
    messages.push_back(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                   {QStringLiteral("content"), userPrompt}});

    QJsonObject payload;
    payload.insert(QStringLiteral("model"), config.model);
    payload.insert(QStringLiteral("messages"), messages);
    payload.insert(QStringLiteral("temperature"), 0.15);
    payload.insert(QStringLiteral("stream"), false);
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

QString buildCapabilityPrompt(
    const QString& projectRootPath,
    const savt::core::AnalysisReport& report,
    const savt::core::CapabilityGraph& graph) {
    QJsonArray nodes;
    for (const savt::core::CapabilityNode& node : graph.nodes) {
        nodes.push_back(buildCapabilityNodeObject(node));
    }

    QJsonObject request;
    request.insert(QStringLiteral("projectName"), projectNameFromRootPath(projectRootPath));
    request.insert(QStringLiteral("projectRootPath"), QDir::toNativeSeparators(projectRootPath));
    request.insert(QStringLiteral("analysisDigest"), buildReportDigest(report));
    request.insert(QStringLiteral("scope"), QStringLiteral("l2_capability_map"));
    request.insert(QStringLiteral("audience"), QStringLiteral("beginner vibecoder with limited coding background"));
    request.insert(QStringLiteral("graphSummary"), QString::fromStdString(graph.diagnostics.empty() ? std::string{} : graph.diagnostics.front()));
    request.insert(QStringLiteral("nodes"), nodes);

    QString prompt;
    prompt += QStringLiteral("Task: rewrite SAVT capability-map nodes into natural Simplified Chinese for a beginner vibecoder.\n");
    prompt += QStringLiteral("Use only the supplied evidence. Keep claims conservative when the evidence is weak.\n");
    prompt += QStringLiteral("Return exactly one JSON object with this shape and nothing else:\n");
    prompt += QStringLiteral("{\n");
    prompt += QStringLiteral("  \"graph_summary\": \"optional short summary\",\n");
    prompt += QStringLiteral("  \"nodes\": [\n");
    prompt += QStringLiteral("    {\n");
    prompt += QStringLiteral("      \"id\": 1,\n");
    prompt += QStringLiteral("      \"label\": \"human-friendly title\",\n");
    prompt += QStringLiteral("      \"summary\": \"1-2 sentences in plain Chinese\",\n");
    prompt += QStringLiteral("      \"responsibility\": \"2-3 short sentences explaining what it handles and what it does not handle\",\n");
    prompt += QStringLiteral("      \"confidence\": \"high|medium|low\",\n");
    prompt += QStringLiteral("      \"confidence_reason\": \"one short reason\"\n");
    prompt += QStringLiteral("    }\n");
    prompt += QStringLiteral("  ]\n");
    prompt += QStringLiteral("}\n");
    prompt += QStringLiteral("Rules:\n");
    prompt += QStringLiteral("- Keep each node id unchanged.\n");
    prompt += QStringLiteral("- Labels should sound like product or feature names a non-programmer can read quickly.\n");
    prompt += QStringLiteral("- Do not use raw file paths, class names, symbol lists, slash-separated paths, or formulas like '围绕xxx'.\n");
    prompt += QStringLiteral("- Prefer what-the-part-does over how-it-is-implemented.\n");
    prompt += QStringLiteral("- Reply in Simplified Chinese.\n");
    prompt += QStringLiteral("Evidence package:\n");
    prompt += QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Indented));
    return prompt;
}

QString buildComponentPrompt(
    const QString& projectRootPath,
    const savt::core::AnalysisReport& report,
    const savt::core::ComponentGraph& graph) {
    QJsonArray nodes;
    for (const savt::core::ComponentNode& node : graph.nodes) {
        nodes.push_back(buildComponentNodeObject(node));
    }

    QJsonObject request;
    request.insert(QStringLiteral("projectName"), projectNameFromRootPath(projectRootPath));
    request.insert(QStringLiteral("projectRootPath"), QDir::toNativeSeparators(projectRootPath));
    request.insert(QStringLiteral("analysisDigest"), buildReportDigest(report));
    request.insert(QStringLiteral("scope"), QStringLiteral("l3_component_workbench"));
    request.insert(QStringLiteral("audience"), QStringLiteral("beginner vibecoder with limited coding background"));
    request.insert(QStringLiteral("capabilityName"), QString::fromStdString(graph.capabilityName));
    request.insert(QStringLiteral("capabilitySummary"), QString::fromStdString(graph.capabilitySummary));
    request.insert(QStringLiteral("nodes"), nodes);

    QString prompt;
    prompt += QStringLiteral("Task: rewrite SAVT component-workbench nodes into plain Simplified Chinese for a beginner vibecoder.\n");
    prompt += QStringLiteral("Use only the supplied evidence. Treat the current capability as the working context.\n");
    prompt += QStringLiteral("Return exactly one JSON object with this shape and nothing else:\n");
    prompt += QStringLiteral("{\n");
    prompt += QStringLiteral("  \"graph_summary\": \"optional short summary of this capability\",\n");
    prompt += QStringLiteral("  \"nodes\": [\n");
    prompt += QStringLiteral("    {\n");
    prompt += QStringLiteral("      \"id\": 1,\n");
    prompt += QStringLiteral("      \"label\": \"human-friendly title\",\n");
    prompt += QStringLiteral("      \"summary\": \"1-2 sentences in plain Chinese\",\n");
    prompt += QStringLiteral("      \"responsibility\": \"2-3 short sentences explaining what this part is responsible for, and what it is not responsible for\",\n");
    prompt += QStringLiteral("      \"confidence\": \"high|medium|low\",\n");
    prompt += QStringLiteral("      \"confidence_reason\": \"one short reason\"\n");
    prompt += QStringLiteral("    }\n");
    prompt += QStringLiteral("  ]\n");
    prompt += QStringLiteral("}\n");
    prompt += QStringLiteral("Rules:\n");
    prompt += QStringLiteral("- Keep each node id unchanged.\n");
    prompt += QStringLiteral("- Turn code-ish names into what-this-part-does labels.\n");
    prompt += QStringLiteral("- Avoid raw identifiers, file paths, and symbol dumps unless they are essential to understanding.\n");
    prompt += QStringLiteral("- The wording must be easy to read even for someone who cannot explain parser, AST, or dependency graph in technical terms.\n");
    prompt += QStringLiteral("- Reply in Simplified Chinese.\n");
    prompt += QStringLiteral("Evidence package:\n");
    prompt += QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Indented));
    return prompt;
}

QString extractJsonObjectText(QString text) {
    text = text.trimmed();
    const int start = text.indexOf(QLatin1Char('{'));
    if (start < 0) {
        return text;
    }

    int depth = 0;
    bool inString = false;
    bool escaping = false;
    for (int index = start; index < text.size(); ++index) {
        const QChar ch = text.at(index);
        if (escaping) {
            escaping = false;
            continue;
        }
        if (ch == QLatin1Char('\\')) {
            escaping = inString;
            continue;
        }
        if (ch == QLatin1Char('"')) {
            inString = !inString;
            continue;
        }
        if (inString) {
            continue;
        }
        if (ch == QLatin1Char('{')) {
            ++depth;
        } else if (ch == QLatin1Char('}')) {
            --depth;
            if (depth == 0) {
                return text.mid(start, index - start + 1).trimmed();
            }
        }
    }
    return text;
}

bool requestBatchAnnotations(
    QNetworkAccessManager& manager,
    const savt::ai::DeepSeekConfig& config,
    const QString& scope,
    const QString& target,
    const int nodeCount,
    const QString& prompt,
    BatchAnnotationResult* outResult,
    QString* errorMessage) {
    if (!outResult) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("AI annotation result pointer was null.");
        }
        return false;
    }

    BatchAnnotationResult result;
    const QByteArray payload = buildBatchPayload(config, prompt);
    result.promptBytes = payload.size();
    const std::string cacheKey = buildCacheKey(config, payload);

    {
        std::lock_guard<std::mutex> lock(annotationCacheMutex());
        const auto it = annotationCache().find(cacheKey);
        if (it != annotationCache().end()) {
            result.cacheHit = true;
            result.graphSummary = it->second.graphSummary;
            result.annotations = it->second.annotations;
            logAnnotationStageCacheHit(
                scope, target, nodeCount, result.annotations.size(), result.promptBytes);
            *outResult = std::move(result);
            if (errorMessage) {
                errorMessage->clear();
            }
            return true;
        }
    }

    logAnnotationStageStart(scope, target, config.model, nodeCount, result.promptBytes);
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    QNetworkReply* reply = manager.post(
        savt::ai::buildDeepSeekChatCompletionsRequest(config), payload);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool timedOut = false;

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        timedOut = true;
        if (reply) {
            reply->abort();
        }
        loop.quit();
    });

    timer.start(std::max(3000, config.timeoutMs + 2000));
    loop.exec();
    timer.stop();

    const QByteArray responseBytes = reply->readAll();
    result.elapsedMs = elapsedTimer.elapsed();
    result.responseBytes = responseBytes.size();
    const auto networkError = reply->error();
    const QString networkErrorString = reply->errorString();
    reply->deleteLater();

    QString extractedText;
    QString parseError;
    const bool extracted = savt::ai::extractDeepSeekChatCompletionsText(
        responseBytes, &extractedText, &parseError);

    if (timedOut) {
        QString failureMessage = QStringLiteral("AI annotation timed out.");
        if (errorMessage) {
            *errorMessage = failureMessage;
            if (!parseError.isEmpty()) {
                *errorMessage += QStringLiteral("\n%1").arg(parseError);
            }
            failureMessage = *errorMessage;
        } else if (!parseError.isEmpty()) {
            failureMessage += QStringLiteral("\n%1").arg(parseError);
        }
        logAnnotationStageFailure(scope, target, nodeCount, result.elapsedMs, failureMessage);
        return false;
    }

    if (networkError != QNetworkReply::NoError && !extracted) {
        QString failureMessage =
            QStringLiteral("AI annotation request failed: %1")
                .arg(networkErrorString.trimmed());
        if (errorMessage) {
            *errorMessage = failureMessage;
            if (!parseError.isEmpty()) {
                *errorMessage += QStringLiteral("\n%1").arg(parseError);
            }
            failureMessage = *errorMessage;
        } else if (!parseError.isEmpty()) {
            failureMessage += QStringLiteral("\n%1").arg(parseError);
        }
        logAnnotationStageFailure(scope, target, nodeCount, result.elapsedMs, failureMessage);
        return false;
    }

    if (!extracted) {
        const QString failureMessage = parseError.isEmpty()
            ? QStringLiteral("AI annotation response could not be parsed.")
            : parseError;
        if (errorMessage) {
            *errorMessage = failureMessage;
        }
        logAnnotationStageFailure(scope, target, nodeCount, result.elapsedMs, failureMessage);
        return false;
    }

    QString graphSummary;
    std::vector<NodeAnnotation> annotations;
    QString batchParseError;
    if (!parseBatchAnnotations(
            extractedText,
            &annotations,
            &graphSummary,
            &batchParseError)) {
        if (errorMessage) {
            *errorMessage = batchParseError;
        }
        logAnnotationStageFailure(scope, target, nodeCount, result.elapsedMs, batchParseError);
        return false;
    }

    result.graphSummary = std::move(graphSummary);
    result.annotations = std::move(annotations);

    {
        std::lock_guard<std::mutex> lock(annotationCacheMutex());
        annotationCache()[cacheKey] = CachedBatchAnnotationResponse{
            result.graphSummary,
            result.annotations};
    }
    logAnnotationStageSuccess(scope, target, nodeCount, result);

    *outResult = std::move(result);
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

std::size_t parseAnnotationId(const QJsonObject& object) {
    if (object.value(QStringLiteral("id")).isDouble()) {
        return static_cast<std::size_t>(object.value(QStringLiteral("id")).toDouble());
    }
    return static_cast<std::size_t>(object.value(QStringLiteral("id")).toString().toULongLong());
}

bool parseBatchAnnotations(
    const QString& rawText,
    std::vector<NodeAnnotation>* outAnnotations,
    QString* graphSummary,
    QString* errorMessage) {
    if (!outAnnotations) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Annotation output pointer was null.");
        }
        return false;
    }

    const QString jsonText = extractJsonObjectText(rawText);
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to parse AI annotation JSON: %1")
                .arg(parseError.errorString());
        }
        return false;
    }

    const QJsonObject root = document.object();
    if (graphSummary) {
        *graphSummary = root.value(QStringLiteral("graph_summary")).toString().trimmed();
    }

    QJsonArray nodes = root.value(QStringLiteral("nodes")).toArray();
    if (nodes.isEmpty()) {
        nodes = root.value(QStringLiteral("annotations")).toArray();
    }
    if (nodes.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("AI annotation JSON did not contain a nodes array.");
        }
        return false;
    }

    std::vector<NodeAnnotation> annotations;
    annotations.reserve(static_cast<std::size_t>(nodes.size()));
    for (const QJsonValue& value : nodes) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        const std::size_t id = parseAnnotationId(object);
        if (id == 0) {
            continue;
        }

        NodeAnnotation annotation;
        annotation.id = id;
        annotation.label = object.value(QStringLiteral("label")).toString().trimmed();
        if (annotation.label.isEmpty()) {
            annotation.label = object.value(QStringLiteral("name")).toString().trimmed();
        }
        annotation.summary = object.value(QStringLiteral("summary")).toString().trimmed();
        annotation.responsibility = object.value(QStringLiteral("responsibility")).toString().trimmed();
        annotation.confidence = object.value(QStringLiteral("confidence")).toString().trimmed();
        annotation.confidenceReason = object.value(QStringLiteral("confidence_reason")).toString().trimmed();
        if (annotation.summary.isEmpty() && annotation.responsibility.isEmpty() &&
            annotation.label.isEmpty()) {
            continue;
        }
        annotations.push_back(std::move(annotation));
    }

    if (annotations.empty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("AI annotation JSON was valid but contained no usable node entries.");
        }
        return false;
    }

    *outAnnotations = std::move(annotations);
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

const savt::core::CapabilityNode* findCapabilityNode(
    const savt::core::CapabilityGraph& graph,
    const std::size_t id) {
    const auto it = std::find_if(
        graph.nodes.begin(), graph.nodes.end(), [&](const savt::core::CapabilityNode& node) {
            return node.id == id;
        });
    return it == graph.nodes.end() ? nullptr : &(*it);
}

std::size_t applyCapabilityAnnotations(
    savt::core::CapabilityGraph& graph,
    const std::vector<NodeAnnotation>& annotations) {
    std::unordered_map<std::size_t, NodeAnnotation> annotationIndex;
    for (const NodeAnnotation& annotation : annotations) {
        annotationIndex.emplace(annotation.id, annotation);
    }

    std::size_t appliedCount = 0;
    for (savt::core::CapabilityNode& node : graph.nodes) {
        const auto it = annotationIndex.find(node.id);
        if (it == annotationIndex.end()) {
            continue;
        }
        const NodeAnnotation& annotation = it->second;
        if (!annotation.label.isEmpty()) {
            node.name = annotation.label.toStdString();
        }
        if (!annotation.responsibility.isEmpty()) {
            node.responsibility = annotation.responsibility.toStdString();
        }
        if (!annotation.summary.isEmpty()) {
            node.summary = annotation.summary.toStdString();
        }
        ++appliedCount;
    }
    return appliedCount;
}

std::size_t applyComponentAnnotations(
    savt::core::ComponentGraph& graph,
    const std::vector<NodeAnnotation>& annotations,
    const QString& graphSummary) {
    std::unordered_map<std::size_t, NodeAnnotation> annotationIndex;
    for (const NodeAnnotation& annotation : annotations) {
        annotationIndex.emplace(annotation.id, annotation);
    }

    std::size_t appliedCount = 0;
    for (savt::core::ComponentNode& node : graph.nodes) {
        const auto it = annotationIndex.find(node.id);
        if (it == annotationIndex.end()) {
            continue;
        }
        const NodeAnnotation& annotation = it->second;
        if (!annotation.label.isEmpty()) {
            node.name = annotation.label.toStdString();
        }
        if (!annotation.responsibility.isEmpty()) {
            node.responsibility = annotation.responsibility.toStdString();
        }
        if (!annotation.summary.isEmpty()) {
            node.summary = annotation.summary.toStdString();
        }
        ++appliedCount;
    }

    if (!graphSummary.trimmed().isEmpty()) {
        graph.capabilitySummary = graphSummary.trimmed().toStdString();
    }
    return appliedCount;
}

void syncComponentGraphHeaders(
    const savt::core::CapabilityGraph& capabilityGraph,
    std::unordered_map<std::size_t, savt::core::ComponentGraph>& componentGraphs) {
    for (auto& [capabilityId, graph] : componentGraphs) {
        if (const savt::core::CapabilityNode* capabilityNode =
                findCapabilityNode(capabilityGraph, capabilityId);
            capabilityNode != nullptr) {
            graph.capabilityName = capabilityNode->name;
            graph.capabilitySummary = capabilityNode->summary;
        }
    }
}

AnalysisAnnotationStats annotateCapabilityGraphImpl(
    const QString& projectRootPath,
    const savt::core::AnalysisReport& report,
    savt::core::CapabilityGraph& capabilityGraph,
    const savt::ai::DeepSeekConfig& config,
    QNetworkAccessManager& networkManager) {
    AnalysisAnnotationStats stats;
    stats.configured = true;
    stats.model = config.model;

    BatchAnnotationResult capabilityResult;
    QString capabilityError;
    stats.requestCount = 1;
    if (requestBatchAnnotations(
            networkManager,
            config,
            QStringLiteral("capability_graph"),
            QStringLiteral("project"),
            static_cast<int>(capabilityGraph.nodes.size()),
            buildCapabilityPrompt(projectRootPath, report, capabilityGraph),
            &capabilityResult,
            &capabilityError)) {
        stats.totalElapsedMs = capabilityResult.elapsedMs;
        stats.cacheHitCount = capabilityResult.cacheHit ? 1 : 0;
        stats.capabilityNodeCount =
            applyCapabilityAnnotations(capabilityGraph, capabilityResult.annotations);
        stats.applied = stats.capabilityNodeCount > 0;
        capabilityGraph.diagnostics.push_back(
            "AI annotation pass rewrote capability labels and summaries for beginner-friendly reading.");
    } else {
        capabilityGraph.diagnostics.push_back(
            (QStringLiteral("AI capability annotation skipped: %1")
                 .arg(capabilityError)).toStdString());
    }

    stats.statusMessage = stats.applied
        ? QStringLiteral("AI capability annotation applied with model %1.").arg(stats.model)
        : (capabilityError.isEmpty()
               ? QStringLiteral("AI capability annotation produced no updates.")
               : capabilityError);
    return stats;
}

AnalysisAnnotationStats annotateComponentGraphImpl(
    const QString& projectRootPath,
    const savt::core::AnalysisReport& report,
    savt::core::ComponentGraph& componentGraph,
    const savt::ai::DeepSeekConfig& config,
    QNetworkAccessManager& networkManager) {
    AnalysisAnnotationStats stats;
    stats.configured = true;
    stats.model = config.model;

    const QString targetName = QString::fromStdString(componentGraph.capabilityName).trimmed();
    if (componentGraph.nodes.size() <= 1) {
        stats.skippedComponentGraphCount = 1;
        stats.statusMessage =
            QStringLiteral("AI component annotation skipped for a trivial graph.");
        componentGraph.diagnostics.push_back(
            "AI component annotation skipped for a trivial graph; heuristic labels remain active.");
        qInfo().noquote()
            << QStringLiteral("[AI annotation][skipped] scope=component_graph target=%1 reason=trivial_graph nodes=%2")
                   .arg(targetName.isEmpty() ? QStringLiteral("unknown") : targetName,
                        QString::number(static_cast<qulonglong>(componentGraph.nodes.size())));
        return stats;
    }

    BatchAnnotationResult componentResult;
    QString componentError;
    stats.requestCount = 1;
    if (requestBatchAnnotations(
            networkManager,
            config,
            QStringLiteral("component_graph"),
            targetName,
            static_cast<int>(componentGraph.nodes.size()),
            buildComponentPrompt(projectRootPath, report, componentGraph),
            &componentResult,
            &componentError)) {
        stats.totalElapsedMs = componentResult.elapsedMs;
        stats.cacheHitCount = componentResult.cacheHit ? 1 : 0;
        stats.componentNodeCount =
            applyComponentAnnotations(
                componentGraph,
                componentResult.annotations,
                componentResult.graphSummary);
        stats.applied = stats.componentNodeCount > 0;
        if (stats.applied) {
            componentGraph.diagnostics.push_back(
                "AI annotation pass rewrote component labels and summaries for beginner-friendly reading.");
        }
    } else {
        componentGraph.diagnostics.push_back(
            (QStringLiteral("AI component annotation skipped: %1")
                 .arg(componentError)).toStdString());
    }

    stats.statusMessage = stats.applied
        ? QStringLiteral("AI component annotation applied with model %1.").arg(stats.model)
        : (componentError.isEmpty()
               ? QStringLiteral("AI component annotation produced no updates.")
               : componentError);
    return stats;
}

}  // namespace

AnalysisAnnotationStats AnalysisAnnotationService::annotateCapabilityGraph(
    const QString& projectRootPath,
    const savt::core::AnalysisReport& report,
    savt::core::CapabilityGraph& capabilityGraph) {
    const savt::ai::DeepSeekConfigLoadResult loadResult = savt::ai::loadDeepSeekConfig();
    if (!loadResult.loadedFromFile || !loadResult.config.isUsable()) {
        AnalysisAnnotationStats stats;
        stats.statusMessage = loadResult.errorMessage.isEmpty()
            ? QStringLiteral("AI annotation is not configured.")
            : loadResult.errorMessage;
        capabilityGraph.diagnostics.push_back(
            "AI capability annotation skipped because no usable AI configuration was found.");
        return stats;
    }

    QNetworkAccessManager networkManager;
    return annotateCapabilityGraphImpl(
        projectRootPath, report, capabilityGraph, loadResult.config, networkManager);
}

AnalysisAnnotationStats AnalysisAnnotationService::annotateComponentGraph(
    const QString& projectRootPath,
    const savt::core::AnalysisReport& report,
    savt::core::ComponentGraph& componentGraph) {
    const savt::ai::DeepSeekConfigLoadResult loadResult = savt::ai::loadDeepSeekConfig();
    if (!loadResult.loadedFromFile || !loadResult.config.isUsable()) {
        AnalysisAnnotationStats stats;
        stats.statusMessage = loadResult.errorMessage.isEmpty()
            ? QStringLiteral("AI annotation is not configured.")
            : loadResult.errorMessage;
        componentGraph.diagnostics.push_back(
            "AI component annotation skipped because no usable AI configuration was found.");
        return stats;
    }

    QNetworkAccessManager networkManager;
    return annotateComponentGraphImpl(
        projectRootPath, report, componentGraph, loadResult.config, networkManager);
}

AnalysisAnnotationStats AnalysisAnnotationService::annotate(
    const QString& projectRootPath,
    const savt::core::AnalysisReport& report,
    savt::core::CapabilityGraph& capabilityGraph,
    std::unordered_map<std::size_t, savt::core::ComponentGraph>& componentGraphs) {
    AnalysisAnnotationStats stats;
    QElapsedTimer totalTimer;
    totalTimer.start();

    if (!startupAnnotationEnabled()) {
        stats.statusMessage = QStringLiteral(
            "Startup AI annotation is disabled to keep initial analysis fast.");
        capabilityGraph.diagnostics.push_back(
            "AI annotation pass skipped during startup; set SAVT_ENABLE_STARTUP_AI_ANNOTATION=1 to opt in.");
        for (auto& [capabilityId, componentGraph] : componentGraphs) {
            Q_UNUSED(capabilityId);
            componentGraph.diagnostics.push_back(
                "AI annotation pass skipped during startup; use on-demand AI or opt in explicitly.");
        }
        return stats;
    }

    const savt::ai::DeepSeekConfigLoadResult loadResult = savt::ai::loadDeepSeekConfig();
    if (!loadResult.loadedFromFile || !loadResult.config.isUsable()) {
        stats.statusMessage = loadResult.errorMessage.isEmpty()
            ? QStringLiteral("AI annotation is not configured.")
            : loadResult.errorMessage;
        capabilityGraph.diagnostics.push_back(
            "AI annotation pass skipped because no usable AI configuration was found.");
        return stats;
    }

    stats.configured = true;
    stats.model = loadResult.config.model;
    QNetworkAccessManager networkManager;
    BatchAnnotationResult capabilityResult;
    QString capabilityError;
    stats.requestCount += 1;
    if (requestBatchAnnotations(
            networkManager,
            loadResult.config,
            QStringLiteral("capability_graph"),
            QStringLiteral("project"),
            static_cast<int>(capabilityGraph.nodes.size()),
            buildCapabilityPrompt(projectRootPath, report, capabilityGraph),
            &capabilityResult,
            &capabilityError)) {
        stats.totalElapsedMs += capabilityResult.elapsedMs;
        stats.cacheHitCount += capabilityResult.cacheHit ? 1 : 0;
        stats.capabilityNodeCount =
            applyCapabilityAnnotations(capabilityGraph, capabilityResult.annotations);
        stats.applied = stats.capabilityNodeCount > 0;
        capabilityGraph.diagnostics.push_back(
            "AI annotation pass rewrote capability labels and summaries for beginner-friendly reading.");
    } else {
        capabilityGraph.diagnostics.push_back(
            (QStringLiteral("AI capability annotation skipped: %1")
                 .arg(capabilityError)).toStdString());
    }

    syncComponentGraphHeaders(capabilityGraph, componentGraphs);

    for (auto& [capabilityId, componentGraph] : componentGraphs) {
        Q_UNUSED(capabilityId);
        const QString targetName = QString::fromStdString(componentGraph.capabilityName).trimmed();
        if (componentGraph.nodes.size() <= 1) {
            stats.skippedComponentGraphCount += 1;
            componentGraph.diagnostics.push_back(
                "AI component annotation skipped for a trivial graph; heuristic labels remain active.");
            qInfo().noquote()
                << QStringLiteral("[AI annotation][skipped] scope=component_graph target=%1 reason=trivial_graph nodes=%2")
                       .arg(targetName.isEmpty() ? QStringLiteral("unknown") : targetName,
                            QString::number(static_cast<qulonglong>(componentGraph.nodes.size())));
            continue;
        }

        BatchAnnotationResult componentResult;
        QString componentError;
        stats.requestCount += 1;
        if (!requestBatchAnnotations(
                networkManager,
                loadResult.config,
                QStringLiteral("component_graph"),
                targetName,
                static_cast<int>(componentGraph.nodes.size()),
                buildComponentPrompt(projectRootPath, report, componentGraph),
                &componentResult,
                &componentError)) {
            componentGraph.diagnostics.push_back(
                (QStringLiteral("AI component annotation skipped: %1")
                     .arg(componentError)).toStdString());
            continue;
        }

        const std::size_t appliedCount =
            applyComponentAnnotations(
                componentGraph,
                componentResult.annotations,
                componentResult.graphSummary);
        stats.totalElapsedMs += componentResult.elapsedMs;
        stats.cacheHitCount += componentResult.cacheHit ? 1 : 0;
        stats.componentNodeCount += appliedCount;
        if (appliedCount > 0) {
            stats.applied = true;
            componentGraph.diagnostics.push_back(
                "AI annotation pass rewrote component labels and summaries for beginner-friendly reading.");
        }
    }

    syncComponentGraphHeaders(capabilityGraph, componentGraphs);
    stats.statusMessage = stats.applied
        ? QStringLiteral("AI annotation applied with model %1.").arg(stats.model)
        : QStringLiteral("AI annotation was configured, but no node text was updated.");
    qInfo().noquote()
        << QStringLiteral("[AI annotation][summary] model=%1 requests=%2 cache_hits=%3 skipped_component_graphs=%4 capability_nodes=%5 component_nodes=%6 total_elapsed_ms=%7 wall_clock_ms=%8")
               .arg(stats.model,
                    QString::number(static_cast<qulonglong>(stats.requestCount)),
                    QString::number(static_cast<qulonglong>(stats.cacheHitCount)),
                    QString::number(static_cast<qulonglong>(stats.skippedComponentGraphCount)),
                    QString::number(static_cast<qulonglong>(stats.capabilityNodeCount)),
                    QString::number(static_cast<qulonglong>(stats.componentNodeCount)),
                    QString::number(stats.totalElapsedMs),
                    QString::number(totalTimer.elapsed()));
    return stats;
}

}  // namespace savt::ui
