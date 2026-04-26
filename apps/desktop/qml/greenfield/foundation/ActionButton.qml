import QtQuick
import QtQuick.Controls

Button {
    id: control

    required property QtObject tokens
    property string tone: "secondary"
    property bool compact: false
    property bool square: false
    property int fixedWidth: 0
    property string hint: ""
    property string disabledHint: ""
    readonly property string effectiveHint: !enabled && disabledHint.length > 0 ? disabledHint : hint

    readonly property color fillColor: {
        if (tone === "primary")
            return tokens.signalCobalt
        if (tone === "analysis")
            return tokens.signalCobaltSoft
        if (tone === "danger")
            return tokens.signalCoral
        if (tone === "ai")
            return tokens.signalRaspberry
        if (tone === "ghost")
            return Qt.rgba(1, 1, 1, 0.0)
        return tokens.panelStrong
    }
    readonly property color fillColor2: {
        if (tone === "primary")
            return Qt.darker(tokens.signalCobalt, 1.10)
        if (tone === "analysis")
            return tokens.panelStrong
        if (tone === "danger")
            return Qt.darker(tokens.signalCoral, 1.04)
        if (tone === "ai")
            return Qt.lighter(tokens.signalRaspberry, 1.06)
        if (tone === "ghost")
            return Qt.rgba(1, 1, 1, 0.0)
        return tokens.panelSoft
    }
    readonly property color strokeColor: {
        if (tone === "primary")
            return Qt.rgba(tokens.signalCobalt.r, tokens.signalCobalt.g, tokens.signalCobalt.b, 0.22)
        if (tone === "analysis")
            return Qt.rgba(tokens.signalCobalt.r, tokens.signalCobalt.g, tokens.signalCobalt.b, 0.28)
        if (tone === "danger")
            return Qt.rgba(tokens.signalCoral.r, tokens.signalCoral.g, tokens.signalCoral.b, 0.3)
        if (tone === "ai")
            return Qt.rgba(tokens.signalRaspberry.r, tokens.signalRaspberry.g, tokens.signalRaspberry.b, 0.26)
        if (tone === "ghost")
            return tokens.border2
        return tokens.border1
    }
    readonly property color labelColor: {
        if (tone === "primary" || tone === "danger" || tone === "ai")
            return "#FFFFFF"
        if (tone === "analysis")
            return tokens.signalCobalt
        return tokens.text1
    }

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    transformOrigin: Item.Center
    scale: !enabled ? 1.0 : (down ? 0.975 : (hovered ? 1.018 : 1.0))
    opacity: enabled ? 1.0 : 0.62
    z: hovered || down || activeFocus ? 4 : 0
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
    Accessible.name: text
    Accessible.description: effectiveHint
    ToolTip.visible: effectiveHint.length > 0 && hovered
    ToolTip.text: effectiveHint
    ToolTip.delay: 620
    ToolTip.timeout: 5200

    Behavior on scale {
        NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
    }

    Behavior on opacity {
        NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
    }

    HoverHandler {
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

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
        color: "transparent"
        border.width: 1
        border.color: control.enabled
                      ? control.strokeColor
                      : Qt.rgba(control.strokeColor.r, control.strokeColor.g, control.strokeColor.b, 0.4)

        Behavior on border.color {
            ColorAnimation { duration: 120; easing.type: Easing.OutCubic }
        }

        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: {
                    if (!control.enabled)
                        return Qt.rgba(control.fillColor.r, control.fillColor.g, control.fillColor.b, 0.36)
                    if (control.down && control.tone !== "ghost")
                        return Qt.darker(control.fillColor, 1.08)
                    if (control.hovered && control.tone === "analysis")
                        return Qt.rgba(control.tokens.signalCobalt.r, control.tokens.signalCobalt.g, control.tokens.signalCobalt.b, 0.11)
                    if (control.hovered && control.tone === "secondary")
                        return Qt.rgba(1, 1, 1, 0.94)
                    if (control.hovered && control.tone === "ghost")
                        return Qt.rgba(1, 1, 1, 0.34)
                    if (control.hovered)
                        return Qt.lighter(control.fillColor, 1.04)
                    return control.fillColor
                }

                Behavior on color {
                    ColorAnimation { duration: 120; easing.type: Easing.OutCubic }
                }
            }

            GradientStop {
                position: 1.0
                color: {
                    if (!control.enabled)
                        return Qt.rgba(control.fillColor2.r, control.fillColor2.g, control.fillColor2.b, 0.28)
                    if (control.down && control.tone !== "ghost")
                        return Qt.darker(control.fillColor2, 1.08)
                    if (control.hovered && control.tone === "analysis")
                        return Qt.rgba(1, 1, 1, 0.94)
                    if (control.hovered && control.tone === "secondary")
                        return Qt.rgba(0.952, 0.952, 0.968, 0.86)
                    if (control.hovered && control.tone === "ghost")
                        return Qt.rgba(0.952, 0.952, 0.968, 0.38)
                    if (control.hovered)
                        return Qt.lighter(control.fillColor2, 1.04)
                    return control.fillColor2
                }

                Behavior on color {
                    ColorAnimation { duration: 120; easing.type: Easing.OutCubic }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.color: control.tone === "ghost" ? "transparent" : control.tokens.shineBorder
            border.width: control.enabled ? 1 : 0
            opacity: control.tone === "secondary" || control.tone === "analysis" ? 0.28 : 0.18
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.color: control.tokens.signalCobalt
            border.width: control.activeFocus ? 2 : 0
            opacity: control.activeFocus ? 0.92 : 0.0

            Behavior on opacity {
                NumberAnimation { duration: 120; easing.type: Easing.OutCubic }
            }
        }
    }
}
