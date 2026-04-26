import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "../foundation"

Item {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState
    property real displayedAnalysisProgress: 0.0
    readonly property bool overviewInspectorVisible: root.caseState.route === "overview"
                                                     && root.focusState.inspectorOpen

    signal chooseProjectRequested()

    function clampedProgress(value) {
        return Math.max(0, Math.min(1, Number(value || 0)))
    }

    function closeCurrentWindow() {
        if (root.Window.window)
            root.Window.window.close()
        else
            Qt.quit()
    }

    function handleBackAction() {
        if (root.caseState.route === "component") {
            if (root.focusState.focusedNode && root.focusState.focusedNode.id !== undefined) {
                root.focusState.clearNodeFocus()
                return
            }
            root.caseState.navigate("overview")
            return
        }

        if (root.caseState.route === "report" || root.caseState.route === "config") {
            root.caseState.navigate("overview")
            return
        }

        if (root.caseState.route === "overview"
                && (root.focusState.focusedNode
                    || root.focusState.focusedCapability
                    || root.focusState.focusedEdge
                    || root.focusState.inspectorOpen)) {
            root.focusState.clear()
        }
    }

    Shortcut { sequence: "Ctrl+1"; onActivated: root.caseState.navigate("overview") }
    Shortcut { sequence: "Ctrl+2"; onActivated: root.caseState.navigate("component") }
    Shortcut { sequence: "Ctrl+3"; onActivated: root.caseState.navigate("report") }
    Shortcut { sequence: "Ctrl+4"; onActivated: root.caseState.navigate("config") }
    Shortcut {
        sequences: [StandardKey.Close]
        context: Qt.ApplicationShortcut
        onActivated: root.closeCurrentWindow()
    }
    Shortcut {
        sequences: [StandardKey.Quit]
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }
    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        onActivated: root.handleBackAction()
    }

    Connections {
        target: root.analysisController

        function onAnalyzingChanged() {
            if (root.analysisController.analyzing)
                root.displayedAnalysisProgress = 0
            else
                root.displayedAnalysisProgress = root.clampedProgress(root.analysisController.analysisProgress)
        }

        function onAnalysisProgressChanged() {
            root.displayedAnalysisProgress = root.clampedProgress(root.analysisController.analysisProgress)
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: root.caseState.hasWorkspaceChrome ? 18 : 0

        CaseRail {
            visible: root.caseState.hasWorkspaceChrome
            Layout.preferredWidth: visible ? 264 : 0
            Layout.fillHeight: true
            tokens: root.tokens
            analysisController: root.analysisController
            caseState: root.caseState
            focusState: root.focusState
        }

        Rectangle {
            id: mainContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: root.tokens.radiusXxl + 4
            color: root.tokens.panelSoft
            border.color: root.tokens.border1
            clip: true

            gradient: Gradient {
                GradientStop { position: 0.0; color: root.tokens.panelStrong }
                GradientStop { position: 1.0; color: root.tokens.panelSoft }
            }

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                border.color: root.tokens.shineBorder
                border.width: 1
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 180
                radius: parent.radius
                color: "transparent"
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(root.tokens.signalCobalt.r, root.tokens.signalCobalt.g, root.tokens.signalCobalt.b, 0.045) }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }

            Item {
                id: contentHost
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.width - (root.overviewInspectorVisible ? inspector.width + 12 : 0)

                Behavior on width {
                    NumberAnimation {
                        duration: 260
                        easing.type: Easing.OutCubic
                    }
                }
            }

            ColumnLayout {
                anchors.fill: contentHost
                spacing: 0

                WorkspaceStage {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    tokens: root.tokens
                    analysisController: root.analysisController
                    caseState: root.caseState
                    focusState: root.focusState
                    onChooseProjectRequested: root.chooseProjectRequested()
                }
            }

            FocusBrief {
                id: inspector
                visible: root.caseState.hasWorkspaceChrome && root.caseState.route === "overview"
                width: 336
                height: parent.height - 16
                x: root.overviewInspectorVisible ? parent.width - width - 8 : parent.width + 28
                y: 8
                tokens: root.tokens
                analysisController: root.analysisController
                caseState: root.caseState
                focusState: root.focusState

                Behavior on x {
                    NumberAnimation {
                        duration: 360
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }
    }

    Rectangle {
        id: analysisOverlay
        anchors.fill: parent
        visible: root.analysisController.analyzing
        opacity: visible ? 1.0 : 0.0
        z: 100
        color: Qt.rgba(8 / 255, 15 / 255, 28 / 255, 0.42)

        Behavior on opacity {
            NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            hoverEnabled: true
        }

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width - 64, 460)
            height: 264
            radius: root.tokens.radiusXxl
            color: Qt.rgba(1, 1, 1, 0.94)
            border.color: root.tokens.border1

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                anchors.topMargin: 28
                anchors.bottomMargin: 28
                spacing: 14

                Label {
                    Layout.fillWidth: true
                    text: "正在加载"
                    color: root.tokens.text1
                    font.family: root.tokens.displayFontFamily
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    Layout.fillWidth: true
                    Layout.minimumHeight: 46
                    text: root.analysisController.analysisPhase || "正在准备计算..."
                    color: root.tokens.text2
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }

                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: root.displayedAnalysisProgress

                    Behavior on value {
                        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: Math.round(root.displayedAnalysisProgress * 100) + "%"
                    color: root.tokens.text3
                    font.family: root.tokens.textFontFamily
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                }

                ActionButton {
                    Layout.alignment: Qt.AlignHCenter
                    tokens: root.tokens
                    text: "停止"
                    tone: "secondary"
                    fixedWidth: 104
                    hint: "立即停止当前计算任务。"
                    onClicked: root.analysisController.stopAnalysis()
                }
            }
        }
    }
}
