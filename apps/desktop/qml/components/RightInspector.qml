import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property QtObject theme

    SurfaceCard {
        anchors.fill: parent
        minimumContentHeight: 520
        fillColor: root.theme.surfacePrimary
        borderColor: root.theme.borderStrong
        stacked: !window.inspectorCollapsed

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    visible: !window.inspectorCollapsed

                    Label {
                        text: "Inspector"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 22
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "围绕当前选中模块，集中查看职责、证据线索和 AI 说明。"
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: window.inspectorCollapsed ? "展开" : "收起"
                    onClicked: window.inspectorCollapsed = !window.inspectorCollapsed
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Loader {
                    anchors.fill: parent
                    active: window.inspectorCollapsed
                    sourceComponent: collapsedComponent
                }

                Loader {
                    anchors.fill: parent
                    active: !window.inspectorCollapsed
                    sourceComponent: expandedComponent
                }
            }
        }
    }

    Component {
        id: collapsedComponent

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            TagChip {
                text: window.selectedCapabilityNode ? "已选中模块" : "未选中"
                fillColor: "#eef4f9"
                borderColor: "#c8d6e4"
                textColor: "#28435c"
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 24
                color: root.theme.surfaceSecondary
                border.color: root.theme.borderSubtle

                Column {
                    anchors.centerIn: parent
                    spacing: 12

                    Label {
                        text: "AI"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 22
                        font.weight: Font.DemiBold
                    }

                    Label {
                        width: parent.parent ? parent.parent.width - 28 : 60
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: "收起模式只保留状态。\n展开后查看模块细节。"
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }
            }
        }
    }

    Component {
        id: expandedComponent

        ScrollView {
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: 12

                SurfaceCard {
                    Layout.fillWidth: true
                    minimumContentHeight: 110
                    fillColor: root.theme.surfaceSecondary
                    borderColor: root.theme.borderSubtle

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Label {
                                    text: window.selectedCapabilityNode ? window.selectedCapabilityNode.name : "尚未选择模块"
                                    color: root.theme.inkStrong
                                    wrapMode: Text.WordWrap
                                    font.family: root.theme.displayFontFamily
                                    font.pixelSize: 22
                                    font.weight: Font.DemiBold
                                    Layout.fillWidth: true
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: window.selectedCapabilityNode ? window.selectedCapabilityNode.responsibility : "先在 L2 视图里点一个模块，再从这里看它的职责和上下文。"
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 3
                                    elide: Text.ElideRight
                                    color: root.theme.inkMuted
                                    font.family: root.theme.textFontFamily
                                    font.pixelSize: 12
                                }
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8

                            TagChip {
                                visible: !!window.selectedCapabilityNode
                                text: window.selectedCapabilityNode ? window.displayNodeKind(window.selectedCapabilityNode.kind) : ""
                                fillColor: "#ffffff"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }

                            TagChip {
                                visible: !!window.selectedCapabilityNode && (window.selectedCapabilityNode.role || "").length > 0
                                text: window.selectedCapabilityNode ? window.selectedCapabilityNode.role : ""
                                fillColor: "#ffffff"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }

                            TagChip {
                                visible: !!window.selectedCapabilityNode
                                text: window.selectedCapabilityNode ? ("文件 " + (window.selectedCapabilityNode.fileCount || 0)) : ""
                                fillColor: "#ffffff"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }
                        }
                    }
                }

                SurfaceCard {
                    Layout.fillWidth: true
                    minimumContentHeight: 118
                    fillColor: root.theme.surfacePrimary
                    borderColor: root.theme.borderSubtle

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        Label {
                            text: "证据线索"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: window.selectedNodeEvidenceSummary().length > 0
                                  ? window.selectedNodeEvidenceSummary()
                                  : "当前还没有提取到可展示的证据摘要。"
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            color: root.theme.inkNormal
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

                        Repeater {
                            model: window.selectedCapabilityNode && window.selectedCapabilityNode.topSymbols
                                   ? window.selectedCapabilityNode.topSymbols.slice(0, 6)
                                   : []

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: evidenceLabel.implicitHeight + 18
                                radius: 16
                                color: "#ffffff"
                                border.color: root.theme.borderSubtle

                                Label {
                                    id: evidenceLabel
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.margins: 12
                                    text: modelData
                                    wrapMode: Text.WordWrap
                                    color: root.theme.inkNormal
                                    font.family: root.theme.monoFontFamily
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }
                }

                SurfaceCard {
                    Layout.fillWidth: true
                    minimumContentHeight: 126
                    fillColor: "#eef4f8"
                    borderColor: root.theme.borderSubtle

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Label {
                                    text: "AI 辅读图"
                                    color: root.theme.inkStrong
                                    font.family: root.theme.displayFontFamily
                                    font.pixelSize: 17
                                    font.weight: Font.DemiBold
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: analysisController.aiStatusMessage
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 3
                                    elide: Text.ElideRight
                                    color: root.theme.inkMuted
                                    font.family: root.theme.textFontFamily
                                    font.pixelSize: 12
                                }
                            }

                            AppButton {
                                theme: root.theme
                                tone: "accent"
                                compact: true
                                text: analysisController.aiBusy
                                      ? "解读中..."
                                      : (analysisController.aiAvailable ? "生成解释" : "AI 未就绪")
                                enabled: window.selectedCapabilityNode !== null
                                         && analysisController.aiAvailable
                                         && !analysisController.aiBusy
                                onClicked: analysisController.requestAiExplanation(window.selectedCapabilityNode)
                            }

                            AppButton {
                                theme: root.theme
                                tone: "ghost"
                                compact: true
                                text: "刷新状态"
                                enabled: !analysisController.aiBusy
                                onClicked: analysisController.refreshAiAvailability()
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            radius: 16
                            color: analysisController.aiAvailable ? "#eff6ff" : "#fff7e7"
                            border.color: analysisController.aiAvailable ? "#9ec4ee" : "#e8c97f"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 6

                                Label {
                                    text: analysisController.aiAvailable ? "AI 状态" : "AI 配置提示"
                                    color: analysisController.aiAvailable ? "#1f5e96" : "#8a5d12"
                                    font.family: root.theme.textFontFamily
                                    font.pixelSize: 12
                                    font.weight: Font.DemiBold
                                }

                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 4
                                    elide: Text.ElideRight
                                    color: analysisController.aiAvailable ? "#2c4f74" : "#7a5823"
                                    font.family: root.theme.textFontFamily
                                    font.pixelSize: 12
                                    text: {
                                        if (!analysisController.aiAvailable)
                                            return analysisController.aiSetupMessage
                                        if (window.selectedCapabilityNode === null)
                                            return "AI 已就绪。先在图里选中一个模块，再生成 AI 解读。"
                                        return analysisController.aiStatusMessage.length > 0
                                               ? analysisController.aiStatusMessage
                                               : analysisController.aiSetupMessage
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: analysisController.aiBusy
                            spacing: 10

                            BusyIndicator {
                                running: analysisController.aiBusy
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                            }

                            Label {
                                Layout.fillWidth: true
                                text: "正在结合结构结果和右侧上下文生成说明。"
                                wrapMode: Text.WordWrap
                                color: root.theme.accentStrong
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            visible: analysisController.aiHasResult && !analysisController.aiBusy

                            Label {
                                Layout.fillWidth: true
                                visible: analysisController.aiSummary.length > 0
                                text: analysisController.aiSummary
                                wrapMode: Text.WordWrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                                color: root.theme.inkStrong
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                visible: analysisController.aiResponsibility.length > 0
                                radius: 18
                                color: "#ffffff"
                                border.color: root.theme.borderSubtle

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    spacing: 8

                                    Label {
                                        text: "详细职责"
                                        color: root.theme.inkNormal
                                        font.family: root.theme.textFontFamily
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: analysisController.aiResponsibility
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 3
                                        elide: Text.ElideRight
                                        color: root.theme.inkNormal
                                        font.family: root.theme.textFontFamily
                                        font.pixelSize: 12
                                    }
                                }
                            }

                            Flow {
                                Layout.fillWidth: true
                                spacing: 8
                                visible: analysisController.aiCollaborators.length > 0

                                Repeater {
                                    model: analysisController.aiCollaborators

                                    TagChip {
                                        text: modelData
                                        fillColor: "#ffffff"
                                        borderColor: root.theme.borderSubtle
                                        textColor: root.theme.inkNormal
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                visible: analysisController.aiUncertainty.length > 0
                                radius: 18
                                color: "#fbf3e2"
                                border.color: "#ddc28e"

                                Label {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    text: analysisController.aiUncertainty
                                    wrapMode: Text.WordWrap
                                    color: "#7a5823"
                                    font.family: root.theme.textFontFamily
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }
                }

                SurfaceCard {
                    Layout.fillWidth: true
                    minimumContentHeight: 108
                    fillColor: "#f0f4f9"
                    borderColor: root.theme.borderSubtle

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        Label {
                            text: "AI 修改入口"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "直接复制当前模块的关键文件、核心符号和协作线索，给外部编码代理作为上下文。"
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

                        AppButton {
                            theme: root.theme
                            Layout.fillWidth: true
                            tone: "accent"
                            text: "复制当前模块上下文"
                            enabled: window.selectedCapabilityNode !== null
                            onClicked: {
                                if (window.selectedCapabilityNode)
                                    analysisController.copyCodeContextToClipboard(window.selectedCapabilityNode.id)
                            }
                        }
                    }
                }
            }
        }
    }
}
