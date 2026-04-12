import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject uiState

    signal capabilityMapRequested()
    signal askRequested()
    signal promptPackRequested()
    signal compareRequested()

    implicitHeight: card.implicitHeight

    AppCard {
        id: card
        anchors.fill: parent
        fillColor: "#FFFFFF"
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXxl

        AppSection {
            anchors.fill: parent
            theme: root.theme
            eyebrow: "动作"
            title: "深入阅读建议与下一步动作"
            subtitle: root.uiState.inspector.nextStepSummary.length > 0
                      ? root.uiState.inspector.nextStepSummary
                      : "从报告直接回跳到能力地图、高级对比或 AI 导览，把理解尽快转成下一步动作。"

            Flow {
                Layout.fillWidth: true
                spacing: 8
                visible: (root.analysisController.aiNextActions || []).length > 0

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

            Flow {
                Layout.fillWidth: true
                spacing: 10

                AppButton {
                    theme: root.theme
                    tone: "accent"
                    text: "进入能力地图"
                    onClicked: root.capabilityMapRequested()
                }

                AppButton {
                    theme: root.theme
                    tone: "ai"
                    text: "打开 AI 导览"
                    onClicked: root.askRequested()
                }

                AppButton {
                    theme: root.theme
                    tone: "ghost"
                    text: "打开高级对比"
                    onClicked: root.compareRequested()
                }

                AppButton {
                    theme: root.theme
                    tone: "ghost"
                    text: "AI 提示词包"
                    onClicked: root.promptPackRequested()
                }

                AppButton {
                    theme: root.theme
                    tone: "ghost"
                    text: "复制工程报告"
                    onClicked: root.analysisController.copyTextToClipboard(root.analysisController.analysisReport)
                }
            }
        }
    }
}
