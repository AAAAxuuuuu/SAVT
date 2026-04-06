import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject inspectorState

    readonly property var summaryItems: root.buildSummaryItems()

    implicitHeight: card.implicitHeight

    function buildSummaryItems() {
        var sections = []

        if ((root.inspectorState.subtitle || "").length > 0)
            sections.push({"title": "这是什么", "body": root.inspectorState.subtitle})

        if ((root.inspectorState.importanceSummary || "").length > 0)
            sections.push({"title": "为什么重要", "body": root.inspectorState.importanceSummary})

        if ((root.inspectorState.sourceFiles || []).length > 0)
            sections.push({"title": "关键文件", "body": root.inspectorState.sourceFiles.slice(0, 3).join("\n")})

        if (root.inspectorState.kind === "edge") {
            sections.push({
                "title": "关系解释",
                "body": "两端节点数 " + root.inspectorState.endpointNodes.length + "，建议沿着证据继续核对责任链。"
            })
        } else if ((root.inspectorState.relationshipItems || []).length > 0) {
            sections.push({
                "title": "关系解释",
                "body": "当前对象直接关联 " + root.inspectorState.relationshipItems.length + " 条关系。"
            })
        }

        if ((root.inspectorState.nextStepSummary || "").length > 0)
            sections.push({"title": "下一步", "body": root.inspectorState.nextStepSummary})

        if (sections.length === 0)
            sections.push({"title": "摘要", "body": "当前对象还没有可展示的摘要。"})

        return sections.slice(0, 4)
    }

    AppCard {
        id: card
        anchors.fill: parent
        fillColor: root.theme.surfaceSecondary
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXl
        minimumContentHeight: 92

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Repeater {
                model: root.summaryItems

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: modelData.title
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 15
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

                    Rectangle {
                        Layout.fillWidth: true
                        visible: index < root.summaryItems.length - 1
                        height: 1
                        color: root.theme.borderSubtle
                        opacity: 0.75
                    }
                }
            }
        }
    }
}
