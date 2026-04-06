import QtQuick
import QtQuick.Controls

Control {
    id: root

    property string text: ""
    property color fillColor: "#EEF4F9"
    property color borderColor: "#C7D6E4"
    property color textColor: "#28435C"
    property bool compact: false
    property real maximumWidth: compact ? 240 : 320

    readonly property real horizontalInset: compact ? 16 : 22
    readonly property real clampedWidth: maximumWidth > 0
                                       ? maximumWidth
                                       : (label.implicitWidth + horizontalInset)

    implicitWidth: Math.min(label.implicitWidth + horizontalInset, clampedWidth)
    implicitHeight: label.implicitHeight + (compact ? 8 : 12)
    padding: 0
    clip: true
    visible: root.text.length > 0

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
        elide: Text.ElideRight
        maximumLineCount: 1
        color: root.textColor
        font.pixelSize: root.compact ? 11 : 12
        font.weight: Font.DemiBold
    }
}
