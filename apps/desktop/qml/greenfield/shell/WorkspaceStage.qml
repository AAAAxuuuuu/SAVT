import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../atlas"
import "../config"
import "../foundation"
import "../report"

Item {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState
    property var componentFileDetail: ({})
    readonly property bool componentNodeDetailActive: root.caseState.route === "component"
                                                      && !!root.focusState.focusedNode
                                                      && root.focusState.focusedNode.id !== undefined
    readonly property bool componentFileDetailActive: root.componentNodeDetailActive
                                                      && !!root.componentFileDetail
                                                      && !!root.componentFileDetail.available
                                                      && !!root.componentFileDetail.singleFile

    signal chooseProjectRequested()

    function normalizedPreviewText(rawText, fallbackText) {
        var text = String(rawText || "").trim()
        if (text.length === 0)
            return fallbackText || ""

        var stopMarkers = [
            "\n👉",
            "👉",
            "核心类/函数",
            "核心函数",
            "Core classes/functions",
            "Core classes",
            "Top symbols"
        ]
        for (var i = 0; i < stopMarkers.length; ++i) {
            var markerIndex = text.indexOf(stopMarkers[i])
            if (markerIndex > 0) {
                text = text.slice(0, markerIndex).trim()
                break
            }
        }

        text = text.replace(/\s+/g, " ").trim()
        return text.length > 0 ? text : (fallbackText || "")
    }

    function componentSceneForFocus() {
        if (!root.focusState.focusedCapability || root.focusState.focusedCapability.id === undefined)
            return ({})
        var catalog = root.analysisController.componentSceneCatalog || ({})
        var id = String(root.focusState.focusedCapability.id)
        return catalog[id] || catalog[root.focusState.focusedCapability.id] || ({})
    }

    function refreshComponentFileDetail() {
        if (root.caseState.route !== "component"
                || !root.focusState.focusedNode
                || root.focusState.focusedNode.id === undefined) {
            root.componentFileDetail = ({})
            return
        }
        root.componentFileDetail = root.analysisController.describeFileNode(root.focusState.focusedNode)
    }

    function openComponentLab(node) {
        if (node && node.id !== undefined) {
            root.focusState.prepareComponentDrill(node)
            root.analysisController.ensureComponentSceneForCapability(node.id)
        }
        root.caseState.navigate("component")
    }

    Connections {
        target: root.caseState

        function onRouteChanged() {
            if (root.caseState.route === "component") {
                if (root.focusState.focusedNode &&
                        root.focusState.focusedCapability &&
                        root.focusState.focusedNode.id === root.focusState.focusedCapability.id) {
                    root.focusState.rememberOverviewCapability(root.focusState.focusedCapability)
                }
                root.focusState.clearNodeFocus()
            }

            if (root.caseState.route === "overview" && root.focusState.restoreOverviewFocusPending)
                root.focusState.restoreOverviewCapabilityFocus()

            if (root.caseState.route === "component" &&
                    root.focusState.focusedCapability &&
                    root.focusState.focusedCapability.id !== undefined) {
                root.analysisController.ensureComponentSceneForCapability(
                            root.focusState.focusedCapability.id)
            }

            root.refreshComponentFileDetail()
        }
    }

    Connections {
        target: root.focusState

        function onFocusedNodeChanged() { root.refreshComponentFileDetail() }
    }

    Connections {
        target: root.analysisController

        function onProjectRootPathChanged() { root.refreshComponentFileDetail() }
    }

    Component.onCompleted: root.refreshComponentFileDetail()

    Loader {
        anchors.fill: parent
        sourceComponent: root.caseState.route === "component" ? componentView
                         : root.caseState.route === "report" ? reportView
                         : root.caseState.route === "config" ? configView
                         : overviewView
    }

    Component {
        id: overviewView

        Item {
            id: overviewScroll

            readonly property var guide: root.analysisController.systemContextData || ({})
            readonly property var topModules: guide.topModules || []
            readonly property var algorithmSummary: guide.algorithmSummary || ({})
            readonly property bool overviewBusy: !!guide.projectOverviewBusy

            function cleanText(value) {
                return String(value || "").replace(/\s+/g, " ").trim()
            }

            function overviewSummary() {
                return cleanText(algorithmSummary.summary || guide.projectOverview || "")
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Rectangle {
                    visible: root.caseState.hasProject
                    Layout.fillWidth: true
                    radius: root.tokens.radius8
                    color: root.tokens.panelStrong
                    border.color: root.tokens.border1
                    implicitHeight: heroColumn.implicitHeight + 24

                    ColumnLayout {
                        id: heroColumn
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "Architecture Reconstruction"
                                    color: root.tokens.signalCobalt
                                    font.family: root.tokens.textFontFamily
                                    font.pixelSize: 12
                                    font.weight: Font.DemiBold
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: cleanText(guide.projectName || root.caseState.projectName())
                                    color: root.tokens.text1
                                    font.family: root.tokens.displayFontFamily
                                    font.pixelSize: 26
                                    font.weight: Font.DemiBold
                                    wrapMode: Text.WordWrap
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: cleanText(algorithmSummary.headline || "从源码事实到多粒度架构视图")
                                    color: root.tokens.text1
                                    wrapMode: Text.WordWrap
                                    font.family: root.tokens.displayFontFamily
                                    font.pixelSize: 18
                                    font.weight: Font.DemiBold
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: overviewSummary()
                                    visible: text.length > 0
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                    color: root.tokens.text2
                                    font.family: root.tokens.textFontFamily
                                    font.pixelSize: 12
                                    lineHeight: 1.16
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 196
                                spacing: 6

                                ActionButton {
                                    Layout.fillWidth: true
                                    tokens: root.tokens
                                    text: root.analysisController.analyzing ? "停止重建" : "重新重建"
                                    compact: true
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
                                    Layout.fillWidth: true
                                    tokens: root.tokens
                                    text: "高精度重建"
                                    compact: true
                                    tone: "secondary"
                                    enabled: root.caseState.hasProject && !root.analysisController.analyzing
                                    onClicked: root.analysisController.analyzeCurrentProjectHighPrecision()
                                }

                                ActionButton {
                                    Layout.fillWidth: true
                                    tokens: root.tokens
                                    text: overviewBusy ? "刷新摘要中..." : "刷新算法摘要"
                                    tone: "ai"
                                    compact: true
                                    enabled: root.analysisController.aiAvailable && !overviewBusy
                                    onClicked: root.analysisController.requestProjectOverviewRefresh()
                                }
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 6

                            PillChip {
                                tokens: root.tokens
                                text: root.analysisController.analyzing
                                      ? ((root.analysisController.analysisPhase || "重建中")
                                         + " · " + Math.round(root.analysisController.analysisProgress) + "%")
                                      : String(algorithmSummary.modeLine || root.caseState.trustLabel())
                                tone: root.caseState.trustTone()
                            }

                            PillChip {
                                tokens: root.tokens
                                text: String(algorithmSummary.cacheLine || "")
                                tone: "moss"
                            }

                            PillChip {
                                tokens: root.tokens
                                text: String(algorithmSummary.evidenceLine || "")
                                tone: "ai"
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            visible: topModules.length > 0
                            spacing: 6

                            Repeater {
                                model: topModules

                                PillChip {
                                    required property var modelData

                                    tokens: root.tokens
                                    text: String(modelData || "")
                                    tone: "info"
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 440
                    radius: root.tokens.radius8
                    color: Qt.rgba(root.tokens.panelStrong.r, root.tokens.panelStrong.g, root.tokens.panelStrong.b, 0.7)
                    border.color: root.tokens.border1
                    clip: true

                    ColumnLayout {
                        id: canvasColumn
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 392
                            radius: root.tokens.radius8
                            color: Qt.rgba(root.tokens.base0.r, root.tokens.base0.g, root.tokens.base0.b, 0.82)
                            border.color: root.tokens.border1
                            clip: true

                            ArchitectureCanvas {
                                anchors.fill: parent
                                anchors.margins: 4
                                tokens: root.tokens
                                scene: root.analysisController.capabilityScene || ({})
                                selectedNode: root.focusState.focusedNode
                                emptyText: root.analysisController.analyzing
                                           ? "正在重建能力全景，画布会随着布局结果逐步刷新..."
                                           : "选择项目并运行重建后，这里会显示 L2 能力域全景图。"
                                onNodeSelected: root.focusState.setCapability(node)
                                onNodeDrilled: root.openComponentLab(node)
                                onBlankClicked: root.focusState.clear()
                            }

                            StartOverlay {
                                anchors.centerIn: parent
                                visible: !root.caseState.hasProject || (!root.caseState.hasAtlas && !root.analysisController.analyzing)
                                tokens: root.tokens
                                analysisController: root.analysisController
                                caseState: root.caseState
                                onChooseProjectRequested: root.chooseProjectRequested()
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: componentView

        Item {
            Loader {
                anchors.fill: parent
                sourceComponent: root.componentNodeDetailActive
                                 ? (root.componentFileDetailActive ? componentFileView : componentNodeView)
                                 : componentCanvasView
            }

            Rectangle {
                visible: !root.componentNodeDetailActive
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 18
                width: Math.min(parent.width - 36, 560)
                height: Math.max(88, componentHeaderLayout.implicitHeight + 24)
                radius: root.tokens.radius8
                color: root.tokens.panelStrong
                border.color: root.tokens.border1

                RowLayout {
                    id: componentHeaderLayout
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10
                    layoutDirection: Qt.LeftToRight

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Label {
                            Layout.fillWidth: true
                            text: root.componentFileDetailActive
                                  ? ("文件证据钻取 · " + (root.componentFileDetail.fileName
                                                     || (root.focusState.focusedNode
                                                         ? root.focusState.focusedNode.name
                                                         : "未命名文件")))
                                  : root.componentNodeDetailActive
                                    ? ("节点证据钻取 · " + (root.focusState.focusedNode
                                                     ? root.focusState.focusedNode.name
                                                     : "未命名节点"))
                                  : (root.focusState.focusedCapability
                                     ? ("边界钻取 · " + root.focusState.focusedCapability.name)
                                     : "边界钻取工作台")
                            color: root.tokens.text1
                            elide: Text.ElideRight
                            font.family: root.tokens.displayFontFamily
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.normalizedPreviewText(
                                      root.componentFileDetailActive
                                      ? root.componentFileDetail.summary
                                      : root.componentNodeDetailActive
                                        ? (root.focusState.focusedNode.summary
                                           || root.focusState.focusedNode.responsibility
                                           || root.focusState.focusedNode.role)
                                      : root.componentSceneForFocus().summary,
                                      root.componentFileDetailActive
                                      ? "当前已切换到文件解读模式，会结合路径、声明、依赖和片段来理解这个文件。"
                                      : root.componentNodeDetailActive
                                        ? "当前已切换到节点证据钻取模式，会围绕职责、关系和证据包解释这个节点为什么被划入当前能力域。"
                                      : "双击全景图中的能力域后，SAVT 会展开 L3 组件图，帮助继续验证边界与责任分配。")
                            color: root.tokens.text3
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 12
                            lineHeight: 1.15
                        }
                    }

                    ActionButton {
                        tokens: root.tokens
                        text: root.componentNodeDetailActive ? "返回钻取图" : "返回全景"
                        tone: "secondary"
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: {
                            if (root.componentNodeDetailActive) {
                                root.focusState.clearNodeFocus()
                            } else {
                                root.caseState.navigate("overview")
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: componentCanvasView

        ArchitectureCanvas {
            anchors.fill: parent
            tokens: root.tokens
            scene: root.componentSceneForFocus()
            selectedNode: root.focusState.focusedNode
            componentMode: true
            emptyText: root.focusState.focusedCapability
                       ? "正在准备该能力域的组件图，或当前能力域暂时没有可继续钻取的组件。"
                       : "请在重建全景图中双击一个能力域进入边界钻取。"
            onNodeSelected: root.focusState.setNode(node)
            onNodeDrilled: root.focusState.setNode(node)
            onBlankClicked: root.focusState.clearNodeFocus()
        }
    }

    Component {
        id: componentNodeView

        FocusBrief {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            anchors.topMargin: 18
            anchors.bottomMargin: 18
            tokens: root.tokens
            analysisController: root.analysisController
            caseState: root.caseState
            focusState: root.focusState
            embeddedMode: true
        }
    }

    Component {
        id: componentFileView

        FileFocusView {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            anchors.topMargin: 18
            anchors.bottomMargin: 18
            tokens: root.tokens
            analysisController: root.analysisController
            focusState: root.focusState
            scene: root.componentSceneForFocus()
            fileDetail: root.componentFileDetail
        }
    }

    Component {
        id: reportView

        ReportDesk {
            anchors.fill: parent
            anchors.margins: 18
            tokens: root.tokens
            analysisController: root.analysisController
            caseState: root.caseState
            focusState: root.focusState
        }
    }

    Component {
        id: configView

        ConfigDesk {
            anchors.fill: parent
            anchors.margins: 18
            tokens: root.tokens
            analysisController: root.analysisController
            caseState: root.caseState
            focusState: root.focusState
            onChooseProjectRequested: root.chooseProjectRequested()
        }
    }

    component PillChip: Rectangle {
        required property QtObject tokens
        property string text: ""
        property string tone: "info"

        visible: text.length > 0
        radius: height / 2
        implicitHeight: 30
        implicitWidth: chipLabel.implicitWidth + 18
        color: tokens.toneSoft(tone)
        border.color: Qt.rgba(tokens.toneColor(tone).r,
                              tokens.toneColor(tone).g,
                              tokens.toneColor(tone).b,
                              0.24)

        Label {
            id: chipLabel
            anchors.centerIn: parent
            text: parent.text
            color: tokens.toneColor(tone)
            font.family: tokens.textFontFamily
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }
    }

    component StartOverlay: Rectangle {
        id: startOverlay
        required property QtObject tokens
        required property QtObject analysisController
        required property QtObject caseState

        signal chooseProjectRequested()

        width: 520
        height: 240
        radius: tokens.radius8
        color: Qt.rgba(tokens.panelStrong.r, tokens.panelStrong.g, tokens.panelStrong.b, 1.0)
        border.color: tokens.border1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 14

            Label {
                text: "开始架构重建"
                color: tokens.text1
                font.family: tokens.displayFontFamily
                font.pixelSize: 26
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                text: caseState.hasProject
                      ? "项目已选择，但还没有重建结果。点击开始后，SAVT 会依次执行源码扫描、事实提取、能力聚合、证据校准和多层视图输出。"
                      : "选择一个 C/C++ 项目目录，SAVT 会把源码事实转成 L1/L2/L3 架构视图、证据链和可继续钻取的边界结果。"
                wrapMode: Text.WordWrap
                color: tokens.text3
                font.family: tokens.textFontFamily
                font.pixelSize: 13
            }

            RowLayout {
                ActionButton {
                    tokens: startOverlay.tokens
                    text: caseState.hasProject ? "切换项目" : "选择项目"
                    tone: "secondary"
                    onClicked: chooseProjectRequested()
                }

                ActionButton {
                    tokens: startOverlay.tokens
                    text: analysisController.analyzing ? "重建中..." : "开始重建"
                    tone: "primary"
                    enabled: caseState.hasProject && !analysisController.analyzing
                    onClicked: analysisController.analyzeCurrentProject()
                }

                ActionButton {
                    tokens: startOverlay.tokens
                    text: "高精度重建"
                    tone: "secondary"
                    enabled: caseState.hasProject && !analysisController.analyzing
                    onClicked: analysisController.analyzeCurrentProjectHighPrecision()
                }
            }
        }
    }
}
