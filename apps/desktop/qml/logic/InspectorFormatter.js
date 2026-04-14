.pragma library

function dedupe(values) {
    var result = []
    var seen = {}
    var list = values || []
    for (var i = 0; i < list.length; ++i) {
        var value = list[i]
        var key = String(value)
        if (!seen[key]) {
            seen[key] = true
            result.push(value)
        }
    }
    return result
}

function isSingleFileNode(node) {
    return !!node && (node.fileCount || 0) === 1 && ((node.exampleFiles || []).length === 1)
}

function primaryFilePath(node) {
    if (!isSingleFileNode(node))
        return ""
    return (node.exampleFiles || [])[0] || ""
}

function displayNodeKind(kind, node) {
    if (isSingleFileNode(node))
        return "具体文件"
    if (kind === "entry")
        return "入口能力"
    if (kind === "entry_component")
        return "入口组件"
    if (kind === "infrastructure")
        return "基础设施"
    if (kind === "support_component")
        return "支撑组件"
    if (kind === "service")
        return "服务模块"
    return "核心模块"
}

function inspectorTitle(node, edge) {
    if (edge)
        return (edge.fromName || "上游") + " → " + (edge.toName || "下游")
    if (node)
        return node.name || "未命名节点"
    return "尚未选择图元素"
}

function inspectorSubtitle(node, edge) {
    if (edge)
        return edge.summary || "这条关系表示两个模块之间的主依赖路径。"
    if (node && isSingleFileNode(node))
        return primaryFilePath(node) || node.name || "当前选中了一个具体文件。"
    if (node)
        return node.responsibility || node.summary || "先在图中选中一个能力域或组件。"
    return "先在图中选中一个能力域、组件或关系。"
}

function importanceSummary(node, edge) {
    if (edge)
        return "这条关系权重为 " + (edge.weight || 0) + "，说明它在当前视图中属于需要优先核对的协作链路。"
    if (!node)
        return "当前还没有选中对象。"
    if (isSingleFileNode(node)) {
        if ((node.topSymbols || []).length > 0)
            return "这是一个具体文件，已经抽取到 " + node.topSymbols.length + " 个声明/符号线索，适合直接下钻。"
        if ((node.collaboratorNames || []).length > 0)
            return "这是一个具体文件，它和 " + node.collaboratorNames.length + " 个邻接组件存在关系，适合边看代码边核对边界。"
        return "这是一个具体文件，继续停留在关系图里的价值已经不高，更适合切到文件解读。"
    }
    if ((node.riskSignals || []).length > 0)
        return "该模块携带 " + node.riskSignals.length + " 个风险信号，建议优先核对其职责边界与依赖路径。"
    if ((node.collaboratorNames || []).length > 0)
        return "它与 " + node.collaboratorNames.length + " 个相邻模块存在协作，是理解主流程的关键枢纽。"
    if ((node.topSymbols || []).length > 0)
        return "它暴露了清晰的核心符号，可以作为继续下钻源码的稳定切入点。"
    return "这个模块位于当前分析链路中，适合作为理解局部结构与继续改造的切入口。"
}

function keyFiles(node, evidence) {
    var files = []
    if (node && node.exampleFiles)
        files = files.concat(node.exampleFiles)
    if (evidence && evidence.sourceFiles)
        files = files.concat(evidence.sourceFiles)
    return dedupe(files).slice(0, 6)
}

function keySymbols(node, evidence) {
    var items = []
    if (node && node.topSymbols)
        items = items.concat(node.topSymbols)
    if (evidence && evidence.symbols)
        items = items.concat(evidence.symbols)
    return dedupe(items).slice(0, 8)
}

function confidenceLabelText(label) {
    if (label === "high")
        return "高置信"
    if (label === "medium")
        return "中置信"
    if (label === "low")
        return "低置信"
    return "证据待补"
}

function nextStepSummary(node, edge, componentViewActive) {
    if (edge)
        return "继续查看关系两端模块的证据，并确认它们是否属于同一条责任链。"
    if (!node)
        return "先从项目总览或能力地图进入，再选择一个对象继续分析。"
    if (isSingleFileNode(node))
        return "先看文件解读里的导入、声明和关键片段，再回到组件关系确认这个文件只负责哪一段。"
    if (componentViewActive)
        return "继续核对相关文件、协作关系和 AI 建议，把理解直接转成具体改造动作。"
    return "先查看该能力域的组件工作台，再把关键文件上下文复制给外部编码代理。"
}
