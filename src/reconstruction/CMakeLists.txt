add_library(savt_reconstruction STATIC
    include/savt/reconstruction/ArchitectureReconstruction.h
    src/ArchitectureReconstruction.cpp
)

target_include_directories(savt_reconstruction
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(savt_reconstruction
    PUBLIC
        savt_core
        savt_layout
)

set_target_properties(savt_reconstruction PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER src
)
