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

    function joinItems(items, emptyText) {
        return (items || []).length > 0 ? (items || []).slice(0, 5).join("\n") : emptyText
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
            eyebrow: "证据"
            title: "证据、诊断与结论"
            subtitle: "把静态分析诊断、当前对象证据包和 AI 补充线索拆开承载，避免全部挤进右侧 Inspector。"

            GridLayout {
                Layout.fillWidth: true
                columns: width > 1080 ? 3 : 1
                columnSpacing: 12
                rowSpacing: 12

                Repeater {
                    model: [
                        {
                            "title": "静态诊断",
                            "body": root.joinItems(root.uiState.selection.capabilityScene.diagnostics || [], "当前没有诊断信息。")
                        },
                        {
                            "title": "当前焦点证据",
                            "body": root.joinItems(root.uiState.inspector.facts, "当前焦点还没有可展示的事实。") + "\n\n"
                                  + root.joinItems(root.uiState.inspector.conclusions, "当前焦点还没有结论补充。")
                        },
                        {
                            "title": "AI 补充线索",
                            "body": root.joinItems(root.analysisController.aiEvidence || [], "还没有 AI 补充线索。")
                        }
                    ]

                    AppCard {
                        Layout.fillWidth: true
                        fillColor: root.theme.surfaceSecondary
                        borderColor: root.theme.borderSubtle
                        cornerRadius: root.theme.radiusLg
                        minimumContentHeight: 132

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
                                text: modelData.body
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
