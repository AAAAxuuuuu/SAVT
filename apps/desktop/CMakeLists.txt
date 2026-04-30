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
        qml/greenfield/atlas/ArchitectureSceneLayout.js
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

install(TARGETS savt_desktop
    RUNTIME DESTINATION .
)

if(WIN32)
    get_target_property(_savt_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    if(NOT _savt_qmake_executable)
        get_target_property(_savt_qmake_executable Qt6::qmake IMPORTED_LOCATION_RELEASE)
    endif()

    if(_savt_qmake_executable)
        get_filename_component(_savt_qt_bin_dir "${_savt_qmake_executable}" DIRECTORY)
        find_program(SAVT_WINDEPLOYQT_EXECUTABLE
            NAMES windeployqt
            HINTS "${_savt_qt_bin_dir}"
            REQUIRED
        )
    endif()

    if(SAVT_WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET savt_desktop POST_BUILD
            COMMAND "${SAVT_WINDEPLOYQT_EXECUTABLE}"
                --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
                --dir "$<TARGET_FILE_DIR:savt_desktop>"
                --compiler-runtime
                "$<TARGET_FILE:savt_desktop>"
            COMMENT "Deploying Qt runtime for savt_desktop"
            VERBATIM
        )
    endif()

    qt_generate_deploy_app_script(
        TARGET savt_desktop
        OUTPUT_SCRIPT savt_desktop_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT "${savt_desktop_deploy_script}")
endif()
