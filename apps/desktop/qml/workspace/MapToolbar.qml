import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject selectionState
    required property QtObject analysisController

    signal drillRequested()
    signal askRequested()

    implicitHeight: card.implicitHeight

    AppCard {
        id: card
        anchors.fill: parent
        fillColor: root.theme.accentCanvas
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXl
        contentPadding: 14

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: "能力地图工作区"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.selectionState.selectedCapabilityNode
                              ? ("当前已聚焦「" + root.selectionState.selectedCapabilityNode.name + "」，可以沿着关系继续核对上下游，再决定是否下钻。")
                              : (root.selectionState.capabilityScene.summary || "以业务能力为前门理解项目，把模块和关系收敛到可继续下钻的结构地图。")
                        wrapMode: Text.WordWrap
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }

                SegmentedControl {
                    theme: root.theme
                    model: [
                        {"value": "focused", "label": "聚焦关系"},
                        {"value": "all", "label": "全部关系"}
                    ]
                    currentValue: root.selectionState.relationshipViewMode
                    onActivated: root.selectionState.relationshipViewMode = value
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                AppChip {
                    text: "节点 " + root.selectionState.capabilityNodes.length
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: "关系 " + root.selectionState.capabilityEdges.length
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: "分组 " + root.selectionState.capabilityGroups.length
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    visible: !!root.selectionState.selectedCapabilityNode
                    text: "当前能力域 " + (root.selectionState.selectedCapabilityNode ? root.selectionState.selectedCapabilityNode.name : "")
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "accent"
                    text: "进入组件工作台"
                    enabled: !!root.selectionState.selectedCapabilityNode
                    onClicked: root.drillRequested()
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ai"
                    text: "生成当前能力解读"
                    enabled: !!root.selectionState.selectedCapabilityNode
                    onClicked: root.askRequested()
                }

                Item { Layout.fillWidth: true }

                StatusBadge {
                    theme: root.theme
                    text: root.analysisController.analyzing ? "图谱刷新中" : "图谱已就绪"
                    tone: root.analysisController.analyzing ? "info" : "success"
                }
            }
        }
    }
}
