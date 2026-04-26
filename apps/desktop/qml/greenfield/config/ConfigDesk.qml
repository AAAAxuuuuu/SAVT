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
    readonly property int detailColumns: width >= 1120 ? 2 : 1
    readonly property var draftRulePanelItems: buildDraftRulePanelItems()
    readonly property var draftRulePanelColumns: splitPanelItems(draftRulePanelItems, detailColumns)
    readonly property var activeRulePanelItems: buildActiveRulePanelItems()
    readonly property var activeRulePanelColumns: splitPanelItems(activeRulePanelItems, detailColumns)

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

    function rulePanelSpec(title, subtitle, items, emptyText, formatter) {
        return {
            "kind": "rule",
            "title": title,
            "subtitle": subtitle,
            "items": items || [],
            "emptyText": emptyText,
            "formatter": formatter
        }
    }

    function buildDraftRulePanelItems() {
        return [
            rulePanelSpec("忽略目录", "草案", root.draftIgnoreDirectories, "草案里还没有忽略目录。",
                          function(item) { return String(item || "").trim() }),
            rulePanelSpec("模块归并", "草案", root.draftModuleMerges, "草案里还没有模块归并。",
                          function(item) {
                              return String(item.match || "").trim() + " -> " + String(item.target || "").trim()
                          }),
            rulePanelSpec("角色覆盖", "草案", root.draftRoleOverrides, "草案里还没有角色覆盖。",
                          function(item) {
                              return String(item.match || "").trim() + " -> " + String(item.role || "").trim()
                          }),
            rulePanelSpec("入口覆盖", "草案", root.draftEntryOverrides, "草案里还没有入口覆盖。",
                          function(item) {
                              var label = item.entry ? "入口" : "非入口"
                              return String(item.match || "").trim() + " -> " + label
                          }),
            rulePanelSpec("依赖折叠", "草案", root.draftDependencyFoldRules, "草案里还没有依赖折叠。",
                          function(item) {
                              var flags = []
                              if (item.collapse)
                                  flags.push("默认折叠")
                              if (item.hideByDefault)
                                  flags.push("默认隐藏")
                              return String(item.match || "").trim() + " -> " + (flags.length > 0 ? flags.join(" / ") : "保留显示")
                          })
        ]
    }

    function buildActiveRulePanelItems() {
        return [
            rulePanelSpec("忽略目录", "不进入扫描", root.ignoreDirectories, "当前没有配置忽略目录。",
                          function(item) { return String(item || "").trim() }),
            rulePanelSpec("模块归并", "收敛模块命名", root.moduleMerges, "当前没有配置模块归并。",
                          function(item) {
                    return String(item.match || "").trim() + " -> " + String(item.target || "").trim()
                }),
            rulePanelSpec("角色覆盖", "覆盖角色推断", root.roleOverrides, "当前没有配置角色覆盖。",
                          function(item) {
                    return String(item.match || "").trim() + " -> " + String(item.role || "").trim()
                }),
            rulePanelSpec("入口覆盖", "显式声明入口", root.entryOverrides, "当前没有配置入口覆盖。",
                          function(item) {
                    var label = item.entry ? "入口" : "非入口"
                    return String(item.match || "").trim() + " -> " + label
                }),
            rulePanelSpec("依赖折叠", "控制默认显隐", root.dependencyFoldRules, "当前没有配置依赖折叠。",
                          function(item) {
                    var flags = []
                    if (item.collapse)
                        flags.push("默认折叠")
                    if (item.hideByDefault)
                        flags.push("默认隐藏")
                    return String(item.match || "").trim() + " -> " + (flags.length > 0 ? flags.join(" / ") : "保留显示")
                }),
            rulePanelSpec("查找路径", "按顺序搜索", root.configSearchPaths, "当前没有可用的查找路径。",
                          function(item) { return String(item || "").trim() }),
            {
                "kind": "diagnostics",
                "title": "配置诊断",
                "items": root.configDiagnostics
            }
        ]
    }

    function estimatePanelWeight(item) {
        if (!item)
            return 0

        var source = item.items || []
        var totalLength = String(item.title || "").length
                + String(item.subtitle || "").length
                + String(item.emptyText || "").length
        for (var index = 0; index < source.length; ++index) {
            var row = item.formatter ? item.formatter(source[index]) : String(source[index] || "")
            totalLength += String(row || "").length
        }
        return 1.8 + Math.max(1, source.length) * 1.1 + Math.ceil(totalLength / 150)
    }

    function splitPanelItems(items, columnCount) {
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
            weights[lightestIndex] += estimatePanelWeight(source[itemIndex])
        }
        return columns
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
                            hint: "切换要分析和配置的项目根目录。"
                            tone: "secondary"
                            onClicked: root.chooseProjectRequested()
                        }

                        ActionButton {
                            tokens: root.tokens
                            text: "快速建模"
                            hint: "使用快速建模重新生成当前项目的架构结果。"
                            disabledHint: "先选择项目目录，才能启动快速建模。"
                            tone: "primary"
                            enabled: root.caseState.hasProject
                            onClicked: root.analysisController.analyzeCurrentProject()
                        }

                        ActionButton {
                            tokens: root.tokens
                            text: "精确推演"
                            hint: "检查或生成 compile_commands.json 后，执行精确推演。"
                            disabledHint: "先选择项目目录，再启动精确推演。"
                            tone: "analysis"
                            enabled: root.caseState.hasProject
                            onClicked: root.analysisController.analyzeCurrentProjectHighPrecision()
                        }

                        ActionButton {
                            tokens: root.tokens
                            text: "刷新 AI"
                            hint: "重新检测本机 AI 配置和服务可用状态。"
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
                        title: "精度状态"
                        body: root.textOr(root.readiness.headline || root.readiness.modeLabel,
                                          "等待分析结果。")
                        detail: root.textOr(root.readiness.summary || root.readiness.reason,
                                            "SAVT 会在这里区分快速建模、精确推演和阻断原因。")
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
                        hint: "根据当前项目重新生成配置草案，帮助补齐忽略目录、模块归并和角色覆盖。"
                        disabledHint: root.analysisController.analyzing
                                      ? "当前正在分析，请等待完成。"
                                      : "先选择项目目录，再刷新配置草案。"
                        tone: "secondary"
                        enabled: root.caseState.hasProject && !root.analysisController.analyzing
                        onClicked: root.analysisController.refreshProjectConfigRecommendation()
                    }

                    ActionButton {
                        tokens: root.tokens
                        text: root.textOr(configRecommendation.writeActionLabel, "生成配置")
                        hint: "把推荐配置写入项目的 .savt/project.json，后续分析会自动加载。"
                        disabledHint: "当前没有可写入的推荐配置，或项目正在分析中。"
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
                                        hint: root.textOr(modelData.description, "选择这个配置方案。")
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

                Loader {
                    Layout.fillWidth: true
                    Layout.preferredHeight: item ? item.implicitHeight : 0
                    sourceComponent: root.detailColumns > 1 ? draftRulePanelsTwoColumns : draftRulePanelsSingleColumn
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

        Loader {
            Layout.fillWidth: true
            Layout.preferredHeight: item ? item.implicitHeight : 0
            sourceComponent: root.detailColumns > 1 ? activeRulePanelsTwoColumns : activeRulePanelsSingleColumn
        }
    }

    Component {
        id: activeRulePanelsSingleColumn

        ColumnLayout {
            width: parent ? parent.width : 0
            spacing: 16

            Repeater {
                model: root.activeRulePanelItems

                Loader {
                    Layout.fillWidth: true
                    property var panelSpec: modelData
                    sourceComponent: panelSpec.kind === "diagnostics" ? diagnosticsPanelCard : rulePanelCard
                    onLoaded: {
                        if (item)
                            item.panelSpec = panelSpec
                    }
                }
            }
        }
    }

    Component {
        id: draftRulePanelsSingleColumn

        ColumnLayout {
            width: parent ? parent.width : 0
            spacing: 16

            Repeater {
                model: root.draftRulePanelItems

                Loader {
                    Layout.fillWidth: true
                    property var panelSpec: modelData
                    sourceComponent: rulePanelCard
                    onLoaded: {
                        if (item)
                            item.panelSpec = panelSpec
                    }
                }
            }
        }
    }

    Component {
        id: activeRulePanelsTwoColumns

        RowLayout {
            width: parent ? parent.width : 0
            spacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 16

                Repeater {
                    model: root.activeRulePanelColumns.length > 0 ? root.activeRulePanelColumns[0] : []

                    Loader {
                        Layout.fillWidth: true
                        property var panelSpec: modelData
                        sourceComponent: panelSpec.kind === "diagnostics" ? diagnosticsPanelCard : rulePanelCard
                        onLoaded: {
                            if (item)
                                item.panelSpec = panelSpec
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 16

                Repeater {
                    model: root.activeRulePanelColumns.length > 1 ? root.activeRulePanelColumns[1] : []

                    Loader {
                        Layout.fillWidth: true
                        property var panelSpec: modelData
                        sourceComponent: panelSpec.kind === "diagnostics" ? diagnosticsPanelCard : rulePanelCard
                        onLoaded: {
                            if (item)
                                item.panelSpec = panelSpec
                        }
                    }
                }
            }
        }
    }

    Component {
        id: draftRulePanelsTwoColumns

        RowLayout {
            width: parent ? parent.width : 0
            spacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 16

                Repeater {
                    model: root.draftRulePanelColumns.length > 0 ? root.draftRulePanelColumns[0] : []

                    Loader {
                        Layout.fillWidth: true
                        property var panelSpec: modelData
                        sourceComponent: rulePanelCard
                        onLoaded: {
                            if (item)
                                item.panelSpec = panelSpec
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 16

                Repeater {
                    model: root.draftRulePanelColumns.length > 1 ? root.draftRulePanelColumns[1] : []

                    Loader {
                        Layout.fillWidth: true
                        property var panelSpec: modelData
                        sourceComponent: rulePanelCard
                        onLoaded: {
                            if (item)
                                item.panelSpec = panelSpec
                        }
                    }
                }
            }
        }
    }

    Component {
        id: rulePanelCard

        RuleGroupCard {
            property var panelSpec: ({})

            tokens: root.tokens
            title: String(panelSpec.title || "")
            subtitle: String(panelSpec.subtitle || "")
            items: panelSpec.items || []
            emptyText: String(panelSpec.emptyText || "")
            formatter: panelSpec.formatter
        }
    }

    Component {
        id: diagnosticsPanelCard

        DiagnosticsCard {
            property var panelSpec: ({})

            tokens: root.tokens
            title: String(panelSpec.title || "配置诊断")
            items: panelSpec.items || []
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

    component DiagnosticsCard: Rectangle {
        id: diagnosticsCard
        required property QtObject tokens
        property string title: "配置诊断"
        property var items: []

        radius: tokens.radius8
        color: tokens.panelStrong
        border.color: tokens.border1
        implicitHeight: Math.max(154, diagnosticsColumn.implicitHeight + 28)

        ColumnLayout {
            id: diagnosticsColumn
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

            Label {
                text: diagnosticsCard.title
                color: diagnosticsCard.tokens.text1
                font.family: diagnosticsCard.tokens.displayFontFamily
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                text: diagnosticsCard.items.length > 0
                      ? "这里显示配置加载、解析和规则忽略的结果。"
                      : "当前没有额外的配置诊断。配置状态正常时，这里会保持干净。"
                wrapMode: Text.WordWrap
                color: diagnosticsCard.tokens.text3
                font.family: diagnosticsCard.tokens.textFontFamily
                font.pixelSize: 12
            }

            Repeater {
                model: diagnosticsCard.items

                Rectangle {
                    Layout.fillWidth: true
                    width: diagnosticsColumn.width
                    radius: diagnosticsCard.tokens.radius6
                    color: diagnosticsCard.tokens.base1
                    border.color: diagnosticsCard.tokens.border1
                    implicitHeight: diagnosticText.implicitHeight + 14

                    Label {
                        id: diagnosticText
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 7
                        text: String(modelData || "")
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        color: diagnosticsCard.tokens.text2
                        font.family: diagnosticsCard.tokens.textFontFamily
                        font.pixelSize: 12
                        lineHeight: 1.12
                    }
                }
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
        implicitHeight: Math.max(154, ruleColumn.implicitHeight + 24)

        ColumnLayout {
            id: ruleColumn
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

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
                    implicitHeight: ruleText.implicitHeight + 14

                    Label {
                        id: ruleText
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 7
                        text: ruleGroupCard.formatter
                              ? ruleGroupCard.formatter(modelData)
                              : String(modelData || "")
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        color: ruleGroupCard.tokens.text2
                        font.family: ruleGroupCard.tokens.textFontFamily
                        font.pixelSize: 12
                        lineHeight: 1.12
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
                    lineHeight: 1.12
                }
            }
        }
    }
}
