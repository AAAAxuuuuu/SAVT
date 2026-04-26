import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState

    readonly property var guide: root.analysisController.systemContextData || ({})
    readonly property var algorithmSummary: guide.algorithmSummary || ({})
    readonly property var algorithmMetrics: guide.algorithmMetrics || []

    radius: tokens.radiusXxl + 4
    color: tokens.sidebarBase
    border.color: tokens.border1
    clip: true

    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.74) }
        GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.54) }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.color: root.tokens.shineBorder
        border.width: 1
    }

    function itemIconColor(route) {
        return root.caseState.route === route ? "#FFFFFF" : root.tokens.text3
    }

    function metricAt(index) {
        return index >= 0 && index < algorithmMetrics.length ? algorithmMetrics[index] : ({})
    }

    function searchCapability(queryText) {
        var query = String(queryText || "").trim().toLowerCase()
        if (query.length === 0)
            return

        var nodes = ((root.analysisController.capabilityScene || ({})).nodes || [])
        for (var index = 0; index < nodes.length; ++index) {
            var node = nodes[index]
            var text = ((node.name || "") + " " + (node.role || "") + " "
                        + (node.summary || "") + " " + ((node.moduleNames || []).join(" "))).toLowerCase()
            if (text.indexOf(query) >= 0) {
                root.focusState.setCapability(node)
                root.caseState.navigate("overview")
                return
            }
        }
    }

    function requestProjectAiReview() {
        root.analysisController.requestProjectAiExplanation(
                    "请基于当前 SAVT 架构重建结果，总结这次算法分析的关键发现。"
                    + " 输出请明确写出：当前重建链路最稳定的能力域、证据最弱的边界、缓存或精度状态对结论的影响、以及下一步最值得补强的算法点。")
        root.caseState.navigate("report")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 22
        anchors.bottomMargin: 22
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 8

        Label {
            Layout.leftMargin: 8
            text: "SAVT RECONSTRUCTION"
            color: root.tokens.text3
            font.family: root.tokens.textFontFamily
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        Repeater {
            model: [
                {"route": "overview", "label": "重建全景"},
                {"route": "component", "label": "边界钻取"},
                {"route": "report", "label": "算法报告"},
                {"route": "config", "label": "环境与精度"}
            ]

            Rectangle {
                id: navItem
                property bool active: root.caseState.route === modelData.route
                property bool hovered: navMouse.containsMouse
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                radius: root.tokens.radiusLg
                color: "transparent"
                border.color: active
                              ? Qt.rgba(1, 1, 1, 0.24)
                              : (hovered ? root.tokens.border1 : "transparent")

                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: navItem.active
                               ? root.tokens.signalCobalt
                               : (navItem.hovered ? Qt.rgba(1, 1, 1, 0.46) : "transparent")
                    }

                    GradientStop {
                        position: 1.0
                        color: navItem.active
                               ? Qt.rgba(root.tokens.signalRaspberry.r,
                                         root.tokens.signalRaspberry.g,
                                         root.tokens.signalRaspberry.b,
                                         0.92)
                               : (navItem.hovered ? Qt.rgba(1, 1, 1, 0.18) : "transparent")
                    }
                }

                Behavior on border.color {
                    ColorAnimation { duration: 120 }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 18
                        Layout.preferredHeight: 18
                        radius: root.tokens.radius4
                        color: root.itemIconColor(modelData.route)
                        opacity: root.caseState.route === modelData.route ? 1.0 : 0.82
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData.label
                        color: navItem.active ? "#FFFFFF" : root.tokens.text1
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: navMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.caseState.navigate(modelData.route)
                }
            }
        }

        Item { Layout.fillHeight: true }

        TextField {
            id: searchField
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            placeholderText: "搜索能力域 / 证据..."
            selectByMouse: true
            color: root.tokens.text1
            placeholderTextColor: root.tokens.text3
            font.family: root.tokens.textFontFamily
            font.pixelSize: 12
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
                               : Qt.rgba(1, 1, 1, 0.9)
                    }
                    GradientStop {
                        position: 1.0
                        color: searchField.activeFocus
                               ? Qt.rgba(1, 1, 1, 0.82)
                               : Qt.rgba(1, 1, 1, 0.72)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: root.tokens.radiusLg
            color: root.tokens.panelStrong
            border.color: root.tokens.border1
            implicitHeight: snapshotColumn.implicitHeight + 24

            ColumnLayout {
                id: snapshotColumn
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Label {
                    text: "算法快照"
                    color: root.tokens.text3
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 8
                    rowSpacing: 8

                    RailMetricCard {
                        Layout.fillWidth: true
                        tokens: root.tokens
                        title: String(root.metricAt(0).label || "源码事实")
                        value: String(root.metricAt(0).value || "--")
                        tone: String(root.metricAt(0).tone || "info")
                    }

                    RailMetricCard {
                        Layout.fillWidth: true
                        tokens: root.tokens
                        title: String(root.metricAt(2).label || "能力域")
                        value: String(root.metricAt(2).value || "--")
                        tone: String(root.metricAt(2).tone || "success")
                    }

                    RailMetricCard {
                        Layout.fillWidth: true
                        tokens: root.tokens
                        title: String(root.metricAt(4).label || "证据包")
                        value: String(root.metricAt(4).value || "--")
                        tone: String(root.metricAt(4).tone || "ai")
                    }

                    RailMetricCard {
                        Layout.fillWidth: true
                        tokens: root.tokens
                        title: String(root.metricAt(5).label || "增量更新")
                        value: String(root.metricAt(5).value || "--")
                        tone: String(root.metricAt(5).tone || "warning")
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: String(root.algorithmSummary.evidenceLine || "")
                    wrapMode: Text.WordWrap
                    color: root.tokens.text2
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 11
                    lineHeight: 1.16
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ActionButton {
                Layout.fillWidth: true
                tokens: root.tokens
                text: root.analysisController.analyzing ? "停止" : "快速建模"
                hint: root.analysisController.analyzing
                      ? "停止当前计算任务，保留已经生成的阶段结果。"
                      : "快速整理源码事实、能力域和架构地图，适合先建立全景。"
                disabledHint: "先选择一个项目目录，才能开始快速建模。"
                compact: true
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
                Layout.fillWidth: true
                tokens: root.tokens
                text: "精确推演"
                hint: "接入 compile_commands.json 和语义后端，提升关系推断精度。"
                disabledHint: root.analysisController.analyzing
                              ? "当前正在计算，请等待结束后再启动精确推演。"
                              : "先选择项目，再启动精确推演。"
                compact: true
                tone: "secondary"
                enabled: root.caseState.hasProject && !root.analysisController.analyzing
                onClicked: root.analysisController.analyzeCurrentProjectHighPrecision()
            }
        }

        ActionButton {
            Layout.fillWidth: true
            tokens: root.tokens
            text: "AI 复盘"
            hint: "让 AI 基于当前重建结果总结算法亮点、证据薄弱点和下一步补强方向。"
            disabledHint: "AI 服务未配置或当前不可用。"
            compact: true
            tone: "ai"
            enabled: root.analysisController.aiAvailable
            onClicked: root.requestProjectAiReview()
        }
    }

    component RailMetricCard: Rectangle {
        required property QtObject tokens
        property string title: ""
        property string value: ""
        property string tone: "info"

        radius: tokens.radiusLg
        color: tokens.toneSoft(tone)
        border.color: Qt.rgba(tokens.toneColor(tone).r,
                              tokens.toneColor(tone).g,
                              tokens.toneColor(tone).b,
                              0.24)
        implicitHeight: contentColumn.implicitHeight + 16

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 8
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: title
                color: tokens.text3
                font.family: tokens.textFontFamily
                font.pixelSize: 10
            }

            Label {
                Layout.fillWidth: true
                text: value
                color: tokens.text1
                font.family: tokens.textFontFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
            }
        }
    }
}
