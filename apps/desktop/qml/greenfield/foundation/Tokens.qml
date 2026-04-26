import QtQuick

QtObject {
    readonly property string displayFontFamily: "Segoe UI Variable Display, Microsoft YaHei UI, PingFang SC"
    readonly property string textFontFamily: "Segoe UI Variable Text, Microsoft YaHei UI, PingFang SC"
    readonly property string monoFontFamily: Qt.platform.os === "windows" ? "Consolas"
                                             : Qt.platform.os === "osx" ? "Menlo"
                                             : "monospace"

    readonly property color base0: "#F5F5F7"
    readonly property color base1: "#F2F2F7"
    readonly property color base2: "#E8E8ED"
    readonly property color base3: "#D1D1D6"
    readonly property color sidebarBase: Qt.rgba(1.0, 1.0, 1.0, 0.72)
    readonly property color topbarBase: Qt.rgba(1.0, 1.0, 1.0, 0.66)
    readonly property color panelBase: Qt.rgba(1.0, 1.0, 1.0, 0.78)
    readonly property color panelStrong: Qt.rgba(1.0, 1.0, 1.0, 0.90)
    readonly property color panelSoft: Qt.rgba(0.972, 0.972, 0.984, 0.74)
    readonly property color searchBase: Qt.rgba(1.0, 1.0, 1.0, 0.86)
    readonly property color border1: Qt.rgba(0.235, 0.235, 0.263, 0.12)
    readonly property color border2: Qt.rgba(0.235, 0.235, 0.263, 0.20)
    readonly property color shineBorder: Qt.rgba(1.0, 1.0, 1.0, 0.66)
    readonly property color shadowColor: Qt.rgba(0.0, 0.0, 0.0, 0.12)

    readonly property color text1: "#1D1D1F"
    readonly property color text2: "#3C3C43"
    readonly property color text3: "#8E8E93"
    readonly property color textInverse: "#FFFFFF"

    readonly property color signalTeal: "#5AC8FA"
    readonly property color signalTealSoft: "#EDF9FF"
    readonly property color signalCobalt: "#007AFF"
    readonly property color signalCobaltSoft: "#EAF4FF"
    readonly property color signalAmber: "#FF9500"
    readonly property color signalAmberSoft: "#FFF4E5"
    readonly property color signalCoral: "#FF3B30"
    readonly property color signalCoralSoft: "#FFEDEC"
    readonly property color signalRaspberry: "#5856D6"
    readonly property color signalRaspberrySoft: "#EFEFFF"
    readonly property color signalMoss: "#64D2FF"
    readonly property color signalMossSoft: "#EFFBFF"

    readonly property color graphCanvas: "#F5F5F7"
    readonly property color graphGrid: Qt.rgba(0.235, 0.235, 0.263, 0.055)
    readonly property color graphEdge: "#A2A2AA"
    readonly property color graphEdgeFocus: "#007AFF"
    readonly property color ambientBlue: Qt.rgba(0.0, 0.478, 1.0, 0.07)
    readonly property color ambientViolet: Qt.rgba(0.345, 0.337, 0.839, 0.045)
    readonly property color ambientCyan: Qt.rgba(0.353, 0.784, 0.980, 0.055)

    readonly property int radiusXs: 6
    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16
    readonly property int radiusXl: 20
    readonly property int radiusXxl: 24
    readonly property int radiusPill: 999

    // Backward-compatible aliases used by the current greenfield shell.
    readonly property int radius2: radiusXs
    readonly property int radius4: radiusSm
    readonly property int radius6: radiusMd
    readonly property int radius8: radiusLg

    readonly property int space4: 4
    readonly property int space8: 8
    readonly property int space12: 12
    readonly property int space16: 16
    readonly property int space20: 20
    readonly property int space24: 24
    readonly property int space32: 32

    readonly property real panelOpacity: 0.82
    readonly property real sidebarOpacity: 0.62

    function toneColor(tone) {
        if (tone === "success")
            return signalTeal
        if (tone === "warning")
            return signalAmber
        if (tone === "danger")
            return signalCoral
        if (tone === "ai")
            return signalRaspberry
        if (tone === "moss")
            return signalMoss
        return signalCobalt
    }

    function toneSoft(tone) {
        if (tone === "success")
            return signalTealSoft
        if (tone === "warning")
            return signalAmberSoft
        if (tone === "danger")
            return signalCoralSoft
        if (tone === "ai")
            return signalRaspberrySoft
        if (tone === "moss")
            return signalMossSoft
        return signalCobaltSoft
    }
}
