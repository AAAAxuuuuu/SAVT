import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../foundation"

ScrollView {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState

    signal chooseProjectRequested()

    readonly property var readiness: (analysisController.systemContextData || ({})).semanticReadiness || ({})

    clip: true
    contentWidth: availableWidth

    ColumnLayout {
        width: parent.width
        spacing: 16

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 156
            radius: root.tokens.radius8
            color: root.tokens.panelBase
            border.color: root.tokens.border1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10

                Label {
                    text: "分析环境配置"
                    color: root.tokens.text1
                    font.family: root.tokens.displayFontFamily
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: root.analysisController.projectRootPath || "还没有选择项目目录。"
                    elide: Text.ElideMiddle
                    color: root.tokens.text3
                    font.family: root.tokens.monoFontFamily
                    font.pixelSize: 12
                }

                RowLayout {
                    ActionButton {
                        tokens: root.tokens
                        text: "选择项目"
                        tone: "secondary"
                        onClicked: root.chooseProjectRequested()
                    }

                    ActionButton {
                        tokens: root.tokens
                        text: root.analysisController.analyzing ? "停止分析" : "开始分析"
                        tone: root.analysisController.analyzing ? "danger" : "primary"
                        enabled: root.caseState.hasProject
                        onClicked: {
                            if (root.analysisController.analyzing)
                                root.analysisController.stopAnalysis()
                            else
                                root.analysisController.analyzeCurrentProject()
                        }
                    }

                    ActionButton {
                        tokens: root.tokens
                        text: "刷新 AI"
                        tone: "secondary"
                        onClicked: root.analysisController.refreshAiAvailability()
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            radius: root.tokens.radius8
            color: root.tokens.panelBase
            border.color: root.tokens.border1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10

                Label {
                    text: "语义分析状态"
                    color: root.tokens.text1
                    font.family: root.tokens.displayFontFamily
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: root.readiness.headline || root.readiness.modeLabel || "等待分析结果。"
                    wrapMode: Text.WordWrap
                    color: root.tokens.text1
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: root.readiness.summary || root.readiness.reason || "SAVT 会明确区分语法级推断、语义优先和语义必需的阻断原因。"
                    wrapMode: Text.WordWrap
                    color: root.tokens.text2
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 13
                }

                Label {
                    Layout.fillWidth: true
                    text: "下一步：" + (root.readiness.action || "选择项目并运行分析。")
                    wrapMode: Text.WordWrap
                    color: root.tokens.text3
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 12
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 430
            radius: root.tokens.radius8
            color: root.tokens.panelBase
            border.color: root.tokens.border1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14

                ListView {
                    Layout.preferredWidth: 280
                    Layout.fillHeight: true
                    clip: true
                    model: root.analysisController.astFileItems || []

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 42
                        radius: root.tokens.radius8
                        color: root.analysisController.selectedAstFilePath === modelData.path
                               ? root.tokens.signalCobaltSoft : "transparent"

                        Label {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            verticalAlignment: Text.AlignVCenter
                            text: modelData.label || modelData.path || ""
                            elide: Text.ElideMiddle
                            color: root.analysisController.selectedAstFilePath === modelData.path
                                   ? root.tokens.signalCobalt : root.tokens.text2
                            font.family: root.tokens.monoFontFamily
                            font.pixelSize: 11
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.analysisController.selectedAstFilePath = modelData.path
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: root.tokens.border1
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    Label {
                        text: root.analysisController.astPreviewTitle || "AST 预览"
                        color: root.tokens.text1
                        font.family: root.tokens.displayFontFamily
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.analysisController.astPreviewSummary || "完成项目分析后，可在这里查看 Tree-sitter AST。"
                        wrapMode: Text.WordWrap
                        color: root.tokens.text3
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 12
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: availableWidth

                        Label {
                            width: parent.width
                            text: root.analysisController.astPreviewText || "暂无 AST 文本。"
                            wrapMode: Text.WordWrap
                            color: root.tokens.text2
                            font.family: root.tokens.monoFontFamily
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }
    }
}
