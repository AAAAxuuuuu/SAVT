import QtQuick
import QtQuick.Controls

Control {
    id: root

    property string text: ""
    property color fillColor: "#eef4f9"
    property color borderColor: "#c7d6e4"
    property color textColor: "#28435c"
    property bool compact: false

    implicitWidth: label.implicitWidth + (compact ? 16 : 22)
    implicitHeight: label.implicitHeight + (compact ? 8 : 12)
    padding: 0

    background: Rectangle {
        radius: root.height / 2
        color: root.fillColor
        border.color: root.borderColor
        border.width: 1
    }

    contentItem: Label {
        id: label
        text: root.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: root.textColor
        font.pixelSize: root.compact ? 11 : 12
        font.weight: Font.DemiBold
    }
}
