import QtQuick
import QtQuick.Dialogs
import "foundation"
import "shell"
import "state"

Item {
    id: root

    required property QtObject analysisController

    Tokens {
        id: tokens
    }

    CaseState {
        id: caseState
        analysisController: root.analysisController
    }

    FocusState {
        id: focusState
    }

    Rectangle {
        anchors.fill: parent
        color: tokens.base1
    }

    DeskShell {
        anchors.fill: parent
        tokens: tokens
        analysisController: root.analysisController
        caseState: caseState
        focusState: focusState
        onChooseProjectRequested: folderDialog.open()
    }

    FolderDialog {
        id: folderDialog
        title: "选择项目文件夹"
        onAccepted: root.analysisController.analyzeProjectUrl(selectedFolder)
    }

    Component.onCompleted: root.analysisController.refreshAiAvailability()
}
