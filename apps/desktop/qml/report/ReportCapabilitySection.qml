import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController

    signal capabilityRequested(string capabilityName)

    implicitHeight: card.implicitHeight

    AppCard {
        id: card
        anchors.fill: parent
        fillColor: "#FFFFFF"
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXxl

        AppSection {
            anchors.fill: parent
            theme: root.theme
            eyebrow: "能力映射"
            title: "关键能力与目录映射"
            subtitle: "把能力域卡片、容器线索和文件映射放在同一个阅读区，便于从报告回跳到能力地图。"

            Flow {
                Layout.fillWidth: true
                spacing: 8
                visible: (root.analysisController.systemContextData.containerNames || []).length > 0

                Repeater {
                    model: root.analysisController.systemContextData.containerNames || []

                    AppChip {
                        text: modelData
                        compact: true
                        fillColor: "#FFFFFF"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: width > 1080 ? 3 : (width > 760 ? 2 : 1)
                columnSpacing: 12
                rowSpacing: 12

                Repeater {
                    model: root.analysisController.systemContextCards || []

                    AppCard {
                        Layout.fillWidth: true
                        fillColor: root.theme.surfaceSecondary
                        borderColor: root.theme.borderSubtle
                        cornerRadius: root.theme.radiusLg
                        minimumContentHeight: 118

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: modelData.name || ""
                                wrapMode: Text.WordWrap
                                color: root.theme.inkStrong
                                font.family: root.theme.displayFontFamily
                                font.pixelSize: 17
                                font.weight: Font.DemiBold
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.summary || ""
                                wrapMode: Text.WordWrap
                                color: root.theme.inkMuted
                                font.family: root.theme.textFontFamily
                                font.pixelSize: 12
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.capabilityRequested(modelData.capabilityName || modelData.name || "")
                        }
                    }
                }
            }

            Label {
                visible: (root.analysisController.systemContextCards || []).length === 0
                text: "还没有形成可展示的能力卡片，先执行一次分析。"
                color: root.theme.inkMuted
                font.family: root.theme.textFontFamily
                font.pixelSize: 12
            }
        }
    }
}
