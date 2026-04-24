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
    readonly property var algorithmSummary: guide.algorithmSummary || ({})
    readonly property var algorithmMetrics: guide.algorithmMetrics || []
    readonly property var algorithmStages: guide.algorithmStages || []
    readonly property var algorithmCaches: guide.algorithmCaches || []
    readonly property var algorithmEvidence: guide.algorithmEvidence || []
    readonly property var hotspotSignals: guide.hotspotSignals || []
    readonly property var riskSignals: guide.riskSignals || []
    readonly property var readingOrder: guide.readingOrder || []
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
        if (summary.length > 0)
            parts.push(summary)
        if (responsibility.length > 0)
            parts.push(responsibility)
        if (nextActions.length > 0)
            parts.push("建议下一步：\n" + nextActions.map(function(item, index) {
                return (index + 1) + ". " + item
            }).join("\n"))
        if (uncertainty.length > 0)
            parts.push("边界提醒：" + uncertainty)
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
            implicitHeight: headerColumn.implicitHeight + 34

            ColumnLayout {
                id: headerColumn
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: "Algorithm Report"
                            color: root.tokens.signalCobalt
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.textOr(root.algorithmSummary.headline, "证据驱动多粒度架构重建")
                            color: root.tokens.text1
                            font.family: root.tokens.displayFontFamily
                            font.pixelSize: 24
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.textOr(root.algorithmSummary.summary, root.analysisController.statusMessage)
                            wrapMode: Text.WordWrap
                            color: root.tokens.text2
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 13
                            lineHeight: 1.18
                        }
                    }

                    ActionButton {
                        tokens: root.tokens
                        text: root.analysisController.aiBusy ? "AI 复盘中..." : "AI 复盘"
                        tone: "ai"
                        compact: true
                        enabled: root.analysisController.aiAvailable && !root.analysisController.aiBusy
                        onClicked: root.analysisController.requestReportAiExplanation(
                                       "请基于当前 SAVT 架构重建报告做一次算法复盘。"
                                       + " 请明确总结：当前算法链路最强的亮点、证据不足的环节、缓存和精度状态对结论的影响、以及接下来最值得补强的代码或实验方向。")
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    StatusPill {
                        tokens: root.tokens
                        text: root.textOr(root.algorithmSummary.modeLine, root.caseState.trustLabel())
                        tone: root.caseState.trustTone()
                    }

                    StatusPill {
                        tokens: root.tokens
                        text: root.textOr(root.algorithmSummary.cacheLine, "")
                        tone: "moss"
                    }

                    StatusPill {
                        tokens: root.tokens
                        text: root.textOr(root.algorithmSummary.evidenceLine, "")
                        tone: "ai"
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width >= 1320 ? 3 : 2
            columnSpacing: 14
            rowSpacing: 14

            Repeater {
                model: root.algorithmMetrics

                ReportMetricCard {
                    required property var modelData

                    Layout.fillWidth: true
                    tokens: root.tokens
                    title: String(modelData.label || "")
                    value: String(modelData.value || "")
                    detail: String(modelData.detail || "")
                    tone: String(modelData.tone || "info")
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: root.tokens.radius8
            color: root.tokens.panelStrong
            border.color: root.tokens.border1
            implicitHeight: stageColumn.implicitHeight + 28

            ColumnLayout {
                id: stageColumn
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Label {
                    text: "阶段复盘"
                    color: root.tokens.text1
                    font.family: root.tokens.displayFontFamily
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 12

                    Repeater {
                        model: root.algorithmStages

                        StageCard {
                            required property var modelData

                            tokens: root.tokens
                            width: 248
                            title: String(modelData.title || "")
                            kicker: String(modelData.kicker || "")
                            value: String(modelData.value || "")
                            summary: String(modelData.summary || "")
                            tone: String(modelData.tone || "info")
                        }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width >= 1400 ? 3 : 2
            columnSpacing: 14
            rowSpacing: 14

            SignalPanel { Layout.fillWidth: true; tokens: root.tokens; title: "证据计量"; items: root.algorithmEvidence; emptyText: "暂无证据统计。"; tone: "ai" }
            SignalPanel { Layout.fillWidth: true; tokens: root.tokens; title: "增量缓存"; items: root.algorithmCaches; emptyText: "暂无缓存层状态。"; tone: "moss" }
            SignalPanel { Layout.fillWidth: true; tokens: root.tokens; title: "阅读路径"; items: root.readingOrder; emptyText: "暂无推荐阅读路径。"; tone: "info" }
            SignalPanel { Layout.fillWidth: true; tokens: root.tokens; title: "结构热点"; items: root.hotspotSignals; emptyText: "暂无显著热点。"; tone: "warning" }
            SignalPanel { Layout.fillWidth: true; tokens: root.tokens; title: "关键风险"; items: root.riskSignals; emptyText: "暂无显著风险。"; tone: "danger" }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: root.analysisController.aiAvailable
            radius: root.tokens.radius8
            color: Qt.rgba(root.tokens.signalRaspberry.r, root.tokens.signalRaspberry.g, root.tokens.signalRaspberry.b, 0.07)
            border.color: Qt.rgba(root.tokens.signalRaspberry.r, root.tokens.signalRaspberry.g, root.tokens.signalRaspberry.b, 0.16)
            implicitHeight: aiColumn.implicitHeight + 28

            ColumnLayout {
                id: aiColumn
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                Label {
                    text: root.analysisController.aiBusy ? "AI 复盘 · 生成中" : "AI 复盘"
                    color: root.tokens.signalRaspberry
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: root.analysisController.aiBusy
                          ? "AI 正在结合当前报告、证据和结构上下文生成算法复盘。"
                          : (root.aiDigest().length > 0
                             ? root.aiDigest()
                             : "点击“AI 复盘”后，这里会给出面向比赛叙事的算法总结、风险边界和下一步补强建议。")
                    wrapMode: Text.WordWrap
                    color: root.tokens.text1
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 13
                    lineHeight: 1.18
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            MarkdownCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 660
                tokens: root.tokens
                title: "系统上下文"
                htmlText: root.contextHtml
                rawText: root.analysisController.systemContextReport
                emptyText: "运行重建后，这里会显示系统上下文报告。"
            }

            MarkdownCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 660
                tokens: root.tokens
                title: "工程报告"
                htmlText: root.reportHtml
                rawText: root.analysisController.analysisReport
                emptyText: "当前还没有生成工程分析报告。"
            }
        }
    }

    component StatusPill: Rectangle {
        required property QtObject tokens
        property string text: ""
        property string tone: "info"

        visible: text.length > 0
        radius: height / 2
        implicitHeight: 30
        implicitWidth: pillLabel.implicitWidth + 18
        color: tokens.toneSoft(tone)
        border.color: Qt.rgba(tokens.toneColor(tone).r, tokens.toneColor(tone).g, tokens.toneColor(tone).b, 0.24)

        Label {
            id: pillLabel
            anchors.centerIn: parent
            text: parent.text
            color: tokens.toneColor(tone)
            font.family: tokens.textFontFamily
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }
    }

    component ReportMetricCard: Rectangle {
        required property QtObject tokens
        property string title: ""
        property string value: ""
        property string detail: ""
        property string tone: "info"

        radius: tokens.radius8
        color: tokens.panelStrong
        border.color: Qt.rgba(tokens.toneColor(tone).r, tokens.toneColor(tone).g, tokens.toneColor(tone).b, 0.18)
        implicitHeight: metricColumn.implicitHeight + 24

        ColumnLayout {
            id: metricColumn
            anchors.fill: parent
            anchors.margins: 14
            spacing: 6

            Label { text: title; color: tokens.text3; font.family: tokens.textFontFamily; font.pixelSize: 11; font.weight: Font.DemiBold }
            Label { Layout.fillWidth: true; text: value; color: tokens.toneColor(tone); font.family: tokens.displayFontFamily; font.pixelSize: 22; font.weight: Font.DemiBold; wrapMode: Text.WordWrap }
            Label { Layout.fillWidth: true; text: detail; color: tokens.text2; font.family: tokens.textFontFamily; font.pixelSize: 12; wrapMode: Text.WordWrap; lineHeight: 1.16 }
        }
    }

    component StageCard: Rectangle {
        required property QtObject tokens
        property string title: ""
        property string kicker: ""
        property string value: ""
        property string summary: ""
        property string tone: "info"

        radius: tokens.radius8
        color: tokens.panelStrong
        border.color: Qt.rgba(tokens.toneColor(tone).r, tokens.toneColor(tone).g, tokens.toneColor(tone).b, 0.18)
        implicitHeight: stageLayout.implicitHeight + 24

        ColumnLayout {
            id: stageLayout
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

            Label { text: kicker; color: tokens.toneColor(tone); font.family: tokens.textFontFamily; font.pixelSize: 11; font.weight: Font.DemiBold }
            Label { Layout.fillWidth: true; text: title; color: tokens.text1; font.family: tokens.displayFontFamily; font.pixelSize: 17; font.weight: Font.DemiBold; wrapMode: Text.WordWrap }
            Label { Layout.fillWidth: true; text: value; color: tokens.text2; font.family: tokens.textFontFamily; font.pixelSize: 13; font.weight: Font.DemiBold }
            Label { Layout.fillWidth: true; text: summary; color: tokens.text3; font.family: tokens.textFontFamily; font.pixelSize: 12; wrapMode: Text.WordWrap; lineHeight: 1.16 }
        }
    }

    component SignalPanel: Rectangle {
        required property QtObject tokens
        property string title: ""
        property var items: []
        property string emptyText: ""
        property string tone: "info"

        function itemTitle(item) { return String((item || {}).title || (item || {}).label || "") }
        function itemValue(item) { return String((item || {}).value || "") }
        function itemBody(item) { return String((item || {}).detail || (item || {}).summary || (item || {}).body || "") }

        radius: tokens.radius8
        color: tokens.panelStrong
        border.color: Qt.rgba(tokens.toneColor(tone).r, tokens.toneColor(tone).g, tokens.toneColor(tone).b, 0.18)
        implicitHeight: panelColumn.implicitHeight + 26

        ColumnLayout {
            id: panelColumn
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

            Label { Layout.fillWidth: true; text: title; color: tokens.text1; font.family: tokens.displayFontFamily; font.pixelSize: 18; font.weight: Font.DemiBold }

            Repeater {
                model: items.length > 0 ? items : [{"title": "", "body": emptyText}]

                Rectangle {
                    required property var modelData

                    Layout.fillWidth: true
                    radius: tokens.radius6
                    color: Qt.rgba(tokens.toneSoft(tone).r, tokens.toneSoft(tone).g, tokens.toneSoft(tone).b, 0.48)
                    border.color: Qt.rgba(tokens.toneColor(tone).r, tokens.toneColor(tone).g, tokens.toneColor(tone).b, 0.16)
                    implicitHeight: entryColumn.implicitHeight + 18

                    ColumnLayout {
                        id: entryColumn
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Label { Layout.fillWidth: true; text: itemTitle(modelData); color: tokens.text1; font.family: tokens.textFontFamily; font.pixelSize: 12; font.weight: Font.DemiBold; wrapMode: Text.WordWrap; visible: text.length > 0 }
                            Label { text: itemValue(modelData); color: tokens.toneColor(tone); font.family: tokens.textFontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; visible: text.length > 0 }
                        }

                        Label { Layout.fillWidth: true; text: itemBody(modelData); color: tokens.text2; font.family: tokens.textFontFamily; font.pixelSize: 12; wrapMode: Text.WordWrap; lineHeight: 1.16 }
                    }
                }
            }
        }
    }

    component MarkdownCard: Rectangle {
        required property QtObject tokens
        property string title: ""
        property string htmlText: ""
        property string rawText: ""
        property string emptyText: ""

        radius: tokens.radius8
        color: tokens.panelStrong
        border.color: tokens.border1

        Label { anchors.left: parent.left; anchors.top: parent.top; anchors.leftMargin: 16; anchors.topMargin: 16; text: title; color: tokens.text1; font.family: tokens.displayFontFamily; font.pixelSize: 18; font.weight: Font.DemiBold }

        ActionButton {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 16
            anchors.topMargin: 12
            tokens: parent.tokens
            text: "复制原文"
            tone: "secondary"
            compact: true
            enabled: rawText.length > 0
            onClicked: root.analysisController.copyTextToClipboard(rawText)
        }

        Flickable {
            id: paneFlick
            anchors.fill: parent
            anchors.margins: 16
            anchors.topMargin: 54
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            contentWidth: width
            contentHeight: textFrame.height

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Item {
                id: textFrame
                width: paneFlick.width
                height: Math.max(paneFlick.height, markdownText.contentHeight + 28)

                TextEdit {
                    id: markdownText
                    x: 0
                    y: 0
                    width: Math.max(0, textFrame.width)
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
