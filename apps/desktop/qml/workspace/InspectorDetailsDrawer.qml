import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"
import "../logic/InspectorFormatter.js" as InspectorFormatter

Item {
    id: root

    required property QtObject theme
    required property QtObject inspectorState
    required property QtObject analysisController

    readonly property bool hasSelection: root.inspectorState.kind.length > 0

    implicitHeight: root.hasSelection ? 360 : 140

    function detailGroups() {
        return [
            {"title": "事实", "items": root.inspectorState.facts},
            {"title": "规则", "items": root.inspectorState.rules},
            {"title": "更多结论", "items": root.inspectorState.conclusions.slice(1)}
        ]
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: parent.width
            spacing: 10

            AppCard {
                Layout.fillWidth: true
                visible: !root.hasSelection
                fillColor: root.theme.surfaceSecondary
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusLg
                minimumContentHeight: 110

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    Label {
                        text: "详情将在选择后出现"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "先在能力地图或组件工作台中选中一个节点或关系，这里会展示证据、规则、来源和 AI 辅助说明。"
                        wrapMode: Text.WordWrap
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }
            }

            AppCard {
                Layout.fillWidth: true
                visible: root.hasSelection
                fillColor: root.theme.surfaceSecondary
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusLg
                minimumContentHeight: 96

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "证据结论"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }

                        Item { Layout.fillWidth: true }

                        StatusBadge {
                            theme: root.theme
                            text: InspectorFormatter.confidenceLabelText(root.inspectorState.confidenceLabel)
                            tone: root.inspectorState.confidenceLabel === "high"
                                  ? "success"
                                  : (root.inspectorState.confidenceLabel === "medium"
                                     ? "warning"
                                     : (root.inspectorState.confidenceLabel === "low" ? "danger" : "neutral"))
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.inspectorState.conclusions.length > 0
                              ? root.inspectorState.conclusions[0]
                              : "当前还没有可展示的结论。"
                        wrapMode: Text.WordWrap
                        color: root.theme.inkStrong
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: (root.inspectorState.confidenceReason || "").length > 0
                        text: root.inspectorState.confidenceReason
                        wrapMode: Text.WordWrap
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }
            }

            Repeater {
                model: root.detailGroups()

                AppCard {
                    Layout.fillWidth: true
                    visible: root.hasSelection && (modelData.items || []).length > 0
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    cornerRadius: root.theme.radiusLg
                    minimumContentHeight: 82

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            text: modelData.title
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }

                        Repeater {
                            model: (modelData.items || []).slice(0, 5)

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: detailLabel.implicitHeight + 16
                                radius: root.theme.radiusMd
                                color: root.theme.surfaceSecondary
                                border.color: root.theme.borderSubtle

                                Label {
                                    id: detailLabel
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.margins: 10
                                    text: modelData
                                    wrapMode: Text.WordWrap
                                    color: root.theme.inkNormal
                                    font.family: root.theme.textFontFamily
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }
                }
            }

            AppCard {
                Layout.fillWidth: true
                visible: root.hasSelection
                         && (root.inspectorState.sourceSymbols.length > 0
                             || root.inspectorState.sourceModules.length > 0)
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusLg
                minimumContentHeight: 82

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        text: "证据来源"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: root.inspectorState.sourceSymbols

                            AppChip {
                                text: modelData
                                compact: true
                                fillColor: "#FFFFFF"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: root.inspectorState.sourceModules.length > 0
                        text: "模块线索：" + root.inspectorState.sourceModules.join("、")
                        wrapMode: Text.WordWrap
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }
            }

            AppCard {
                Layout.fillWidth: true
                visible: root.hasSelection
                         && (root.analysisController.aiAvailable
                             || root.analysisController.aiHasResult
                             || (root.analysisController.aiSetupMessage || "").length > 0)
                fillColor: "#F0F5F7"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusLg
                minimumContentHeight: 102

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "AI 补充说明"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }

                        Item { Layout.fillWidth: true }

                        AppButton {
                            theme: root.theme
                            compact: true
                            tone: "ghost"
                            text: "刷新状态"
                            enabled: !root.analysisController.aiBusy
                            onClicked: root.analysisController.refreshAiAvailability()
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.analysisController.aiAvailable
                              ? root.analysisController.aiStatusMessage
                              : root.analysisController.aiSetupMessage
                        wrapMode: Text.WordWrap
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: root.analysisController.aiHasResult && !root.analysisController.aiBusy
                        text: root.analysisController.aiSummary
                        wrapMode: Text.WordWrap
                        maximumLineCount: 4
                        elide: Text.ElideRight
                        color: root.theme.inkStrong
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }
            }
        }
    }
}
