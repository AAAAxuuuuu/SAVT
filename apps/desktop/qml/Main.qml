import QtQuick
import QtQuick.Controls
import "theme"
import "state"
import "shell"

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
            "purposeSummary": "",
            "entrySummary": "",
            "inputSummary": "",
            "outputSummary": "",
            "technologySummary": "",
            "containerSummary": "",
            "containerNames": []
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
        function refreshAiAvailability() {}
        function clearAiExplanation() {}
        function requestAiExplanation(nodeData, userTask) {}
        function requestProjectAiExplanation(userTask) {}
        function copyCodeContextToClipboard(nodeId) {}
        function copyTextToClipboard(text) {}
    }

    readonly property var controller: _analysisController ? _analysisController : fallbackController

    width: 1560
    height: 940
    minimumWidth: 1320
    minimumHeight: 820
    visible: true
    title: "SAVT"
    color: appTheme.windowBase

    DesignTokens {
        id: designTokens
    }

    AppTheme {
        id: appTheme
        tokens: designTokens
    }

    DesktopUiState {
        id: uiState
        capabilityScene: window.controller ? window.controller.capabilityScene : ({})
        componentSceneCatalog: window.controller ? window.controller.componentSceneCatalog : ({})
    }

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: appTheme.windowTop }
            GradientStop { position: 1.0; color: appTheme.windowBase }
        }

        Rectangle {
            width: 520
            height: 520
            radius: 260
            x: -160
            y: -220
            color: "#DCE7F0"
            opacity: 0.86
        }

        Rectangle {
            width: 420
            height: 420
            radius: 210
            x: parent.width - width * 0.7
            y: -90
            color: "#EDF3F8"
            opacity: 0.8
        }

        Rectangle {
            width: 560
            height: 560
            radius: 280
            x: parent.width - width * 0.5
            y: parent.height - height * 0.56
            color: "#D8E3ED"
            opacity: 0.42
        }
    }

    AppShell {
        anchors.fill: parent
        theme: appTheme
        analysisController: window.controller
        uiState: uiState
    }
}
