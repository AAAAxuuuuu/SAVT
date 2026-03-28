cmake_minimum_required(VERSION 3.24)

include(FetchContent)

if(WIN32)
    function(savt_find_first_existing out_var)
        foreach(candidate IN LISTS ARGN)
            if(candidate AND EXISTS "${candidate}")
                set(${out_var} "${candidate}" PARENT_SCOPE)
                return()
            endif()
        endforeach()
        set(${out_var} "" PARENT_SCOPE)
    endfunction()

    function(savt_find_latest_glob out_var pattern suffix)
        file(GLOB candidate_dirs LIST_DIRECTORIES true "${pattern}")
        if(candidate_dirs)
            list(SORT candidate_dirs COMPARE NATURAL ORDER DESCENDING)
            foreach(candidate_dir IN LISTS candidate_dirs)
                set(candidate_path "${candidate_dir}/${suffix}")
                if(EXISTS "${candidate_path}")
                    set(${out_var} "${candidate_path}" PARENT_SCOPE)
                    return()
                endif()
            endforeach()
        endif()
        set(${out_var} "" PARENT_SCOPE)
    endfunction()

    if(NOT DEFINED CMAKE_C_COMPILER OR CMAKE_C_COMPILER STREQUAL "cl.exe")
        set(_savt_msvc_cl_candidates)
        if(DEFINED ENV{VCToolsInstallDir} AND NOT "$ENV{VCToolsInstallDir}" STREQUAL "")
            list(APPEND _savt_msvc_cl_candidates "$ENV{VCToolsInstallDir}/bin/HostX64/x64/cl.exe")
        endif()

        foreach(_savt_vs_edition IN ITEMS Preview Community Professional Enterprise BuildTools)
            savt_find_latest_glob(
                _savt_vs_cl
                "C:/Program Files/Microsoft Visual Studio/2022/${_savt_vs_edition}/VC/Tools/MSVC/*"
                "bin/HostX64/x64/cl.exe")
            if(_savt_vs_cl)
                list(APPEND _savt_msvc_cl_candidates "${_savt_vs_cl}")
            endif()
        endforeach()

        savt_find_first_existing(_savt_msvc_cl_path ${_savt_msvc_cl_candidates})
        if(_savt_msvc_cl_path)
            set(CMAKE_C_COMPILER "${_savt_msvc_cl_path}" CACHE FILEPATH "Resolved MSVC C compiler" FORCE)
            set(CMAKE_CXX_COMPILER "${_savt_msvc_cl_path}" CACHE FILEPATH "Resolved MSVC CXX compiler" FORCE)
        endif()
    endif()

    if(NOT DEFINED CMAKE_RC_COMPILER OR CMAKE_RC_COMPILER STREQUAL "rc")
        set(_savt_rc_candidates)
        if(DEFINED ENV{WindowsSdkDir} AND DEFINED ENV{WindowsSDKVersion} AND
           NOT "$ENV{WindowsSdkDir}" STREQUAL "" AND NOT "$ENV{WindowsSDKVersion}" STREQUAL "")
            list(APPEND _savt_rc_candidates "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x64/rc.exe")
        endif()
        savt_find_latest_glob(_savt_sdk_rc "C:/Program Files (x86)/Windows Kits/10/bin/*" "x64/rc.exe")
        if(_savt_sdk_rc)
            list(APPEND _savt_rc_candidates "${_savt_sdk_rc}")
        endif()
        savt_find_first_existing(_savt_rc_path ${_savt_rc_candidates})
        if(_savt_rc_path)
            set(CMAKE_RC_COMPILER "${_savt_rc_path}" CACHE FILEPATH "Resolved Windows resource compiler" FORCE)
        endif()
    endif()

    if(NOT DEFINED CMAKE_MT OR CMAKE_MT STREQUAL "mt" OR CMAKE_MT STREQUAL "mt.exe")
        set(_savt_mt_candidates)
        if(DEFINED ENV{WindowsSdkDir} AND DEFINED ENV{WindowsSDKVersion} AND
           NOT "$ENV{WindowsSdkDir}" STREQUAL "" AND NOT "$ENV{WindowsSDKVersion}" STREQUAL "")
            list(APPEND _savt_mt_candidates "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x64/mt.exe")
        endif()
        savt_find_latest_glob(_savt_sdk_mt "C:/Program Files (x86)/Windows Kits/10/bin/*" "x64/mt.exe")
        if(_savt_sdk_mt)
            list(APPEND _savt_mt_candidates "${_savt_sdk_mt}")
        endif()
        savt_find_first_existing(_savt_mt_path ${_savt_mt_candidates})
        if(_savt_mt_path)
            set(CMAKE_MT "${_savt_mt_path}" CACHE FILEPATH "Resolved manifest tool" FORCE)
        endif()
    endif()
endif()

project(SAVT
    VERSION 0.1.0
    DESCRIPTION "Software Architecture Visualization Tool"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C,CXX>:-finput-charset=UTF-8>
        $<$<COMPILE_LANGUAGE:C,CXX>:-fexec-charset=UTF-8>
    )
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries by default" FORCE)

if(WIN32)
    find_program(SAVT_POWERSHELL_EXECUTABLE NAMES pwsh powershell)
endif()

option(SAVT_ENABLE_CLANG_TOOLING "Build the Clang/LibTooling semantic analysis backend" OFF)
option(SAVT_FETCH_THIRD_PARTY "Automatically fetch required third-party sources when missing" ON)

set(SAVT_TREE_SITTER_GIT_REPOSITORY "https://github.com/tree-sitter/tree-sitter.git")
set(SAVT_TREE_SITTER_GIT_TAG "cb01ffcc3f27b0be8930132d941f162a3a854211")
set(SAVT_TREE_SITTER_CPP_GIT_REPOSITORY "https://github.com/tree-sitter/tree-sitter-cpp.git")
set(SAVT_TREE_SITTER_CPP_GIT_TAG "8b5b49eb196bec7040441bee33b2c9a4838d6967")

if(NOT DEFINED CMAKE_PREFIX_PATH AND NOT DEFINED ENV{SAVT_QT_ROOT})
    message(STATUS "Qt root was not provided. Set CMAKE_PREFIX_PATH or the SAVT_QT_ROOT environment variable before configuring.")
endif()

find_package(Qt6 6.9 REQUIRED COMPONENTS Concurrent Core Gui Network Qml Quick QuickControls2 QuickDialogs2)
qt_standard_project_setup(REQUIRES 6.9)

set(SAVT_TREE_SITTER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/tree-sitter")
if(EXISTS "${SAVT_TREE_SITTER_SOURCE_DIR}/CMakeLists.txt")
    add_subdirectory("${SAVT_TREE_SITTER_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_deps/tree-sitter-build" EXCLUDE_FROM_ALL)
else()
    if(NOT SAVT_FETCH_THIRD_PARTY)
        message(FATAL_ERROR
            "tree-sitter source was not found under third_party/tree-sitter. "
            "Either populate the directory manually or reconfigure with SAVT_FETCH_THIRD_PARTY=ON.")
    endif()

    message(STATUS "Fetching tree-sitter from ${SAVT_TREE_SITTER_GIT_REPOSITORY} at ${SAVT_TREE_SITTER_GIT_TAG}")
    FetchContent_Declare(savt_tree_sitter
        GIT_REPOSITORY "${SAVT_TREE_SITTER_GIT_REPOSITORY}"
        GIT_TAG "${SAVT_TREE_SITTER_GIT_TAG}"
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(savt_tree_sitter)
endif()

set(SAVT_TREE_SITTER_CPP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/tree-sitter-cpp")
set(SAVT_TREE_SITTER_CPP_PARSER "${SAVT_TREE_SITTER_CPP_SOURCE_DIR}/src/parser.c")
if(NOT EXISTS "${SAVT_TREE_SITTER_CPP_PARSER}")
    if(NOT SAVT_FETCH_THIRD_PARTY)
        message(FATAL_ERROR
            "tree-sitter-cpp parser.c is missing under third_party/tree-sitter-cpp. "
            "Either populate the directory manually or reconfigure with SAVT_FETCH_THIRD_PARTY=ON.")
    endif()

    message(STATUS "Fetching tree-sitter-cpp from ${SAVT_TREE_SITTER_CPP_GIT_REPOSITORY} at ${SAVT_TREE_SITTER_CPP_GIT_TAG}")
    FetchContent_Declare(savt_tree_sitter_cpp
        GIT_REPOSITORY "${SAVT_TREE_SITTER_CPP_GIT_REPOSITORY}"
        GIT_TAG "${SAVT_TREE_SITTER_CPP_GIT_TAG}"
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(savt_tree_sitter_cpp)
    if(NOT savt_tree_sitter_cpp_POPULATED)
        cmake_policy(PUSH)
        cmake_policy(SET CMP0169 OLD)
        FetchContent_Populate(savt_tree_sitter_cpp)
        cmake_policy(POP)
    endif()
    set(SAVT_TREE_SITTER_CPP_SOURCE_DIR "${savt_tree_sitter_cpp_SOURCE_DIR}")
    set(SAVT_TREE_SITTER_CPP_PARSER "${SAVT_TREE_SITTER_CPP_SOURCE_DIR}/src/parser.c")
endif()

set(SAVT_TREE_SITTER_CPP_BINDINGS_DIR "${SAVT_TREE_SITTER_CPP_SOURCE_DIR}/bindings/c")
if(NOT EXISTS "${SAVT_TREE_SITTER_CPP_PARSER}")
    message(FATAL_ERROR "tree-sitter-cpp parser.c is missing after dependency resolution.")
endif()

if(NOT EXISTS "${SAVT_TREE_SITTER_CPP_BINDINGS_DIR}/tree-sitter-cpp.h")
    message(FATAL_ERROR "tree-sitter-cpp headers are missing after dependency resolution.")
endif()

add_library(tree-sitter-cpp "${SAVT_TREE_SITTER_CPP_PARSER}")
if(EXISTS "${SAVT_TREE_SITTER_CPP_SOURCE_DIR}/src/scanner.c")
    target_sources(tree-sitter-cpp PRIVATE "${SAVT_TREE_SITTER_CPP_SOURCE_DIR}/src/scanner.c")
endif()
target_include_directories(tree-sitter-cpp
    PUBLIC "${SAVT_TREE_SITTER_CPP_BINDINGS_DIR}"
    PRIVATE "${SAVT_TREE_SITTER_CPP_SOURCE_DIR}/src"
)
target_compile_definitions(tree-sitter-cpp PRIVATE $<$<CONFIG:Debug>:TREE_SITTER_DEBUG>)

set_target_properties(tree-sitter PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER third_party
)

set_target_properties(tree-sitter-cpp PROPERTIES
    C_STANDARD 11
    POSITION_INDEPENDENT_CODE ON
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER third_party
)

add_subdirectory(src/core)
add_subdirectory(src/parser)
add_subdirectory(src/analyzer)
add_subdirectory(src/ai)
add_subdirectory(src/layout)

add_subdirectory(src/ui)

add_subdirectory(apps/desktop)

set(SAVT_UTF8_CHECK_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/dev/check_utf8.ps1")
if(WIN32 AND SAVT_POWERSHELL_EXECUTABLE AND EXISTS "${SAVT_UTF8_CHECK_SCRIPT}")
    add_custom_target(savt_check_utf8
        COMMAND "${SAVT_POWERSHELL_EXECUTABLE}"
                -NoProfile
                -ExecutionPolicy Bypass
                -File "${SAVT_UTF8_CHECK_SCRIPT}"
                -Root "${CMAKE_SOURCE_DIR}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Checking repository text files are valid UTF-8"
        VERBATIM
    )
endif()

option(SAVT_BUILD_TESTS "Build tests" OFF)
if(SAVT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
