import QtQuick

QtObject {
    id: root

    required property QtObject analysisController

    property string route: "overview"

    readonly property bool hasProject: (analysisController.projectRootPath || "").length > 0
    readonly property bool hasSystemBrief: (analysisController.systemContextReport || "").length > 0
                                           || ((analysisController.systemContextData || ({})).headline || "").length > 0
    readonly property bool hasAtlas: (((analysisController.capabilityScene || ({})).nodes || []).length > 0)
    readonly property var semanticReadiness: (analysisController.systemContextData || ({})).semanticReadiness || ({})

    function navigate(nextRoute) {
        route = nextRoute
    }

    function projectName() {
        var dataName = (analysisController.systemContextData || ({})).projectName || ""
        if (dataName.length > 0)
            return dataName

        var path = analysisController.projectRootPath || ""
        if (path.length === 0)
            return "未选择项目"

        var parts = path.split(/[\\\\/]/)
        return parts.length > 0 ? parts[parts.length - 1] : path
    }

    function trustLabel() {
        if (analysisController.analyzing)
            return analysisController.analysisPhase || "分析中"
        return semanticReadiness.modeLabel || (hasSystemBrief ? "分析已就绪" : "未分析")
    }

    function routeTitle() {
        if (route === "component")
            return "边界钻取"
        if (route === "report")
            return "算法报告"
        if (route === "config")
            return "环境与精度"
        return "重建全景"
    }

    function trustTone() {
        var tone = semanticReadiness.badgeTone || ""
        if (tone === "success")
            return "success"
        if (tone === "warning")
            return "warning"
        if (tone === "danger")
            return "danger"
        return hasSystemBrief ? "success" : "info"
    }
}
