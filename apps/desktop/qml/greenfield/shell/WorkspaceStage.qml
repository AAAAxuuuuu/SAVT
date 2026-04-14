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
    readonly property bool componentFileDetailActive: root.caseState.route === "component"
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
            ArchitectureCanvas {
                anchors.fill: parent
                tokens: root.tokens
                scene: root.analysisController.capabilityScene || ({})
                selectedNode: root.focusState.focusedNode
                emptyText: root.analysisController.analyzing
                           ? "分析进行中，能力地图即将生成..."
                           : "选择项目并运行分析后，这里会显示架构全景图。"
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

    Component {
        id: componentView

        Item {
            Loader {
                anchors.fill: parent
                sourceComponent: root.componentFileDetailActive ? componentFileView : componentCanvasView
            }

            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 18
                width: Math.min(parent.width - 36, 560)
                height: Math.max(88, componentHeaderLayout.implicitHeight + 24)
                radius: root.tokens.radius8
                color: root.tokens.panelBase
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
                                  ? ("文件解读：" + (root.componentFileDetail.fileName
                                                     || (root.focusState.focusedNode
                                                         ? root.focusState.focusedNode.name
                                                         : "未命名文件")))
                                  : (root.focusState.focusedCapability
                                     ? ("组件探测：" + root.focusState.focusedCapability.name)
                                     : "组件探测实验室")
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
                                      : root.componentSceneForFocus().summary,
                                      root.componentFileDetailActive
                                      ? "当前已切换到文件解读模式，会结合路径、声明、依赖和片段来理解这个文件。"
                                      : "双击全景图节点后，SAVT 会接入 componentSceneCatalog 展开组件关系。")
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
                        text: "返回全景"
                        tone: "secondary"
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: root.caseState.navigate("overview")
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
                       ? "正在准备该能力域的组件图，或当前能力域暂无可下钻组件。"
                       : "请在架构全景图中双击一个能力域以下钻。"
            onNodeSelected: root.focusState.setNode(node)
            onNodeDrilled: root.focusState.setNode(node)
            onBlankClicked: root.focusState.clearNodeFocus()
        }
    }

    Component {
        id: componentFileView

        FileFocusView {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            anchors.topMargin: 122
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

    component StartOverlay: Rectangle {
        id: startOverlay
        required property QtObject tokens
        required property QtObject analysisController
        required property QtObject caseState

        signal chooseProjectRequested()

        width: 520
        height: 240
        radius: tokens.radius8
        color: tokens.panelBase
        border.color: tokens.border1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 14

            Label {
                text: "开始架构分析"
                color: tokens.text1
                font.family: tokens.displayFontFamily
                font.pixelSize: 26
                font.weight: Font.DemiBold
            }

            Label {
                Layout.fillWidth: true
                text: caseState.hasProject
                      ? "项目已选择，但还没有分析结果。点击开始分析后会生成架构全景图、组件探测数据、报告和 AST 预览。"
                      : "选择一个项目文件夹，SAVT 会把源码事实转成架构图、证据链和可行动的诊断报告。"
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
                    text: analysisController.analyzing ? "分析中..." : "开始分析"
                    tone: "primary"
                    enabled: caseState.hasProject && !analysisController.analyzing
                    onClicked: analysisController.analyzeCurrentProject()
                }
            }
        }
    }
}
