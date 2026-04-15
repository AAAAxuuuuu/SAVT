#include "savt/ui/AnalysisController.h"

#include "savt/ui/AiService.h"
#include "savt/ui/AnalysisOrchestrator.h"
#include "savt/ui/AstPreviewService.h"
#include "savt/ui/FileInsightService.h"
#include "savt/ui/ProjectConfigRecommendationService.h"
#include "savt/core/ComponentGraph.h"

#include <QtGui/QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QtGui/QGuiApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QVariantMap>

#include <QtConcurrent/qtconcurrentrun.h>

#include <algorithm>

namespace savt::ui {

namespace {

QStringList toQStringList(const QVariant& value) {
    QStringList items;
    for (const QVariant& entry : value.toList()) {
        const QString cleaned = entry.toString().trimmed();
        if (!cleaned.isEmpty() && !items.contains(cleaned)) {
            items.push_back(cleaned);
        }
    }
    return items;
}

QVariantMap findNodeById(const QVariantList& nodeItems, const qulonglong nodeId) {
    for (const QVariant& item : nodeItems) {
        const QVariantMap node = item.toMap();
        if (node.value(QStringLiteral("id")).toULongLong() == nodeId) {
            return node;
        }
    }
    return {};
}

QVariantMap findNodeById(
    const QVariantMap& componentSceneCatalog,
    const qulonglong nodeId) {
    for (auto it = componentSceneCatalog.constBegin();
         it != componentSceneCatalog.constEnd();
         ++it) {
        const QVariantMap sceneMap = it.value().toMap();
        const QVariantMap targetNode =
            findNodeById(sceneMap.value(QStringLiteral("nodes")).toList(), nodeId);
        if (!targetNode.isEmpty()) {
            return targetNode;
        }
    }
    return {};
}

QString copyContextKindLabel(const QString& kind) {
    if (kind == QStringLiteral("entry")) {
        return QStringLiteral("发起入口（Entry）");
    }
    if (kind == QStringLiteral("entry_component")) {
        return QStringLiteral("入口组件（Entry Component）");
    }
    if (kind == QStringLiteral("infrastructure")) {
        return QStringLiteral("后台支撑（Infrastructure）");
    }
    if (kind == QStringLiteral("support_component")) {
        return QStringLiteral("支撑组件（Support Component）");
    }
    if (kind == QStringLiteral("service")) {
        return QStringLiteral("服务模块（Service）");
    }
    return QStringLiteral("核心模块（Core）");
}

QString collapseWhitespace(QString text) {
    text.replace(QStringLiteral("\r"), QString());
    text.replace(QStringLiteral("\n"), QStringLiteral(" "));
    return text.simplified();
}

QString stripLeadingSectionLabel(QString text, const QString& label) {
    text = text.trimmed();
    text.replace(label, QString());
    return text.trimmed();
}

QString defaultProjectOverviewPrompt() {
    return QStringLiteral(
        "先阅读当前项目的 README、工程配置、关键源码和静态分析线索，再写一段中文项目总览。"
        " 直接说明这个仓库是做什么的、给谁用、核心模块、入口主路径和输入输出。"
        " 如果 README 缺失或说得不清楚，就根据工程结构和代码行为谨慎归纳。"
        " 不要写“从当前结构看”“整体来看”“这个项目很重要”这类套话。"
        " 控制在 120 到 200 个中文字符之间；证据不够时只点出真正的不确定点。");
}

QString stripProjectOverviewBoilerplate(QString text) {
    text = collapseWhitespace(text);
    if (text.isEmpty()) {
        return {};
    }

    static const QList<QRegularExpression> patterns = {
        QRegularExpression(QStringLiteral(
            R"(^(从当前(?:仓库|项目|静态结构|代码结构|目录结构)(?:[^，。]*?)(?:看|来看)[，,、\s]*)")),
        QRegularExpression(QStringLiteral(R"(^(整体来看[，,、\s]*))")),
        QRegularExpression(QStringLiteral(R"(^(总体来看[，,、\s]*))")),
        QRegularExpression(QStringLiteral(R"(^(大体上看[，,、\s]*))")),
        QRegularExpression(QStringLiteral(R"(^(简单来说[，,、\s]*))")),
        QRegularExpression(QStringLiteral(R"(^(可以把它理解成[，,、\s]*))")),
        QRegularExpression(QStringLiteral(R"(^(可以理解为[，,、\s]*))"))
    };

    bool changed = true;
    while (changed) {
        changed = false;
        for (const QRegularExpression& pattern : patterns) {
            QString updated = text;
            updated.replace(pattern, QString());
            updated = updated.trimmed();
            if (updated != text) {
                text = updated;
                changed = true;
            }
        }
    }
    return text;
}

QVariantMap recommendationChoiceById(const QVariantMap& recommendation, const QString& choiceId) {
    const QVariantList choices = recommendation.value(QStringLiteral("choiceItems")).toList();
    for (const QVariant& item : choices) {
        const QVariantMap choice = item.toMap();
        if (choice.value(QStringLiteral("id")).toString().trimmed() == choiceId.trimmed()) {
            return choice;
        }
    }
    return {};
}

QVariantMap recommendationOptionById(const QVariantMap& choice, const QString& optionId) {
    const QVariantList options = choice.value(QStringLiteral("options")).toList();
    for (const QVariant& item : options) {
        const QVariantMap option = item.toMap();
        if (option.value(QStringLiteral("id")).toString().trimmed() == optionId.trimmed()) {
            return option;
        }
    }
    return {};
}

}  // namespace

AnalysisController::AnalysisController(QObject* parent)
    : QObject(parent),
      m_aiNetworkManager(new QNetworkAccessManager(this)),
      m_analysisWatcher(new QFutureWatcher<void>(this)) {
    setProjectRootPathInternal(AnalysisOrchestrator::defaultProjectRootPath(), false);
    setStatusMessage(QStringLiteral("准备就绪。选择一个项目目录后，就可以先看项目分工，再按需进入程序员视图。"));
    setAnalysisReport(QStringLiteral(
        "程序员入口的详细分析报告会显示在这里。\n\n完成项目分析后，你可以在右侧切换到“程序员入口”，查看完整报告和 AST 预览。"));
    setAnalysisPhase(QStringLiteral("等待开始"));
    clearVisualizationState();
    refreshAiAvailability();

    connect(
        m_analysisWatcher,
        &QFutureWatcher<void>::progressValueChanged,
        this,
        [this](const int progressValue) {
            setAnalysisProgress(std::clamp(
                static_cast<double>(progressValue) / 100.0, 0.0, 1.0));
        });
    connect(
        m_analysisWatcher,
        &QFutureWatcher<void>::progressTextChanged,
        this,
        [this](const QString& progressText) {
            if (progressText.isEmpty() || m_stopRequested) {
                return;
            }
            setAnalysisPhase(progressText);
            setStatusMessage(progressText);
        });
    connect(
        m_analysisWatcher,
        &QFutureWatcher<void>::finished,
        this,
        [this]() { finishAnalysis(); });
}

QString AnalysisController::projectRootPath() const { return m_projectRootPath; }

void AnalysisController::setProjectRootPath(const QString& value) {
    setProjectRootPathInternal(QDir::cleanPath(value.trimmed()), true);
}

QString AnalysisController::statusMessage() const { return m_statusMessage; }

QString AnalysisController::analysisReport() const { return m_analysisReport; }

QVariantList AnalysisController::astFileItems() const { return m_astFileItems; }

QString AnalysisController::selectedAstFilePath() const {
    return m_selectedAstFilePath;
}

QString AnalysisController::astPreviewTitle() const { return m_astPreviewTitle; }

QString AnalysisController::astPreviewSummary() const {
    return m_astPreviewSummary;
}

QString AnalysisController::astPreviewText() const { return m_astPreviewText; }

QVariantMap AnalysisController::capabilityScene() const {
    return SceneMapper::toVariantMap(m_capabilityScene);
}

QVariantMap AnalysisController::componentSceneCatalog() const {
    return m_componentSceneCatalog;
}

QVariantList AnalysisController::capabilityNodeItems() const {
    return m_capabilityScene.nodeItems;
}

QVariantList AnalysisController::capabilityEdgeItems() const {
    return m_capabilityScene.edgeItems;
}

QVariantList AnalysisController::capabilityGroupItems() const {
    return m_capabilityScene.groupItems;
}

double AnalysisController::capabilitySceneWidth() const {
    return m_capabilityScene.sceneWidth;
}

double AnalysisController::capabilitySceneHeight() const {
    return m_capabilityScene.sceneHeight;
}

bool AnalysisController::analyzing() const { return m_analyzing; }

bool AnalysisController::stopRequested() const { return m_stopRequested; }

double AnalysisController::analysisProgress() const { return m_analysisProgress; }

QString AnalysisController::analysisPhase() const { return m_analysisPhase; }

QVariantMap AnalysisController::systemContextData() const {
    return m_systemContextData;
}

QVariantList AnalysisController::systemContextCards() const {
    return m_systemContextCards;
}

bool AnalysisController::aiAvailable() const { return m_aiAvailable; }

QString AnalysisController::aiConfigPath() const { return m_aiConfigPath; }

QString AnalysisController::aiSetupMessage() const { return m_aiSetupMessage; }

bool AnalysisController::aiBusy() const { return m_aiBusy; }

bool AnalysisController::aiHasResult() const { return m_aiHasResult; }

QString AnalysisController::aiNodeName() const { return m_aiNodeName; }

QString AnalysisController::aiSummary() const { return m_aiSummary; }

QString AnalysisController::aiResponsibility() const {
    return m_aiResponsibility;
}

QString AnalysisController::aiUncertainty() const { return m_aiUncertainty; }

QVariantList AnalysisController::aiCollaborators() const {
    return m_aiCollaborators;
}

QVariantList AnalysisController::aiEvidence() const { return m_aiEvidence; }

QVariantList AnalysisController::aiNextActions() const {
    return m_aiNextActions;
}

QString AnalysisController::aiStatusMessage() const { return m_aiStatusMessage; }

QString AnalysisController::aiScope() const { return m_aiScope; }

void AnalysisController::setSelectedAstFilePath(const QString& value) {
    const QString cleanedValue = QDir::fromNativeSeparators(value.trimmed());
    if (m_selectedAstFilePath == cleanedValue) {
        return;
    }
    setSelectedAstFilePathInternal(cleanedValue, true);
    refreshAstPreview();
}

void AnalysisController::analyzeCurrentProject() { analyzeProject(m_projectRootPath); }

void AnalysisController::stopAnalysis() {
    if (!m_analyzing || m_stopRequested) {
        return;
    }
    setStopRequested(true);
    setAnalysisPhase(QStringLiteral("正在停止..."));
    setStatusMessage(QStringLiteral("已发出停止请求，会在当前阶段结束后安全退出。"));
    if (m_analysisWatcher != nullptr) {
        m_analysisWatcher->cancel();
    }
}

void AnalysisController::analyzeProject(const QString& projectRootPath) {
    const QString cleanedPath = QDir::cleanPath(projectRootPath.trimmed());
    if (cleanedPath.isEmpty()) {
        setStatusMessage(QStringLiteral("请先选择一个项目目录。"));
        return;
    }

    const QFileInfo info(cleanedPath);
    if (!info.exists() || !info.isDir()) {
        setProjectRootPathInternal(cleanedPath, true);
        clearVisualizationState();
        setStatusMessage(
            QStringLiteral("项目目录不存在或不可访问：%1")
                .arg(QDir::toNativeSeparators(cleanedPath)));
        setAnalysisReport(
            QStringLiteral("无法开始分析，因为项目目录不存在或不可访问。\n\n路径：%1")
                .arg(QDir::toNativeSeparators(cleanedPath)));
        setAnalysisPhase(QStringLiteral("等待开始"));
        return;
    }

    if (m_analyzing) {
        setStatusMessage(QStringLiteral("当前已经有分析在进行，请先等待完成或点击停止。"));
        return;
    }

    setProjectRootPathInternal(cleanedPath, true);
    beginAnalysis(cleanedPath);
}

void AnalysisController::analyzeProjectUrl(const QUrl& projectRootUrl) {
    if (projectRootUrl.isLocalFile()) {
        analyzeProject(projectRootUrl.toLocalFile());
    } else {
        analyzeProject(projectRootUrl.toString());
    }
}

void AnalysisController::ensureComponentSceneForCapability(const qulonglong capabilityId) {
    if (capabilityId == 0 || m_lastCapabilityGraph.nodes.empty()) {
        return;
    }

    const QString capabilityKey = QString::number(capabilityId);
    if (m_componentSceneCatalog.contains(capabilityKey)) {
        return;
    }

    const auto capabilityIt = std::find_if(
        m_lastCapabilityGraph.nodes.begin(),
        m_lastCapabilityGraph.nodes.end(),
        [capabilityId](const savt::core::CapabilityNode& node) {
            return node.id == static_cast<std::size_t>(capabilityId);
        });
    if (capabilityIt == m_lastCapabilityGraph.nodes.end()) {
        return;
    }

    const QString previousStatusMessage = m_statusMessage;
    setStatusMessage(QStringLiteral("正在生成组件工作台..."));

    savt::layout::LayeredGraphLayout layoutEngine;
    auto componentGraph = savt::core::buildComponentGraphForCapability(
        m_lastReport,
        m_lastOverview,
        m_lastCapabilityGraph,
        static_cast<std::size_t>(capabilityId));
    auto componentLayout = layoutEngine.layoutComponentScene(componentGraph);
    const auto componentScene =
        SceneMapper::buildComponentSceneData(componentGraph, componentLayout);

    QVariantMap updatedCatalog = m_componentSceneCatalog;
    updatedCatalog.insert(capabilityKey, SceneMapper::toVariantMap(componentScene));
    setComponentSceneCatalog(std::move(updatedCatalog));
    setStatusMessage(previousStatusMessage);
}

void AnalysisController::copyCodeContextToClipboard(const qulonglong nodeId) {
    QVariantMap targetNode = findNodeById(m_capabilityScene.nodeItems, nodeId);
    if (targetNode.isEmpty()) {
        targetNode = findNodeById(m_componentSceneCatalog, nodeId);
    }
    if (targetNode.isEmpty()) {
        return;
    }

    const QString name = targetNode.value(QStringLiteral("name")).toString();
    const QString responsibility =
        targetNode.value(QStringLiteral("responsibility")).toString();
    const QString summary = targetNode.value(QStringLiteral("summary")).toString();
    const QString kind = targetNode.value(QStringLiteral("kind")).toString();
    const QStringList exampleFiles =
        toQStringList(targetNode.value(QStringLiteral("exampleFiles")));
    const QStringList topSymbols =
        toQStringList(targetNode.value(QStringLiteral("topSymbols")));
    const QStringList collaborators =
        toQStringList(targetNode.value(QStringLiteral("collaboratorNames")));

    const QString kindLabel = copyContextKindLabel(kind);
    const QString primaryDescription =
        !responsibility.trimmed().isEmpty()
            ? responsibility.trimmed()
            : (!summary.trimmed().isEmpty()
                   ? summary.trimmed()
                   : QStringLiteral("当前对象还缺少稳定的职责说明。"));

    QString prompt = QStringLiteral(
                         "我现在需要修改【%1】模块（架构角色：%2）的代码。以下是由静态分析引擎提取的架构上下文，请先阅读理解，稍后我会给出具体指令。\n\n")
                         .arg(name, kindLabel);

    prompt +=
        QStringLiteral("### 🎯 模块核心职责\n%1\n\n").arg(primaryDescription);

    if (!summary.trimmed().isEmpty() && summary.trimmed() != responsibility.trimmed()) {
        prompt += QStringLiteral("### 🧭 补充说明\n%1\n\n").arg(summary.trimmed());
    }

    if (!exampleFiles.isEmpty()) {
        prompt += QStringLiteral("### 📁 核心关联文件\n");
        for (const QString& file : exampleFiles) {
            prompt += QStringLiteral("- `%1`\n").arg(file);
        }
        prompt += QLatin1Char('\n');
    }
    if (!topSymbols.isEmpty()) {
        prompt += QStringLiteral("### 🧱 核心类与关键符号（AST 提取）\n");
        for (const QString& symbol : topSymbols) {
            prompt += QStringLiteral("- `%1`\n").arg(symbol);
        }
        prompt += QLatin1Char('\n');
    }
    if (!collaborators.isEmpty()) {
        prompt += QStringLiteral("### 🤝 直接协作模块\n");
        for (const QString& collaborator : collaborators) {
            prompt += QStringLiteral("- `%1`\n").arg(collaborator);
        }
        prompt += QLatin1Char('\n');
    }
    prompt += QStringLiteral("---\n*请回复“已了解上下文”后再等待我的具体指令。*");

    if (QClipboard* clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(prompt);
        setStatusMessage(
            QStringLiteral("✅ 已复制「%1」的上下文 Prompt，粘贴进 Cursor / Claude 即可")
                .arg(name));
    }
}

void AnalysisController::copyTextToClipboard(const QString& text) {
    if (text.trimmed().isEmpty()) {
        return;
    }
    if (QClipboard* clipboard = QGuiApplication::clipboard(); clipboard != nullptr) {
        clipboard->setText(text);
    }
}

QVariantMap AnalysisController::describeFileNode(const QVariantMap& nodeData) const {
    return FileInsightService::buildDetail(m_projectRootPath, nodeData);
}

void AnalysisController::refreshProjectConfigRecommendation() {
    setProjectConfigRecommendation(
        ProjectConfigRecommendationService::buildRecommendation(
            m_projectRootPath,
            m_lastReport,
            m_lastOverview,
            m_lastCapabilityGraph));
}

void AnalysisController::selectProjectConfigRecommendationOption(
    const QString& choiceId,
    const QString& optionId) {
    const QVariantMap recommendation =
        m_systemContextData.value(QStringLiteral("projectConfigRecommendation")).toMap();
    if (recommendation.isEmpty()) {
        refreshProjectConfigRecommendation();
        return;
    }

    const QVariantMap currentChoice = recommendationChoiceById(recommendation, choiceId);
    const QVariantMap targetOption = recommendationOptionById(currentChoice, optionId);
    if (currentChoice.isEmpty() || targetOption.isEmpty()) {
        setStatusMessage(QStringLiteral("这个推荐项暂时没切换成功，请再点一次试试。"));
        return;
    }

    const QString currentSelectionId =
        currentChoice.value(QStringLiteral("selectedOptionId")).toString().trimmed();
    const QString choiceTitle =
        currentChoice.value(QStringLiteral("title")).toString().trimmed();
    const QString optionLabel =
        targetOption.value(QStringLiteral("label")).toString().trimmed();
    if (currentSelectionId == optionId.trimmed()) {
        setStatusMessage(
            choiceTitle.isEmpty()
                ? QStringLiteral("当前已经是这个选择了。")
                : QStringLiteral("已保持：%1 -> %2").arg(choiceTitle, optionLabel));
        return;
    }

    setProjectConfigRecommendation(
        ProjectConfigRecommendationService::selectChoice(
            recommendation,
            choiceId,
            optionId));
    setStatusMessage(
        choiceTitle.isEmpty()
            ? QStringLiteral("已更新推荐草案。")
            : QStringLiteral("已更新：%1 -> %2").arg(choiceTitle, optionLabel));
}

void AnalysisController::generateRecommendedProjectConfig() {
    if (m_analyzing) {
        setStatusMessage(QStringLiteral("当前正在分析，请等这一轮结束后再生成配置。"));
        return;
    }

    QVariantMap recommendation =
        m_systemContextData.value(QStringLiteral("projectConfigRecommendation")).toMap();
    if (recommendation.isEmpty()) {
        recommendation =
            ProjectConfigRecommendationService::buildRecommendation(
                m_projectRootPath,
                m_lastReport,
                m_lastOverview,
                m_lastCapabilityGraph);
        setProjectConfigRecommendation(recommendation);
    }

    QString writtenPath;
    QString errorMessage;
    if (!ProjectConfigRecommendationService::writeRecommendation(
            m_projectRootPath,
            recommendation,
            &writtenPath,
            &errorMessage)) {
        setStatusMessage(
            errorMessage.isEmpty()
                ? QStringLiteral("生成配置文件时失败了。")
                : errorMessage);
        return;
    }

    setStatusMessage(
        QStringLiteral("已写入配置文件：%1，正在重新分析项目。")
            .arg(writtenPath));
    analyzeCurrentProject();
}

void AnalysisController::refreshAiAvailability() {
    applyAiAvailability(AiService::inspectAvailability());
    if (!m_aiBusy && !m_aiHasResult) {
        setAiStatusMessage(m_aiSetupMessage);
    }
}

void AnalysisController::clearAiExplanation() {
    cancelActiveAiReply();
    resetAiState(true);
}

void AnalysisController::requestProjectOverviewRefresh(const QString& userTask) {
    requestProjectOverviewRefreshInternal(userTask);
}

void AnalysisController::requestAiExplanation(
    const QVariantMap& nodeData,
    const QString& userTask) {
    if (m_aiBusy) {
        setAiStatusMessage(QStringLiteral("AI 正在生成，请等待当前请求完成。"));
        return;
    }
    const AiPreparedRequest prepared = AiService::prepareNodeRequest(
        AiRequestContext{
            m_projectRootPath,
            m_analysisReport,
            m_statusMessage,
            m_analysisPhase,
            userTask,
            m_selectedAstFilePath,
            m_astPreviewTitle,
            m_astPreviewSummary
        },
        nodeData);
    applyAiAvailability(prepared.availability);
    if (!prepared.ready) {
        setAiStatusMessage(prepared.failureStatusMessage);
        return;
    }

    cancelActiveAiReply();
    resetAiState(false);
    setAiScope(prepared.scope);
    setAiNodeName(prepared.targetName);
    setAiBusy(true);
    setAiStatusMessage(prepared.pendingStatusMessage);
    AiService::logRequest(
        prepared,
        QStringLiteral("%1 (%2)")
            .arg(nodeData.value(QStringLiteral("id")).toULongLong())
            .arg(prepared.targetName));
    m_aiReply = m_aiNetworkManager->post(prepared.networkRequest, prepared.payload);
    connect(
        m_aiReply,
        &QNetworkReply::finished,
        this,
        [this, reply = m_aiReply]() { finishAiReply(reply); });
}

void AnalysisController::requestProjectAiExplanation(const QString& userTask) {
    if (m_aiBusy) {
        setAiStatusMessage(QStringLiteral("AI 正在生成，请等待当前请求完成。"));
        return;
    }
    const AiPreparedRequest prepared = AiService::prepareProjectRequest(
        AiRequestContext{
            m_projectRootPath,
            m_analysisReport,
            m_statusMessage,
            m_analysisPhase,
            userTask,
            m_selectedAstFilePath,
            m_astPreviewTitle,
            m_astPreviewSummary
        },
        m_systemContextData,
        m_systemContextCards,
        m_capabilityScene.nodeItems);
    applyAiAvailability(prepared.availability);
    if (!prepared.ready) {
        setAiStatusMessage(prepared.failureStatusMessage);
        return;
    }

    cancelActiveAiReply();
    resetAiState(false);
    setAiScope(prepared.scope);
    setAiNodeName(prepared.targetName);
    setAiBusy(true);
    setAiStatusMessage(prepared.pendingStatusMessage);
    AiService::logRequest(prepared, prepared.targetName);
    m_aiReply = m_aiNetworkManager->post(prepared.networkRequest, prepared.payload);
    connect(
        m_aiReply,
        &QNetworkReply::finished,
        this,
        [this, reply = m_aiReply]() { finishAiReply(reply); });
}

void AnalysisController::requestProjectOverviewRefreshInternal(const QString& userTask) {
    if (m_aiBusy) {
        updateProjectOverviewState(
            m_systemContextData.value(QStringLiteral("projectOverview")).toString(),
            false,
            m_systemContextData.value(QStringLiteral("projectOverviewSource"))
                .toString()
                .trimmed()
                .isEmpty()
                ? QStringLiteral("heuristic")
                : m_systemContextData.value(QStringLiteral("projectOverviewSource"))
                      .toString()
                      .trimmed(),
            QStringLiteral("AI 正在处理其他请求，请稍后再刷新项目总览。"));
        setAiStatusMessage(QStringLiteral("AI 正在生成，请等待当前请求完成。"));
        return;
    }

    const QString effectiveTask =
        userTask.trimmed().isEmpty() ? defaultProjectOverviewPrompt()
                                     : userTask.trimmed();
    const AiPreparedRequest prepared = AiService::prepareProjectOverviewRequest(
        AiRequestContext{
            m_projectRootPath,
            m_analysisReport,
            m_statusMessage,
            m_analysisPhase,
            effectiveTask,
            m_selectedAstFilePath,
            m_astPreviewTitle,
            m_astPreviewSummary
        },
        m_systemContextData,
        m_systemContextCards,
        m_capabilityScene.nodeItems);
    applyAiAvailability(prepared.availability);
    if (!prepared.ready) {
        updateProjectOverviewState(
            m_systemContextData.value(QStringLiteral("projectOverview")).toString(),
            false,
            m_systemContextData.value(QStringLiteral("projectOverviewSource"))
                .toString()
                .trimmed()
                .isEmpty()
                ? QStringLiteral("heuristic")
                : m_systemContextData.value(QStringLiteral("projectOverviewSource"))
                      .toString()
                      .trimmed(),
            prepared.failureStatusMessage);
        return;
    }

    cancelActiveAiReply();
    resetAiState(false);
    setAiScope(prepared.scope);
    setAiNodeName(prepared.targetName);
    setAiBusy(true);
    setAiStatusMessage(prepared.pendingStatusMessage);
    updateProjectOverviewState(
        m_systemContextData.value(QStringLiteral("projectOverview")).toString(),
        true,
        m_systemContextData.value(QStringLiteral("projectOverviewSource"))
            .toString()
            .trimmed()
            .isEmpty()
            ? QStringLiteral("heuristic")
            : m_systemContextData.value(QStringLiteral("projectOverviewSource"))
                  .toString()
                  .trimmed(),
        prepared.pendingStatusMessage);
    AiService::logRequest(prepared, prepared.targetName);
    m_aiReply = m_aiNetworkManager->post(prepared.networkRequest, prepared.payload);
    connect(
        m_aiReply,
        &QNetworkReply::finished,
        this,
        [this, reply = m_aiReply]() { finishAiReply(reply); });
}

void AnalysisController::requestReportAiExplanation(const QString& userTask) {
    if (m_aiBusy) {
        setAiStatusMessage(QStringLiteral("AI 正在生成，请等待当前请求完成。"));
        return;
    }
    const AiPreparedRequest prepared = AiService::prepareReportRequest(
        AiRequestContext{
            m_projectRootPath,
            m_analysisReport,
            m_statusMessage,
            m_analysisPhase,
            userTask,
            m_selectedAstFilePath,
            m_astPreviewTitle,
            m_astPreviewSummary
        },
        m_systemContextData,
        m_systemContextCards,
        m_capabilityScene.nodeItems);
    applyAiAvailability(prepared.availability);
    if (!prepared.ready) {
        setAiStatusMessage(prepared.failureStatusMessage);
        return;
    }

    cancelActiveAiReply();
    resetAiState(false);
    setAiScope(prepared.scope);
    setAiNodeName(prepared.targetName);
    setAiBusy(true);
    setAiStatusMessage(prepared.pendingStatusMessage);
    AiService::logRequest(prepared, prepared.targetName);
    m_aiReply = m_aiNetworkManager->post(prepared.networkRequest, prepared.payload);
    connect(
        m_aiReply,
        &QNetworkReply::finished,
        this,
        [this, reply = m_aiReply]() { finishAiReply(reply); });
}

void AnalysisController::beginAnalysis(const QString& cleanedPath) {
    clearVisualizationState();
    setAnalyzing(true);
    setStopRequested(false);
    setAnalysisProgress(0.0);
    setAnalysisPhase(QStringLiteral("准备分析..."));
    setStatusMessage(QStringLiteral("准备分析..."));
    setAnalysisReport(
        QStringLiteral("正在分析项目：%1\n\n系统会先整理项目的主要分工，再为程序员准备详细分析报告和 AST 预览。")
            .arg(QDir::toNativeSeparators(cleanedPath)));
    m_pendingAnalysisResult = std::make_shared<PendingAnalysisResult>();
    m_analysisWatcher->setFuture(QtConcurrent::run(
        [cleanedPath, pendingAnalysisResult = m_pendingAnalysisResult](
            QPromise<void>& promise) {
            AnalysisOrchestrator::run(promise, cleanedPath, pendingAnalysisResult);
        }));
}

void AnalysisController::finishAnalysis() {
    const bool canceled =
        m_stopRequested ||
        (m_analysisWatcher != nullptr && m_analysisWatcher->future().isCanceled());
    setAnalyzing(false);

    const auto result = m_pendingAnalysisResult;
    m_pendingAnalysisResult.reset();
    if (!result) {
        setStopRequested(false);
        setAnalysisProgress(0.0);
        setAnalysisPhase(canceled ? QStringLiteral("已停止") : QStringLiteral("分析失败"));
        setStatusMessage(
            canceled ? QStringLiteral("当前分析已停止。")
                     : QStringLiteral("分析任务结束，但没有拿到结果。"));
        clearVisualizationState();
        return;
    }

    setStopRequested(false);
    if (canceled || result->canceled) {
        setAnalysisProgress(0.0);
        setAnalysisPhase(QStringLiteral("已停止"));
    } else {
        setAnalysisProgress(1.0);
        setAnalysisPhase(QStringLiteral("分析完成"));
    }

    setStatusMessage(
        result->statusMessage.isEmpty()
            ? (canceled ? QStringLiteral("当前分析已停止。")
                        : QStringLiteral("分析完成。"))
            : result->statusMessage);
    setAnalysisReport(
        result->analysisReport.isEmpty()
            ? QStringLiteral("没有可显示的分析报告。")
            : result->analysisReport);
    m_lastReport = result->report;
    m_lastOverview = result->overview;
    m_lastCapabilityGraph = result->capabilityGraph;
    setAstFileItems(result->astFileItems);
    setSelectedAstFilePathInternal(result->selectedAstFilePath, true);
    setAstPreviewTitle(result->astPreviewTitle);
    setAstPreviewSummary(result->astPreviewSummary);
    setAstPreviewText(result->astPreviewText);
    setCapabilityScene(result->capabilityScene);
    setComponentSceneCatalog(result->componentSceneCatalog);
    if (m_systemContextReport != result->systemContextReport) {
        m_systemContextReport = result->systemContextReport;
        emit systemContextReportChanged();
    }
    setSystemContextData(result->systemContextData);
    setSystemContextCards(result->systemContextCards);
    if (!canceled && !result->canceled && !m_systemContextData.isEmpty()) {
        requestProjectOverviewRefreshInternal({});
    }
}

void AnalysisController::clearVisualizationState() {
    clearAiExplanation();
    setAstFileItems({});
    setSelectedAstFilePathInternal({}, true);
    const auto preview = AstPreviewService::buildEmptyPreview();
    setAstPreviewTitle(preview.title);
    setAstPreviewSummary(preview.summary);
    setAstPreviewText(preview.text);
    setCapabilityScene({});
    setComponentSceneCatalog({});
    m_lastReport = {};
    m_lastOverview = {};
    m_lastCapabilityGraph = {};
    setSystemContextData({});
    setSystemContextCards({});
    if (!m_systemContextReport.isEmpty()) {
        m_systemContextReport.clear();
        emit systemContextReportChanged();
    }
}

void AnalysisController::setProjectRootPathInternal(QString value, const bool emitSignal) {
    if (value.isEmpty()) {
        value = AnalysisOrchestrator::defaultProjectRootPath();
    }
    if (m_projectRootPath == value) {
        return;
    }
    m_projectRootPath = std::move(value);
    if (emitSignal) {
        emit projectRootPathChanged();
    }
}

void AnalysisController::setSelectedAstFilePathInternal(QString value, const bool emitSignal) {
    value = QDir::fromNativeSeparators(value.trimmed());
    if (m_selectedAstFilePath == value) {
        return;
    }
    m_selectedAstFilePath = std::move(value);
    if (emitSignal) {
        emit selectedAstFilePathChanged();
    }
}

void AnalysisController::setStatusMessage(QString value) {
    if (m_statusMessage == value) {
        return;
    }
    m_statusMessage = std::move(value);
    emit statusMessageChanged();
}

void AnalysisController::setAnalysisReport(QString value) {
    if (m_analysisReport == value) {
        return;
    }
    m_analysisReport = std::move(value);
    emit analysisReportChanged();
}

void AnalysisController::setAstFileItems(QVariantList value) {
    if (m_astFileItems == value) {
        return;
    }
    m_astFileItems = std::move(value);
    emit astFileItemsChanged();
}

void AnalysisController::setAstPreviewTitle(QString value) {
    if (m_astPreviewTitle == value) {
        return;
    }
    m_astPreviewTitle = std::move(value);
    emit astPreviewTitleChanged();
}

void AnalysisController::setAstPreviewSummary(QString value) {
    if (m_astPreviewSummary == value) {
        return;
    }
    m_astPreviewSummary = std::move(value);
    emit astPreviewSummaryChanged();
}

void AnalysisController::setAstPreviewText(QString value) {
    if (m_astPreviewText == value) {
        return;
    }
    m_astPreviewText = std::move(value);
    emit astPreviewTextChanged();
}

void AnalysisController::setCapabilityScene(CapabilitySceneData value) {
    const bool nodeItemsChanged = (m_capabilityScene.nodeItems != value.nodeItems);
    const bool edgeItemsChanged = (m_capabilityScene.edgeItems != value.edgeItems);
    const bool groupItemsChanged = (m_capabilityScene.groupItems != value.groupItems);
    const bool sceneWidthChanged = !qFuzzyCompare(m_capabilityScene.sceneWidth, value.sceneWidth);
    const bool sceneHeightChanged = !qFuzzyCompare(m_capabilityScene.sceneHeight, value.sceneHeight);
    if (!nodeItemsChanged && !edgeItemsChanged && !groupItemsChanged &&
        !sceneWidthChanged && !sceneHeightChanged) {
        return;
    }

    m_capabilityScene = std::move(value);
    emit capabilitySceneChanged();
    if (nodeItemsChanged) {
        emit capabilityNodeItemsChanged();
    }
    if (edgeItemsChanged) {
        emit capabilityEdgeItemsChanged();
    }
    if (groupItemsChanged) {
        emit capabilityGroupItemsChanged();
    }
    if (sceneWidthChanged) {
        emit capabilitySceneWidthChanged();
    }
    if (sceneHeightChanged) {
        emit capabilitySceneHeightChanged();
    }
}

void AnalysisController::setComponentSceneCatalog(QVariantMap value) {
    if (m_componentSceneCatalog == value) {
        return;
    }
    m_componentSceneCatalog = std::move(value);
    emit componentSceneCatalogChanged();
}

void AnalysisController::setAnalyzing(const bool value) {
    if (m_analyzing == value) {
        return;
    }
    m_analyzing = value;
    emit analyzingChanged();
}

void AnalysisController::setStopRequested(const bool value) {
    if (m_stopRequested == value) {
        return;
    }
    m_stopRequested = value;
    emit stopRequestedChanged();
}

void AnalysisController::setAnalysisProgress(const double value) {
    if (qFuzzyCompare(m_analysisProgress, value)) {
        return;
    }
    m_analysisProgress = value;
    emit analysisProgressChanged();
}

void AnalysisController::setAnalysisPhase(QString value) {
    if (m_analysisPhase == value) {
        return;
    }
    m_analysisPhase = std::move(value);
    emit analysisPhaseChanged();
}

void AnalysisController::setSystemContextData(QVariantMap value) {
    if (m_systemContextData == value) {
        return;
    }
    m_systemContextData = std::move(value);
    emit systemContextDataChanged();
}

void AnalysisController::setProjectConfigRecommendation(QVariantMap value) {
    QVariantMap updated = m_systemContextData;
    if (updated.value(QStringLiteral("projectConfigRecommendation")).toMap() == value) {
        return;
    }
    updated.insert(QStringLiteral("projectConfigRecommendation"), std::move(value));
    setSystemContextData(std::move(updated));
}

void AnalysisController::updateProjectOverviewState(
    QString overviewText,
    const bool busy,
    QString source,
    QString statusMessage) {
    if (m_systemContextData.isEmpty()) {
        return;
    }

    overviewText = overviewText.trimmed();
    source = source.trimmed();
    statusMessage = statusMessage.trimmed();

    QVariantMap updated = m_systemContextData;
    updated.insert(QStringLiteral("projectOverview"), overviewText);
    updated.insert(QStringLiteral("projectOverviewBusy"), busy);
    updated.insert(
        QStringLiteral("projectOverviewSource"),
        source.isEmpty() ? QStringLiteral("heuristic") : source);
    updated.insert(QStringLiteral("projectOverviewStatus"), statusMessage);
    setSystemContextData(std::move(updated));
}

QString AnalysisController::formatProjectOverviewText(const AiReplyState& state) const {
    QStringList parts;

    const QString summary =
        stripProjectOverviewBoilerplate(state.summary);
    if (!summary.isEmpty()) {
        parts.push_back(summary);
    }

    QString responsibility = state.responsibility;
    responsibility = stripLeadingSectionLabel(responsibility, QStringLiteral("为什么重要："));
    responsibility = stripLeadingSectionLabel(responsibility, QStringLiteral("具体职责："));
    responsibility = stripProjectOverviewBoilerplate(responsibility);
    if (!responsibility.isEmpty() && !parts.contains(responsibility) && summary.size() < 96) {
        QString clippedResponsibility = responsibility;
        const int remainingBudget = 240 - summary.size();
        if (remainingBudget > 48 && clippedResponsibility.size() > remainingBudget) {
            clippedResponsibility =
                clippedResponsibility.left(remainingBudget - 3).trimmed() +
                QStringLiteral("...");
        }
        if (!clippedResponsibility.isEmpty() &&
            clippedResponsibility != summary &&
            clippedResponsibility.size() <= std::max(remainingBudget, 0)) {
            parts.push_back(clippedResponsibility);
        }
    }

    const QString uncertainty = collapseWhitespace(state.uncertainty);
    if (!uncertainty.isEmpty() &&
        parts.join(QStringLiteral(" ")).size() < 170) {
        parts.push_back(QStringLiteral("当前仍有不确定点：%1").arg(uncertainty));
    }

    return parts.join(QStringLiteral(" "));
}

void AnalysisController::setSystemContextCards(QVariantList value) {
    if (m_systemContextCards == value) {
        return;
    }
    m_systemContextCards = std::move(value);
    emit systemContextCardsChanged();
}

void AnalysisController::refreshAstPreview() {
    const auto preview =
        AstPreviewService::buildPreview(m_projectRootPath, m_selectedAstFilePath);
    setAstPreviewTitle(preview.title);
    setAstPreviewSummary(preview.summary);
    setAstPreviewText(preview.text);
}

void AnalysisController::applyAiAvailability(const AiAvailabilityState& state) {
    setAiConfigPath(state.configPath);
    setAiAvailable(state.available);
    setAiSetupMessage(state.setupMessage);
}

void AnalysisController::applyAiReplyState(const AiReplyState& state) {
    setAiHasResult(state.hasResult);
    setAiStatusMessage(state.statusMessage);
    if (!state.hasResult) {
        return;
    }
    setAiSummary(state.summary);
    setAiResponsibility(state.responsibility);
    setAiUncertainty(state.uncertainty);
    setAiCollaborators(state.collaborators);
    setAiEvidence(state.evidence);
    setAiNextActions(state.nextActions);
}

void AnalysisController::cancelActiveAiReply() {
    if (m_aiReply == nullptr) {
        return;
    }
    m_aiReply->disconnect(this);
    m_aiReply->abort();
    m_aiReply->deleteLater();
    m_aiReply = nullptr;
}

void AnalysisController::setAiAvailable(const bool value) {
    if (m_aiAvailable == value) {
        return;
    }
    m_aiAvailable = value;
    emit aiAvailableChanged();
}

void AnalysisController::setAiConfigPath(QString value) {
    if (m_aiConfigPath == value) {
        return;
    }
    m_aiConfigPath = std::move(value);
    emit aiConfigPathChanged();
}

void AnalysisController::setAiSetupMessage(QString value) {
    if (m_aiSetupMessage == value) {
        return;
    }
    m_aiSetupMessage = std::move(value);
    emit aiSetupMessageChanged();
}

void AnalysisController::setAiBusy(const bool value) {
    if (m_aiBusy == value) {
        return;
    }
    m_aiBusy = value;
    emit aiBusyChanged();
}

void AnalysisController::setAiHasResult(const bool value) {
    if (m_aiHasResult == value) {
        return;
    }
    m_aiHasResult = value;
    emit aiHasResultChanged();
}

void AnalysisController::setAiNodeName(QString value) {
    if (m_aiNodeName == value) {
        return;
    }
    m_aiNodeName = std::move(value);
    emit aiNodeNameChanged();
}

void AnalysisController::setAiSummary(QString value) {
    if (m_aiSummary == value) {
        return;
    }
    m_aiSummary = std::move(value);
    emit aiSummaryChanged();
}

void AnalysisController::setAiResponsibility(QString value) {
    if (m_aiResponsibility == value) {
        return;
    }
    m_aiResponsibility = std::move(value);
    emit aiResponsibilityChanged();
}

void AnalysisController::setAiUncertainty(QString value) {
    if (m_aiUncertainty == value) {
        return;
    }
    m_aiUncertainty = std::move(value);
    emit aiUncertaintyChanged();
}

void AnalysisController::setAiCollaborators(QVariantList value) {
    if (m_aiCollaborators == value) {
        return;
    }
    m_aiCollaborators = std::move(value);
    emit aiCollaboratorsChanged();
}

void AnalysisController::setAiEvidence(QVariantList value) {
    if (m_aiEvidence == value) {
        return;
    }
    m_aiEvidence = std::move(value);
    emit aiEvidenceChanged();
}

void AnalysisController::setAiNextActions(QVariantList value) {
    if (m_aiNextActions == value) {
        return;
    }
    m_aiNextActions = std::move(value);
    emit aiNextActionsChanged();
}

void AnalysisController::setAiStatusMessage(QString value) {
    if (m_aiStatusMessage == value) {
        return;
    }
    m_aiStatusMessage = std::move(value);
    emit aiStatusMessageChanged();
}

void AnalysisController::setAiScope(QString value) {
    if (m_aiScope == value) {
        return;
    }
    m_aiScope = std::move(value);
    emit aiScopeChanged();
}

void AnalysisController::resetAiState(const bool keepSetupMessage) {
    setAiBusy(false);
    setAiHasResult(false);
    setAiNodeName({});
    setAiSummary({});
    setAiResponsibility({});
    setAiUncertainty({});
    setAiCollaborators({});
    setAiEvidence({});
    setAiNextActions({});
    setAiScope({});
    setAiStatusMessage(keepSetupMessage ? m_aiSetupMessage : QString());
}

void AnalysisController::finishAiReply(QNetworkReply* reply) {
    if (reply == nullptr) {
        return;
    }

    const bool isActiveReply = (reply == m_aiReply);
    if (isActiveReply) {
        m_aiReply = nullptr;
    }

    const QByteArray responseBytes = reply->readAll();
    const auto cleanup = [reply]() { reply->deleteLater(); };
    if (!isActiveReply) {
        cleanup();
        return;
    }

    setAiBusy(false);
    const QString completedScope = m_aiScope;
    AiService::logResponse(responseBytes, completedScope);
    const AiReplyState state = AiService::parseReply(
        responseBytes,
        completedScope,
        reply->error() != QNetworkReply::NoError,
        reply->errorString());
    applyAiReplyState(state);
    if (completedScope == QStringLiteral("project_overview")) {
        const QString currentSource =
            m_systemContextData.value(QStringLiteral("projectOverviewSource"))
                .toString()
                .trimmed();
        updateProjectOverviewState(
            state.hasResult
                ? formatProjectOverviewText(state)
                : m_systemContextData.value(QStringLiteral("projectOverview")).toString(),
            false,
            state.hasResult
                ? QStringLiteral("ai")
                : (currentSource.isEmpty() ? QStringLiteral("heuristic") : currentSource),
            state.statusMessage);
    }
    cleanup();
}

}  // namespace savt::ui
