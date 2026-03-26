add_library(savt_ai STATIC
    include/savt/ai/DeepSeekClient.h
    src/DeepSeekClient.cpp
)

target_include_directories(savt_ai
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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
