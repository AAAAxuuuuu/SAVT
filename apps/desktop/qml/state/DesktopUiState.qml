import QtQuick

QtObject {
    id: root

    property var capabilityScene: ({})
    property var componentSceneCatalog: ({})

    property bool inspectorCollapsed: false
    property bool inspectorDetailsExpanded: false
    property int inspectorExpandedWidth: 348
    property bool contextTrayExpanded: false
    property string contextTab: "context"
    property bool askPanelOpen: false

    readonly property NavigationState navigation: NavigationState {
        id: navigationState
    }

    readonly property SelectionState selection: SelectionState {
        id: selectionState
        capabilityScene: root.capabilityScene
        componentSceneCatalog: root.componentSceneCatalog
        pageId: navigationState.pageId
    }

    readonly property CompareState compare: CompareState {
        id: compareState
    }

    readonly property InspectorState inspector: InspectorState {
        id: inspectorState
        selectionState: selectionState
    }
}
