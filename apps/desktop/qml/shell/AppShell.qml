import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../pages"
import "../workspace"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject uiState

    anchors.fill: parent

    readonly property string currentPageId: root.uiState.navigation.pageId

    function openAskPanel() {
        root.uiState.askPanelOpen = true
        root.uiState.contextTrayExpanded = true
        root.uiState.contextTab = "ai"
    }

    function closeAskPanel() {
        root.uiState.askPanelOpen = false
    }

    function navigateTo(pageId) {
        root.uiState.navigation.navigateTo(pageId)
    }

    function openLevel(levelId) {
        root.uiState.navigation.openLevel(levelId)
    }

    function syncLayoutForPage() {
        var pageId = root.currentPageId
        var graphWorkspace = pageId === "project.capabilityMap"
                             || pageId === "project.componentWorkbench"
                             || pageId === "levels.l2"
                             || pageId === "levels.l3"
        var reportWorkspace = pageId.indexOf("report.") === 0 || pageId === "levels.l4"
        var quietWorkspace = pageId === "project.overview"
                             || pageId === "project.compare"
                             || pageId === "levels.l1"

        root.uiState.inspectorCollapsed = quietWorkspace || graphWorkspace
        if (quietWorkspace)
            root.uiState.inspectorDetailsExpanded = false
        if (!graphWorkspace && !reportWorkspace) {
            root.uiState.contextTrayExpanded = false
            root.uiState.contextTab = "context"
        }
        if (graphWorkspace && root.uiState.contextTab === "ai" && !root.uiState.askPanelOpen)
            root.uiState.contextTab = "context"
    }

    function handleSearch(queryText) {
        var query = (queryText || "").trim().toLowerCase()
        if (root.uiState.selection.componentViewActive) {
            root.uiState.selection.componentFilterText = queryText || ""
            return
        }

        if (query.length === 0)
            return

        var nodes = root.uiState.selection.capabilityNodes || []
        for (var index = 0; index < nodes.length; ++index) {
            var node = nodes[index]
            var haystack = ((node.name || "") + " " + (node.role || "") + " " +
                            ((node.moduleNames || []).join(" ")) + " " +
                            ((node.topSymbols || []).join(" "))).toLowerCase()
            if (haystack.indexOf(query) >= 0) {
                root.uiState.selection.selectCapabilityNode(node)
                root.navigateTo("project.capabilityMap")
                return
            }
        }

        root.navigateTo("project.capabilityMap")
    }

    function sideNavSections() {
        return [
            {
                "title": "项目",
                "items": [
                    {"id": "project.overview", "label": "总览", "hint": "前门认知入口和推荐阅读路径"},
                    {"id": "project.capabilityMap", "label": "能力地图", "hint": "用能力图理解项目结构"},
                    {"id": "project.componentWorkbench", "label": "组件工作台", "hint": root.uiState.selection.selectedCapabilityNode ? ("继续展开 " + root.uiState.selection.selectedCapabilityNode.name) : "从能力域继续下钻到组件级"},
                    {"id": "project.compare", "label": "对比", "hint": "承接快照差异和归属比较"}
                ]
            },
            {
                "title": "报告",
                "items": [
                    {"id": "report.engineering", "label": "工程分析报告", "hint": "汇总摘要、证据与原始 Markdown"},
                    {"id": "report.recommendations", "label": "改造建议", "hint": "围绕当前结果组织下一步动作"},
                    {"id": "report.promptPack", "label": "AI 提示词包", "hint": "把分析结果转成可复制提示词"}
                ]
            },
            {
                "title": "视图层级",
                "items": [
                    {"id": "levels.l1", "label": "L1 系统上下文", "hint": "专业直达的系统上下文视角"},
                    {"id": "levels.l2", "label": "L2 能力视图", "hint": "业务能力地图和能力关系"},
                    {"id": "levels.l3", "label": "L3 组件视图", "hint": "能力域下钻后的组件工作台"},
                    {"id": "levels.l4", "label": "L4 全景报告", "hint": "完整工程分析报告"}
                ]
            }
        ]
    }

    function currentEyebrow() {
        if (root.currentPageId.indexOf("project.") === 0)
            return "项目"
        if (root.currentPageId.indexOf("report.") === 0)
            return "报告"
        return "专业入口"
    }

    function currentTitle() {
        if (root.currentPageId === "project.overview")
            return "项目总览"
        if (root.currentPageId === "project.capabilityMap")
            return "能力地图"
        if (root.currentPageId === "project.componentWorkbench")
            return root.uiState.selection.selectedCapabilityNode
                   ? ("组件工作台 · " + root.uiState.selection.selectedCapabilityNode.name)
                   : "组件工作台"
        if (root.currentPageId === "project.compare")
            return "对照工作台"
        if (root.currentPageId === "report.engineering")
            return "工程分析报告"
        if (root.currentPageId === "report.recommendations")
            return "改造建议"
        if (root.currentPageId === "report.promptPack")
            return "AI 提示词包"
        if (root.currentPageId === "levels.l1")
            return "L1 系统上下文"
        if (root.currentPageId === "levels.l2")
            return "L2 能力视图"
        if (root.currentPageId === "levels.l3")
            return "L3 组件视图"
        if (root.currentPageId === "levels.l4")
            return "L4 全景报告"
        return "工程分析报告"
    }

    function currentSummary() {
        if (root.currentPageId === "project.overview")
            return root.analysisController.systemContextData.headline || "先回答项目是做什么的，再决定下一步从哪里继续。"
        if (root.currentPageId === "project.capabilityMap")
            return root.uiState.selection.capabilityScene.summary || "以能力地图作为主工作对象，理解项目由哪些能力块构成。"
        if (root.currentPageId === "project.componentWorkbench")
            return root.uiState.selection.selectedCapabilityNode
                   ? (root.uiState.selection.selectedComponentSceneSummary() || "保留父级能力域上下文，继续展开组件、阶段和依赖路径。")
                   : "先在能力地图里选中一个能力域，再进入这里查看其内部组件图。"
        if (root.currentPageId === "project.compare")
            return "把项目快照、当前焦点对象和关键证据放在同一工作区中对照，减少来回切页。"
        if (root.currentPageId === "report.engineering")
            return "把摘要、证据、动作和原始 Markdown 组织成可继续执行的工程工作流。"
        if (root.currentPageId === "report.recommendations")
            return root.analysisController.aiSummary.length > 0
                   ? root.analysisController.aiSummary
                   : "围绕当前分析结果组织改造建议、下一步动作和回跳入口。"
        if (root.currentPageId === "report.promptPack")
            return "把分析结果转成可复制的 AI 提示词，减少上下文整理成本。"
        if (root.currentPageId === "levels.l1")
            return "保留专业直达入口，从系统上下文建立整体认知。"
        if (root.currentPageId === "levels.l2")
            return "从业务能力关系理解项目整体结构。"
        if (root.currentPageId === "levels.l3")
            return "继续从能力域下钻到组件视角，形成具体改造切口。"
        if (root.currentPageId === "levels.l4")
            return "保留完整报告，同时把后续动作接回主工作流。"
        return "把摘要、证据、动作和提示词承接到报告工作流中。"
    }

    function currentBreadcrumbs() {
        if (root.currentPageId === "project.overview")
            return [{"label": "项目", "pageId": "project.overview"}, {"label": "总览"}]
        if (root.currentPageId === "project.capabilityMap")
            return [{"label": "项目", "pageId": "project.overview"}, {"label": "能力地图"}]
        if (root.currentPageId === "project.componentWorkbench")
            return [
                {"label": "项目", "pageId": "project.overview"},
                {"label": "能力地图", "pageId": "project.capabilityMap"},
                {"label": root.uiState.selection.selectedCapabilityNode ? root.uiState.selection.selectedCapabilityNode.name : "组件工作台"}
            ]
        if (root.currentPageId === "project.compare")
            return [{"label": "项目", "pageId": "project.overview"}, {"label": "对比"}]
        if (root.currentPageId.indexOf("report.") === 0)
            return [
                {"label": "报告", "pageId": "report.engineering"},
                {"label": root.currentTitle()}
            ]
        return [
            {"label": "视图层级", "pageId": "levels.l1"},
            {"label": root.currentTitle()}
        ]
    }

    function pageComponentFor(pageId) {
        if (pageId === "project.overview")
            return overviewPageComponent
        if (pageId === "project.capabilityMap")
            return capabilityMapPageComponent
        if (pageId === "project.componentWorkbench")
            return componentWorkbenchPageComponent
        if (pageId === "project.compare")
            return comparePageComponent
        if (pageId === "report.recommendations")
            return recommendationsPageComponent
        if (pageId === "report.promptPack")
            return promptPackPageComponent
        if (pageId === "levels.l1")
            return levelL1PageComponent
        if (pageId === "levels.l2")
            return levelL2PageComponent
        if (pageId === "levels.l3")
            return levelL3PageComponent
        if (pageId === "levels.l4")
            return levelL4PageComponent
        return engineeringReportPageComponent
    }

    Shortcut {
        sequence: "Ctrl+1"
        onActivated: root.navigateTo("project.overview")
    }

    Shortcut {
        sequence: "Ctrl+2"
        onActivated: root.navigateTo("project.capabilityMap")
    }

    Shortcut {
        sequence: "Ctrl+3"
        onActivated: root.navigateTo("project.componentWorkbench")
    }

    Shortcut {
        sequence: "Ctrl+4"
        onActivated: root.navigateTo("report.engineering")
    }

    Shortcut {
        sequence: "Ctrl+Shift+A"
        onActivated: {
            if (root.uiState.askPanelOpen)
                root.closeAskPanel()
            else
                root.openAskPanel()
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (root.uiState.askPanelOpen) {
                root.closeAskPanel()
                return
            }
            root.uiState.selection.clearSelection()
        }
    }

    Component.onCompleted: {
        root.analysisController.refreshAiAvailability()
        root.syncLayoutForPage()
    }

    onCurrentPageIdChanged: root.syncLayoutForPage()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        AppTopBar {
            id: topBar
            Layout.fillWidth: true
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            onSearchRequested: root.handleSearch(text)
            onCompareRequested: root.navigateTo("project.compare")
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            SideNavTree {
                Layout.preferredWidth: 248
                Layout.minimumWidth: 232
                Layout.maximumWidth: 264
                Layout.fillHeight: true
                theme: root.theme
                analysisController: root.analysisController
                sections: root.sideNavSections()
                currentPageId: root.currentPageId
                onPageRequested: root.navigateTo(pageId)
            }

            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Horizontal

                Item {
                    id: centralWorkspace
                    SplitView.fillWidth: true
                    SplitView.fillHeight: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 16

                        WorkspaceHeader {
                            Layout.fillWidth: true
                            theme: root.theme
                            analysisController: root.analysisController
                            uiState: root.uiState
                            compactMode: root.currentPageId === "project.capabilityMap"
                                         || root.currentPageId === "project.componentWorkbench"
                                         || root.currentPageId === "levels.l2"
                                         || root.currentPageId === "levels.l3"
                            eyebrow: root.currentEyebrow()
                            title: root.currentTitle()
                            summary: root.currentSummary()
                            breadcrumbs: root.currentBreadcrumbs()
                            onNavigateTo: root.navigateTo(pageId)
                        }

                        Loader {
                            id: pageLoader
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            active: true
                            asynchronous: true
                            sourceComponent: root.pageComponentFor(root.currentPageId)
                        }

                        BottomContextTray {
                            id: bottomTray
                            Layout.fillWidth: true
                            theme: root.theme
                            uiState: root.uiState
                            analysisController: root.analysisController
                        }
                    }

                    AskSavtPanel {
                        id: askSavtPanel
                        visible: root.uiState.askPanelOpen
                        z: 20
                        width: Math.min(560, Math.max(400, centralWorkspace.width * 0.42))
                        height: Math.min(520, Math.max(360, centralWorkspace.height * 0.56))
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.rightMargin: 14
                        anchors.bottomMargin: (bottomTray.visible ? bottomTray.implicitHeight : 0) + 14
                        theme: root.theme
                        uiState: root.uiState
                        analysisController: root.analysisController
                        onCloseRequested: root.closeAskPanel()
                    }
                }

                InspectorPanel {
                    SplitView.preferredWidth: root.uiState.inspectorCollapsed ? 76 : root.uiState.inspectorExpandedWidth
                    SplitView.minimumWidth: root.uiState.inspectorCollapsed ? 72 : 320
                    SplitView.maximumWidth: root.uiState.inspectorCollapsed ? 88 : 428
                    SplitView.fillHeight: true
                    theme: root.theme
                    uiState: root.uiState
                    analysisController: root.analysisController
                    onAskRequested: root.openAskPanel()
                }
            }
        }
    }

    Component {
        id: overviewPageComponent

        ProjectOverviewPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            onAskRequested: root.openAskPanel()
            onChooseProjectRequested: topBar.openProjectPicker()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: capabilityMapPageComponent

        CapabilityMapPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            onAskRequested: root.openAskPanel()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: componentWorkbenchPageComponent

        ComponentWorkbenchPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            onAskRequested: root.openAskPanel()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: comparePageComponent

        ComparePage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            onAskRequested: root.openAskPanel()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: engineeringReportPageComponent

        ReportPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            mode: "engineering"
            onAskRequested: root.openAskPanel()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: recommendationsPageComponent

        ReportPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            mode: "recommendations"
            onAskRequested: root.openAskPanel()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: promptPackPageComponent

        ReportPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            mode: "promptPack"
            onAskRequested: root.openAskPanel()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: levelL1PageComponent

        ViewLevelPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            levelId: "l1"
            onAskRequested: root.openAskPanel()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: levelL2PageComponent

        ViewLevelPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            levelId: "l2"
            onAskRequested: root.openAskPanel()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: levelL3PageComponent

        ViewLevelPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            levelId: "l3"
            onAskRequested: root.openAskPanel()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: levelL4PageComponent

        ViewLevelPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            levelId: "l4"
            onAskRequested: root.openAskPanel()
            onNavigateTo: root.navigateTo(pageId)
        }
    }
}
