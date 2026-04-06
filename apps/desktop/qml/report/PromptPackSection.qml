import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject uiState

    implicitHeight: card.implicitHeight

    function promptItems() {
        var focusTitle = root.uiState.inspector.title.length > 0 ? root.uiState.inspector.title : "当前项目"
        return [
            {
                "title": "架构重构提示词",
                "summary": "面向 AI 继续改造工程结构、模块边界和依赖切口。",
                "prompt": "请基于当前 SAVT 分析结果，为「" + focusTitle + "」生成一份可执行的架构重构方案，要求给出目标边界、拆分顺序、风险点和验证步骤。"
            },
            {
                "title": "问题定位提示词",
                "summary": "面向 AI 解释依赖链、职责冲突或技术债来源。",
                "prompt": "请结合当前静态分析证据，解释「" + focusTitle + "」为什么重要、它与上下游如何协作、当前最值得优先核对的风险或不确定性是什么。"
            },
            {
                "title": "交接说明提示词",
                "summary": "把当前理解转成可以交给外部代理或团队成员的工作描述。",
                "prompt": "请把「" + focusTitle + "」整理成一份交接说明，包含背景、职责边界、关键文件、依赖关系、建议的第一步修改点和验收标准。"
            }
        ]
    }

    AppCard {
        id: card
        anchors.fill: parent
        fillColor: "#FFFFFF"
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXxl

        AppSection {
            anchors.fill: parent
            theme: root.theme
            eyebrow: "提示词包"
            title: "AI 提示词包"
            subtitle: "把当前分析结果压缩成可复制、可继续执行的提示词模板，减少来回整理上下文的成本。"

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: root.promptItems()

                    AppCard {
                        Layout.fillWidth: true
                        fillColor: root.theme.surfaceSecondary
                        borderColor: root.theme.borderSubtle
                        cornerRadius: root.theme.radiusLg
                        minimumContentHeight: 116

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.title
                                    color: root.theme.inkStrong
                                    font.family: root.theme.displayFontFamily
                                    font.pixelSize: 16
                                    font.weight: Font.DemiBold
                                }

                                AppButton {
                                    theme: root.theme
                                    compact: true
                                    tone: "ghost"
                                    text: "复制"
                                    onClicked: root.analysisController.copyTextToClipboard(modelData.prompt)
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.summary
                                wrapMode: Text.WordWrap
                                color: root.theme.inkMuted
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.prompt
                                wrapMode: Text.WordWrap
                                color: root.theme.inkNormal
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }
                        }
                    }
                }
            }
        }
    }
}
