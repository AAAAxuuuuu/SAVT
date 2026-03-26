add_executable(savt_backend_tests
    cpp/c4_backend_tests.cpp
)

target_include_directories(savt_backend_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/analyzer/src
)

target_link_libraries(savt_backend_tests
    PRIVATE
        savt_core
        savt_analyzer
        savt_layout
)

set_target_properties(savt_backend_tests PROPERTIES
    FOLDER tests
)

add_test(NAME savt_backend_tests COMMAND $<TARGET_FILE:savt_backend_tests>)

add_executable(savt_snapshot_tests
    cpp/snapshot_tests.cpp
)

target_link_libraries(savt_snapshot_tests
    PRIVATE
        savt_core
        savt_analyzer
        savt_layout
        savt_ui_formatters
)

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" SAVT_SNAPSHOT_TESTS_SOURCE_DIR)
file(TO_CMAKE_PATH "${CMAKE_BINARY_DIR}/snapshot_artifacts" SAVT_SNAPSHOT_ARTIFACT_DIR)
target_compile_definitions(savt_snapshot_tests
    PRIVATE
        "SAVT_SNAPSHOT_TESTS_SOURCE_DIR=${SAVT_SNAPSHOT_TESTS_SOURCE_DIR}"
        "SAVT_SNAPSHOT_ARTIFACT_DIR=${SAVT_SNAPSHOT_ARTIFACT_DIR}"
)
if(SAVT_LLVM_RUNTIME_BIN_DIR)
    file(TO_CMAKE_PATH "${SAVT_LLVM_RUNTIME_BIN_DIR}" SAVT_SNAPSHOT_LLVM_BIN_DIR)
    target_compile_definitions(savt_snapshot_tests
        PRIVATE
            "SAVT_SNAPSHOT_LLVM_BIN_DIR=${SAVT_SNAPSHOT_LLVM_BIN_DIR}"
    )
endif()

set_target_properties(savt_snapshot_tests PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER tests
)

add_test(NAME savt_snapshot_tests COMMAND $<TARGET_FILE:savt_snapshot_tests>)

add_executable(savt_ai_tests
    cpp/ai_backend_tests.cpp
)

target_link_libraries(savt_ai_tests
    PRIVATE
        Qt6::Core
        Qt6::Network
        savt_ai
)

set_target_properties(savt_ai_tests PROPERTIES
    FOLDER tests
)

add_test(NAME savt_ai_tests COMMAND $<TARGET_FILE:savt_ai_tests>)

if(WIN32 AND SAVT_POWERSHELL_EXECUTABLE AND EXISTS "${CMAKE_SOURCE_DIR}/scripts/dev/check_utf8.ps1")
    add_test(NAME savt_utf8_check
        COMMAND "${SAVT_POWERSHELL_EXECUTABLE}"
                -NoProfile
                -ExecutionPolicy Bypass
                -File "${CMAKE_SOURCE_DIR}/scripts/dev/check_utf8.ps1"
                -Root "${CMAKE_SOURCE_DIR}"
    )
endif()

get_filename_component(SAVT_TEST_RUNTIME_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_target_property(SAVT_QT_CORE_DLL Qt6::Core IMPORTED_LOCATION_DEBUG)
if(NOT SAVT_QT_CORE_DLL)
    get_target_property(SAVT_QT_CORE_DLL Qt6::Core IMPORTED_LOCATION_RELEASE)
endif()
if(NOT SAVT_QT_CORE_DLL)
    get_target_property(SAVT_QT_CORE_DLL Qt6::Core IMPORTED_LOCATION)
endif()
get_filename_component(SAVT_QT_RUNTIME_BIN_DIR "${SAVT_QT_CORE_DLL}" DIRECTORY)
set(SAVT_TEST_PATH_SEGMENTS "${SAVT_QT_RUNTIME_BIN_DIR};${SAVT_TEST_RUNTIME_BIN_DIR}")
if(SAVT_CLANG_TOOLING_AVAILABLE AND SAVT_LLVM_RUNTIME_BIN_DIR AND EXISTS "${SAVT_LLVM_RUNTIME_BIN_DIR}")
    set(SAVT_TEST_PATH_SEGMENTS "${SAVT_LLVM_RUNTIME_BIN_DIR};${SAVT_TEST_PATH_SEGMENTS}")
endif()
set(SAVT_TEST_PATH_VALUE "${SAVT_TEST_PATH_SEGMENTS};$ENV{PATH}")
string(REPLACE ";" "\\;" SAVT_TEST_PATH_VALUE_ESCAPED "${SAVT_TEST_PATH_VALUE}")
set(SAVT_TEST_PATH_ENV "PATH=${SAVT_TEST_PATH_VALUE_ESCAPED}")
set_tests_properties(savt_backend_tests PROPERTIES
    ENVIRONMENT "${SAVT_TEST_PATH_ENV}"
)
set_tests_properties(savt_snapshot_tests PROPERTIES
    ENVIRONMENT "${SAVT_TEST_PATH_ENV}"
)
set_tests_properties(savt_ai_tests PROPERTIES
    ENVIRONMENT "${SAVT_TEST_PATH_ENV}"
)
if(WIN32 AND SAVT_POWERSHELL_EXECUTABLE AND EXISTS "${CMAKE_SOURCE_DIR}/scripts/dev/check_utf8.ps1")
    set_tests_properties(savt_utf8_check PROPERTIES
        ENVIRONMENT "${SAVT_TEST_PATH_ENV}"
    )
endif()
