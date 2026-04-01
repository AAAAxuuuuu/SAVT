import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property QtObject theme

    function confidenceLabelText(label) {
        if (label === "high")
            return "高置信"
        if (label === "medium")
            return "中置信"
        if (label === "low")
            return "低置信"
        return "待补证据"
    }

    function confidenceFillColor(label) {
        if (label === "high")
            return "#edf7ee"
        if (label === "medium")
            return "#fff7e8"
        if (label === "low")
            return "#f9ecec"
        return "#eef4f8"
    }

    function confidenceBorderColor(label) {
        if (label === "high")
            return "#9fc8aa"
        if (label === "medium")
            return "#e1c27b"
        if (label === "low")
            return "#d6a8a8"
        return "#cad8e5"
    }

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
                        text: "围绕当前图元素，集中查看事实、规则、结论、证据来源和 AI 附加说明。"
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
                text: window.selectedInspectorKind() === "edge"
                      ? "已选中关系"
                      : (window.selectedInspectorKind() === "node" ? "已选中模块" : "未选中")
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
                    width: parent.width - 28
                    spacing: 12

                    Label {
                        width: parent.width
                        text: window.selectedInspectorTitle()
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 22
                        font.weight: Font.DemiBold
                    }

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: window.selectedInspectorKind().length > 0
                              ? "收起模式只保留当前选中状态。展开后查看完整证据链。"
                              : "先在 L2 里选中模块，或在展开模式里选中关系。"
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
                    minimumContentHeight: 126
                    fillColor: root.theme.surfaceSecondary
                    borderColor: root.theme.borderSubtle

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        Label {
                            Layout.fillWidth: true
                            text: window.selectedInspectorTitle()
                            color: root.theme.inkStrong
                            wrapMode: Text.WordWrap
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 22
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: window.selectedInspectorSubtitle()
                            wrapMode: Text.WordWrap
                            maximumLineCount: 4
                            elide: Text.ElideRight
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8

                            TagChip {
                                visible: window.selectedInspectorKind() === "node"
                                text: window.selectedInspectorNodeKindLabel()
                                fillColor: "#ffffff"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }

                            TagChip {
                                visible: window.selectedInspectorKind() === "node" && window.selectedInspectorNodeRoleLabel().length > 0
                                text: window.selectedInspectorNodeRoleLabel()
                                fillColor: "#ffffff"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }

                            TagChip {
                                visible: window.selectedInspectorKind() === "node"
                                text: window.selectedInspectorNodeFileCountLabel()
                                fillColor: "#ffffff"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }

                            TagChip {
                                visible: window.selectedInspectorKind() === "edge"
                                text: window.selectedInspectorEdgeWeightLabel()
                                fillColor: "#ffffff"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }

                            TagChip {
                                visible: window.selectedInspectorKind() === "edge"
                                text: window.selectedInspectorEdgeKindLabel()
                                fillColor: "#ffffff"
                                borderColor: root.theme.borderSubtle
                                textColor: root.theme.inkNormal
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8
                            visible: window.selectedInspectorKind() === "edge"

                            Repeater {
                                model: window.selectedEdgeEndpointNodes()

                                AppButton {
                                    theme: root.theme
                                    compact: true
                                    tone: "ghost"
                                    text: "查看 " + (modelData.name || "节点")
                                    onClicked: window.selectInspectorEndpointNode(modelData)
                                }
                            }
                        }
                    }
                }

                SurfaceCard {
                    Layout.fillWidth: true
                    minimumContentHeight: 110
                    fillColor: root.theme.surfacePrimary
                    borderColor: root.theme.borderSubtle

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: "证据结论"
                                color: root.theme.inkStrong
                                font.family: root.theme.displayFontFamily
                                font.pixelSize: 17
                                font.weight: Font.DemiBold
                            }

                            Item { Layout.fillWidth: true }

                            TagChip {
                                text: root.confidenceLabelText(window.selectedEvidenceConfidenceLabel())
                                fillColor: root.confidenceFillColor(window.selectedEvidenceConfidenceLabel())
                                borderColor: root.confidenceBorderColor(window.selectedEvidenceConfidenceLabel())
                                textColor: root.theme.inkNormal
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: (window.selectedEvidenceConclusions()[0] || "当前还没有可展示的结论。")
                            wrapMode: Text.WordWrap
                            color: root.theme.inkStrong
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 13
                        }

                        Label {
                            Layout.fillWidth: true
                            text: window.selectedEvidenceConfidenceReason().length > 0
                                  ? window.selectedEvidenceConfidenceReason()
                                  : "还没有足够证据来给出可信度提示。"
                            wrapMode: Text.WordWrap
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }
                }

                Repeater {
                    model: [
                        {"title": "事实", "items": window.selectedEvidenceFacts()},
                        {"title": "规则", "items": window.selectedEvidenceRules()},
                        {"title": "结论", "items": window.selectedEvidenceConclusions().slice(1)}
                    ]

                    SurfaceCard {
                        Layout.fillWidth: true
                        minimumContentHeight: 96
                        fillColor: root.theme.surfacePrimary
                        borderColor: root.theme.borderSubtle

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 10

                            Label {
                                text: modelData.title
                                color: root.theme.inkStrong
                                font.family: root.theme.displayFontFamily
                                font.pixelSize: 17
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                visible: (modelData.items || []).length === 0
                                text: "当前还没有可展示的" + modelData.title + "。"
                                wrapMode: Text.WordWrap
                                color: root.theme.inkMuted
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }

                            Repeater {
                                model: modelData.items || []

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: sectionLabel.implicitHeight + 18
                                    radius: 16
                                    color: "#ffffff"
                                    border.color: root.theme.borderSubtle

                                    Label {
                                        id: sectionLabel
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.margins: 12
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

                SurfaceCard {
                    Layout.fillWidth: true
                    minimumContentHeight: 130
                    fillColor: root.theme.surfacePrimary
                    borderColor: root.theme.borderSubtle

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        Label {
                            text: "证据来源"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 1
                            rowSpacing: 10

                            Repeater {
                                model: [
                                    {"title": "文件", "items": window.selectedEvidenceFiles(), "mono": true},
                                    {"title": "符号", "items": window.selectedEvidenceSymbols(), "mono": true},
                                    {"title": "模块", "items": window.selectedEvidenceModules(), "mono": false},
                                    {"title": "关系", "items": window.selectedEvidenceRelationships(), "mono": false}
                                ]

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Label {
                                        text: modelData.title
                                        color: root.theme.inkNormal
                                        font.family: root.theme.textFontFamily
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        visible: (modelData.items || []).length === 0
                                        text: "暂无"
                                        color: root.theme.inkMuted
                                        font.family: root.theme.textFontFamily
                                        font.pixelSize: 12
                                    }

                                    Repeater {
                                        model: modelData.items || []

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: sourceLabel.implicitHeight + 16
                                            radius: 14
                                            color: "#ffffff"
                                            border.color: root.theme.borderSubtle

                                            Label {
                                                id: sourceLabel
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
                    }
                }

                SurfaceCard {
                    Layout.fillWidth: true
                    minimumContentHeight: 110
                    fillColor: root.theme.surfacePrimary
                    borderColor: root.theme.borderSubtle
                    visible: window.selectedInspectorKind() === "node"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 12

                        Label {
                            text: "关系入口"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: window.selectedNodeRelationshipItems().length === 0
                            text: "当前节点还没有可下钻查看的关系。"
                            wrapMode: Text.WordWrap
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

                        Repeater {
                            model: window.selectedNodeRelationshipItems()

                            Rectangle {
                                Layout.fillWidth: true
                                radius: 16
                                color: "#ffffff"
                                border.color: root.theme.borderSubtle

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 10

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4

                                        Label {
                                            Layout.fillWidth: true
                                            text: modelData.summary || ""
                                            wrapMode: Text.WordWrap
                                            color: root.theme.inkNormal
                                            font.family: root.theme.textFontFamily
                                            font.pixelSize: 12
                                            font.weight: Font.DemiBold
                                        }

                                        Label {
                                            Layout.fillWidth: true
                                            text: "权重 " + (modelData.weight || 0) + " · " + (modelData.kind || "")
                                            color: root.theme.inkMuted
                                            font.family: root.theme.textFontFamily
                                            font.pixelSize: 11
                                        }
                                    }

                                    AppButton {
                                        theme: root.theme
                                        compact: true
                                        tone: "ghost"
                                        text: "查看证据"
                                        onClicked: window.selectInspectorRelationshipEdge(modelData)
                                    }
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
                                enabled: window.selectedInspectorKind() === "node"
                                         && analysisController.aiAvailable
                                         && !analysisController.aiBusy
                                onClicked: analysisController.requestAiExplanation(window.selectedInspectorNodeData())
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
                                        if (window.selectedInspectorKind() !== "node")
                                            return "AI 解读只针对模块节点开放。要解释关系，请先查看上面的事实、规则和结论。"
                                        if (window.selectedInspectorNodeData() === null)
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
                                text: "正在基于当前节点的静态证据生成补充说明。"
                                wrapMode: Text.WordWrap
                                color: root.theme.accentStrong
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            visible: window.selectedInspectorKind() === "node"
                                     && analysisController.aiHasResult
                                     && !analysisController.aiBusy

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
                            enabled: window.selectedInspectorSupportsCodeContext()
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
