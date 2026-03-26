import QtQuick
import QtQuick.Controls

Frame {
    id: root
    background: Rectangle {
        radius: 10
        color: "#ffffff"
        border.color: "#e2e8f0"
    }

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

            radius: 12
            clip: true
            opacity: window.capabilityNodeOpacity(nodeData)
            color: window.nodeFill(nodeData.kind, window.selectedCapabilityNode && window.selectedCapabilityNode.id === nodeData.id)
            border.color: nodeData.pinned ? "#fde68a" : "#93c5fd"
            border.width: window.selectedCapabilityNode && window.selectedCapabilityNode.id === nodeData.id ? 2 : 1

            Column {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 5

                Text {
                    text: nodeData.name
                    color: window.selectedCapabilityNode && window.selectedCapabilityNode.id === nodeData.id ? "#ffffff" : "#0f172a"
                    font.pixelSize: Math.max(12, 16 - Math.max(0, Math.ceil((nodeData.name.length - 18) / 6)))
                    font.bold: true
                    width: parent.width
                    wrapMode: Text.WordWrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }

                Text {
                    text: window.displayNodeKind(nodeData.kind)
                    color: window.selectedCapabilityNode && window.selectedCapabilityNode.id === nodeData.id ? "#bfdbfe" : "#1e40af"
                    font.pixelSize: 10
                    width: parent.width
                    elide: Text.ElideRight
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

    Flickable {
        id: graphFlick
        anchors.fill: parent
        anchors.margins: 4
        clip: true
        contentWidth: Math.max(width, window.currentCapabilitySceneWidth())
        contentHeight: Math.max(height, window.currentCapabilitySceneHeight())
        boundsBehavior: Flickable.StopAtBounds

        property real viewLeft: contentX - 300
        property real viewRight: contentX + width + 300
        property real viewTop: contentY - 300
        property real viewBottom: contentY + height + 300

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onWheel: function(wheel) { wheel.accepted = true }
        }

        Rectangle {
            width: window.currentCapabilitySceneWidth()
            height: window.currentCapabilitySceneHeight()
            color: "transparent"

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
                    function onCapabilityEdgeItemsChanged() { root.scheduleEdgeRepaint() }
                    function onCapabilityNodeItemsChanged() { root.scheduleEdgeRepaint() }
                    function onCapabilitySceneWidthChanged() { root.scheduleEdgeRepaint() }
                    function onCapabilitySceneHeightChanged() { root.scheduleEdgeRepaint() }
                    function onAnalyzingChanged() { root.scheduleEdgeRepaint() }
                }

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.lineCap = "round"
                    var visibleEdges = window.visibleCapabilityEdges()
                    for (var i = 0; i < visibleEdges.length; ++i) {
                        window.drawCapabilityEdge(ctx, visibleEdges[i], true)
                    }
                }

                onAvailableChanged: root.scheduleEdgeRepaint()
                Component.onCompleted: root.scheduleEdgeRepaint()
                onVisibleChanged: root.scheduleEdgeRepaint()
            }

            Repeater {
                model: analysisController.capabilityNodeItems

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
        }
    }
    Text {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 16
        text: "\u70b9\u51fb\u4efb\u610f\u6a21\u5757\u67e5\u770b\u5176\u5173\u8054\u5173\u7cfb"
        color: "#94a3b8"
        font.pixelSize: 13
        visible: !window.selectedCapabilityNode
    }
}
