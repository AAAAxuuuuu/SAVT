import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme

    property string title: ""
    property string summary: ""
    property var chips: []
    property string primaryText: ""
    property bool primaryEnabled: true
    property string secondaryText: ""
    property bool secondaryVisible: false
    property bool copyVisible: false
    property bool copyEnabled: false

    signal primaryRequested()
    signal secondaryRequested()
    signal askRequested()
    signal copyRequested()

    implicitHeight: card.implicitHeight

    AppCard {
        id: card
        anchors.fill: parent
        fillColor: root.theme.accentCanvas
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXl
        contentPadding: 16

        RowLayout {
            anchors.fill: parent
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                Label {
                    text: root.title
                    color: root.theme.inkStrong
                    font.family: root.theme.displayFontFamily
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    text: root.summary
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    color: root.theme.inkMuted
                    font.family: root.theme.textFontFamily
                    font.pixelSize: 12
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: root.chips || []

                        AppChip {
                            text: modelData.text || ""
                            compact: true
                            fillColor: modelData.fillColor || "#FFFFFF"
                            borderColor: modelData.borderColor || root.theme.borderSubtle
                            textColor: modelData.textColor || root.theme.inkNormal
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: 8

                AppButton {
                    visible: root.primaryText.length > 0
                    theme: root.theme
                    compact: true
                    tone: "accent"
                    text: root.primaryText
                    enabled: root.primaryEnabled
                    onClicked: root.primaryRequested()
                }

                RowLayout {
                    spacing: 8

                    AppButton {
                        visible: root.secondaryVisible
                        theme: root.theme
                        compact: true
                        tone: "ghost"
                        text: root.secondaryText
                        onClicked: root.secondaryRequested()
                    }

                    AppButton {
                        theme: root.theme
                        compact: true
                        tone: "ai"
                        text: "Ask SAVT"
                        onClicked: root.askRequested()
                    }

                    AppButton {
                        visible: root.copyVisible
                        theme: root.theme
                        compact: true
                        tone: "ghost"
                        text: "复制上下文"
                        enabled: root.copyEnabled
                        onClicked: root.copyRequested()
                    }
                }
            }
        }
    }
}
