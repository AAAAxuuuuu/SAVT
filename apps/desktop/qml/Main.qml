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
    property var capabilityNavigationStack: []
    property int selectedC4LevelIndex: 1
    property string relationshipViewMode: "focused"
    property bool inspectorCollapsed: false
    property int inspectorExpandedWidth: 398

    readonly property var capabilitySceneData: analysisController.capabilityScene || ({})
    readonly property var capabilityNodes: capabilitySceneData.nodes || []
    readonly property var capabilityEdges: capabilitySceneData.edges || []
    readonly property var capabilityGroups: capabilitySceneData.groups || []
    property var capabilityNodeIndex: ({})
    property var capabilityNeighborIndex: ({})
    property var capabilityEdgesByNodeIndex: ({})

    property var c4Levels: [
        {"title": "L1 系统上下文", "eyebrow": "Landscape", "summary": "项目整体定位、外部输入输出和主要容器"},
        {"title": "L2 容器层", "eyebrow": "Containers", "summary": "项目被拆成哪些主要部分，以及它们如何协作"},
        {"title": "L3 组件层", "eyebrow": "Components", "summary": "当前仍以程序员入口 AST 预览作为深挖界面"},
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
        var neighborIndex = {}
        var edgesByNodeIndex = {}

        for (var i = 0; i < nodes.length; ++i) {
            var node = nodes[i]
            nodeIndex[capabilityIdKey(node.id)] = node
        }

        for (var j = 0; j < edges.length; ++j) {
            var edge = edges[j]
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
        capabilityNeighborIndex = neighborIndex
        capabilityEdgesByNodeIndex = edgesByNodeIndex

        if (selectedCapabilityNode && selectedCapabilityNode.id !== undefined) {
            var selectedKey = capabilityIdKey(selectedCapabilityNode.id)
            selectedCapabilityNode = nodeIndex[selectedKey] || null
        }
    }

    Component.onCompleted: rebuildCapabilityIndexes()

    Connections {
        target: analysisController
        function onCapabilitySceneChanged() { window.rebuildCapabilityIndexes() }
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
        return kind === "entry" ? "发起入口" : (kind === "infrastructure" ? "后台支撑" : "核心能力")
    }

    function capabilityNodeById(nodeId) {
        return capabilityNodeIndex[capabilityIdKey(nodeId)] || null
    }

    function nodeShouldBeDisplayed(nodeId) {
        return true
    }

    function selectCapabilityNode(node) {
        if (!node || node.id === undefined)
            return
        selectedCapabilityNode = node
    }

    function drillIntoCapabilityNode(node) {
        selectCapabilityNode(node)
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
        return selectedCapabilityNode ? selectedCapabilityNode.name : "尚未选择模块"
    }

    function selectedNodeResponsibility() {
        return selectedCapabilityNode ? selectedCapabilityNode.responsibility : ""
    }

    function selectedNodeEvidenceSummary() {
        if (!selectedCapabilityNode || !selectedCapabilityNode.topSymbols)
            return ""
        var values = selectedCapabilityNode.topSymbols || []
        if (values.length === 0)
            return ""
        return values.slice(0, 4).join("、")
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

    function capabilityNodeOpacity(node) {
        if (!node || !selectedCapabilityNode || selectedCapabilityNode.id === undefined)
            return 1.0
        if (selectedCapabilityNode.id === node.id)
            return 1.0
        var selectedNeighbors = capabilityNeighborIndex[capabilityIdKey(selectedCapabilityNode.id)] || {}
        return selectedNeighbors[capabilityIdKey(node.id)] ? 1.0 : 0.34
    }

    function edgeRoutePoints(edge) {
        if (edge && edge.routePoints && edge.routePoints.length >= 2)
            return edge.routePoints
        return []
    }

    function edgeTouchesSelection(edge) {
        return selectedCapabilityNode && (edge.fromId === selectedCapabilityNode.id || edge.toId === selectedCapabilityNode.id)
    }

    function visibleCapabilityEdges() {
        if (relationshipViewMode === "all")
            return capabilityEdges.slice(0, Math.min(capabilityEdges.length, 180))
        if (!selectedCapabilityNode)
            return []
        return capabilityEdgesByNodeIndex[capabilityIdKey(selectedCapabilityNode.id)] || []
    }

    function drawCapabilityEdge(ctx, edge) {
        var points = edgeRoutePoints(edge)
        if (!points || points.length < 2)
            return

        var emphasized = edgeTouchesSelection(edge)
        var hasSelection = selectedCapabilityNode && selectedCapabilityNode.id !== undefined
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
                            text: selectedCapabilityNode ? ("已选中 " + selectedCapabilityNode.name) : c4Levels[selectedC4LevelIndex].eyebrow
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

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Loader {
                                anchors.fill: parent
                                active: window.selectedC4LevelIndex === 2
                                asynchronous: true
                                sourceComponent: astPreviewPageComponent
                            }
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
