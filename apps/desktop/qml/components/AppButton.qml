import QtQuick
import QtQuick.Controls

Button {
    id: control

    required property QtObject theme
    property string tone: "neutral"
    property bool compact: false

    implicitHeight: compact ? 34 : 42
    leftPadding: compact ? 14 : 18
    rightPadding: compact ? 14 : 18
    topPadding: 0
    bottomPadding: 0
    font.family: theme.textFontFamily
    font.pixelSize: compact ? 13 : 14
    font.weight: Font.DemiBold

    background: Rectangle {
        radius: control.compact ? 12 : 16
        color: control.fillColor()
        border.color: control.borderColor()
        border.width: 1
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
            return down || checked ? "#173958" : "#214d77"
        if (tone === "danger")
            return down || checked ? "#7e3d33" : "#9c4c3e"
        if (tone === "ghost")
            return down || checked ? "#dbe6ef" : "#edf3f8"
        return down || checked ? "#eef4f9" : "#ffffff"
    }

    function borderColor() {
        if (!enabled)
            return "#d7e0e8"
        if (tone === "accent")
            return "#173958"
        if (tone === "danger")
            return "#7e3d33"
        if (tone === "ghost")
            return "#cedae5"
        return checked ? "#7f98ae" : "#c8d5e2"
    }

    function textColor() {
        if (!enabled)
            return "#91a1af"
        if (tone === "accent" || tone === "danger")
            return "#f8fbff"
        return "#1c3148"
    }
}
