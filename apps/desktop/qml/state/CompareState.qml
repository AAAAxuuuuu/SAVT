import QtQuick

QtObject {
    property string compareMode: "summary"
    property string baselineLabel: "项目快照"
    property string currentLabel: "当前焦点"
    property bool comparisonReady: false
    property string statusText: "对照页优先承接当前项目和当前焦点对象，后续再继续接入分析快照差异。"

    function reset() {
        comparisonReady = false
        compareMode = "summary"
        statusText = "对照页优先承接当前项目和当前焦点对象，后续再继续接入分析快照差异。"
    }
}
