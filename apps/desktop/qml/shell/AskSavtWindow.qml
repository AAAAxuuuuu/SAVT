import QtQuick
import QtQuick.Window
import "../workspace"

Window {
    id: root

    required property QtObject theme
    required property QtObject uiState
    required property QtObject analysisController
    required property Window ownerWindow

    property bool defaultPositioned: false

    width: Math.min(620, Math.max(460, (root.ownerWindow ? root.ownerWindow.width : 1560) * 0.34))
    height: Math.min(760, Math.max(540, (root.ownerWindow ? root.ownerWindow.height : 940) * 0.72))
    minimumWidth: 460
    minimumHeight: 540
    visible: root.uiState.askPanelOpen
    title: "Ask SAVT"
    transientParent: root.ownerWindow
    modality: Qt.NonModal
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint | Qt.WindowMinimizeButtonHint
    color: root.theme.windowBase

    function positionNearOwner() {
        if (!root.ownerWindow)
            return

        root.x = root.ownerWindow.x + root.ownerWindow.width - root.width - 56
        root.y = root.ownerWindow.y + 84
        root.defaultPositioned = true
    }

    onVisibleChanged: {
        if (!visible)
            return
        if (!root.defaultPositioned)
            Qt.callLater(root.positionNearOwner)
        requestActivate()
    }

    onClosing: function(close) {
        close.accepted = false
        root.uiState.askPanelOpen = false
    }

    AskSavtPanel {
        anchors.fill: parent
        allowInternalDrag: false
        theme: root.theme
        uiState: root.uiState
        analysisController: root.analysisController
        onCloseRequested: root.uiState.askPanelOpen = false
    }
}
