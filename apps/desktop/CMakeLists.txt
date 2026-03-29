qt_add_executable(savt_desktop
    src/main.cpp
)

qt_add_qml_module(savt_desktop
    URI SAVT.Desktop
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/AppButton.qml
        qml/components/AppTheme.qml
        qml/components/TopControlBar.qml
        qml/components/L2GraphView.qml
        qml/components/RightInspector.qml
        qml/components/SurfaceCard.qml
        qml/components/TagChip.qml
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
