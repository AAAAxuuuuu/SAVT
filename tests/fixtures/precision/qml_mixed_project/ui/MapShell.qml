import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 1280
    height: 720
    title: "Traffic Map"

    LegendPanel {
        anchors.left: parent.left
        anchors.top: parent.top
    }
}
