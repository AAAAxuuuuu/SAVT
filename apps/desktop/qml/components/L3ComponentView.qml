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
        id: componentNodeComponent

        Rectangle {
            property var nodeData: parent.nodeData
            property bool selected: window.selectedComponentNode && window.selectedComponentNode.id === nodeData.id

            implicitWidth: 232
            implicitHeight: 124
            radius: 22
            clip: true
            opacity: window.componentNodeOpacity(nodeData)
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
                    spacing: 8

                    TagChip {
                        text: window.displayNodeKind(nodeData.kind)
                        compact: true
                        fillColor: selected ? "#2e618f" : "#ffffff"
                        borderColor: selected ? "#7fa6c8" : "#d6e1eb"
                        textColor: selected ? "#f7fbff" : root.theme.inkNormal
                    }

                    TagChip {
                        visible: (nodeData.stageName || "").length > 0
                        text: nodeData.stageName || ""
                        compact: true
                        fillColor: selected ? "#264d72" : "#ffffff"
                        borderColor: selected ? "#7fa6c8" : "#d6e1eb"
                        textColor: selected ? "#f7fbff" : root.theme.inkNormal
                    }

                    Item { Layout.fillWidth: true }
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
                onClicked: window.selectComponentNode(nodeData)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        SurfaceCard {
            Layout.fillWidth: true
            minimumContentHeight: 152
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
                            text: window.selectedCapabilityNode
                                  ? ("L3 组件视图 · " + window.selectedComponentSceneTitle())
                                  : "L3 组件视图"
                            color: root.theme.inkStrong
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 20
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: window.selectedCapabilityNode
                                  ? ((window.selectedComponentSceneSummary() || "").length > 0
                                         ? window.selectedComponentSceneSummary()
                                         : "从当前能力域继续下钻，查看内部组件、阶段和协作关系。")
                                  : "先在 L2 里选中一个能力域，再进入这里查看它的内部组件图。"
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            color: root.theme.inkMuted
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
                    }

                    AppButton {
                        theme: root.theme
                        compact: true
                        tone: "ghost"
                        text: "返回 L2"
                        onClicked: window.selectedC4LevelIndex = 1
                    }

                    AppButton {
                        theme: root.theme
                        compact: true
                        checkable: true
                        checked: window.componentRelationshipViewMode === "focused"
                        text: "聚焦关系"
                        onClicked: window.componentRelationshipViewMode = "focused"
                    }

                    AppButton {
                        theme: root.theme
                        compact: true
                        checkable: true
                        checked: window.componentRelationshipViewMode === "all"
                        text: "全部关系"
                        onClicked: window.componentRelationshipViewMode = "all"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    TextField {
                        id: filterField
                        Layout.fillWidth: true
                        placeholderText: "按名称、角色或符号筛选组件"
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 13
                        leftPadding: 14
                        rightPadding: 14
                        verticalAlignment: Text.AlignVCenter
                        onTextChanged: window.componentFilterText = text

                        background: Rectangle {
                            radius: 16
                            color: "#ffffff"
                            border.color: filterField.activeFocus ? root.theme.borderStrong : root.theme.borderSubtle
                        }

                        Component.onCompleted: text = window.componentFilterText

                        Connections {
                            target: window
                            function onComponentFilterTextChanged() {
                                if (filterField.text !== window.componentFilterText)
                                    filterField.text = window.componentFilterText
                            }
                        }
                    }

                    AppButton {
                        theme: root.theme
                        compact: true
                        tone: "ghost"
                        text: "清空"
                        enabled: (window.componentFilterText || "").length > 0
                        onClicked: window.componentFilterText = ""
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    TagChip {
                        text: window.selectedCapabilityNode
                              ? ("能力域 " + window.selectedCapabilityNode.name)
                              : "等待选择能力域"
                        compact: true
                        fillColor: "#ffffff"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }

                    TagChip {
                        text: "组件 " + window.componentNodes.length
                        compact: true
                        fillColor: "#ffffff"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }

                    TagChip {
                        text: "关系 " + window.componentEdges.length
                        compact: true
                        fillColor: "#ffffff"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }

                    TagChip {
                        text: "阶段 " + window.componentGroups.length
                        compact: true
                        fillColor: "#ffffff"
                        borderColor: root.theme.borderSubtle
                        textColor: root.theme.inkNormal
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    visible: !!window.selectedCapabilityNode
                             && window.componentNodes.length === 1
                             && window.componentEdges.length === 0
                    radius: 18
                    color: "#fff6e7"
                    border.color: "#e1c07c"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 6

                        Label {
                            text: "当前能力域暂时只能展开到一个组件"
                            color: "#8a5d12"
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "这通常表示当前 L2 能力域在现有拆分粒度下只映射到了 1 个 overview 模块，所以 L3 先展示它自己。后续把组件拆分做细后，这里才会继续展开更多内部组件。"
                            wrapMode: Text.WordWrap
                            color: "#7a5823"
                            font.family: root.theme.textFontFamily
                            font.pixelSize: 12
                        }
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
                    visible: !!window.selectedCapabilityNode && window.componentNodes.length > 0
                    clip: true
                    contentWidth: Math.max(width, window.currentComponentSceneWidth())
                    contentHeight: Math.max(height, window.currentComponentSceneHeight())
                    boundsBehavior: Flickable.StopAtBounds

                    property real viewLeft: contentX - 260
                    property real viewRight: contentX + width + 260
                    property real viewTop: contentY - 220
                    property real viewBottom: contentY + height + 220

                    Rectangle {
                        width: window.currentComponentSceneWidth()
                        height: window.currentComponentSceneHeight()
                        color: "#f8fbfd"

                        Repeater {
                            model: window.componentGroups

                            delegate: Rectangle {
                                property var groupBounds: modelData.layoutBounds || {"x": 0, "y": 0, "width": 0, "height": 0}
                                x: groupBounds.x
                                y: groupBounds.y
                                width: groupBounds.width
                                height: groupBounds.height
                                radius: 24
                                visible: modelData.hasBounds
                                color: root.theme.groupFill(index)
                                opacity: 0.82
                                border.color: root.theme.borderSubtle
                                border.width: 1

                                Label {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.margins: 14
                                    text: modelData.name
                                    color: root.theme.inkNormal
                                    font.family: root.theme.displayFontFamily
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                }
                            }
                        }

                        Canvas {
                            id: gridCanvas
                            anchors.fill: parent
                            opacity: 0.22

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.strokeStyle = root.theme.graphGrid
                                ctx.lineWidth = 1

                                for (var x = 0; x < width; x += 44) {
                                    ctx.beginPath()
                                    ctx.moveTo(x, 0)
                                    ctx.lineTo(x, height)
                                    ctx.stroke()
                                }

                                for (var y = 0; y < height; y += 44) {
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
                                function onSelectedComponentNodeChanged() { root.scheduleEdgeRepaint() }
                                function onSelectedComponentEdgeChanged() { root.scheduleEdgeRepaint() }
                                function onComponentRelationshipViewModeChanged() { root.scheduleEdgeRepaint() }
                                function onComponentFilterTextChanged() { root.scheduleEdgeRepaint() }
                            }

                            Connections {
                                target: analysisController
                                function onComponentSceneCatalogChanged() { root.scheduleEdgeRepaint() }
                                function onAnalyzingChanged() { root.scheduleEdgeRepaint() }
                            }

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.lineJoin = "round"
                                ctx.lineCap = "round"
                                var visibleEdges = window.visibleComponentEdges()
                                for (var i = 0; i < visibleEdges.length; ++i)
                                    window.drawComponentEdge(ctx, visibleEdges[i])
                            }

                            onAvailableChanged: root.scheduleEdgeRepaint()
                            Component.onCompleted: root.scheduleEdgeRepaint()
                            onVisibleChanged: root.scheduleEdgeRepaint()
                        }

                        Repeater {
                            model: window.componentNodes

                            delegate: Item {
                                property var layoutRect: window.nodeDisplayRect(modelData)
                                x: layoutRect.x
                                y: layoutRect.y
                                width: layoutRect.width
                                height: layoutRect.height
                                visible: window.componentNodeShouldBeDisplayed(modelData.id)

                                property bool inViewport: (x + width > graphFlick.viewLeft)
                                                          && (x < graphFlick.viewRight)
                                                          && (y + height > graphFlick.viewTop)
                                                          && (y < graphFlick.viewBottom)
                                property var nodeData: modelData

                                Loader {
                                    anchors.fill: parent
                                    sourceComponent: componentNodeComponent
                                    active: parent.inViewport && parent.visible
                                    asynchronous: true
                                    property var nodeData: parent.nodeData
                                }
                            }
                        }
                    }
                }

                Column {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 56, 520)
                    spacing: 12
                    visible: !window.selectedCapabilityNode

                    Label {
                        width: parent.width
                        text: "先在 L2 里选中一个能力域"
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 24
                        font.weight: Font.DemiBold
                    }

                    Label {
                        width: parent.width
                        text: "双击 L2 节点可以直接下钻，也可以先选中能力域，再切到 L3 查看其内部组件图。"
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 13
                    }
                }

                Column {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 56, 560)
                    spacing: 10
                    visible: !!window.selectedCapabilityNode && window.componentNodes.length === 0

                    Label {
                        width: parent.width
                        text: "当前能力域还没有可展示的 L3 组件"
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        color: root.theme.inkStrong
                        font.family: root.theme.displayFontFamily
                        font.pixelSize: 22
                        font.weight: Font.DemiBold
                    }

                    Label {
                        width: parent.width
                        text: (window.componentSceneDiagnostics()[0] || "当前能力域缺少内部 overview 模块或内部依赖边，暂时无法展开更细的组件图。")
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        color: root.theme.inkMuted
                        font.family: root.theme.textFontFamily
                        font.pixelSize: 13
                    }
                }

                Label {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 18
                    anchors.bottomMargin: 16
                    visible: !!window.selectedCapabilityNode && window.componentNodes.length > 0
                    text: window.selectedComponentNode
                          ? "当前高亮：与「" + window.selectedComponentNode.name + "」直接关联的内部关系"
                          : "在组件图里选中节点后，可以继续在右侧查看证据链和关系详情"
                    color: root.theme.inkMuted
                    font.family: root.theme.textFontFamily
                    font.pixelSize: 11
                }
            }
        }
    }
}
