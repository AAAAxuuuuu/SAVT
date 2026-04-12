import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState

    clip: true
    contentWidth: availableWidth

    ColumnLayout {
        width: parent.width
        spacing: 16

        ReportPanel {
            Layout.fillWidth: true
            tokens: root.tokens
            title: "深度诊断报告"
            subtitle: root.analysisController.analysisReport.length > 0
                      ? "报告已由项目内核生成，可复制、审阅或继续交给 AI 解释。"
                      : "运行分析后，这里会显示完整工程分析报告、系统上下文和 AI 诊断。"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 86
            radius: root.tokens.radius8
            color: root.tokens.panelBase
            border.color: root.tokens.border1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                StatusChip { tokens: root.tokens; label: "分析状态"; value: root.analysisController.analysisPhase || "待命"; tone: "info" }
                StatusChip { tokens: root.tokens; label: "能力节点"; value: (((root.analysisController.capabilityScene || ({})).nodes || []).length).toString(); tone: "success" }
                StatusChip { tokens: root.tokens; label: "AI"; value: root.analysisController.aiAvailable ? "已就绪" : "未配置"; tone: root.analysisController.aiAvailable ? "ai" : "warning" }
                StatusChip { tokens: root.tokens; label: "AST 文件"; value: (root.analysisController.astFileItems || []).length.toString(); tone: "moss" }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(180, aiSummary.implicitHeight + 86)
            radius: root.tokens.radius8
            color: root.tokens.panelBase
            border.color: root.tokens.border1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "AI 洞察"
                        color: root.tokens.text1
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "生成诊断"
                        enabled: root.analysisController.aiAvailable
                        onClicked: root.analysisController.requestReportAiExplanation("基于当前报告生成架构诊断、风险和下一步建议。")
                    }
                }

                Label {
                    id: aiSummary
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: root.analysisController.aiBusy ? "DeepSeek 正在结合报告和证据链分析..."
                          : (root.analysisController.aiSummary || root.analysisController.aiStatusMessage || "点击“生成诊断”后，这里会显示受证据约束的 AI 架构解释。")
                    wrapMode: Text.WordWrap
                    color: root.tokens.text2
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 13
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 380
            radius: root.tokens.radius8
            color: root.tokens.panelBase
            border.color: root.tokens.border1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "完整 Markdown 报告"
                        color: root.tokens.text1
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "复制报告"
                        enabled: root.analysisController.analysisReport.length > 0
                        onClicked: root.analysisController.copyTextToClipboard(root.analysisController.analysisReport)
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    Label {
                        width: parent.width
                        text: root.analysisController.analysisReport.length > 0
                              ? root.analysisController.analysisReport
                              : "还没有生成工程分析报告。"
                        wrapMode: Text.WordWrap
                        color: root.tokens.text2
                        font.family: root.tokens.monoFontFamily
                        font.pixelSize: 12
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(760, Math.max(420, systemContextText.implicitHeight + 92))
            Layout.minimumHeight: 360
            radius: root.tokens.radius8
            color: root.tokens.panelBase
            border.color: root.tokens.border1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: "系统上下文"
                        color: root.tokens.text1
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "复制上下文"
                        enabled: root.analysisController.systemContextReport.length > 0
                        onClicked: root.analysisController.copyTextToClipboard(root.analysisController.systemContextReport)
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    Label {
                        id: systemContextText
                        width: parent.width
                        text: root.analysisController.systemContextReport.length > 0
                              ? root.analysisController.systemContextReport
                              : "运行分析后，这里会显示系统上下文报告。"
                        wrapMode: Text.WordWrap
                        color: root.tokens.text2
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 13
                    }
                }
            }
        }
    }

    component ReportPanel: Rectangle {
        required property QtObject tokens
        property string title: ""
        property string subtitle: ""

        implicitHeight: 104
        radius: tokens.radius8
        color: tokens.panelBase
        border.color: tokens.border1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 7

            Label {
                text: title
                color: tokens.text1
                font.family: tokens.displayFontFamily
                font.pixelSize: 24
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                text: subtitle
                wrapMode: Text.WordWrap
                color: tokens.text3
                font.family: tokens.textFontFamily
                font.pixelSize: 13
            }
        }
    }

    component StatusChip: Rectangle {
        required property QtObject tokens
        property string label: ""
        property string value: ""
        property string tone: "info"

        Layout.fillWidth: true
        Layout.fillHeight: true
        radius: tokens.radius8
        color: tokens.toneSoft(tone)
        border.color: tokens.toneColor(tone)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 3

            Label {
                text: label
                color: tokens.text3
                font.family: tokens.textFontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                text: value
                color: tokens.toneColor(tone)
                elide: Text.ElideRight
                font.family: tokens.textFontFamily
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
        }
    }
}
