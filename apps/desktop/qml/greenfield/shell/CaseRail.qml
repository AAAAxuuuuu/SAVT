import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property QtObject tokens
    required property QtObject caseState

    color: tokens.sidebarBase

    function itemIconColor(route) {
        return root.caseState.route === route ? "#FFFFFF" : root.tokens.text3
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 24
        anchors.bottomMargin: 24
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 4

        Label {
            Layout.leftMargin: 8
            Layout.bottomMargin: 12
            text: "SAVT WORKSPACE"
            color: root.tokens.text3
            font.family: root.tokens.textFontFamily
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        Repeater {
            model: [
                {"route": "overview", "label": "架构全景图"},
                {"route": "component", "label": "组件探测实验室"},
                {"route": "report", "label": "深度诊断报告"},
                {"route": "config", "label": "分析环境配置"}
            ]

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                radius: root.tokens.radius8
                color: root.caseState.route === modelData.route ? root.tokens.signalCobalt : "transparent"

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
                        color: root.caseState.route === modelData.route ? "#FFFFFF" : root.tokens.text1
                        font.family: root.tokens.textFontFamily
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.caseState.navigate(modelData.route)
                }
            }
        }

        Item { Layout.fillHeight: true }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.tokens.border1
        }

        Label {
            Layout.leftMargin: 8
            Layout.topMargin: 8
            text: "当前项目"
            color: root.tokens.text3
            font.family: root.tokens.textFontFamily
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            text: root.caseState.projectName()
            color: root.tokens.text2
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            font.family: root.tokens.textFontFamily
            font.pixelSize: 12
        }
    }
}
