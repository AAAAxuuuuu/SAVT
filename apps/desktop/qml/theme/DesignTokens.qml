import QtQuick

QtObject {
    readonly property string displayFontFamily: "IBM Plex Sans, Segoe UI Variable Display, PingFang SC"
    readonly property string textFontFamily: "IBM Plex Sans, Segoe UI Variable Text, PingFang SC"
    readonly property string monoFontFamily: Qt.platform.os === "windows" ? "Consolas"
                                             : Qt.platform.os === "osx" ? "Menlo"
                                             : "monospace"

    readonly property color neutral0: "#FFFFFF"
    readonly property color neutral25: "#FBFBFD"
    readonly property color neutral50: "#F5F5F7"
    readonly property color neutral100: "#F2F2F7"
    readonly property color neutral200: "#E5E5EA"
    readonly property color neutral300: "#D1D1D6"
    readonly property color neutral500: "#8E8E93"
    readonly property color neutral700: "#3C3C43"
    readonly property color neutral900: "#1D1D1F"

    readonly property color brand400: "#1A86FF"
    readonly property color brand500: "#007AFF"
    readonly property color brand600: "#006EDB"
    readonly property color brand700: "#005BB5"

    readonly property color ai400: "#6866E0"
    readonly property color ai500: "#5856D6"
    readonly property color ai600: "#4846C8"

    readonly property color success: "#5AC8FA"
    readonly property color warning: "#FF9500"
    readonly property color danger: "#FF3B30"
    readonly property color info: "#007AFF"

    readonly property color windowBase: "#F5F5F7"
    readonly property color windowTop: "#FBFBFD"
    readonly property color navBase: "#F2F2F7"
    readonly property color canvasBase: "#F5F5F7"
    readonly property color surfacePrimary: "#FFFFFF"
    readonly property color surfaceSecondary: "#F2F2F7"
    readonly property color surfaceTertiary: "#E5E5EA"
    readonly property color inspectorBase: "#FBFBFD"

    readonly property color textPrimary: "#1D1D1F"
    readonly property color textSecondary: "#3C3C43"
    readonly property color textTertiary: "#8E8E93"
    readonly property color textInverse: "#FFFFFF"

    readonly property color borderSubtle: "#D1D1D6"
    readonly property color borderStrong: "#C7C7CC"
    readonly property color focusRing: "#5AC8FA"
    readonly property color selectionRing: "#007AFF"

    readonly property color graphGrid: "#E5E5EA"
    readonly property color graphEdge: "#A2A2AA"
    readonly property color graphEdgeFocus: "#007AFF"

    readonly property int space1: 4
    readonly property int space2: 8
    readonly property int space3: 12
    readonly property int space4: 16
    readonly property int space5: 20
    readonly property int space6: 24
    readonly property int space7: 32
    readonly property int space8: 40

    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16
    readonly property int radiusXl: 20
    readonly property int radiusXxl: 24
}
