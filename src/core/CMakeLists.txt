add_library(savt_core STATIC
    src/ArchitectureGraph.cpp
    src/ArchitectureAggregation.cpp
    src/ArchitectureOverview.cpp
    src/ArchitectureRuleEngine.cpp
    src/CapabilityGraph.cpp
    src/ComponentGraph.cpp
    src/ProjectAnalysisConfig.cpp
    src/SyntaxTree.cpp
)

target_include_directories(savt_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(savt_core
    PRIVATE
        Qt6::Core
)

set_target_properties(savt_core PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER src
)
