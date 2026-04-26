import QtQuick

QtObject {
    readonly property string displayFontFamily: Qt.platform.os === "windows" ? "Segoe UI" : "Avenir Next"
    readonly property string textFontFamily: Qt.platform.os === "windows" ? "Segoe UI" : "PingFang SC"
    readonly property string monoFontFamily: Qt.platform.os === "windows" ? "Cascadia Mono" : "Menlo"

    readonly property color windowBase: "#F5F5F7"
    readonly property color windowTop: "#FBFBFD"
    readonly property color windowWash: "#E8E8ED"
    readonly property color surfacePrimary: "#FFFFFF"
    readonly property color surfaceSecondary: "#F2F2F7"
    readonly property color surfaceMuted: "#E5E5EA"
    readonly property color borderSubtle: "#D1D1D6"
    readonly property color borderStrong: "#C7C7CC"
    readonly property color inkStrong: "#1D1D1F"
    readonly property color inkNormal: "#3C3C43"
    readonly property color inkMuted: "#8E8E93"
    readonly property color accentStrong: "#007AFF"
    readonly property color accentSoft: "#EAF4FF"
    readonly property color accentMuted: "#5AC8FA"
    readonly property color tealSoft: "#EDF9FF"
    readonly property color amberSoft: "#FFF4E5"
    readonly property color roseSoft: "#FFEDEC"
    readonly property color success: "#5AC8FA"
    readonly property color warning: "#FF9500"
    readonly property color danger: "#FF3B30"
    readonly property color graphGrid: "#E5E5EA"

    function nodeFill(kind, selected) {
        if (selected)
            return "#007AFF"
        if (kind === "entry" || kind === "entry_component")
            return "#EDF9FF"
        if (kind === "infrastructure")
            return "#FFF4E5"
        if (kind === "support_component")
            return "#F2F2F7"
        return "#EAF4FF"
    }

    function nodeBorder(kind, selected, pinned) {
        if (selected)
            return "#006EDB"
        if (pinned)
            return "#FF9500"
        if (kind === "entry" || kind === "entry_component")
            return "#5AC8FA"
        if (kind === "infrastructure")
            return "#FF9500"
        if (kind === "support_component")
            return "#AEAEB2"
        return "#A2A2AA"
    }

    function nodeAccent(kind, selected) {
        if (selected)
            return "#E5E5EA"
        if (kind === "entry" || kind === "entry_component")
            return "#5AC8FA"
        if (kind === "infrastructure")
            return "#FF9500"
        if (kind === "support_component")
            return "#5856D6"
        return "#007AFF"
    }

    function edgeColor(kind) {
        if (kind === "activates")
            return "#007AFF"
        if (kind === "uses_infrastructure" || kind === "uses_support")
            return "#FF9500"
        return "#A2A2AA"
    }

    function groupFill(index) {
        var fills = ["#FFFFFF", "#F5F5F7", "#F2F2F7"]
        return fills[index % fills.length]
    }

    function levelFill(index, selected) {
        if (selected)
            return index === 1 ? "#EAF4FF" : "#F2F2F7"
        return "#FFFFFF"
    }

    function levelBorder(index, selected) {
        if (selected)
            return index === 1 ? "#5AC8FA" : "#C7C7CC"
        return "#D1D1D6"
    }

    function levelInk(selected) {
        return selected ? "#006EDB" : "#6E6E73"
    }
}
