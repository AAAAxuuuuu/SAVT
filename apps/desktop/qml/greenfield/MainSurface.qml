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

    Canvas {
        id: ambientBackground
        anchors.fill: parent

        function rgbaWithAlpha(color, alpha) {
            return Qt.rgba(color.r, color.g, color.b, alpha)
        }

        onPaint: {
            var ctx = getContext("2d")
            var gradient = ctx.createLinearGradient(0, 0, 0, height)
            var beam = ctx.createLinearGradient(width * 0.08, height * 0.06,
                                                width * 0.92, height * 0.78)
            var horizon = ctx.createLinearGradient(0, height * 0.18,
                                                   width, height * 0.86)

            ctx.reset()
            gradient.addColorStop(0.0, tokens.base0)
            gradient.addColorStop(0.58, tokens.base1)
            gradient.addColorStop(1.0, tokens.base2)
            ctx.fillStyle = gradient
            ctx.fillRect(0, 0, width, height)

            beam.addColorStop(0.0, "transparent")
            beam.addColorStop(0.34, rgbaWithAlpha(tokens.signalCobalt, 0.030))
            beam.addColorStop(0.62, rgbaWithAlpha(tokens.signalMoss, 0.022))
            beam.addColorStop(1.0, "transparent")
            ctx.fillStyle = beam
            ctx.fillRect(0, 0, width, height)

            horizon.addColorStop(0.0, "transparent")
            horizon.addColorStop(0.48, rgbaWithAlpha(tokens.signalCobalt, 0.018))
            horizon.addColorStop(1.0, "transparent")
            ctx.fillStyle = horizon
            ctx.fillRect(0, 0, width, height)
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
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
        onAccepted: {
            root.analysisController.selectProjectUrl(selectedFolder)
            caseState.navigate("overview")
        }
    }

    Component.onCompleted: root.analysisController.refreshAiAvailability()
}
