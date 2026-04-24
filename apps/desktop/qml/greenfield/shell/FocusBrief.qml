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
    property bool embeddedMode: false

    radius: tokens.radiusXxl + 4
    color: tokens.panelSoft
    border.color: tokens.border1
    clip: true

    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.76) }
        GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.56) }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.color: root.tokens.shineBorder
        border.width: 1
    }

    readonly property var node: focusState.focusedNode || ({})
    readonly property var evidence: node.evidence || ({})
    readonly property var rawFacts: evidence.facts || []
    readonly property var rawRules: evidence.rules || []
    readonly property var rawConclusions: evidence.conclusions || []
    readonly property var rawSourceFiles: evidence.sourceFiles || node.exampleFiles || []
    property var fileDetail: ({})
    readonly property bool singleFileMode: !!fileDetail && !!fileDetail.available && !!fileDetail.singleFile
    readonly property string kindLabel: singleFileMode ? "具体文件" : displayKind(node.kind || "")
    readonly property string roleLabel: singleFileMode
                                         ? cleanText(fileDetail.roleLabel || displayRole(node.role || ""))
                                         : displayRole(node.role || "")
    readonly property int fileCount: Number(node.fileCount || node.sourceFileCount || 0)
    readonly property string summaryText: singleFileMode
                                          ? cleanText(fileDetail.summary)
                                          : cleanText(node.summary || node.responsibility)
    readonly property string conclusionText: localizedConclusionText()
    readonly property var factRows: buildFactRows()
    readonly property var ruleRows: buildRuleRows()
    readonly property var sourceFiles: singleFileMode
                                       ? dedupeClean([fileDetail.filePath || ""]).slice(0, 1)
                                       : dedupeClean(rawSourceFiles).slice(0, 4)
    readonly property var fileImportRows: dedupeClean(fileDetail.importClues || []).slice(0, 5)
    readonly property var fileDeclarationRows: dedupeClean(fileDetail.declarationClues || []).slice(0, 5)
    readonly property var fileBehaviorRows: dedupeClean(fileDetail.behaviorSignals || []).slice(0, 5)
    readonly property var fileReadingRows: dedupeClean(fileDetail.readingHints || []).slice(0, 5)
    readonly property string titleText: singleFileMode
                                        ? cleanText(fileDetail.fileName || node.name)
                                        : (cleanText(node.name) || "未命名节点")
    readonly property string subtitleText: joinUnique(singleFileMode
                                                      ? [cleanText(fileDetail.categoryLabel || "具体文件"),
                                                         cleanText(fileDetail.languageLabel || ""),
                                                         roleLabel]
                                                      : [kindLabel, roleLabel])
    readonly property string detailPathText: singleFileMode ? localizePath(fileDetail.filePath || "") : ""
    readonly property string expectedAiScope: singleFileMode
                                              ? "file_node"
                                              : ((node && node.capabilityId !== undefined)
                                                 ? "component_node"
                                                 : "capability_map")
    property var aiCache: ({})
    property string activeAiNodeKey: ""
    readonly property string currentNodeKey: currentNodeCacheKey()
    readonly property var currentAiEntry: currentNodeKey.length > 0 && aiCache[currentNodeKey] ? aiCache[currentNodeKey] : null
    readonly property bool aiTargetMatchesCurrentNode: targetMatchesCurrentNode()
    readonly property bool aiBusyForCurrentNode: analysisController.aiBusy
                                                  && (activeAiNodeKey === currentNodeKey
                                                      || aiTargetMatchesCurrentNode)
    readonly property bool aiHasCachedResultForCurrentNode: !!currentAiEntry
                                                            || (analysisController.aiHasResult
                                                                && aiTargetMatchesCurrentNode)
    readonly property string aiCardText: currentAiCardText()
    readonly property bool showAiCard: aiBusyForCurrentNode || aiHasCachedResultForCurrentNode
    readonly property int detailColumns: embeddedMode && width >= 1120 ? 2 : 1

    function cleanText(value) {
        return String(value || "")
                .replace(/\uFFFD/g, "")
                .replace(/[\u25CF\u25A0\uFE0F]/g, "")
                .replace(/[\u0000-\u0008\u000B-\u001F]/g, " ")
                .replace(/\s+/g, " ")
                .trim()
    }

    function cleanMultilineText(value) {
        return String(value || "")
                .replace(/\uFFFD/g, "")
                .replace(/[\u0000-\u0008\u000B-\u001F]/g, " ")
                .replace(/\r/g, "")
                .trim()
    }

    function normalizedText(value) {
        return cleanText(value).toLowerCase()
    }

    function joinUnique(values) {
        var items = []
        var seen = {}
        var list = values || []
        for (var index = 0; index < list.length; ++index) {
            var text = cleanText(list[index])
            if (!text.length || seen[text])
                continue
            seen[text] = true
            items.push(text)
        }
        return items.join(" · ")
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

    function refreshFileDetail() {
        if (!root.node || root.node.id === undefined) {
            root.fileDetail = ({})
            return
        }
        root.fileDetail = root.analysisController.describeFileNode(root.node)
    }

    function currentNodeCacheKey() {
        return node && node.id !== undefined ? (expectedAiScope + ":" + String(node.id)) : ""
    }

    function aiScopeMatchesCurrentNode() {
        var scope = cleanText(root.analysisController.aiScope)
        return scope.length > 0 && scope === root.expectedAiScope
    }

    function targetMatchesCurrentNode() {
        if (!aiScopeMatchesCurrentNode())
            return false

        var aiName = normalizedText(root.analysisController.aiNodeName)
        if (!aiName.length || !root.node || root.node.id === undefined)
            return false

        var candidates = [normalizedText(root.node.name)]
        if (root.singleFileMode) {
            candidates.push(normalizedText(root.fileDetail.fileName))
            candidates.push(normalizedText(root.fileDetail.filePath))
        }

        for (var index = 0; index < candidates.length; ++index) {
            var candidate = candidates[index]
            if (!candidate.length)
                continue
            if (candidate === aiName || candidate.indexOf(aiName) >= 0 || aiName.indexOf(candidate) >= 0)
                return true
        }
        return false
    }

    function localizedConclusionText() {
        if (singleFileMode) {
            if (fileReadingRows.length > 0)
                return fileReadingRows[0]
            if (summaryText.length > 0)
                return summaryText
            return "当前文件已选中，但还没有形成足够稳定的职责判断。"
        }
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
        if (analysisController.aiBusy || !analysisController.aiHasResult || !aiTargetMatchesCurrentNode)
            return

        var targetKey = currentNodeKey.length > 0 ? currentNodeKey : activeAiNodeKey
        if (targetKey.length === 0)
            return

        var cachedText = buildAiDisplayText(
                    analysisController.aiSummary,
                    analysisController.aiResponsibility)
        if (cachedText.length === 0)
            return

        var nextCache = Object.assign({}, aiCache || ({}))
        nextCache[targetKey] = {
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
                   : (singleFileMode
                      ? "正在结合当前文件的路径、声明和关键片段生成更完整的中文解读，请稍候。"
                      : "正在结合当前节点的证据、规则和关键文件生成更完整的中文解读，请稍候。")
        }
        if (!currentAiEntry && analysisController.aiHasResult && aiTargetMatchesCurrentNode)
            return buildAiDisplayText(analysisController.aiSummary, analysisController.aiResponsibility)
        if (currentAiEntry && currentAiEntry.text !== undefined)
            return cleanText(currentAiEntry.text)
        return ""
    }

    function requestAiInterpretation() {
        if (!root.focusState.focusedNode)
            return

        root.activeAiNodeKey = root.currentNodeKey
        root.analysisController.requestAiExplanation(
                    root.focusState.focusedNode,
                    root.singleFileMode
                    ? ("请只围绕当前文件进行中文解读。"
                       + " 请结合文件路径、导入线索、声明符号、行为信号和关键片段来说明："
                       + " 这个文件的核心职责、它依赖谁或被谁依赖、建议先看哪些符号、有哪些风险或不确定性。"
                       + " 不要把整个能力域当成主体，不要给通用模块套话。")
                    : ("请基于当前节点的证据链，生成一份更完整的中文解读。"
                       + " 输出请使用简体中文，整体尽量写到约 200 到 300 字。"
                       + " 请至少覆盖：这个模块是什么、负责什么、不负责什么、为什么值得现在阅读、应该先看哪些文件、有哪些风险或注意点。"
                       + " 内容可以适当展开，不要只给一两句；如果证据不足，要明确说明证据缺口。"))
    }

    onNodeChanged: {
        activeAiNodeKey = ""
        refreshFileDetail()
    }
    Component.onCompleted: refreshFileDetail()

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 16

            Rectangle {
                Layout.topMargin: 18
                Layout.leftMargin: 18
                Layout.rightMargin: 18
                Layout.fillWidth: true
                radius: root.tokens.radiusXl
                color: Qt.rgba(1, 1, 1, 0.82)
                border.color: root.tokens.border1
                implicitHeight: headerColumn.implicitHeight + 32

                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: "transparent"
                    border.color: root.tokens.shineBorder
                    border.width: 1
                }

                ColumnLayout {
                    id: headerColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    anchors.topMargin: 16
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                Layout.fillWidth: true
                                text: root.embeddedMode ? "证据钻取" : "边界解读"
                                color: root.tokens.text3
                                font.family: root.tokens.textFontFamily
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.titleText
                                color: root.tokens.text1
                                wrapMode: Text.WordWrap
                                maximumLineCount: root.embeddedMode ? 2 : 3
                                elide: Text.ElideRight
                                font.family: root.tokens.displayFontFamily
                                font.pixelSize: root.embeddedMode ? 24 : 22
                                font.weight: Font.DemiBold
                            }
                        }

                        ActionButton {
                            tokens: root.tokens
                            text: root.embeddedMode ? "返回组件图" : "X"
                            hint: root.embeddedMode
                                  ? "退出当前详情，回到组件关系图。"
                                  : "关闭右侧焦点面板。"
                            compact: true
                            square: !root.embeddedMode
                            fixedWidth: root.embeddedMode ? 0 : 30
                            tone: root.embeddedMode ? "secondary" : "ghost"
                            Layout.alignment: Qt.AlignTop
                            onClicked: {
                                if (root.embeddedMode) {
                                    root.focusState.clearNodeFocus()
                                } else {
                                    root.focusState.inspectorOpen = false
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.subtitleText
                        color: root.tokens.text3
                        wrapMode: Text.WordWrap
                        visible: text.length > 0
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 13
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.detailPathText
                        visible: root.singleFileMode && text.length > 0
                        wrapMode: Text.WrapAnywhere
                        color: root.tokens.text3
                        font.family: root.tokens.monoFontFamily
                        font.pixelSize: 11
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        ChipBadge {
                            tokens: root.tokens
                            visible: root.kindLabel.length > 0
                            text: root.singleFileMode ? cleanText(root.fileDetail.languageLabel || "具体文件") : root.kindLabel
                            tone: "brand"
                        }

                        ChipBadge {
                            tokens: root.tokens
                            visible: root.singleFileMode
                                     ? cleanText(root.fileDetail.categoryLabel || "").length > 0
                                     : (root.roleLabel.length > 0 && root.roleLabel !== root.kindLabel)
                            text: root.singleFileMode ? cleanText(root.fileDetail.categoryLabel || "") : root.roleLabel
                            tone: "neutral"
                        }

                        ChipBadge {
                            tokens: root.tokens
                            visible: root.singleFileMode
                                     ? cleanText(root.fileDetail.roleLabel || "").length > 0
                                     : true
                            text: root.singleFileMode ? cleanText(root.fileDetail.roleLabel || "") : (root.fileCount + " 个文件")
                            tone: root.singleFileMode ? "neutral" : "success"
                        }

                        ChipBadge {
                            tokens: root.tokens
                            visible: root.singleFileMode
                            text: "总行数 " + Number(root.fileDetail.lineCount || 0)
                            tone: "success"
                        }
                    }
                }
            }

            Loader {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: 18
                Layout.rightMargin: 18
                Layout.bottomMargin: 18
                sourceComponent: root.singleFileMode ? fileModeContent : moduleModeContent
            }
        }
    }

    Component {
        id: moduleModeContent

        ColumnLayout {
            width: parent ? parent.width : 0
            spacing: 16

            GridLayout {
                width: parent ? parent.width : 0
                columns: root.detailColumns
                columnSpacing: 14
                rowSpacing: 14

                AccentCard {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    tokens: root.tokens
                    title: "模块概览"
                    body: root.summaryText.length > 0
                          ? root.summaryText
                          : "当前组件已选中，但还没有形成稳定的模块摘要。"
                    tone: "neutral"
                }

                AccentCard {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    tokens: root.tokens
                    title: "边界结论"
                    body: root.conclusionText
                    tone: "brand"
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: root.tokens.radiusXl
                color: Qt.rgba(root.tokens.signalRaspberry.r, root.tokens.signalRaspberry.g, root.tokens.signalRaspberry.b, 0.07)
                border.color: Qt.rgba(root.tokens.signalRaspberry.r, root.tokens.signalRaspberry.g, root.tokens.signalRaspberry.b, 0.16)
                implicitHeight: aiColumn.implicitHeight + 26

                ColumnLayout {
                    id: aiColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    anchors.topMargin: 14
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: root.analysisController.aiBusy ? "AI 辅助解读 · 生成中" : "AI 辅助解读"
                            color: root.tokens.signalRaspberry
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Item { Layout.fillWidth: true }

                        ActionButton {
                            tokens: root.tokens
                            text: root.analysisController.aiBusy ? "生成中..." : "生成解读"
                            hint: "让 AI 解释当前选中节点的职责、证据和下一步排查方向。"
                            disabledHint: root.analysisController.aiBusy
                                          ? "AI 正在生成解读，请稍等。"
                                          : "需要先选中节点，并确保 AI 服务可用。"
                            tone: "ai"
                            compact: true
                            enabled: root.analysisController.aiAvailable
                                     && !!root.focusState.focusedNode
                                     && !root.analysisController.aiBusy
                            onClicked: root.requestAiInterpretation()
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.showAiCard
                              ? root.aiCardText
                              : "这里会围绕当前组件的职责、边界、风险和建议阅读路径生成一段中文解读。"
                        wrapMode: Text.WordWrap
                        color: root.tokens.text1
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 13
                        lineHeight: 1.22
                    }
                }
            }

            GridLayout {
                width: parent ? parent.width : 0
                columns: root.detailColumns
                columnSpacing: 14
                rowSpacing: 14

                SectionBlock {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    tokens: root.tokens
                    title: "结构证据"
                    rows: root.factRows
                }

                SectionBlock {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    tokens: root.tokens
                    title: "规则依据"
                    rows: root.ruleRows
                }
            }

            SectionBlock {
                width: parent ? parent.width : 0
                tokens: root.tokens
                title: "关键证据文件"
                rows: root.sourceFiles.length > 0 ? root.sourceFiles.map(localizePath) : ["暂无源文件线索。"]
                mono: true
            }
        }
    }

    Component {
        id: fileModeContent

        ColumnLayout {
            width: parent ? parent.width : 0
            spacing: 16

            GridLayout {
                width: parent ? parent.width : 0
                columns: 2
                columnSpacing: 14
                rowSpacing: 14

                AccentCard {
                    Layout.fillWidth: true
                    tokens: root.tokens
                    title: "文件定位"
                    body: root.summaryText.length > 0
                          ? root.summaryText
                          : "当前文件已选中，但还没有稳定的文件定位摘要。"
                    tone: "neutral"
                }

                AccentCard {
                    Layout.fillWidth: true
                    tokens: root.tokens
                    title: "文件结论"
                    body: root.conclusionText
                    tone: "brand"
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: root.tokens.radiusXl
                color: Qt.rgba(root.tokens.signalRaspberry.r, root.tokens.signalRaspberry.g, root.tokens.signalRaspberry.b, 0.07)
                border.color: Qt.rgba(root.tokens.signalRaspberry.r, root.tokens.signalRaspberry.g, root.tokens.signalRaspberry.b, 0.16)
                implicitHeight: fileAiColumn.implicitHeight + 26

                ColumnLayout {
                    id: fileAiColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    anchors.topMargin: 14
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: root.aiBusyForCurrentFile ? "AI 文件解读 · 生成中" : "AI 文件解读"
                            color: root.tokens.signalRaspberry
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Item { Layout.fillWidth: true }

                        ActionButton {
                            tokens: root.tokens
                            text: root.analysisController.aiBusy ? "生成中..." : "生成解读"
                            hint: "让 AI 解释当前选中节点的职责、证据和下一步排查方向。"
                            disabledHint: root.analysisController.aiBusy
                                          ? "AI 正在生成解读，请稍等。"
                                          : "需要先选中节点，并确保 AI 服务可用。"
                            tone: "ai"
                            compact: true
                            enabled: root.analysisController.aiAvailable
                                     && !!root.focusState.focusedNode
                                     && !root.analysisController.aiBusy
                            onClicked: root.requestAiInterpretation()
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.aiBusyForCurrentFile
                              ? (root.cleanText(root.analysisController.aiStatusMessage).length > 0
                                 ? root.cleanText(root.analysisController.aiStatusMessage)
                                 : "正在结合当前文件的路径、声明和关键片段生成解读，请稍候。")
                              : (root.aiResultForCurrentFile
                                 ? root.aiDigestText()
                                 : "这里会只围绕当前文件生成中文解读，不再泛泛解释整个模块。")
                        wrapMode: Text.WordWrap
                        color: root.tokens.text1
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 13
                        lineHeight: 1.22
                    }
                }
            }

            GridLayout {
                width: parent ? parent.width : 0
                columns: 2
                columnSpacing: 14
                rowSpacing: 14

                SectionBlock {
                    Layout.fillWidth: true
                    tokens: root.tokens
                    title: "导入 / 依赖线索"
                    rows: root.fileImportRows.length > 0 ? root.fileImportRows : ["当前文件还没有抽取到明显的导入线索。"]
                }

                SectionBlock {
                    Layout.fillWidth: true
                    tokens: root.tokens
                    title: "声明 / 主符号"
                    rows: root.fileDeclarationRows.length > 0 ? root.fileDeclarationRows : ["当前文件还没有抽取到稳定的声明符号。"]
                }

                SectionBlock {
                    Layout.fillWidth: true
                    tokens: root.tokens
                    title: "行为信号"
                    rows: root.fileBehaviorRows.length > 0 ? root.fileBehaviorRows : ["当前文件还没有抽取到明显的行为信号。"]
                }

                SectionBlock {
                    Layout.fillWidth: true
                    tokens: root.tokens
                    title: "建议阅读顺序"
                    rows: root.fileReadingRows.length > 0 ? root.fileReadingRows : ["建议先从导入、主符号和首个非空代码片段开始。"]
                }
            }

            AccentCard {
                width: parent ? parent.width : 0
                visible: root.cleanMultilineText(root.fileDetail.previewText).length > 0
                tokens: root.tokens
                title: "关键片段"
                body: root.cleanMultilineText(root.fileDetail.previewText)
                bodyMono: true
                tone: "neutral"
            }
        }
    }

    Connections {
        target: root.analysisController

        function onAiSummaryChanged() { root.cacheCurrentAiResult() }
        function onAiResponsibilityChanged() { root.cacheCurrentAiResult() }
        function onCapabilitySceneChanged() { root.clearAiCache() }
        function onProjectRootPathChanged() {
            root.clearAiCache()
            root.refreshFileDetail()
        }
    }

    component ChipBadge: Rectangle {
        required property QtObject tokens
        required property string text
        property string tone: "neutral"

        radius: height / 2
        implicitHeight: 26
        implicitWidth: chipLabel.implicitWidth + 16
        color: tone === "brand"
               ? Qt.rgba(root.tokens.signalCobalt.r, root.tokens.signalCobalt.g, root.tokens.signalCobalt.b, 0.12)
               : (tone === "success"
                  ? Qt.rgba(root.tokens.signalTeal.r, root.tokens.signalTeal.g, root.tokens.signalTeal.b, 0.14)
                  : Qt.rgba(1, 1, 1, 0.62))
        border.color: tone === "brand"
                      ? Qt.rgba(root.tokens.signalCobalt.r, root.tokens.signalCobalt.g, root.tokens.signalCobalt.b, 0.24)
                      : (tone === "success"
                         ? Qt.rgba(root.tokens.signalTeal.r, root.tokens.signalTeal.g, root.tokens.signalTeal.b, 0.22)
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
        property bool bodyMono: false

        radius: tokens.radius8
        color: tone === "brand"
               ? Qt.rgba(root.tokens.signalCobalt.r, root.tokens.signalCobalt.g, root.tokens.signalCobalt.b, 0.08)
               : (tone === "ai"
                  ? Qt.rgba(root.tokens.signalRaspberry.r, root.tokens.signalRaspberry.g, root.tokens.signalRaspberry.b, 0.08)
                  : tokens.panelStrong)
        border.color: tone === "brand"
                      ? Qt.rgba(root.tokens.signalCobalt.r, root.tokens.signalCobalt.g, root.tokens.signalCobalt.b, 0.18)
                      : (tone === "ai"
                         ? Qt.rgba(root.tokens.signalRaspberry.r, root.tokens.signalRaspberry.g, root.tokens.signalRaspberry.b, 0.2)
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
                wrapMode: bodyMono ? Text.WrapAnywhere : Text.WordWrap
                color: root.tokens.text1
                font.family: bodyMono ? root.tokens.monoFontFamily : root.tokens.textFontFamily
                font.pixelSize: bodyMono ? 11 : 13
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
            color: tokens.panelStrong
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
