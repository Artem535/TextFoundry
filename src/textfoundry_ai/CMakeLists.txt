add_library(textfoundry_ai_openai STATIC
    openai_compatible_block_generator.cc
    openai_compatible_block_generator.h
)

target_include_directories(textfoundry_ai_openai
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(textfoundry_ai_openai
    PUBLIC
        textfoundry_core
)

set_target_properties(textfoundry_ai_openai PROPERTIES EXPORT_NAME AiOpenAI)

install(
    TARGETS textfoundry_ai_openai
    EXPORT TextFoundryTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/openai_compatible_block_generator.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/textfoundry_ai
)
