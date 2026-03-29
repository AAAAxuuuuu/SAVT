import QtQuick

QtObject {
    readonly property string displayFontFamily: Qt.platform.os === "windows" ? "Segoe UI" : "Avenir Next"
    readonly property string textFontFamily: Qt.platform.os === "windows" ? "Segoe UI" : "PingFang SC"
    readonly property string monoFontFamily: Qt.platform.os === "windows" ? "Cascadia Mono" : "Menlo"

    readonly property color windowBase: "#ebf1f4"
    readonly property color windowTop: "#f6f8fb"
    readonly property color windowWash: "#dbe7f0"
    readonly property color surfacePrimary: "#fbfdff"
    readonly property color surfaceSecondary: "#f2f6fa"
    readonly property color surfaceMuted: "#e9eff5"
    readonly property color borderSubtle: "#d7e0e8"
    readonly property color borderStrong: "#b6c5d3"
    readonly property color inkStrong: "#112133"
    readonly property color inkNormal: "#32465a"
    readonly property color inkMuted: "#65788a"
    readonly property color accentStrong: "#214d77"
    readonly property color accentSoft: "#d8e5f0"
    readonly property color accentMuted: "#8ea8c1"
    readonly property color tealSoft: "#dbefe8"
    readonly property color amberSoft: "#f9ecd4"
    readonly property color roseSoft: "#f5e3de"
    readonly property color success: "#1f7a69"
    readonly property color warning: "#b77319"
    readonly property color danger: "#9c4c3e"
    readonly property color graphGrid: "#dfe7ee"

    function nodeFill(kind, selected) {
        if (selected)
            return "#214d77"
        if (kind === "entry")
            return "#e0f2ec"
        if (kind === "infrastructure")
            return "#fff2db"
        return "#eef4f9"
    }

    function nodeBorder(kind, selected, pinned) {
        if (selected)
            return "#163754"
        if (pinned)
            return "#c0892e"
        if (kind === "entry")
            return "#8bb8ad"
        if (kind === "infrastructure")
            return "#d1a24f"
        return "#9bb2c7"
    }

    function nodeAccent(kind, selected) {
        if (selected)
            return "#d8e5f0"
        if (kind === "entry")
            return "#2f7f70"
        if (kind === "infrastructure")
            return "#b87818"
        return "#35608b"
    }

    function edgeColor(kind) {
        if (kind === "activates")
            return "#2f6aa0"
        if (kind === "uses_infrastructure")
            return "#bf8b2d"
        return "#71879c"
    }

    function groupFill(index) {
        var fills = ["#f6fafc", "#f8f6fb", "#fbf8f3"]
        return fills[index % fills.length]
    }

    function levelFill(index, selected) {
        if (selected)
            return index === 1 ? "#e1ebf4" : "#eaf1f7"
        return "#f7fafc"
    }

    function levelBorder(index, selected) {
        if (selected)
            return index === 1 ? "#5f7d99" : "#8aa2b8"
        return "#d7e0e8"
    }

    function levelInk(selected) {
        return selected ? "#163552" : "#55687a"
    }
}
