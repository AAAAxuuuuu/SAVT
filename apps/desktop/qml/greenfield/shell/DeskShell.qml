import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property QtObject tokens
    required property QtObject analysisController
    required property QtObject caseState
    required property QtObject focusState

    signal chooseProjectRequested()

    Shortcut { sequence: "Ctrl+1"; onActivated: root.caseState.navigate("overview") }
    Shortcut { sequence: "Ctrl+2"; onActivated: root.caseState.navigate("component") }
    Shortcut { sequence: "Ctrl+3"; onActivated: root.caseState.navigate("report") }
    Shortcut { sequence: "Ctrl+4"; onActivated: root.caseState.navigate("config") }
    Shortcut { sequence: "Escape"; onActivated: root.focusState.clear() }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        CaseRail {
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            tokens: root.tokens
            caseState: root.caseState
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: root.tokens.border1
        }

        Item {
            id: mainContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Item {
                id: contentHost
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.width - (root.focusState.inspectorOpen ? inspector.width : 0)

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

                CommandStrip {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    tokens: root.tokens
                    analysisController: root.analysisController
                    caseState: root.caseState
                    focusState: root.focusState
                    onChooseProjectRequested: root.chooseProjectRequested()
                }

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
                width: 320
                height: parent.height
                x: root.focusState.inspectorOpen ? parent.width - width : parent.width + 20
                y: 0
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
}
