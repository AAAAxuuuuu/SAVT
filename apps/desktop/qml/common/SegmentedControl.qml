import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property QtObject theme

    property var model: []
    property string currentValue: ""

    signal activated(string value)

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    RowLayout {
        id: row
        anchors.fill: parent
        spacing: 8

        Repeater {
            model: root.model || []

            AppButton {
                theme: root.theme
                compact: true
                checkable: true
                checked: root.currentValue === modelData.value
                text: modelData.label || ""
                onClicked: root.activated(modelData.value)
            }
        }
    }
}
