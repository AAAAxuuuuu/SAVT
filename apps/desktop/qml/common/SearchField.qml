import QtQuick
import QtQuick.Controls

TextField {
    id: control

    required property QtObject theme

    signal submitted(string text)

    placeholderText: "搜索"
    font.family: theme.textFontFamily
    font.pixelSize: 13
    leftPadding: 38
    rightPadding: clearButton.visible ? 38 : 14
    verticalAlignment: Text.AlignVCenter
    selectByMouse: true

    background: Rectangle {
        radius: theme.radiusLg
        color: "#FFFFFF"
        border.color: control.activeFocus ? theme.focusRing : theme.borderSubtle
        border.width: control.activeFocus ? 2 : 1
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        width: 18
        height: 18
        radius: 9
        color: Qt.rgba(0, 0, 0, 0)
        border.color: control.activeFocus ? theme.accentBase : theme.borderStrong
        border.width: 2

        Rectangle {
            width: 7
            height: 2
            radius: 1
            rotation: 45
            color: control.activeFocus ? theme.accentBase : theme.borderStrong
            x: parent.width - 1
            y: parent.height - 1
            transformOrigin: Item.Left
        }
    }

    Keys.onReturnPressed: submitted(text)

    Rectangle {
        id: clearButton
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        visible: control.text.length > 0
        width: 22
        height: 22
        radius: 11
        color: control.activeFocus ? "#E7EFF7" : "#F1F5F8"
        border.color: theme.borderSubtle

        Label {
            anchors.centerIn: parent
            text: "×"
            color: theme.inkMuted
            font.family: theme.textFontFamily
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: control.clear()
        }
    }
}
