import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"
import "../report"

ScrollView {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject uiState

    property string mode: "engineering"

    signal askRequested()
    signal navigateTo(string pageId)

    anchors.fill: parent
    clip: true
    contentWidth: availableWidth

    function focusCapability(capabilityName) {
        var targetName = capabilityName || ""
        var nodes = root.uiState.selection.capabilityNodes || []
        for (var index = 0; index < nodes.length; ++index) {
            if ((nodes[index].name || "") === targetName) {
                root.uiState.selection.selectCapabilityNode(nodes[index])
                root.navigateTo("project.capabilityMap")
                return
            }
        }
        root.navigateTo("project.capabilityMap")
    }

    function reportTitle() {
        if (root.mode === "recommendations")
            return "改造动作摘要"
        if (root.mode === "promptPack")
            return "提示词工作台"
        return "报告工作流总览"
    }

    function reportSummary() {
        if (root.mode === "recommendations")
            return root.analysisController.aiSummary.length > 0
                   ? root.analysisController.aiSummary
                   : "围绕当前分析结果，把可执行的改造建议、下一步动作和回跳入口组织到同一页。"
        if (root.mode === "promptPack")
            return "把当前分析结果转成可复制的 AI 提示词模板，便于继续交给外部代理或团队成员。"
        return "报告页不再只是静态 Markdown 贴片，而是能继续回到能力地图、对照工作台和 Ask SAVT 的操作页面。"
    }

    ColumnLayout {
        width: parent.width
        spacing: 16

        AppCard {
            Layout.fillWidth: true
            fillColor: "#FFFFFF"
            borderColor: root.theme.borderStrong
            cornerRadius: root.theme.radiusXxl
            minimumContentHeight: 132

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: root.reportTitle()
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 24
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.reportSummary()
                            wrapMode: Text.WordWrap
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }

                    AppButton {
                        theme: root.theme
                        tone: "ghost"
                        text: root.mode === "promptPack" ? "复制提示词包" : "复制报告正文"
                        onClicked: root.analysisController.copyTextToClipboard(root.mode === "promptPack"
                                                                               ? root.analysisController.aiSummary
                                                                               : root.analysisController.analysisReport)
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    AppChip {
                        text: "状态 " + (root.analysisController.analysisPhase.length > 0
                                         ? root.analysisController.analysisPhase
                                         : "待命")
                        compact: true
                        fillColor: "#FFFFFF"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }

                    AppChip {
                        text: "节点 " + root.uiState.selection.capabilityNodes.length
                        compact: true
                        fillColor: "#FFFFFF"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }

                    AppChip {
                        text: "AI " + (root.analysisController.aiAvailable ? "已就绪" : "未就绪")
                        compact: true
                        fillColor: "#FFFFFF"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }

                    AppChip {
                        text: root.mode === "promptPack" ? "可复制" : "可回跳"
                        compact: true
                        fillColor: "#FFFFFF"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }
                }
            }
        }

        ReportSummarySection {
            Layout.fillWidth: true
            visible: root.mode !== "promptPack"
            theme: root.theme
            analysisController: root.analysisController
            selectionState: root.uiState.selection
        }

        ReportCapabilitySection {
            Layout.fillWidth: true
            visible: root.mode === "engineering"
            theme: root.theme
            analysisController: root.analysisController
            onCapabilityRequested: root.focusCapability(capabilityName)
        }

        ReportEvidenceSection {
            Layout.fillWidth: true
            visible: root.mode !== "promptPack"
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
        }

        ReportActionsSection {
            Layout.fillWidth: true
            visible: root.mode !== "promptPack"
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            onCapabilityMapRequested: root.navigateTo("project.capabilityMap")
            onAskRequested: root.askRequested()
            onPromptPackRequested: root.navigateTo("report.promptPack")
            onCompareRequested: root.navigateTo("project.compare")
        }

        PromptPackSection {
            Layout.fillWidth: true
            visible: root.mode === "engineering" || root.mode === "promptPack" || root.mode === "recommendations"
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
        }

        AppCard {
            Layout.fillWidth: true
            visible: root.mode !== "promptPack"
            fillColor: "#FFFFFF"
            borderColor: root.theme.borderStrong
            cornerRadius: root.theme.radiusXxl
            minimumContentHeight: 260
            stacked: false

            AppSection {
                anchors.fill: parent
                theme: root.theme
                eyebrow: "Markdown"
                title: root.mode === "recommendations" ? "原始工程报告参考" : "完整 Markdown 报告"
                subtitle: "保留原始输出，既方便复制，也方便继续扩展导出、分享和审阅流程。"

                Label {
                    Layout.fillWidth: true
                    text: root.analysisController.analysisReport.length > 0
                          ? root.analysisController.analysisReport
                          : "还没有生成工程分析报告。"
                    wrapMode: Text.WordWrap
                    color: root.theme.inkMuted
                    font.family: root.theme.textFontFamily
                    font.pixelSize: 12
                }
            }
        }
    }
}
