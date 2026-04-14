import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject selectionState

    readonly property var nodeData: root.selectionState.selectedComponentNode || null
    readonly property var relationshipItems: root.selectionState.selectedNodeRelationshipItems()
    readonly property bool singleFileActive: !!root.nodeData
                                              && (root.nodeData.fileCount || 0) === 1
                                              && ((root.nodeData.exampleFiles || []).length === 1)
    property var fileDetail: ({})

    readonly property bool aiMatchesCurrentFile: root.analysisController.aiHasResult
                                                 && root.analysisController.aiScope === "file_node"
                                                 && root.analysisController.aiNodeName === ((root.fileDetail.fileName || "") || (root.nodeData ? (root.nodeData.name || "") : ""))

    function refreshFileDetail() {
        if (!root.singleFileActive) {
            root.fileDetail = ({})
            return
        }
        root.fileDetail = root.analysisController.describeFileNode(root.nodeData)
    }

    onNodeDataChanged: refreshFileDetail()
    Component.onCompleted: refreshFileDetail()

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: parent.width
            spacing: 12

            AppCard {
                Layout.fillWidth: true
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusXxl

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                Layout.fillWidth: true
                                text: root.fileDetail.fileName || (root.nodeData ? (root.nodeData.name || "文件解读") : "文件解读")
                                wrapMode: Text.WordWrap
                                color: root.theme.inkStrong
                                font.family: root.theme.displayFontFamily
                                font.pixelSize: 24
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.fileDetail.filePath || ""
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
                            text: "回到组件关系"
                            onClicked: root.selectionState.clearComponentFocus()
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8

                        AppChip {
                            text: root.fileDetail.languageLabel || "Unknown"
                            compact: true
                            fillColor: "#FFFFFF"
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }

                        AppChip {
                            text: root.fileDetail.categoryLabel || "源码文件"
                            compact: true
                            fillColor: "#FFFFFF"
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }

                        AppChip {
                            text: root.fileDetail.roleLabel || "具体实现"
                            compact: true
                            fillColor: "#FFFFFF"
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }

                        AppChip {
                            text: "行数 " + (root.fileDetail.lineCount || 0)
                            compact: true
                            fillColor: "#FFFFFF"
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }

                        AppChip {
                            text: "非空 " + (root.fileDetail.nonEmptyLineCount || 0)
                            compact: true
                            fillColor: "#FFFFFF"
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.fileDetail.summary || "当前文件还没有形成稳定解读。"
                        wrapMode: Text.WordWrap
                        color: root.theme.inkStrong
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 13
                    }
                }
            }

            AppCard {
                Layout.fillWidth: true
                fillColor: "#F4F8FC"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusXl

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: "AI 文件解读"
                                color: root.theme.inkStrong
                                font.family: root.theme.displayFontFamily
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: "这次不再让 AI 讲模块套话，而是围绕这个文件自己的路径、声明、依赖和片段来解读。"
                                wrapMode: Text.WordWrap
                                color: root.theme.inkMuted
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }
                        }

                        AppButton {
                            theme: root.theme
                            compact: true
                            tone: "accent"
                            text: root.analysisController.aiBusy ? "解读中..." : "请 AI 解读这个文件"
                            enabled: root.analysisController.aiAvailable
                                     && !root.analysisController.aiBusy
                                     && !!root.nodeData
                            onClicked: root.analysisController.requestAiExplanation(root.nodeData)
                        }
                    }

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
                        visible: root.aiMatchesCurrentFile
                        text: root.analysisController.aiSummary
                        wrapMode: Text.WordWrap
                        color: root.theme.inkStrong
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }
            }

            Repeater {
                model: [
                    {"title": "导入 / 依赖线索", "items": root.fileDetail.importClues || []},
                    {"title": "声明 / 主符号", "items": root.fileDetail.declarationClues || []},
                    {"title": "行为信号", "items": root.fileDetail.behaviorSignals || []},
                    {"title": "建议阅读顺序", "items": root.fileDetail.readingHints || []}
                ]

                AppCard {
                    Layout.fillWidth: true
                    visible: (modelData.items || []).length > 0
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    cornerRadius: root.theme.radiusXl

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

                        Repeater {
                            model: modelData.items || []

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: clueLabel.implicitHeight + 16
                                radius: root.theme.radiusMd
                                color: root.theme.surfaceSecondary
                                border.color: root.theme.borderSubtle

                                Label {
                                    id: clueLabel
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.margins: 10
                                    text: modelData
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

            AppCard {
                Layout.fillWidth: true
                visible: root.relationshipItems.length > 0
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusXl

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        text: "关联关系"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    Repeater {
                        model: root.relationshipItems.slice(0, 6)

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: relationLabel.implicitHeight + 16
                            radius: root.theme.radiusMd
                            color: root.theme.surfaceSecondary
                            border.color: root.theme.borderSubtle

                            Label {
                                id: relationLabel
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: 10
                                text: modelData.summary || ((modelData.fromName || "") + " -> " + (modelData.toName || ""))
                                wrapMode: Text.WordWrap
                                color: root.theme.inkNormal
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }
                        }
                    }
                }
            }

            AppCard {
                Layout.fillWidth: true
                visible: (root.fileDetail.previewText || "").length > 0
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusXl

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        text: "关键片段"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: root.theme.radiusMd
                        color: "#F7F9FC"
                        border.color: root.theme.borderSubtle

                        Label {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 12
                            text: root.fileDetail.previewText || ""
                            wrapMode: Text.WrapAnywhere
                            color: root.theme.inkNormal
                            font.family: "Consolas"
                            font.pixelSize: 12
                        }

                        implicitHeight: 24 + childrenRect.height
                    }
                }
            }
        }
    }
}
