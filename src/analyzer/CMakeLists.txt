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

function(savt_escape_compile_definition out_var value)
    string(REPLACE "\\" "\\\\" escaped_value "${value}")
    string(REPLACE "\"" "\\\"" escaped_value "${escaped_value}")
    set(${out_var} "${escaped_value}" PARENT_SCOPE)
endfunction()

set(SAVT_CLANG_TOOLING_AVAILABLE OFF)
set(SAVT_CLANG_TOOLING_STATUS "disabled")
set(SAVT_CLANG_TOOLING_STATUS_CODE "backend_unavailable")
set(SAVT_CLANG_TOOLING_STATUS_MESSAGE
    "This build does not include the Clang/LibTooling semantic backend. Reconfigure with SAVT_ENABLE_CLANG_TOOLING=ON to enable semantic analysis.")
set(SAVT_LLVM_RUNTIME_BIN_DIR "")
set(SAVT_LIBCLANG_INCLUDE_DIR "")
set(SAVT_LIBCLANG_LIBRARY "")

set(_savt_default_llvm_root "")
if(DEFINED ENV{SAVT_LLVM_ROOT} AND NOT "$ENV{SAVT_LLVM_ROOT}" STREQUAL "")
    set(_savt_default_llvm_root "$ENV{SAVT_LLVM_ROOT}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/tools/llvm")
    file(GLOB _savt_repo_llvm_candidates LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/tools/llvm/*")
    if(_savt_repo_llvm_candidates)
        list(SORT _savt_repo_llvm_candidates COMPARE NATURAL ORDER DESCENDING)
        foreach(_savt_candidate IN LISTS _savt_repo_llvm_candidates)
            if(EXISTS "${_savt_candidate}/include/clang-c/Index.h" OR
               EXISTS "${_savt_candidate}/lib" OR
               EXISTS "${_savt_candidate}/bin")
                set(_savt_default_llvm_root "${_savt_candidate}")
                break()
            endif()
        endforeach()
    endif()
endif()

set(SAVT_LLVM_ROOT "${_savt_default_llvm_root}"
    CACHE PATH "Root directory of the LLVM/Clang distribution used for semantic analysis")

if(SAVT_ENABLE_CLANG_TOOLING)
    set(_savt_llvm_hints)
    if(NOT "${SAVT_LLVM_ROOT}" STREQUAL "")
        list(APPEND _savt_llvm_hints "${SAVT_LLVM_ROOT}")
    endif()
    if(DEFINED ENV{SAVT_LLVM_ROOT} AND NOT "$ENV{SAVT_LLVM_ROOT}" STREQUAL "")
        list(APPEND _savt_llvm_hints "$ENV{SAVT_LLVM_ROOT}")
    endif()

    find_path(SAVT_LIBCLANG_INCLUDE_DIR
        NAMES clang-c/Index.h
        HINTS ${_savt_llvm_hints}
        PATH_SUFFIXES include
    )

    find_library(SAVT_LIBCLANG_LIBRARY
        NAMES libclang clang
        HINTS ${_savt_llvm_hints}
        PATH_SUFFIXES lib bin
    )

    if(SAVT_LIBCLANG_INCLUDE_DIR AND SAVT_LIBCLANG_LIBRARY)
        set(SAVT_CLANG_TOOLING_AVAILABLE ON)
        set(SAVT_CLANG_TOOLING_STATUS "enabled")
        set(SAVT_CLANG_TOOLING_STATUS_CODE "semantic_ready")
        set(SAVT_CLANG_TOOLING_STATUS_MESSAGE
            "Clang/LibTooling semantic backend is available in this build.")
        if(NOT "${SAVT_LLVM_ROOT}" STREQUAL "" AND EXISTS "${SAVT_LLVM_ROOT}/bin")
            set(SAVT_LLVM_RUNTIME_BIN_DIR "${SAVT_LLVM_ROOT}/bin")
        else()
            get_filename_component(SAVT_LLVM_RUNTIME_BIN_DIR "${SAVT_LIBCLANG_LIBRARY}" DIRECTORY)
        endif()
        target_include_directories(savt_analyzer PRIVATE ${SAVT_LIBCLANG_INCLUDE_DIR})
        target_link_libraries(savt_analyzer PRIVATE ${SAVT_LIBCLANG_LIBRARY})
        message(STATUS
            "SAVT semantic backend: enabled "
            "(headers: ${SAVT_LIBCLANG_INCLUDE_DIR}, libclang: ${SAVT_LIBCLANG_LIBRARY})")
    else()
        if(NOT SAVT_LIBCLANG_INCLUDE_DIR AND NOT SAVT_LIBCLANG_LIBRARY)
            set(SAVT_CLANG_TOOLING_STATUS "llvm_not_found")
            set(SAVT_CLANG_TOOLING_STATUS_CODE "llvm_not_found")
            if(NOT "${SAVT_LLVM_ROOT}" STREQUAL "")
                set(SAVT_CLANG_TOOLING_STATUS_MESSAGE
                    "SAVT_ENABLE_CLANG_TOOLING was requested, but no compatible LLVM/Clang headers or libclang library were found under ${SAVT_LLVM_ROOT}.")
            else()
                set(SAVT_CLANG_TOOLING_STATUS_MESSAGE
                    "SAVT_ENABLE_CLANG_TOOLING was requested, but no compatible LLVM/Clang installation was found. Set SAVT_LLVM_ROOT to a distribution that contains clang-c/Index.h and libclang.")
            endif()
        elseif(NOT SAVT_LIBCLANG_INCLUDE_DIR)
            set(SAVT_CLANG_TOOLING_STATUS "llvm_headers_missing")
            set(SAVT_CLANG_TOOLING_STATUS_CODE "llvm_headers_missing")
            if(NOT "${SAVT_LLVM_ROOT}" STREQUAL "")
                set(SAVT_CLANG_TOOLING_STATUS_MESSAGE
                    "SAVT_ENABLE_CLANG_TOOLING was requested, but clang-c/Index.h was not found under ${SAVT_LLVM_ROOT}.")
            else()
                set(SAVT_CLANG_TOOLING_STATUS_MESSAGE
                    "SAVT_ENABLE_CLANG_TOOLING was requested, but clang-c/Index.h was not found in the configured LLVM/Clang installation.")
            endif()
        else()
            set(SAVT_CLANG_TOOLING_STATUS "libclang_not_found")
            set(SAVT_CLANG_TOOLING_STATUS_CODE "libclang_not_found")
            if(NOT "${SAVT_LLVM_ROOT}" STREQUAL "")
                set(SAVT_CLANG_TOOLING_STATUS_MESSAGE
                    "SAVT_ENABLE_CLANG_TOOLING was requested, but the libclang library was not found under ${SAVT_LLVM_ROOT}.")
            else()
                set(SAVT_CLANG_TOOLING_STATUS_MESSAGE
                    "SAVT_ENABLE_CLANG_TOOLING was requested, but the libclang library was not found in the configured LLVM/Clang installation.")
            endif()
        endif()
        message(WARNING
            "SAVT semantic backend: unavailable (${SAVT_CLANG_TOOLING_STATUS_CODE}). "
            "${SAVT_CLANG_TOOLING_STATUS_MESSAGE}")
    endif()
endif()

set(SAVT_CLANG_TOOLING_AVAILABLE ${SAVT_CLANG_TOOLING_AVAILABLE}
    CACHE BOOL "Whether the Clang/LibTooling semantic backend is available in this build" FORCE)
set(SAVT_CLANG_TOOLING_STATUS "${SAVT_CLANG_TOOLING_STATUS}"
    CACHE STRING "Resolved status of the Clang/LibTooling semantic backend" FORCE)
set(SAVT_CLANG_TOOLING_STATUS_CODE "${SAVT_CLANG_TOOLING_STATUS_CODE}"
    CACHE STRING "Resolved runtime status code for the Clang/LibTooling semantic backend" FORCE)
set(SAVT_CLANG_TOOLING_STATUS_MESSAGE "${SAVT_CLANG_TOOLING_STATUS_MESSAGE}"
    CACHE STRING "Resolved runtime status message for the Clang/LibTooling semantic backend" FORCE)
set(SAVT_LLVM_RUNTIME_BIN_DIR "${SAVT_LLVM_RUNTIME_BIN_DIR}"
    CACHE INTERNAL "LLVM runtime bin directory used by SAVT tests and tools")

savt_escape_compile_definition(SAVT_CLANG_TOOLING_STATUS_CODE_ESCAPED "${SAVT_CLANG_TOOLING_STATUS_CODE}")
savt_escape_compile_definition(SAVT_CLANG_TOOLING_STATUS_MESSAGE_ESCAPED "${SAVT_CLANG_TOOLING_STATUS_MESSAGE}")
target_compile_definitions(savt_analyzer PUBLIC
    "SAVT_CLANG_TOOLING_STATUS_CODE=\"${SAVT_CLANG_TOOLING_STATUS_CODE_ESCAPED}\""
    "SAVT_CLANG_TOOLING_STATUS_MESSAGE=\"${SAVT_CLANG_TOOLING_STATUS_MESSAGE_ESCAPED}\""
)

if(SAVT_ENABLE_CLANG_TOOLING)
    savt_escape_compile_definition(SAVT_LLVM_ROOT_ESCAPED "${SAVT_LLVM_ROOT}")
    target_compile_definitions(savt_analyzer PUBLIC
        SAVT_CLANG_TOOLING_REQUESTED=1
        "SAVT_CONFIGURED_LLVM_ROOT=\"${SAVT_LLVM_ROOT_ESCAPED}\""
    )
endif()

if(SAVT_CLANG_TOOLING_AVAILABLE)
    target_compile_definitions(savt_analyzer PUBLIC SAVT_ENABLE_CLANG_TOOLING=1)
endif()

set_target_properties(savt_analyzer PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER src
)
