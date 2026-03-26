add_library(savt_core STATIC
    src/ArchitectureGraph.cpp
    src/ArchitectureOverview.cpp
    src/CapabilityGraph.cpp
    src/SyntaxTree.cpp
)

target_include_directories(savt_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(savt_core PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER src
)
