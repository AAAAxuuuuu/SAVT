import QtQuick

QtObject {
    required property QtObject tokens

    readonly property string displayFontFamily: tokens.displayFontFamily
    readonly property string textFontFamily: tokens.textFontFamily
    readonly property string monoFontFamily: tokens.monoFontFamily

    readonly property color windowBase: tokens.windowBase
    readonly property color windowTop: tokens.windowTop
    readonly property color navBase: tokens.navBase
    readonly property color canvasBase: tokens.canvasBase
    readonly property color surfacePrimary: tokens.surfacePrimary
    readonly property color surfaceSecondary: tokens.surfaceSecondary
    readonly property color surfaceTertiary: tokens.surfaceTertiary
    readonly property color inspectorBase: tokens.inspectorBase

    readonly property color borderSubtle: tokens.borderSubtle
    readonly property color borderStrong: tokens.borderStrong
    readonly property color focusRing: tokens.focusRing
    readonly property color selectionRing: tokens.selectionRing

    readonly property color inkStrong: tokens.textPrimary
    readonly property color inkNormal: tokens.textSecondary
    readonly property color inkMuted: tokens.textTertiary
    readonly property color inkInverse: tokens.textInverse

    readonly property color accentStrong: tokens.brand600
    readonly property color accentBase: tokens.brand500
    readonly property color accentMuted: "#4B7FB2"
    readonly property color accentSoft: "#DCE8F3"
    readonly property color accentWash: "#EEF5FB"
    readonly property color accentCanvas: "#F2F7FB"
    readonly property color aiAccent: tokens.ai500
    readonly property color aiSoft: "#DCEFEB"
    readonly property color aiWash: "#EEF8F6"

    readonly property color success: tokens.success
    readonly property color warning: tokens.warning
    readonly property color danger: tokens.danger
    readonly property color info: tokens.info

    readonly property color graphGrid: tokens.graphGrid
    readonly property color graphEdge: tokens.graphEdge
    readonly property color graphEdgeFocus: tokens.graphEdgeFocus

    readonly property int space1: tokens.space1
    readonly property int space2: tokens.space2
    readonly property int space3: tokens.space3
    readonly property int space4: tokens.space4
    readonly property int space5: tokens.space5
    readonly property int space6: tokens.space6
    readonly property int space7: tokens.space7
    readonly property int space8: tokens.space8

    readonly property int radiusSm: tokens.radiusSm
    readonly property int radiusMd: tokens.radiusMd
    readonly property int radiusLg: tokens.radiusLg
    readonly property int radiusXl: tokens.radiusXl
    readonly property int radiusXxl: tokens.radiusXxl

    function blend(baseColor, overlayColor, overlayOpacity) {
        return Qt.rgba(
                    (baseColor.r * (1.0 - overlayOpacity)) + (overlayColor.r * overlayOpacity),
                    (baseColor.g * (1.0 - overlayOpacity)) + (overlayColor.g * overlayOpacity),
                    (baseColor.b * (1.0 - overlayOpacity)) + (overlayColor.b * overlayOpacity),
                    1.0)
    }

    function statusColors(tone) {
        if (tone === "success")
            return {"fill": "#EAF6F2", "border": "#A8D0C1", "text": success}
        if (tone === "warning")
            return {"fill": "#FFF6E8", "border": "#E2C18B", "text": warning}
        if (tone === "danger")
            return {"fill": "#F8ECEA", "border": "#D9ADA7", "text": danger}
        if (tone === "info")
            return {"fill": "#EAF2FA", "border": "#A9C7E1", "text": info}
        if (tone === "brand")
            return {"fill": "#E8F0F8", "border": "#A9C3DD", "text": accentStrong}
        if (tone === "ai")
            return {"fill": "#E7F3F1", "border": "#A8D3CE", "text": aiAccent}
        return {"fill": surfaceSecondary, "border": borderSubtle, "text": inkNormal}
    }

    function nodeFill(kind, selected) {
        if (selected)
            return accentStrong
        if (kind === "entry" || kind === "entry_component")
            return "#E3F4EE"
        if (kind === "infrastructure")
            return "#FFF2DC"
        if (kind === "support_component")
            return "#F6F0E3"
        return "#EDF3F8"
    }

    function nodeBorder(kind, selected, pinned) {
        if (selected)
            return accentStrong
        if (pinned)
            return "#B78228"
        if (kind === "entry" || kind === "entry_component")
            return "#7CB4A5"
        if (kind === "infrastructure")
            return "#D1A24F"
        if (kind === "support_component")
            return "#B89A65"
        return "#9FB4C8"
    }

    function nodeAccent(kind, selected) {
        if (selected)
            return "#D9E7F3"
        if (kind === "entry" || kind === "entry_component")
            return "#2E8176"
        if (kind === "infrastructure")
            return "#B87417"
        if (kind === "support_component")
            return "#8D6F34"
        return accentBase
    }

    function edgeColor(kind) {
        if (kind === "activates")
            return accentBase
        if (kind === "uses_infrastructure" || kind === "uses_support")
            return "#B88328"
        return graphEdge
    }

    function groupFill(index) {
        var fills = ["#F6FAFC", "#F9F7FB", "#FBF9F5", "#F4F8FB"]
        return fills[index % fills.length]
    }

    function elevatedSurface(baseColor, hovered, emphasized) {
        var overlay = emphasized ? accentWash : "#FFFFFF"
        var opacity = emphasized ? 0.72 : (hovered ? 0.36 : 0.16)
        return blend(Qt.color(baseColor), Qt.color(overlay), opacity)
    }

    function riskTone(level) {
        if (level === "high")
            return "danger"
        if (level === "medium")
            return "warning"
        return "success"
    }
}
