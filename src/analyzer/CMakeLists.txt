add_library(savt_analyzer STATIC
    src/AnalysisGraphBuilder.cpp
    src/AnalyzerUtilities.cpp
    src/CppProjectAnalyzer.cpp
    src/SemanticProjectAnalyzer.cpp
    src/SyntaxProjectAnalyzer.cpp
)

target_include_directories(savt_analyzer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${SAVT_TREE_SITTER_CPP_BINDINGS_DIR}
)

target_link_libraries(savt_analyzer
    PUBLIC
        savt_core
    PRIVATE
        tree-sitter
        tree-sitter-cpp
)

set(SAVT_CLANG_TOOLING_AVAILABLE OFF)
set(SAVT_CLANG_TOOLING_STATUS "disabled")
set(SAVT_LLVM_RUNTIME_BIN_DIR "")

if(SAVT_ENABLE_CLANG_TOOLING)
    set(SAVT_LLVM_ROOT
        "${CMAKE_SOURCE_DIR}/tools/llvm/clang+llvm-21.1.7-x86_64-pc-windows-msvc"
        CACHE PATH "Root directory of the LLVM/Clang distribution used for semantic analysis")

    find_path(SAVT_LIBCLANG_INCLUDE_DIR
        NAMES clang-c/Index.h
        HINTS
            "${SAVT_LLVM_ROOT}"
            "$ENV{SAVT_LLVM_ROOT}"
        PATH_SUFFIXES include
    )

    find_library(SAVT_LIBCLANG_LIBRARY
        NAMES libclang clang
        HINTS
            "${SAVT_LLVM_ROOT}"
            "$ENV{SAVT_LLVM_ROOT}"
        PATH_SUFFIXES lib
    )

    if(SAVT_LIBCLANG_INCLUDE_DIR AND SAVT_LIBCLANG_LIBRARY)
        set(SAVT_CLANG_TOOLING_AVAILABLE ON)
        set(SAVT_CLANG_TOOLING_STATUS "enabled")
        set(SAVT_LLVM_RUNTIME_BIN_DIR "${SAVT_LLVM_ROOT}/bin")
        target_include_directories(savt_analyzer PRIVATE ${SAVT_LIBCLANG_INCLUDE_DIR})
        target_link_libraries(savt_analyzer PRIVATE ${SAVT_LIBCLANG_LIBRARY})
        message(STATUS "SAVT semantic backend: enabled (LLVM root: ${SAVT_LLVM_ROOT})")
    else()
        set(SAVT_CLANG_TOOLING_STATUS "llvm_not_found")
        message(WARNING
            "SAVT_ENABLE_CLANG_TOOLING=ON, but LLVM/Clang headers or libraries were not found. "
            "Semantic analysis will be unavailable at runtime. "
            "Searched root: ${SAVT_LLVM_ROOT}")
    endif()
endif()

set(SAVT_CLANG_TOOLING_AVAILABLE ${SAVT_CLANG_TOOLING_AVAILABLE}
    CACHE BOOL "Whether the Clang/LibTooling semantic backend is available in this build" FORCE)
set(SAVT_CLANG_TOOLING_STATUS "${SAVT_CLANG_TOOLING_STATUS}"
    CACHE STRING "Resolved status of the Clang/LibTooling semantic backend" FORCE)
set(SAVT_LLVM_RUNTIME_BIN_DIR "${SAVT_LLVM_RUNTIME_BIN_DIR}"
    CACHE INTERNAL "LLVM runtime bin directory used by SAVT tests and tools")

if(SAVT_ENABLE_CLANG_TOOLING)
    string(REPLACE "\\" "\\\\" SAVT_LLVM_ROOT_ESCAPED "${SAVT_LLVM_ROOT}")
    string(REPLACE "\"" "\\\"" SAVT_LLVM_ROOT_ESCAPED "${SAVT_LLVM_ROOT_ESCAPED}")
    target_compile_definitions(savt_analyzer PUBLIC
        SAVT_CLANG_TOOLING_REQUESTED=1
        "SAVT_CONFIGURED_LLVM_ROOT=\"${SAVT_LLVM_ROOT_ESCAPED}\""
    )
endif()

if(SAVT_CLANG_TOOLING_AVAILABLE)
    target_compile_definitions(savt_analyzer PUBLIC SAVT_ENABLE_CLANG_TOOLING=1)
elseif(SAVT_ENABLE_CLANG_TOOLING)
    target_compile_definitions(savt_analyzer PUBLIC SAVT_CLANG_TOOLING_MISSING=1)
endif()

set_target_properties(savt_analyzer PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER src
)
