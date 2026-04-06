import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../common"

Item {
    id: root

    required property QtObject theme

    property var breadcrumbs: []

    signal navigateTo(string pageId)

    implicitWidth: crumbsRow.implicitWidth
    implicitHeight: crumbsRow.implicitHeight

    RowLayout {
        id: crumbsRow
        anchors.fill: parent
        spacing: 6

        Repeater {
            model: root.breadcrumbs || []

            delegate: RowLayout {
                spacing: 6

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: index === (root.breadcrumbs.length - 1) ? "ghost" : "neutral"
                    enabled: !!modelData.pageId && index < (root.breadcrumbs.length - 1)
                    text: modelData.label || ""
                    onClicked: root.navigateTo(modelData.pageId)
                }

                Label {
                    visible: index < (root.breadcrumbs.length - 1)
                    text: "/"
                    color: root.theme.inkMuted
                    font.family: root.theme.textFontFamily
                    font.pixelSize: 12
                }
            }
        }
    }
}
