import QtQuick

QtObject {
    property string pageId: "project.overview"

    function navigateTo(targetPageId) {
        if ((targetPageId || "").length > 0)
            pageId = targetPageId
    }

    function openLevel(levelId) {
        navigateTo("levels." + levelId)
    }

    function isLevelPage(targetPageId) {
        return (targetPageId || "").indexOf("levels.") === 0
    }
}
