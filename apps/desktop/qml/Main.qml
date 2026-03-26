import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

ApplicationWindow {
    id: window
    width: 1540
    height: 900
    visible: true
    title: "SAVT (Light Theme)"
    color: "#f8fafc"

    property var selectedCapabilityNode: null
    property var capabilityNavigationStack: []
    property int selectedC4LevelIndex: 1
    property string relationshipViewMode: "focused"
    property bool inspectorCollapsed: false
    property int inspectorExpandedWidth: 420

    property var c4Levels: [
        {"title": "L1 \u7cfb\u7edf\u4e0a\u4e0b\u6587", "summary": "\u9879\u76ee\u8fd0\u884c\u73af\u5883\u3001\u5916\u90e8\u8f93\u5165\u8f93\u51fa"},
        {"title": "L2 \u5bb9\u5668\u5c42", "summary": "\u9879\u76ee\u88ab\u62c6\u6210\u54ea\u4e9b\u4e3b\u8981\u90e8\u5206\uff0c\u4ee5\u53ca\u5b83\u4eec\u5982\u4f55\u914d\u5408"},
        {"title": "L3 \u7ec4\u4ef6\u5c42", "summary": "\u70b9\u5f00\u67d0\u4e2a\u4e3b\u8981\u90e8\u5206\u540e\uff0c\u770b\u5b83\u5185\u90e8\u627f\u62c5\u7684\u804c\u8d23"},
        {"title": "L4 \u5168\u666f\u62a5\u544a", "summary": "\u2728 \u79d2\u7ea7\u751f\u6210\u7684 C4 \u67b6\u6784 Markdown \u62a5\u544a"}
    ]

    Component {
        id: systemContextPageComponent

        Frame {
            background: Rectangle { radius: 10; color: "#ffffff" }
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                contentWidth: availableWidth

                ColumnLayout {
                    width: parent.width
                    spacing: 16

                    Frame {
                        Layout.fillWidth: true
                        background: Rectangle {
                            radius: 16
                            color: "#eff6ff"
                            border.color: "#bfdbfe"
                        }

                        ColumnLayout {
                            width: parent.width
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 10

                            Label {
                                text: analysisController.systemContextData.title || "L1 系统上下文"
                                color: "#1d4ed8"
                                font.pixelSize: 22
                                font.bold: true
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Label {
                                text: analysisController.systemContextData.headline || "先完成一次分析，这里会显示项目的整体定位。"
                                color: "#0f172a"
                                font.pixelSize: 16
                                font.bold: true
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Label {
                                visible: (analysisController.systemContextData.purposeSummary || "").length > 0
                                text: analysisController.systemContextData.purposeSummary || ""
                                color: "#334155"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Flow {
                                Layout.fillWidth: true
                                spacing: 8
                                visible: (analysisController.systemContextData.containerNames || []).length > 0

                                Repeater {
                                    model: analysisController.systemContextData.containerNames || []

                                    Rectangle {
                                        radius: 999
                                        color: "#dbeafe"
                                        border.color: "#93c5fd"
                                        width: chipText.implicitWidth + 20
                                        height: chipText.implicitHeight + 10

                                        Label {
                                            id: chipText
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: "#1e3a8a"
                                            font.pixelSize: 12
                                            font.bold: true
                                        }
                                    }
                                }
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: width > 980 ? 3 : (width > 640 ? 2 : 1)
                        rowSpacing: 12
                        columnSpacing: 12

                        Repeater {
                            model: analysisController.systemContextCards || []

                            Frame {
                                Layout.fillWidth: true
                                Layout.minimumHeight: 128
                                background: Rectangle {
                                    radius: 14
                                    color: "#ffffff"
                                    border.color: "#dbe4f0"
                                }

                                ColumnLayout {
                                    width: parent.width
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    spacing: 8

                                    Label {
                                        text: modelData.name || ""
                                        color: "#0f172a"
                                        font.pixelSize: 15
                                        font.bold: true
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: modelData.summary || ""
                                        color: "#475569"
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }

                    Frame {
                        Layout.fillWidth: true
                        background: Rectangle {
                            radius: 14
                            color: "#ffffff"
                            border.color: "#e2e8f0"
                        }

                        ColumnLayout {
                            width: parent.width
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 10

                            Label {
                                text: "快速摘要"
                                color: "#0f172a"
                                font.pixelSize: 18
                                font.bold: true
                            }

                            Label {
                                text: "适合先看："
                                color: "#1e293b"
                                font.bold: true
                                visible: (analysisController.systemContextData.entrySummary || "").length > 0
                            }

                            Label {
                                text: analysisController.systemContextData.entrySummary || ""
                                color: "#475569"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                visible: text.length > 0
                            }

                            Label {
                                text: "输入 / 输出："
                                color: "#1e293b"
                                font.bold: true
                                visible: (analysisController.systemContextData.inputSummary || "").length > 0
                                         || (analysisController.systemContextData.outputSummary || "").length > 0
                            }

                            Label {
                                text: ((analysisController.systemContextData.inputSummary || "")
                                       + "\n"
                                       + (analysisController.systemContextData.outputSummary || "")).trim()
                                color: "#475569"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                visible: text.length > 0
                            }

                            Label {
                                text: "技术线索："
                                color: "#1e293b"
                                font.bold: true
                                visible: (analysisController.systemContextData.technologySummary || "").length > 0
                            }

                            Label {
                                text: analysisController.systemContextData.technologySummary || ""
                                color: "#475569"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                visible: text.length > 0
                            }

                            Label {
                                text: "容器摘要："
                                color: "#1e293b"
                                font.bold: true
                                visible: (analysisController.systemContextData.containerSummary || "").length > 0
                            }

                            Label {
                                text: analysisController.systemContextData.containerSummary || ""
                                color: "#475569"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                visible: text.length > 0
                            }
                        }
                    }

                    Frame {
                        Layout.fillWidth: true
                        background: Rectangle { radius: 14; color: "#ffffff"; border.color: "#e2e8f0" }

                        ColumnLayout {
                            width: parent.width
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 10

                            Label {
                                text: "详细 Markdown 报告"
                                color: "#0f172a"
                                font.pixelSize: 18
                                font.bold: true
                            }

                            TextArea {
                                Layout.fillWidth: true
                                text: analysisController.systemContextReport
                                readOnly: true
                                wrapMode: TextEdit.Wrap
                                textFormat: TextEdit.MarkdownText
                                color: "#0f172a"
                                font.pixelSize: 15
                                background: null
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: astPreviewPageComponent

        Frame {
            background: Rectangle { radius: 10; color: "#ffffff" }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                Frame {
                    Layout.fillWidth: true
                    background: Rectangle {
                        radius: 14
                        color: "#f8fafc"
                        border.color: "#dbe4f0"
                    }

                    ColumnLayout {
                        width: parent.width
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        Label {
                            text: "AST 预览"
                            color: "#0f172a"
                            font.pixelSize: 20
                            font.bold: true
                        }

                        Label {
                            text: analysisController.astPreviewSummary
                            color: "#475569"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: astFileSelector
                            Layout.fillWidth: true
                            model: analysisController.astFileItems
                            textRole: "label"
                            enabled: (analysisController.astFileItems || []).length > 0

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

                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    background: Rectangle {
                        radius: 14
                        color: "#0b1220"
                        border.color: "#1e293b"
                    }

                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 12
                        clip: true

                        TextArea {
                            text: analysisController.astPreviewText
                            readOnly: true
                            wrapMode: TextEdit.NoWrap
                            color: "#dbeafe"
                            font.pixelSize: 13
                            font.family: "Consolas"
                            background: null
                        }
                    }
                }
            }
        }
    }

    Component {
        id: analysisReportPageComponent

        Frame {
            background: Rectangle { radius: 10; color: "#ffffff" }
            ScrollView {
                anchors.fill: parent
                anchors.margins: 20
                TextArea {
                    text: analysisController.analysisReport
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    textFormat: TextEdit.MarkdownText
                    color: "#0f172a"
                    font.pixelSize: 15
                    background: null
                }
            }
        }
    }

    function displayNodeKind(kind) {
        return kind === "entry" ? "\u53d1\u8d77\u5165\u53e3" : (kind === "infrastructure" ? "\u540e\u53f0\u652f\u6491" : kind)
    }

    function nodeFill(kind, selected) {
        return selected ? "#3b82f6" : (kind === "entry" ? "#14b8a6" : (kind === "infrastructure" ? "#f59e0b" : "#eff6ff"))
    }

    function edgeColor(kind) {
        return kind === "activates" ? "#60a5fa" : (kind === "uses_infrastructure" ? "#fcd34d" : "#cbd5e1")
    }

    function capabilityNodeById(nodeId) {
        var nodes = analysisController.capabilityNodeItems || []
        for (var i = 0; i < nodes.length; ++i) {
            if (nodes[i].id === nodeId)
                return nodes[i]
        }
        return null
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
        return node ? node.name : ""
    }

    function nodeDisplayRect(node) {
        if (!node)
            return {"x": 0, "y": 0, "width": 0, "height": 0}
        if (node.layoutBounds)
            return node.layoutBounds
        return {"x": node.x, "y": node.y, "width": node.width, "height": node.height}
    }

    function selectedNodeDisplayName() {
        return selectedCapabilityNode ? selectedCapabilityNode.name : "\u8fd8\u6ca1\u6709\u9009\u4e2d\u4efb\u4f55\u90e8\u5206"
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

    function focusedLayoutData() {
        return {
            "useFocusedLayout": false,
            "width": Math.max(analysisController.capabilitySceneWidth, 980),
            "height": Math.max(analysisController.capabilitySceneHeight, 420),
            "positions": {}
        }
    }

    function currentCapabilitySceneWidth() {
        return focusedLayoutData().width
    }

    function currentCapabilitySceneHeight() {
        return focusedLayoutData().height
    }

    function capabilityNodeOpacity(node) {
        if (!node || !selectedCapabilityNode || selectedCapabilityNode.id === undefined)
            return 1.0
        if (selectedCapabilityNode.id === node.id)
            return 1.0

        var edges = analysisController.capabilityEdgeItems || []
        for (var i = 0; i < edges.length; ++i) {
            if ((edges[i].fromId === selectedCapabilityNode.id && edges[i].toId === node.id)
                    || (edges[i].toId === selectedCapabilityNode.id && edges[i].fromId === node.id)) {
                return 1.0
            }
        }
        return 0.35
    }

    function edgeRoutePoints(edge) {
        if (edge && edge.routePoints && edge.routePoints.length >= 2)
            return edge.routePoints
        var fromRect = nodeDisplayRect(capabilityNodeById(edge.fromId))
        var toRect = nodeDisplayRect(capabilityNodeById(edge.toId))
        if (!fromRect || !toRect)
            return []
        var leftToRight = fromRect.x <= toRect.x
        return [
            {"x": leftToRight ? fromRect.x + fromRect.width : fromRect.x,
             "y": fromRect.y + fromRect.height / 2},
            {"x": leftToRight ? toRect.x : toRect.x + toRect.width,
             "y": toRect.y + toRect.height / 2}
        ]
    }

    function edgeEndpoints(edge) {
        var points = edgeRoutePoints(edge)
        if (!points || points.length < 2)
            return {"x1": 0, "y1": 0, "x2": 0, "y2": 0}
        return {
            "x1": points[0].x,
            "y1": points[0].y,
            "x2": points[points.length - 1].x,
            "y2": points[points.length - 1].y
        }
    }

    function edgeTouchesSelection(edge) {
        return selectedCapabilityNode && (edge.fromId === selectedCapabilityNode.id || edge.toId === selectedCapabilityNode.id)
    }

    function visibleCapabilityEdges() {
        var edges = analysisController.capabilityEdgeItems || []
        if (relationshipViewMode === "all")
            return edges.slice(0, Math.min(edges.length, 150))
        if (!selectedCapabilityNode)
            return []
        var filtered = []
        for (var i = 0; i < edges.length; ++i) {
            if (edgeTouchesSelection(edges[i]))
                filtered.push(edges[i])
        }
        return filtered
    }

    function drawCapabilityEdge(ctx, edge, highlighted) {
        var points = edgeRoutePoints(edge)
        if (!points || points.length < 2)
            return

        var endpoints = edgeEndpoints(edge)
        var emphasized = edgeTouchesSelection(edge)
        var hasSelection = selectedCapabilityNode && selectedCapabilityNode.id !== undefined
        var alpha = hasSelection ? (emphasized ? 0.92 : 0.04) : 0.5
        var lineWidth = emphasized ? 3.0 : 1.2

        ctx.beginPath()
        ctx.lineJoin = "round"
        ctx.lineCap = "round"
        ctx.strokeStyle = edgeColor(edge.kind)
        ctx.lineWidth = lineWidth
        ctx.globalAlpha = alpha
        ctx.moveTo(points[0].x, points[0].y)

        if (points.length > 2) {
            for (var i = 1; i < points.length; ++i)
                ctx.lineTo(points[i].x, points[i].y)
        } else {
            var dx = Math.abs(endpoints.x2 - endpoints.x1)
            var dy = Math.abs(endpoints.y2 - endpoints.y1)
            var cpX = Math.max(60, dx * 0.5)

            if (dx < 100) {
                var sweepDir = (endpoints.x1 < 500) ? -1 : 1
                var sweepAmt = Math.max(80, dy * 0.4)
                ctx.bezierCurveTo(
                    endpoints.x1 + sweepDir * sweepAmt, endpoints.y1,
                    endpoints.x2 + sweepDir * sweepAmt, endpoints.y2,
                    endpoints.x2, endpoints.y2)
            } else {
                ctx.bezierCurveTo(
                    endpoints.x1 + cpX, endpoints.y1,
                    endpoints.x2 - cpX, endpoints.y2,
                    endpoints.x2, endpoints.y2)
            }
        }
        ctx.stroke()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        TopControlBar {
            Layout.fillWidth: true
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Frame {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                background: Rectangle { radius: 10; color: "#ffffff"; border.color: "#e2e8f0" }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: window.c4Levels
                            delegate: Rectangle {
                                width: 160
                                height: 52
                                radius: 10
                                color: window.selectedC4LevelIndex === index ? "#eff6ff" : "#ffffff"
                                border.color: window.selectedC4LevelIndex === index ? "#3b82f6" : "#e2e8f0"

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.title
                                    color: window.selectedC4LevelIndex === index ? "#1d4ed8" : "#475569"
                                    font.bold: true
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: window.selectedC4LevelIndex = index
                                }
                            }
                        }
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
                SplitView.preferredWidth: window.inspectorCollapsed ? 64 : window.inspectorExpandedWidth
                SplitView.minimumWidth: window.inspectorCollapsed ? 64 : 320
                SplitView.fillHeight: true
            }
        }
    }
}


