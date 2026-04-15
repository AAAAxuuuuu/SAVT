import QtQuick
import QtQuick.Controls
import "greenfield"

ApplicationWindow {
    id: window

    QtObject {
        id: fallbackController

        property string projectRootPath: ""
        property string statusMessage: ""
        property string analysisReport: ""
        property string systemContextReport: ""
        property var astFileItems: []
        property string selectedAstFilePath: ""
        property string astPreviewTitle: ""
        property string astPreviewSummary: ""
        property string astPreviewText: ""
        property var capabilityScene: ({ "nodes": [], "edges": [], "groups": [], "summary": "" })
        property var componentSceneCatalog: ({})
        property var capabilityNodeItems: []
        property var capabilityEdgeItems: []
        property var capabilityGroupItems: []
        property real capabilitySceneWidth: 0
        property real capabilitySceneHeight: 0
        property bool analyzing: false
        property bool stopRequested: false
        property real analysisProgress: 0
        property string analysisPhase: ""
        property var systemContextData: ({
            "headline": "",
            "projectName": "",
            "projectOverview": "",
            "projectOverviewBusy": false,
            "projectOverviewSource": "heuristic",
            "projectOverviewStatus": "",
            "purposeSummary": "",
            "projectKindSummary": "",
            "entrySummary": "",
            "inputSummary": "",
            "outputSummary": "",
            "technologySummary": "",
            "containerSummary": "",
            "containerNames": [],
            "topModules": [],
            "mainFlowSummary": "",
            "contextSections": [],
            "ruleFindings": [],
            "readingOrder": [],
            "riskSignals": [],
            "hotspotSignals": [],
            "precisionSummary": "",
            "semanticReadiness": ({
                "modeKey": "",
                "modeLabel": "",
                "badgeTone": "info",
                "headline": "",
                "summary": "",
                "reason": "",
                "impact": "",
                "action": "",
                "confidenceSummary": "",
                "statusCode": "",
                "compilationDatabasePath": "",
                "semanticRequested": false,
                "semanticEnabled": false,
                "needsAttention": false
            })
        })
        property var systemContextCards: []
        property bool aiAvailable: false
        property string aiConfigPath: ""
        property string aiSetupMessage: ""
        property bool aiBusy: false
        property bool aiHasResult: false
        property string aiNodeName: ""
        property string aiSummary: ""
        property string aiResponsibility: ""
        property string aiUncertainty: ""
        property var aiCollaborators: []
        property var aiEvidence: []
        property var aiNextActions: []
        property string aiStatusMessage: ""
        property string aiScope: ""

        function analyzeCurrentProject() {}
        function stopAnalysis() {}
        function analyzeProject(projectRootPath) {}
        function analyzeProjectUrl(projectRootUrl) {}
        function ensureComponentSceneForCapability(capabilityId) {}
        function refreshAiAvailability() {}
        function clearAiExplanation() {}
        function requestAiExplanation(nodeData, userTask) {}
        function requestProjectAiExplanation(userTask) {}
        function requestReportAiExplanation(userTask) {}
        function copyCodeContextToClipboard(nodeId) {}
        function copyTextToClipboard(text) {}
        function describeFileNode(nodeData) { return ({ "available": false, "singleFile": false, "importClues": [], "declarationClues": [], "behaviorSignals": [], "readingHints": [], "previewText": "" }) }
    }

    readonly property var controller: _analysisController ? _analysisController : fallbackController

    width: 1600
    height: 940
    minimumWidth: 1280
    minimumHeight: 780
    visible: true
    title: "SAVT"
    color: "#F6F7F9"

    MainSurface {
        anchors.fill: parent
        analysisController: window.controller
    }
}
