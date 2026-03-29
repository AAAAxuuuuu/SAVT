import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root

    required property QtObject theme

    implicitHeight: layoutShell.implicitHeight

    FolderDialog {
        id: projectFolderDialog
        title: "选择项目根目录"
        onAccepted: analysisController.analyzeProjectUrl(selectedFolder)
    }

    GridLayout {
        id: layoutShell
        anchors.fill: parent
        columns: width > 1180 ? 2 : 1
        columnSpacing: 16
        rowSpacing: 16

        SurfaceCard {
            Layout.fillWidth: true
            minimumContentHeight: 172
            stacked: true
            fillColor: root.theme.surfacePrimary
            borderColor: root.theme.borderStrong

            ColumnLayout {
                anchors.fill: parent
                spacing: 16

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: "SAVT Architecture Workbench"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 26
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "保留现有分析信息流，把路径、状态和入口操作压进统一的工作台。"
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }

                    ColumnLayout {
                        spacing: 8

                        TagChip {
                            text: analysisController.analyzing ? "分析进行中" : "待命"
                            compact: true
                            fillColor: analysisController.analyzing ? "#dceee8" : "#edf3f8"
                            borderColor: analysisController.analyzing ? "#8eb8ab" : "#cad6e2"
                            textColor: analysisController.analyzing ? "#1f6f61" : "#4d6278"
                        }

                        TagChip {
                            visible: analysisController.analysisPhase.length > 0
                            text: analysisController.analysisPhase
                            compact: true
                            fillColor: "#f4f6f9"
                            borderColor: "#d7e0e8"
                            textColor: "#596c7f"
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: root.theme.borderSubtle
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        text: "项目根目录"
                        color: root.theme.inkNormal
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }

                    TextField {
                        id: projectRootField
                        Layout.fillWidth: true
                        text: analysisController.projectRootPath
                        selectByMouse: true
                        enabled: !analysisController.analyzing
                        placeholderText: "输入或粘贴项目根目录"
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 14
                        color: root.theme.inkStrong
                        placeholderTextColor: root.theme.inkMuted

                        background: Rectangle {
                            radius: 18
                            color: root.theme.surfaceSecondary
                            border.color: projectRootField.activeFocus ? root.theme.borderStrong : root.theme.borderSubtle
                            border.width: 1
                        }

                        onAccepted: {
                            analysisController.projectRootPath = text
                            analysisController.analyzeCurrentProject()
                        }
                        onEditingFinished: analysisController.projectRootPath = text
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    AppButton {
                        theme: root.theme
                        compact: true
                        text: "浏览"
                        tone: "ghost"
                        enabled: !analysisController.analyzing
                        onClicked: projectFolderDialog.open()
                    }

                    AppButton {
                        theme: root.theme
                        compact: true
                        text: analysisController.analyzing ? "分析中..." : "开始分析"
                        tone: "accent"
                        enabled: !analysisController.analyzing
                        onClicked: {
                            analysisController.projectRootPath = projectRootField.text
                            analysisController.analyzeCurrentProject()
                        }
                    }

                    AppButton {
                        theme: root.theme
                        compact: true
                        text: analysisController.stopRequested ? "停止中..." : "停止"
                        tone: "danger"
                        enabled: analysisController.analyzing && !analysisController.stopRequested
                        onClicked: analysisController.stopAnalysis()
                    }

                    Item { Layout.fillWidth: true }

                    TagChip {
                        text: (analysisController.projectRootPath || "").length > 0 ? "已绑定项目" : "未选择项目"
                        compact: true
                        fillColor: "#f7fafc"
                        borderColor: "#d7e0e8"
                        textColor: "#5d7185"
                    }
                }
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            Layout.preferredWidth: 420
            Layout.alignment: Qt.AlignTop
            minimumContentHeight: 140
            fillColor: root.theme.surfaceSecondary
            borderColor: root.theme.borderSubtle

            ColumnLayout {
                anchors.fill: parent
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: "运行状态"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: analysisController.statusMessage
                            color: root.theme.inkMuted
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }

                    TagChip {
                        text: Math.round(analysisController.analysisProgress * 100) + "%"
                        compact: true
                        fillColor: "#ffffff"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: root.theme.borderSubtle
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 1
                        value: analysisController.analysisProgress

                        background: Rectangle {
                            implicitHeight: 12
                            radius: 6
                            color: "#e0e8ef"
                        }

                        contentItem: Item {
                            implicitHeight: 12

                            Rectangle {
                                width: parent.width * (analysisController.analysisProgress || 0)
                                height: parent.height
                                radius: 6
                                color: root.theme.accentStrong
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TagChip {
                            text: analysisController.analysisPhase.length > 0 ? analysisController.analysisPhase : "等待开始"
                            compact: true
                            fillColor: "#ffffff"
                            borderColor: root.theme.borderSubtle
                            textColor: root.theme.inkNormal
                        }

                        TagChip {
                            text: analysisController.analyzing ? "异步运行" : "空闲"
                            compact: true
                            fillColor: analysisController.analyzing ? "#e5eef6" : "#ffffff"
                            borderColor: root.theme.borderSubtle
                            textColor: analysisController.analyzing ? root.theme.accentStrong : root.theme.inkNormal
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }
        }
    }
}
