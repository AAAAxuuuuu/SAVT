import QtQuick

QtObject {
    id: root

    property var focusedNode: null
    property var focusedCapability: null
    property var overviewReturnCapability: null
    property var focusedEdge: null
    property var compareSet: []
    property bool inspectorOpen: false
    property bool restoreOverviewFocusPending: false
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

    function rememberOverviewCapability(node) {
        overviewReturnCapability = node
        restoreOverviewFocusPending = !!node && node.id !== undefined
    }

    function prepareComponentDrill(node) {
        if (!node || node.id === undefined)
            return

        rememberOverviewCapability(node)
        focusedCapability = node
        clearNodeFocus()
    }

    function restoreOverviewCapabilityFocus() {
        var target = overviewReturnCapability
        if ((!target || target.id === undefined) && focusedCapability && focusedCapability.id !== undefined)
            target = focusedCapability

        restoreOverviewFocusPending = false
        if (target && target.id !== undefined)
            setCapability(target)
    }

    function clearNodeFocus() {
        focusedNode = null
        focusedEdge = null
        inspectorOpen = false
        ledgerTab = "证据"
    }

    function clear() {
        clearNodeFocus()
        focusedCapability = null
        overviewReturnCapability = null
        restoreOverviewFocusPending = false
        compareSet = []
    }
}
