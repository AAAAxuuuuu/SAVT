import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme
    required property QtObject analysisController
    required property QtObject uiState

    property string eyebrow: ""
    property string title: ""
    property string summary: ""
    property var breadcrumbs: []
    property bool compactMode: false

    readonly property bool hasSelection: (root.uiState.inspector.title || "").length > 0
    readonly property bool showAiSetupHint: !root.analysisController.aiAvailable
                                        && (root.analysisController.aiSetupMessage || "").length > 0
    readonly property bool showRightColumn: root.analysisController.analyzing
                                            || root.showAiSetupHint
                                            || (!root.compactMode && root.hasSelection)

    signal navigateTo(string pageId)

    implicitHeight: card.implicitHeight

    function selectionSummary() {
        if (!root.hasSelection)
            return ""
        if (root.uiState.inspector.kind === "edge")
            return "当前关系 · " + root.uiState.inspector.title
        return "当前对象 · " + root.uiState.inspector.title
    }

    AppCard {
        id: card
        anchors.fill: parent
        fillColor: "#FFFFFF"
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXxl
        contentPadding: root.compactMode ? 10 : 14

        ColumnLayout {
            anchors.fill: parent
            spacing: root.compactMode ? 8 : 10

            BreadcrumbBar {
                Layout.fillWidth: true
                theme: root.theme
                breadcrumbs: root.breadcrumbs
                onNavigateTo: root.navigateTo(pageId)
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: root.compactMode ? 10 : 14

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        visible: root.eyebrow.length > 0
                        text: root.eyebrow
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }

                    Label {
                        text: root.title
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: root.compactMode ? 22 : 28
                        font.weight: Font.DemiBold
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.summary
                        wrapMode: Text.WordWrap
                        maximumLineCount: root.compactMode ? 1 : 2
                        elide: Text.ElideRight
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: root.compactMode ? 12 : 13
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: root.compactMode ? 232 : 280
                    Layout.alignment: Qt.AlignTop
                    spacing: 8
                    visible: root.showRightColumn

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: root.hasSelection || root.analysisController.analyzing

                        StatusBadge {
                            theme: root.theme
                            text: root.analysisController.analyzing
                                  ? ((root.analysisController.analysisPhase || "分析中") + " · "
                                     + Math.round((root.analysisController.analysisProgress || 0) * 100) + "%")
                                  : ""
                            tone: "info"
                        }

                        AppChip {
                            text: root.selectionSummary()
                            compact: true
                            fillColor: root.theme.surfaceSecondary
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: root.showAiSetupHint
                        text: root.analysisController.aiSetupMessage
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 11
                    }
                }
            }
        }
    }
}
