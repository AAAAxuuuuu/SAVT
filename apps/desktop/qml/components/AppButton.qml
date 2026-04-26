import QtQuick
import QtQuick.Controls

Button {
    id: control

    required property QtObject theme
    property string tone: "neutral"
    property bool compact: false
    property string hint: ""
    property string disabledHint: ""
    readonly property string effectiveHint: !enabled && disabledHint.length > 0 ? disabledHint : hint

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    transformOrigin: Item.Center
    scale: !enabled ? 1.0 : (down ? 0.975 : (hovered ? 1.015 : 1.0))
    opacity: enabled ? 1.0 : 0.68
    z: hovered || down || activeFocus ? 4 : 0
    implicitHeight: compact ? 34 : 42
    leftPadding: compact ? 14 : 18
    rightPadding: compact ? 14 : 18
    topPadding: 0
    bottomPadding: 0
    font.family: theme.textFontFamily
    font.pixelSize: compact ? 13 : 14
    font.weight: Font.DemiBold
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

    background: Rectangle {
        radius: control.compact ? 12 : 16
        color: control.fillColor()
        border.color: control.borderColor()
        border.width: control.activeFocus ? 2 : 1

        Behavior on color {
            ColorAnimation { duration: 120; easing.type: Easing.OutCubic }
        }

        Behavior on border.color {
            ColorAnimation { duration: 120; easing.type: Easing.OutCubic }
        }
    }

    contentItem: Label {
        text: control.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: control.textColor()
        font: control.font
        elide: Text.ElideRight
    }

    function fillColor() {
        if (!enabled)
            return "#F2F2F7"
        if (tone === "accent")
            return down || checked ? "#006EDB" : (hovered ? "#1A86FF" : "#007AFF")
        if (tone === "danger")
            return down || checked ? "#D70015" : (hovered ? "#FF453A" : "#FF3B30")
        if (tone === "ghost")
            return down || checked ? "#E5E5EA" : (hovered ? "#FFFFFF" : "#F2F2F7")
        if (tone === "ai")
            return down || checked ? "#4846C8" : (hovered ? "#6866E0" : "#5856D6")
        return down || checked ? "#F2F2F7" : (hovered ? "#FFFFFF" : "#FFFFFF")
    }

    function borderColor() {
        if (!enabled)
            return "#D1D1D6"
        if (activeFocus)
            return "#5AC8FA"
        if (tone === "accent")
            return "#006EDB"
        if (tone === "danger")
            return "#D70015"
        if (tone === "ai")
            return "#4846C8"
        if (tone === "ghost")
            return "#D1D1D6"
        return checked ? "#5AC8FA" : "#D1D1D6"
    }

    function textColor() {
        if (!enabled)
            return "#8E8E93"
        if (tone === "accent" || tone === "danger" || tone === "ai")
            return "#f8fbff"
        return "#1D1D1F"
    }
}
