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
        anchors.margins: 18
        spacing: 18

        CaseRail {
            Layout.preferredWidth: 264
            Layout.fillHeight: true
            tokens: root.tokens
            caseState: root.caseState
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
                GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.72) }
                GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.46) }
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
                    GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.32) }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }

            Item {
                id: contentHost
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.width - (root.focusState.inspectorOpen ? inspector.width + 12 : 0)

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
                width: 336
                height: parent.height - 16
                x: root.focusState.inspectorOpen ? parent.width - width - 8 : parent.width + 28
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
}
