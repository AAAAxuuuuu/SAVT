import QtQuick

QtObject {
    readonly property string displayFontFamily: "Segoe UI Variable Display, Microsoft YaHei UI, PingFang SC"
    readonly property string textFontFamily: "Segoe UI Variable Text, Microsoft YaHei UI, PingFang SC"
    readonly property string monoFontFamily: "Cascadia Code, IBM Plex Mono, Consolas"

    readonly property color base0: "#FFFFFF"
    readonly property color base1: "#F5F5F7"
    readonly property color base2: "#ECECEF"
    readonly property color base3: "#D8D8DE"
    readonly property color sidebarBase: Qt.rgba(0.9608, 0.9608, 0.9686, 0.78)
    readonly property color topbarBase: Qt.rgba(0.9608, 0.9608, 0.9686, 0.68)
    readonly property color panelBase: Qt.rgba(1.0, 1.0, 1.0, 0.88)
    readonly property color searchBase: Qt.rgba(0, 0, 0, 0.05)
    readonly property color border1: Qt.rgba(0, 0, 0, 0.06)
    readonly property color border2: Qt.rgba(0, 0, 0, 0.12)

    readonly property color text1: "#1D1D1F"
    readonly property color text2: "#56565C"
    readonly property color text3: "#86868B"
    readonly property color textInverse: "#FFFFFF"

    readonly property color signalTeal: "#34C759"
    readonly property color signalTealSoft: "#E7F8EB"
    readonly property color signalCobalt: "#007AFF"
    readonly property color signalCobaltSoft: "#E5F1FF"
    readonly property color signalAmber: "#FF9500"
    readonly property color signalAmberSoft: "#FFF4E0"
    readonly property color signalCoral: "#FF3B30"
    readonly property color signalCoralSoft: "#FFE8E6"
    readonly property color signalRaspberry: "#AF52DE"
    readonly property color signalRaspberrySoft: "#F6E8FC"
    readonly property color signalMoss: "#30B0C7"
    readonly property color signalMossSoft: "#E4F7FA"

    readonly property color graphCanvas: "#F5F5F7"
    readonly property color graphGrid: Qt.rgba(0, 0, 0, 0.06)
    readonly property color graphEdge: "#A1A1A6"
    readonly property color graphEdgeFocus: "#007AFF"

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

    readonly property real panelOpacity: 0.88
    readonly property real sidebarOpacity: 0.78

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
