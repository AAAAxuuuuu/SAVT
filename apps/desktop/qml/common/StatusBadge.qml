import QtQuick
import QtQuick.Controls

Control {
    id: root

    required property QtObject theme

    property string text: ""
    property string tone: "neutral"
    property real maximumWidth: 300

    readonly property var palette: theme.statusColors(tone)
    readonly property real clampedWidth: maximumWidth > 0
                                       ? maximumWidth
                                       : (label.implicitWidth + 22)

    implicitWidth: Math.min(label.implicitWidth + 22, clampedWidth)
    implicitHeight: label.implicitHeight + 12
    padding: 0
    clip: true
    visible: root.text.length > 0

    background: Rectangle {
        radius: root.height / 2
        color: root.palette.fill
        border.color: root.palette.border
        border.width: 1
    }

    contentItem: Label {
        id: label
        text: root.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        maximumLineCount: 1
        color: root.palette.text
        font.family: theme.textFontFamily
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }
}
