qt_add_executable(savt_desktop
    src/main.cpp
)

qt_add_qml_module(savt_desktop
    URI SAVT.Desktop
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/common/AppButton.qml
        qml/common/AppCard.qml
        qml/common/AppChip.qml
        qml/common/AppEmptyState.qml
        qml/common/AppSection.qml
        qml/common/SearchField.qml
        qml/common/SegmentedControl.qml
        qml/common/StatusBadge.qml
        qml/graph/CapabilityGraphCanvas.qml
        qml/graph/CapabilityNodeItem.qml
        qml/graph/ComponentGraphCanvas.qml
        qml/graph/ComponentNodeItem.qml
        qml/graph/GraphEdgeCanvas.qml
        qml/graph/GraphLegend.qml
        qml/graph/GraphViewport.qml
        qml/pages/CapabilityMapPage.qml
        qml/pages/ComparePage.qml
        qml/pages/ComponentWorkbenchPage.qml
        qml/pages/ProjectOverviewPage.qml
        qml/pages/ReportPage.qml
        qml/pages/ViewLevelPage.qml
        qml/report/PromptPackSection.qml
        qml/report/ReportActionsSection.qml
        qml/report/ReportCapabilitySection.qml
        qml/report/ReportEvidenceSection.qml
        qml/report/ReportSummarySection.qml
        qml/shell/AppShell.qml
        qml/shell/AppTopBar.qml
        qml/shell/AskSavtWindow.qml
        qml/shell/BreadcrumbBar.qml
        qml/shell/SideNavTree.qml
        qml/shell/WorkspaceHeader.qml
        qml/state/CompareState.qml
        qml/state/DesktopUiState.qml
        qml/state/InspectorState.qml
        qml/state/NavigationState.qml
        qml/state/SelectionState.qml
        qml/theme/AppTheme.qml
        qml/theme/DesignTokens.qml
        qml/workspace/AskSavtPanel.qml
        qml/workspace/BottomContextTray.qml
        qml/workspace/ComponentToolbar.qml
        qml/workspace/ContextActionStrip.qml
        qml/workspace/InspectorDetailsDrawer.qml
        qml/workspace/InspectorPanel.qml
        qml/workspace/InspectorSummaryCard.qml
        qml/workspace/MapToolbar.qml
    RESOURCES
        qml/logic/GraphSelection.js
        qml/logic/InspectorFormatter.js
)

target_link_libraries(savt_desktop
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickDialogs2
        savt_ui
)

set_target_properties(savt_desktop PROPERTIES
    WIN32_EXECUTABLE TRUE
    FOLDER apps
)
