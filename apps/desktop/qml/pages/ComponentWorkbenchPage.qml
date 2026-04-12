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

        ComponentToolbar {
            Layout.fillWidth: true
            theme: root.theme
            selectionState: root.uiState.selection
            analysisController: root.analysisController
            onBackRequested: root.navigateTo("project.capabilityMap")
            onAskRequested: root.askRequested()
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            active: true
            sourceComponent: root.uiState.selection.selectedCapabilityNode ? graphComponent : emptyComponent
        }

        ContextActionStrip {
            Layout.fillWidth: true
            theme: root.theme
            title: root.uiState.selection.selectedComponentNode
                   ? ("围绕组件「" + root.uiState.selection.selectedComponentNode.name + "」继续")
                   : (root.uiState.selection.selectedCapabilityNode
                      ? ("当前能力域 · " + root.uiState.selection.selectedCapabilityNode.name)
                      : "等待选择能力域")
            summary: root.uiState.selection.selectedComponentNode
                     ? (root.uiState.selection.selectedComponentNode.summary || root.uiState.inspector.nextStepSummary)
                     : (root.uiState.selection.selectedCapabilityNode
                        ? "当前已经进入 L3。你可以继续选中内部组件，结合右侧摘要、底部托盘和 AI 导览形成具体改造切口。"
                        : "先在能力地图里选中一个能力域，再进入这里查看其内部组件关系。")
            chips: [
                {"text": root.uiState.selection.selectedCapabilityNode ? ("能力域 " + root.uiState.selection.selectedCapabilityNode.name) : "等待能力域", "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal},
                {"text": "组件 " + root.uiState.selection.componentNodes.length, "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal},
                {"text": "阶段 " + root.uiState.selection.componentGroups.length, "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal},
                {"text": "父能力上下文已保留", "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal}
            ]
            primaryText: root.uiState.selection.selectedCapabilityNode ? "返回能力地图" : "前往能力地图"
            secondaryText: "查看高级报告"
            secondaryVisible: true
            copyVisible: !!root.uiState.selection.selectedComponentNode || !!root.uiState.selection.selectedCapabilityNode
            copyEnabled: copyVisible
            onPrimaryRequested: root.navigateTo("project.capabilityMap")
            onSecondaryRequested: root.navigateTo("report.engineering")
            onAskRequested: root.askRequested()
            onCopyRequested: root.analysisController.copyCodeContextToClipboard(
                                 root.uiState.selection.selectedComponentNode
                                 ? root.uiState.selection.selectedComponentNode.id
                                 : root.uiState.selection.selectedCapabilityNode.id)
        }
    }

    Component {
        id: graphComponent

        AppCard {
            contentPadding: 0
            fillColor: root.theme.canvasBase
            borderColor: root.theme.borderSubtle
            cornerRadius: root.theme.radiusXxl
            stacked: false

            ComponentGraphCanvas {
                anchors.fill: parent
                theme: root.theme
                analysisController: root.analysisController
                selectionState: root.uiState.selection
            }
        }
    }

    Component {
        id: emptyComponent

        AppCard {
            fillColor: "#FFFFFF"
            borderColor: root.theme.borderSubtle
            cornerRadius: root.theme.radiusXxl

            AppEmptyState {
                anchors.fill: parent
                theme: root.theme
                title: "还没有进入组件工作台"
                description: "先在能力地图里选择一个能力域，再进入这里展开组件、阶段和依赖路径。进入后会保留父能力上下文，不需要重新定位。"
                actionText: "前往能力地图"
                onActionRequested: root.navigateTo("project.capabilityMap")
            }
        }
    }
}
