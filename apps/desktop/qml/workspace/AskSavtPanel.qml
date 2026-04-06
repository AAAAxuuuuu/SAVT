import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject uiState
    required property QtObject analysisController

    property string promptText: ""

    signal closeRequested()

    function buildResponseDigest() {
        var sections = []
        if (root.analysisController.aiSummary.length > 0)
            sections.push(root.analysisController.aiSummary)
        if (root.analysisController.aiResponsibility.length > 0)
            sections.push("详细职责:\n" + root.analysisController.aiResponsibility)
        if ((root.analysisController.aiNextActions || []).length > 0)
            sections.push("建议动作:\n- " + root.analysisController.aiNextActions.join("\n- "))
        if (root.analysisController.aiUncertainty.length > 0)
            sections.push("不确定性:\n" + root.analysisController.aiUncertainty)
        return sections.join("\n\n")
    }

    function focusLabel() {
        if (root.uiState.inspector.kind === "node")
            return root.uiState.inspector.title
        if (root.uiState.inspector.kind === "edge")
            return "当前关系"
        return "当前项目"
    }

    AppCard {
        anchors.fill: parent
        fillColor: root.theme.aiWash
        borderColor: root.theme.borderStrong
        cornerRadius: root.theme.radiusXxl
        contentPadding: 16
        stacked: false

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            RowLayout {
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: "Ask SAVT"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 24
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.uiState.inspector.kind === "node"
                              ? ("当前将围绕「" + root.uiState.inspector.title + "」生成补充解释或改造建议。")
                              : "可以围绕当前项目或当前选中关系生成补充解释、改造建议和可复制提示词。"
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
                    text: "关闭"
                    onClicked: root.closeRequested()
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                AppChip {
                    text: "焦点 " + root.focusLabel()
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: root.analysisController.aiAvailable ? "AI 已就绪" : "AI 待配置"
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: root.analysisController.aiBusy ? "正在生成" : "可即时追问"
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: [
                        "解释当前对象的职责边界",
                        "给出下一步改造建议",
                        "生成可以交给外部代理的提示词"
                    ]

                    AppButton {
                        theme: root.theme
                        compact: true
                        tone: "ghost"
                        text: modelData
                        onClicked: {
                            root.promptText = modelData
                            promptEditor.text = modelData
                        }
                    }
                }
            }

            TextArea {
                id: promptEditor
                Layout.fillWidth: true
                Layout.preferredHeight: 108
                text: root.promptText
                placeholderText: "补充你想问 SAVT 的问题，例如：解释这条协作链为什么关键，并给出下一步最值得验证的改造切口。"
                wrapMode: TextEdit.Wrap
                font.family: root.theme.textFontFamily
                font.pixelSize: 13
                selectByMouse: true
                onTextChanged: root.promptText = text

                background: Rectangle {
                    radius: root.theme.radiusLg
                    color: root.theme.surfaceSecondary
                    border.color: promptEditor.activeFocus ? root.theme.focusRing : root.theme.borderSubtle
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ai"
                    text: root.analysisController.aiBusy ? "生成中..." : "解释当前项目"
                    enabled: root.analysisController.aiAvailable && !root.analysisController.aiBusy
                    onClicked: root.analysisController.requestProjectAiExplanation(root.promptText)
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "accent"
                    text: root.analysisController.aiBusy ? "生成中..." : "解释当前对象"
                    enabled: root.uiState.inspector.kind === "node" &&
                             root.analysisController.aiAvailable &&
                             !root.analysisController.aiBusy
                    onClicked: root.analysisController.requestAiExplanation(root.uiState.inspector.selectedNode, root.promptText)
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: "复制结果"
                    enabled: root.analysisController.aiHasResult && !root.analysisController.aiBusy
                    onClicked: root.analysisController.copyTextToClipboard(root.buildResponseDigest())
                }
            }

            AppCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusLg

                ScrollView {
                    anchors.fill: parent
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width
                        spacing: 10

                        Label {
                            Layout.fillWidth: true
                            text: root.analysisController.aiAvailable
                                  ? root.analysisController.aiStatusMessage
                                  : root.analysisController.aiSetupMessage
                            wrapMode: Text.WordWrap
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: root.analysisController.aiBusy
                            text: "正在基于当前静态证据生成补充说明。"
                            wrapMode: Text.WordWrap
                            color: root.theme.aiAccent
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: root.analysisController.aiHasResult && !root.analysisController.aiBusy
                            text: root.analysisController.aiSummary
                            wrapMode: Text.WordWrap
                            color: root.theme.inkStrong
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: root.analysisController.aiResponsibility.length > 0
                            text: root.analysisController.aiResponsibility
                            wrapMode: Text.WordWrap
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
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

                        Label {
                            Layout.fillWidth: true
                            visible: root.analysisController.aiUncertainty.length > 0
                            text: root.analysisController.aiUncertainty
                            wrapMode: Text.WordWrap
                            color: root.theme.warning
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}
