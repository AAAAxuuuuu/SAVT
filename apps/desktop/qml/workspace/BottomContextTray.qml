import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject uiState
    required property QtObject analysisController

    implicitHeight: root.uiState.contextTrayExpanded ? 176 : 44

    Behavior on implicitHeight {
        NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
    }

    function traySummary() {
        if (root.uiState.contextTab === "evidence")
            return "把事实、规则和结论收纳在底部，需要时再展开。"
        if (root.uiState.contextTab === "ai")
            return "承接 AI 摘要和下一步建议，不和正文抢主视线。"
        return root.uiState.inspector.title.length > 0
               ? ("当前围绕「" + root.uiState.inspector.title + "」补充关系和上下文。")
               : "优先放补充上下文和关系线索，避免主工作区被次要信息挤占。"
    }

    AppCard {
        anchors.fill: parent
        fillColor: "#FFFFFF"
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXxl
        contentPadding: root.uiState.contextTrayExpanded ? 14 : 10

        ColumnLayout {
            anchors.fill: parent
            spacing: root.uiState.contextTrayExpanded ? 10 : 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: "上下文托盘"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: root.uiState.contextTrayExpanded ? 16 : 14
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.traySummary()
                        wrapMode: Text.WordWrap
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: root.uiState.contextTrayExpanded ? 11 : 10
                    }
                }

                SegmentedControl {
                    theme: root.theme
                    model: [
                        {"value": "context", "label": "上下文"},
                        {"value": "evidence", "label": "证据"},
                        {"value": "ai", "label": "AI"}
                    ]
                    currentValue: root.uiState.contextTab
                    onActivated: root.uiState.contextTab = value
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: root.uiState.contextTrayExpanded ? "收起" : "展开"
                    onClicked: root.uiState.contextTrayExpanded = !root.uiState.contextTrayExpanded
                }
            }

            Loader {
                Layout.fillWidth: true
                Layout.fillHeight: true
                active: root.uiState.contextTrayExpanded
                visible: root.uiState.contextTrayExpanded
                sourceComponent: root.uiState.contextTab === "evidence"
                                 ? evidenceComponent
                                 : (root.uiState.contextTab === "ai" ? aiComponent : contextComponent)
            }
        }
    }

    Component {
        id: contextComponent

        ScrollView {
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 8

                Repeater {
                    model: root.uiState.navigation.pageId === "project.overview"
                           ? (root.analysisController.systemContextCards || [])
                           : root.uiState.inspector.relationshipItems.slice(0, 4)

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
                                text: root.uiState.navigation.pageId === "project.overview"
                                      ? (modelData.name || "")
                                      : (modelData.summary || "")
                                color: root.theme.inkStrong
                                font.family: root.theme.displayFontFamily
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.uiState.navigation.pageId === "project.overview"
                                      ? (modelData.summary || "")
                                      : ("权重 " + (modelData.weight || 0) + " · " + (modelData.kind || ""))
                                wrapMode: Text.WordWrap
                                color: root.theme.inkMuted
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }
                        }
                    }
                }

                Label {
                    visible: root.uiState.navigation.pageId !== "project.overview"
                             && root.uiState.inspector.relationshipItems.length === 0
                    text: "当前对象没有更多可展示的关系上下文。"
                    color: root.theme.inkMuted
                    font.family: root.theme.textFontFamily
                    font.pixelSize: 12
                }
            }
        }
    }

    Component {
        id: evidenceComponent

        ScrollView {
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 8

                Repeater {
                    model: [
                        {"title": "事实", "items": root.uiState.inspector.facts},
                        {"title": "规则", "items": root.uiState.inspector.rules},
                        {"title": "结论", "items": root.uiState.inspector.conclusions}
                    ]

                    AppCard {
                        Layout.fillWidth: true
                        fillColor: root.theme.surfaceSecondary
                        borderColor: root.theme.borderSubtle
                        cornerRadius: root.theme.radiusLg
                        minimumContentHeight: 72
                        visible: (modelData.items || []).length > 0

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 6

                            Label {
                                text: modelData.title
                                color: root.theme.inkStrong
                                font.family: root.theme.displayFontFamily
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: (modelData.items || []).slice(0, 3).join("\n")
                                wrapMode: Text.WordWrap
                                color: root.theme.inkMuted
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }
                        }
                    }
                }

                Label {
                    visible: root.uiState.inspector.facts.length === 0
                             && root.uiState.inspector.rules.length === 0
                             && root.uiState.inspector.conclusions.length === 0
                    text: "当前对象还没有可展开的证据条目。"
                    color: root.theme.inkMuted
                    font.family: root.theme.textFontFamily
                    font.pixelSize: 12
                }
            }
        }
    }

    Component {
        id: aiComponent

        ScrollView {
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 8

                AppCard {
                    Layout.fillWidth: true
                    fillColor: root.theme.surfaceSecondary
                    borderColor: root.theme.borderSubtle
                    cornerRadius: root.theme.radiusLg
                    minimumContentHeight: 72

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6

                        Label {
                            text: "AI 摘要"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.analysisController.aiHasResult
                                  ? root.analysisController.aiSummary
                                  : "还没有 AI 输出，可以从顶部栏或右侧面板打开 Ask SAVT。"
                            wrapMode: Text.WordWrap
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }
                }

                Repeater {
                    model: root.analysisController.aiNextActions || []

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
}
