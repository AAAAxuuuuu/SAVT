#pragma once

#include "savt/ai/DeepSeekClient.h"

#include <QByteArray>
#include <QNetworkRequest>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace savt::ui {

struct AiAvailabilityState {
    bool available = false;
    QString configPath;
    QString setupMessage;
};

struct AiRequestContext {
    QString projectRootPath;
    QString analysisReport;
    QString statusMessage;
    QString analysisPhase;
    QString userTask;
    QString selectedAstFilePath;
    QString astPreviewTitle;
    QString astPreviewSummary;
};

struct AiPreparedRequest {
    bool ready = false;
    QString scope;
    QString targetName;
    QString pendingStatusMessage;
    QString failureStatusMessage;
    AiAvailabilityState availability;
    ai::ArchitectureAssistantRequest assistantRequest;
    QNetworkRequest networkRequest;
    QByteArray payload;
};

struct AiReplyState {
    bool hasResult = false;
    QString statusMessage;
    QString summary;
    QString responsibility;
    QString uncertainty;
    QVariantList collaborators;
    QVariantList evidence;
    QVariantList nextActions;
};

class AiService {
public:
    static AiAvailabilityState inspectAvailability();
    static QString classifyNodeScope(const QVariantMap& nodeData);

    static AiPreparedRequest prepareNodeRequest(
        const AiRequestContext& context,
        const QVariantMap& nodeData);

    static AiPreparedRequest prepareProjectRequest(
        const AiRequestContext& context,
        const QVariantMap& systemContextData,
        const QVariantList& systemContextCards,
        const QVariantList& capabilityNodeItems);

    static AiPreparedRequest prepareReportRequest(
        const AiRequestContext& context,
        const QVariantMap& systemContextData,
        const QVariantList& systemContextCards,
        const QVariantList& capabilityNodeItems);

    static AiReplyState parseReply(
        const QByteArray& responseBytes,
        const QString& scope,
        bool hasNetworkError,
        const QString& networkErrorString);

    static void logRequest(const AiPreparedRequest& request, const QString& targetDebug);
    static void logResponse(const QByteArray& responseBytes, const QString& scope);
};

}  // namespace savt::ui
