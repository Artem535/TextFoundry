find_package(doctest CONFIG REQUIRED)

add_executable(core_tests
        test_main.cc
)

target_link_libraries(core_tests textfoundry_core doctest::doctest)
target_link_libraries(core_tests textfoundry_ai_openai)

add_test(NAME core_tests COMMAND core_tests)
