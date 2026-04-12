import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject uiState

    readonly property bool controllerReady: !!root.analysisController
    readonly property var controllerData: root.controllerReady && root.analysisController.systemContextData
                                          ? root.analysisController.systemContextData
                                          : ({})
    readonly property bool hasProject: root.controllerReady
                                       && (root.analysisController.projectRootPath || "").length > 0
    readonly property bool isBusy: root.controllerReady && root.analysisController.analyzing
    readonly property bool hasAnalysisData: ((root.uiState.selection.capabilityNodes || []).length > 0)
                                            || (root.controllerReady
                                                && (((root.analysisController.systemContextReport || "").length > 0)
                                                    || ((root.analysisController.analysisReport || "").length > 0)
                                                    || ((root.analysisController.systemContextCards || []).length > 0)
                                                    || ((root.controllerData.headline || "").length > 0)))

    signal searchRequested(string text)
    signal compareRequested()

    implicitHeight: 58

    function openProjectPicker() {
        projectFolderDialog.open()
    }

    function analyzeSelectedFolder(folderUrl) {
        if (!root.controllerReady)
            return
        root.analysisController.analyzeProjectUrl(folderUrl)
    }

    function analyzeCurrentProject() {
        if (!root.controllerReady)
            return
        root.analysisController.analyzeCurrentProject()
    }

    function stopCurrentAnalysis() {
        if (!root.controllerReady)
            return
        root.analysisController.stopAnalysis()
    }

    function statusText() {
        if (!root.controllerReady)
            return "分析控制器初始化中..."
        if (root.isBusy)
            return (root.analysisController.analysisPhase || "分析中") + " · "
                   + Math.round((root.analysisController.analysisProgress || 0) * 100) + "%"
        if (!root.hasProject)
            return "点击“选择项目并开始分析”，选完项目后会自动启动分析。"
        if (!root.hasAnalysisData)
            return "项目已连接，等待生成第一批结构摘要。"
        return "项目已连接，工作区已就绪。"
    }

    function statusTone() {
        if (!root.controllerReady)
            return "neutral"
        if (root.isBusy)
            return "info"
        if (root.hasProject)
            return "success"
        return "neutral"
    }

    FolderDialog {
        id: projectFolderDialog
        title: "选择项目根目录"
        onAccepted: root.analyzeSelectedFolder(selectedFolder)
    }

    AppCard {
        anchors.fill: parent
        fillColor: "#FFFFFF"
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXxl
        contentPadding: 12

        RowLayout {
            anchors.fill: parent
            spacing: 10

            SearchField {
                id: searchField
                Layout.fillWidth: true
                theme: root.theme
                enabled: root.controllerReady && root.hasAnalysisData
                placeholderText: root.uiState.selection.componentViewActive
                                 ? "定位组件、能力域或关键符号"
                                 : "定位能力域、模块或关键符号"
                onSubmitted: root.searchRequested(text)
            }

            StatusBadge {
                theme: root.theme
                text: root.statusText()
                tone: root.statusTone()
            }

            AppButton {
                theme: root.theme
                compact: true
                tone: "ghost"
                text: "高级对比"
                enabled: root.controllerReady
                onClicked: root.compareRequested()
            }

            AppButton {
                theme: root.theme
                compact: true
                visible: !root.hasProject
                tone: "accent"
                text: "选择项目并开始分析"
                enabled: root.controllerReady && !root.isBusy
                onClicked: root.openProjectPicker()
            }

            AppButton {
                theme: root.theme
                compact: true
                visible: root.controllerReady && root.hasProject && !root.isBusy
                tone: "ghost"
                text: "切换项目"
                onClicked: root.openProjectPicker()
            }

            AppButton {
                theme: root.theme
                compact: true
                tone: "accent"
                visible: root.controllerReady && root.hasProject && !root.isBusy
                text: root.hasAnalysisData ? "重新分析" : "开始分析"
                enabled: root.controllerReady
                onClicked: root.analyzeCurrentProject()
            }

            AppButton {
                theme: root.theme
                compact: true
                tone: "danger"
                visible: root.controllerReady && root.isBusy
                text: root.controllerReady && root.analysisController.stopRequested ? "停止中..." : "停止"
                enabled: root.isBusy && root.controllerReady && !root.analysisController.stopRequested
                onClicked: root.stopCurrentAnalysis()
            }

            AppButton {
                theme: root.theme
                compact: true
                tone: "ai"
                text: root.uiState.askPanelOpen ? "收起 AI 导览" : "AI 导览"
                enabled: root.controllerReady
                onClicked: root.uiState.askPanelOpen = !root.uiState.askPanelOpen
            }
        }
    }
}
