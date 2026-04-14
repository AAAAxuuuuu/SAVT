add_library(savt_ui STATIC
    include/savt/ui/AnalysisController.h
    include/savt/ui/AiService.h
    include/savt/ui/AstPreviewService.h
    include/savt/ui/AnalysisOrchestrator.h
    include/savt/ui/AnalysisAnnotationService.h
    include/savt/ui/FileInsightService.h
    include/savt/ui/IncrementalAnalysisPipeline.h
    include/savt/ui/ReadingGuideService.h
    include/savt/ui/ReportService.h
    include/savt/ui/SemanticReadinessService.h
    include/savt/ui/SceneMapper.h
    include/savt/ui/WorkspaceProjectionBuilder.h
    src/AiService.cpp
    src/AstPreviewService.cpp
    src/AnalysisController.cpp
    src/IncrementalAnalysisPipeline.cpp
    src/AnalysisOrchestrator.cpp
    src/AnalysisAnnotationService.cpp
    src/FileInsightService.cpp
    src/ReadingGuideService.cpp
    src/ReportService.cpp
    src/SemanticReadinessService.cpp
    src/SceneMapper.cpp
    src/WorkspaceProjectionBuilder.cpp
)

target_include_directories(savt_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(savt_ui
    PUBLIC
        Qt6::Concurrent
        Qt6::Core
        Qt6::Network
        savt_core
        savt_analyzer
        savt_ai
        savt_layout
        savt_ui_formatters
    PRIVATE
        savt_parser
)

set_target_properties(savt_ui PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER src
)

add_library(savt_ui_formatters STATIC
    include/savt/ui/AnalysisTextFormatter.h
    src/AnalysisTextFormatter.cpp
)

target_include_directories(savt_ui_formatters
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(savt_ui_formatters
    PUBLIC
        Qt6::Core
        savt_core
)

set_target_properties(savt_ui_formatters PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    FOLDER src
)

