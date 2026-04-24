set(savt_desktop_sources
    src/main.cpp
    src/MarkdownRenderer.cpp
    assets/app_icon.qrc
)

if(WIN32)
    list(APPEND savt_desktop_sources
        assets/app_icon.rc
    )
endif()

qt_add_executable(savt_desktop
    ${savt_desktop_sources}
)

qt_add_qml_module(savt_desktop
    URI SAVT.Desktop
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/greenfield/MainSurface.qml
        qml/greenfield/atlas/ArchitectureCanvas.qml
        qml/greenfield/atlas/ArchitectureEdgePainter.js
        qml/greenfield/atlas/ArchitectureEdgeRouter.js
        qml/greenfield/atlas/ArchitectureEdgeUtils.js
        qml/greenfield/config/ConfigDesk.qml
        qml/greenfield/foundation/ActionButton.qml
        qml/greenfield/foundation/Tokens.qml
        qml/greenfield/report/ReportDesk.qml
        qml/greenfield/shell/CaseRail.qml
        qml/greenfield/shell/CommandStrip.qml
        qml/greenfield/shell/DeskShell.qml
        qml/greenfield/shell/FileFocusView.qml
        qml/greenfield/shell/FocusBrief.qml
        qml/greenfield/shell/WorkspaceStage.qml
        qml/greenfield/state/CaseState.qml
        qml/greenfield/state/FocusState.qml
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
