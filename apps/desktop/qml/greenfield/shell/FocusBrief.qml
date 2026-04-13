import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../foundation"

Rectangle {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState

    color: tokens.panelBase
    border.color: tokens.border1

    readonly property var node: focusState.focusedNode || ({})
    readonly property var evidence: node.evidence || ({})
    readonly property var facts: evidence.facts || []
    readonly property var rules: evidence.rules || []
    readonly property var conclusions: evidence.conclusions || []
    readonly property var sourceFiles: evidence.sourceFiles || node.exampleFiles || []

    function cleanText(value) {
        return String(value || "")
                .replace(/\uFFFD/g, "")
                .replace(/[\u25CF\u25A0\uFE0F]/g, "")
                .replace(/[\u0000-\u0008\u000B-\u001F]/g, " ")
                .replace(/\s+/g, " ")
                .trim()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 96

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                anchors.topMargin: 22
                anchors.bottomMargin: 14
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        text: root.cleanText(node.name) || "Node Name"
                        color: root.tokens.text1
                        elide: Text.ElideRight
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                    }

                    ActionButton {
                        tokens: root.tokens
                        text: "X"
                        compact: true
                        square: true
                        fixedWidth: 30
                        tone: "ghost"
                        onClicked: root.focusState.inspectorOpen = false
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: root.cleanText(node.kind || node.role) || "Component Type"
                    color: root.tokens.text3
                    elide: Text.ElideRight
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 13
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.tokens.border1
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 12

                Label {
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 20
                    Layout.fillWidth: true
                    text: root.cleanText(node.summary || node.responsibility) || "选中架构节点后，这里会显示来自项目内核的摘要、证据链和 AI 解释入口。"
                    wrapMode: Text.WordWrap
                    color: root.tokens.text2
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 13
                }

                SectionBlock {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    tokens: root.tokens
                    title: "组件事实 (Facts)"
                    rows: root.facts.length > 0 ? root.facts.slice(0, 4) : [
                        "文件数：" + (node.fileCount || node.sourceFileCount || 0),
                        "模块：" + ((node.moduleNames || []).join(", ") || "暂无"),
                        "关键符号：" + ((node.topSymbols || []).join(", ") || "暂无")
                    ]
                }

                SectionBlock {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    tokens: root.tokens
                    title: "架构证据链 (Evidence)"
                    rows: root.rules.length > 0 ? root.rules.slice(0, 3) : [
                        node.role || "等待分析生成推断规则。"
                    ]
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.preferredHeight: Math.max(76, conclusionText.implicitHeight + 34)
                    radius: root.tokens.radius8
                    color: Qt.rgba(0, 0.478, 1, 0.08)
                    border.color: Qt.rgba(0, 0.478, 1, 0.20)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 5

                        Label {
                            text: "Conclusion"
                            color: root.tokens.signalCobalt
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }

                        Label {
                            id: conclusionText
                            Layout.fillWidth: true
                            text: root.conclusions.length > 0 ? root.conclusions[0]
                                  : (root.cleanText(node.responsibility) || "角色：" + (root.cleanText(node.role) || "架构焦点"))
                            wrapMode: Text.WordWrap
                            color: root.tokens.signalCobalt
                            font.family: root.tokens.textFontFamily
                            font.pixelSize: 13
                            font.weight: Font.Medium
                        }
                    }
                }

                SectionBlock {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    tokens: root.tokens
                    title: "Source Path"
                    rows: root.sourceFiles.length > 0 ? root.sourceFiles.slice(0, 4) : ["暂无源文件证据。"]
                    mono: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.bottomMargin: 16
                    Layout.preferredHeight: aiText.implicitHeight + 28
                    radius: root.tokens.radius8
                    color: root.analysisController.aiBusy ? root.tokens.signalRaspberrySoft : root.tokens.base0
                    border.color: root.tokens.border1
                    visible: root.analysisController.aiBusy || root.analysisController.aiHasResult

                    Label {
                        id: aiText
                        anchors.fill: parent
                        anchors.margins: 12
                        text: root.analysisController.aiBusy
                              ? "DeepSeek 正在结合证据链阅读源码..."
                              : root.cleanText(root.analysisController.aiSummary || root.analysisController.aiStatusMessage)
                        wrapMode: Text.WordWrap
                        color: root.tokens.text1
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 12
                    }
                }
            }
        }

        ActionButton {
            Layout.fillWidth: true
            Layout.margins: 20
            Layout.preferredHeight: 42
            tokens: root.tokens
            text: "请求 AI 辅助解释"
            tone: "primary"
            enabled: root.analysisController.aiAvailable && !!root.focusState.focusedNode
            onClicked: root.analysisController.requestAiExplanation(root.focusState.focusedNode, "结合证据链解释这个架构节点，并指出风险和下一步动作。")
        }
    }

    component SectionBlock: ColumnLayout {
        required property QtObject tokens
        property string title: ""
        property var rows: []
        property bool mono: false

        spacing: 8

        Label {
            text: title
            color: tokens.text3
            font.family: tokens.textFontFamily
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: rowsColumn.implicitHeight
            radius: tokens.radius8
            color: tokens.base0
            border.color: tokens.border1

            ColumnLayout {
                id: rowsColumn
                width: parent.width
                spacing: 0

                Repeater {
                    model: rows

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.max(42, rowValue.implicitHeight + 22)

                        Label {
                            id: rowValue
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            text: root.cleanText(modelData)
                            wrapMode: Text.WordWrap
                            color: tokens.text1
                            font.family: mono ? tokens.monoFontFamily : tokens.textFontFamily
                            font.pixelSize: mono ? 11 : 13
                            font.weight: mono ? Font.Normal : Font.Medium
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: tokens.border1
                            visible: index < rows.length - 1
                        }
                    }
                }
            }
        }
    }
}
