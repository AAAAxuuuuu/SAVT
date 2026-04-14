import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../foundation"

Rectangle {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState

    color: tokens.panelBase
    border.color: tokens.border1

    readonly property var node: focusState.focusedNode || ({})
    readonly property var evidence: node.evidence || ({})
    readonly property var rawFacts: evidence.facts || []
    readonly property var rawRules: evidence.rules || []
    readonly property var rawConclusions: evidence.conclusions || []
    readonly property var rawSourceFiles: evidence.sourceFiles || node.exampleFiles || []
    readonly property string kindLabel: displayKind(node.kind || "")
    readonly property string roleLabel: displayRole(node.role || "")
    readonly property int fileCount: Number(node.fileCount || node.sourceFileCount || 0)
    readonly property string summaryText: cleanText(node.summary || node.responsibility)
    readonly property string conclusionText: localizedConclusionText()
    readonly property var factRows: buildFactRows()
    readonly property var ruleRows: buildRuleRows()
    readonly property var sourceFiles: dedupeClean(rawSourceFiles).slice(0, 4)
    property var aiCache: ({})
    property string activeAiNodeKey: ""
    readonly property string currentNodeKey: currentNodeCacheKey()
    readonly property var currentAiEntry: currentNodeKey.length > 0 && aiCache[currentNodeKey] ? aiCache[currentNodeKey] : null
    readonly property bool aiBusyForCurrentNode: analysisController.aiBusy
                                                  && activeAiNodeKey.length > 0
                                                  && activeAiNodeKey === currentNodeKey
    readonly property bool aiHasCachedResultForCurrentNode: !!currentAiEntry
    readonly property string aiCardText: currentAiCardText()
    readonly property bool showAiCard: aiBusyForCurrentNode || aiHasCachedResultForCurrentNode

    function cleanText(value) {
        return String(value || "")
                .replace(/\uFFFD/g, "")
                .replace(/[\u25CF\u25A0\uFE0F]/g, "")
                .replace(/[\u0000-\u0008\u000B-\u001F]/g, " ")
                .replace(/\s+/g, " ")
                .trim()
    }

    function dedupeClean(values) {
        var output = []
        var seen = {}
        var list = values || []
        for (var index = 0; index < list.length; ++index) {
            var text = cleanText(list[index])
            if (!text.length || seen[text])
                continue
            seen[text] = true
            output.push(text)
        }
        return output
    }

    function displayKind(kind) {
        var key = cleanText(kind).toLowerCase()
        if (key === "entry")
            return "入口能力"
        if (key === "entry_component")
            return "入口组件"
        if (key === "capability")
            return "能力域"
        if (key === "service")
            return "服务模块"
        if (key === "infrastructure")
            return "基础设施"
        if (key === "support_component")
            return "支撑组件"
        return key.length ? cleanText(kind) : "架构节点"
    }

    function displayRole(role) {
        var key = cleanText(role).toLowerCase()
        if (key === "analysis")
            return "分析层"
        if (key === "core")
            return "核心层"
        if (key === "support")
            return "支撑层"
        if (key === "infrastructure")
            return "基础设施"
        if (key === "presentation")
            return "展示层"
        if (key === "visual")
            return "可视层"
        if (key === "experience")
            return "交互层"
        if (key === "entry")
            return "入口"
        return key.length ? cleanText(role) : ""
    }

    function replaceCommonTerms(text) {
        var output = cleanText(text)
        output = output.replace(/\bcapability\b/gi, "能力域")
        output = output.replace(/\banalysis\b/gi, "分析")
        output = output.replace(/\bcore\b/gi, "核心")
        output = output.replace(/\binfrastructure\b/gi, "基础设施")
        output = output.replace(/\bservice\b/gi, "服务")
        output = output.replace(/\bentry\b/gi, "入口")
        output = output.replace(/\brole\b/gi, "角色")
        output = output.replace(/\boverview nodes\b/gi, "总览节点")
        output = output.replace(/\bbusiness-facing capabilities\b/gi, "面向业务的能力域")
        return output
    }

    function localizeEvidenceText(value) {
        var text = cleanText(value)
        if (!text.length)
            return ""

        var capabilityMatch = text.match(/^Capability kind:\s*([^,]+),\s*dominant role:\s*([^.]+)\.?$/i)
        if (capabilityMatch)
            return "节点类型：" + displayKind(capabilityMatch[1]) + "；主导角色：" + displayRole(capabilityMatch[2]) + "。"

        var aggregateMatch = text.match(/^Aggregates\s+(\d+)\s+module\(s\)\s+and\s+(\d+)\s+file\(s\);\s*stage span\s+(\d+)\.?$/i)
        if (aggregateMatch)
            return "聚合了 " + aggregateMatch[1] + " 个模块、" + aggregateMatch[2] + " 个文件；阶段跨度 " + aggregateMatch[3] + "。"

        var folderMatch = text.match(/^Folder scope hint:\s*(.+?)\.?$/i)
        if (folderMatch)
            return "目录范围提示：" + replaceCommonTerms(folderMatch[1]) + "。"

        var aggregationRule = text.match(/^Aggregation rule:\s*(.+)$/i)
        if (aggregationRule)
            return "归并规则：" + replaceCommonTerms(aggregationRule[1])

        var kindRule = text.match(/^Kind rule:\s*(.+)$/i)
        if (kindRule)
            return "类型规则：" + replaceCommonTerms(kindRule[1])

        var roleRule = text.match(/^Role bucketing rule:\s*(.+)$/i)
        if (roleRule)
            return "角色归桶规则：" + replaceCommonTerms(roleRule[1])

        return replaceCommonTerms(text)
    }

    function localizePath(value) {
        return cleanText(value).replace(/\\/g, "/")
    }

    function currentNodeCacheKey() {
        return node && node.id !== undefined ? String(node.id) : ""
    }

    function localizedConclusionText() {
        if (rawConclusions.length > 0)
            return localizeEvidenceText(rawConclusions[0])
        if (summaryText.length > 0)
            return summaryText
        return "当前对象已经选中，但还缺少更完整的结论说明。"
    }

    function buildFactRows() {
        var rows = []
        var localizedFacts = dedupeClean(rawFacts).map(localizeEvidenceText)
        for (var index = 0; index < localizedFacts.length && rows.length < 3; ++index)
            rows.push(localizedFacts[index])

        if (rows.length === 0) {
            rows.push("文件数：" + fileCount)
            rows.push(roleLabel.length > 0 ? ("当前角色：" + roleLabel) : ("节点类型：" + kindLabel))
            rows.push(summaryText.length > 0 ? summaryText : "当前对象还没有额外摘要。")
        }
        return rows.slice(0, 3)
    }

    function buildRuleRows() {
        var rows = dedupeClean(rawRules).map(localizeEvidenceText).slice(0, 3)
        if (rows.length === 0)
            rows.push(roleLabel.length > 0 ? ("当前角色判断：" + roleLabel) : "当前对象还没有额外规则说明。")
        return rows
    }

    function buildAiDisplayText(summary, responsibility) {
        var sections = []
        var summaryText = cleanText(summary)
        var responsibilityText = cleanText(responsibility)
        if (summaryText.length > 0)
            sections.push(summaryText)
        if (responsibilityText.length > 0)
            sections.push(responsibilityText)
        return sections.join("\n\n")
    }

    function cacheCurrentAiResult() {
        if (activeAiNodeKey.length === 0 || analysisController.aiBusy || !analysisController.aiHasResult)
            return

        var cachedText = buildAiDisplayText(
                    analysisController.aiSummary,
                    analysisController.aiResponsibility)
        if (cachedText.length === 0)
            return

        var nextCache = Object.assign({}, aiCache || ({}))
        nextCache[activeAiNodeKey] = {
            "text": cachedText
        }
        aiCache = nextCache
    }

    function clearAiCache() {
        aiCache = ({})
        activeAiNodeKey = ""
    }

    function currentAiCardText() {
        if (aiBusyForCurrentNode) {
            var statusMessage = cleanText(analysisController.aiStatusMessage)
            return statusMessage.length > 0
                   ? statusMessage
                   : "正在结合当前节点的证据、规则和关键文件生成更完整的中文解读，请稍候。"
        }
        if (currentAiEntry && currentAiEntry.text !== undefined)
            return cleanText(currentAiEntry.text)
        return ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 122

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                anchors.topMargin: 18
                anchors.bottomMargin: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        Layout.fillWidth: true
                        text: cleanText(node.name) || "未命名节点"
                        color: root.tokens.text1
                        elide: Text.ElideRight
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 22
                        font.weight: Font.DemiBold
                    }

                    ActionButton {
                        tokens: root.tokens
                        text: "X"
                        compact: true
                        square: true
                        fixedWidth: 30
                        tone: "ghost"
                        onClicked: root.focusState.inspectorOpen = false
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: [kindLabel, roleLabel].filter(function(item, index, list) {
                        return item.length > 0 && list.indexOf(item) === index
                    }).join(" · ")
                    color: root.tokens.text3
                    elide: Text.ElideRight
                    visible: text.length > 0
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 13
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    ChipBadge {
                        tokens: root.tokens
                        visible: root.kindLabel.length > 0
                        text: root.kindLabel
                        tone: "brand"
                    }

                    ChipBadge {
                        tokens: root.tokens
                        visible: root.roleLabel.length > 0 && root.roleLabel !== root.kindLabel
                        text: root.roleLabel
                        tone: "neutral"
                    }

                    ChipBadge {
                        tokens: root.tokens
                        text: root.fileCount + " 个文件"
                        tone: "success"
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.tokens.border1
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 14

                AccentCard {
                    Layout.topMargin: 18
                    Layout.leftMargin: 18
                    Layout.rightMargin: 18
                    Layout.fillWidth: true
                    tokens: root.tokens
                    title: "模块概览"
                    body: root.summaryText.length > 0
                          ? root.summaryText
                          : "选中一个架构对象后，这里会优先给出简洁摘要，帮助你快速判断它是什么。"
                    tone: "neutral"
                }

                AccentCard {
                    Layout.leftMargin: 18
                    Layout.rightMargin: 18
                    Layout.fillWidth: true
                    tokens: root.tokens
                    title: "当前判断"
                    body: root.conclusionText
                    tone: "brand"
                }

                AccentCard {
                    Layout.leftMargin: 18
                    Layout.rightMargin: 18
                    Layout.fillWidth: true
                    visible: root.showAiCard
                    tokens: root.tokens
                    title: root.analysisController.aiBusy ? "AI 辅助解读 · 生成中" : "AI 辅助解读"
                    body: root.aiCardText
                    tone: "ai"
                }

                Rectangle {
                    Layout.leftMargin: 18
                    Layout.rightMargin: 18
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    visible: !root.showAiCard && !!root.focusState.focusedNode
                    radius: root.tokens.radius8
                    color: Qt.rgba(1, 1, 1, 0.92)
                    border.color: root.tokens.border1

                    ActionButton {
                        anchors.centerIn: parent
                        width: parent.width - 28
                        tokens: root.tokens
                        text: root.analysisController.aiBusy ? "AI 正在生成..." : "请求 AI 辅助解读"
                        tone: "primary"
                        enabled: root.analysisController.aiAvailable
                                 && !!root.focusState.focusedNode
                                 && !root.analysisController.aiBusy
                        onClicked: {
                            root.activeAiNodeKey = root.currentNodeKey
                            root.analysisController.requestAiExplanation(
                                        root.focusState.focusedNode,
                                        "请基于当前节点的证据链，生成一份更完整的中文解读。"
                                        + " 输出请使用简体中文，整体尽量写到约 200 到 300 字。"
                                        + " 请至少覆盖：这个模块是什么、负责什么、不负责什么、为什么值得现在阅读、应该先看哪些文件、有哪些风险或注意点。"
                                        + " 内容可以适当展开，不要只给一两句；如果证据不足，要明确说明证据缺口。")
                        }
                    }
                }

                SectionBlock {
                    Layout.fillWidth: true
                    Layout.leftMargin: 18
                    Layout.rightMargin: 18
                    tokens: root.tokens
                    title: "结构事实"
                    rows: root.factRows
                }

                SectionBlock {
                    Layout.fillWidth: true
                    Layout.leftMargin: 18
                    Layout.rightMargin: 18
                    tokens: root.tokens
                    title: "判断依据"
                    rows: root.ruleRows
                }

                SectionBlock {
                    Layout.fillWidth: true
                    Layout.leftMargin: 18
                    Layout.rightMargin: 18
                    Layout.bottomMargin: 16
                    tokens: root.tokens
                    title: "关键文件"
                    rows: root.sourceFiles.length > 0 ? root.sourceFiles.map(localizePath) : ["暂无源文件线索。"]
                    mono: true
                }
            }
        }

    }

    Connections {
        target: root.analysisController

        function onAiSummaryChanged() { root.cacheCurrentAiResult() }
        function onAiResponsibilityChanged() { root.cacheCurrentAiResult() }
        function onCapabilitySceneChanged() { root.clearAiCache() }
        function onProjectRootPathChanged() { root.clearAiCache() }
    }

    component ChipBadge: Rectangle {
        required property QtObject tokens
        required property string text
        property string tone: "neutral"

        radius: height / 2
        implicitHeight: 26
        implicitWidth: chipLabel.implicitWidth + 16
        color: tone === "brand"
               ? Qt.rgba(0.00, 0.48, 1.00, 0.10)
               : (tone === "success"
                  ? Qt.rgba(0.20, 0.78, 0.34, 0.12)
                  : Qt.rgba(0.12, 0.18, 0.28, 0.06))
        border.color: tone === "brand"
                      ? Qt.rgba(0.00, 0.48, 1.00, 0.20)
                      : (tone === "success"
                         ? Qt.rgba(0.20, 0.78, 0.34, 0.18)
                         : tokens.border1)

        Label {
            id: chipLabel
            anchors.centerIn: parent
            text: parent.text
            color: tone === "brand"
                   ? root.tokens.signalCobalt
                   : (tone === "success" ? root.tokens.signalTeal : root.tokens.text2)
            font.family: root.tokens.textFontFamily
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }
    }

    component AccentCard: Rectangle {
        required property QtObject tokens
        property string title: ""
        property string body: ""
        property string tone: "neutral"

        radius: tokens.radius8
        color: tone === "brand"
               ? Qt.rgba(0.00, 0.48, 1.00, 0.08)
               : (tone === "ai"
                  ? Qt.rgba(0.69, 0.32, 0.87, 0.08)
                  : Qt.rgba(1, 1, 1, 0.92))
        border.color: tone === "brand"
                      ? Qt.rgba(0.00, 0.48, 1.00, 0.18)
                      : (tone === "ai"
                         ? Qt.rgba(0.69, 0.32, 0.87, 0.18)
                         : tokens.border1)
        implicitHeight: contentColumn.implicitHeight + 28

        ColumnLayout {
            id: contentColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            anchors.topMargin: 12
            spacing: 6

            Label {
                text: title
                color: tone === "brand"
                       ? root.tokens.signalCobalt
                       : (tone === "ai" ? root.tokens.signalRaspberry : root.tokens.text2)
                font.family: root.tokens.textFontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                text: body
                wrapMode: Text.WordWrap
                color: root.tokens.text1
                font.family: root.tokens.textFontFamily
                font.pixelSize: 13
                lineHeight: 1.2
            }
        }
    }

    component SectionBlock: ColumnLayout {
        required property QtObject tokens
        property string title: ""
        property var rows: []
        property bool mono: false

        spacing: 8

        Label {
            text: title
            color: tokens.text3
            font.family: tokens.textFontFamily
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: rowsColumn.implicitHeight + 8
            radius: tokens.radius8
            color: Qt.rgba(1, 1, 1, 0.92)
            border.color: tokens.border1

            ColumnLayout {
                id: rowsColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.topMargin: 4
                spacing: 0

                Repeater {
                    model: rows

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.max(44, rowValue.implicitHeight + 20)

                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.topMargin: 15
                            width: 6
                            height: 6
                            radius: width / 2
                            color: mono ? tokens.text3 : root.tokens.signalCobalt
                        }

                        Label {
                            id: rowValue
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 16
                            anchors.rightMargin: 4
                            text: root.cleanText(modelData)
                            wrapMode: Text.WordWrap
                            color: tokens.text1
                            font.family: mono ? tokens.monoFontFamily : tokens.textFontFamily
                            font.pixelSize: mono ? 11 : 13
                            font.weight: mono ? Font.Normal : Font.Medium
                            lineHeight: 1.18
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: tokens.border1
                            visible: index < rows.length - 1
                        }
                    }
                }
            }
        }
    }
}
