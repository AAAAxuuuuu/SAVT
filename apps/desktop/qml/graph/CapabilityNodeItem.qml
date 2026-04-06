import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var theme: parent.theme
    property var nodeData: parent.nodeData
    property bool selected: parent.selected
    property real opacityValue: parent.opacityValue
    property string kindLabel: parent.kindLabel
    property var handleNodeClicked: parent.handleNodeClicked
    property var handleNodeDoubleClicked: parent.handleNodeDoubleClicked

    implicitWidth: 228
    implicitHeight: 112
    radius: 16
    clip: true
    opacity: opacityValue
    color: theme.elevatedSurface("#FFFFFF", hoverArea.containsMouse, selected)
    border.color: selected ? theme.selectionRing : theme.nodeBorder(nodeData.kind, false, nodeData.pinned)
    border.width: selected ? 2 : 1

    Rectangle {
        width: 6
        radius: 3
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 10
        color: theme.nodeAccent(nodeData.kind, selected)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 14
        anchors.topMargin: 12
        anchors.bottomMargin: 12
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: kindLabel
                Layout.maximumWidth: 88
                color: theme.nodeAccent(nodeData.kind, selected)
                font.family: theme.textFontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Item { Layout.fillWidth: true }

            Label {
                visible: !!nodeData.pinned
                text: "Pinned"
                color: "#A67217"
                font.family: theme.textFontFamily
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
        }

        Label {
            Layout.fillWidth: true
            text: nodeData.name || ""
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            color: theme.inkStrong
            font.family: theme.displayFontFamily
            font.pixelSize: 18
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            text: nodeData.responsibility || nodeData.summary || ""
            visible: (nodeData.responsibility || nodeData.summary || "").length > 0
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            color: theme.inkNormal
            font.family: theme.textFontFamily
            font.pixelSize: 12
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: (nodeData.role || "").length > 0 ? nodeData.role : "未标注角色"
                color: theme.inkMuted
                font.family: theme.textFontFamily
                font.pixelSize: 11
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: "文件 " + (nodeData.fileCount || 0)
                color: theme.inkMuted
                font.family: theme.textFontFamily
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
        }
    }

    ToolTip.visible: hoverArea.containsMouse
    ToolTip.delay: 280
    ToolTip.timeout: 4000
    ToolTip.text: {
        var message = nodeData.summary || nodeData.name || ""
        return message.length > 120 ? message.slice(0, 120) + "…" : message
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (typeof root.handleNodeClicked === "function")
                root.handleNodeClicked(root.nodeData)
        }
        onDoubleClicked: {
            if (typeof root.handleNodeDoubleClicked === "function")
                root.handleNodeDoubleClicked(root.nodeData)
        }
    }
}
