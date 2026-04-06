import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject uiState

    property string levelId: "l1"

    signal askRequested()
    signal navigateTo(string pageId)

    anchors.fill: parent

    function levelTitle() {
        return "专业入口说明"
    }

    function levelSummary() {
        if (root.levelId === "l1")
            return "你当前走的是 L1 专业入口，用系统上下文直接建立整体认知，而不是从前门总览开始。"
        if (root.levelId === "l2")
            return "你当前走的是 L2 专业入口，会直接进入能力关系视角，适合已经明确要做结构判断的场景。"
        if (root.levelId === "l3")
            return "你当前走的是 L3 专业入口，会保留父能力上下文，直接进入组件级工作台。"
        return "你当前走的是 L4 专业入口，适合从完整报告和原始 Markdown 直接回看全局结论。"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        AppCard {
            Layout.fillWidth: true
            fillColor: "#FFFFFF"
            borderColor: root.theme.borderSubtle
            cornerRadius: root.theme.radiusXxl
            minimumContentHeight: 108

            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: root.levelTitle()
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 22
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.levelSummary()
                        wrapMode: Text.WordWrap
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: "回到项目入口"
                    onClicked: root.navigateTo("project.overview")
                }
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            active: true
            sourceComponent: root.levelId === "l1"
                             ? l1Component
                             : (root.levelId === "l2"
                                ? l2Component
                                : (root.levelId === "l3" ? l3Component : l4Component))
        }
    }

    Component {
        id: l1Component

        ProjectOverviewPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            pageMode: "l1"
            onAskRequested: root.askRequested()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: l2Component

        CapabilityMapPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            onAskRequested: root.askRequested()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: l3Component

        ComponentWorkbenchPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            onAskRequested: root.askRequested()
            onNavigateTo: root.navigateTo(pageId)
        }
    }

    Component {
        id: l4Component

        ReportPage {
            theme: root.theme
            analysisController: root.analysisController
            uiState: root.uiState
            mode: "engineering"
            onAskRequested: root.askRequested()
            onNavigateTo: root.navigateTo(pageId)
        }
    }
}
