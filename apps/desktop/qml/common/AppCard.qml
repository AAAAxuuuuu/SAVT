import QtQuick
import QtQuick.Controls

Control {
    id: root

    property color fillColor: "#FFFFFF"
    property color borderColor: "#D7E0E8"
    property color stackColorFront: "#E8EEF4"
    property color stackColorBack: "#DCE6EF"
    property int cornerRadius: 24
    property int contentPadding: 18
    property bool stacked: false
    property real minimumContentWidth: 220
    property real minimumContentHeight: 96

    default property alias contentData: contentRoot.data

    function measuredChildWidth(child) {
        if (!child || child.visible === false)
            return 0

        var width = child.implicitWidth
        if (!isFinite(width) || width <= 0)
            width = child.childrenRect ? child.childrenRect.width : 0

        return Math.max(0, width + Math.max(0, child.x || 0))
    }

    function measuredChildHeight(child) {
        if (!child || child.visible === false)
            return 0

        var height = child.implicitHeight
        if (!isFinite(height) || height <= 0)
            height = child.childrenRect ? child.childrenRect.height : 0

        return Math.max(0, height + Math.max(0, child.y || 0))
    }

    function contentImplicitWidth() {
        var width = contentRoot.childrenRect.width
        var children = contentRoot.children
        for (var index = 0; index < children.length; ++index)
            width = Math.max(width, root.measuredChildWidth(children[index]))
        return width
    }

    function contentImplicitHeight() {
        var height = contentRoot.childrenRect.height
        var children = contentRoot.children
        for (var index = 0; index < children.length; ++index)
            height = Math.max(height, root.measuredChildHeight(children[index]))
        return height
    }

    padding: contentPadding
    implicitWidth: Math.max(minimumContentWidth + leftPadding + rightPadding,
                            root.contentImplicitWidth() + leftPadding + rightPadding)
    implicitHeight: Math.max(minimumContentHeight + topPadding + bottomPadding,
                             root.contentImplicitHeight() + topPadding + bottomPadding)

    background: Item {
        Rectangle {
            visible: root.stacked
            x: 12
            y: 12
            width: parent.width
            height: parent.height
            radius: root.cornerRadius
            color: root.stackColorBack
            border.color: Qt.rgba(1, 1, 1, 0.12)
        }

        Rectangle {
            visible: root.stacked
            x: 6
            y: 6
            width: parent.width
            height: parent.height
            radius: root.cornerRadius
            color: root.stackColorFront
            border.color: Qt.rgba(1, 1, 1, 0.16)
        }

        Rectangle {
            anchors.fill: parent
            radius: root.cornerRadius
            color: root.fillColor
            border.color: root.borderColor
            border.width: 1
        }
    }

    contentItem: Item {
        id: contentRoot
        clip: true
        implicitWidth: root.contentImplicitWidth()
        implicitHeight: root.contentImplicitHeight()
    }
}
