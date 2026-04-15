import QtQuick

QtObject {
    readonly property string displayFontFamily: "Segoe UI Variable Display, Microsoft YaHei UI, PingFang SC"
    readonly property string textFontFamily: "Segoe UI Variable Text, Microsoft YaHei UI, PingFang SC"
    readonly property string monoFontFamily: "Cascadia Code, IBM Plex Mono, Consolas"

    readonly property color base0: "#FCFEFF"
    readonly property color base1: "#F4F6FB"
    readonly property color base2: "#EEF2F8"
    readonly property color base3: "#DCE5EF"
    readonly property color sidebarBase: Qt.rgba(1.0, 1.0, 1.0, 0.62)
    readonly property color topbarBase: Qt.rgba(1.0, 1.0, 1.0, 0.48)
    readonly property color panelBase: Qt.rgba(1.0, 1.0, 1.0, 0.72)
    readonly property color panelStrong: Qt.rgba(1.0, 1.0, 1.0, 0.82)
    readonly property color panelSoft: Qt.rgba(1.0, 1.0, 1.0, 0.58)
    readonly property color searchBase: Qt.rgba(1.0, 1.0, 1.0, 0.82)
    readonly property color border1: Qt.rgba(0.0588, 0.0902, 0.1647, 0.08)
    readonly property color border2: Qt.rgba(0.0588, 0.0902, 0.1647, 0.14)
    readonly property color shineBorder: Qt.rgba(1.0, 1.0, 1.0, 0.64)
    readonly property color shadowColor: Qt.rgba(0.0588, 0.0902, 0.1647, 0.12)

    readonly property color text1: "#0F172A"
    readonly property color text2: "#334155"
    readonly property color text3: "#64748B"
    readonly property color textInverse: "#FFFFFF"

    readonly property color signalTeal: "#22C55E"
    readonly property color signalTealSoft: "#E9FAEF"
    readonly property color signalCobalt: "#007AFF"
    readonly property color signalCobaltSoft: "#E7F1FF"
    readonly property color signalAmber: "#F59E0B"
    readonly property color signalAmberSoft: "#FFF5DF"
    readonly property color signalCoral: "#EF4444"
    readonly property color signalCoralSoft: "#FDECEC"
    readonly property color signalRaspberry: "#8B5CF6"
    readonly property color signalRaspberrySoft: "#F3EDFF"
    readonly property color signalMoss: "#32ADE6"
    readonly property color signalMossSoft: "#E8F7FD"

    readonly property color graphCanvas: Qt.rgba(0.9725, 0.9804, 0.9922, 0.82)
    readonly property color graphGrid: Qt.rgba(0.0588, 0.0902, 0.1647, 0.055)
    readonly property color graphEdge: "#97A7BA"
    readonly property color graphEdgeFocus: "#007AFF"
    readonly property color ambientBlue: Qt.rgba(0.0, 0.4784, 1.0, 0.18)
    readonly property color ambientViolet: Qt.rgba(0.4314, 0.4196, 1.0, 0.16)
    readonly property color ambientCyan: Qt.rgba(0.1961, 0.6784, 0.9020, 0.14)

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
