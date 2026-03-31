#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include "savt/ui/SceneMapper.h"

#include <memory>

class QNetworkAccessManager;
class QNetworkReply;

namespace savt::ui {

struct PendingAnalysisResult;
struct AiAvailabilityState;
struct AiReplyState;

class AnalysisController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString projectRootPath READ projectRootPath WRITE setProjectRootPath NOTIFY projectRootPathChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString analysisReport READ analysisReport NOTIFY analysisReportChanged)
    Q_PROPERTY(QString systemContextReport READ systemContextReport NOTIFY systemContextReportChanged)
    Q_PROPERTY(QVariantList astFileItems READ astFileItems NOTIFY astFileItemsChanged)
    Q_PROPERTY(QString selectedAstFilePath READ selectedAstFilePath WRITE setSelectedAstFilePath NOTIFY selectedAstFilePathChanged)
    Q_PROPERTY(QString astPreviewTitle READ astPreviewTitle NOTIFY astPreviewTitleChanged)
    Q_PROPERTY(QString astPreviewSummary READ astPreviewSummary NOTIFY astPreviewSummaryChanged)
    Q_PROPERTY(QString astPreviewText READ astPreviewText NOTIFY astPreviewTextChanged)
    Q_PROPERTY(QVariantMap capabilityScene READ capabilityScene NOTIFY capabilitySceneChanged)
    Q_PROPERTY(QVariantList capabilityNodeItems READ capabilityNodeItems NOTIFY capabilityNodeItemsChanged)
    Q_PROPERTY(QVariantList capabilityEdgeItems READ capabilityEdgeItems NOTIFY capabilityEdgeItemsChanged)
    Q_PROPERTY(QVariantList capabilityGroupItems READ capabilityGroupItems NOTIFY capabilityGroupItemsChanged)
    Q_PROPERTY(double capabilitySceneWidth READ capabilitySceneWidth NOTIFY capabilitySceneWidthChanged)
    Q_PROPERTY(double capabilitySceneHeight READ capabilitySceneHeight NOTIFY capabilitySceneHeightChanged)
    Q_PROPERTY(bool analyzing READ analyzing NOTIFY analyzingChanged)
    Q_PROPERTY(bool stopRequested READ stopRequested NOTIFY stopRequestedChanged)
    Q_PROPERTY(double analysisProgress READ analysisProgress NOTIFY analysisProgressChanged)
    Q_PROPERTY(QString analysisPhase READ analysisPhase NOTIFY analysisPhaseChanged)
    Q_PROPERTY(QVariantMap systemContextData READ systemContextData NOTIFY systemContextDataChanged)
    Q_PROPERTY(QVariantList systemContextCards READ systemContextCards NOTIFY systemContextCardsChanged)
    Q_PROPERTY(bool aiAvailable READ aiAvailable NOTIFY aiAvailableChanged)
    Q_PROPERTY(QString aiConfigPath READ aiConfigPath NOTIFY aiConfigPathChanged)
    Q_PROPERTY(QString aiSetupMessage READ aiSetupMessage NOTIFY aiSetupMessageChanged)
    Q_PROPERTY(bool aiBusy READ aiBusy NOTIFY aiBusyChanged)
    Q_PROPERTY(bool aiHasResult READ aiHasResult NOTIFY aiHasResultChanged)
    Q_PROPERTY(QString aiNodeName READ aiNodeName NOTIFY aiNodeNameChanged)
    Q_PROPERTY(QString aiSummary READ aiSummary NOTIFY aiSummaryChanged)
    Q_PROPERTY(QString aiResponsibility READ aiResponsibility NOTIFY aiResponsibilityChanged)
    Q_PROPERTY(QString aiUncertainty READ aiUncertainty NOTIFY aiUncertaintyChanged)
    Q_PROPERTY(QVariantList aiCollaborators READ aiCollaborators NOTIFY aiCollaboratorsChanged)
    Q_PROPERTY(QVariantList aiEvidence READ aiEvidence NOTIFY aiEvidenceChanged)
    Q_PROPERTY(QVariantList aiNextActions READ aiNextActions NOTIFY aiNextActionsChanged)
    Q_PROPERTY(QString aiStatusMessage READ aiStatusMessage NOTIFY aiStatusMessageChanged)
    Q_PROPERTY(QString aiScope READ aiScope NOTIFY aiScopeChanged)

public:
    explicit AnalysisController(QObject* parent = nullptr);

    QString projectRootPath() const;
    void setProjectRootPath(const QString& value);

    QString statusMessage() const;
    QString analysisReport() const;
    QString systemContextReport() const { return m_systemContextReport; }
    QVariantList astFileItems() const;
    QString selectedAstFilePath() const;
    void setSelectedAstFilePath(const QString& value);
    QString astPreviewTitle() const;
    QString astPreviewSummary() const;
    QString astPreviewText() const;
    QVariantMap capabilityScene() const;
    QVariantList capabilityNodeItems() const;
    QVariantList capabilityEdgeItems() const;
    QVariantList capabilityGroupItems() const;
    double capabilitySceneWidth() const;
    double capabilitySceneHeight() const;
    bool analyzing() const;
    bool stopRequested() const;
    double analysisProgress() const;
    QString analysisPhase() const;
    QVariantMap systemContextData() const;
    QVariantList systemContextCards() const;
    bool aiAvailable() const;
    QString aiConfigPath() const;
    QString aiSetupMessage() const;
    bool aiBusy() const;
    bool aiHasResult() const;
    QString aiNodeName() const;
    QString aiSummary() const;
    QString aiResponsibility() const;
    QString aiUncertainty() const;
    QVariantList aiCollaborators() const;
    QVariantList aiEvidence() const;
    QVariantList aiNextActions() const;
    QString aiStatusMessage() const;
    QString aiScope() const;

    Q_INVOKABLE void analyzeCurrentProject();
    Q_INVOKABLE void stopAnalysis();
    Q_INVOKABLE void analyzeProject(const QString& projectRootPath);
    Q_INVOKABLE void analyzeProjectUrl(const QUrl& projectRootUrl);
    Q_INVOKABLE void refreshAiAvailability();
    Q_INVOKABLE void clearAiExplanation();
    Q_INVOKABLE void requestAiExplanation(const QVariantMap& nodeData, const QString& userTask = QString());
    Q_INVOKABLE void requestProjectAiExplanation(const QString& userTask = QString());
    Q_INVOKABLE void copyCodeContextToClipboard(qulonglong nodeId);
    Q_INVOKABLE void copyTextToClipboard(const QString& text);

signals:
    void projectRootPathChanged();
    void statusMessageChanged();
    void analysisReportChanged();
    void systemContextReportChanged();
    void astFileItemsChanged();
    void selectedAstFilePathChanged();
    void astPreviewTitleChanged();
    void astPreviewSummaryChanged();
    void astPreviewTextChanged();
    void capabilitySceneChanged();
    void capabilityNodeItemsChanged();
    void capabilityEdgeItemsChanged();
    void capabilityGroupItemsChanged();
    void capabilitySceneWidthChanged();
    void capabilitySceneHeightChanged();
    void analyzingChanged();
    void stopRequestedChanged();
    void analysisProgressChanged();
    void analysisPhaseChanged();
    void systemContextDataChanged();
    void systemContextCardsChanged();
    void aiAvailableChanged();
    void aiConfigPathChanged();
    void aiSetupMessageChanged();
    void aiBusyChanged();
    void aiHasResultChanged();
    void aiNodeNameChanged();
    void aiSummaryChanged();
    void aiResponsibilityChanged();
    void aiUncertaintyChanged();
    void aiCollaboratorsChanged();
    void aiEvidenceChanged();
    void aiNextActionsChanged();
    void aiStatusMessageChanged();
    void aiScopeChanged();

private:
    void beginAnalysis(const QString& cleanedPath);
    void finishAnalysis();
    void clearVisualizationState();
    void setProjectRootPathInternal(QString value, bool emitSignal);
    void setSelectedAstFilePathInternal(QString value, bool emitSignal);
    void setStatusMessage(QString value);
    void setAnalysisReport(QString value);
    void setAstFileItems(QVariantList value);
    void setAstPreviewTitle(QString value);
    void setAstPreviewSummary(QString value);
    void setAstPreviewText(QString value);
    void setCapabilityScene(CapabilitySceneData value);
    void setAnalyzing(bool value);
    void setStopRequested(bool value);
    void setAnalysisProgress(double value);
    void setAnalysisPhase(QString value);
    void setSystemContextData(QVariantMap value);
    void setSystemContextCards(QVariantList value);
    void refreshAstPreview();
    void applyAiAvailability(const AiAvailabilityState& state);
    void applyAiReplyState(const AiReplyState& state);
    void cancelActiveAiReply();
    void setAiAvailable(bool value);
    void setAiConfigPath(QString value);
    void setAiSetupMessage(QString value);
    void setAiBusy(bool value);
    void setAiHasResult(bool value);
    void setAiNodeName(QString value);
    void setAiSummary(QString value);
    void setAiResponsibility(QString value);
    void setAiUncertainty(QString value);
    void setAiCollaborators(QVariantList value);
    void setAiEvidence(QVariantList value);
    void setAiNextActions(QVariantList value);
    void setAiStatusMessage(QString value);
    void setAiScope(QString value);
    void resetAiState(bool keepSetupMessage = true);
    void finishAiReply(QNetworkReply* reply);

    QString m_projectRootPath;
    QString m_statusMessage;
    QString m_analysisReport;
    QString m_systemContextReport;
    QVariantList m_astFileItems;
    QString m_selectedAstFilePath;
    QString m_astPreviewTitle;
    QString m_astPreviewSummary;
    QString m_astPreviewText;
    CapabilitySceneData m_capabilityScene;
    bool m_analyzing = false;
    bool m_stopRequested = false;
    double m_analysisProgress = 0.0;
    QString m_analysisPhase;
    QVariantMap m_systemContextData;
    QVariantList m_systemContextCards;
    bool m_aiAvailable = false;
    QString m_aiConfigPath;
    QString m_aiSetupMessage;
    bool m_aiBusy = false;
    bool m_aiHasResult = false;
    QString m_aiNodeName;
    QString m_aiSummary;
    QString m_aiResponsibility;
    QString m_aiUncertainty;
    QVariantList m_aiCollaborators;
    QVariantList m_aiEvidence;
    QVariantList m_aiNextActions;
    QString m_aiStatusMessage;
    QString m_aiScope;
    QNetworkAccessManager* m_aiNetworkManager = nullptr;
    QNetworkReply* m_aiReply = nullptr;
    QFutureWatcher<void>* m_analysisWatcher = nullptr;
    std::shared_ptr<PendingAnalysisResult> m_pendingAnalysisResult;
};

}  // namespace savt::ui
