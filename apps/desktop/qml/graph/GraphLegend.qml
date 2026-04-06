import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme

    property string mode: "capability"

    implicitWidth: card.implicitWidth
    implicitHeight: card.implicitHeight

    AppCard {
        id: card
        anchors.fill: parent
        fillColor: "#FFFFFF"
        borderColor: root.theme.borderSubtle
        cornerRadius: root.theme.radiusXl
        contentPadding: 10
        minimumContentWidth: 208
        minimumContentHeight: 72

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Label {
                text: root.mode === "component" ? "组件图例" : "能力图例"
                color: root.theme.inkStrong
                font.family: root.theme.displayFontFamily
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: root.mode === "component"
                           ? [
                                 {"label": "入口组件", "fill": "#E3F4EE", "border": "#7CB4A5"},
                                 {"label": "核心组件", "fill": "#EDF3F8", "border": "#9FB4C8"},
                                 {"label": "支撑组件", "fill": "#F6F0E3", "border": "#B89A65"}
                             ]
                           : [
                                 {"label": "入口能力", "fill": "#E3F4EE", "border": "#7CB4A5"},
                                 {"label": "核心能力", "fill": "#EDF3F8", "border": "#9FB4C8"},
                                 {"label": "基础设施", "fill": "#FFF2DC", "border": "#D1A24F"}
                             ]

                    AppChip {
                        text: modelData.label
                        compact: true
                        fillColor: modelData.fill
                        borderColor: modelData.border
                        textColor: root.theme.inkNormal
                    }
                }
            }
        }
    }
}
