import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"
import "../graph"
import "../workspace"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject uiState

    signal askRequested()
    signal navigateTo(string pageId)

    anchors.fill: parent

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        MapToolbar {
            Layout.fillWidth: true
            theme: root.theme
            selectionState: root.uiState.selection
            analysisController: root.analysisController
            onDrillRequested: root.navigateTo("project.componentWorkbench")
            onAskRequested: root.askRequested()
        }

        AppCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            stacked: false
            contentPadding: 0
            fillColor: root.theme.canvasBase
            borderColor: root.theme.borderSubtle
            cornerRadius: root.theme.radiusXxl

            CapabilityGraphCanvas {
                anchors.fill: parent
                theme: root.theme
                analysisController: root.analysisController
                selectionState: root.uiState.selection
                onDrillRequested: {
                    root.uiState.selection.drillIntoCapabilityNode(node)
                    root.navigateTo("project.componentWorkbench")
                }
            }
        }

        ContextActionStrip {
            Layout.fillWidth: true
            theme: root.theme
            title: root.uiState.selection.selectedCapabilityNode
                   ? ("围绕「" + root.uiState.selection.selectedCapabilityNode.name + "」继续")
                   : "先从能力地图里选一个对象"
            summary: root.uiState.selection.selectedCapabilityNode
                     ? (root.uiState.selection.selectedCapabilityNode.summary || "可以直接下钻到组件工作台，查看其内部组件关系和改造切口。")
                     : "L2 把能力图作为主工作对象。先选一个能力域，右侧摘要和底部上下文才会进入更具体的工作态。"
            chips: [
                {"text": "节点 " + root.uiState.selection.capabilityNodes.length, "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal},
                {"text": "关系 " + root.uiState.selection.capabilityEdges.length, "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal},
                {"text": root.uiState.selection.relationshipViewMode === "all" ? "全部关系" : "聚焦关系", "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal},
                {"text": "图优先，详情按需展开", "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal}
            ]
            primaryText: root.uiState.selection.selectedCapabilityNode ? "进入组件工作台" : ""
            primaryEnabled: !!root.uiState.selection.selectedCapabilityNode
            secondaryText: "L2 专业视图"
            secondaryVisible: true
            copyVisible: !!root.uiState.selection.selectedCapabilityNode
            copyEnabled: !!root.uiState.selection.selectedCapabilityNode
            onPrimaryRequested: root.navigateTo("project.componentWorkbench")
            onSecondaryRequested: root.navigateTo("levels.l2")
            onAskRequested: root.askRequested()
            onCopyRequested: root.analysisController.copyCodeContextToClipboard(root.uiState.selection.selectedCapabilityNode.id)
        }
    }
}
