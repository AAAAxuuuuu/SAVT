add_library(savt_parser STATIC
    src/TreeSitterCppParser.cpp
)

target_include_directories(savt_parser
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/tree-sitter-cpp/bindings/c
)

target_link_libraries(savt_parser
    PUBLIC
        savt_core
    PRIVATE
        tree-sitter
        tree-sitter-cpp
)

set_target_properties(savt_parser PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER src
)
