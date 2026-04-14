import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject uiState
    required property QtObject analysisController

    property string promptText: ""
    property bool allowInternalDrag: true
    property real preferredX: 0
    property real preferredY: 0
    property real viewportPadding: 14
    property real bottomSafeMargin: 14
    property bool manuallyPositioned: false
    property real dragGrabX: 0
    property real dragGrabY: 0

    signal closeRequested()

    function buildResponseDigest() {
        var sections = []
        if (root.analysisController.aiSummary.length > 0)
            sections.push(root.analysisController.aiSummary)
        if (root.analysisController.aiResponsibility.length > 0)
            sections.push("详细职责:\n" + root.analysisController.aiResponsibility)
        if ((root.analysisController.aiNextActions || []).length > 0)
            sections.push("建议动作:\n- " + root.analysisController.aiNextActions.join("\n- "))
        if (root.analysisController.aiUncertainty.length > 0)
            sections.push("不确定性:\n" + root.analysisController.aiUncertainty)
        return sections.join("\n\n")
    }

    function isReportFocus() {
        var pageId = root.uiState.navigation.pageId || ""
        return pageId.indexOf("report.") === 0 || pageId === "levels.l4"
    }

    function scopeLabel() {
        var scope = root.analysisController.aiScope || ""
        if (scope === "system_context")
            return "项目导览"
        if (scope === "engineering_report")
            return "深入阅读建议"
        if (scope === "file_node")
            return "文件解读"
        if (scope === "capability_map" || scope === "component_node")
            return "模块解读"
        if (root.uiState.inspector.kind === "node")
            return root.uiState.inspector.selectedNodeIsSingleFile ? "文件解读" : "模块解读"
        if (root.isReportFocus())
            return "深入阅读建议"
        return "项目导览"
    }

    function focusLabel() {
        if (root.uiState.inspector.kind === "node")
            return root.uiState.inspector.title
        if (root.uiState.inspector.kind === "edge")
            return "当前关系"
        if (root.isReportFocus())
            return "当前报告"
        return "当前项目"
    }

    function maximumX() {
        if (!root.allowInternalDrag)
            return 0
        if (!root.parent)
            return root.viewportPadding
        return Math.max(root.viewportPadding, root.parent.width - root.width - root.viewportPadding)
    }

    function maximumY() {
        if (!root.allowInternalDrag)
            return 0
        if (!root.parent)
            return root.viewportPadding
        return Math.max(root.viewportPadding, root.parent.height - root.height - root.bottomSafeMargin)
    }

    function clampPosition() {
        if (!root.allowInternalDrag)
            return
        if (!root.parent)
            return
        root.x = Math.max(root.viewportPadding, Math.min(root.x, root.maximumX()))
        root.y = Math.max(root.viewportPadding, Math.min(root.y, root.maximumY()))
    }

    function syncToPreferredPosition() {
        if (!root.allowInternalDrag)
            return
        if (!root.parent)
            return
        root.x = Math.max(root.viewportPadding, Math.min(root.preferredX, root.maximumX()))
        root.y = Math.max(root.viewportPadding, Math.min(root.preferredY, root.maximumY()))
    }

    onVisibleChanged: {
        if (!root.allowInternalDrag)
            return
        if (!visible)
            return
        if (root.manuallyPositioned)
            root.clampPosition()
        else
            root.syncToPreferredPosition()
    }

    onPreferredXChanged: {
        if (!root.allowInternalDrag)
            return
        if (!root.visible)
            return
        if (root.manuallyPositioned)
            root.clampPosition()
        else
            root.syncToPreferredPosition()
    }

    onPreferredYChanged: {
        if (!root.allowInternalDrag)
            return
        if (!root.visible)
            return
        if (root.manuallyPositioned)
            root.clampPosition()
        else
            root.syncToPreferredPosition()
    }

    onWidthChanged: {
        if (!root.allowInternalDrag)
            return
        if (!root.visible)
            return
        if (root.manuallyPositioned)
            root.clampPosition()
        else
            root.syncToPreferredPosition()
    }

    onHeightChanged: {
        if (!root.allowInternalDrag)
            return
        if (!root.visible)
            return
        if (root.manuallyPositioned)
            root.clampPosition()
        else
            root.syncToPreferredPosition()
    }

    onBottomSafeMarginChanged: {
        if (!root.allowInternalDrag)
            return
        if (!root.visible)
            return
        if (root.manuallyPositioned)
            root.clampPosition()
        else
            root.syncToPreferredPosition()
    }

    Component.onCompleted: {
        if (!root.allowInternalDrag)
            return
        if (root.visible)
            Qt.callLater(root.syncToPreferredPosition)
    }

    Connections {
        target: root.parent || null

        function onWidthChanged() {
            if (!root.allowInternalDrag)
                return
            if (!root.visible)
                return
            if (root.manuallyPositioned)
                root.clampPosition()
            else
                root.syncToPreferredPosition()
        }

        function onHeightChanged() {
            if (!root.allowInternalDrag)
                return
            if (!root.visible)
                return
            if (root.manuallyPositioned)
                root.clampPosition()
            else
                root.syncToPreferredPosition()
        }
    }

    AppCard {
        anchors.fill: parent
        fillColor: root.theme.aiWash
        borderColor: root.theme.borderStrong
        cornerRadius: root.theme.radiusXxl
        contentPadding: 16
        stacked: false

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
                        text: "AI 导览"
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 24
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.uiState.inspector.kind === "node"
                              ? (root.uiState.inspector.selectedNodeIsSingleFile
                                 ? ("当前将围绕「" + root.uiState.inspector.title + "」生成文件解读，说明它在系统里做什么、文件里该先看哪里、和谁协作。")
                                 : ("当前将围绕「" + root.uiState.inspector.title + "」生成模块解读，说明它的职责、重要性和下一步先看哪里。"))
                              : (root.isReportFocus()
                                 ? "可以把当前分析结果整理成深入阅读建议，帮助你决定接下来先读哪里、先验证什么。"
                                 : "可以围绕当前项目生成项目导览，先说明项目是什么、建议从哪里开始看。")
                        wrapMode: Text.WordWrap
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                    }
                }

                Item {
                    id: dragHandle
                    visible: root.allowInternalDrag
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 30
                    Layout.alignment: Qt.AlignTop

                    Rectangle {
                        anchors.fill: parent
                        radius: height / 2
                        color: Qt.rgba(1, 1, 1, 0.84)
                        border.color: dragArea.containsMouse ? root.theme.focusRing : root.theme.borderSubtle
                        border.width: 1
                    }

                    Row {
                        anchors.centerIn: parent
                        spacing: 6

                        Repeater {
                            model: 3

                            Rectangle {
                                width: 4
                                height: 4
                                radius: 2
                                color: root.theme.inkMuted
                            }
                        }

                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Drag"
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                    }

                    MouseArea {
                        id: dragArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

                        onPressed: function(mouse) {
                            root.manuallyPositioned = true
                            var pointInRoot = dragHandle.mapToItem(root, mouse.x, mouse.y)
                            root.dragGrabX = pointInRoot.x
                            root.dragGrabY = pointInRoot.y
                        }

                        onPositionChanged: function(mouse) {
                            if (!pressed || !root.parent)
                                return
                            var pointInParent = dragHandle.mapToItem(root.parent, mouse.x, mouse.y)
                            root.x = pointInParent.x - root.dragGrabX
                            root.y = pointInParent.y - root.dragGrabY
                            root.clampPosition()
                        }

                        onReleased: root.clampPosition()
                        onCanceled: root.clampPosition()
                    }
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: "关闭"
                    onClicked: root.closeRequested()
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                AppChip {
                    text: "焦点 " + root.focusLabel()
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: root.analysisController.aiAvailable ? "AI 已就绪" : "AI 待配置"
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: root.analysisController.aiBusy ? "正在生成" : "可即时追问"
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppChip {
                    text: "输出 " + root.scopeLabel()
                    compact: true
                    fillColor: "#FFFFFF"
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: [
                        "解释当前焦点的职责边界",
                        "说明它为什么值得先看",
                        "给出下一步阅读顺序"
                    ]

                    AppButton {
                        theme: root.theme
                        compact: true
                        tone: "ghost"
                        text: modelData
                        onClicked: {
                            root.promptText = modelData
                            promptEditor.text = modelData
                        }
                    }
                }
            }

                TextArea {
                id: promptEditor
                Layout.fillWidth: true
                Layout.preferredHeight: 108
                text: root.promptText
                placeholderText: root.uiState.inspector.selectedNodeIsSingleFile
                                 ? "补充你想问的问题，例如：解释这个文件在主流程里扮演什么角色，并告诉我应该先看哪几段代码。"
                                 : "补充你想问的问题，例如：解释这个模块为什么重要，并告诉我接下来先看哪两个文件。"
                wrapMode: TextEdit.Wrap
                font.family: root.theme.textFontFamily
                font.pixelSize: 13
                selectByMouse: true
                onTextChanged: root.promptText = text

                background: Rectangle {
                    radius: root.theme.radiusLg
                    color: root.theme.surfaceSecondary
                    border.color: promptEditor.activeFocus ? root.theme.focusRing : root.theme.borderSubtle
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ai"
                    text: root.analysisController.aiBusy ? "生成中..." : "生成项目导览"
                    enabled: root.analysisController.aiAvailable && !root.analysisController.aiBusy
                    onClicked: root.analysisController.requestProjectAiExplanation(root.promptText)
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    visible: root.isReportFocus()
                    text: root.analysisController.aiBusy ? "生成中..." : "生成深入阅读建议"
                    enabled: root.analysisController.aiAvailable && !root.analysisController.aiBusy
                    onClicked: root.analysisController.requestReportAiExplanation(root.promptText)
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "accent"
                    text: root.analysisController.aiBusy
                          ? "生成中..."
                          : (root.uiState.inspector.selectedNodeIsSingleFile ? "生成文件解读" : "生成模块解读")
                    enabled: root.uiState.inspector.kind === "node" &&
                             root.analysisController.aiAvailable &&
                             !root.analysisController.aiBusy
                    onClicked: root.analysisController.requestAiExplanation(root.uiState.inspector.selectedNode, root.promptText)
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: "复制结果"
                    enabled: root.analysisController.aiHasResult && !root.analysisController.aiBusy
                    onClicked: root.analysisController.copyTextToClipboard(root.buildResponseDigest())
                }
            }

            AppCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusLg

                ScrollView {
                    anchors.fill: parent
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width
                        spacing: 10

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
                            visible: root.analysisController.aiBusy
                            text: root.uiState.inspector.selectedNodeIsSingleFile
                                  ? "正在基于当前文件的静态证据生成解读内容。"
                                  : "正在基于当前静态证据生成导览内容。"
                            wrapMode: Text.WordWrap
                            color: root.theme.aiAccent
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: !root.analysisController.aiHasResult && !root.analysisController.aiBusy
                            text: root.uiState.inspector.selectedNodeIsSingleFile
                                  ? "还没有 AI 文件解读，可以先围绕当前文件生成一版具体说明。"
                                  : "还没有 AI 导览，可以先生成项目导览、模块解读或深入阅读建议。"
                            wrapMode: Text.WordWrap
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: root.analysisController.aiHasResult && !root.analysisController.aiBusy
                            text: root.analysisController.aiSummary
                            wrapMode: Text.WordWrap
                            color: root.theme.inkStrong
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: root.analysisController.aiResponsibility.length > 0
                            text: root.analysisController.aiResponsibility
                            wrapMode: Text.WordWrap
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }

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

                        Label {
                            Layout.fillWidth: true
                            visible: root.analysisController.aiUncertainty.length > 0
                            text: root.analysisController.aiUncertainty
                            wrapMode: Text.WordWrap
                            color: root.theme.warning
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}
