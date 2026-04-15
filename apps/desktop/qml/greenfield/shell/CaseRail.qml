import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property QtObject tokens
    required property QtObject caseState

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

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 24
        anchors.bottomMargin: 24
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 6

        Label {
            Layout.leftMargin: 8
            Layout.topMargin: 2
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

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: root.tokens.border1
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 6
            radius: root.tokens.radiusLg
            color: root.tokens.panelStrong
            border.color: root.tokens.border1
            implicitHeight: projectColumn.implicitHeight + 24

            ColumnLayout {
                id: projectColumn
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                Label {
                    text: "当前项目"
                    color: root.tokens.text3
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
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
    }
}
