import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

ApplicationWindow {
    id: window

    width: 1500
    height: 920
    visible: true
    title: "SAVT"
    color: theme.windowBase

    property var theme: appTheme
    property var selectedCapabilityNode: null
    property var selectedCapabilityEdge: null
    property var selectedComponentNode: null
    property var selectedComponentEdge: null
    property var capabilityNavigationStack: []
    property int selectedC4LevelIndex: 1
    property string relationshipViewMode: "focused"
    property string componentRelationshipViewMode: "focused"
    property string componentFilterText: ""
    property bool inspectorCollapsed: false
    property int inspectorExpandedWidth: 398

    readonly property var capabilitySceneData: analysisController.capabilityScene || ({})
    readonly property var capabilityNodes: capabilitySceneData.nodes || []
    readonly property var capabilityEdges: capabilitySceneData.edges || []
    readonly property var capabilityGroups: capabilitySceneData.groups || []
    readonly property var componentSceneCatalog: analysisController.componentSceneCatalog || ({})
    readonly property var selectedCapabilityComponentScene: selectedCapabilityNode && selectedCapabilityNode.id !== undefined
                                                      ? (componentSceneCatalog[capabilityIdKey(selectedCapabilityNode.id)] || ({}))
                                                      : ({})
    readonly property var componentNodes: selectedCapabilityComponentScene.nodes || []
    readonly property var componentEdges: selectedCapabilityComponentScene.edges || []
    readonly property var componentGroups: selectedCapabilityComponentScene.groups || []
    property var capabilityNodeIndex: ({})
    property var capabilityEdgeIndex: ({})
    property var capabilityNeighborIndex: ({})
    property var capabilityEdgesByNodeIndex: ({})
    property var componentNodeIndex: ({})
    property var componentEdgeIndex: ({})
    property var componentNeighborIndex: ({})
    property var componentEdgesByNodeIndex: ({})

    property var c4Levels: [
        {"title": "L1 系统上下文", "eyebrow": "Landscape", "summary": "项目整体定位、外部输入输出和主要容器"},
        {"title": "L2 容器层", "eyebrow": "Containers", "summary": "项目被拆成哪些主要部分，以及它们如何协作"},
        {"title": "L3 组件层", "eyebrow": "Components", "summary": "从单个能力域继续下钻，查看内部组件、阶段和协作关系"},
        {"title": "L4 全景报告", "eyebrow": "Report", "summary": "静态分析生成的 Markdown 全景报告"}
    ]

    AppTheme {
        id: appTheme
    }

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: theme.windowTop }
            GradientStop { position: 1.0; color: theme.windowBase }
        }

        Rectangle {
            width: 460
            height: 460
            radius: 230
            x: -140
            y: -180
            color: "#dbe7f0"
            opacity: 0.82
        }

        Rectangle {
            width: 390
            height: 390
            radius: 195
            x: parent.width - width * 0.75
            y: -90
            color: "#edf2f7"
            opacity: 0.78
        }

        Rectangle {
            width: 540
            height: 540
            radius: 270
            x: parent.width - width * 0.55
            y: parent.height - height * 0.55
            color: "#d9e4ed"
            opacity: 0.42
        }
    }

    function capabilityIdKey(nodeId) {
        return String(nodeId)
    }

    function rebuildCapabilityIndexes() {
        var nodes = capabilityNodes
        var edges = capabilityEdges
        var nodeIndex = {}
        var edgeIndex = {}
        var neighborIndex = {}
        var edgesByNodeIndex = {}

        for (var i = 0; i < nodes.length; ++i) {
            var node = nodes[i]
            nodeIndex[capabilityIdKey(node.id)] = node
        }

        for (var j = 0; j < edges.length; ++j) {
            var edge = edges[j]
            edgeIndex[String(edge.id)] = edge
            var fromKey = capabilityIdKey(edge.fromId)
            var toKey = capabilityIdKey(edge.toId)

            if (!neighborIndex[fromKey])
                neighborIndex[fromKey] = {}
            if (!neighborIndex[toKey])
                neighborIndex[toKey] = {}
            neighborIndex[fromKey][toKey] = true
            neighborIndex[toKey][fromKey] = true

            if (!edgesByNodeIndex[fromKey])
                edgesByNodeIndex[fromKey] = []
            if (!edgesByNodeIndex[toKey])
                edgesByNodeIndex[toKey] = []
            edgesByNodeIndex[fromKey].push(edge)
            edgesByNodeIndex[toKey].push(edge)
        }

        capabilityNodeIndex = nodeIndex
        capabilityEdgeIndex = edgeIndex
        capabilityNeighborIndex = neighborIndex
        capabilityEdgesByNodeIndex = edgesByNodeIndex

        if (selectedCapabilityNode && selectedCapabilityNode.id !== undefined) {
            var selectedKey = capabilityIdKey(selectedCapabilityNode.id)
            selectedCapabilityNode = nodeIndex[selectedKey] || null
        }
        if (selectedCapabilityEdge && selectedCapabilityEdge.id !== undefined)
            selectedCapabilityEdge = edgeIndex[String(selectedCapabilityEdge.id)] || null
    }

    function rebuildComponentIndexes() {
        var nodes = componentNodes
        var edges = componentEdges
        var nodeIndex = {}
        var edgeIndex = {}
        var neighborIndex = {}
        var edgesByNodeIndex = {}

        for (var i = 0; i < nodes.length; ++i) {
            var node = nodes[i]
            nodeIndex[capabilityIdKey(node.id)] = node
        }

        for (var j = 0; j < edges.length; ++j) {
            var edge = edges[j]
            edgeIndex[String(edge.id)] = edge
            var fromKey = capabilityIdKey(edge.fromId)
            var toKey = capabilityIdKey(edge.toId)

            if (!neighborIndex[fromKey])
                neighborIndex[fromKey] = {}
            if (!neighborIndex[toKey])
                neighborIndex[toKey] = {}
            neighborIndex[fromKey][toKey] = true
            neighborIndex[toKey][fromKey] = true

            if (!edgesByNodeIndex[fromKey])
                edgesByNodeIndex[fromKey] = []
            if (!edgesByNodeIndex[toKey])
                edgesByNodeIndex[toKey] = []
            edgesByNodeIndex[fromKey].push(edge)
            edgesByNodeIndex[toKey].push(edge)
        }

        componentNodeIndex = nodeIndex
        componentEdgeIndex = edgeIndex
        componentNeighborIndex = neighborIndex
        componentEdgesByNodeIndex = edgesByNodeIndex

        if (selectedComponentNode && selectedComponentNode.id !== undefined) {
            var selectedNodeKey = capabilityIdKey(selectedComponentNode.id)
            selectedComponentNode = nodeIndex[selectedNodeKey] || null
        }
        if (selectedComponentEdge && selectedComponentEdge.id !== undefined)
            selectedComponentEdge = edgeIndex[String(selectedComponentEdge.id)] || null

        if ((!selectedComponentNode || selectedComponentNode.id === undefined) &&
                (!selectedComponentEdge || selectedComponentEdge.id === undefined) &&
                nodes.length === 1) {
            selectedComponentNode = nodes[0]
        }
    }

    Component.onCompleted: {
        rebuildCapabilityIndexes()
        rebuildComponentIndexes()
    }

    Connections {
        target: analysisController
        function onCapabilitySceneChanged() { window.rebuildCapabilityIndexes() }
        function onComponentSceneCatalogChanged() { window.rebuildComponentIndexes() }
    }

    Connections {
        target: window
        function onSelectedCapabilityNodeChanged() { window.rebuildComponentIndexes() }
    }

    Component {
        id: systemContextPageComponent

        ScrollView {
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 16

                SurfaceCard {
                    Layout.fillWidth: true
                    minimumContentHeight: 152
                    stacked: true
                    fillColor: theme.surfacePrimary
                    borderColor: theme.borderStrong

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 14

                        Label {
                            Layout.fillWidth: true
                            text: analysisController.systemContextData.title || "L1 系统上下文"
                            color: theme.inkStrong
                            font.family: theme.displayFontFamily
                            font.pixelSize: 26
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: analysisController.systemContextData.headline || "先完成一次分析，这里会显示项目定位和整体脉络。"
                            color: theme.inkNormal
                            font.family: theme.textFontFamily
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: (analysisController.systemContextData.purposeSummary || "").length > 0
                            text: analysisController.systemContextData.purposeSummary || ""
                            color: theme.inkMuted
                            font.family: theme.textFontFamily
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8
                            visible: (analysisController.systemContextData.containerNames || []).length > 0

                            Repeater {
                                model: analysisController.systemContextData.containerNames || []

                                TagChip {
                                    text: modelData
                                    fillColor: "#ffffff"
                                    borderColor: theme.borderSubtle
                                    textColor: theme.inkNormal
                                }
                            }
                        }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: width > 1100 ? 3 : (width > 720 ? 2 : 1)
                    columnSpacing: 14
                    rowSpacing: 14

                    Repeater {
                        model: analysisController.systemContextCards || []

                        SurfaceCard {
                            Layout.fillWidth: true
                            fillColor: theme.surfaceSecondary
                            borderColor: theme.borderSubtle

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.name || ""
                                    color: theme.inkStrong
                                    font.family: theme.displayFontFamily
                                    font.pixelSize: 17
                                    font.weight: Font.DemiBold
                                    wrapMode: Text.WordWrap
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.summary || ""
                                    color: theme.inkMuted
                                    font.family: theme.textFontFamily
                                    font.pixelSize: 13
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: width > 1050 ? 2 : 1
                    columnSpacing: 16
                    rowSpacing: 16

                    SurfaceCard {
                        Layout.fillWidth: true
                        minimumContentHeight: 210
                        fillColor: theme.surfacePrimary
                        borderColor: theme.borderSubtle

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            Label {
                                text: "快速摘要"
                                color: theme.inkStrong
                                font.family: theme.displayFontFamily
                                font.pixelSize: 20
                                font.weight: Font.DemiBold
                            }

                            Repeater {
                                model: [
                                    {"title": "适合先看", "body": analysisController.systemContextData.entrySummary || ""},
                                    {"title": "输入 / 输出", "body": ((analysisController.systemContextData.inputSummary || "") + "\n" + (analysisController.systemContextData.outputSummary || "")).trim()},
                                    {"title": "技术线索", "body": analysisController.systemContextData.technologySummary || ""},
                                    {"title": "容器摘要", "body": analysisController.systemContextData.containerSummary || ""}
                                ]

                                Rectangle {
                                    Layout.fillWidth: true
                                    visible: (modelData.body || "").length > 0
                                    radius: 18
                                    color: "#ffffff"
                                    border.color: theme.borderSubtle

                                    ColumnLayout {
                                        anchors.fill: parent
                                        anchors.margins: 14
                                        spacing: 6

                                        Label {
                                            text: modelData.title
                                            color: theme.inkNormal
                                            font.family: theme.textFontFamily
                                            font.pixelSize: 12
                                            font.weight: Font.DemiBold
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.body
                                            color: theme.inkMuted
                                            font.family: theme.textFontFamily
                                            font.pixelSize: 13
                                            wrapMode: Text.WordWrap
                                        }
                                    }
                                }
                            }
                        }
                    }

                    SurfaceCard {
                        Layout.fillWidth: true
                        minimumContentHeight: 260
                        fillColor: theme.surfacePrimary
                        borderColor: theme.borderSubtle

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    text: "详细 Markdown 报告"
                                    color: theme.inkStrong
                                    font.family: theme.displayFontFamily
                                    font.pixelSize: 20
                                    font.weight: Font.DemiBold
                                }

                                Item { Layout.fillWidth: true }

                                AppButton {
                                    theme: window.theme
                                    compact: true
                                    tone: "ghost"
                                    text: "复制"
                                    onClicked: analysisController.copyTextToClipboard(analysisController.systemContextReport)
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                implicitHeight: 340
                                radius: 22
                                color: "#ffffff"
                                border.color: theme.borderSubtle

                                ScrollView {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    clip: true

                                    TextArea {
                                        text: analysisController.systemContextReport
                                        readOnly: true
                                        wrapMode: TextEdit.Wrap
                                        textFormat: TextEdit.MarkdownText
                                        color: theme.inkStrong
                                        font.family: theme.textFontFamily
                                        font.pixelSize: 14
                                        background: null
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: astPreviewPageComponent

        ColumnLayout {
            spacing: 16

            SurfaceCard {
                Layout.fillWidth: true
                fillColor: theme.surfaceSecondary
                borderColor: theme.borderSubtle

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: "AST 预览"
                                color: theme.inkStrong
                                font.family: theme.displayFontFamily
                                font.pixelSize: 22
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: analysisController.astPreviewSummary
                                wrapMode: Text.WordWrap
                                color: theme.inkMuted
                                font.family: theme.textFontFamily
                                font.pixelSize: 13
                            }
                        }

                        TagChip {
                            text: "文件 " + ((analysisController.astFileItems || []).length)
                            fillColor: "#ffffff"
                            borderColor: theme.borderSubtle
                            textColor: theme.inkNormal
                        }
                    }

                    ComboBox {
                        id: astFileSelector
                        Layout.fillWidth: true
                        model: analysisController.astFileItems
                        textRole: "label"
                        enabled: (analysisController.astFileItems || []).length > 0
                        font.family: theme.textFontFamily
                        font.pixelSize: 14

                        background: Rectangle {
                            radius: 18
                            color: "#ffffff"
                            border.color: astFileSelector.activeFocus ? theme.borderStrong : theme.borderSubtle
                        }

                        contentItem: Text {
                            leftPadding: 14
                            rightPadding: 36
                            verticalAlignment: Text.AlignVCenter
                            text: astFileSelector.displayText
                            color: theme.inkStrong
                            font: astFileSelector.font
                            elide: Text.ElideRight
                        }

                        function syncSelection() {
                            var items = analysisController.astFileItems || []
                            var selectedPath = analysisController.selectedAstFilePath
                            var matched = -1
                            for (var i = 0; i < items.length; ++i) {
                                if (items[i].path === selectedPath) {
                                    matched = i
                                    break
                                }
                            }
                            currentIndex = matched
                        }

                        Component.onCompleted: syncSelection()

                        onActivated: {
                            var items = analysisController.astFileItems || []
                            if (index >= 0 && index < items.length)
                                analysisController.selectedAstFilePath = items[index].path
                        }

                        Connections {
                            target: analysisController
                            function onSelectedAstFilePathChanged() { astFileSelector.syncSelection() }
                            function onAstFileItemsChanged() { astFileSelector.syncSelection() }
                        }
                    }
                }
            }

            SurfaceCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                fillColor: "#0f1c2b"
                borderColor: "#24374b"
                stacked: true

                ScrollView {
                    anchors.fill: parent
                    clip: true

                    TextArea {
                        text: analysisController.astPreviewText
                        readOnly: true
                        wrapMode: TextEdit.NoWrap
                        color: "#dce8f4"
                        font.family: theme.monoFontFamily
                        font.pixelSize: 13
                        selectionColor: "#2d5d87"
                        background: null
                    }
                }
            }
        }
    }

    Component {
        id: analysisReportPageComponent

        ColumnLayout {
            spacing: 16

            SurfaceCard {
                Layout.fillWidth: true
                minimumContentHeight: 104
                fillColor: theme.surfaceSecondary
                borderColor: theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: "L4 全景报告"
                            color: theme.inkStrong
                            font.family: theme.displayFontFamily
                            font.pixelSize: 22
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "保留 Markdown 入口，但用更清晰的阅读框体承载，便于后续继续扩展导出和分享能力。"
                            wrapMode: Text.WordWrap
                            color: theme.inkMuted
                            font.family: theme.textFontFamily
                            font.pixelSize: 13
                        }
                    }

                    AppButton {
                        theme: window.theme
                        tone: "ghost"
                        text: "复制报告"
                        onClicked: analysisController.copyTextToClipboard(analysisController.analysisReport)
                    }
                }
            }

            SurfaceCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                fillColor: theme.surfacePrimary
                borderColor: theme.borderStrong
                stacked: true

                ScrollView {
                    anchors.fill: parent
                    clip: true

                    TextArea {
                        text: analysisController.analysisReport
                        readOnly: true
                        wrapMode: TextEdit.Wrap
                        textFormat: TextEdit.MarkdownText
                        color: theme.inkStrong
                        font.family: theme.textFontFamily
                        font.pixelSize: 14
                        background: null
                    }
                }
            }
        }
    }

    function displayNodeKind(kind) {
        if (kind === "entry" || kind === "entry_component")
            return "发起入口"
        if (kind === "infrastructure")
            return "后台支撑"
        if (kind === "support_component")
            return "支撑组件"
        if (kind === "component")
            return "内部组件"
        return "核心能力"
    }

    function capabilityNodeById(nodeId) {
        return capabilityNodeIndex[capabilityIdKey(nodeId)] || null
    }

    function capabilityEdgeById(edgeId) {
        return capabilityEdgeIndex[String(edgeId)] || null
    }

    function componentNodeById(nodeId) {
        return componentNodeIndex[capabilityIdKey(nodeId)] || null
    }

    function componentEdgeById(edgeId) {
        return componentEdgeIndex[String(edgeId)] || null
    }

    function nodeShouldBeDisplayed(nodeId) {
        return capabilityNodeById(nodeId) !== null
    }

    function componentNodeShouldBeDisplayed(nodeId) {
        var node = componentNodeById(nodeId)
        return node !== null && componentNodeMatchesFilter(node)
    }

    function selectCapabilityNode(node) {
        if (!node || node.id === undefined)
            return
        selectedCapabilityEdge = null
        selectedCapabilityNode = capabilityNodeById(node.id) || node
        selectedComponentNode = null
        selectedComponentEdge = null
    }

    function selectCapabilityEdge(edge) {
        if (!edge || edge.id === undefined)
            return
        selectedCapabilityEdge = capabilityEdgeById(edge.id) || edge
        selectedComponentNode = null
        selectedComponentEdge = null
    }

    function selectComponentNode(node) {
        if (!node || node.id === undefined)
            return
        selectedComponentEdge = null
        selectedComponentNode = componentNodeById(node.id) || node
    }

    function selectComponentEdge(edge) {
        if (!edge || edge.id === undefined)
            return
        selectedComponentEdge = componentEdgeById(edge.id) || edge
    }

    function selectInspectorEndpointNode(node) {
        if (!node || node.id === undefined)
            return
        if (selectedC4LevelIndex === 2 && componentNodeById(node.id)) {
            selectComponentNode(node)
            return
        }
        selectCapabilityNode(node)
    }

    function selectInspectorRelationshipEdge(edge) {
        if (!edge || edge.id === undefined)
            return
        if (selectedC4LevelIndex === 2 && componentEdgeById(edge.id)) {
            selectComponentEdge(edge)
            return
        }
        selectCapabilityEdge(edge)
    }

    function drillIntoCapabilityNode(node) {
        selectCapabilityNode(node)
        componentFilterText = ""
        selectedC4LevelIndex = 2
    }

    function selectedInspectorNodeData() {
        if (selectedC4LevelIndex === 2 && selectedComponentNode && selectedComponentNode.id !== undefined)
            return selectedComponentNode
        if (selectedCapabilityNode && selectedCapabilityNode.id !== undefined)
            return selectedCapabilityNode
        return null
    }

    function selectedInspectorEdgeData() {
        if (selectedC4LevelIndex === 2 && selectedComponentEdge && selectedComponentEdge.id !== undefined)
            return selectedComponentEdge
        if (selectedCapabilityEdge && selectedCapabilityEdge.id !== undefined)
            return selectedCapabilityEdge
        return null
    }

    function selectedInspectorHasComponentNode() {
        return selectedC4LevelIndex === 2 && selectedComponentNode && selectedComponentNode.id !== undefined
    }

    function selectedInspectorHasComponentEdge() {
        return selectedC4LevelIndex === 2 && selectedComponentEdge && selectedComponentEdge.id !== undefined
    }

    function nodeTooltipSummary(node) {
        if (!node)
            return ""
        return (node.summary || "").length > 0 ? node.summary : node.name
    }

    function nodeDisplayRect(node) {
        if (!node)
            return {"x": 0, "y": 0, "width": 0, "height": 0}
        if (node.layoutBounds)
            return node.layoutBounds
        return {"x": node.x || 0, "y": node.y || 0, "width": node.width || 0, "height": node.height || 0}
    }

    function selectedNodeDisplayName() {
        var node = selectedInspectorNodeData()
        return node ? node.name : "尚未选择模块"
    }

    function selectedNodeResponsibility() {
        var node = selectedInspectorNodeData()
        return node ? node.responsibility : ""
    }

    function selectedNodeEvidenceSummary() {
        var node = selectedInspectorNodeData()
        if (!node || !node.topSymbols)
            return ""
        var values = node.topSymbols || []
        if (values.length === 0)
            return ""
        return values.slice(0, 4).join("、")
    }

    function selectedInspectorKind() {
        if (selectedInspectorEdgeData())
            return "edge"
        if (selectedInspectorNodeData())
            return "node"
        return ""
    }

    function selectedInspectorNodeKindLabel() {
        var node = selectedInspectorNodeData()
        return node ? displayNodeKind(node.kind) : ""
    }

    function selectedInspectorNodeRoleLabel() {
        var node = selectedInspectorNodeData()
        return node ? (node.role || "") : ""
    }

    function selectedInspectorNodeFileCountLabel() {
        var node = selectedInspectorNodeData()
        return node ? ("文件 " + (node.fileCount || 0)) : ""
    }

    function selectedInspectorEdgeWeightLabel() {
        var edge = selectedInspectorEdgeData()
        return edge ? ("权重 " + (edge.weight || 0)) : ""
    }

    function selectedInspectorEdgeKindLabel() {
        var edge = selectedInspectorEdgeData()
        return edge ? (edge.kind || "") : ""
    }

    function selectedInspectorSupportsCodeContext() {
        return selectedInspectorKind() === "node" && !selectedInspectorHasComponentNode() && selectedCapabilityNode !== null
    }

    function selectedEvidencePackage() {
        var edge = selectedInspectorEdgeData()
        if (edge)
            return edge.evidence || ({})
        var node = selectedInspectorNodeData()
        if (node)
            return node.evidence || ({})
        return ({})
    }

    function selectedEvidenceFacts() { return selectedEvidencePackage().facts || [] }
    function selectedEvidenceRules() { return selectedEvidencePackage().rules || [] }
    function selectedEvidenceConclusions() { return selectedEvidencePackage().conclusions || [] }
    function selectedEvidenceFiles() { return selectedEvidencePackage().sourceFiles || [] }
    function selectedEvidenceSymbols() { return selectedEvidencePackage().symbols || [] }
    function selectedEvidenceModules() { return selectedEvidencePackage().modules || [] }
    function selectedEvidenceRelationships() { return selectedEvidencePackage().relationships || [] }
    function selectedEvidenceConfidenceLabel() { return selectedEvidencePackage().confidenceLabel || "" }
    function selectedEvidenceConfidenceReason() { return selectedEvidencePackage().confidenceReason || "" }

    function selectedInspectorTitle() {
        var edge = selectedInspectorEdgeData()
        if (edge)
            return (edge.fromName || "上游") + " → " + (edge.toName || "下游")
        var node = selectedInspectorNodeData()
        return node ? node.name : "尚未选择图元素"
    }

    function selectedInspectorSubtitle() {
        var edge = selectedInspectorEdgeData()
        if (edge)
            return edge.summary || ""
        var node = selectedInspectorNodeData()
        return node ? node.responsibility : "先在图里选中一个模块，或在右侧关系列表里点一条边。"
    }

    function selectedNodeRelationshipItems() {
        var items = []
        if (selectedInspectorHasComponentNode())
            items = (componentEdgesByNodeIndex[capabilityIdKey(selectedComponentNode.id)] || []).slice()
        else if (selectedCapabilityNode && selectedCapabilityNode.id !== undefined)
            items = (capabilityEdgesByNodeIndex[capabilityIdKey(selectedCapabilityNode.id)] || []).slice()
        else
            return []

        items.sort(function(left, right) {
            var leftWeight = left.weight || 0
            var rightWeight = right.weight || 0
            if (leftWeight !== rightWeight)
                return rightWeight - leftWeight
            var leftSummary = left.summary || ""
            var rightSummary = right.summary || ""
            if (leftSummary < rightSummary)
                return -1
            if (leftSummary > rightSummary)
                return 1
            return 0
        })
        return items
    }

    function selectedEdgeEndpointNodes() {
        var edge = selectedInspectorEdgeData()
        if (!edge)
            return []
        var items = []
        var useComponentScene = selectedC4LevelIndex === 2 && componentEdgeById(edge.id)
        var fromNode = useComponentScene ? componentNodeById(edge.fromId) : capabilityNodeById(edge.fromId)
        var toNode = useComponentScene ? componentNodeById(edge.toId) : capabilityNodeById(edge.toId)
        if (fromNode)
            items.push(fromNode)
        if (toNode)
            items.push(toNode)
        return items
    }

    function capabilitySceneBounds() {
        return capabilitySceneData.bounds ? capabilitySceneData.bounds : {"x": 0, "y": 0, "width": 0, "height": 0}
    }

    function currentCapabilitySceneWidth() {
        var bounds = capabilitySceneBounds()
        return Math.max(bounds.width || 0, 1040)
    }

    function currentCapabilitySceneHeight() {
        var bounds = capabilitySceneBounds()
        return Math.max(bounds.height || 0, 520)
    }

    function selectedComponentSceneTitle() {
        return selectedCapabilityComponentScene.title || (selectedCapabilityNode ? selectedCapabilityNode.name : "L3 组件层")
    }

    function selectedComponentSceneSummary() {
        return selectedCapabilityComponentScene.summary || ""
    }

    function componentSceneDiagnostics() {
        return selectedCapabilityComponentScene.diagnostics || []
    }

    function componentSceneBounds() {
        return selectedCapabilityComponentScene.bounds ? selectedCapabilityComponentScene.bounds : {"x": 0, "y": 0, "width": 0, "height": 0}
    }

    function currentComponentSceneWidth() {
        var bounds = componentSceneBounds()
        return Math.max(bounds.width || 0, 880)
    }

    function currentComponentSceneHeight() {
        var bounds = componentSceneBounds()
        return Math.max(bounds.height || 0, 420)
    }

    function componentNodeMatchesFilter(node) {
        var query = (componentFilterText || "").trim().toLowerCase()
        if (query.length === 0)
            return true
        var haystack = ((node.name || "") + " " + (node.role || "") + " " +
                        ((node.moduleNames || []).join(" ")) + " " +
                        ((node.topSymbols || []).join(" "))).toLowerCase()
        return haystack.indexOf(query) >= 0
    }

    function capabilityNodeOpacity(node) {
        if (!node)
            return 1.0
        if (selectedCapabilityEdge && selectedCapabilityEdge.id !== undefined)
            return (selectedCapabilityEdge.fromId === node.id || selectedCapabilityEdge.toId === node.id) ? 1.0 : 0.34
        if (!selectedCapabilityNode || selectedCapabilityNode.id === undefined)
            return 1.0
        if (selectedCapabilityNode.id === node.id)
            return 1.0
        var selectedNeighbors = capabilityNeighborIndex[capabilityIdKey(selectedCapabilityNode.id)] || {}
        return selectedNeighbors[capabilityIdKey(node.id)] ? 1.0 : 0.34
    }

    function componentNodeOpacity(node) {
        if (!node)
            return 1.0
        if (!componentNodeMatchesFilter(node))
            return 0.1
        if (selectedComponentEdge && selectedComponentEdge.id !== undefined)
            return (selectedComponentEdge.fromId === node.id || selectedComponentEdge.toId === node.id) ? 1.0 : 0.3
        if (!selectedComponentNode || selectedComponentNode.id === undefined)
            return 1.0
        if (selectedComponentNode.id === node.id)
            return 1.0
        var neighbors = componentNeighborIndex[capabilityIdKey(selectedComponentNode.id)] || {}
        return neighbors[capabilityIdKey(node.id)] ? 1.0 : 0.34
    }

    function edgeRoutePoints(edge) {
        if (edge && edge.routePoints && edge.routePoints.length >= 2)
            return edge.routePoints
        return []
    }

    function edgeTouchesSelection(edge) {
        if (selectedCapabilityEdge && selectedCapabilityEdge.id !== undefined)
            return selectedCapabilityEdge.id === edge.id
        return selectedCapabilityNode && (edge.fromId === selectedCapabilityNode.id || edge.toId === selectedCapabilityNode.id)
    }

    function componentEdgeTouchesSelection(edge) {
        if (selectedComponentEdge && selectedComponentEdge.id !== undefined)
            return selectedComponentEdge.id === edge.id
        return selectedComponentNode && (edge.fromId === selectedComponentNode.id || edge.toId === selectedComponentNode.id)
    }

    function visibleCapabilityEdges() {
        if (relationshipViewMode === "all")
            return capabilityEdges.slice(0, Math.min(capabilityEdges.length, 180))
        if (selectedCapabilityEdge && selectedCapabilityEdge.id !== undefined)
            return [selectedCapabilityEdge]
        if (!selectedCapabilityNode)
            return []
        return capabilityEdgesByNodeIndex[capabilityIdKey(selectedCapabilityNode.id)] || []
    }

    function visibleComponentEdges() {
        var filteredNodeIds = {}
        for (var i = 0; i < componentNodes.length; ++i) {
            var node = componentNodes[i]
            if (componentNodeMatchesFilter(node))
                filteredNodeIds[capabilityIdKey(node.id)] = true
        }
        if (componentRelationshipViewMode === "all") {
            return componentEdges.filter(function(edge) {
                return filteredNodeIds[capabilityIdKey(edge.fromId)] && filteredNodeIds[capabilityIdKey(edge.toId)]
            }).slice(0, Math.min(componentEdges.length, 240))
        }
        if (selectedComponentEdge && selectedComponentEdge.id !== undefined)
            return [selectedComponentEdge]
        if (!selectedComponentNode)
            return []
        return (componentEdgesByNodeIndex[capabilityIdKey(selectedComponentNode.id)] || []).filter(function(edge) {
            return filteredNodeIds[capabilityIdKey(edge.fromId)] && filteredNodeIds[capabilityIdKey(edge.toId)]
        })
    }

    function drawCapabilityEdge(ctx, edge) {
        var points = edgeRoutePoints(edge)
        if (!points || points.length < 2)
            return

        var emphasized = edgeTouchesSelection(edge)
        var hasSelection = (selectedCapabilityNode && selectedCapabilityNode.id !== undefined) ||
                           (selectedCapabilityEdge && selectedCapabilityEdge.id !== undefined)
        ctx.beginPath()
        ctx.lineJoin = "round"
        ctx.lineCap = "round"
        ctx.strokeStyle = theme.edgeColor(edge.kind)
        ctx.lineWidth = emphasized ? 3.0 : 1.4
        ctx.globalAlpha = hasSelection ? (emphasized ? 0.92 : 0.12) : 0.52
        ctx.moveTo(points[0].x, points[0].y)

        for (var i = 1; i < points.length; ++i)
            ctx.lineTo(points[i].x, points[i].y)

        ctx.stroke()
    }

    function drawComponentEdge(ctx, edge) {
        var points = edgeRoutePoints(edge)
        if (!points || points.length < 2)
            return

        var emphasized = componentEdgeTouchesSelection(edge)
        var hasSelection = (selectedComponentNode && selectedComponentNode.id !== undefined) ||
                           (selectedComponentEdge && selectedComponentEdge.id !== undefined)
        ctx.beginPath()
        ctx.lineJoin = "round"
        ctx.lineCap = "round"
        ctx.strokeStyle = theme.edgeColor(edge.kind)
        ctx.lineWidth = emphasized ? 2.8 : 1.3
        ctx.globalAlpha = hasSelection ? (emphasized ? 0.9 : 0.14) : 0.46
        ctx.moveTo(points[0].x, points[0].y)
        for (var i = 1; i < points.length; ++i)
            ctx.lineTo(points[i].x, points[i].y)
        ctx.stroke()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 18

        TopControlBar {
            Layout.fillWidth: true
            theme: window.theme
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            SurfaceCard {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                stacked: true
                fillColor: theme.surfacePrimary
                borderColor: theme.borderStrong

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 18

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: "Analysis Workspace"
                                color: theme.inkStrong
                                font.family: theme.displayFontFamily
                                font.pixelSize: 26
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: c4Levels[selectedC4LevelIndex].summary
                                color: theme.inkMuted
                                wrapMode: Text.WordWrap
                                font.family: theme.textFontFamily
                                font.pixelSize: 13
                            }
                        }

                        TagChip {
                            text: selectedInspectorKind() === "edge"
                                  ? ("已选中关系 " + selectedInspectorTitle())
                                  : (selectedInspectorNodeData() ? ("已选中 " + selectedInspectorNodeData().name) : c4Levels[selectedC4LevelIndex].eyebrow)
                            fillColor: "#ffffff"
                            borderColor: theme.borderSubtle
                            textColor: theme.inkNormal
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 10

                        Repeater {
                            model: window.c4Levels

                            delegate: Rectangle {
                                width: 176
                                height: 82
                                radius: 20
                                color: theme.levelFill(index, window.selectedC4LevelIndex === index)
                                border.color: theme.levelBorder(index, window.selectedC4LevelIndex === index)
                                border.width: 1
                                clip: true

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    height: 6
                                    color: window.selectedC4LevelIndex === index ? theme.accentStrong : "#d7e0e8"
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    anchors.topMargin: 16
                                    spacing: 3

                                    Label {
                                        text: modelData.eyebrow
                                        color: theme.levelInk(window.selectedC4LevelIndex === index)
                                        font.family: theme.textFontFamily
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                    }

                                    Label {
                                        text: modelData.title
                                        color: theme.inkStrong
                                        font.family: theme.displayFontFamily
                                        font.pixelSize: 17
                                        font.weight: Font.DemiBold
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData.summary
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                        color: theme.inkMuted
                                        font.family: theme.textFontFamily
                                        font.pixelSize: 11
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: window.selectedC4LevelIndex = index
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: theme.borderSubtle
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: window.selectedC4LevelIndex

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Loader {
                                anchors.fill: parent
                                active: window.selectedC4LevelIndex === 0
                                asynchronous: true
                                sourceComponent: systemContextPageComponent
                            }
                        }

                        L2GraphView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            theme: window.theme
                        }

                        L3ComponentView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            theme: window.theme
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Loader {
                                anchors.fill: parent
                                active: window.selectedC4LevelIndex === 3
                                asynchronous: true
                                sourceComponent: analysisReportPageComponent
                            }
                        }
                    }
                }
            }

            RightInspector {
                theme: window.theme
                SplitView.preferredWidth: window.inspectorCollapsed ? 86 : window.inspectorExpandedWidth
                SplitView.minimumWidth: window.inspectorCollapsed ? 86 : 340
                SplitView.fillHeight: true
            }
        }
    }
}
