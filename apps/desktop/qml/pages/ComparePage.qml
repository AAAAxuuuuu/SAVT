import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"
import "../workspace"

ScrollView {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject uiState

    readonly property bool hasProject: (root.analysisController.projectRootPath || "").length > 0
    readonly property bool hasFocus: (root.uiState.inspector.kind || "").length > 0
    readonly property string compareMode: root.uiState.compare.compareMode

    signal askRequested()
    signal navigateTo(string pageId)

    anchors.fill: parent
    clip: true
    contentWidth: availableWidth

    function joinedText(items, emptyText) {
        return (items || []).length > 0 ? items.join("、") : emptyText
    }

    function projectSummaryItems() {
        return [
            "项目定位：" + (root.analysisController.systemContextData.purposeSummary || root.analysisController.systemContextData.headline || "等待分析结果生成项目定位。"),
            "推荐入口：" + (root.analysisController.systemContextData.entrySummary || "先从能力地图识别主能力域。"),
            "技术线索：" + (root.analysisController.systemContextData.technologySummary || "等待分析结果生成技术摘要。"),
            "目录映射：" + (root.analysisController.systemContextData.containerSummary || "当前还没有目录到能力映射摘要。")
        ]
    }

    function projectEvidenceItems() {
        return [
            "能力域 " + (root.uiState.selection.capabilityNodes || []).length + " 个",
            "容器 " + ((root.analysisController.systemContextData.containerNames || []).length) + " 个",
            "分析阶段：" + (root.analysisController.analysisPhase || "待命"),
            "状态：" + (root.analysisController.statusMessage || "等待开始分析。")
        ]
    }

    function focusSummaryItems() {
        if (!root.hasFocus) {
            return [
                "还没有选中焦点对象。",
                "先在能力地图或组件工作台里选择一个节点或关系，这里就会自动补齐右侧对照列。"
            ]
        }

        return [
            "这是什么：" + (root.uiState.inspector.subtitle || "当前对象缺少摘要说明。"),
            "为什么重要：" + (root.uiState.inspector.importanceSummary || "等待更多结构证据。"),
            "关键文件：" + root.joinedText((root.uiState.inspector.sourceFiles || []).slice(0, 3), "当前对象还没有关键文件线索。"),
            "下一步：" + (root.uiState.inspector.nextStepSummary || "可以继续展开详情、查看证据或打开 AI 导览。")
        ]
    }

    function focusEvidenceItems() {
        if (!root.hasFocus) {
            return [
                "事实 0 条",
                "规则 0 条",
                "结论 0 条",
                "先选择对象后再查看证据对照。"
            ]
        }

        return [
            "事实 " + (root.uiState.inspector.facts || []).length + " 条",
            "规则 " + (root.uiState.inspector.rules || []).length + " 条",
            "结论 " + (root.uiState.inspector.conclusions || []).length + " 条",
            "关系 " + (root.uiState.inspector.relationshipItems || []).length + " 条"
        ]
    }

    function subjectCard(kind) {
        if (kind === "project") {
            return {
                "eyebrow": "项目快照",
                "title": root.analysisController.systemContextData.projectName || "当前项目",
                "summary": root.analysisController.systemContextData.headline || "这里承接项目层的稳定背景，用来和当前焦点对象做对照。",
                "chips": [
                    "能力域 " + (root.uiState.selection.capabilityNodes || []).length,
                    "容器 " + ((root.analysisController.systemContextData.containerNames || []).length),
                    root.analysisController.analysisPhase || "待命"
                ],
                "items": root.compareMode === "evidence" ? root.projectEvidenceItems() : root.projectSummaryItems()
            }
        }

        return {
            "eyebrow": "当前焦点",
            "title": root.hasFocus ? root.uiState.inspector.title : "等待选择焦点对象",
            "summary": root.hasFocus
                       ? (root.uiState.inspector.subtitle || "当前对象缺少摘要说明。")
                       : "在能力地图或组件工作台中选中一个节点或关系后，这里会自动切到对象级对照。",
            "chips": root.hasFocus
                     ? [
                           root.uiState.inspector.kind === "edge" ? "关系" : "节点",
                           root.uiState.inspector.nodeKindLabel || root.uiState.inspector.edgeWeightLabel || "焦点对象",
                           "文件 " + ((root.uiState.inspector.sourceFiles || []).length)
                       ]
                     : ["等待对象", "从主画布选择", "自动联动"],
            "items": root.compareMode === "evidence" ? root.focusEvidenceItems() : root.focusSummaryItems()
        }
    }

    function comparisonRows() {
        if (root.compareMode === "evidence") {
            return [
                {
                    "title": "结构体量",
                    "left": "能力域 " + (root.uiState.selection.capabilityNodes || []).length + " / 容器 "
                            + ((root.analysisController.systemContextData.containerNames || []).length),
                    "right": root.hasFocus
                             ? ("事实 " + (root.uiState.inspector.facts || []).length + " / 结论 "
                                + (root.uiState.inspector.conclusions || []).length)
                             : "等待焦点对象"
                },
                {
                    "title": "证据来源",
                    "left": root.analysisController.systemContextData.containerSummary || "目录到能力映射还未生成。",
                    "right": root.joinedText((root.uiState.inspector.sourceFiles || []).slice(0, 3), "还没有关键文件线索。")
                },
                {
                    "title": "不确定性",
                    "left": root.analysisController.statusMessage || "暂无额外状态。",
                    "right": root.uiState.inspector.confidenceReason || "当前对象还没有额外置信说明。"
                }
            ]
        }

        return [
            {
                "title": "定位",
                "left": root.analysisController.systemContextData.purposeSummary || root.analysisController.systemContextData.headline || "等待生成项目定位。",
                "right": root.hasFocus ? (root.uiState.inspector.subtitle || "当前对象缺少摘要说明。") : "等待焦点对象"
            },
            {
                "title": "重要性",
                "left": root.analysisController.systemContextData.entrySummary || "建议先从能力地图进入。",
                "right": root.hasFocus ? (root.uiState.inspector.importanceSummary || "当前对象还没有重要性说明。") : "等待焦点对象"
            },
            {
                "title": "下一步",
                "left": root.hasProject ? "继续进入能力地图或报告，把项目层理解收紧到一个能力域。" : "先选择项目并开始分析。",
                "right": root.hasFocus ? (root.uiState.inspector.nextStepSummary || "打开详情或 AI 导览。") : "先从主画布选择对象"
            }
        ]
    }

    ColumnLayout {
        width: parent.width
        spacing: 16

        AppCard {
            Layout.fillWidth: true
            fillColor: root.theme.surfacePrimary
            borderColor: root.theme.borderStrong
            cornerRadius: root.theme.radiusXxl
            minimumContentHeight: 128

            AppSection {
                anchors.fill: parent
                theme: root.theme
                eyebrow: "对照模式"
                title: "当前项目与当前焦点的对照"
                subtitle: "先把项目层稳定背景和对象级焦点放到同一视野，再决定要不要继续进入快照差异或文件归属对比。"

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    SegmentedControl {
                        theme: root.theme
                        model: [
                            {"value": "summary", "label": "摘要对照"},
                            {"value": "evidence", "label": "证据对照"}
                        ]
                        currentValue: root.uiState.compare.compareMode
                        onActivated: root.uiState.compare.compareMode = value
                    }

                    StatusBadge {
                        theme: root.theme
                        text: root.hasFocus ? "对象对照已就绪" : "等待选择焦点对象"
                        tone: root.hasFocus ? "success" : "warning"
                    }

                    Item { Layout.fillWidth: true }

                    AppButton {
                        theme: root.theme
                        compact: true
                        tone: "ghost"
                        text: "进入能力地图"
                        onClicked: root.navigateTo("project.capabilityMap")
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 980 ? 2 : 1
            columnSpacing: 16
            rowSpacing: 16

            Repeater {
                model: [root.subjectCard("project"), root.subjectCard("focus")]

                AppCard {
                    Layout.fillWidth: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    cornerRadius: root.theme.radiusXxl
                    minimumContentHeight: 264

                    AppSection {
                        anchors.fill: parent
                        theme: root.theme
                        eyebrow: modelData.eyebrow
                        title: modelData.title
                        subtitle: modelData.summary

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8

                            Repeater {
                                model: modelData.chips || []

                                AppChip {
                                    text: modelData
                                    compact: true
                                    fillColor: "#FFFFFF"
                                    borderColor: root.theme.borderSubtle
                                    textColor: root.theme.inkNormal
                                }
                            }
                        }

                        Repeater {
                            model: modelData.items || []

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: bodyLabel.implicitHeight + 18
                                radius: root.theme.radiusLg
                                color: root.theme.surfaceSecondary
                                border.color: root.theme.borderSubtle

                                Label {
                                    id: bodyLabel
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.margins: 12
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
        }

        AppCard {
            Layout.fillWidth: true
            fillColor: "#FFFFFF"
            borderColor: root.theme.borderSubtle
            cornerRadius: root.theme.radiusXxl

            AppSection {
                anchors.fill: parent
                theme: root.theme
                eyebrow: "快速比较"
                title: "先看这几个关键维度"
                subtitle: "这一层只回答最需要先对齐的问题，不把所有深内容一次性挤进一个页面。"

                Repeater {
                    model: root.comparisonRows()

                    AppCard {
                        Layout.fillWidth: true
                        fillColor: root.theme.surfaceSecondary
                        borderColor: root.theme.borderSubtle
                        cornerRadius: root.theme.radiusLg
                        minimumContentHeight: 104

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            Label {
                                text: modelData.title
                                color: root.theme.inkStrong
                                font.family: root.theme.displayFontFamily
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: width > 720 ? 2 : 1
                                columnSpacing: 12
                                rowSpacing: 12

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: leftLabel.implicitHeight + 18
                                    radius: root.theme.radiusMd
                                    color: "#FFFFFF"
                                    border.color: root.theme.borderSubtle

                                    Label {
                                        id: leftLabel
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        text: "项目侧：\n" + modelData.left
                                        wrapMode: Text.WordWrap
                                        color: root.theme.inkNormal
                                        font.family: root.theme.textFontFamily
                                        font.pixelSize: 12
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: rightLabel.implicitHeight + 18
                                    radius: root.theme.radiusMd
                                    color: "#FFFFFF"
                                    border.color: root.theme.borderSubtle

                                    Label {
                                        id: rightLabel
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        text: "焦点侧：\n" + modelData.right
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
            }
        }

        ContextActionStrip {
            Layout.fillWidth: true
            theme: root.theme
            title: root.hasFocus ? "当前对照已经可用" : "先补齐一个焦点对象"
            summary: root.hasFocus
                     ? "继续进入能力地图、组件工作台或 AI 导览，把刚刚对齐出的重点转成下一步动作。"
                     : "对照页已经准备好项目侧基线。只要在主画布中再选一个对象，右侧这条对照链就会立即完整。"
            chips: [
                {"text": "项目快照", "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal},
                {"text": root.hasFocus ? "当前焦点已联动" : "等待主画布选择", "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal},
                {"text": root.compareMode === "summary" ? "摘要对照" : "证据对照", "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal}
            ]
            primaryText: root.hasFocus ? "打开 AI 导览" : "进入能力地图"
            secondaryText: "查看高级报告"
            secondaryVisible: true
            onPrimaryRequested: {
                if (root.hasFocus)
                    root.askRequested()
                else
                    root.navigateTo("project.capabilityMap")
            }
            onSecondaryRequested: root.navigateTo("report.engineering")
            onAskRequested: root.askRequested()
        }
    }
}
