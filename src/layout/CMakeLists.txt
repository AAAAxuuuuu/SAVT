add_library(savt_layout STATIC
    src/LayeredGraphLayout.cpp
)

target_include_directories(savt_layout
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(savt_layout
    PUBLIC
        savt_core
)

set_target_properties(savt_layout PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER src
)
