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

    signal chooseProjectRequested()

    color: tokens.topbarBase
    border.color: tokens.border1

    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.54) }
        GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.24) }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 1
        color: root.tokens.shineBorder
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: root.tokens.border1
    }

    function searchCapability(queryText) {
        var query = (queryText || "").trim().toLowerCase()
        if (query.length === 0)
            return

        var nodes = ((root.analysisController.capabilityScene || ({})).nodes || [])
        for (var index = 0; index < nodes.length; ++index) {
            var node = nodes[index]
            var text = ((node.name || "") + " " + (node.role || "") + " " +
                        (node.summary || "") + " " + ((node.moduleNames || []).join(" "))).toLowerCase()
            if (text.indexOf(query) >= 0) {
                root.focusState.setCapability(node)
                root.caseState.navigate("overview")
                return
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 26
        anchors.rightMargin: 26
        spacing: 14

        Item {
            Layout.fillWidth: true
        }

        TextField {
            id: searchField
            Layout.preferredWidth: activeFocus ? 260 : 190
            Layout.preferredHeight: 36
            placeholderText: "Ask SAVT 架构搜索..."
            selectByMouse: true
            color: root.tokens.text1
            placeholderTextColor: root.tokens.text3
            font.family: root.tokens.textFontFamily
            font.pixelSize: 13
            onAccepted: root.searchCapability(text)

            background: Rectangle {
                radius: root.tokens.radiusLg
                color: root.tokens.searchBase
                border.color: searchField.activeFocus ? root.tokens.signalCobalt : root.tokens.border1

                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: searchField.activeFocus
                               ? Qt.rgba(1, 1, 1, 0.96)
                               : Qt.rgba(1, 1, 1, 0.88)
                    }
                    GradientStop {
                        position: 1.0
                        color: searchField.activeFocus
                               ? Qt.rgba(1, 1, 1, 0.82)
                               : Qt.rgba(1, 1, 1, 0.68)
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: "transparent"
                    border.color: root.tokens.shineBorder
                    border.width: 1
                    opacity: 0.52
                }
            }
        }

        Rectangle {
            Layout.preferredHeight: 30
            Layout.minimumWidth: 126
            radius: root.tokens.radiusLg
            color: Qt.rgba(1, 1, 1, 0.72)
            border.color: Qt.rgba(root.tokens.toneColor(root.caseState.trustTone()).r,
                                  root.tokens.toneColor(root.caseState.trustTone()).g,
                                  root.tokens.toneColor(root.caseState.trustTone()).b,
                                  0.28)

            Label {
                anchors.centerIn: parent
                text: root.caseState.trustLabel()
                color: root.tokens.toneColor(root.caseState.trustTone())
                font.family: root.tokens.textFontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
        }

        ActionButton {
            tokens: root.tokens
            text: root.caseState.hasProject ? "快速建模" : "选择项目"
            hint: !root.caseState.hasProject
                  ? "先选择要分析的项目目录。"
                  : "快速整理当前项目，生成架构地图和报告。"
            compact: true
            tone: "primary"
            fixedWidth: 104
            onClicked: {
                if (!root.caseState.hasProject) {
                    root.chooseProjectRequested()
                    return
                }
                root.analysisController.analyzeCurrentProject()
            }
        }

        ActionButton {
            tokens: root.tokens
            text: "AI"
            hint: "让 AI 根据当前选中节点或项目整体生成解释和下一步建议。"
            disabledHint: "AI 服务未配置或当前不可用。"
            compact: true
            tone: "ai"
            fixedWidth: 64
            enabled: root.analysisController.aiAvailable
            onClicked: {
                if (root.focusState.focusedNode)
                    root.analysisController.requestAiExplanation(root.focusState.focusedNode, "请解释当前架构节点，并给出下一步建议。")
                else
                    root.analysisController.requestProjectAiExplanation("请用架构工作台视角总结当前项目。")
                root.caseState.navigate("report")
            }
        }
    }
}
