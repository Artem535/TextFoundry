nanobind_add_module(textfoundry module.cc)

target_link_libraries(textfoundry PRIVATE
    textfoundry_core
    textfoundry_ai_openai
)

set_target_properties(textfoundry PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python"
)
