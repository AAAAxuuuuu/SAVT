import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../atlas"
import "../config"
import "../report"

Item {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState

    signal chooseProjectRequested()

    function componentSceneForFocus() {
        if (!root.focusState.focusedCapability || root.focusState.focusedCapability.id === undefined)
            return ({})
        var catalog = root.analysisController.componentSceneCatalog || ({})
        var id = String(root.focusState.focusedCapability.id)
        return catalog[id] || catalog[root.focusState.focusedCapability.id] || ({})
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
        }
    }

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

            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 18
                width: Math.min(parent.width - 36, 520)
                height: 74
                radius: root.tokens.radius8
                color: root.tokens.panelBase
                border.color: root.tokens.border1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Label {
                            Layout.fillWidth: true
                            text: root.focusState.focusedCapability ? ("组件探测：" + root.focusState.focusedCapability.name) : "组件探测实验室"
                            color: root.tokens.text1
                            elide: Text.ElideRight
                            font.family: root.tokens.displayFontFamily
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.componentSceneForFocus().summary || "双击全景图节点后，SAVT 会接入 componentSceneCatalog 展开组件关系。"
                            color: root.tokens.text3
                            elide: Text.ElideRight
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 12
                        }
                    }

                    Button {
                        text: "返回全景"
                        onClicked: root.caseState.navigate("overview")
                    }
                }
            }
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
                Button {
                    text: caseState.hasProject ? "切换项目" : "选择项目"
                    onClicked: chooseProjectRequested()
                }

                Button {
                    text: analysisController.analyzing ? "分析中..." : "开始分析"
                    enabled: caseState.hasProject && !analysisController.analyzing
                    onClicked: analysisController.analyzeCurrentProject()
                }
            }
        }
    }
}
