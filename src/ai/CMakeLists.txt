set(SAVT_AI_BUILTIN_ENABLED OFF CACHE BOOL "Compile a default AI configuration into the application")
set(SAVT_AI_BUILTIN_PROVIDER "deepseek" CACHE STRING "Built-in AI provider label")
set(SAVT_AI_BUILTIN_BASE_URL "" CACHE STRING "Built-in AI base URL")
set(SAVT_AI_BUILTIN_ENDPOINT_PATH "" CACHE STRING "Built-in AI endpoint path override")
set(SAVT_AI_BUILTIN_MODEL "" CACHE STRING "Built-in AI model name")
set(SAVT_AI_BUILTIN_API_KEY "" CACHE STRING "Built-in AI API key")
set(SAVT_AI_BUILTIN_TIMEOUT_MS 30000 CACHE STRING "Built-in AI timeout in milliseconds")

set(SAVT_AI_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/DeepSeekBuildConfig.h.in"
    "${SAVT_AI_GENERATED_INCLUDE_DIR}/DeepSeekBuildConfig.h"
    @ONLY
)

add_library(savt_ai STATIC
    include/savt/ai/DeepSeekClient.h
    src/DeepSeekClient.cpp
)

target_include_directories(savt_ai
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${SAVT_AI_GENERATED_INCLUDE_DIR}
)

target_link_libraries(savt_ai
    PUBLIC
        Qt6::Core
        Qt6::Network
)

set_target_properties(savt_ai PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER src
)
