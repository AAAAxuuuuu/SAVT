import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject uiState
    required property QtObject analysisController

    signal askRequested()

    readonly property QtObject inspectorState: uiState.inspector
    readonly property bool hasSelection: root.inspectorState.kind.length > 0

    implicitWidth: root.uiState.inspectorCollapsed ? 84 : root.uiState.inspectorExpandedWidth

    Behavior on implicitWidth {
        NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
    }

    AppCard {
        anchors.fill: parent
        fillColor: root.theme.inspectorBase
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXxl
        contentPadding: 14

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    visible: !root.uiState.inspectorCollapsed

                    Label {
                        text: "当前焦点"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "摘要优先，长证据和 AI 详情按需展开，尽量不和主画布抢视线。"
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
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
                    text: root.uiState.inspectorCollapsed ? "展开" : "收起"
                    onClicked: root.uiState.inspectorCollapsed = !root.uiState.inspectorCollapsed
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12
                visible: root.uiState.inspectorCollapsed

                StatusBadge {
                    theme: root.theme
                    text: root.hasSelection
                          ? (root.inspectorState.kind === "edge" ? "关系" : "模块")
                          : "未选中"
                    tone: root.hasSelection ? "brand" : "neutral"
                }

                Label {
                    Layout.fillWidth: true
                    text: root.hasSelection ? root.inspectorState.title : "等待选择"
                    wrapMode: Text.WordWrap
                    maximumLineCount: 8
                    elide: Text.ElideRight
                    color: root.theme.inkStrong
                    font.family: root.theme.displayFontFamily
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    visible: !root.hasSelection
                    text: "在画布中选中一个能力域、组件或关系。"
                    wrapMode: Text.WordWrap
                    color: root.theme.inkMuted
                    font.family: root.theme.textFontFamily
                    font.pixelSize: 12
                }

                Item { Layout.fillHeight: true }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12
                visible: !root.uiState.inspectorCollapsed

                AppCard {
                    Layout.fillWidth: true
                    visible: root.hasSelection
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    cornerRadius: root.theme.radiusXl
                    minimumContentHeight: 104

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: root.inspectorState.title
                            wrapMode: Text.WordWrap
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 22
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.inspectorState.subtitle
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8

                            AppChip {
                                visible: root.inspectorState.kind === "node"
                                text: root.inspectorState.nodeKindLabel
                                compact: true
                                fillColor: "#FFFFFF"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }

                            AppChip {
                                visible: root.inspectorState.kind === "node"
                                         && root.inspectorState.nodeRoleLabel.length > 0
                                text: root.inspectorState.nodeRoleLabel
                                compact: true
                                fillColor: "#FFFFFF"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }

                            AppChip {
                                visible: root.inspectorState.kind === "node"
                                text: root.inspectorState.nodeFileCountLabel
                                compact: true
                                fillColor: "#FFFFFF"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }

                            AppChip {
                                visible: root.inspectorState.kind === "edge"
                                text: root.inspectorState.edgeWeightLabel
                                compact: true
                                fillColor: "#FFFFFF"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }
                        }
                    }
                }

                AppCard {
                    Layout.fillWidth: true
                    visible: !root.hasSelection
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    cornerRadius: root.theme.radiusXl
                    minimumContentHeight: 164

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        StatusBadge {
                            theme: root.theme
                            text: "等待选择"
                            tone: "neutral"
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "先在主画布中选一个节点或关系"
                            wrapMode: Text.WordWrap
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "右侧面板只保留摘要、证据线索和下一步动作；长内容继续放到底部托盘或 AI 面板。"
                            wrapMode: Text.WordWrap
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8

                            AppChip {
                                text: "摘要优先"
                                compact: true
                                fillColor: "#FFFFFF"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }

                            AppChip {
                                text: "详情按需展开"
                                compact: true
                                fillColor: "#FFFFFF"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }
                        }

                        AppButton {
                            theme: root.theme
                            compact: true
                            tone: "ai"
                            text: "打开 Ask SAVT"
                            onClicked: root.askRequested()
                        }
                    }
                }

                InspectorSummaryCard {
                    Layout.fillWidth: true
                    visible: root.hasSelection
                    theme: root.theme
                    inspectorState: root.inspectorState
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: root.hasSelection

                    AppButton {
                        theme: root.theme
                        compact: true
                        tone: "ghost"
                        text: root.uiState.inspectorDetailsExpanded ? "收起详情" : "展开详情"
                        onClicked: root.uiState.inspectorDetailsExpanded = !root.uiState.inspectorDetailsExpanded
                    }

                    AppButton {
                        theme: root.theme
                        compact: true
                        tone: "ai"
                        text: "Ask SAVT"
                        onClicked: root.askRequested()
                    }

                    AppButton {
                        theme: root.theme
                        compact: true
                        tone: "ghost"
                        text: "复制对象上下文"
                        visible: root.inspectorState.supportsCodeContext
                        enabled: root.inspectorState.supportsCodeContext
                        onClicked: root.analysisController.copyCodeContextToClipboard(root.inspectorState.selectedNode.id)
                    }
                }

                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root.hasSelection && root.uiState.inspectorDetailsExpanded
                    active: visible
                    sourceComponent: detailsComponent
                }
            }
        }
    }

    Component {
        id: detailsComponent

        InspectorDetailsDrawer {
            theme: root.theme
            inspectorState: root.inspectorState
            analysisController: root.analysisController
        }
    }
}
