import QtQuick
import QtQuick.Controls

Button {
    id: control

    required property QtObject theme

    property string tone: "neutral"
    property bool compact: false

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    implicitHeight: compact ? 34 : 42
    leftPadding: compact ? 14 : 18
    rightPadding: compact ? 14 : 18
    topPadding: 0
    bottomPadding: 0
    font.family: theme.textFontFamily
    font.pixelSize: compact ? 13 : 14
    font.weight: Font.DemiBold

    background: Rectangle {
        radius: control.compact ? theme.radiusMd : theme.radiusLg
        color: control.fillColor()
        border.color: control.borderColor()
        border.width: control.activeFocus ? 2 : 1
        opacity: control.enabled ? 1.0 : 0.92
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
            return theme.surfaceTertiary
        if (tone === "accent")
            return down || checked
                   ? theme.accentStrong
                   : (hovered ? theme.accentMuted : theme.accentBase)
        if (tone === "danger")
            return down || checked ? "#8E4136" : (hovered ? "#B35645" : theme.danger)
        if (tone === "ghost")
            return down || checked
                   ? "#DDE7F0"
                   : (hovered ? "#F7FAFC" : Qt.rgba(1, 1, 1, 0.82))
        if (tone === "ai")
            return down || checked ? "#1D6763" : (hovered ? "#2B827D" : theme.aiAccent)
        return down || checked ? "#EFF4F8" : (hovered ? "#F9FBFD" : "#FFFFFF")
    }

    function borderColor() {
        if (!enabled)
            return theme.borderSubtle
        if (tone === "accent")
            return control.activeFocus ? theme.focusRing : theme.accentStrong
        if (tone === "danger")
            return control.activeFocus ? theme.focusRing : "#8E4136"
        if (tone === "ghost")
            return control.activeFocus ? theme.focusRing : "#CED9E4"
        if (tone === "ai")
            return control.activeFocus ? theme.focusRing : "#1D6763"
        return control.activeFocus ? theme.focusRing : (checked ? theme.selectionRing : "#C9D6E2")
    }

    function textColor() {
        if (!enabled)
            return "#8E9EAD"
        if (tone === "accent" || tone === "danger" || tone === "ai")
            return theme.inkInverse
        return theme.inkStrong
    }
}
