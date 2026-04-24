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
            return "#edf2f6"
        if (tone === "accent")
            return down || checked ? "#173958" : (hovered ? "#2d5f91" : "#214d77")
        if (tone === "danger")
            return down || checked ? "#7e3d33" : (hovered ? "#ad5a4b" : "#9c4c3e")
        if (tone === "ghost")
            return down || checked ? "#dbe6ef" : (hovered ? "#f7fbff" : "#edf3f8")
        if (tone === "ai")
            return down || checked ? "#6842c2" : (hovered ? "#8264d8" : "#7657cf")
        return down || checked ? "#eef4f9" : (hovered ? "#f8fbfd" : "#ffffff")
    }

    function borderColor() {
        if (!enabled)
            return "#d7e0e8"
        if (activeFocus)
            return "#78AEE4"
        if (tone === "accent")
            return "#173958"
        if (tone === "danger")
            return "#7e3d33"
        if (tone === "ai")
            return "#6842c2"
        if (tone === "ghost")
            return "#cedae5"
        return checked ? "#7f98ae" : "#c8d5e2"
    }

    function textColor() {
        if (!enabled)
            return "#91a1af"
        if (tone === "accent" || tone === "danger" || tone === "ai")
            return "#f8fbff"
        return "#1c3148"
    }
}
