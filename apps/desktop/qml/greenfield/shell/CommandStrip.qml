import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState

    signal chooseProjectRequested()

    color: tokens.topbarBase

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
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 12

        Label {
            Layout.fillWidth: true
            textFormat: Text.RichText
            text: "<span style='color:#86868B'>Workspace</span> / <b>" + root.caseState.routeTitle() + "</b>"
            color: root.tokens.text1
            font.family: root.tokens.textFontFamily
            font.pixelSize: 15
            font.weight: Font.Medium
            elide: Text.ElideRight
        }

        TextField {
            id: searchField
            Layout.preferredWidth: activeFocus ? 260 : 190
            Layout.preferredHeight: 34
            placeholderText: "Ask SAVT 架构搜索..."
            selectByMouse: true
            color: root.tokens.text1
            placeholderTextColor: root.tokens.text3
            font.family: root.tokens.textFontFamily
            font.pixelSize: 13
            onAccepted: root.searchCapability(text)

            background: Rectangle {
                radius: root.tokens.radius8
                color: root.tokens.searchBase
                border.color: searchField.activeFocus ? root.tokens.signalCobalt : "transparent"
            }
        }

        Rectangle {
            Layout.preferredHeight: 28
            Layout.minimumWidth: 118
            radius: root.tokens.radius8
            color: root.tokens.toneSoft(root.caseState.trustTone())
            border.color: root.tokens.toneColor(root.caseState.trustTone())

            Label {
                anchors.centerIn: parent
                text: root.caseState.trustLabel()
                color: root.tokens.toneColor(root.caseState.trustTone())
                font.family: root.tokens.textFontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
        }

        Button {
            text: root.caseState.hasProject ? (root.analysisController.analyzing ? "停止" : "分析") : "选择项目"
            implicitHeight: 32
            implicitWidth: 92
            onClicked: {
                if (!root.caseState.hasProject) {
                    root.chooseProjectRequested()
                    return
                }
                if (root.analysisController.analyzing)
                    root.analysisController.stopAnalysis()
                else
                    root.analysisController.analyzeCurrentProject()
            }
        }

        Button {
            text: "AI"
            implicitHeight: 32
            implicitWidth: 64
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
