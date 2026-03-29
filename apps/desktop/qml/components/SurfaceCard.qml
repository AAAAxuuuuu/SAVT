import QtQuick
import QtQuick.Controls

Control {
    id: root

    property color fillColor: "#ffffff"
    property color borderColor: "#d7e0e8"
    property color stackColorFront: "#e7eef5"
    property color stackColorBack: "#dce6ef"
    property int cornerRadius: 24
    property int contentPadding: 18
    property bool stacked: false
    property real minimumContentWidth: 220
    property real minimumContentHeight: 96

    default property alias contentData: contentRoot.data

    padding: contentPadding
    implicitWidth: Math.max(
                       minimumContentWidth + leftPadding + rightPadding,
                       contentRoot.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        minimumContentHeight + topPadding + bottomPadding,
                        contentRoot.implicitHeight + topPadding + bottomPadding)

    background: Item {
        Rectangle {
            visible: root.stacked
            x: 14
            y: 14
            width: parent.width
            height: parent.height
            radius: root.cornerRadius
            color: root.stackColorBack
            border.color: Qt.rgba(1, 1, 1, 0.15)
        }

        Rectangle {
            visible: root.stacked
            x: 7
            y: 7
            width: parent.width
            height: parent.height
            radius: root.cornerRadius
            color: root.stackColorFront
            border.color: Qt.rgba(1, 1, 1, 0.2)
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
        implicitWidth: {
            var maxWidth = 0
            for (var i = 0; i < children.length; ++i) {
                maxWidth = Math.max(maxWidth, children[i].implicitWidth || 0)
            }
            return maxWidth
        }
        implicitHeight: {
            var maxHeight = 0
            for (var i = 0; i < children.length; ++i) {
                maxHeight = Math.max(maxHeight, children[i].implicitHeight || 0)
            }
            return maxHeight
        }
    }
}
