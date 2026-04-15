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

    signal chooseProjectRequested()

    readonly property var systemData: analysisController.systemContextData || ({})
    readonly property var readiness: systemData.semanticReadiness || ({})
    readonly property var projectConfig: systemData.projectConfig || ({})
    readonly property var configCounts: projectConfig.counts || ({})
    readonly property var ignoreDirectories: projectConfig.ignoreDirectories || []
    readonly property var moduleMerges: projectConfig.moduleMerges || []
    readonly property var roleOverrides: projectConfig.roleOverrides || []
    readonly property var entryOverrides: projectConfig.entryOverrides || []
    readonly property var dependencyFoldRules: projectConfig.dependencyFoldRules || []
    readonly property var configDiagnostics: projectConfig.diagnostics || []
    readonly property var configSearchPaths: projectConfig.searchPaths || []
    readonly property var configRecommendation: systemData.projectConfigRecommendation || ({})
    readonly property var draftConfig: configRecommendation.draftConfig || ({})
    readonly property var draftCounts: configRecommendation.draftCounts || ({})
    readonly property var draftIgnoreDirectories: draftConfig.ignoreDirectories || []
    readonly property var draftModuleMerges: draftConfig.moduleMerges || []
    readonly property var draftRoleOverrides: draftConfig.roleOverrides || []
    readonly property var draftEntryOverrides: draftConfig.entryOverrides || []
    readonly property var draftDependencyFoldRules: draftConfig.dependencyFoldRules || []
    readonly property var configChoiceItems: configRecommendation.choiceItems || []

    clip: true
    contentWidth: availableWidth

    function textOr(value, fallbackText) {
        var text = String(value || "").trim()
        return text.length > 0 ? text : (fallbackText || "")
    }

    function countOr(value) {
        return Number(value || 0).toString()
    }

    function joinLines(items, formatter) {
        var lines = []
        var source = items || []
        for (var i = 0; i < source.length; ++i) {
            var line = formatter(source[i], i)
            if (line && String(line).trim().length > 0)
                lines.push(String(line).trim())
        }
        return lines
    }

    function selectedChoiceDescription(choice) {
        var selectedId = textOr(choice.selectedOptionId, "")
        var options = choice.options || []
        for (var i = 0; i < options.length; ++i) {
            if (textOr(options[i].id, "") === selectedId)
                return textOr(options[i].description, "")
        }
        return ""
    }

    function selectedChoiceLabel(choice) {
        var selectedId = textOr(choice.selectedOptionId, "")
        var options = choice.options || []
        for (var i = 0; i < options.length; ++i) {
            if (textOr(options[i].id, "") === selectedId)
                return textOr(options[i].label, "")
        }
        return ""
    }

    function configStatusTone() {
        if (projectConfig.loaded)
            return "success"
        if (textOr(projectConfig.configPath, "").length > 0)
            return "warning"
        return "secondary"
    }

    function semanticTone() {
        var code = textOr(readiness.statusCode, "")
        if (code === "semantic_ready")
            return "success"
        if (code === "backend_unavailable" || code === "llvm_not_found"
                || code === "llvm_headers_missing" || code === "libclang_not_found"
                || code === "compilation_database_load_failed"
                || code === "compilation_database_empty"
                || code === "system_headers_unresolved"
                || code === "translation_unit_parse_failed")
            return "warning"
        return "info"
    }

    function aiTone() {
        return analysisController.aiAvailable ? "ai" : "warning"
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

            ColumnLayout {
                id: headerColumn
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: "分析环境配置"
                            color: root.tokens.text1
                            font.family: root.tokens.displayFontFamily
                            font.pixelSize: 24
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.textOr(projectConfig.headline,
                                              "集中查看项目级配置、语义分析状态和 AI 可用性。")
                            wrapMode: Text.WordWrap
                            color: root.tokens.text2
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 13
                        }
                    }

                    RowLayout {
                        spacing: 10

                        ActionButton {
                            tokens: root.tokens
                            text: "选择项目"
                            tone: "secondary"
                            onClicked: root.chooseProjectRequested()
                        }

                        ActionButton {
                            tokens: root.tokens
                            text: root.analysisController.analyzing ? "停止分析" : "重新分析"
                            tone: root.analysisController.analyzing ? "danger" : "primary"
                            enabled: root.caseState.hasProject
                            onClicked: {
                                if (root.analysisController.analyzing)
                                    root.analysisController.stopAnalysis()
                                else
                                    root.analysisController.analyzeCurrentProject()
                            }
                        }

                        ActionButton {
                            tokens: root.tokens
                            text: "刷新 AI"
                            tone: "secondary"
                            onClicked: root.analysisController.refreshAiAvailability()
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: root.tokens.radius6
                    color: root.tokens.base1
                    border.color: root.tokens.border1
                    implicitHeight: 44

                    Label {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        verticalAlignment: Text.AlignVCenter
                        text: root.analysisController.projectRootPath || "还没有选择项目目录。"
                        elide: Text.ElideMiddle
                        color: root.tokens.text3
                        font.family: root.tokens.monoFontFamily
                        font.pixelSize: 12
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width >= 1320 ? 2 : 1
            columnSpacing: 16
            rowSpacing: 16

            Rectangle {
                Layout.fillWidth: true
                radius: root.tokens.radius8
                color: root.tokens.panelStrong
                border.color: root.tokens.border1
                implicitHeight: projectConfigColumn.implicitHeight + 36

                ColumnLayout {
                    id: projectConfigColumn
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Label {
                            Layout.fillWidth: true
                            text: "项目级配置"
                            color: root.tokens.text1
                            font.family: root.tokens.displayFontFamily
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }

                        TonePill {
                            tokens: root.tokens
                            tone: root.configStatusTone()
                            text: root.textOr(projectConfig.statusLabel, "未发现")
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.textOr(projectConfig.summary,
                                          "当前还没有项目级配置，系统会继续使用默认启发式。")
                        wrapMode: Text.WordWrap
                        color: root.tokens.text2
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 13
                    }

                    InfoRow {
                        tokens: root.tokens
                        label: "配置文件"
                        value: root.textOr(projectConfig.configPath, "未找到项目配置文件")
                        mono: true
                    }

                    Flow {
                        Layout.fillWidth: true
                        width: projectConfigColumn.width
                        spacing: 10

                        MetricChip {
                            tokens: root.tokens
                            tone: "info"
                            label: "忽略目录"
                            value: root.countOr(configCounts.ignore)
                            fixedWidth: 94
                        }

                        MetricChip {
                            tokens: root.tokens
                            tone: "success"
                            label: "模块归并"
                            value: root.countOr(configCounts.merge)
                            fixedWidth: 94
                        }

                        MetricChip {
                            tokens: root.tokens
                            tone: "warning"
                            label: "角色覆盖"
                            value: root.countOr(configCounts.role)
                            fixedWidth: 94
                        }

                        MetricChip {
                            tokens: root.tokens
                            tone: "moss"
                            label: "入口覆盖"
                            value: root.countOr(configCounts.entry)
                            fixedWidth: 94
                        }

                        MetricChip {
                            tokens: root.tokens
                            tone: "secondary"
                            label: "依赖折叠"
                            value: root.countOr(configCounts.fold)
                            fixedWidth: 94
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: root.tokens.radius8
                color: root.tokens.panelStrong
                border.color: root.tokens.border1
                implicitHeight: runtimeColumn.implicitHeight + 36

                ColumnLayout {
                    id: runtimeColumn
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12

                    Label {
                        text: "运行环境"
                        color: root.tokens.text1
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    InfoCard {
                        Layout.fillWidth: true
                        tokens: root.tokens
                        tone: root.semanticTone()
                        title: "语义分析状态"
                        body: root.textOr(root.readiness.headline || root.readiness.modeLabel,
                                          "等待分析结果。")
                        detail: root.textOr(root.readiness.summary || root.readiness.reason,
                                            "SAVT 会在这里区分语法级推断、语义可用和阻断原因。")
                    }

                    InfoCard {
                        Layout.fillWidth: true
                        tokens: root.tokens
                        tone: root.aiTone()
                        title: "AI 连接状态"
                        body: root.analysisController.aiAvailable ? "AI 已就绪" : "AI 未就绪"
                        detail: root.textOr(root.analysisController.aiSetupMessage,
                                            "刷新 AI 后会在这里显示配置结果。")
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: root.tokens.radius8
            color: root.tokens.panelStrong
            border.color: root.tokens.border1
            implicitHeight: suggestedConfigColumn.implicitHeight + 36

            ColumnLayout {
                id: suggestedConfigColumn
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        Layout.fillWidth: true
                        text: "推荐草案"
                        color: root.tokens.text1
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    TonePill {
                        tokens: root.tokens
                        tone: root.textOr(configRecommendation.tone, "info")
                        text: root.configChoiceItems.length > 0
                              ? ("待确认 " + root.configChoiceItems.length)
                              : "可直接生成"
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: root.textOr(configRecommendation.summary,
                                      "分析完成后，这里会整理出一份可落盘的 SAVT 配置草案。")
                    wrapMode: Text.WordWrap
                    color: root.tokens.text2
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 13
                }

                InfoRow {
                    tokens: root.tokens
                    label: "写入位置"
                    value: root.textOr(configRecommendation.targetPath,
                                       ".savt/project.json")
                    mono: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ActionButton {
                        tokens: root.tokens
                        text: "刷新草案"
                        tone: "secondary"
                        enabled: root.caseState.hasProject && !root.analysisController.analyzing
                        onClicked: root.analysisController.refreshProjectConfigRecommendation()
                    }

                    ActionButton {
                        tokens: root.tokens
                        text: root.textOr(configRecommendation.writeActionLabel, "生成配置")
                        tone: "primary"
                        enabled: root.caseState.hasProject
                                 && !root.analysisController.analyzing
                                 && configRecommendation.canGenerate !== false
                        onClicked: root.analysisController.generateRecommendedProjectConfig()
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    width: suggestedConfigColumn.width
                    spacing: 10

                    MetricChip {
                        tokens: root.tokens
                        tone: "info"
                        label: "忽略目录"
                        value: root.countOr(draftCounts.ignore)
                        fixedWidth: 94
                    }

                    MetricChip {
                        tokens: root.tokens
                        tone: "success"
                        label: "模块归并"
                        value: root.countOr(draftCounts.merge)
                        fixedWidth: 94
                    }

                    MetricChip {
                        tokens: root.tokens
                        tone: "warning"
                        label: "角色覆盖"
                        value: root.countOr(draftCounts.role)
                        fixedWidth: 94
                    }

                    MetricChip {
                        tokens: root.tokens
                        tone: "moss"
                        label: "入口覆盖"
                        value: root.countOr(draftCounts.entry)
                        fixedWidth: 94
                    }

                    MetricChip {
                        tokens: root.tokens
                        tone: "secondary"
                        label: "依赖折叠"
                        value: root.countOr(draftCounts.fold)
                        fixedWidth: 94
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.textOr(configRecommendation.decisionHint, "").length > 0
                    text: root.textOr(configRecommendation.decisionHint, "")
                    wrapMode: Text.WordWrap
                    color: root.tokens.text3
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 12
                }

                Repeater {
                    model: root.configChoiceItems

                    Rectangle {
                        required property var modelData

                        readonly property var choiceData: modelData || ({})

                        Layout.fillWidth: true
                        width: suggestedConfigColumn.width
                        radius: root.tokens.radius6
                        color: root.tokens.base1
                        border.color: root.tokens.border1
                        implicitHeight: choiceColumn.implicitHeight + 24

                        ColumnLayout {
                            id: choiceColumn
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Label {
                                Layout.fillWidth: true
                                text: root.textOr(parent.parent.choiceData.title, "")
                                wrapMode: Text.WordWrap
                                color: root.tokens.text1
                                font.family: root.tokens.displayFontFamily
                                font.pixelSize: 15
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.textOr(parent.parent.choiceData.summary, "")
                                wrapMode: Text.WordWrap
                                color: root.tokens.text2
                                font.family: root.tokens.textFontFamily
                                font.pixelSize: 12
                            }

                            Flow {
                                Layout.fillWidth: true
                                width: choiceColumn.width
                                spacing: 8

                                Repeater {
                                    model: parent.parent.parent.choiceData.options || []

                                    ActionButton {
                                        required property var modelData

                                        tokens: root.tokens
                                        compact: true
                                        text: root.textOr(modelData.label, "")
                                        tone: root.textOr(modelData.id, "")
                                              === root.textOr(parent.parent.parent.choiceData.selectedOptionId, "")
                                              ? "primary"
                                              : "secondary"
                                        onClicked: root.analysisController.selectProjectConfigRecommendationOption(
                                                       root.textOr(parent.parent.parent.choiceData.id, ""),
                                                       root.textOr(modelData.id, ""))
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                TonePill {
                                    tokens: root.tokens
                                    tone: "info"
                                    text: "当前选择"
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: root.selectedChoiceLabel(parent.parent.choiceData)
                                    wrapMode: Text.WordWrap
                                    color: root.tokens.text1
                                    font.family: root.tokens.textFontFamily
                                    font.pixelSize: 12
                                    font.weight: Font.DemiBold
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.selectedChoiceDescription(parent.parent.choiceData)
                                wrapMode: Text.WordWrap
                                color: root.tokens.text3
                                font.family: root.tokens.textFontFamily
                                font.pixelSize: 12
                            }
                        }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: width >= 1560 ? 3 : 2
                    columnSpacing: 16
                    rowSpacing: 16

                    RuleGroupCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        tokens: root.tokens
                        title: "忽略目录"
                        subtitle: "草案"
                        items: root.draftIgnoreDirectories
                        emptyText: "草案里还没有忽略目录。"
                        formatter: function(item) { return String(item || "").trim() }
                    }

                    RuleGroupCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        tokens: root.tokens
                        title: "模块归并"
                        subtitle: "草案"
                        items: root.draftModuleMerges
                        emptyText: "草案里还没有模块归并。"
                        formatter: function(item) {
                            return String(item.match || "").trim() + " -> " + String(item.target || "").trim()
                        }
                    }

                    RuleGroupCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        tokens: root.tokens
                        title: "角色覆盖"
                        subtitle: "草案"
                        items: root.draftRoleOverrides
                        emptyText: "草案里还没有角色覆盖。"
                        formatter: function(item) {
                            return String(item.match || "").trim() + " -> " + String(item.role || "").trim()
                        }
                    }

                    RuleGroupCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        tokens: root.tokens
                        title: "入口覆盖"
                        subtitle: "草案"
                        items: root.draftEntryOverrides
                        emptyText: "草案里还没有入口覆盖。"
                        formatter: function(item) {
                            var label = item.entry ? "入口" : "非入口"
                            return String(item.match || "").trim() + " -> " + label
                        }
                    }

                    RuleGroupCard {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        tokens: root.tokens
                        title: "依赖折叠"
                        subtitle: "草案"
                        items: root.draftDependencyFoldRules
                        emptyText: "草案里还没有依赖折叠。"
                        formatter: function(item) {
                            var flags = []
                            if (item.collapse)
                                flags.push("默认折叠")
                            if (item.hideByDefault)
                                flags.push("默认隐藏")
                            return String(item.match || "").trim() + " -> " + (flags.length > 0 ? flags.join(" / ") : "保留显示")
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: "当前已生效规则"
            color: root.tokens.text1
            font.family: root.tokens.displayFontFamily
            font.pixelSize: 20
            font.weight: Font.DemiBold
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width >= 1560 ? 3 : 2
            columnSpacing: 16
            rowSpacing: 16

            RuleGroupCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                tokens: root.tokens
                title: "忽略目录"
                subtitle: "不进入扫描"
                items: root.ignoreDirectories
                emptyText: "当前没有配置忽略目录。"
                formatter: function(item) { return String(item || "").trim() }
            }

            RuleGroupCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                tokens: root.tokens
                title: "模块归并"
                subtitle: "收敛模块命名"
                items: root.moduleMerges
                emptyText: "当前没有配置模块归并。"
                formatter: function(item) {
                    return String(item.match || "").trim() + " -> " + String(item.target || "").trim()
                }
            }

            RuleGroupCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                tokens: root.tokens
                title: "角色覆盖"
                subtitle: "覆盖角色推断"
                items: root.roleOverrides
                emptyText: "当前没有配置角色覆盖。"
                formatter: function(item) {
                    return String(item.match || "").trim() + " -> " + String(item.role || "").trim()
                }
            }

            RuleGroupCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                tokens: root.tokens
                title: "入口覆盖"
                subtitle: "显式声明入口"
                items: root.entryOverrides
                emptyText: "当前没有配置入口覆盖。"
                formatter: function(item) {
                    var label = item.entry ? "入口" : "非入口"
                    return String(item.match || "").trim() + " -> " + label
                }
            }

            RuleGroupCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                tokens: root.tokens
                title: "依赖折叠"
                subtitle: "控制默认显隐"
                items: root.dependencyFoldRules
                emptyText: "当前没有配置依赖折叠。"
                formatter: function(item) {
                    var flags = []
                    if (item.collapse)
                        flags.push("默认折叠")
                    if (item.hideByDefault)
                        flags.push("默认隐藏")
                    return String(item.match || "").trim() + " -> " + (flags.length > 0 ? flags.join(" / ") : "保留显示")
                }
            }

            RuleGroupCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                tokens: root.tokens
                title: "查找路径"
                subtitle: "按顺序搜索"
                items: root.configSearchPaths
                emptyText: "当前没有可用的查找路径。"
                formatter: function(item) { return String(item || "").trim() }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: root.tokens.radius8
            color: root.tokens.panelStrong
            border.color: root.tokens.border1
            implicitHeight: diagnosticsColumn.implicitHeight + 32

            ColumnLayout {
                id: diagnosticsColumn
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                Label {
                    text: "配置诊断"
                    color: root.tokens.text1
                    font.family: root.tokens.displayFontFamily
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: root.configDiagnostics.length > 0
                          ? "这里显示配置加载、解析和规则忽略的结果。"
                          : "当前没有额外的配置诊断。配置状态正常时，这里会保持干净。"
                    wrapMode: Text.WordWrap
                    color: root.tokens.text3
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 12
                }

                Repeater {
                    model: root.configDiagnostics

                    Rectangle {
                        Layout.fillWidth: true
                        width: diagnosticsColumn.width
                        radius: root.tokens.radius6
                        color: root.tokens.base1
                        border.color: root.tokens.border1
                        implicitHeight: diagnosticText.implicitHeight + 20

                        Label {
                            id: diagnosticText
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 10
                            text: String(modelData || "")
                            wrapMode: Text.WordWrap
                            color: root.tokens.text2
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }

    component TonePill: Rectangle {
        id: tonePill
        required property QtObject tokens
        property string tone: "info"
        property string text: ""

        radius: height / 2
        color: tokens.toneSoft(tone)
        border.color: tokens.toneColor(tone)
        implicitHeight: 28
        implicitWidth: pillLabel.implicitWidth + 20

        Label {
            id: pillLabel
            anchors.centerIn: parent
            text: tonePill.text
            color: tonePill.tokens.toneColor(tonePill.tone)
            font.family: tonePill.tokens.textFontFamily
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }
    }

    component MetricChip: Rectangle {
        id: metricChip
        required property QtObject tokens
        property string tone: "info"
        property string label: ""
        property string value: ""
        property int fixedWidth: 0

        radius: tokens.radius6
        color: tokens.toneSoft(tone)
        border.color: tokens.toneColor(tone)
        implicitWidth: fixedWidth > 0 ? fixedWidth : 94
        implicitHeight: 62

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
                font.family: tokens.displayFontFamily
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }
        }
    }

    component InfoRow: ColumnLayout {
        id: infoRow
        required property QtObject tokens
        property string label: ""
        property string value: ""
        property bool mono: false

        spacing: 6

        Label {
            text: infoRow.label
            color: infoRow.tokens.text3
            font.family: infoRow.tokens.textFontFamily
            font.pixelSize: 11
        }

        Label {
            Layout.fillWidth: true
            text: infoRow.value
            wrapMode: Text.WordWrap
            color: infoRow.tokens.text2
            font.family: infoRow.mono ? infoRow.tokens.monoFontFamily : infoRow.tokens.textFontFamily
            font.pixelSize: 12
        }
    }

    component InfoCard: Rectangle {
        id: infoCard
        required property QtObject tokens
        property string tone: "info"
        property string title: ""
        property string body: ""
        property string detail: ""

        radius: tokens.radius6
        color: tokens.panelStrong
        border.color: tokens.toneColor(tone)
        implicitHeight: infoColumn.implicitHeight + 24

        ColumnLayout {
            id: infoColumn
            anchors.fill: parent
            anchors.margins: 12
            spacing: 6

            Label {
                text: infoCard.title
                color: infoCard.tokens.text3
                font.family: infoCard.tokens.textFontFamily
                font.pixelSize: 11
            }

            Label {
                Layout.fillWidth: true
                text: infoCard.body
                wrapMode: Text.WordWrap
                color: infoCard.tokens.text1
                font.family: infoCard.tokens.textFontFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                text: infoCard.detail
                wrapMode: Text.WordWrap
                color: infoCard.tokens.text3
                font.family: infoCard.tokens.textFontFamily
                font.pixelSize: 12
            }
        }
    }

    component RuleGroupCard: Rectangle {
        id: ruleGroupCard
        required property QtObject tokens
        property string title: ""
        property string subtitle: ""
        property var items: []
        property string emptyText: ""
        property var formatter

        radius: tokens.radius8
        color: tokens.panelStrong
        border.color: tokens.border1
        implicitHeight: Math.max(172, ruleColumn.implicitHeight + 28)

        ColumnLayout {
            id: ruleColumn
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    text: ruleGroupCard.title
                    color: ruleGroupCard.tokens.text1
                    font.family: ruleGroupCard.tokens.displayFontFamily
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }

                Label {
                    text: ruleGroupCard.subtitle
                    color: ruleGroupCard.tokens.text3
                    font.family: ruleGroupCard.tokens.textFontFamily
                    font.pixelSize: 11
                }
            }

            Repeater {
                model: ruleGroupCard.items

                Rectangle {
                    Layout.fillWidth: true
                    width: ruleColumn.width
                    radius: ruleGroupCard.tokens.radius6
                    color: ruleGroupCard.tokens.base1
                    border.color: ruleGroupCard.tokens.border1
                    implicitHeight: ruleText.implicitHeight + 18

                    Label {
                        id: ruleText
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 9
                        text: ruleGroupCard.formatter
                              ? ruleGroupCard.formatter(modelData)
                              : String(modelData || "")
                        wrapMode: Text.WordWrap
                        color: ruleGroupCard.tokens.text2
                        font.family: ruleGroupCard.tokens.textFontFamily
                        font.pixelSize: 12
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: emptyLabel.visible ? emptyLabel.implicitHeight : 0

                Label {
                    id: emptyLabel
                    anchors.left: parent.left
                    anchors.right: parent.right
                    visible: (ruleGroupCard.items || []).length === 0
                    text: ruleGroupCard.emptyText
                    wrapMode: Text.WordWrap
                    color: ruleGroupCard.tokens.text3
                    font.family: ruleGroupCard.tokens.textFontFamily
                    font.pixelSize: 12
                }
            }
        }
    }
}
