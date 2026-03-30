#include "savt/ai/DeepSeekClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

#include <iostream>
#include <string>

namespace {

int failureCount = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] " << message << std::endl;
        ++failureCount;
    }
}

bool containsText(const QString& text, const QString& token) {
    return text.contains(token, Qt::CaseInsensitive);
}

void testPromptGuardrails() {
    const QString prompt = savt::ai::deepSeekSavtSystemPrompt();
    expect(containsText(prompt, QStringLiteral("SAVT")), "system prompt should explicitly mention SAVT");
    expect(containsText(prompt, QStringLiteral("only help")), "system prompt should limit scope to SAVT work");
    expect(containsText(prompt, QStringLiteral("Refuse")), "system prompt should refuse unrelated tasks");
    expect(containsText(prompt, QStringLiteral("Reply in Simplified Chinese")),
           "system prompt should force Chinese output for the tool");
    expect(containsText(prompt, QStringLiteral("Do not invent")),
           "system prompt should forbid fabricated architecture facts");
}

void testOfficialConfigParsing() {
    const QByteArray json = R"JSON({
        "provider": "deepseek",
        "enabled": true,
        "baseUrl": "https://api.deepseek.com",
        "model": "deepseek-chat",
        "apiKey": "sk-test-value",
        "timeoutMs": 45000
    })JSON";

    savt::ai::DeepSeekConfig config;
    QString errorMessage;
    const bool parsed = savt::ai::parseDeepSeekConfigJson(json, &config, &errorMessage);
    expect(parsed, "official config JSON should parse successfully");
    expect(errorMessage.isEmpty(), "successful config parse should not return an error");
    expect(config.isUsable(), "parsed official config should be considered usable");
    expect(config.enabled, "enabled flag should be parsed");
    expect(config.model == QStringLiteral("deepseek-chat"), "model should match config JSON");
    expect(config.timeoutMs == 45000, "timeout should be parsed exactly");
    expect(config.resolvedChatCompletionsUrl() == QStringLiteral("https://api.deepseek.com/chat/completions"),
           "official DeepSeek config should resolve to the native chat completions endpoint");
}

void testCompatibleConfigParsing() {
    const QByteArray json = R"JSON({
        "provider": "cherry-studio-proxy",
        "enabled": true,
        "baseUrl": "https://chat.cqjtu.edu.cn/ds/api",
        "model": "deepseek-chat",
        "apiKey": "sk-compatible-value",
        "timeoutMs": 30000
    })JSON";

    savt::ai::DeepSeekConfig config;
    QString errorMessage;
    const bool parsed = savt::ai::parseDeepSeekConfigJson(json, &config, &errorMessage);
    expect(parsed, "compatible config JSON should parse successfully");
    expect(errorMessage.isEmpty(), "compatible config parse should not return an error");
    expect(config.isUsable(), "parsed compatible config should be considered usable");
    expect(config.providerLabel().contains(QStringLiteral("Cherry")), "compatible provider label should reflect Cherry Studio style config");
    expect(config.resolvedChatCompletionsUrl() == QStringLiteral("https://chat.cqjtu.edu.cn/ds/api/v1/chat/completions"),
           "compatible config should infer the OpenAI-compatible v1 chat completions endpoint");
}

void testExplicitEndpointOverride() {
    const QByteArray json = R"JSON({
        "provider": "custom-gateway",
        "enabled": true,
        "baseUrl": "https://example.com/proxy",
        "endpointPath": "/openai/chat/completions",
        "model": "deepseek-chat",
        "apiKey": "sk-custom-value"
    })JSON";

    savt::ai::DeepSeekConfig config;
    QString errorMessage;
    const bool parsed = savt::ai::parseDeepSeekConfigJson(json, &config, &errorMessage);
    expect(parsed, "config with explicit endpoint path should parse successfully");
    expect(config.resolvedChatCompletionsUrl() == QStringLiteral("https://example.com/proxy/openai/chat/completions"),
           "explicit endpointPath should override the inferred endpoint path");
}

savt::ai::ArchitectureAssistantRequest buildSampleRequest() {
    savt::ai::ArchitectureAssistantRequest request;
    request.projectName = QStringLiteral("SAVT");
    request.projectRootPath = QStringLiteral("G:/SAVT");
    request.analyzerPrecision = QStringLiteral("semantic_preferred");
    request.analysisSummary = QStringLiteral("Capability graph preserves module-level detail.");
    request.nodeName = QStringLiteral("App Backend Algorithm");
    request.nodeKind = QStringLiteral("capability");
    request.nodeRole = QStringLiteral("analysis");
    request.nodeSummary = QStringLiteral("Extracts and computes architecture facts from source code.");
    request.userTask = QStringLiteral("Explain what this module does for the architecture reading UI.");
    request.moduleNames = {QStringLiteral("App/backend/algorithm")};
    request.exampleFiles = {QStringLiteral("App/backend/algorithm/AlgorithmLibrary.cpp")};
    request.topSymbols = {QStringLiteral("AlgorithmLibrary"), QStringLiteral("IncomingEdge")};
    request.collaboratorNames = {QStringLiteral("App Backend Facade"), QStringLiteral("App Backend Graph")};
    request.diagnostics = {QStringLiteral("semantic analysis unavailable")};
    return request;
}

void testRequestPayloadAndHeaders() {
    savt::ai::DeepSeekConfig config;
    config.enabled = true;
    config.apiKey = QStringLiteral("sk-demo-key");

    const savt::ai::ArchitectureAssistantRequest request = buildSampleRequest();
    const QByteArray payloadBytes = savt::ai::buildDeepSeekChatCompletionsPayload(config, request);
    const QJsonDocument payloadDocument = QJsonDocument::fromJson(payloadBytes);
    expect(payloadDocument.isObject(), "payload should be valid JSON object");

    const QJsonObject payload = payloadDocument.object();
    expect(payload.value(QStringLiteral("model")).toString() == QStringLiteral("deepseek-chat"),
           "payload should use the configured model");
    expect(payload.value(QStringLiteral("stream")).toBool() == false,
           "payload should disable streaming in the initial SAVT adapter");

    const QJsonArray messages = payload.value(QStringLiteral("messages")).toArray();
    expect(messages.size() == 2, "payload should contain system and user messages");
    const QString systemPrompt = messages.at(0).toObject().value(QStringLiteral("content")).toString();
    const QString userPrompt = messages.at(1).toObject().value(QStringLiteral("content")).toString();
    expect(containsText(systemPrompt, QStringLiteral("SAVT Architecture Assistant")),
           "system message should identify the restricted SAVT assistant");
    expect(containsText(userPrompt, QStringLiteral("App Backend Algorithm")),
           "user prompt should include the selected node name");
    expect(containsText(userPrompt, QStringLiteral("AlgorithmLibrary.cpp")),
           "user prompt should include example file evidence");
    expect(containsText(userPrompt, QStringLiteral("Return exactly one JSON object")),
           "user prompt should enforce the response contract");

    const QNetworkRequest networkRequest = savt::ai::buildDeepSeekChatCompletionsRequest(config);
    expect(networkRequest.url().toString() == QStringLiteral("https://api.deepseek.com/chat/completions"),
           "request URL should point to the default DeepSeek chat completions endpoint");
    expect(networkRequest.rawHeader("Authorization") == QByteArray("Bearer sk-demo-key"),
           "request should include bearer token authorization");
}

void testCompatibleRequestHeaders() {
    savt::ai::DeepSeekConfig config;
    config.provider = QStringLiteral("cherry-studio-proxy");
    config.enabled = true;
    config.baseUrl = QStringLiteral("https://chat.cqjtu.edu.cn/ds/api");
    config.apiKey = QStringLiteral("sk-demo-compatible-key");

    const QNetworkRequest networkRequest = savt::ai::buildDeepSeekChatCompletionsRequest(config);
    expect(networkRequest.url().toString() == QStringLiteral("https://chat.cqjtu.edu.cn/ds/api/v1/chat/completions"),
           "compatible request URL should point to the inferred v1 chat completions endpoint");
    expect(networkRequest.rawHeader("Authorization") == QByteArray("Bearer sk-demo-compatible-key"),
           "compatible request should still use bearer token authorization");
}

void testResponseParsing() {
    const QByteArray response = R"JSON({
        "id": "demo",
        "choices": [
            {
                "message": {
                    "role": "assistant",
                    "content": "{\"summary\":\"Core algorithm module\",\"responsibility\":\"Computes graph evidence for the architecture view\",\"collaborators\":[\"App Backend Facade\",\"App Backend Graph\"],\"evidence\":[\"File: App/backend/algorithm/AlgorithmLibrary.cpp\",\"Symbol: AlgorithmLibrary\"],\"uncertainty\":\"This is a conservative summary built from the available node evidence\",\"next_actions\":[\"Inspect AlgorithmLibrary.cpp\",\"Verify the edge from Facade to Algorithm\"]}"
                }
            }
        ]
    })JSON";

    savt::ai::ArchitectureAssistantInsight insight;
    QString errorMessage;
    QString rawContent;
    const bool parsed = savt::ai::parseDeepSeekChatCompletionsResponse(response, &insight, &errorMessage, &rawContent);
    expect(parsed, "chat completions response should parse successfully");
    expect(errorMessage.isEmpty(), "successful response parse should not return an error");
    expect(insight.summary == QStringLiteral("Core algorithm module"), "summary should be extracted from the model response");
    expect(insight.responsibility.contains(QStringLiteral("graph evidence")), "responsibility should be extracted from the model response");
    expect(insight.collaborators.size() == 2, "collaborators list should be parsed");
    expect(insight.evidence.size() == 2, "evidence list should be parsed");
    expect(insight.nextActions.size() == 2, "next actions list should be parsed");
    expect(rawContent.contains(QStringLiteral("summary")), "raw content should preserve the model JSON body");
}

void testResponsesApiStyleResponseParsing() {
    const QByteArray response = R"JSON({
        "id": "resp_demo",
        "output": [
            {
                "type": "message",
                "content": [
                    {
                        "type": "output_text",
                        "text": "{\"summary\":\"Structured summary from a compatible gateway\",\"responsibility\":\"Adapts third-party output into SAVT JSON\",\"collaborators\":[\"Compatibility Gateway\"],\"evidence\":[\"output[0].content[0].text\"],\"uncertainty\":\"This is a compatibility regression test\",\"next_actions\":[\"Keep output_text compatibility\"]}"
                    }
                ]
            }
        ]
    })JSON";

    savt::ai::ArchitectureAssistantInsight insight;
    QString errorMessage;
    QString rawContent;
    const bool parsed = savt::ai::parseDeepSeekChatCompletionsResponse(response, &insight, &errorMessage, &rawContent);
    expect(parsed, "responses-api style output should parse successfully");
    expect(errorMessage.isEmpty(), "responses-api style parse should not return an error");
    expect(insight.summary.contains(QStringLiteral("compatible gateway")), "summary should be parsed from output_text content");
    expect(insight.nextActions.size() == 1, "responses-api style parse should preserve next actions");
    expect(rawContent.contains(QStringLiteral("responsibility")), "raw content should preserve output_text JSON body");
}

void testErrorResponseMessageExtraction() {
    const QByteArray response = R"JSON({
        "error": {
            "message": "model not found"
        }
    })JSON";

    savt::ai::ArchitectureAssistantInsight insight;
    QString errorMessage;
    QString rawContent;
    const bool parsed = savt::ai::parseDeepSeekChatCompletionsResponse(response, &insight, &errorMessage, &rawContent);
    expect(!parsed, "error response should fail parsing");
    expect(errorMessage.contains(QStringLiteral("model not found")), "error message should be surfaced from the API response");
}

}  // namespace

int main() {
    testPromptGuardrails();
    testOfficialConfigParsing();
    testCompatibleConfigParsing();
    testExplicitEndpointOverride();
    testRequestPayloadAndHeaders();
    testCompatibleRequestHeaders();
    testResponseParsing();
    testResponsesApiStyleResponseParsing();
    testErrorResponseMessageExtraction();

    if (failureCount > 0) {
        std::cerr << "savt_ai_tests failed with " << failureCount << " assertion(s)." << std::endl;
        return 1;
    }

    std::cout << "savt_ai_tests passed" << std::endl;
    return 0;
}

