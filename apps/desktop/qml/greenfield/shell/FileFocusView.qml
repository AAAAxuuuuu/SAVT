import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../foundation"

Item {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject focusState
    property var scene: ({})
    property var fileDetail: ({})

    readonly property var node: focusState.focusedNode || ({})
    readonly property bool fileDetailActive: !!fileDetail && !!fileDetail.available && !!fileDetail.singleFile
    readonly property string fileNameText: cleanText(fileDetail.fileName || node.name || "文件解读")
    readonly property string filePathText: cleanText(fileDetail.filePath || "")
    readonly property bool aiBusyForCurrentFile: analysisController.aiBusy && aiTargetMatchesCurrentFile()
    readonly property bool aiResultForCurrentFile: analysisController.aiHasResult && aiTargetMatchesCurrentFile()
    readonly property var relationRows: buildRelationRows()
    readonly property var detailSections: buildDetailSections()
    readonly property string detailSectionText: buildDetailSectionText()

    function cleanText(value) {
        return String(value || "")
                .replace(/\uFFFD/g, "")
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

    function aiTargetMatchesCurrentFile() {
        if (!root.fileDetailActive)
            return false

        if ((root.analysisController.aiScope || "") !== "file_node")
            return false

        var aiName = normalizedText(root.analysisController.aiNodeName)
        if (!aiName.length)
            return false

        var candidates = [
            normalizedText(root.node.name),
            normalizedText(root.fileDetail.fileName),
            normalizedText(root.fileDetail.filePath)
        ]
        for (var index = 0; index < candidates.length; ++index) {
            var candidate = candidates[index]
            if (!candidate.length)
                continue
            if (candidate === aiName || candidate.indexOf(aiName) >= 0 || aiName.indexOf(candidate) >= 0)
                return true
        }
        return false
    }

    function aiDigestText() {
        var parts = []
        var summary = cleanText(root.analysisController.aiSummary)
        var responsibility = cleanText(root.analysisController.aiResponsibility)
        if (summary.length > 0)
            parts.push(summary)
        if (responsibility.length > 0)
            parts.push(responsibility)
        return parts.join("\n\n")
    }

    function buildRelationRows() {
        var rows = []
        if (!root.node || root.node.id === undefined)
            return rows

        var edges = root.scene.edges || []
        var nodes = root.scene.nodes || []

        function nodeNameById(nodeId) {
            for (var index = 0; index < nodes.length; ++index) {
                if (nodes[index].id === nodeId)
                    return cleanText(nodes[index].name || "未命名节点")
            }
            return "未命名节点"
        }

        for (var edgeIndex = 0; edgeIndex < edges.length; ++edgeIndex) {
            var edge = edges[edgeIndex]
            if (edge.toId === root.node.id) {
                rows.push("上游：" + nodeNameById(edge.fromId) + " -> " + root.fileNameText)
            } else if (edge.fromId === root.node.id) {
                rows.push("下游：" + root.fileNameText + " -> " + nodeNameById(edge.toId))
            }
        }

        if (rows.length === 0 && root.focusState.focusedCapability && root.focusState.focusedCapability.name) {
            rows.push("当前文件归属于能力域「" + cleanText(root.focusState.focusedCapability.name) + "」。")
        }
        return rows.slice(0, 8)
    }

    function cleanRows(values) {
        var rows = []
        var seen = {}
        var source = values || []
        for (var index = 0; index < source.length; ++index) {
            var row = cleanText(source[index])
            if (!row.length || seen[row])
                continue
            seen[row] = true
            rows.push(row)
        }
        return rows
    }

    function buildDetailSections() {
        var specs = [
            {"title": "导入 / 依赖线索", "rows": cleanRows(root.fileDetail.importClues), "mono": false},
            {"title": "声明 / 主符号", "rows": cleanRows(root.fileDetail.declarationClues), "mono": false},
            {"title": "行为信号", "rows": cleanRows(root.fileDetail.behaviorSignals), "mono": false},
            {"title": "建议阅读顺序", "rows": cleanRows(root.fileDetail.readingHints), "mono": false},
            {"title": "关联关系", "rows": cleanRows(root.relationRows), "mono": false}
        ]
        var sections = []
        for (var index = 0; index < specs.length; ++index) {
            if (specs[index].rows.length > 0)
                sections.push(specs[index])
        }
        return sections
    }

    function buildDetailSectionText() {
        var parts = []
        for (var sectionIndex = 0; sectionIndex < root.detailSections.length; ++sectionIndex) {
            var section = root.detailSections[sectionIndex]
            var rows = section.rows || []
            if (!rows.length)
                continue

            var sectionRows = []
            for (var rowIndex = 0; rowIndex < rows.length && rowIndex < 4; ++rowIndex)
                sectionRows.push("· " + rows[rowIndex])
            parts.push(section.title + "\n" + sectionRows.join("\n"))
        }
        return parts.join("\n\n")
    }

    function requestFileAiExplanation() {
        if (!root.node || root.node.id === undefined)
            return

        root.analysisController.requestAiExplanation(
                    root.node,
                    "请只围绕当前文件进行中文解读。"
                    + " 请结合文件路径、语言、导入线索、声明符号、行为信号和关键片段来说明："
                    + " 这个文件的核心职责、它依赖谁或被谁依赖、建议先看哪些符号、有哪些风险或不确定性。"
                    + " 不要把整个能力域当成主体，不要给通用模块套话。")
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: parent.width
            spacing: 14

            Rectangle {
                Layout.fillWidth: true
                radius: root.tokens.radius8
                color: root.tokens.panelStrong
                border.color: root.tokens.border1
                implicitHeight: fileHeaderColumn.implicitHeight + 28

                ColumnLayout {
                    id: fileHeaderColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    anchors.topMargin: 14
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                Layout.fillWidth: true
                                text: root.fileNameText
                                wrapMode: Text.WordWrap
                                color: root.tokens.text1
                                font.family: root.tokens.displayFontFamily
                                font.pixelSize: 24
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.filePathText
                                visible: text.length > 0
                                wrapMode: Text.WrapAnywhere
                                color: root.tokens.text3
                                font.family: root.tokens.monoFontFamily
                                font.pixelSize: 11
                            }
                        }

                        ActionButton {
                            tokens: root.tokens
                            text: "回到组件概览"
                            tone: "secondary"
                            compact: true
                            Layout.alignment: Qt.AlignTop
                            onClicked: root.focusState.clearNodeFocus()
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        ChipBadge {
                            tokens: root.tokens
                            text: root.cleanText(root.fileDetail.languageLabel || "源码文件")
                            tone: "brand"
                        }

                        ChipBadge {
                            tokens: root.tokens
                            text: root.cleanText(root.fileDetail.categoryLabel || "具体文件")
                        }

                        ChipBadge {
                            tokens: root.tokens
                            text: root.cleanText(root.fileDetail.roleLabel || "实现文件")
                        }

                        ChipBadge {
                            tokens: root.tokens
                            text: "总行数 " + Number(root.fileDetail.lineCount || 0)
                            tone: "success"
                        }

                        ChipBadge {
                            tokens: root.tokens
                            text: "非空 " + Number(root.fileDetail.nonEmptyLineCount || 0)
                            tone: "neutral"
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.cleanText(root.fileDetail.summary || "当前文件还没有形成稳定的详细解读。")
                        wrapMode: Text.WordWrap
                        color: root.tokens.text1
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 13
                        lineHeight: 1.2
                    }
                }
            }

            AccentCard {
                Layout.fillWidth: true
                tokens: root.tokens
                title: root.aiBusyForCurrentFile ? "AI 文件解读 · 生成中" : "AI 文件解读"
                body: root.aiBusyForCurrentFile
                      ? (root.cleanText(root.analysisController.aiStatusMessage).length > 0
                         ? root.cleanText(root.analysisController.aiStatusMessage)
                         : "正在结合当前文件的路径、声明和关键片段生成解读，请稍候。")
                      : (root.aiResultForCurrentFile
                         ? root.aiDigestText()
                         : "这里会把 AI 的输出约束到当前文件，不再泛泛解释整个模块。")
                tone: "ai"
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                radius: root.tokens.radius8
                color: root.tokens.panelStrong
                border.color: root.tokens.border1

                ActionButton {
                    anchors.centerIn: parent
                    width: parent.width - 28
                    tokens: root.tokens
                    text: root.analysisController.aiBusy ? "AI 正在生成..." : "请 AI 解读这个文件"
                    tone: "primary"
                    enabled: root.analysisController.aiAvailable
                             && !root.analysisController.aiBusy
                             && root.fileDetailActive
                    onClicked: root.requestFileAiExplanation()
                }
            }

            AccentCard {
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? implicitHeight : 0
                visible: root.cleanMultilineText(root.detailSectionText).length > 0
                tokens: root.tokens
                title: "文件线索索引"
                body: root.cleanMultilineText(root.detailSectionText)
                tone: "neutral"
            }

            AccentCard {
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? implicitHeight : 0
                visible: root.cleanMultilineText(root.fileDetail.previewText).length > 0
                tokens: root.tokens
                title: "候选关键代码"
                body: root.cleanMultilineText(root.fileDetail.previewText)
                bodyMono: true
                tone: "neutral"
            }
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
        property bool bodyMono: false

        radius: tokens.radius8
        color: tone === "ai"
               ? Qt.rgba(0.69, 0.32, 0.87, 0.08)
               : root.tokens.panelStrong
        border.color: tone === "ai"
                      ? Qt.rgba(0.69, 0.32, 0.87, 0.18)
                      : tokens.border1
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
                color: tone === "ai" ? root.tokens.signalRaspberry : root.tokens.text2
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
}
