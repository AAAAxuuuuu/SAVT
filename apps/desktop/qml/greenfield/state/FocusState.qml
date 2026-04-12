import QtQuick

QtObject {
    id: root

    property var focusedNode: null
    property var focusedCapability: null
    property var focusedEdge: null
    property var compareSet: []
    property bool inspectorOpen: false
    property string ledgerTab: "证据"
    property string searchText: ""

    function setNode(node) {
        focusedNode = node
        focusedEdge = null
        inspectorOpen = true
        ledgerTab = "证据"
    }

    function setCapability(node) {
        focusedCapability = node
        setNode(node)
    }

    function clear() {
        focusedNode = null
        focusedCapability = null
        focusedEdge = null
        compareSet = []
        inspectorOpen = false
    }
}
