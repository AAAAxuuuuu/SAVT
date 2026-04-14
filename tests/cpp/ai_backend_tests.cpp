#include "savt/ai/DeepSeekClient.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QTemporaryDir>

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
    expect(containsText(prompt, QStringLiteral("audience is beginner")),
           "system prompt should explain how to adapt output for beginners");
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
    request.uiScope = QStringLiteral("l3_module_guide");
    request.learningStage = QStringLiteral("L3");
    request.audience = QStringLiteral("beginner");
    request.explanationGoal = QStringLiteral("Help a beginner understand what this module does.");
    request.nodeName = QStringLiteral("App Backend Algorithm");
    request.nodeKind = QStringLiteral("capability");
    request.nodeRole = QStringLiteral("analysis");
    request.nodeSummary = QStringLiteral("Extracts and computes architecture facts from source code.");
    request.userTask = QStringLiteral("Explain this module to a beginner and suggest what to read first.");
    request.moduleNames = {QStringLiteral("App/backend/algorithm")};
    request.exampleFiles = {QStringLiteral("App/backend/algorithm/AlgorithmLibrary.cpp")};
    request.topSymbols = {QStringLiteral("AlgorithmLibrary"), QStringLiteral("IncomingEdge")};
    request.collaboratorNames = {QStringLiteral("App Backend Facade"), QStringLiteral("App Backend Graph")};
    request.filePath = QStringLiteral("App/backend/algorithm/AlgorithmLibrary.cpp");
    request.fileLanguage = QStringLiteral("C++");
    request.fileCategory = QStringLiteral("源码文件");
    request.fileRoleHint = QStringLiteral("服务/编排");
    request.fileSummary = QStringLiteral("围绕 AlgorithmLibrary 组织分析流程。");
    request.codeExcerpt = QStringLiteral("class AlgorithmLibrary {\npublic:\n  void build();\n};");
    request.fileImports = {QStringLiteral("#include \"AlgorithmLibrary.h\"")};
    request.fileDeclarations = {QStringLiteral("AlgorithmLibrary"), QStringLiteral("IncomingEdge")};
    request.fileSignals = {QStringLiteral("包含数据转换或序列化线索")};
    request.fileReadingHints = {QStringLiteral("先看 AlgorithmLibrary::build")};
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
    expect(containsText(userPrompt, QStringLiteral("Learning stage: L3")),
           "user prompt should include the target learning stage");
    expect(containsText(userPrompt, QStringLiteral("Audience: beginner")),
           "user prompt should include the target audience");
    expect(containsText(userPrompt, QStringLiteral("Goal: Help a beginner understand what this module does.")),
           "user prompt should include the explanation goal");
    expect(containsText(userPrompt, QStringLiteral("\"guide\"")),
           "user prompt evidence package should include guide metadata");
    expect(containsText(userPrompt, QStringLiteral("\"file\"")),
           "user prompt evidence package should include file metadata when available");
    expect(containsText(userPrompt, QStringLiteral("AlgorithmLibrary::build")),
           "user prompt evidence package should include file reading hints");
    expect(containsText(userPrompt, QStringLiteral("\"uiScope\": \"l3_module_guide\"")),
           "user prompt evidence package should serialize the UI scope");
    expect(containsText(userPrompt, QStringLiteral("plain_summary")),
           "user prompt should advertise the beginner-friendly optional fields");
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
                    "content": "{\"summary\":\"这是核心算法模块\",\"plain_summary\":\"这是项目里负责整理架构信息的一块逻辑。你可以先把它理解成把代码事实整理成可读结果的地方。\",\"responsibility\":\"负责整理和计算项目里的图结构证据\",\"why_it_matters\":\"如果你想知道系统如何从代码里提炼出结构图，这里是关键入口之一\",\"collaborators\":[\"App Backend Facade\",\"App Backend Graph\"],\"evidence\":[\"代表文件：App/backend/algorithm/AlgorithmLibrary.cpp\",\"关键符号：AlgorithmLibrary\"],\"where_to_start\":[\"先看 AlgorithmLibrary.cpp\",\"再看它和 Facade 的连接处\"],\"glossary\":[\"图结构证据: 指分析器整理出来的节点和关系信息\"],\"uncertainty\":\"目前看不到完整语义调用链，所以只给出保守判断\",\"next_actions\":[\"核对 Facade 到 Algorithm 的边\"]}"
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
    expect(insight.summary == QStringLiteral("这是核心算法模块"), "summary should be extracted from the model response");
    expect(insight.plainSummary.contains(QStringLiteral("整理架构信息")), "plain summary should be extracted for beginner output");
    expect(insight.whyItMatters.contains(QStringLiteral("关键入口")), "why_it_matters should be extracted from the model response");
    expect(insight.whereToStart.size() == 2, "where_to_start list should be parsed");
    expect(insight.glossary.size() == 1, "glossary list should be parsed");
    expect(insight.nextActions.size() == 1, "next actions list should be parsed");
    expect(rawContent.contains(QStringLiteral("plain_summary")), "raw content should preserve the extended model JSON body");
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
                        "text": "{\"summary\":\"兼容网关输出摘要\",\"plain_summary\":\"这是兼容层返回的新手摘要。\",\"responsibility\":\"把第三方 output_text 包装层还原为 SAVT Insight JSON\",\"collaborators\":[\"Compatibility Gateway\"],\"evidence\":[\"output[0].content[0].text\"],\"uncertainty\":\"这是兼容回归测试\",\"next_actions\":[\"保持 output_text 兼容\"]}"
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
    expect(insight.summary.contains(QStringLiteral("兼容网关")), "summary should be parsed from output_text content");
    expect(insight.plainSummary.contains(QStringLiteral("新手摘要")), "plain summary should be parsed from output_text content");
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

void testFileConfigTakesPrecedenceOverBuiltInDefaults() {
    QTemporaryDir tempDir;
    expect(tempDir.isValid(), "temporary directory should be created");
    if (!tempDir.isValid()) {
        return;
    }

    const QString configDirPath = tempDir.filePath(QStringLiteral("config"));
    QDir().mkpath(configDirPath);
    QFile configFile(QDir(configDirPath).filePath(QStringLiteral("deepseek-ai.local.json")));
    const bool opened = configFile.open(QIODevice::WriteOnly | QIODevice::Text);
    expect(opened, "temporary config file should open for writing");
    if (!opened) {
        return;
    }

    configFile.write(R"JSON({
        "provider": "deepseek",
        "enabled": true,
        "baseUrl": "https://example.com/direct",
        "endpointPath": "/v1/chat/completions",
        "model": "file-priority-model",
        "apiKey": "sk-file-priority"
    })JSON");
    configFile.close();

    const savt::ai::DeepSeekConfigLoadResult loadResult =
        savt::ai::loadDeepSeekConfig(configFile.fileName());
    expect(loadResult.loadedFromFile,
           "explicit config path should load from file even when built-in defaults exist");
    expect(!loadResult.loadedFromBuiltInDefaults,
           "explicit config path should not report built-in defaults as the source");
    expect(loadResult.config.model == QStringLiteral("file-priority-model"),
           "explicit config path should preserve the file model value");
}

void testBuiltInConfigFallbackWhenFileIsMissing() {
    const savt::ai::DeepSeekConfigLoadResult loadResult =
        savt::ai::loadDeepSeekConfig(QStringLiteral("config/definitely-missing-ai-config.json"));
    if (loadResult.loadedFromBuiltInDefaults) {
        expect(loadResult.config.isUsable(),
               "built-in fallback config should still be usable");
        return;
    }

    expect(!loadResult.hasConfig(),
           "missing config file without built-in defaults should report no config");
    expect(loadResult.errorMessage.contains(QStringLiteral("not found"), Qt::CaseInsensitive),
           "missing config without built-in defaults should surface a file-not-found error");
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
    testFileConfigTakesPrecedenceOverBuiltInDefaults();
    testBuiltInConfigFallbackWhenFileIsMissing();

    if (failureCount > 0) {
        std::cerr << "savt_ai_tests failed with " << failureCount << " assertion(s)." << std::endl;
        return 1;
    }

    std::cout << "savt_ai_tests passed" << std::endl;
    return 0;
}
