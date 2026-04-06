import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject selectionState
    required property QtObject analysisController

    signal backRequested()
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
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: root.selectionState.selectedCapabilityNode
                              ? ("组件工作台 · " + root.selectionState.selectedComponentSceneTitle())
                              : "组件工作台"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.selectionState.selectedCapabilityNode
                              ? ((root.selectionState.selectedComponentSceneSummary() || "").length > 0
                                 ? root.selectionState.selectedComponentSceneSummary()
                                 : "保留父级能力域上下文，继续展开组件、阶段和依赖路径。")
                              : "先在能力地图里选中一个能力域，再进入这里查看其内部组件图。"
                        wrapMode: Text.WordWrap
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: "返回能力地图"
                    onClicked: root.backRequested()
                }

                SegmentedControl {
                    theme: root.theme
                    model: [
                        {"value": "focused", "label": "聚焦关系"},
                        {"value": "all", "label": "全部关系"}
                    ]
                    currentValue: root.selectionState.componentRelationshipViewMode
                    onActivated: root.selectionState.componentRelationshipViewMode = value
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                SearchField {
                    Layout.fillWidth: true
                    theme: root.theme
                    text: root.selectionState.componentFilterText
                    placeholderText: "按名称、角色或关键符号筛选组件"
                    onTextChanged: {
                        if (root.selectionState.componentFilterText !== text)
                            root.selectionState.componentFilterText = text
                    }
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: "清空筛选"
                    enabled: root.selectionState.componentFilterText.length > 0
                    onClicked: root.selectionState.componentFilterText = ""
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                AppChip {
                    text: root.selectionState.selectedCapabilityNode
                          ? ("能力域 " + root.selectionState.selectedCapabilityNode.name)
                          : "等待选择能力域"
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: "组件 " + root.selectionState.componentNodes.length
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: "关系 " + root.selectionState.componentEdges.length
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: "阶段 " + root.selectionState.componentGroups.length
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

            }
        }
    }
}
