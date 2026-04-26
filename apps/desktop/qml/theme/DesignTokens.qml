import QtQuick

QtObject {
    readonly property string displayFontFamily: "IBM Plex Sans, Segoe UI Variable Display, PingFang SC"
    readonly property string textFontFamily: "IBM Plex Sans, Segoe UI Variable Text, PingFang SC"
    readonly property string monoFontFamily: Qt.platform.os === "windows" ? "Consolas"
                                             : Qt.platform.os === "osx" ? "Menlo"
                                             : "monospace"

    readonly property color neutral0: "#FBFCFD"
    readonly property color neutral25: "#F6F8FA"
    readonly property color neutral50: "#EFF3F6"
    readonly property color neutral100: "#E3EAF0"
    readonly property color neutral200: "#D3DDE6"
    readonly property color neutral300: "#BBC8D4"
    readonly property color neutral500: "#6D7D8C"
    readonly property color neutral700: "#314252"
    readonly property color neutral900: "#0F1A24"

    readonly property color brand400: "#3A78B7"
    readonly property color brand500: "#245F98"
    readonly property color brand600: "#1C4E7E"
    readonly property color brand700: "#153C62"

    readonly property color ai400: "#2F8F8A"
    readonly property color ai500: "#247772"
    readonly property color ai600: "#1B5F5B"

    readonly property color success: "#1E7A63"
    readonly property color warning: "#B87417"
    readonly property color danger: "#A24B3C"
    readonly property color info: "#2E6EA3"

    readonly property color windowBase: "#F4F7FA"
    readonly property color windowTop: "#FCFDFE"
    readonly property color navBase: "#EEF3F7"
    readonly property color canvasBase: "#F8FBFD"
    readonly property color surfacePrimary: "#FFFFFF"
    readonly property color surfaceSecondary: "#F3F6F9"
    readonly property color surfaceTertiary: "#EAF0F5"
    readonly property color inspectorBase: "#F7FAFC"

    readonly property color textPrimary: "#132231"
    readonly property color textSecondary: "#425466"
    readonly property color textTertiary: "#6C7B89"
    readonly property color textInverse: "#F7FAFD"

    readonly property color borderSubtle: "#D7E0E8"
    readonly property color borderStrong: "#B4C2CF"
    readonly property color focusRing: "#78AEE4"
    readonly property color selectionRing: "#245F98"

    readonly property color graphGrid: "#E2E9EF"
    readonly property color graphEdge: "#7A8D9F"
    readonly property color graphEdgeFocus: "#245F98"

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
