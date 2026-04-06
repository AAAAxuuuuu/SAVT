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

    property string pageMode: "overview"
    readonly property bool hasProject: (root.analysisController.projectRootPath || "").length > 0
    readonly property bool hasAnalysisData: (root.analysisController.systemContextReport || "").length > 0
                                            || (root.analysisController.analysisReport || "").length > 0
                                            || (root.analysisController.systemContextCards || []).length > 0
                                            || (root.analysisController.systemContextData.headline || "").length > 0
                                            || (root.uiState.selection.capabilityNodes || []).length > 0
    readonly property bool showEntryEmptyState: !root.analysisController.analyzing && !root.hasAnalysisData

    signal askRequested()
    signal navigateTo(string pageId)
    signal chooseProjectRequested()

    anchors.fill: parent
    clip: true
    contentWidth: availableWidth

    function projectName() {
        var title = root.analysisController.systemContextData.projectName || ""
        if (title.length > 0)
            return title
        var path = root.analysisController.projectRootPath || ""
        if (path.length === 0)
            return "当前项目"
        var parts = path.split(/[\\\\/]/)
        return parts.length > 0 ? parts[parts.length - 1] : path
    }

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

    ColumnLayout {
        width: parent.width
        spacing: 16

        AppCard {
            Layout.fillWidth: true
            minimumContentHeight: 156
            stacked: false
            fillColor: "#FFFFFF"
            borderColor: root.theme.borderStrong

            ColumnLayout {
                anchors.fill: parent
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: root.pageMode === "l1" ? "L1 系统上下文" : (root.hasProject ? ("当前项目 · " + root.projectName()) : "当前项目")
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 28
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.analysisController.systemContextData.headline
                                  || (!root.hasProject
                                      ? "先选择项目并开始分析，系统会自动生成总览、能力地图和报告入口。"
                                      : (root.showEntryEmptyState
                                         ? "项目目录已经绑定，但还没有分析结果；点击“开始分析”后，这里会先回答项目是做什么的、主要能力块有哪些、接下来从哪里开始看。"
                                         : "分析完成后，这里会先回答这个项目是做什么的、主要能力块有哪些、接下来从哪里开始看。"))
                            wrapMode: Text.WordWrap
                            color: root.theme.inkNormal
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.analysisController.systemContextData.purposeSummary
                                  || (!root.hasProject
                                      ? "入口说明：右上角“选择项目并开始分析”会打开文件夹选择器，选完后会自动启动分析。"
                                      : root.analysisController.statusMessage)
                            wrapMode: Text.WordWrap
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }

                    Flow {
                        Layout.preferredWidth: 320
                        Layout.alignment: Qt.AlignTop
                        spacing: 8

                        StatusBadge {
                            theme: root.theme
                            text: root.analysisController.analyzing ? "分析进行中" : "总览已就绪"
                            tone: root.analysisController.analyzing ? "info" : "success"
                        }

                        AppChip {
                            text: "能力域 " + root.uiState.selection.capabilityNodes.length
                            compact: true
                            fillColor: "#FFFFFF"
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }

                        AppChip {
                            text: "容器 " + ((root.analysisController.systemContextData.containerNames || []).length)
                            compact: true
                            fillColor: "#FFFFFF"
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: (root.analysisController.systemContextData.containerNames || []).length > 0

                    Repeater {
                        model: root.analysisController.systemContextData.containerNames || []

                        AppChip {
                            text: modelData
                            compact: true
                            fillColor: "#FFFFFF"
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 10

                    AppButton {
                        theme: root.theme
                        tone: "accent"
                        text: !root.hasProject ? "选择项目并开始分析"
                              : (root.showEntryEmptyState ? "开始分析" : "进入能力地图")
                        onClicked: {
                            if (!root.hasProject)
                                root.chooseProjectRequested()
                            else if (root.showEntryEmptyState)
                                root.analysisController.analyzeCurrentProject()
                            else
                                root.navigateTo("project.capabilityMap")
                        }
                    }

                    AppButton {
                        theme: root.theme
                        tone: "ghost"
                        visible: root.hasProject
                        text: root.showEntryEmptyState ? "切换项目" : "查看工程报告"
                        onClicked: {
                            if (root.showEntryEmptyState)
                                root.chooseProjectRequested()
                            else
                                root.navigateTo("report.engineering")
                        }
                    }

                    AppButton {
                        theme: root.theme
                        tone: "ai"
                        text: "Ask SAVT"
                        onClicked: root.askRequested()
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            visible: !root.showEntryEmptyState
            columns: width > 1080 ? 3 : 1
            columnSpacing: 16
            rowSpacing: 16

            AppCard {
                Layout.fillWidth: true
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusXxl
                minimumContentHeight: 196

                AppSection {
                    anchors.fill: parent
                    theme: root.theme
                    eyebrow: "前门摘要"
                    title: "项目摘要"
                    subtitle: "用非图形方式快速回答项目定位、发起入口和技术线索。"

                    Repeater {
                        model: [
                            {"title": "适合先看", "body": root.analysisController.systemContextData.entrySummary || "先从能力地图里识别主能力域。"},
                            {"title": "输入 / 输出", "body": ((root.analysisController.systemContextData.inputSummary || "") + "\n" + (root.analysisController.systemContextData.outputSummary || "")).trim()},
                            {"title": "技术线索", "body": root.analysisController.systemContextData.technologySummary || "等待分析结果生成技术摘要。"}
                        ]

                        AppCard {
                            Layout.fillWidth: true
                            fillColor: root.theme.surfaceSecondary
                            borderColor: root.theme.borderSubtle
                            cornerRadius: root.theme.radiusLg
                            minimumContentHeight: 74

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 6

                                Label {
                                    text: modelData.title
                                    color: root.theme.inkStrong
                                    font.family: root.theme.displayFontFamily
                                    font.pixelSize: 15
                                    font.weight: Font.DemiBold
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: (modelData.body || "").length > 0 ? modelData.body : "暂无。"
                                    wrapMode: Text.WordWrap
                                    color: root.theme.inkMuted
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
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusXxl
                minimumContentHeight: 196

                AppSection {
                    anchors.fill: parent
                    theme: root.theme
                    eyebrow: "能力域"
                    title: "主要能力块"
                    subtitle: "点击能力块卡片可以直接回跳到能力地图，继续做结构理解和下钻。"

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Repeater {
                            model: root.analysisController.systemContextCards || []

                            AppCard {
                                Layout.fillWidth: true
                                fillColor: root.theme.surfaceSecondary
                                borderColor: root.theme.borderSubtle
                                cornerRadius: root.theme.radiusLg
                                minimumContentHeight: 78

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 6

                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData.name || ""
                                        wrapMode: Text.WordWrap
                                        color: root.theme.inkStrong
                                        font.family: root.theme.displayFontFamily
                                        font.pixelSize: 16
                                        font.weight: Font.DemiBold
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData.summary || ""
                                        wrapMode: Text.WordWrap
                                        color: root.theme.inkMuted
                                        font.family: root.theme.textFontFamily
                                        font.pixelSize: 12
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.focusCapability(modelData.name || "")
                                }
                            }
                        }

                        Label {
                            visible: (root.analysisController.systemContextCards || []).length === 0
                            text: "等待分析结果生成能力块摘要。"
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }
                }
            }

            AppCard {
                Layout.fillWidth: true
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusXxl
                minimumContentHeight: 196

                AppSection {
                    anchors.fill: parent
                    theme: root.theme
                    eyebrow: "建议路径"
                    title: "推荐阅读路径与快速动作"
                    subtitle: "总览页不把所有信息一次性铺开，而是明确告诉你下一步从哪里继续。"

                    Repeater {
                        model: [
                            {"title": "1. 先看项目定位", "body": root.analysisController.systemContextData.purposeSummary || "从项目一句话解释建立前门认知。"},
                            {"title": "2. 进入能力地图", "body": "识别主能力域和它们之间的协作关系，再决定从哪个能力块继续下钻。"},
                            {"title": "3. 进入组件工作台", "body": "在能力地图里选中一个能力域后，下钻到 L3 组件工作台定位可执行的改造切口。"}
                        ]

                        AppCard {
                            Layout.fillWidth: true
                            fillColor: root.theme.surfaceSecondary
                            borderColor: root.theme.borderSubtle
                            cornerRadius: root.theme.radiusLg
                            minimumContentHeight: 68

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 6

                                Label {
                                    text: modelData.title
                                    color: root.theme.inkStrong
                                    font.family: root.theme.displayFontFamily
                                    font.pixelSize: 15
                                    font.weight: Font.DemiBold
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.body
                                    wrapMode: Text.WordWrap
                                    color: root.theme.inkMuted
                                    font.family: root.theme.textFontFamily
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            visible: !root.showEntryEmptyState
            columns: width > 980 ? 2 : 1
            columnSpacing: 16
            rowSpacing: 16

            AppCard {
                Layout.fillWidth: true
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusXxl
                minimumContentHeight: 164

                AppSection {
                    anchors.fill: parent
                    theme: root.theme
                    eyebrow: "目录映射"
                    title: "目录到能力映射概览"
                    subtitle: root.analysisController.systemContextData.containerSummary || "容器、目录和能力摘要会一起出现在这里。"

                    Label {
                        Layout.fillWidth: true
                        text: root.analysisController.systemContextData.containerSummary || "还没有可展示的目录到能力映射摘要。"
                        wrapMode: Text.WordWrap
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: (root.analysisController.systemContextData.containerNames || []).length > 0

                        Repeater {
                            model: root.analysisController.systemContextData.containerNames || []

                            AppChip {
                                text: modelData
                                compact: true
                                fillColor: "#FFFFFF"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
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
                minimumContentHeight: 164

                AppSection {
                    anchors.fill: parent
                    theme: root.theme
                    eyebrow: root.pageMode === "l1" ? "系统上下文" : "报告摘录"
                    title: root.pageMode === "l1" ? "L1 Markdown 系统上下文" : "总览报告摘录"
                    subtitle: "保留 Markdown 入口，方便复制、汇报和继续交给 AI 或团队成员。"

                    RowLayout {
                        Layout.fillWidth: true

                        Item { Layout.fillWidth: true }

                        AppButton {
                            theme: root.theme
                            compact: true
                            tone: "ghost"
                            text: "复制"
                            onClicked: root.analysisController.copyTextToClipboard(root.analysisController.systemContextReport)
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.analysisController.systemContextReport.length > 0
                              ? root.analysisController.systemContextReport
                              : "还没有生成系统上下文报告。"
                        wrapMode: Text.WordWrap
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }
            }
        }

        AppCard {
            Layout.fillWidth: true
            visible: root.showEntryEmptyState
            fillColor: "#FFFFFF"
            borderColor: root.theme.borderSubtle
            cornerRadius: root.theme.radiusXxl
            minimumContentHeight: 220

            AppEmptyState {
                anchors.fill: parent
                theme: root.theme
                title: root.hasProject ? "项目已连接，等待开始分析" : "先选择项目并开始分析"
                description: root.hasProject
                             ? "目录已经绑定，但还没有可展示的分析结果。点击下面按钮后，会生成项目总览、能力地图、组件工作台和报告入口。"
                             : "右上角和这里的“选择项目并开始分析”都会打开文件夹选择器。选完项目根目录后，分析会自动开始，不需要再额外按别的键。"
                actionText: root.hasProject ? "开始分析" : "选择项目并开始分析"
                onActionRequested: {
                    if (root.hasProject)
                        root.analysisController.analyzeCurrentProject()
                    else
                        root.chooseProjectRequested()
                }
            }
        }

        ContextActionStrip {
            Layout.fillWidth: true
            visible: !root.showEntryEmptyState
            theme: root.theme
            title: "下一步建议"
            summary: root.uiState.selection.selectedCapabilityNode
                     ? ("当前已经选中过「" + root.uiState.selection.selectedCapabilityNode.name + "」，可以直接进入组件工作台继续下钻。")
                     : "建议先进入能力地图，锁定一个能力域，再继续查看其内部组件和改造切口。"
            chips: [
                {"text": "图是主入口", "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal},
                {"text": "摘要优先", "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal},
                {"text": "理解转行动", "fillColor": "#FFFFFF", "borderColor": root.theme.borderSubtle, "textColor": root.theme.inkNormal}
            ]
            primaryText: root.uiState.selection.selectedCapabilityNode ? "进入组件工作台" : "进入能力地图"
            secondaryText: "查看工程报告"
            secondaryVisible: true
            copyVisible: true
            copyEnabled: root.analysisController.systemContextReport.length > 0
            onPrimaryRequested: {
                if (root.uiState.selection.selectedCapabilityNode)
                    root.navigateTo("project.componentWorkbench")
                else
                    root.navigateTo("project.capabilityMap")
            }
            onSecondaryRequested: root.navigateTo("report.engineering")
            onAskRequested: root.askRequested()
            onCopyRequested: root.analysisController.copyTextToClipboard(root.analysisController.systemContextReport)
        }
    }
}
