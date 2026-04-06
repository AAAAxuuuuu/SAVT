import QtQuick
import QtQuick.Controls
import "../common"

Item {
    id: root

    required property QtObject theme

    property var nodes: []
    property var groups: []
    property var visibleEdges: []
    property var sceneBounds: ({})
    property Component nodeDelegate
    property var nodeDisplayRectProvider: function(node) { return {"x": 0, "y": 0, "width": 0, "height": 0} }
    property var nodeVisibleProvider: function(nodeId) { return true }
    property var nodeOpacityProvider: function(node) { return 1.0 }
    property var nodeSelectedProvider: function(node) { return false }
    property var nodeKindLabelProvider: function(kind) { return "" }
    property var edgePainter: function(ctx, edge) {}
    property int gridStep: 48
    property real minSceneWidth: 960
    property real minSceneHeight: 520
    property string emptyText: ""
    property string emptyHint: ""
    property real zoomLevel: 1.0
    property real minZoom: 0.64
    property real maxZoom: 1.9

    signal nodeClicked(var node)
    signal nodeDoubleClicked(var node)

    function sceneWidth() {
        return Math.max((sceneBounds && sceneBounds.width) || 0, minSceneWidth)
    }

    function sceneHeight() {
        return Math.max((sceneBounds && sceneBounds.height) || 0, minSceneHeight)
    }

    function clampZoom(value) {
        return Math.max(root.minZoom, Math.min(root.maxZoom, value))
    }

    function clampContentX(value) {
        return Math.max(0, Math.min(value, Math.max(0, graphFlick.contentWidth - graphFlick.width)))
    }

    function clampContentY(value) {
        return Math.max(0, Math.min(value, Math.max(0, graphFlick.contentHeight - graphFlick.height)))
    }

    function applyContentPosition(targetX, targetY) {
        graphFlick.contentX = root.clampContentX(targetX)
        graphFlick.contentY = root.clampContentY(targetY)
    }

    function setZoom(nextZoom, focusX, focusY) {
        var previousZoom = root.zoomLevel
        nextZoom = root.clampZoom(nextZoom)
        if (Math.abs(nextZoom - previousZoom) < 0.001)
            return

        var pivotX = focusX === undefined ? graphFlick.width / 2 : focusX
        var pivotY = focusY === undefined ? graphFlick.height / 2 : focusY
        var sceneX = (graphFlick.contentX + pivotX) / previousZoom
        var sceneY = (graphFlick.contentY + pivotY) / previousZoom

        root.zoomLevel = nextZoom
        root.applyContentPosition(sceneX * nextZoom - pivotX, sceneY * nextZoom - pivotY)
    }

    function nudgeZoom(delta, focusX, focusY) {
        root.setZoom(root.zoomLevel + delta, focusX, focusY)
    }

    function centerScene() {
        root.applyContentPosition((graphFlick.contentWidth - graphFlick.width) / 2,
                                  (graphFlick.contentHeight - graphFlick.height) / 2)
    }

    function fitToScene() {
        if ((root.nodes || []).length === 0) {
            root.zoomLevel = 1.0
            root.applyContentPosition(0, 0)
            return
        }

        var horizontalPadding = 56
        var verticalPadding = 64
        var nextZoom = Math.min((graphFlick.width - horizontalPadding) / root.sceneWidth(),
                                (graphFlick.height - verticalPadding) / root.sceneHeight(),
                                1.0)
        root.zoomLevel = root.clampZoom(nextZoom)
        root.centerScene()
    }

    function zoomLabel() {
        return Math.round(root.zoomLevel * 100) + "%"
    }

    onNodesChanged: Qt.callLater(root.fitToScene)
    onSceneBoundsChanged: Qt.callLater(root.fitToScene)

    Component.onCompleted: Qt.callLater(root.fitToScene)

    Flickable {
        id: graphFlick
        anchors.fill: parent
        clip: true
        contentWidth: Math.max(width, sceneContainer.width)
        contentHeight: Math.max(height, sceneContainer.height)
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentWidth > width || contentHeight > height

        property real viewLeft: (contentX / root.zoomLevel) - 300
        property real viewRight: ((contentX + width) / root.zoomLevel) + 300
        property real viewTop: (contentY / root.zoomLevel) - 260
        property real viewBottom: ((contentY + height) / root.zoomLevel) + 260

        Item {
            id: sceneContainer
            width: root.sceneWidth() * root.zoomLevel
            height: root.sceneHeight() * root.zoomLevel

            Item {
                id: sceneContent
                width: root.sceneWidth()
                height: root.sceneHeight()
                scale: root.zoomLevel
                transformOrigin: Item.TopLeft

                Rectangle {
                    anchors.fill: parent
                    color: root.theme.canvasBase
                }

                Repeater {
                    model: root.groups || []

                    delegate: Rectangle {
                        property var groupBounds: modelData.layoutBounds || {"x": 0, "y": 0, "width": 0, "height": 0}
                        x: groupBounds.x
                        y: groupBounds.y
                        width: groupBounds.width
                        height: groupBounds.height
                        radius: root.theme.radiusXxl
                        visible: modelData.hasBounds
                        color: root.theme.groupFill(index)
                        opacity: 0.72
                        border.color: root.theme.borderSubtle
                        border.width: 1
                    }
                }

                Canvas {
                    anchors.fill: parent
                    opacity: 0.28

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.strokeStyle = root.theme.graphGrid
                        ctx.lineWidth = 1

                        for (var x = 0; x < width; x += root.gridStep) {
                            ctx.beginPath()
                            ctx.moveTo(x, 0)
                            ctx.lineTo(x, height)
                            ctx.stroke()
                        }

                        for (var y = 0; y < height; y += root.gridStep) {
                            ctx.beginPath()
                            ctx.moveTo(0, y)
                            ctx.lineTo(width, y)
                            ctx.stroke()
                        }
                    }
                }

                GraphEdgeCanvas {
                    anchors.fill: parent
                    edges: root.visibleEdges
                    edgePainter: root.edgePainter
                }

                Repeater {
                    model: root.nodes || []

                    delegate: Item {
                        property var layoutRect: root.nodeDisplayRectProvider(modelData)
                        property var nodeData: modelData
                        property bool inViewport: (x + width > graphFlick.viewLeft)
                                                  && (x < graphFlick.viewRight)
                                                  && (y + height > graphFlick.viewTop)
                                                  && (y < graphFlick.viewBottom)

                        x: layoutRect.x
                        y: layoutRect.y
                        width: layoutRect.width
                        height: layoutRect.height
                        visible: root.nodeVisibleProvider(modelData.id)

                        Loader {
                            anchors.fill: parent
                            sourceComponent: root.nodeDelegate
                            active: parent.inViewport && parent.visible
                            asynchronous: true
                            property var theme: root.theme
                            property var nodeData: parent.nodeData
                            property bool selected: root.nodeSelectedProvider(parent.nodeData)
                            property real opacityValue: root.nodeOpacityProvider(parent.nodeData)
                            property string kindLabel: root.nodeKindLabelProvider(parent.nodeData.kind)
                            property var handleNodeClicked: function(node) { root.nodeClicked(node) }
                            property var handleNodeDoubleClicked: function(node) { root.nodeDoubleClicked(node) }
                        }
                    }
                }

                Repeater {
                    model: root.groups || []

                    delegate: Item {
                        property var groupBounds: modelData.layoutBounds || {"x": 0, "y": 0, "width": 0, "height": 0}
                        x: groupBounds.x + 14
                        y: Math.max(8, groupBounds.y + 8)
                        width: Math.max(0, Math.min(groupBounds.width - 28, titleLabel.implicitWidth + 20))
                        height: titleLabel.implicitHeight + 10
                        visible: modelData.hasBounds && (modelData.name || "").length > 0 && width > 0
                        z: 2

                        Rectangle {
                            anchors.fill: parent
                            radius: height / 2
                            color: Qt.rgba(1, 1, 1, 0.94)
                            border.color: root.theme.borderSubtle
                            border.width: 1
                        }

                        Label {
                            id: titleLabel
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            verticalAlignment: Text.AlignVCenter
                            text: modelData.name || ""
                            elide: Text.ElideRight
                            color: root.theme.inkNormal
                            font.family: root.theme.displayFontFamily
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }
            }
        }
    }

    WheelHandler {
        id: wheelZoom
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        target: null
        onWheel: function(event) {
            var delta = event.angleDelta.y > 0 ? 0.12 : -0.12
            root.nudgeZoom(delta, wheelZoom.point.position.x, wheelZoom.point.position.y)
        }
    }

    Column {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 12
        anchors.topMargin: 12
        visible: (root.nodes || []).length > 0

        AppCard {
            fillColor: "#FFFFFF"
            borderColor: root.theme.borderSubtle
            cornerRadius: root.theme.radiusLg
            contentPadding: 6
            minimumContentWidth: 0
            minimumContentHeight: 0

            Row {
                spacing: 6

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: "-"
                    onClicked: root.nudgeZoom(-0.12)
                }

                AppChip {
                    text: root.zoomLabel()
                    compact: true
                    fillColor: root.theme.surfaceSecondary
                    borderColor: root.theme.borderSubtle
                    textColor: root.theme.inkNormal
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: "+"
                    onClicked: root.nudgeZoom(0.12)
                }

                AppButton {
                    theme: root.theme
                    compact: true
                    tone: "ghost"
                    text: "适配"
                    onClicked: root.fitToScene()
                }
            }
        }
    }

    Column {
        anchors.centerIn: parent
        width: Math.min(parent.width - 56, 520)
        spacing: 10
        visible: (root.nodes || []).length === 0

        Label {
            width: parent.width
            text: root.emptyText
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: root.theme.inkStrong
            font.family: root.theme.displayFontFamily
            font.pixelSize: 24
            font.weight: Font.DemiBold
        }

        Label {
            width: parent.width
            visible: root.emptyHint.length > 0
            text: root.emptyHint
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            color: root.theme.inkMuted
            font.family: root.theme.textFontFamily
            font.pixelSize: 13
        }
    }
}
