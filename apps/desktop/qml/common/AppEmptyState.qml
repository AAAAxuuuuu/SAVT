import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property QtObject theme

    property string title: ""
    property string description: ""
    property string actionText: ""

    signal actionRequested()

    implicitWidth: 420
    implicitHeight: 184

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width, 500)
        spacing: 10

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 62
            height: 62
            radius: 31
            color: theme.accentCanvas
            border.color: theme.borderSubtle

            Rectangle {
                anchors.centerIn: parent
                width: 24
                height: 24
                radius: 12
                color: "#FFFFFF"
                border.color: theme.borderSubtle
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.title
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: theme.inkStrong
            font.family: theme.displayFontFamily
            font.pixelSize: 22
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            text: root.description
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: theme.inkMuted
            font.family: theme.textFontFamily
            font.pixelSize: 13
        }

        AppButton {
            visible: root.actionText.length > 0
            Layout.alignment: Qt.AlignHCenter
            theme: root.theme
            tone: "accent"
            text: root.actionText
            onClicked: root.actionRequested()
        }
    }
}
