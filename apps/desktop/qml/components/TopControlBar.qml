import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Frame {
    id: root
    background: Rectangle {
        radius: 10
        color: "#ffffff"
        border.color: "#e2e8f0"
    }

    FolderDialog {
        id: projectFolderDialog
        title: "\u9009\u62e9\u9879\u76ee\u6839\u76ee\u5f55"
        onAccepted: analysisController.analyzeProjectUrl(selectedFolder)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            text: "\u8981\u5206\u6790\u7684\u9879\u76ee"
            color: "#2563eb"
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: projectRootField
                Layout.fillWidth: true
                text: analysisController.projectRootPath
                selectByMouse: true
                enabled: !analysisController.analyzing
                placeholderText: "\u8f93\u5165\u6216\u7c98\u8d34\u9879\u76ee\u6839\u76ee\u5f55"
                color: "#0f172a"
                onAccepted: {
                    analysisController.projectRootPath = text
                    analysisController.analyzeCurrentProject()
                }
                onEditingFinished: analysisController.projectRootPath = text
            }

            Button {
                text: "\u6d4f\u89c8"
                enabled: !analysisController.analyzing
                onClicked: projectFolderDialog.open()
            }

            Button {
                text: analysisController.analyzing ? "\u5206\u6790\u4e2d..." : "\u5f00\u59cb\u5206\u6790"
                highlighted: true
                enabled: !analysisController.analyzing
                onClicked: {
                    analysisController.projectRootPath = projectRootField.text
                    analysisController.analyzeCurrentProject()
                }
            }

            Button {
                text: analysisController.stopRequested ? "\u505c\u6b62\u4e2d..." : "\u505c\u6b62"
                enabled: analysisController.analyzing && !analysisController.stopRequested
                onClicked: analysisController.stopAnalysis()
            }
        }

        Label {
            text: analysisController.statusMessage
            color: "#475569"
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: analysisController.analyzing
            spacing: 6

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 1
                value: analysisController.analysisProgress
            }

            Label {
                text: analysisController.analysisPhase + " " + Math.round(analysisController.analysisProgress * 100) + "%"
                color: "#2563eb"
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
    }
}
