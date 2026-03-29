import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property QtObject theme

    property bool repaintQueued: false

    function scheduleEdgeRepaint() {
        if (repaintQueued || analysisController.analyzing)
            return
        repaintQueued = true
        Qt.callLater(function() {
            repaintQueued = false
            if (analysisController.analyzing)
                return
            if (!root.visible || !edgeCanvas.available)
                return
            edgeCanvas.requestPaint()
        })
    }

    Component {
        id: capabilityNodeComponent

        Rectangle {
            property var nodeData: parent.nodeData
            property bool selected: window.selectedCapabilityNode && window.selectedCapabilityNode.id === nodeData.id

            implicitWidth: 248
            implicitHeight: 132
            radius: 22
            clip: true
            opacity: window.capabilityNodeOpacity(nodeData)
            color: root.theme.nodeFill(nodeData.kind, selected)
            border.color: root.theme.nodeBorder(nodeData.kind, selected, nodeData.pinned)
            border.width: selected ? 2 : 1

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 8
                color: root.theme.nodeAccent(nodeData.kind, selected)
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                anchors.topMargin: 18
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    TagChip {
                        text: window.displayNodeKind(nodeData.kind)
                        compact: true
                        fillColor: selected ? "#2e618f" : "#ffffff"
                        borderColor: selected ? "#7fa6c8" : "#d6e1eb"
                        textColor: selected ? "#f7fbff" : root.theme.inkNormal
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        visible: nodeData.pinned
                        width: 10
                        height: 10
                        radius: 5
                        color: selected ? "#ffe3a8" : "#d09a35"
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: nodeData.name
                    wrapMode: Text.WordWrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                    color: selected ? "#f7fbff" : root.theme.inkStrong
                    font.family: root.theme.displayFontFamily
                    font.pixelSize: Math.max(14, 18 - Math.max(0, Math.ceil((nodeData.name.length - 16) / 5)))
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    visible: (nodeData.responsibility || "").length > 0
                    text: nodeData.responsibility
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    color: selected ? "#d8e6f2" : root.theme.inkMuted
                    font.family: root.theme.textFontFamily
                    font.pixelSize: 12
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TagChip {
                        text: "文件 " + (nodeData.fileCount || 0)
                        compact: true
                        fillColor: selected ? "#264d72" : "#ffffff"
                        borderColor: selected ? "#7fa6c8" : "#d6e1eb"
                        textColor: selected ? "#f7fbff" : root.theme.inkNormal
                    }

                    TagChip {
                        visible: (nodeData.role || "").length > 0
                        text: nodeData.role
                        compact: true
                        fillColor: selected ? "#264d72" : "#ffffff"
                        borderColor: selected ? "#7fa6c8" : "#d6e1eb"
                        textColor: selected ? "#f7fbff" : root.theme.inkNormal
                    }
                }
            }

            ToolTip.visible: nodeHover.containsMouse
            ToolTip.text: window.nodeTooltipSummary(nodeData)

            MouseArea {
                id: nodeHover
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: window.selectCapabilityNode(nodeData)
                onDoubleClicked: window.drillIntoCapabilityNode(nodeData)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        SurfaceCard {
            Layout.fillWidth: true
            minimumContentHeight: 108
            fillColor: root.theme.surfaceSecondary
            borderColor: root.theme.borderSubtle

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: "L2 容器视图"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "入口、核心能力和后台支撑之间的关系全部来自统一 scene。"
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }

                    RowLayout {
                        spacing: 8

                        AppButton {
                            theme: root.theme
                            compact: true
                            checkable: true
                            checked: window.relationshipViewMode === "focused"
                            text: "聚焦关系"
                            onClicked: window.relationshipViewMode = "focused"
                        }

                        AppButton {
                            theme: root.theme
                            compact: true
                            checkable: true
                            checked: window.relationshipViewMode === "all"
                            text: "全部关系"
                            onClicked: window.relationshipViewMode = "all"
                        }
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    TagChip {
                        text: "节点 " + ((analysisController.capabilityScene && analysisController.capabilityScene.nodes) ? analysisController.capabilityScene.nodes.length : 0)
                        compact: true
                        fillColor: "#ffffff"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }

                    TagChip {
                        text: "边 " + ((analysisController.capabilityScene && analysisController.capabilityScene.edges) ? analysisController.capabilityScene.edges.length : 0)
                        compact: true
                        fillColor: "#ffffff"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }

                    TagChip {
                        text: "分组 " + ((analysisController.capabilityScene && analysisController.capabilityScene.groups) ? analysisController.capabilityScene.groups.length : 0)
                        compact: true
                        fillColor: "#ffffff"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }
                }
            }
        }

        SurfaceCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            minimumContentHeight: 420
            fillColor: root.theme.surfacePrimary
            borderColor: root.theme.borderStrong
            stacked: true

            Item {
                anchors.fill: parent

                Flickable {
                    id: graphFlick
                    anchors.fill: parent
                    clip: true
                    contentWidth: Math.max(width, window.currentCapabilitySceneWidth())
                    contentHeight: Math.max(height, window.currentCapabilitySceneHeight())
                    boundsBehavior: Flickable.StopAtBounds

                    property real viewLeft: contentX - 300
                    property real viewRight: contentX + width + 300
                    property real viewTop: contentY - 300
                    property real viewBottom: contentY + height + 300

                    Rectangle {
                        width: window.currentCapabilitySceneWidth()
                        height: window.currentCapabilitySceneHeight()
                        color: "#f8fbfd"

                        Repeater {
                            model: (analysisController.capabilityScene && analysisController.capabilityScene.groups)
                                   ? analysisController.capabilityScene.groups
                                   : []

                            delegate: Rectangle {
                                property var groupBounds: modelData.layoutBounds || {"x": 0, "y": 0, "width": 0, "height": 0}
                                x: groupBounds.x
                                y: groupBounds.y
                                width: groupBounds.width
                                height: groupBounds.height
                                radius: 28
                                visible: modelData.hasBounds
                                color: root.theme.groupFill(index)
                                opacity: 0.82
                                border.color: root.theme.borderSubtle
                                border.width: 1

                                Label {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.margins: 16
                                    text: modelData.name
                                    color: root.theme.inkNormal
                                    font.family: root.theme.displayFontFamily
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                }
                            }
                        }

                        Canvas {
                            id: gridCanvas
                            anchors.fill: parent
                            opacity: 0.28

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.strokeStyle = root.theme.graphGrid
                                ctx.lineWidth = 1

                                for (var x = 0; x < width; x += 48) {
                                    ctx.beginPath()
                                    ctx.moveTo(x, 0)
                                    ctx.lineTo(x, height)
                                    ctx.stroke()
                                }

                                for (var y = 0; y < height; y += 48) {
                                    ctx.beginPath()
                                    ctx.moveTo(0, y)
                                    ctx.lineTo(width, y)
                                    ctx.stroke()
                                }
                            }
                        }

                        Canvas {
                            id: edgeCanvas
                            anchors.fill: parent

                            Connections {
                                target: window
                                function onSelectedCapabilityNodeChanged() { root.scheduleEdgeRepaint() }
                                function onRelationshipViewModeChanged() { root.scheduleEdgeRepaint() }
                            }

                            Connections {
                                target: analysisController
                                function onCapabilitySceneChanged() { root.scheduleEdgeRepaint() }
                                function onAnalyzingChanged() { root.scheduleEdgeRepaint() }
                            }

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.lineJoin = "round"
                                ctx.lineCap = "round"
                                var visibleEdges = window.visibleCapabilityEdges()
                                for (var i = 0; i < visibleEdges.length; ++i)
                                    window.drawCapabilityEdge(ctx, visibleEdges[i], true)
                            }

                            onAvailableChanged: root.scheduleEdgeRepaint()
                            Component.onCompleted: root.scheduleEdgeRepaint()
                            onVisibleChanged: root.scheduleEdgeRepaint()
                        }

                        Repeater {
                            model: (analysisController.capabilityScene && analysisController.capabilityScene.nodes)
                                   ? analysisController.capabilityScene.nodes
                                   : []

                            delegate: Item {
                                property var layoutRect: window.nodeDisplayRect(modelData)
                                x: layoutRect.x
                                y: layoutRect.y
                                width: layoutRect.width
                                height: layoutRect.height
                                visible: window.nodeShouldBeDisplayed(modelData.id)

                                property bool inViewport: (x + width > graphFlick.viewLeft)
                                                          && (x < graphFlick.viewRight)
                                                          && (y + height > graphFlick.viewTop)
                                                          && (y < graphFlick.viewBottom)

                                property var nodeData: modelData

                                Loader {
                                    anchors.fill: parent
                                    sourceComponent: capabilityNodeComponent
                                    active: parent.inViewport && parent.visible
                                    asynchronous: true
                                    property var nodeData: parent.nodeData
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: ((analysisController.capabilityScene && analysisController.capabilityScene.nodes)
                                      ? analysisController.capabilityScene.nodes.length
                                      : 0) === 0
                            text: "当前还没有可展示的 L2 节点。先完成一次分析。"
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 14
                        }
                    }
                }

                Label {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 18
                    anchors.bottomMargin: 16
                    text: window.selectedCapabilityNode
                          ? "当前高亮：与「" + window.selectedCapabilityNode.name + "」直接关联的关系"
                          : "点击任意模块查看其关联关系"
                    color: root.theme.inkMuted
                    font.family: root.theme.textFontFamily
                    font.pixelSize: 11
                }
            }
        }
    }
}
