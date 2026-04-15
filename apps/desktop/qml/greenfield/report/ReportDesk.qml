import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../foundation"

ScrollView {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState

    readonly property var guide: root.analysisController.systemContextData || ({})
    readonly property var topModules: guide.topModules || []
    readonly property string reportHtml: renderMarkdown(root.analysisController.analysisReport)
    readonly property string contextHtml: renderMarkdown(root.analysisController.systemContextReport)

    clip: true
    contentWidth: availableWidth

    function textOr(value, fallbackText) {
        var text = String(value || "").trim()
        return text.length > 0 ? text : (fallbackText || "")
    }

    function renderMarkdown(markdownText) {
        var source = String(markdownText || "")
        if (!source.length)
            return ""
        return markdownRenderer ? markdownRenderer.toHtml(source) : source
    }

    function trimmedList(listValue, maxItems) {
        var values = []
        var source = listValue || []
        for (var i = 0; i < source.length; ++i) {
            var text = String(source[i] || "").trim()
            if (text.length === 0 || values.indexOf(text) >= 0)
                continue
            values.push(text)
            if (maxItems > 0 && values.length >= maxItems)
                break
        }
        return values
    }

    function aiDigest() {
        var parts = []
        var summary = String(root.analysisController.aiSummary || "").trim()
        var responsibility = String(root.analysisController.aiResponsibility || "").trim()
        var uncertainty = String(root.analysisController.aiUncertainty || "").trim()
        var nextActions = trimmedList(root.analysisController.aiNextActions, 4)
        var evidenceItems = trimmedList(root.analysisController.aiEvidence, 4)
        if (summary.length > 0)
            parts.push(summary)
        if (responsibility.length > 0)
            parts.push(responsibility)
        if (nextActions.length > 0)
            parts.push("建议下一步：\n" + nextActions.map(function(item, index) {
                return (index + 1) + ". " + item
            }).join("\n"))
        if (evidenceItems.length > 0)
            parts.push("判断依据：\n" + evidenceItems.map(function(item) {
                return "• " + item
            }).join("\n"))
        if (uncertainty.length > 0)
            parts.push("判断边界：" + uncertainty)
        return parts.join("\n\n").trim()
    }

    ColumnLayout {
        width: root.availableWidth > 0 ? root.availableWidth : root.width
        spacing: 16

        Rectangle {
            Layout.fillWidth: true
            radius: root.tokens.radius8
            color: root.tokens.panelStrong
            border.color: root.tokens.border1
            implicitHeight: headerColumn.implicitHeight + 36

            Column {
                id: headerColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 18
                spacing: 8

                Label {
                    width: parent.width
                    text: "深度诊断报告"
                    color: root.tokens.text1
                    font.family: root.tokens.displayFontFamily
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: root.tokens.radius8
            color: root.tokens.panelStrong
            border.color: root.tokens.border1
            implicitHeight: overviewColumn.implicitHeight + 32

            Column {
                id: overviewColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 16
                spacing: 12

                Label {
                    width: parent.width
                    text: root.analysisController.statusMessage || "当前还没有生成诊断结论。"
                    wrapMode: Text.WordWrap
                    color: root.tokens.text1
                    font.family: root.tokens.displayFontFamily
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                }


                Row {
                    width: parent.width
                    spacing: 12

                    StatusChip {
                        width: (parent.width - 36) / 4
                        tokens: root.tokens
                        label: "分析阶段"
                        value: root.analysisController.analysisPhase || "待命"
                        tone: "info"
                    }

                    StatusChip {
                        width: (parent.width - 36) / 4
                        tokens: root.tokens
                        label: "可见模块"
                        value: (((root.analysisController.capabilityScene || ({})).nodes || []).length).toString()
                        tone: "success"
                    }

                    StatusChip {
                        width: (parent.width - 36) / 4
                        tokens: root.tokens
                        label: "优先模块"
                        value: root.topModules.length > 0 ? root.topModules.length.toString() : "0"
                        tone: "warning"
                    }

                    StatusChip {
                        width: (parent.width - 36) / 4
                        tokens: root.tokens
                        label: "AI 状态"
                        value: root.analysisController.aiAvailable ? "已就绪" : "未配置"
                        tone: root.analysisController.aiAvailable ? "ai" : "warning"
                    }
                }

                Flow {
                    width: parent.width
                    spacing: 8

                    Repeater {
                        model: root.topModules

                        Rectangle {
                            radius: height / 2
                            color: root.tokens.signalCobaltSoft
                            border.color: root.tokens.signalCobalt
                            implicitWidth: moduleChipLabel.implicitWidth + 16
                            implicitHeight: 26

                            Label {
                                id: moduleChipLabel
                                anchors.centerIn: parent
                                text: modelData
                                color: root.tokens.signalCobalt
                                font.family: root.tokens.textFontFamily
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: root.tokens.radius8
            color: root.tokens.panelStrong
            border.color: root.tokens.border1
            implicitHeight: aiColumn.implicitHeight + 32

            Column {
                id: aiColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 16
                spacing: 12

                Row {
                    width: parent.width
                    spacing: 12

                    Label {
                        width: parent.width - aiButton.width - 12
                        text: "AI 诊断"
                        color: root.tokens.text1
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    ActionButton {
                        id: aiButton
                        tokens: root.tokens
                        text: root.analysisController.aiBusy ? "正在生成..." : "生成诊断"
                        tone: "ai"
                        compact: true
                        enabled: root.analysisController.aiAvailable && !root.analysisController.aiBusy
                        onClicked: root.analysisController.requestReportAiExplanation(
                                       "基于当前报告生成更完整的中文架构诊断。请明确写出总体判断、关键风险来源、建议阅读路径、最值得先看的模块或文件，以及具体下一步动作。不要只给简短概括，要给到适合报告页阅读的展开说明，尽量达到约 200-300 字的信息量，但所有判断都必须受当前证据约束。")
                    }
                }

                Label {
                    width: parent.width
                    text: root.analysisController.aiBusy
                          ? "AI 正在结合当前报告和系统上下文生成诊断。"
                          : (root.aiDigest().length > 0
                             ? root.aiDigest()
                             : "点击“生成诊断”后，这里会显示面向当前报告的补充解读。")
                    wrapMode: Text.WordWrap
                    color: root.tokens.text2
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 13
                    lineHeight: 1.2
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 640
                radius: root.tokens.radius8
                color: root.tokens.panelStrong
                border.color: root.tokens.border1

                Label {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: 16
                    anchors.topMargin: 16
                    text: "系统上下文"
                    color: root.tokens.text1
                    font.family: root.tokens.displayFontFamily
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }

                ActionButton {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: 16
                    anchors.topMargin: 12
                    tokens: root.tokens
                    text: "复制原文"
                    tone: "secondary"
                    compact: true
                    enabled: root.analysisController.systemContextReport.length > 0
                    onClicked: root.analysisController.copyTextToClipboard(root.analysisController.systemContextReport)
                }

                MarkdownPane {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: 16
                    anchors.topMargin: 54
                    tokens: root.tokens
                    htmlText: root.contextHtml
                    emptyText: "运行分析后，这里会显示系统上下文报告。"
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 640
                radius: root.tokens.radius8
                color: root.tokens.panelStrong
                border.color: root.tokens.border1

                Label {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: 16
                    anchors.topMargin: 16
                    text: "工程报告"
                    color: root.tokens.text1
                    font.family: root.tokens.displayFontFamily
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }

                ActionButton {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: 16
                    anchors.topMargin: 12
                    tokens: root.tokens
                    text: "复制原文"
                    tone: "secondary"
                    compact: true
                    enabled: root.analysisController.analysisReport.length > 0
                    onClicked: root.analysisController.copyTextToClipboard(root.analysisController.analysisReport)
                }

                MarkdownPane {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: 16
                    anchors.topMargin: 54
                    tokens: root.tokens
                    htmlText: root.reportHtml
                    emptyText: "还没有生成工程分析报告。"
                }
            }
        }
    }

    component StatusChip: Rectangle {
        required property QtObject tokens
        property string label: ""
        property string value: ""
        property string tone: "info"

        radius: tokens.radius8
        color: tokens.toneSoft(tone)
        border.color: tokens.toneColor(tone)
        implicitHeight: 72

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 4

            Label {
                width: parent.width
                text: label
                color: tokens.text3
                font.family: tokens.textFontFamily
                font.pixelSize: 11
            }

            Label {
                width: parent.width
                text: value
                color: tokens.text1
                font.family: tokens.textFontFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
            }
        }
    }

    component MarkdownPane: Rectangle {
        required property QtObject tokens
        property string htmlText: ""
        property string emptyText: ""

        radius: tokens.radius6
        color: tokens.panelStrong
        border.color: tokens.border1

        Flickable {
            id: paneFlick
            anchors.fill: parent
            anchors.margins: 1
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            contentWidth: width
            contentHeight: textFrame.height

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            Item {
                id: textFrame
                width: paneFlick.width
                height: Math.max(paneFlick.height, markdownText.contentHeight + 28)

                TextEdit {
                    id: markdownText
                    x: 14
                    y: 14
                    width: Math.max(0, textFrame.width - 28)
                    height: contentHeight
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    textFormat: TextEdit.RichText
                    text: htmlText.length > 0 ? htmlText : "<p>" + emptyText + "</p>"
                    color: tokens.text2
                    font.family: tokens.textFontFamily
                    font.pixelSize: 13
                }
            }
        }
    }
}
