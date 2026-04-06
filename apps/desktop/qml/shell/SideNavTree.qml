import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController

    property var sections: []
    property string currentPageId: ""

    readonly property bool hasProject: (root.analysisController.projectRootPath || "").length > 0

    signal pageRequested(string pageId)

    AppCard {
        anchors.fill: parent
        fillColor: root.theme.navBase
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXxl
        contentPadding: 14

        ColumnLayout {
            anchors.fill: parent
            spacing: 14

            AppCard {
                Layout.fillWidth: true
                fillColor: "#FFFFFF"
                borderColor: root.theme.borderSubtle
                cornerRadius: root.theme.radiusXl
                contentPadding: 14
                minimumContentHeight: 92

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "SAVT"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 24
                            font.weight: Font.DemiBold
                        }

                        Item { Layout.fillWidth: true }

                        StatusBadge {
                            theme: root.theme
                            text: root.analysisController.analyzing ? "分析中" : (root.hasProject ? "项目就绪" : "待连接")
                            tone: root.analysisController.analyzing ? "info" : (root.hasProject ? "success" : "neutral")
                        }

                        AppChip {
                            text: root.analysisController.analyzing
                                  ? (Math.round((root.analysisController.analysisProgress || 0) * 100) + "%")
                                  : ""
                            compact: true
                            fillColor: "#FFFFFF"
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "架构驾驶舱"
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.hasProject
                              ? root.analysisController.projectRootPath
                              : "先从顶部选择项目，之后这里会显示当前工作区路径和当前分析入口。"
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideMiddle
                        color: root.theme.inkNormal
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 11
                    }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: parent.width
                    spacing: 14

                    Repeater {
                        model: root.sections || []

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: modelData.title || ""
                                color: root.theme.inkMuted
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }

                            Repeater {
                                model: modelData.items || []

                                Rectangle {
                                    id: navItem

                                    Layout.fillWidth: true
                                    implicitHeight: current ? 56 : 50
                                    radius: root.theme.radiusLg

                                    readonly property bool current: root.currentPageId === modelData.id
                                    color: current ? root.theme.accentWash : (navMouseArea.containsMouse ? "#F7FAFC" : Qt.rgba(1, 1, 1, 0))
                                    border.color: current ? "#C7D8EA" : (navMouseArea.containsMouse ? root.theme.borderSubtle : Qt.rgba(0, 0, 0, 0))
                                    border.width: (current || navMouseArea.containsMouse) ? 1 : 0

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 10

                                        Rectangle {
                                            width: 4
                                            height: navItem.current ? 24 : 18
                                            radius: 2
                                            color: navItem.current ? root.theme.accentBase : "#D0DCE7"
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2

                                            Label {
                                                text: modelData.label || ""
                                                color: root.theme.inkStrong
                                                font.family: root.theme.displayFontFamily
                                                font.pixelSize: 15
                                                font.weight: navItem.current ? Font.DemiBold : Font.Medium
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.hint || ""
                                                wrapMode: Text.WordWrap
                                                maximumLineCount: 2
                                                elide: Text.ElideRight
                                                color: navItem.current ? root.theme.inkNormal : root.theme.inkMuted
                                                font.family: root.theme.textFontFamily
                                                font.pixelSize: 11
                                            }
                                        }
                                    }

                                    MouseArea {
                                        id: navMouseArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.pageRequested(modelData.id)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
