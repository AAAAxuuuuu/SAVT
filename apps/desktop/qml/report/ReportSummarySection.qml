import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject selectionState

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
            eyebrow: "摘要"
            title: "项目摘要与分析上下文"
            subtitle: root.analysisController.systemContextData.headline || "在同一页里保留项目定位、分析状态和进入下一步的基础上下文。"

            Flow {
                Layout.fillWidth: true
                spacing: 8

                StatusBadge {
                    theme: root.theme
                    text: root.analysisController.analyzing ? "分析进行中" : "分析已完成"
                    tone: root.analysisController.analyzing ? "info" : "success"
                }

                AppChip {
                    text: "进度 " + Math.round((root.analysisController.analysisProgress || 0) * 100) + "%"
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: root.analysisController.analysisPhase.length > 0
                          ? root.analysisController.analysisPhase
                          : "等待分析阶段"
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: "能力域 " + root.selectionState.capabilityNodes.length
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: "容器 " + ((root.analysisController.systemContextData.containerNames || []).length)
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: width > 1080 ? 4 : (width > 760 ? 2 : 1)
                columnSpacing: 12
                rowSpacing: 12

                Repeater {
                    model: [
                        {"title": "项目定位", "body": root.analysisController.systemContextData.purposeSummary || root.analysisController.statusMessage},
                        {"title": "发起入口", "body": root.analysisController.systemContextData.entrySummary || "等待分析结果生成入口摘要。"},
                        {"title": "输入 / 输出", "body": ((root.analysisController.systemContextData.inputSummary || "") + "\n" + (root.analysisController.systemContextData.outputSummary || "")).trim()},
                        {"title": "技术线索", "body": root.analysisController.systemContextData.technologySummary || "等待分析结果生成技术摘要。"}
                    ]

                    AppCard {
                        Layout.fillWidth: true
                        fillColor: root.theme.surfaceSecondary
                        borderColor: root.theme.borderSubtle
                        cornerRadius: root.theme.radiusLg
                        minimumContentHeight: 104

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            Label {
                                text: modelData.title
                                color: root.theme.inkStrong
                                font.family: root.theme.displayFontFamily
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: (modelData.body || "").length > 0 ? modelData.body : "暂无。"
                                wrapMode: Text.WordWrap
                                color: root.theme.inkMuted
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
