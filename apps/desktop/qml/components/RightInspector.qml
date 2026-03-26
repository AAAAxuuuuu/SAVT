import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: root
    background: Rectangle {
        radius: 10
        color: "#ffffff"
        border.color: "#e2e8f0"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: window.inspectorCollapsed ? 8 : 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                visible: !window.inspectorCollapsed
                text: "\u8bfb\u56fe\u8bf4\u660e\u4e0e\u8f85\u52a9"
                color: "#0f172a"
                font.pixelSize: 20
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Button {
                text: window.inspectorCollapsed ? "\u5c55\u5f00" : "\u6536\u8d77"
                onClicked: window.inspectorCollapsed = !window.inspectorCollapsed
            }
        }

        ScrollView {
            visible: !window.inspectorCollapsed
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 12

                Frame {
                    Layout.fillWidth: true
                    background: Rectangle { radius: 10; color: "#f8fafc"; border.color: "#e2e8f0" }

                    ColumnLayout {
                        width: parent.width
                        anchors { top: parent.top; left: parent.left; right: parent.right; margins: 12 }
                        spacing: 8

                        Label {
                            text: "\u5f53\u524d\u9009\u4e2d\u7684\u90e8\u5206"
                            color: "#0f172a"
                            font.pixelSize: 17
                            font.bold: true
                        }
                        Label {
                            text: window.selectedNodeDisplayName()
                            color: "#2563eb"
                            font.pixelSize: 20
                            font.bold: true
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        Label {
                            text: "\u5b83\u4e3b\u8981\u8d1f\u8d23\uff1a" + window.selectedNodeResponsibility()
                            color: "#475569"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    background: Rectangle { radius: 10; color: "#f8fafc"; border.color: "#e2e8f0" }

                    ColumnLayout {
                        width: parent.width
                        anchors { top: parent.top; left: parent.left; right: parent.right; margins: 12 }
                        spacing: 10

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: "AI \u8f85\u52a9\u89e3\u8bfb"
                                color: "#0f172a"
                                font.pixelSize: 17
                                font.bold: true
                            }
                            Button {
                                text: analysisController.aiBusy ? "\u89e3\u8bfb\u4e2d..." : "\u751f\u6210\u8de8\u8bed\u8a00 AI \u89e3\u8bfb"
                                enabled: window.selectedCapabilityNode !== null && !analysisController.aiBusy
                                onClicked: analysisController.requestAiExplanation(window.selectedCapabilityNode)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: analysisController.aiBusy
                            spacing: 12

                            BusyIndicator {
                                running: analysisController.aiBusy
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                            }
                            Label {
                                text: "\u2728 AI \u6b63\u5728\u9605\u8bfb\u8de8\u8bed\u8a00\u6e90\u7801\u4e0a\u4e0b\u6587\uff0c\u52aa\u529b\u601d\u8003\u4e2d..."
                                color: "#2563eb"
                                font.pixelSize: 13
                                font.italic: true
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true

                                SequentialAnimation on opacity {
                                    loops: Animation.Infinite
                                    running: analysisController.aiBusy
                                    NumberAnimation { to: 0.4; duration: 800; easing.type: Easing.InOutQuad }
                                    NumberAnimation { to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            visible: analysisController.aiHasResult && !analysisController.aiBusy

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                visible: analysisController.aiSummary.length > 0

                                Label {
                                    text: "\uD83D\uDCCC \u6982\u8ff0"
                                    color: "#374151"
                                    font.pixelSize: 13
                                    font.bold: true
                                }
                                Label {
                                    text: analysisController.aiSummary
                                    color: "#0f172a"
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                    font.pixelSize: 13
                                    lineHeight: 1.5
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true; height: 1; color: "#e2e8f0"
                                visible: analysisController.aiSummary.length > 0
                                         && analysisController.aiResponsibility.length > 0
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                visible: analysisController.aiResponsibility.length > 0

                                Label {
                                    text: "\u2699\uFE0F \u8be6\u7ec6\u804c\u8d23"
                                    color: "#374151"
                                    font.pixelSize: 13
                                    font.bold: true
                                }
                                Label {
                                    text: analysisController.aiResponsibility
                                    color: "#1e293b"
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                    font.pixelSize: 13
                                    lineHeight: 1.5
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true; height: 1; color: "#e2e8f0"
                                visible: analysisController.aiResponsibility.length > 0
                                         && analysisController.aiCollaborators.length > 0
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                visible: analysisController.aiCollaborators.length > 0

                                Label {
                                    text: "\uD83E\uDD1D \u534f\u4f5c\u8005"
                                    color: "#374151"
                                    font.pixelSize: 13
                                    font.bold: true
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Repeater {
                                        model: analysisController.aiCollaborators

                                        Rectangle {
                                            radius: 4
                                            color: "#eff6ff"
                                            border.color: "#bfdbfe"
                                            width: chipLabel.implicitWidth + 16
                                            height: chipLabel.implicitHeight + 8

                                            Label {
                                                id: chipLabel
                                                anchors.centerIn: parent
                                                text: modelData
                                                color: "#1d4ed8"
                                                font.pixelSize: 12
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true; height: 1; color: "#e2e8f0"
                                visible: analysisController.aiCollaborators.length > 0
                                         && analysisController.aiUncertainty.length > 0
                            }

                            Frame {
                                Layout.fillWidth: true
                                visible: analysisController.aiUncertainty.length > 0
                                padding: 8
                                background: Rectangle {
                                    radius: 6
                                    color: "#fffbeb"
                                    border.color: "#fcd34d"
                                }

                                ColumnLayout {
                                    width: parent.width
                                    anchors { top: parent.top; left: parent.left; right: parent.right; margins: 2 }
                                    spacing: 4

                                    Label {
                                        text: "\u26A0\uFE0F \u7f6e\u4fe1\u5ea6\u8bf4\u660e"
                                        color: "#92400e"
                                        font.pixelSize: 12
                                        font.bold: true
                                    }
                                    Label {
                                        text: analysisController.aiUncertainty
                                        color: "#78350f"
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                        font.pixelSize: 12
                                        lineHeight: 1.4
                                    }
                                }
                            }
                        }
                    }
                }

                Frame {
                    Layout.fillWidth: true
                    background: Rectangle {
                        radius: 10
                        color: "#eff6ff"
                        border.color: "#60a5fa"
                        border.width: 2
                    }

                    ColumnLayout {
                        width: parent.width
                        anchors { top: parent.top; left: parent.left; right: parent.right; margins: 14 }
                        spacing: 10

                        Label {
                            text: "\u2728 \u51c6\u5907\u8ba9 AI \u4fee\u6539\u6b64\u6a21\u5757\uff1f"
                            color: "#1e3a8a"
                            font.pixelSize: 16
                            font.bold: true
                        }
                        Label {
                            text: "\u70b9\u51fb\u4e0b\u65b9\u6309\u9454\uff0c\u76f4\u63a5\u590d\u5236\u8be5\u6a21\u5757\u6240\u6709\u76f8\u5173\u6587\u4ef6\u8def\u5f84\u548c\u6838\u5fc3\u51fd\u6570\uff0c\u7c98\u8d34\u7ed9 Cursor \u5373\u53ef\u3002"
                            color: "#3b82f6"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        Button {
                            Layout.fillWidth: true
                            text: "\uD83D\uDCCB \u4e00\u952e\u63d0\u53d6\u6838\u5fc3\u6e90\u7801\u4e0a\u4e0b\u6587"
                            font.bold: true
                            highlighted: true
                            enabled: window.selectedCapabilityNode !== null
                            onClicked: {
                                if (window.selectedCapabilityNode) {
                                    analysisController.copyCodeContextToClipboard(window.selectedCapabilityNode.id)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
