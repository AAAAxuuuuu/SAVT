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
    readonly property int detailColumns: width >= 1120 ? 2 : 1
    readonly property bool aiBusyForCurrentFile: analysisController.aiBusy && aiTargetMatchesCurrentFile()
    readonly property bool aiResultForCurrentFile: analysisController.aiHasResult && aiTargetMatchesCurrentFile()
    readonly property var relationRows: buildRelationRows()
    readonly property var fileSectionItems: buildFileSectionItems()
    readonly property var fileSectionColumns: splitSectionItems(fileSectionItems, detailColumns)

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

    function buildFileSectionItems() {
        var items = [
            {
                "kind": "accent",
                "title": "文件定位",
                "body": root.cleanText(root.fileDetail.summary || "当前文件已选中，但还没有稳定的定位摘要。"),
                "tone": "neutral"
            },
            {
                "kind": "accent",
                "title": "关键关系",
                "body": root.relationRows.length > 0 ? root.relationRows.slice(0, 4).join("\n") : "当前文件与其他节点的关系还在补充中。",
                "tone": "brand"
            },
            {
                "kind": "section",
                "title": "导入 / 依赖线索",
                "rows": cleanRows(root.fileDetail.importClues).length > 0
                        ? cleanRows(root.fileDetail.importClues)
                        : ["当前文件还没有抽取到明显的导入线索。"]
            },
            {
                "kind": "section",
                "title": "声明 / 主符号",
                "rows": cleanRows(root.fileDetail.declarationClues).length > 0
                        ? cleanRows(root.fileDetail.declarationClues)
                        : ["当前文件还没有抽取到稳定的声明符号。"]
            },
            {
                "kind": "section",
                "title": "行为信号",
                "rows": cleanRows(root.fileDetail.behaviorSignals).length > 0
                        ? cleanRows(root.fileDetail.behaviorSignals)
                        : ["当前文件还没有抽取到明显的行为信号。"]
            },
            {
                "kind": "section",
                "title": "建议阅读顺序",
                "rows": cleanRows(root.fileDetail.readingHints).length > 0
                        ? cleanRows(root.fileDetail.readingHints)
                        : ["建议先从导入、主符号和首个非空代码片段开始。"]
            }
        ]

        var previewText = root.cleanMultilineText(root.fileDetail.previewText)
        if (previewText.length > 0) {
            items.push({
                "kind": "accent",
                "title": "关键片段",
                "body": previewText,
                "bodyMono": true,
                "tone": "neutral"
            })
        }
        return items
    }

    function estimateSectionWeight(item) {
        if (!item)
            return 0

        if (item.kind === "accent") {
            var text = cleanMultilineText(item.body || "")
            return 2.4 + Math.max(1, Math.ceil(text.length / 140))
        }

        var rows = item.rows || []
        var totalLength = 0
        for (var index = 0; index < rows.length; ++index)
            totalLength += cleanText(rows[index]).length
        return 1.4 + rows.length * 0.9 + Math.ceil(totalLength / 160)
    }

    function splitSectionItems(items, columnCount) {
        var source = items || []
        if (columnCount <= 1)
            return [source]

        var columns = []
        var weights = []
        for (var columnIndex = 0; columnIndex < columnCount; ++columnIndex) {
            columns.push([])
            weights.push(0)
        }

        for (var itemIndex = 0; itemIndex < source.length; ++itemIndex) {
            var lightestIndex = 0
            for (var compareIndex = 1; compareIndex < columnCount; ++compareIndex) {
                if (weights[compareIndex] < weights[lightestIndex])
                    lightestIndex = compareIndex
            }
            columns[lightestIndex].push(source[itemIndex])
            weights[lightestIndex] += estimateSectionWeight(source[itemIndex])
        }

        return columns
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
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                radius: root.tokens.radiusXl
                color: Qt.rgba(1, 1, 1, 0.82)
                border.color: root.tokens.border1
                implicitHeight: fileHeaderColumn.implicitHeight + 30

                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: "transparent"
                    border.color: root.tokens.shineBorder
                    border.width: 1
                }

                ColumnLayout {
                    id: fileHeaderColumn
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
                                text: "文件解读"
                                color: root.tokens.text3
                                font.family: root.tokens.textFontFamily
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.fileNameText
                                wrapMode: Text.WordWrap
                                color: root.tokens.text1
                                font.family: root.tokens.displayFontFamily
                                font.pixelSize: 24
                                font.weight: Font.DemiBold
                            }
                        }

                        ActionButton {
                            tokens: root.tokens
                            text: "返回组件图"
                            hint: "退出文件详情，回到组件关系图继续查看上下游。"
                            tone: "secondary"
                            compact: true
                            Layout.alignment: Qt.AlignTop
                            onClicked: root.focusState.clearNodeFocus()
                        }
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
                        lineHeight: 1.22
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: root.tokens.radiusXl
                color: Qt.rgba(root.tokens.signalRaspberry.r, root.tokens.signalRaspberry.g, root.tokens.signalRaspberry.b, 0.07)
                border.color: Qt.rgba(root.tokens.signalRaspberry.r, root.tokens.signalRaspberry.g, root.tokens.signalRaspberry.b, 0.16)
                implicitHeight: aiSectionColumn.implicitHeight + 26

                ColumnLayout {
                    id: aiSectionColumn
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
                            hint: "让 AI 基于当前文件的结构事实生成职责解读和阅读建议。"
                            disabledHint: root.analysisController.aiBusy
                                          ? "AI 正在生成解读，请稍等。"
                                          : "需要 AI 可用且当前文件详情已加载。"
                            tone: "ai"
                            compact: true
                            enabled: root.analysisController.aiAvailable
                                     && !root.analysisController.aiBusy
                                     && root.fileDetailActive
                            onClicked: root.requestFileAiExplanation()
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
                                 : "这里会把 AI 的输出约束到当前文件，不再泛泛解释整个模块。")
                        wrapMode: Text.WordWrap
                        color: root.tokens.text1
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 13
                        lineHeight: 1.22
                    }
                }
            }

            Loader {
                Layout.fillWidth: true
                Layout.preferredHeight: item ? item.implicitHeight : 0
                sourceComponent: root.detailColumns > 1 ? fileSectionsTwoColumns : fileSectionsSingleColumn
            }
        }
    }

    Component {
        id: fileSectionsSingleColumn

        ColumnLayout {
            width: parent ? parent.width : 0
            spacing: 14

            Repeater {
                model: root.fileSectionItems

                Loader {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    property var blockSpec: modelData
                    sourceComponent: blockSpec.kind === "accent" ? accentSectionCard : normalSectionCard
                    onLoaded: {
                        if (item)
                            item.blockSpec = blockSpec
                    }
                }
            }
        }
    }

    Component {
        id: fileSectionsTwoColumns

        RowLayout {
            width: parent ? parent.width : 0
            spacing: 14

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 14

                Repeater {
                    model: (root.fileSectionColumns.length > 0) ? root.fileSectionColumns[0] : []

                    Loader {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        property var blockSpec: modelData
                        sourceComponent: blockSpec.kind === "accent" ? accentSectionCard : normalSectionCard
                        onLoaded: {
                            if (item)
                                item.blockSpec = blockSpec
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 14

                Repeater {
                    model: (root.fileSectionColumns.length > 1) ? root.fileSectionColumns[1] : []

                    Loader {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        property var blockSpec: modelData
                        sourceComponent: blockSpec.kind === "accent" ? accentSectionCard : normalSectionCard
                        onLoaded: {
                            if (item)
                                item.blockSpec = blockSpec
                        }
                    }
                }
            }
        }
    }

    Component {
        id: normalSectionCard

        SectionBlock {
            property var blockSpec: ({})
            tokens: root.tokens
            title: blockSpec.title || ""
            rows: blockSpec.rows || []
        }
    }

    Component {
        id: accentSectionCard

        AccentCard {
            property var blockSpec: ({})
            tokens: root.tokens
            title: blockSpec.title || ""
            body: blockSpec.body || ""
            bodyMono: !!blockSpec.bodyMono
            tone: blockSpec.tone || "neutral"
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
               ? root.tokens.signalCobaltSoft
               : (tone === "success"
                  ? root.tokens.signalTealSoft
                  : Qt.rgba(0.12, 0.18, 0.28, 0.06))
        border.color: tone === "brand"
                      ? Qt.rgba(root.tokens.signalCobalt.r, root.tokens.signalCobalt.g, root.tokens.signalCobalt.b, 0.20)
                      : (tone === "success"
                         ? Qt.rgba(root.tokens.signalTeal.r, root.tokens.signalTeal.g, root.tokens.signalTeal.b, 0.18)
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
