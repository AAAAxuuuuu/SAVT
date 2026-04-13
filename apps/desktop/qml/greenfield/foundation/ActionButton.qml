import QtQuick
import QtQuick.Controls

Button {
    id: control

    required property QtObject tokens
    property string tone: "secondary"
    property bool compact: false
    property bool square: false
    property int fixedWidth: 0

    readonly property color fillColor: {
        if (tone === "primary")
            return tokens.signalCobalt
        if (tone === "danger")
            return tokens.signalCoral
        if (tone === "ai")
            return tokens.signalRaspberry
        if (tone === "ghost")
            return Qt.rgba(1, 1, 1, 0.0)
        return Qt.rgba(1, 1, 1, 0.9)
    }
    readonly property color strokeColor: {
        if (tone === "primary")
            return Qt.darker(tokens.signalCobalt, 1.12)
        if (tone === "danger")
            return Qt.darker(tokens.signalCoral, 1.12)
        if (tone === "ai")
            return Qt.darker(tokens.signalRaspberry, 1.08)
        if (tone === "ghost")
            return tokens.border2
        return Qt.rgba(0, 0, 0, 0.08)
    }
    readonly property color labelColor: {
        if (tone === "primary" || tone === "danger" || tone === "ai")
            return "#FFFFFF"
        return tokens.text1
    }

    hoverEnabled: true
    padding: compact ? 10 : 14
    leftPadding: compact ? 12 : 16
    rightPadding: compact ? 12 : 16
    topPadding: compact ? 7 : 9
    bottomPadding: compact ? 7 : 9
    implicitHeight: compact ? 32 : 38
    implicitWidth: fixedWidth > 0
                   ? fixedWidth
                   : (square ? implicitHeight : Math.max(compact ? 72 : 88,
                                                         contentItem.implicitWidth + leftPadding + rightPadding))

    contentItem: Text {
        text: control.text
        color: control.labelColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        font.family: control.tokens.textFontFamily
        font.pixelSize: control.compact ? 12 : 13
        font.weight: Font.DemiBold
    }

    background: Rectangle {
        radius: control.compact ? control.tokens.radiusMd : control.tokens.radiusLg
        color: {
            if (!control.enabled)
                return Qt.rgba(control.fillColor.r, control.fillColor.g, control.fillColor.b, 0.45)
            if (control.down)
                return Qt.darker(control.fillColor, 1.12)
            if (control.hovered && control.tone === "secondary")
                return Qt.rgba(1, 1, 1, 0.96)
            if (control.hovered && control.tone === "ghost")
                return Qt.rgba(0, 0, 0, 0.04)
            if (control.hovered)
                return Qt.lighter(control.fillColor, 1.05)
            return control.fillColor
        }
        border.width: 1
        border.color: control.enabled
                      ? control.strokeColor
                      : Qt.rgba(control.strokeColor.r, control.strokeColor.g, control.strokeColor.b, 0.4)
    }
}
