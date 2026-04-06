import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property QtObject theme

    property string eyebrow: ""
    property string title: ""
    property string subtitle: ""

    default property alias contentData: contentColumn.data

    implicitWidth: layout.implicitWidth
    implicitHeight: layout.implicitHeight

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                visible: root.eyebrow.length > 0
                text: root.eyebrow
                color: theme.inkMuted
                font.family: theme.textFontFamily
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }

            Label {
                text: root.title
                color: theme.inkStrong
                font.family: theme.displayFontFamily
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            Label {
                visible: root.subtitle.length > 0
                Layout.fillWidth: true
                text: root.subtitle
                wrapMode: Text.WordWrap
                color: theme.inkMuted
                font.family: theme.textFontFamily
                font.pixelSize: 12
            }
        }

        ColumnLayout {
            id: contentColumn
            Layout.fillWidth: true
            spacing: 12
        }
    }
}
