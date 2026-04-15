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

        function paintGlow(ctx, cx, cy, radius, color, coreAlpha, edgeAlpha) {
            var gradient = ctx.createRadialGradient(cx, cy, 0, cx, cy, radius)
            gradient.addColorStop(0.0, rgbaWithAlpha(color, coreAlpha))
            gradient.addColorStop(0.42, rgbaWithAlpha(color, coreAlpha * 0.48))
            gradient.addColorStop(1.0, rgbaWithAlpha(color, edgeAlpha))
            ctx.fillStyle = gradient
            ctx.beginPath()
            ctx.arc(cx, cy, radius, 0, Math.PI * 2)
            ctx.fill()
        }

        onPaint: {
            var ctx = getContext("2d")
            var gradient = ctx.createLinearGradient(0, 0, 0, height)

            ctx.reset()
            gradient.addColorStop(0.0, tokens.base0)
            gradient.addColorStop(0.52, tokens.base1)
            gradient.addColorStop(1.0, tokens.base2)
            ctx.fillStyle = gradient
            ctx.fillRect(0, 0, width, height)

            paintGlow(ctx, width * 0.12, height * 0.16, Math.max(width, height) * 0.22,
                      tokens.ambientCyan, 0.26, 0.0)
            paintGlow(ctx, width * 0.84, height * 0.12, Math.max(width, height) * 0.2,
                      tokens.ambientViolet, 0.22, 0.0)
            paintGlow(ctx, width * 0.54, height * 0.72, Math.max(width, height) * 0.26,
                      tokens.ambientBlue, 0.16, 0.0)
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
        onAccepted: root.analysisController.analyzeProjectUrl(selectedFolder)
    }

    Component.onCompleted: root.analysisController.refreshAiAvailability()
}
